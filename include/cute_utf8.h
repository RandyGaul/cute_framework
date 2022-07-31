/*
	Cute Framework
	Copyright (C) 2021 Randy Gaul https://randygaul.net

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

#ifndef CUTE_UTF8
#define CUTE_UTF8

#include "cute_defines.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

CUTE_API const char* CUTE_CALL cf_decode8(const char* text, int* cp);
CUTE_API char* CUTE_CALL cf_encode8(char *text, int cp);
CUTE_API int cf_codepoint8_size(int cp);

CUTE_API const wchar_t* CUTE_CALL cf_decode16(const wchar_t* text, int* cp);
CUTE_API wchar_t* CUTE_CALL cf_encode16(wchar_t* text, int cp);
CUTE_API int CUTE_CALL cf_codepoint16_size(int cp);

CUTE_API void CUTE_CALL cf_widen(const char* in, wchar_t* out);
CUTE_API void CUTE_CALL cf_widen2(const char* in, int in_len, wchar_t* out);
CUTE_API void CUTE_CALL cf_widen3(const char* in, wchar_t* out, int out_len);
CUTE_API void CUTE_CALL cf_widen4(const char* in, int in_len, wchar_t* out, int out_len);
CUTE_API void CUTE_CALL cf_shorten(const wchar_t* in, char* out);
CUTE_API void CUTE_CALL cf_shorten2(const wchar_t* in, int in_len, char* out);
CUTE_API void CUTE_CALL cf_shorten3(const wchar_t* in, char* out, int out_len);
CUTE_API void CUTE_CALL cf_shorten4(const wchar_t* in, int in_len, char* out, int out_len);

#ifdef __cplusplus
}
#endif // __cplusplus

#ifdef  CUTE_CPP

namespace cute
{
CUTE_INLINE const char* decode8(const char* text, int* cp) { return cf_decode8(text,cp); }
CUTE_INLINE char* encode8(char* text, int cp) { return cf_encode8(text,cp); }
CUTE_INLINE int codepoint8_size(int cp) { return cf_codepoint8_size(cp); }
CUTE_INLINE const wchar_t* decode16(const wchar_t* text, int* cp) { return cf_decode16(text,cp); }
CUTE_INLINE wchar_t* encode16(wchar_t* text, int cp) { return cf_encode16(text,cp); }
CUTE_INLINE int codepoint16_size(int cp) { return cf_codepoint16_size(cp); }
CUTE_INLINE void widen(const char* in, wchar_t* out) { cf_widen(in,out); }
CUTE_INLINE void widen(const char* in, int in_len, wchar_t* out) { cf_widen2(in,in_len,out); }
CUTE_INLINE void widen(const char* in, wchar_t* out, int out_len) { cf_widen3(in,out,out_len); }
CUTE_INLINE void widen(const char* in, int in_len, wchar_t* out, int out_len) { cf_widen4(in,in_len,out,out_len); }
CUTE_INLINE void shorten(const wchar_t* in, char* out) { cf_shorten(in,out); }
CUTE_INLINE void shorten(const wchar_t* in, int in_len, char* out) { cf_shorten2(in,in_len,out); }
CUTE_INLINE void shorten(const wchar_t* in, char* out, int out_len) { cf_shorten3(in,out,out_len); }
CUTE_INLINE void shorten(const wchar_t* in, int in_len, char* out, int out_len) { cf_shorten4(in,in_len,out,out_len); }
}

#endif //  CUTE_CPP

#endif // CUTE_UTF8
