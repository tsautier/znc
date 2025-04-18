/*
Copyright (C) 2004-2025 ZNC, see the NOTICE file for details.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#ifndef ZNC_VERSION_H
#define ZNC_VERSION_H

// Don't use this one
#define VERSION (VERSION_MAJOR + VERSION_MINOR / 10.0)

// CMake: You can add -DVERSION_EXTRA=stuff to cmake!
#ifndef VERSION_EXTRA
#define VERSION_EXTRA ""
#endif
extern const char* ZNC_VERSION_EXTRA;

// Compilation options which affect ABI

#ifdef HAVE_IPV6
#define ZNC_VERSION_TEXT_IPV6 "yes"
#else
#define ZNC_VERSION_TEXT_IPV6 "no"
#endif

#ifdef HAVE_LIBSSL
#define ZNC_VERSION_TEXT_SSL "yes"
#else
#define ZNC_VERSION_TEXT_SSL "no"
#endif

#ifdef HAVE_THREADED_DNS
#define ZNC_VERSION_TEXT_DNS "threads"
#else
#define ZNC_VERSION_TEXT_DNS "blocking"
#endif

#ifdef HAVE_ICU
#define ZNC_VERSION_TEXT_ICU "yes"
#else
#define ZNC_VERSION_TEXT_ICU "no"
#endif

#ifdef HAVE_I18N
#define ZNC_VERSION_TEXT_I18N "yes"
#else
#define ZNC_VERSION_TEXT_I18N "no"
#endif

// This is only here because HASH_DEFAULT has different value
#ifdef ZNC_HAVE_ARGON
#define ZNC_VERSION_TEXT_ARGON "yes"
#else
#define ZNC_VERSION_TEXT_ARGON "no"
#endif

#define ZNC_COMPILE_OPTIONS_STRING                                    \
    "IPv6: " ZNC_VERSION_TEXT_IPV6 ", SSL: " ZNC_VERSION_TEXT_SSL     \
    ", DNS: " ZNC_VERSION_TEXT_DNS ", charset: " ZNC_VERSION_TEXT_ICU \
    ", i18n: " ZNC_VERSION_TEXT_I18N ", Argon2: " ZNC_VERSION_TEXT_ARGON

#endif  // !ZNC_VERSION_H
