// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (c) 2016-2018 CPU and Fundamental Software Research Center, Chinese Academy of Sciences.

#include "components/omnibox/browser/search_bar_suggest_provider.h"

#include <algorithm>
#include <cmath>

#include "base/auto_reset.h"
#include "base/callback.h"
#include "base/i18n/break_iterator.h"
#include "base/i18n/case_conversion.h"
#include "base/i18n/icu_string_conversions.h"
#include "base/json/json_string_value_serializer.h"
#include "base/message_loop/message_loop.h"
#include "base/metrics/histogram.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/omnibox/browser/autocomplete_classifier.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/search_engine_type.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "components/search_engines/template_url_service.h"
#include "components/omnibox/browser/autocomplete_result.h"
#include "components/omnibox/browser/keyword_provider.h"
#include "components/omnibox/browser/url_prefix.h"
#include "components/omnibox/browser/omnibox_field_trial.h"
#include "url/url_util.h"
#include "net/base/escape.h"
#include "net/base/load_flags.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_status.h"

using base::Time;
using base::TimeDelta;

const size_t SearchSuggestProvider::kSuggestMaxMatches = 10 ;

// SearchSuggestProvider::Providers --------------------------------------------------

SearchSuggestProvider::Providers::Providers(TemplateURLService* template_url_service)
    : template_url_service_(template_url_service) {
}

const TemplateURL* SearchSuggestProvider::Providers::GetDefaultProviderURL() const {
  return template_url_service_->GetTemplateURLForKeyword(base::ASCIIToUTF16("baidu.com"));
}

// SearchSuggestProvider -------------------------------------------------------------

// static
const int SearchSuggestProvider::kDefaultProviderURLFetcherID = 1;

SearchSuggestProvider::SearchSuggestProvider(AutocompleteProviderListener* listener,
    AutocompleteProviderClient* client)
    : AutocompleteProvider(AutocompleteProvider::TYPE_SEARCH_BAR_SUGGEST),
      providers_(client->GetTemplateURLService()),
      suggest_results_pending_(0),
      have_suggest_results_(false),
      instant_finalized_(false) ,
      pre_suggest_done_(false),
      field_trial_triggered_(false),
      field_trial_triggered_in_session_(false), 
      listener_(listener),
      client_(client) {
  if (!providers_.template_url_service()->loaded()) {
    providers_.template_url_service()->Load();
  }
}

SearchSuggestProvider::~SearchSuggestProvider() {

}

void SearchSuggestProvider::FinalizeInstantQuery(const base::string16& input_text,
    const InstantSuggestion& suggestion) {
  if (done_ || instant_finalized_) {
    return;
  }

  instant_finalized_ = true;
  UpdateDone();
  pre_suggest_done_ = true;
  if (input_text.empty()) {
    // We only need to update the listener if we're actually done.
    if (done_) {
      listener_->OnProviderUpdate(false);
    }
    return;
  }

  default_provider_suggestion_ = suggestion;

  base::string16 adjusted_input_text(input_text);
  // AutocompleteInput::RemoveForcedQueryStringIfNecessary(input_.type(), &adjusted_input_text);

  const base::string16 text = adjusted_input_text + suggestion.text;
  bool results_updated = false;
  // Remove any matches that are identical to |text|. We don't use the
  // destination_url for comparison as it varies depending upon the index passed
  // to TemplateURL::ReplaceSearchTerms.
  for (ACMatches::iterator i = matches_.begin(); i != matches_.end();) {
    if (((i->type == AutocompleteMatchType::SEARCH_HISTORY) ||
        (i->type == AutocompleteMatchType::SEARCH_SUGGEST)) &&
        (i->fill_into_edit == text)) {
      i = matches_.erase(i);
      results_updated = true;
    } else {
      ++i;
    }
  }
  if (results_updated || done_) {
    listener_->OnProviderUpdate(results_updated);
  }
}

void SearchSuggestProvider::Start(const AutocompleteInput& input, bool minimal_changes) {
  matches_.clear();
  instant_finalized_ = input.want_asynchronous_matches();

  TemplateURLService* model = providers_.template_url_service();
  DCHECK(model);
  model->Load();
  const TemplateURL* default_provider = model->GetDefaultSearchProvider();
  if (default_provider && !default_provider->SupportsReplacement(model->search_terms_data())) {
    default_provider = NULL;
  }

  if (!default_provider) {
    // No valid providers.
    Stop(false, false);
    return;
  }

  // If we're still running an old query but have since changed the query text
  // or the providers, abort the query.
  base::string16 default_provider_keyword(default_provider ?
    default_provider->keyword() : base::string16());
  if (!minimal_changes) {
    if (done_) {
      default_provider_suggestion_ = InstantSuggestion();
    } else {
      Stop(false, false);
    }
  }

  providers_.set(default_provider_keyword);
  if (input.text().empty()) {
    // User typed "?" alone.  Give them a placeholder result indicating what
    // this syntax does.
    if (default_provider) {
      AutocompleteMatch match;
      match.provider = this;
      match.contents_class.push_back(ACMatchClassification(0, ACMatchClassification::NONE));
      match.keyword = providers_.default_provider();
      matches_.push_back(match);
    }
    Stop(false, false);
    return;
  }
  input_ = input;
  StartOrStopSuggestQuery(minimal_changes);
  ConvertResultsToAutocompleteMatches();
}

SearchSuggestProvider::Result::Result(int relevance) : relevance_(relevance) {}
SearchSuggestProvider::Result::~Result() {}

SearchSuggestProvider::SuggestResult::SuggestResult(const base::string16& suggestion,
    int relevance)
    : Result(relevance),
      suggestion_(suggestion) {
}

SearchSuggestProvider::SuggestResult::~SuggestResult() {}

void SearchSuggestProvider::Run() {
  // Start a new request with the current input.
  pre_suggest_done_ = true;
  suggest_results_pending_ = 0;
  time_suggest_request_sent_ = base::TimeTicks::Now();

  default_fetcher_ = CreateSuggestFetcher(kDefaultProviderURLFetcherID,
      providers_.GetDefaultProviderURL(), input_);

  // Both the above can fail if the providers have been modified or deleted
  // since the query began.
  if (suggest_results_pending_ == 0) {
    UpdateDone();
    // We only need to update the listener if we're actually done.
    if (done_) {
      listener_->OnProviderUpdate(false);
    }
  }  
}

void SearchSuggestProvider::Stop(bool clear_cached_results, bool due_to_user_inactivity) {
  StopSuggest();
  done_ = true;
  default_provider_suggestion_ = InstantSuggestion();

  if (clear_cached_results) {
    ClearResults();
  }
}

void SearchSuggestProvider::AddProviderInfo(ProvidersInfo* provider_info) const {
  provider_info->push_back(metrics::OmniboxEventProto_ProviderInfo());
  metrics::OmniboxEventProto_ProviderInfo& new_entry = provider_info->back();
  new_entry.set_provider(AsOmniboxEventProviderType());
  new_entry.set_provider_done(done_);
}

void SearchSuggestProvider::ResetSession() {
  field_trial_triggered_in_session_ = false;
}

void SearchSuggestProvider::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK(!done_);
  suggest_results_pending_--;

  DCHECK_GE(suggest_results_pending_, 0);  // Should never go negative.
  if (!pre_suggest_done_){
    pre_suggest_done_ = true;
    return;
  }
  const net::HttpResponseHeaders* const response_headers = source->GetResponseHeaders();
  std::string json_data;
  source->GetResponseAsString(&json_data);
  // JSON is supposed to be UTF-8, but some suggest service providers send JSON
  // files in non-UTF-8 encodings.  The actual encoding is usually specified in
  // the Content-Type header field.
  if (response_headers) {
    std::string charset;
    if (response_headers->GetCharset(&charset)) {
      base::string16 data_16;
      // TODO(jungshik): Switch to CodePageToUTF8 after it's added.
      if (base::CodepageToUTF16(json_data, charset.c_str(), base::OnStringConversionError::FAIL, &data_16)) {
        json_data = base::UTF16ToUTF8(data_16);
      }
    }
  }
  const bool request_succeeded = source->GetStatus().is_success() && source->GetResponseCode() == 200;
  bool results_updated = false;
  if (request_succeeded) {
    JSONStringValueDeserializer deserializer(json_data);
    deserializer.set_allow_trailing_comma(true);
    std::unique_ptr<base::Value> data(deserializer.Deserialize(NULL, NULL));
    results_updated = data.get() && ParseSuggestResults(data.get());
  }

  default_fetcher_.reset();
  ConvertResultsToAutocompleteMatches();
  if (done_ || results_updated) {
    listener_->OnProviderUpdate(results_updated);
  }
}

void SearchSuggestProvider::StartOrStopSuggestQuery(bool minimal_changes) {
  if (!IsQuerySuitableForSuggest()) {
    StopSuggest();
    ClearResults();
    return;
  }

  // For the minimal_changes case, if we finished the previous query and still
  // have its results, or are allowed to keep running it, just do that, rather
  // than starting a new query.
  if (minimal_changes && (have_suggest_results_ || (!done_ && input_.want_asynchronous_matches()))) {
    return;
  }

  // We can't keep running any previous query, so halt it.
  StopSuggest();

  // Remove existing results that cannot inline autocomplete the new input.
  RemoveStaleResults();

  // We can't start a new query if we're only allowed synchronous results.
  if (!input_.want_asynchronous_matches()) {
    return;
  }

  // To avoid flooding the suggest server, don't send a query until at least 100
  // ms since the last query.
  const int kMinimumTimeBetweenSuggestQueriesMs = 100;
  base::TimeTicks next_suggest_time(time_suggest_request_sent_ +
  TimeDelta::FromMilliseconds(kMinimumTimeBetweenSuggestQueriesMs));
  base::TimeTicks now(base::TimeTicks::Now());
  timer_.Start(FROM_HERE, std::max(TimeDelta(), next_suggest_time - now), this, &SearchSuggestProvider::Run);
}

bool SearchSuggestProvider::IsQuerySuitableForSuggest() const {
  // Don't run Suggest in incognito mode, if the engine doesn't support it, or
  // if the user has disabled it.
  const TemplateURL* default_url = providers_.GetDefaultProviderURL();
  return !(((!default_url || default_url->suggestions_url().empty())));
}

void SearchSuggestProvider::StopSuggest() {
  // Increment the appropriate field in the histogram by the number of
  // pending requests that were invalidated.
  suggest_results_pending_ = 0;
  pre_suggest_done_ = true;
  timer_.Stop();
  default_fetcher_.reset();
}

void SearchSuggestProvider::ClearResults() {
  default_suggest_results_.clear();
  have_suggest_results_ = false;
}

void SearchSuggestProvider::RemoveStaleResults() {
  RemoveStaleSuggestResults(&default_suggest_results_,input_.text());
}

void SearchSuggestProvider::RemoveStaleSuggestResults(SuggestResults* list, const base::string16& input) {
  for (SuggestResults::iterator i = list->begin(); i < list->end();) {
    i = StartsWith(i->suggestion(), input, base::CompareCase::INSENSITIVE_ASCII) ? (i + 1) : list->erase(i);
  }
}

std::unique_ptr<net::URLFetcher> SearchSuggestProvider::CreateSuggestFetcher(
    int id,
    const TemplateURL* template_url,
    const AutocompleteInput& input) {
  if (!template_url || template_url->suggestions_url().empty()) {
    return NULL;
  }

  // Bail if the suggestion URL is invalid with the given replacements.

  GURL suggest_url(template_url->suggestions_url_ref().ReplaceSearchTerms(
      TemplateURLRef::SearchTermsArgs(input.text()), providers_.template_url_service()->search_terms_data()));
  if (!suggest_url.is_valid()) {
    return NULL;
  }    
  suggest_results_pending_++;
  std::unique_ptr<net::URLFetcher> fetcher = net::URLFetcher::Create(suggest_url, net::URLFetcher::GET, this);
  fetcher->SetRequestContext(client_->GetRequestContext());
  fetcher->SetLoadFlags(net::LOAD_DO_NOT_SAVE_COOKIES);
  fetcher->Start();
  return fetcher;
}

bool SearchSuggestProvider::ParseSuggestResults(base::Value* root_val) {
  // TODO(pkasting): Fix |have_suggest_results_|; see http://crbug.com/130631
  have_suggest_results_ = false;

  base::string16 query;
  base::ListValue* root_list = NULL;
  base::ListValue* results = NULL;
  if (!root_val->GetAsList(&root_list) || !root_list->GetString(0, &query) ||
    !root_list->GetList(1, &results)) {
    return false;
  }

  // 5th element: Optional key-value pairs from the Suggest server.
  base::ListValue* types = NULL;
  SuggestResults* suggest_results =  &default_suggest_results_;

  // Clear the previous results now that new results are available.
  suggest_results->clear();

  base::string16 result, title;
  std::string type;
  int relevance = 500;

  if (!query.empty()) {
    suggest_results->push_back(SuggestResult(query, relevance));    
  }

  for (size_t index = 0; results->GetString(index, &result); ++index) {
    // Baidu search may return empty suggestions for weird input characters,
    // they make no sense at all and can cause problems in our code.
     base::string16::size_type pos = result.find(input_.text()) ;
    if (result.empty()  || pos == base::string16::npos) {
      continue;
    }
    if (!(types && types->GetString(index, &type) && (type == "NAVIGATION"))) {
       suggest_results->push_back(SuggestResult(result, relevance));
    }
  }
  have_suggest_results_ = true;
  return true;
}

void SearchSuggestProvider::ConvertResultsToAutocompleteMatches() {
  // Convert all the results to matches and add them to a map, so we can keep
  // the most relevant match for each result.
  matches_.clear();
  AddSuggestResultsToMatch(default_suggest_results_, matches_);
  const size_t max_total_matches = kSuggestMaxMatches;

  if (matches_.size() > max_total_matches)
    matches_.erase(matches_.begin() + max_total_matches, matches_.end());
  UpdateDone();
}


void SearchSuggestProvider::UpdateMatches() {
  ConvertResultsToAutocompleteMatches();
}

void SearchSuggestProvider::AddSuggestResultsToMatch(const SuggestResults& results, ACMatches& matches) {
    const base::string16& input_text = input_.text();
    for (size_t i = 0; i < results.size(); ++i) {
      AddMatchToMatch(results[i].suggestion(), input_text, results[i].relevance(),
        AutocompleteMatchType::SEARCH_SUGGEST, i , matches);
    }
}

int SearchSuggestProvider::CalculateRelevanceForSuggestion(bool for_keyword) const {
  return 500 ;
}

int SearchSuggestProvider::
  CalculateRelevanceForVerbatimIgnoringKeywordModeState() const {
    switch (input_.type()) {
    case metrics::OmniboxInputType::UNKNOWN:
    case metrics::OmniboxInputType::QUERY:
      return 1300;

    case metrics::OmniboxInputType::URL:
      return 850;

    default:
      NOTREACHED();
      return 0;
    }
}

void SearchSuggestProvider::AddMatchToMatch(const base::string16& query_string,
    const base::string16& input_text,
    int relevance,
    AutocompleteMatch::Type type,
    int accepted_suggestion,
    ACMatches& matches) {
  AutocompleteMatch match(this, relevance, false, type);
  std::vector<size_t> content_param_offsets;
  // Bail out now if we don't actually have a valid provider.
  match.keyword = providers_.default_provider();
  const TemplateURL* provider_url = match.GetTemplateURL(providers_.template_url_service(), false);
  if (provider_url == NULL)
    return;

  match.contents.assign(query_string);
  // We do intra-string highlighting for suggestions - the suggested segment
  // will be highlighted, e.g. for input_text = "you" the suggestion may be
  // "youtube", so we'll bold the "tube" section: you*tube*.
  if (input_text != query_string) {
    size_t input_position = match.contents.find(input_text);
    if (input_position == base::string16::npos) {
      // The input text is not a substring of the query string, e.g. input
      // text is "slasdot" and the query string is "slashdot", so we bold the
      // whole thing.
      match.contents_class.push_back(ACMatchClassification(0, ACMatchClassification::MATCH));
    } else {
      // TODO(beng): ACMatchClassification::MATCH now seems to just mean
      //             "bold" this. Consider modifying the terminology.
      // We don't iterate over the string here annotating all matches because
      // it looks odd to have every occurrence of a substring that may be as
      // short as a single character highlighted in a query suggestion result,
      // e.g. for input text "s" and query string "southwest airlines", it
      // looks odd if both the first and last s are highlighted.
      if (input_position != 0) {
        match.contents_class.push_back(ACMatchClassification(0, ACMatchClassification::NONE));
      }
      match.contents_class.push_back(ACMatchClassification(input_position, ACMatchClassification::DIM));
      size_t next_fragment_position = input_position + input_text.length();
      if (next_fragment_position < query_string.length()) {
        match.contents_class.push_back(ACMatchClassification(next_fragment_position, ACMatchClassification::NONE));
      }
    }
  } else {
    // Otherwise, we're dealing with the "default search" result which has no
    // completion.
    match.contents_class.push_back(ACMatchClassification(0, ACMatchClassification::NONE));
  }

  // When the user forced a query, we need to make sure all the fill_into_edit
  // values preserve that property.  Otherwise, if the user starts editing a
  // suggestion, non-Search results will suddenly appear.
  match.fill_into_edit.append(query_string);

  const TemplateURLRef& search_url = provider_url->url_ref();
  DCHECK(search_url.SupportsReplacement(providers_.template_url_service()->search_terms_data()));
  match.search_terms_args.reset(new TemplateURLRef::SearchTermsArgs(query_string));
  match.search_terms_args->original_query = input_text;
  match.search_terms_args->accepted_suggestion = accepted_suggestion;
  // This is the destination URL sans assisted query stats.  This must be set
  // so the AutocompleteController can properly de-dupe; the controller will
  // eventually overwrite it before it reaches the user.
  match.destination_url = GURL(search_url.ReplaceSearchTerms(*match.search_terms_args.get(), 
      providers_.template_url_service()->search_terms_data()));

  // Search results don't look like URLs.
  match.transition = ui::PAGE_TRANSITION_GENERATED;

  matches.push_back(match);
}

void SearchSuggestProvider::UpdateDone() {
  // We're done when the timer isn't running, there are no suggest queries
  // pending, and we're not waiting on instant.
  done_ = !timer_.IsRunning() && (suggest_results_pending_ == 0);
}
