/*
	Cute Framework
	Copyright (C) 2019 Randy Gaul https://randygaul.net

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

#ifndef CUTE_DEFINES_H
#define CUTE_DEFINES_H

#ifndef _CRT_SECURE_NO_WARNINGS
#	define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef _CRT_NONSTDC_NO_DEPRECATE
#	define _CRT_NONSTDC_NO_DEPRECATE
#endif

#if defined(_WIN32)
#	define CUTE_WINDOWS 1
#elif defined(__linux__) || defined(__unix__) && !defined(__APPLE__) && !defined(__EMSCRIPTEN__)
#	define CUTE_LINUX 1
#elif defined(__APPLE__)
#	include <TargetConditionals.h>
#	if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
#		define CUTE_IOS 1
#	elif TARGET_OS_MAC
#		define CUTE_MACOSX 1
#	else
#		error "Unknown Apple platform"
#	endif
#elif defined(__ANDROID__)
#	define CUTE_ANDROID 1
#elif defined(__EMSCRIPTEN__)
#	define CUTE_EMSCRIPTEN 1
#endif

// Vista and later only. This helps MingW builds.
#ifdef CUTE_WINDOWS
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
#	define NOMINMAX
#endif

#ifndef CUTE_STATIC
#	ifdef _MSC_VER
#		ifdef CUTE_EXPORT
#			define CUTE_API __declspec(dllexport)
#		else
#			define CUTE_API __declspec(dllimport)
#		endif
#	else
#		if ((__GNUC__ >= 4) || defined(__clang__))
#			define CUTE_API __attribute__((visibility("default")))
#		else
#			define CUTE_API
#		endif
#	endif
#	define SOKOL_DLL
#else
#	define CUTE_API
#endif

#ifdef CUTE_WINDOWS
#	define CUTE_CALL __cdecl
#else
#	define CUTE_CALL
#endif

#define CUTE_UNUSED(x) (void)x
#define CUTE_ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define CUTE_INLINE inline

#define CUTE_KB 1024
#define CUTE_MB (CUTE_KB * CUTE_KB)
#define CUTE_GB (CUTE_MB * CUTE_MB)

#define CUTE_SERIALIZE_CHECK(x) do { if ((x) != SERIALIZE_SUCCESS) goto cute_error; } while (0)

#define CUTE_STATIC_ASSERT(condition, error_message_string) static_assert(condition, error_message_string)

#define CUTE_STRINGIZE_INTERNAL(...) #__VA_ARGS__
#define CUTE_STRINGIZE(...) CUTE_STRINGIZE_INTERNAL(__VA_ARGS__)

#define CUTE_OFFSET_OF(T, member) ((size_t)((uintptr_t)(&(((T*)0)->member))))

#define CUTE_DEBUG_PRINTF(...)

#define SOKOL_API_DECL CUTE_API

#if defined(CUTE_WINDOWS)
#	define SOKOL_D3D11
#elif defined(CUTE_LINUX)
#	define SOKOL_GLCORE33
#elif defined(CUTE_MACOSX)
#	define SOKOL_GLCORE33
#elif defined(CUTE_EMSCRIPTEN)
#	define SOKOL_GLES3
#	include <emscripten.h>
#endif

#include <stdlib.h> // NULL

namespace cute
{

struct app_t;

}

// -------------------------------------------------------------------------------------------------
// Avoid including <utility> header.

template <class T>
struct remove_reference
{
	using type = T;
};

template <class T>
struct remove_reference<T&>
{
	using type = T;
};

template <class T>
struct remove_reference<T&&>
{
	using type = T;
};

template <class T>
constexpr typename remove_reference<T>::type&& move(T&& arg) noexcept
{
	return (typename remove_reference<T>::type&&)arg;
}

// -------------------------------------------------------------------------------------------------
// Avoid including <initializer_list> header.
// Unfortunately this class *must* be in the std:: namespace or things won't compile. So we try to
// avoid defining this class if someone already included <initializer_list> before including
// cute framework <cute.h>.

#if !defined(_INITIALIZER_LIST_) && !defined(_INITIALIZER_LIST) && !defined(_LIBCPP_INITIALIZER_LIST)
#define _INITIALIZER_LIST_ // MSVC
#define _INITIALIZER_LIST  // GCC
#define _LIBCPP_INITIALIZER_LIST // Clang

namespace std {
template <class t>
class initializer_list {
public:
    using value_type      = t;
    using reference       = const t&;
    using const_reference = const t&;
    using size_type       = size_t;

    using iterator       = const t*;
    using const_iterator = const t*;

    constexpr initializer_list() noexcept
		: m_first(0)
		, m_last(0)
	{
	}

    constexpr initializer_list(const t* first, const t* last) noexcept
        : m_first(first)
		, m_last(last)
	{
	}

    constexpr const t* begin() const noexcept { return m_first; }
    constexpr const t* end() const noexcept { return m_last; }
    constexpr size_t size() const noexcept { return (size_t)(m_last - m_first); }

private:
    const t* m_first;
    const t* m_last;
};

template <class t> constexpr const t* begin(initializer_list<t> list) noexcept { return list.begin(); }
template <class t> constexpr const t* end(initializer_list<t> list) noexcept { return list.end(); }
}

#endif

namespace cute
{
	template <typename T>
	using initializer_list = std::initializer_list<T>;
}

#endif // CUTE_DEFINES_H
