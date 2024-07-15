/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_DEFINES_H
#define CF_DEFINES_H

#include <cute_user_config.h>

#ifndef _CRT_SECURE_NO_WARNINGS
#	define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef _CRT_NONSTDC_NO_DEPRECATE
#	define _CRT_NONSTDC_NO_DEPRECATE
#endif

#if defined(_WIN32)
#	define CF_WINDOWS 1
#elif defined(__linux__) || defined(__unix__) && !defined(__APPLE__) && !defined(__EMSCRIPTEN__)
#	define CF_LINUX 1
#       if !defined(__SSE__)
#               define CUTE_SOUND_SCALAR_MODE
#       endif
#elif defined(__APPLE__)
#	define CF_APPLE 1
#	include <TargetConditionals.h>
#	if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
#		define CF_IOS 1
#	elif TARGET_OS_MAC
#		define CF_MACOS 1
#	else
#		error "Unknown Apple platform"
#	endif
#	ifdef TARGET_CPU_ARM64
#		define CUTE_SOUND_SCALAR_MODE
#	elif TARGET_CPU_X86_64
#	else
#		error "Unknown Apple Architecture"
#	endif
#elif defined(__ANDROID__)
#	define CF_ANDROID 1
#elif defined(__EMSCRIPTEN__)
#	define CF_EMSCRIPTEN 1
#endif

// Vista and later only. This helps MingW builds.
#ifdef CF_WINDOWS
#	include <sdkddkver.h>
#	ifdef _WIN32_WINNT
#		if _WIN32_WINNT < 0x0600
#			undef _WIN32_WINNT
#			define _WIN32_WINNT 0x0600
#		endif
#	endif
#endif

#include <stdint.h>

#ifndef NOMINMAX
#	define NOMINMAX WINDOWS_IS_ANNOYING_AINT_IT
#endif

#ifndef CF_STATIC
#	ifdef _MSC_VER
#		ifdef CF_EXPORT
#			define CF_API __declspec(dllexport)
#		else
#			define CF_API __declspec(dllimport)
#		endif
#	else
#		if ((__GNUC__ >= 4) || defined(__clang__))
#			define CF_API __attribute__((visibility("default")))
#		else
#			define CF_API
#		endif
#	endif
#	define SOKOL_DLL
#else
#	define CF_API
#endif

#ifdef CF_WINDOWS
#	define CF_CALL __cdecl
#else
#	define CF_CALL
#endif

#define CF_UNUSED(x) (void)x
#define CF_ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#ifdef __cplusplus
#	define CF_INLINE inline
#else
#	define CF_INLINE static inline
#endif
#define CF_KB 1024
#define CF_MB (CF_KB * CF_KB)
#define CF_GB (CF_MB * CF_MB)
#define CF_SERIALIZE_CHECK(x) do { if ((x) != SERIALIZE_SUCCESS) goto cute_error; } while (0)
#define CF_STATIC_ASSERT(condition, error_message_string) static_assert(condition, error_message_string)
#define CF_STRINGIZE_INTERNAL(...) #__VA_ARGS__
#define CF_STRINGIZE(...) CF_STRINGIZE_INTERNAL(__VA_ARGS__)
#define CF_OFFSET_OF(T, member) ((size_t)((uintptr_t)(&(((T*)0)->member))))
#define CF_DEBUG_PRINTF(...)
#define CF_ALIGN_TRUNCATE(v, n) ((v) & ~((n) - 1))
#define CF_ALIGN_FORWARD(v, n) CF_ALIGN_TRUNCATE((v) + (n) - 1, (n))
#define CF_ALIGN_TRUNCATE_PTR(p, n) ((void*)CF_ALIGN_TRUNCATE((uintptr_t)(p), n))
#define CF_ALIGN_FORWARD_PTR(p, n) ((void*)CF_ALIGN_FORWARD((uintptr_t)(p), n))
#define CF_GLOBAL

#ifdef __cplusplus
#	ifndef CF_NO_CPP
#		define CF_CPP
#	endif
#endif

#define SOKOL_API_DECL CF_API

#if defined(CF_WINDOWS)
#	define SOKOL_D3D11
#elif defined(CF_LINUX)
#	define SOKOL_GLCORE33
#elif defined(CF_APPLE)
#	define SOKOL_METAL
#elif defined(CF_EMSCRIPTEN)
#	define SOKOL_GLES3
#	include <emscripten.h>
#endif

#define IMGUI_INCLUDE_IMCONFIG_H

#include <stdlib.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifndef CF_NO_WARNINGS
#	define CF_WARN(...) fprintf(stderr, __VA_ARGS__)
#endif

#ifdef CF_CPP
// -------------------------------------------------------------------------------------------------
// Avoid including <utility> header to reduce compile times.

template <typename T>
struct cf_remove_reference
{
	using type = T;
};

template <typename T>
struct cf_remove_reference<T&>
{
	using type = T;
};

template <typename T>
struct cf_remove_reference<T&&>
{
	using type = T;
};

template <typename T>
constexpr typename cf_remove_reference<T>::type&& cf_move(T&& arg) noexcept
{
	return (typename cf_remove_reference<T>::type&&)arg;
}

#include <initializer_list>

template <typename T>
using CF_InitializerList = std::initializer_list<T>;

namespace Cute
{
template <typename T>
using initializer_list = CF_InitializerList<T>;

template <typename T>
using remove_reference = cf_remove_reference<T>;
}

#endif // CF_CPP

// Not sure where to put this... Here is good I guess.
CF_INLINE uint64_t cf_fnv1a(const void* data, int size)
{
	const char* s = (const char*)data;
	uint64_t h = 14695981039346656037ULL;
	char c = 0;
	while (size--) {
		h = h ^ (uint64_t)(*s++);
		h = h * 1099511628211ULL;
	}
	return h;
}

#endif // CF_DEFINES_H
