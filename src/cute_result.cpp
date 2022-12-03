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
