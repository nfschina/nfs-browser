// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/plugins/chrome_plugin_service_filter.h"

#include <utility>

#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "content/public/browser/user_metrics.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/engagement/site_engagement_service.h"
#include "chrome/browser/plugins/plugin_finder.h"
#include "chrome/browser/plugins/plugin_metadata.h"
#include "chrome/browser/plugins/plugin_utils.h"
#include "chrome/browser/plugins/plugins_field_trial.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/render_messages.h"
#include "chrome/grit/generated_resources.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/plugin_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/resource_context.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_constants.h"
#include "grit/components_strings.h"
#include "grit/theme_resources.h"
#include "ui/base/l10n/l10n_util.h"

using content::BrowserThread;
using content::PluginService;
using base::UserMetricsAction;

namespace {

// This enum is recorded in a histogram so entries should not be re-ordered or
// removed.
enum PluginGroup {
  GROUP_NAME_UNKNOWN,
  GROUP_NAME_ADOBE_READER,
  GROUP_NAME_JAVA,
  GROUP_NAME_QUICKTIME,
  GROUP_NAME_SHOCKWAVE,
  GROUP_NAME_REALPLAYER,
  GROUP_NAME_SILVERLIGHT,
  GROUP_NAME_WINDOWS_MEDIA_PLAYER,
  GROUP_NAME_GOOGLE_TALK,
  GROUP_NAME_GOOGLE_EARTH,
  GROUP_NAME_COUNT,
};

static const char kLearnMoreUrl[] =
    "https://support.google.com/chrome/answer/6213033";

class ProfileContentSettingObserver : public content_settings::Observer {
 public:
  explicit ProfileContentSettingObserver(Profile* profile)
      : profile_(profile) {}
  ~ProfileContentSettingObserver() override {}
  void OnContentSettingChanged(const ContentSettingsPattern& primary_pattern,
                               const ContentSettingsPattern& secondary_pattern,
                               ContentSettingsType content_type,
                               std::string resource_identifier) override {
    if (content_type == CONTENT_SETTINGS_TYPE_PLUGINS &&
        PluginUtils::ShouldPreferHtmlOverPlugins(
            HostContentSettingsMapFactory::GetForProfile(profile_))) {
      PluginService::GetInstance()->PurgePluginListCache(profile_, false);
    }
  }

 private:
  Profile* profile_;
};

void AuthorizeRenderer(content::RenderFrameHost* render_frame_host) {
  ChromePluginServiceFilter::GetInstance()->AuthorizePlugin(
      render_frame_host->GetProcess()->GetID(), base::FilePath());
}

class NPAPIRemovalInfoBarDelegate : public ConfirmInfoBarDelegate {
 public:
  static void Create(InfoBarService* infobar_service,
                     const base::string16& plugin_name,
                     bool is_removed);

 private:
  NPAPIRemovalInfoBarDelegate(const base::string16& plugin_name,
                              int message_id);
  ~NPAPIRemovalInfoBarDelegate() override;

  // ConfirmInfobarDelegate:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  int GetIconId() const override;
  base::string16 GetMessageText() const override;
  int GetButtons() const override;
  base::string16 GetLinkText() const override;
  GURL GetLinkURL() const override;
  bool LinkClicked(WindowOpenDisposition disposition) override;

  base::string16 plugin_name_;
  int message_id_;
};

// static
void NPAPIRemovalInfoBarDelegate::Create(InfoBarService* infobar_service,
                                         const base::string16& plugin_name,
                                         bool is_removed) {
/*  int message_id = is_removed ? IDS_PLUGINS_NPAPI_REMOVED
                              : IDS_PLUGINS_NPAPI_BEING_REMOVED_SOON;

  infobar_service->AddInfoBar(
      infobar_service->CreateConfirmInfoBar(std::unique_ptr<ConfirmInfoBarDelegate>(
          new NPAPIRemovalInfoBarDelegate(plugin_name, message_id))));*/
}

NPAPIRemovalInfoBarDelegate::NPAPIRemovalInfoBarDelegate(
    const base::string16& plugin_name,
    int message_id)
    : plugin_name_(plugin_name), message_id_(message_id) {
  content::RecordAction(UserMetricsAction("NPAPIRemovalInfobar.Shown"));

  std::pair<PluginGroup, const char*> types[] = {
      std::make_pair(GROUP_NAME_ADOBE_READER,
                     PluginMetadata::kAdobeReaderGroupName),
      std::make_pair(GROUP_NAME_JAVA,
                     PluginMetadata::kJavaGroupName),
      std::make_pair(GROUP_NAME_QUICKTIME,
                     PluginMetadata::kQuickTimeGroupName),
      std::make_pair(GROUP_NAME_SHOCKWAVE,
                     PluginMetadata::kShockwaveGroupName),
      std::make_pair(GROUP_NAME_REALPLAYER,
                     PluginMetadata::kRealPlayerGroupName),
      std::make_pair(GROUP_NAME_SILVERLIGHT,
                     PluginMetadata::kSilverlightGroupName),
      std::make_pair(GROUP_NAME_WINDOWS_MEDIA_PLAYER,
                     PluginMetadata::kWindowsMediaPlayerGroupName),
      std::make_pair(GROUP_NAME_GOOGLE_TALK,
                     PluginMetadata::kGoogleTalkGroupName),
      std::make_pair(GROUP_NAME_GOOGLE_EARTH,
                     PluginMetadata::kGoogleEarthGroupName)};

  PluginGroup group = GROUP_NAME_UNKNOWN;
  std::string name = base::UTF16ToUTF8(plugin_name);

  for (const auto& type : types) {
    if (name == type.second) {
      group = type.first;
      break;
    }
  }

  if (message_id == IDS_PLUGINS_NPAPI_REMOVED) {
    UMA_HISTOGRAM_ENUMERATION(
        "Plugin.NpapiRemovalInfobar.Removed.PluginGroup", group,
        GROUP_NAME_COUNT);
  } else {
    DCHECK_EQ(IDS_PLUGINS_NPAPI_BEING_REMOVED_SOON, message_id);
    UMA_HISTOGRAM_ENUMERATION(
        "Plugin.NpapiRemovalInfobar.RemovedSoon.PluginGroup", group,
        GROUP_NAME_COUNT);
  }
}

NPAPIRemovalInfoBarDelegate::~NPAPIRemovalInfoBarDelegate() {
}

infobars::InfoBarDelegate::InfoBarIdentifier
NPAPIRemovalInfoBarDelegate::GetIdentifier() const {
  return NPAPI_REMOVAL_INFOBAR_DELEGATE;
}

int NPAPIRemovalInfoBarDelegate::GetIconId() const {
  return IDR_INFOBAR_WARNING;
}

base::string16 NPAPIRemovalInfoBarDelegate::GetMessageText() const {
  return l10n_util::GetStringFUTF16(message_id_, plugin_name_);
}

int NPAPIRemovalInfoBarDelegate::GetButtons() const {
  return BUTTON_NONE;
}

base::string16 NPAPIRemovalInfoBarDelegate::GetLinkText() const {
  return l10n_util::GetStringUTF16(IDS_LEARN_MORE);
}

GURL NPAPIRemovalInfoBarDelegate::GetLinkURL() const {
  return GURL(kLearnMoreUrl);
}

bool NPAPIRemovalInfoBarDelegate::LinkClicked(
    WindowOpenDisposition disposition) {
  content::RecordAction(UserMetricsAction("NPAPIRemovalInfobar.LearnMore"));
  ConfirmInfoBarDelegate::LinkClicked(disposition);
  return true;
}

}  // namespace

// ChromePluginServiceFilter inner struct definitions.

struct ChromePluginServiceFilter::ContextInfo {
  ContextInfo(
      const scoped_refptr<PluginPrefs>& plugin_prefs,
      const scoped_refptr<HostContentSettingsMap>& host_content_settings_map,
      Profile* profile);
  ~ContextInfo();

  scoped_refptr<PluginPrefs> plugin_prefs;
  scoped_refptr<HostContentSettingsMap> host_content_settings_map;
  ProfileContentSettingObserver observer;

 private:
  DISALLOW_COPY_AND_ASSIGN(ContextInfo);
};

ChromePluginServiceFilter::ContextInfo::ContextInfo(
    const scoped_refptr<PluginPrefs>& plugin_prefs,
    const scoped_refptr<HostContentSettingsMap>& host_content_settings_map,
    Profile* profile)
    : plugin_prefs(plugin_prefs),
      host_content_settings_map(host_content_settings_map),
      observer(ProfileContentSettingObserver(profile)) {
  host_content_settings_map->AddObserver(&observer);
}

ChromePluginServiceFilter::ContextInfo::~ContextInfo() {
  host_content_settings_map->RemoveObserver(&observer);
}

ChromePluginServiceFilter::OverriddenPlugin::OverriddenPlugin()
    : render_frame_id(MSG_ROUTING_NONE) {}

ChromePluginServiceFilter::OverriddenPlugin::~OverriddenPlugin() {}

ChromePluginServiceFilter::ProcessDetails::ProcessDetails() {}

ChromePluginServiceFilter::ProcessDetails::ProcessDetails(
    const ProcessDetails& other) = default;

ChromePluginServiceFilter::ProcessDetails::~ProcessDetails() {}

// ChromePluginServiceFilter definitions.

// static
const char ChromePluginServiceFilter::kEngagementSettingAllowedHistogram[] =
    "Plugin.Flash.Engagement.ContentSettingAllowed";
const char ChromePluginServiceFilter::kEngagementSettingBlockedHistogram[] =
    "Plugin.Flash.Engagement.ContentSettingBlocked";
const char ChromePluginServiceFilter::kEngagementNoSettingHistogram[] =
    "Plugin.Flash.Engagement.NoSetting";

// static
ChromePluginServiceFilter* ChromePluginServiceFilter::GetInstance() {
  return base::Singleton<ChromePluginServiceFilter>::get();
}

void ChromePluginServiceFilter::RegisterResourceContext(Profile* profile,
                                                        const void* context) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  base::AutoLock lock(lock_);
  resource_context_map_[context] = base::MakeUnique<ContextInfo>(
      PluginPrefs::GetForProfile(profile),
      HostContentSettingsMapFactory::GetForProfile(profile),
      profile);
}

void ChromePluginServiceFilter::UnregisterResourceContext(
    const void* context) {
  base::AutoLock lock(lock_);
  resource_context_map_.erase(context);
}

void ChromePluginServiceFilter::OverridePluginForFrame(
    int render_process_id,
    int render_frame_id,
    const GURL& url,
    const content::WebPluginInfo& plugin) {
  base::AutoLock auto_lock(lock_);
  ProcessDetails* details = GetOrRegisterProcess(render_process_id);
  OverriddenPlugin overridden_plugin;
  overridden_plugin.render_frame_id = render_frame_id;
  overridden_plugin.url = url;
  overridden_plugin.plugin = plugin;
  details->overridden_plugins.push_back(overridden_plugin);
}

void ChromePluginServiceFilter::AuthorizePlugin(
    int render_process_id,
    const base::FilePath& plugin_path) {
  base::AutoLock auto_lock(lock_);
  ProcessDetails* details = GetOrRegisterProcess(render_process_id);
  details->authorized_plugins.insert(plugin_path);
}

void ChromePluginServiceFilter::AuthorizeAllPlugins(
    content::WebContents* web_contents,
    bool load_blocked,
    const std::string& identifier) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  web_contents->ForEachFrame(base::Bind(&AuthorizeRenderer));
  if (load_blocked) {
    web_contents->SendToAllFrames(new ChromeViewMsg_LoadBlockedPlugins(
        MSG_ROUTING_NONE, identifier));
  }
}

void ChromePluginServiceFilter::RestrictPluginToProfileAndOrigin(
    const base::FilePath& plugin_path,
    Profile* profile,
    const GURL& origin) {
  base::AutoLock auto_lock(lock_);
  restricted_plugins_[plugin_path] =
      std::make_pair(PluginPrefs::GetForProfile(profile).get(), origin);
}

void ChromePluginServiceFilter::UnrestrictPlugin(
    const base::FilePath& plugin_path) {
  base::AutoLock auto_lock(lock_);
  restricted_plugins_.erase(plugin_path);
}

bool ChromePluginServiceFilter::IsPluginRestricted(
    const base::FilePath& plugin_path) {
  base::AutoLock auto_lock(lock_);
  return restricted_plugins_.find(plugin_path) != restricted_plugins_.end();
}

bool ChromePluginServiceFilter::IsPluginAvailable(
    int render_process_id,
    int render_frame_id,
    const void* context,
    const GURL& plugin_content_url,
    const url::Origin& main_frame_origin,
    content::WebPluginInfo* plugin) {
  base::AutoLock auto_lock(lock_);
  const ProcessDetails* details = GetProcess(render_process_id);

  // Check whether the plugin is overridden.
  if (details) {
    for (const auto& plugin_override : details->overridden_plugins) {
      if (plugin_override.render_frame_id == render_frame_id &&
          (plugin_override.url.is_empty() ||
           plugin_override.url == plugin_content_url)) {
        bool use = plugin_override.plugin.path == plugin->path;
        if (use)
          *plugin = plugin_override.plugin;
        return use;
      }
    }
  }

  // Check whether the plugin is disabled.
  auto context_info_it = resource_context_map_.find(context);
  // The context might not be found because RenderFrameMessageFilter might
  // outlive the Profile (the context is unregistered during the Profile
  // destructor).
  if (context_info_it == resource_context_map_.end())
    return false;

  const ContextInfo* context_info = context_info_it->second.get();
  if (!context_info->plugin_prefs.get()->IsPluginEnabled(*plugin))
    return false;

  // If PreferHtmlOverPlugins is enabled and the plugin is Flash, we do
  // additional checks.
  if (plugin->name == base::ASCIIToUTF16(content::kFlashPluginName) &&
      PluginUtils::ShouldPreferHtmlOverPlugins(
          context_info->host_content_settings_map.get())) {
    // Check the content setting first, and always respect the ALLOW or BLOCK
    // state. When IsPluginAvailable() is called to check whether a plugin
    // should be advertised, |url| has the same origin as |main_frame_origin|.
    // The intended behavior is that Flash is advertised only if a Flash embed
    // hosted on the same origin as the main frame origin is allowed to run.
    bool is_managed = false;
    HostContentSettingsMap* settings_map =
        context_info_it->second->host_content_settings_map.get();
    ContentSetting flash_setting = PluginUtils::GetFlashPluginContentSetting(
        settings_map, main_frame_origin, plugin_content_url, &is_managed);
    flash_setting = PluginsFieldTrial::EffectiveContentSetting(
        settings_map, CONTENT_SETTINGS_TYPE_PLUGINS, flash_setting);
    double engagement = SiteEngagementService::GetScoreFromSettings(
        settings_map, main_frame_origin.GetURL());

    if (flash_setting == CONTENT_SETTING_ALLOW) {
      UMA_HISTOGRAM_COUNTS_100(kEngagementSettingAllowedHistogram, engagement);
      return true;
    }

    if (flash_setting == CONTENT_SETTING_BLOCK) {
      UMA_HISTOGRAM_COUNTS_100(kEngagementSettingBlockedHistogram, engagement);
      return false;
    }

    UMA_HISTOGRAM_COUNTS_100(kEngagementNoSettingHistogram, engagement);

    // The content setting is neither ALLOW or BLOCK. Check whether the site
    // meets the engagement cutoff for making Flash available without a prompt.
    // This should only happen if the setting isn't being enforced by an
    // enterprise policy.
    if (is_managed ||
        engagement < PluginsFieldTrial::GetSiteEngagementThresholdForFlash()) {
      return false;
    }
  }

  // Check whether the plugin is restricted to a URL.
  RestrictedPluginMap::const_iterator it =
      restricted_plugins_.find(plugin->path);
  if (it != restricted_plugins_.end()) {
     //zhangyq
    // if (it->second.first != context_info)
    if (it->second.first != context_info->plugin_prefs.get()) {
      return false;
    }
  }

  return true;
}

void ChromePluginServiceFilter::NPAPIPluginLoaded(
    int render_process_id,
    int render_frame_id,
    const std::string& mime_type,
    const content::WebPluginInfo& plugin) {
  PluginFinder* finder = PluginFinder::GetInstance();
  std::unique_ptr<PluginMetadata> metadata(finder->GetPluginMetadata(plugin));

  // Singleton will outlive message loop so safe to use base::Unretained here.
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&ChromePluginServiceFilter::ShowNPAPIInfoBar,
                 base::Unretained(this), render_process_id, render_frame_id,
                 metadata->name(), mime_type, false));
}

#if defined(OS_WIN) || defined(OS_MACOSX)
void ChromePluginServiceFilter::NPAPIPluginNotFound(
    int render_process_id,
    int render_frame_id,
    const std::string& mime_type) {
  // Singleton will outlive message loop so safe to use base::Unretained here.
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&ChromePluginServiceFilter::ShowNPAPIInfoBar,
                 base::Unretained(this), render_process_id, render_frame_id,
                 base::string16(), mime_type, true));
}
#endif

bool ChromePluginServiceFilter::CanLoadPlugin(int render_process_id,
                                              const base::FilePath& path) {
  // The browser itself sometimes loads plugins to e.g. clear plugin data.
  // We always grant the browser permission.
  if (!render_process_id)
    return true;

  base::AutoLock auto_lock(lock_);
  const ProcessDetails* details = GetProcess(render_process_id);
  if (!details)
    return false;

  return (ContainsKey(details->authorized_plugins, path) ||
          ContainsKey(details->authorized_plugins, base::FilePath()));
}

void ChromePluginServiceFilter::ShowNPAPIInfoBar(int render_process_id,
                                                 int render_frame_id,
                                                 const base::string16& name,
                                                 const std::string& mime_type,
                                                 bool is_removed) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto ret = infobared_plugin_mime_types_.insert(mime_type);

  // Only display infobar once per mime type.
  if (!ret.second)
    return;

  base::string16 plugin_name(name);

  if (plugin_name.empty()) {
    plugin_name =
        PluginFinder::GetInstance()->FindPluginName(mime_type, "en-US");
  }

  content::RenderFrameHost* render_frame_host =
      content::RenderFrameHost::FromID(render_process_id, render_frame_id);

  content::WebContents* tab =
      content::WebContents::FromRenderFrameHost(render_frame_host);

  // WebContents could have been destroyed between posting and running the task
  // on the UI thread, so explicit check here.
  if (!tab)
    return;

  InfoBarService* infobar_service = InfoBarService::FromWebContents(tab);

  // NPAPI plugins can load inside extensions and if so there is nowhere to
  // display the infobar.
  if (infobar_service)
    NPAPIRemovalInfoBarDelegate::Create(infobar_service, plugin_name,
                                        is_removed);
}

ChromePluginServiceFilter::ChromePluginServiceFilter() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  registrar_.Add(this, content::NOTIFICATION_RENDERER_PROCESS_CLOSED,
                 content::NotificationService::AllSources());
  registrar_.Add(this, chrome::NOTIFICATION_PLUGIN_ENABLE_STATUS_CHANGED,
                 content::NotificationService::AllSources());
}

ChromePluginServiceFilter::~ChromePluginServiceFilter() {}

void ChromePluginServiceFilter::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  switch (type) {
    case content::NOTIFICATION_RENDERER_PROCESS_CLOSED: {
      int render_process_id =
          content::Source<content::RenderProcessHost>(source).ptr()->GetID();

      base::AutoLock auto_lock(lock_);
      plugin_details_.erase(render_process_id);
      break;
    }
    case chrome::NOTIFICATION_PLUGIN_ENABLE_STATUS_CHANGED: {
      Profile* profile = content::Source<Profile>(source).ptr();
      PluginService::GetInstance()->PurgePluginListCache(profile, false);
      if (profile && profile->HasOffTheRecordProfile()) {
        PluginService::GetInstance()->PurgePluginListCache(
            profile->GetOffTheRecordProfile(), false);
      }
      break;
    }
    default: {
      NOTREACHED();
    }
  }
}

ChromePluginServiceFilter::ProcessDetails*
ChromePluginServiceFilter::GetOrRegisterProcess(
    int render_process_id) {
  return &plugin_details_[render_process_id];
}

const ChromePluginServiceFilter::ProcessDetails*
ChromePluginServiceFilter::GetProcess(
    int render_process_id) const {
  std::map<int, ProcessDetails>::const_iterator it =
      plugin_details_.find(render_process_id);
  if (it == plugin_details_.end())
    return NULL;
  return &it->second;
}
