// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/media.h"

#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/metrics/field_trial.h"
#include "base/trace_event/trace_event.h"
#include "media/base/media_switches.h"
#include "media/base/yuv_convert.h"
#include <string>
#include <sys/wait.h>
#include <signal.h>

#if defined(OS_ANDROID)
#include "base/android/build_info.h"
#include "media/base/android/media_codec_util.h"
#endif

#if !defined(MEDIA_DISABLE_FFMPEG)
#include "media/ffmpeg/ffmpeg_common.h"
#endif

namespace media {

struct ConvInfo
{
  std::string pending_video_name;
  std::string pending_ts;
  bool prohibit_concat;
};

std::vector<std::string> ts_vector;
std::map<std::string, std::vector<std::string>> ts_map;
std::map<pid_t, ConvInfo> conv_pid_map;
std::string video_output_dir;
std::string pending_video_name;
std::string pending_ts;
pid_t pending_conv_pid = 0;
pid_t pending_concat_pid = 0;
bool sigaction_changed = false;

// Media must only be initialized once, so use a LazyInstance to ensure this.
class MediaInitializer {
 public:
  void enable_platform_decoder_support() {
    has_platform_decoder_support_ = true;
  }

  bool has_platform_decoder_support() { return has_platform_decoder_support_; }

 private:
  friend struct base::DefaultLazyInstanceTraits<MediaInitializer>;

  MediaInitializer() {
    TRACE_EVENT_WARMUP_CATEGORY("audio");
    TRACE_EVENT_WARMUP_CATEGORY("media");

    // Perform initialization of libraries which require runtime CPU detection.
    InitializeCPUSpecificYUVConversions();

#if !defined(MEDIA_DISABLE_FFMPEG)
    // Initialize CPU flags outside of the sandbox as this may query /proc for
    // details on the current CPU for NEON, VFP, etc optimizations.
    av_get_cpu_flags();

    // Disable logging as it interferes with layout tests.
    av_log_set_level(AV_LOG_QUIET);

#if defined(ALLOCATOR_SHIM)
    // Remove allocation limit from ffmpeg, so calls go down to shim layer.
    av_max_alloc(0);
#endif  // defined(ALLOCATOR_SHIM)

#endif  // !defined(MEDIA_DISABLE_FFMPEG)
  }

  ~MediaInitializer() {
    NOTREACHED() << "MediaInitializer should be leaky!";
  }

  bool has_platform_decoder_support_ = false;

  DISALLOW_COPY_AND_ASSIGN(MediaInitializer);
};

static base::LazyInstance<MediaInitializer>::Leaky g_media_library =
    LAZY_INSTANCE_INITIALIZER;

void InitializeMediaLibrary() {
  g_media_library.Get();
}

bool isNumber(char c) {
    return (('0' <= c && c <= '9') || ('a' <= c && c <= 'f'));
}

std::string getNumber(const char** str) {
  std::string number;
  bool flag = false;
  while (isNumber(**str)) {
    if (!flag && '0' == **str) {
	    (*str)++;
	    continue;
	  }
	  if (!flag) {
	    flag = true;
	  }
    number += **str;
	  (*str)++;
  }
  return number;
}

bool CompareFunction(std::string str1, std::string str2) {
  const char* s1 = str1.c_str();
  const char* s2 = str2.c_str();
  while (*s1 != '\0' && *s2 != '\0') {
    if (!isNumber(*s1)) {
	    if (*s1 == *s2) {
	      s1++;
		    s2++;
		    continue;
	    } else {
	      return *s1 < *s2;
	    }
	  } else {
	    if (!isNumber(*s2)) {
	      return *s1 < *s2;
	    }
	    std::string number1 = getNumber(&s1);
	    std::string number2 = getNumber(&s2);
	    if (number1.length() == number2.length()) {
	      if (number1 == number2) {
		      continue;
		    } else {
		      return number1 < number2;
		    }
	    } else {
	      return number1.length() < number2.length();
	    }
	  }
  }
  return str1 < str2;
}

void concat(std::string video_name) {
  std::string real_name = video_name.substr(video_name.rfind('/')+1);
  std::string output_file = video_output_dir + '/' + real_name + ".mp4";
  pid_t pidConcat = fork();
  if (pidConcat == 0) {
    int priority = getpriority(PRIO_PROCESS, pidConcat);
    int reulst = setpriority(PRIO_PROCESS, pidConcat, 19);
    printf("concat:-------------priority:%d, reulst:%d\n", priority, reulst);
    sort(ts_map[video_name].begin(), ts_map[video_name].end(), CompareFunction);
    if (!output_file.empty()) {
      remove(output_file.c_str());
    }
    printf("ts_map[%s] size:%zu\n", video_name.c_str(), ts_map[video_name].size());
    printf("ts_vector size:%zu\n", ts_vector.size());
    std::string concat_str = "concat:";
    bool get_first_part = false;
    for (size_t i = 0; i < ts_map[video_name].size(); i++) {
      printf("video_name:%s, ts_map[]:%s\n", video_name.c_str(), ts_map[video_name][i].c_str());
      if (!get_first_part) {
        concat_str += ts_map[video_name][i];
        get_first_part = true;
      } else {
        concat_str += "|" + ts_map[video_name][i];
      }
    }
    if (!get_first_part) {
      printf("concat: input is empty, return\n");
	     exit(-1);
    }
    printf("concat_str:%s, output_file:%s\n", concat_str.c_str(), output_file.c_str());
    char * argvConcat[10] = {const_cast<char*>("ffmpeg"),
                              const_cast<char*>("-i"),
                              const_cast<char*>(concat_str.c_str()),
                              const_cast<char*>("-acodec"),
                              const_cast<char*>("copy"),
                              const_cast<char*>("-vcodec"),
                              const_cast<char*>("copy"),
                              const_cast<char*>("-absf"),
                              const_cast<char*>("aac_adtstoasc"),
                              const_cast<char*>(output_file.c_str())};
    run_cmd(10,argvConcat);
  } else if (pidConcat != -1) {
    pending_concat_pid = pidConcat;
  }
}

void sig_child(int signo) {
  pid_t pid;
  int stat;
  if (pending_concat_pid && (pid = waitpid(pending_concat_pid, &stat, WNOHANG)) > 0) {
    printf("child concat_pid:%d terminated.\n", pid);
    pending_concat_pid = 0;
  } else if (!conv_pid_map.empty()) {
    for (const auto& it : conv_pid_map) {
      pid = waitpid(it.first, &stat, WNOHANG);
      printf("wait pid:%d result:%d.\n", it.first, pid);
      if (pid > 0) {
        printf("child conv_pid:%d terminated.\n", pid);
        if (it.second.pending_video_name.empty() || it.second.pending_ts.empty()) {
          printf("convert error------------pid:%d\n", pid);
          conv_pid_map.erase(pid);
          return;
        }
        if (!it.second.prohibit_concat || conv_pid_map.size() == 1) {
          concat(it.second.pending_video_name);
        }
        conv_pid_map.erase(pid);
        break;
      }
    }
  }
}

void DoMediaConcat(std::string input, std::string output_dir, bool prohibit_concat) {
  if (!sigaction_changed) {
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = sig_child;
    sigaction(SIGCHLD, &action, NULL);
    sigaction_changed = true;
  }

  video_output_dir = output_dir;
  std::string path = input.substr(0, input.rfind('/')+1);
  std::string video_name;
  if (input.substr(input.rfind('.')) == ".ts") {
    size_t ts_name_begin = input.rfind('_')+1;
    std::string tmp_name = input.substr(0, input.rfind('.'));
    size_t ts_name_end = tmp_name.rfind('.');
    video_name = input.substr(ts_name_begin, ts_name_end - ts_name_begin);
    printf("input end of .ts-----------------\n");
    std::string full_video_name = path + video_name;
    ts_map[full_video_name].push_back(input);
    ts_vector.push_back(input);
    if (!prohibit_concat) {
      concat(path + video_name);
    }
    return;
  } else if (input.substr(input.rfind('.')) == ".mp4") {
    if (input.rfind('-') != std::string::npos) {
      size_t mp4_name_begin = input.rfind('-')+1;
      size_t mp4_name_end = input.rfind('.');
      video_name = path + input.substr(mp4_name_begin, mp4_name_end - mp4_name_begin);
    } else {
      std::string tmp_name = input.substr(0, input.rfind('.'));
      video_name = tmp_name.substr(0, tmp_name.rfind('.'));
    }
  } else {
    video_name = input.substr(0, input.rfind('_'));
  }
  std::string output_ts = input.substr(0, input.rfind('.')) + ".ts";
  printf("%s, video_name:%s, output_ts:%s\n", __FUNCTION__, video_name.c_str(), output_ts.c_str());
  if (prohibit_concat) {
    printf("prohibit concat----------------output_ts:%s\n",output_ts.c_str());
  }
  pid_t pidConv = fork();
  if (pidConv == 0) {
    int priority = getpriority(PRIO_PROCESS, pidConv);
    int reulst = setpriority(PRIO_PROCESS, pidConv, 19);
    printf("conv:-------------priority:%d, reulst:%d\n", priority, reulst);
    char* argvConv[10] = {const_cast<char*>("ffmpeg"),
                          const_cast<char*>("-i"),
                          const_cast<char*>(input.c_str()),
                          const_cast<char*>("-vcodec"),
                          const_cast<char*>("copy"),
                          const_cast<char*>("-acodec"),
                          const_cast<char*>("copy"),
                          const_cast<char*>("-vbsf"),
                          const_cast<char*>("h264_mp4toannexb"),
                          const_cast<char*>(output_ts.c_str())};
    run_cmd(10,argvConv);
  } else if (pidConv != -1){
    ts_map[video_name].push_back(output_ts);
    ConvInfo info;
    info.pending_video_name = video_name;
    info.pending_ts = output_ts;
    info.prohibit_concat = prohibit_concat;
    conv_pid_map[pidConv] = info;
  }
}

#if defined(OS_ANDROID)
void EnablePlatformDecoderSupport() {
  g_media_library.Pointer()->enable_platform_decoder_support();
}

bool HasPlatformDecoderSupport() {
  return g_media_library.Pointer()->has_platform_decoder_support();
}

bool PlatformHasOpusSupport() {
  return base::android::BuildInfo::GetInstance()->sdk_int() >= 21;
}

bool IsUnifiedMediaPipelineEnabled() {
  return !base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kDisableUnifiedMediaPipeline);
}

bool ArePlatformDecodersAvailable() {
  return IsUnifiedMediaPipelineEnabled()
             ? HasPlatformDecoderSupport()
             : MediaCodecUtil::IsMediaCodecAvailable();
}
#endif

}  // namespace media
