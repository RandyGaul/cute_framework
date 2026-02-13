/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_clipboard.h>
#include <cute_alloc.h>
#include <cute_c_runtime.h>
#include <SDL3/SDL_clipboard.h>

char* cf_clipboard_get()
{
	char* text = SDL_GetClipboardText();
	return text;
}

CF_Result cf_clipboard_set(const char* string)
{
	int ret = SDL_SetClipboardText(string);
	if (ret) return cf_result_error("Unable to set clipboard data.");
	else return cf_result_success();
}

static void* s_clipboard_data = NULL;
static int s_clipboard_data_size = 0;

static const void* s_clipboard_data_callback(void* userdata, const char* mime_type, size_t* size)
{
	*size = (size_t)s_clipboard_data_size;
	return s_clipboard_data;
}

static void s_clipboard_cleanup_callback(void* userdata)
{
	cf_free(s_clipboard_data);
	s_clipboard_data = NULL;
	s_clipboard_data_size = 0;
}

CF_Result cf_clipboard_set_data(const void* data, int size, const char** mime_types, int num_mime_types)
{
	cf_free(s_clipboard_data);
	s_clipboard_data = cf_alloc(size);
	CF_MEMCPY(s_clipboard_data, data, size);
	s_clipboard_data_size = size;
	bool ok = SDL_SetClipboardData(s_clipboard_data_callback, s_clipboard_cleanup_callback, NULL, mime_types, (size_t)num_mime_types);
	if (!ok) return cf_result_error("Unable to set clipboard data.");
	return cf_result_success();
}

void* cf_clipboard_get_data(const char* mime_type, int* size)
{
	size_t sdl_size = 0;
	void* sdl_data = SDL_GetClipboardData(mime_type, &sdl_size);
	if (!sdl_data || sdl_size == 0) {
		if (size) *size = 0;
		return NULL;
	}
	void* result = cf_alloc((size_t)sdl_size);
	CF_MEMCPY(result, sdl_data, sdl_size);
	SDL_free(sdl_data);
	if (size) *size = (int)sdl_size;
	return result;
}

bool cf_clipboard_has_data(const char* mime_type)
{
	return SDL_HasClipboardData(mime_type);
}
