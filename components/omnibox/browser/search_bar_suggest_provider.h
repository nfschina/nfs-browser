// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file contains the Search autocomplete provider.  This provider is
// responsible for all non-keyword autocomplete entries that start with
// "Search <engine> for ...", including searching for the current input string,
// search history, and search suggestions.  An instance of it gets created and
// managed by the autocomplete controller.

// Copyright (c) 2016-2018 CPU and Fundamental Software Research Center, Chinese Academy of Sciences.

#ifndef COMPONENTS_OMNIBOX_BROWSER_SEARCHBAR_SUGGEST_PROVIDER_H_
#define COMPONENTS_OMNIBOX_BROWSER_SEARCHBAR_SUGGEST_PROVIDER_H_

#include <map>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/timer/timer.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/autocomplete_provider.h"
#include "components/omnibox/browser/autocomplete_provider_listener.h"
#include "components/omnibox/browser/autocomplete_provider_client.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_service.h"
#include "chrome/common/search/instant_types.h"
#include "net/url_request/url_fetcher_delegate.h"

class Profile;
class TemplateURLService;

namespace base {
class Value;
}

namespace net {
class URLFetcher;
}

// Autocomplete provider for searches and suggestions from a search engine.
//
// After construction, the autocomplete controller repeatedly calls Start()
// with some user input, each time expecting to receive a small set of the best
// matches (either synchronously or asynchronously).
//
// Initially the provider creates a match that searches for the current input
// text.  It also starts a task to query the Suggest servers.  When that data
// comes back, the provider creates and returns matches for the best
// suggestions.
class SearchSuggestProvider : public AutocompleteProvider,
                              public net::URLFetcherDelegate {
 public:
  SearchSuggestProvider(AutocompleteProviderListener* listener, AutocompleteProviderClient* client);

  static const size_t kSuggestMaxMatches;

  // Marks the instant query as done. If |input_text| is non-empty this changes
  // the 'search what you typed' results text to |input_text| +
  // |suggestion.text|. |input_text| is the text the user input into the edit.
  // |input_text| differs from |input_.text()| if the input contained
  // whitespace.
  //
  // This method also marks the search provider as no longer needing to wait for
  // the instant result.
  void FinalizeInstantQuery(const base::string16& input_text,
                            const InstantSuggestion& suggestion);
  void ClearInstantSuggestion();

  // AutocompleteProvider:
  void Start(const AutocompleteInput& input,
                     bool minimal_changes) override;
  void Stop(bool clear_cached_results, bool due_to_user_inactivity) override;

  // Adds search-provider-specific information to omnibox event logs.
  void AddProviderInfo(ProvidersInfo* provider_info) const override;

  // Sets |field_trial_triggered_in_session_| to false.
  void ResetSession() override;

  // net::URLFetcherDelegate
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  bool field_trial_triggered_in_session() const {
    return field_trial_triggered_in_session_;
  }

  // ID used in creating URLFetcher for default provider's suggest results.
  static const int kDefaultProviderURLFetcherID;

 private:
  ~SearchSuggestProvider() override;

  class Providers {
   public:
    explicit Providers(TemplateURLService* template_url_service);

    TemplateURLService* template_url_service() { return template_url_service_; }

    // Resets the cached providers.
    void set(const base::string16& default_provider) {default_provider_ = default_provider; }
    const base::string16& default_provider() const { return default_provider_; }

    // NOTE: These may return NULL even if the provider members are nonempty!
    const TemplateURL* GetDefaultProviderURL() const;

   private:
    TemplateURLService* template_url_service_;

    // Cached across the life of a query so we behave consistently even if the
    // user changes their default while the query is running.
    base::string16 default_provider_;

    DISALLOW_COPY_AND_ASSIGN(Providers);
  };

  // The Result classes are intermediate representations of AutocompleteMatches,
  // simply containing relevance-ranked search and navigation suggestions.
  // They may be cached to provide some synchronous matches while requests for
  // new suggestions from updated input are in flight.
  // TODO(msw) Extend these classes to generate their corresponding matches and
  //           other requisite data, in order to consolidate and simplify the
  //           highly fragmented SearchProvider logic for each Result type.
  class Result {
   public:
    explicit Result(int relevance);
    virtual ~Result();

    int relevance() const { return relevance_; }
    void set_relevance(int relevance) { relevance_ = relevance; }

   private:
    // The relevance score.
    int relevance_;
  };

  class SuggestResult : public Result {
   public:
    SuggestResult(const base::string16& suggestion, int relevance);
    ~SuggestResult() override;

    const base::string16& suggestion() const { return suggestion_; }

   private:
    // The search suggestion string.
    base::string16 suggestion_;
  };

  typedef std::vector<SuggestResult> SuggestResults;

  // Called when timer_ expires.
  void Run();

  // Determines whether an asynchronous subcomponent query should run for the
  // current input.  If so, starts it if necessary; otherwise stops it.
  // NOTE: This function does not update |done_|.  Callers must do so.
  void StartOrStopSuggestQuery(bool minimal_changes);

  // Returns true when the current query can be sent to the Suggest service.
  // This will be false e.g. when Suggest is disabled, the query contains
  // potentially private data, etc.
  bool IsQuerySuitableForSuggest() const;

  // Stops the suggest query.
  // NOTE: This does not update |done_|.  Callers must do so.
  void StopSuggest();

  // Clears the current results.
  void ClearResults();

  // Remove results that cannot inline auto-complete the current input.
  void RemoveStaleResults();
  static void RemoveStaleSuggestResults(SuggestResults* list, const base::string16& input);

  // Apply calculated relevance scores to the current results.
  //void ApplyCalculatedRelevance();
  //void ApplyCalculatedSuggestRelevance(SuggestResults* list, bool is_keyword);

  // Starts a new URLFetcher requesting suggest results from |template_url|;
  // callers own the returned URLFetcher, which is NULL for invalid providers.
  std::unique_ptr<net::URLFetcher> CreateSuggestFetcher(int id,
                                        const TemplateURL* template_url,
                                        const AutocompleteInput& input);

  // Parses results from the suggest server and updates the appropriate suggest
  // and navigation result lists, depending on whether |is_keyword| is true.
  // Returns whether the appropriate result list members were updated.
  bool ParseSuggestResults(base::Value* root_val);

  // Converts the parsed results to a set of AutocompleteMatches and adds them
  // to |matches_|.  This also sets |done_| correctly.
  void ConvertResultsToAutocompleteMatches();

  // Updates |matches_| from the latest results; applies calculated relevances
  // if suggested relevances cause undesriable behavior. Updates |done_|.
  void UpdateMatches();
  // Adds matches for |results| to |map|. |is_keyword| indicates whether the
  // results correspond to the keyword provider or default provider.
  void AddSuggestResultsToMatch(const SuggestResults& results, ACMatches& matches);

  // Calculates the relevance for search suggestion results. Set |for_keyword|
  // to true for relevance values applicable to keyword provider results.
  int CalculateRelevanceForSuggestion(bool for_keyword) const;

  // Calculates the relevance score for the verbatim result from the default
  // search engine *ignoring* whether the input is a keyword-based search
  // or not.  This function should only be used to determine the minimum
  // relevance score that the best result from this provider should have.
  // For normal use, prefer the above function.
  int CalculateRelevanceForVerbatimIgnoringKeywordModeState() const;

  // Creates an AutocompleteMatch for "Search <engine> for |query_string|" with
  // the supplied relevance.  Adds this match to |map|; if such a match already
  // exists, whichever one has lower relevance is eliminated.
  void AddMatchToMatch(const base::string16& query_string,
      const base::string16& input_text,
      int relevance,
      AutocompleteMatch::Type type,
      int accepted_suggestion,
      ACMatches& matches);

  // Updates the value of |done_| from the internal state.
  void UpdateDone();

  // Maintains the TemplateURLs used.
  Providers providers_;

  // The user's input.
  AutocompleteInput input_;

  // Number of suggest results that haven't yet arrived. If greater than 0 it
  // indicates one of the URLFetchers is still running.
  int suggest_results_pending_;

  // A timer to start a query to the suggest server after the user has stopped
  // typing for long enough.
  base::OneShotTimer timer_;

  // The time at which we sent a query to the suggest server.
  base::TimeTicks time_suggest_request_sent_;

  std::unique_ptr<net::URLFetcher> default_fetcher_;

  SuggestResults default_suggest_results_;

  // Whether suggest_results_ is valid.
  bool have_suggest_results_;

  // Has FinalizeInstantQuery been invoked since the last |Start|?
  bool instant_finalized_;

  // Has FinalizeInstantQuery been invoked since the last |Start|?
  bool pre_suggest_done_;

  // The |suggestion| parameter passed to FinalizeInstantQuery.
  InstantSuggestion default_provider_suggestion_;


  // Whether a field trial, if any, has triggered in the most recent
  // autocomplete query.  This field is set to false in Start() and may be set
  // to true if either the default provider or keyword provider has completed
  // and their corresponding suggest response contained
  // '"google:fieldtrialtriggered":true'.
  // If the autocomplete query has not returned, this field is set to false.
  bool field_trial_triggered_;

  // Same as above except that it is maintained across the current Omnibox
  // session.
  bool field_trial_triggered_in_session_;

  AutocompleteProviderListener* listener_;

  Profile* profile_;

  AutocompleteProviderClient* client_;

  DISALLOW_COPY_AND_ASSIGN(SearchSuggestProvider);
};

#endif  // COMPONENTS_OMNIBOX_BROWSER_SEARCHBAR_SUGGEST_PROVIDER_H_
