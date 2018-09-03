#include <iostream>
#include <string>

#include "build/build_config.h"

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "components/version_info/version_info.h"

#if defined(OS_WIN)
#include <windows.h>
#include "Shellapi.h"
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

#if defined(OS_WIN)
  typedef std::wstring StringType;
#else
  typedef std::string StringType;
#endif

#if defined(OS_WIN)

void run_exe_on_window(int argc, char** argv) {
  std::string version = version_info::GetVersionNumber();

  StringType exe_name = L"nfs-browser.exe";
  StringType wversion = base::UTF8ToWide(version);
  StringType cmd = wversion + L"\\" + exe_name;

  ShellExecute(NULL, NULL, cmd.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

#else

void run_exe_on_linux(int argc, char** argv) {
  std::string version = version_info::GetVersionNumber();

  StringType exe_name = version + "/nfs-browser";

  base::FilePath path;
  PathService::Get(base::DIR_MODULE, &path);

  base::FilePath exe_path = path.Append(exe_name);
  if (!base::PathExists(exe_path)) {
    LOG(FATAL) << "The nfs-browser is not exist";
  }

  pid_t pid = fork();
  if (0 == pid) {
    if (1 == argc) {
      execl(exe_path.value().c_str(), exe_path.value().c_str(), (char *)0);
    } else {
      char** args = new char* [argc + 1];

      args[0] = new char[exe_path.value().size() + 1];
      strcpy(args[0], exe_path.value().c_str());

      for (int i = 1; i < argc; ++i) {
        args[i] = new char[strlen(argv[i]) + 1];
        strcpy(args[i], argv[i]);
      }
      args[argc] = (char*)0;

      execv(exe_path.value().c_str(), args);

      for (int i = 0; i < argc + 1; ++i) {
        delete[] args[i];
      }
      delete[] args;
    }
  } else if (pid < 0) {
    LOG(FATAL) << "Failed to fork!";
  }
}

#endif

int main(int argc, char** argv) {
  #if defined(OS_WIN)
    run_exe_on_window(argc, argv);
  #else
    run_exe_on_linux(argc, argv);
  #endif

  return 0;
}
