# Install script for directory: /home/libg/Review/browser/browser-2-0/src/third_party/c-ares

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  foreach(file
      "$ENV{DESTDIR}/usr/local/lib/libcares.so.2.2.0"
      "$ENV{DESTDIR}/usr/local/lib/libcares.so.2"
      "$ENV{DESTDIR}/usr/local/lib/libcares.so"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      file(RPATH_CHECK
           FILE "${file}"
           RPATH "")
    endif()
  endforeach()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/lib/libcares.so.2.2.0;/usr/local/lib/libcares.so.2;/usr/local/lib/libcares.so")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/usr/local/lib" TYPE SHARED_LIBRARY FILES
    "/home/libg/Review/browser/browser-2-0/src/third_party/c-ares/lib/libcares.so.2.2.0"
    "/home/libg/Review/browser/browser-2-0/src/third_party/c-ares/lib/libcares.so.2"
    "/home/libg/Review/browser/browser-2-0/src/third_party/c-ares/lib/libcares.so"
    )
  foreach(file
      "$ENV{DESTDIR}/usr/local/lib/libcares.so.2.2.0"
      "$ENV{DESTDIR}/usr/local/lib/libcares.so.2"
      "$ENV{DESTDIR}/usr/local/lib/libcares.so"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "${file}")
      endif()
    endif()
  endforeach()
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/include/ares.h;/usr/local/include/ares_version.h;/usr/local/include/ares_dns.h;/usr/local/include/ares_build.h;/usr/local/include/ares_rules.h")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/usr/local/include" TYPE FILE FILES
    "/home/libg/Review/browser/browser-2-0/src/third_party/c-ares/ares.h"
    "/home/libg/Review/browser/browser-2-0/src/third_party/c-ares/ares_version.h"
    "/home/libg/Review/browser/browser-2-0/src/third_party/c-ares/ares_dns.h"
    "/home/libg/Review/browser/browser-2-0/src/third_party/c-ares/ares_build.h"
    "/home/libg/Review/browser/browser-2-0/src/third_party/c-ares/ares_rules.h"
    )
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  if(EXISTS "$ENV{DESTDIR}/usr/local/bin/ahost" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/local/bin/ahost")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}/usr/local/bin/ahost"
         RPATH "")
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/bin/ahost")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/usr/local/bin" TYPE EXECUTABLE FILES "/home/libg/Review/browser/browser-2-0/src/third_party/c-ares/bin/ahost")
  if(EXISTS "$ENV{DESTDIR}/usr/local/bin/ahost" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/local/bin/ahost")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}/usr/local/bin/ahost"
         OLD_RPATH "/home/libg/Review/browser/browser-2-0/src/third_party/c-ares/lib:"
         NEW_RPATH "")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/usr/local/bin/ahost")
    endif()
  endif()
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  if(EXISTS "$ENV{DESTDIR}/usr/local/bin/adig" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/local/bin/adig")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}/usr/local/bin/adig"
         RPATH "")
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/bin/adig")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/usr/local/bin" TYPE EXECUTABLE FILES "/home/libg/Review/browser/browser-2-0/src/third_party/c-ares/bin/adig")
  if(EXISTS "$ENV{DESTDIR}/usr/local/bin/adig" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/local/bin/adig")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}/usr/local/bin/adig"
         OLD_RPATH "/home/libg/Review/browser/browser-2-0/src/third_party/c-ares/lib:"
         NEW_RPATH "")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/usr/local/bin/adig")
    endif()
  endif()
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  if(EXISTS "$ENV{DESTDIR}/usr/local/bin/acountry" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/local/bin/acountry")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}/usr/local/bin/acountry"
         RPATH "")
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/bin/acountry")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/usr/local/bin" TYPE EXECUTABLE FILES "/home/libg/Review/browser/browser-2-0/src/third_party/c-ares/bin/acountry")
  if(EXISTS "$ENV{DESTDIR}/usr/local/bin/acountry" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/local/bin/acountry")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}/usr/local/bin/acountry"
         OLD_RPATH "/home/libg/Review/browser/browser-2-0/src/third_party/c-ares/lib:"
         NEW_RPATH "")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/usr/local/bin/acountry")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/home/libg/Review/browser/browser-2-0/src/third_party/c-ares/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
