/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_result.h>

#include <internal/cute_app_internal.h>
#include <SDL.h>

static int s_message_box_flags(CF_MessageBoxType type)
{
	switch (type)
	{
	case CF_MESSAGE_BOX_TYPE_ERROR: return SDL_MESSAGEBOX_ERROR;
	case CF_MESSAGE_BOX_TYPE_WARNING: return SDL_MESSAGEBOX_WARNING;
	case CF_MESSAGE_BOX_TYPE_INFORMATION: return SDL_MESSAGEBOX_INFORMATION;
	}
	return SDL_MESSAGEBOX_ERROR;
}

void cf_message_box(CF_MessageBoxType type, const char* title, const char* text)
{
	SDL_ShowSimpleMessageBox(s_message_box_flags(type), title, text, app->window);
}
