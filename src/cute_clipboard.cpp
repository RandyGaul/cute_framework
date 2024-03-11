/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_clipboard.h>
#include <SDL_clipboard.h>

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
