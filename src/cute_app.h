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

#ifndef CUTE_H
#define CUTE_H

#include <cute_defines.h>

namespace cute
{

enum app_options_t
{
	APP_OPTIONS_NO_GFX               = 0x001,
	APP_OPTIONS_NO_AUDIO             = 0x002,
	APP_OPTIONS_NO_NET               = 0x004,
	APP_OPTIONS_GFX_GL               = 0x008,
	APP_OPTIONS_GFX_GLES             = 0x010,
	APP_OPTIONS_GFX_D3D9             = 0x020,
	APP_OPTIONS_FULLSCREEN           = 0x040,
	APP_OPTIONS_RESIZABLE            = 0x080,
	APP_OPTIONS_HIDDEN               = 0x100,
	APP_OPTIONS_WINDOW_POS_CENTERED  = 0x200,
};

extern CUTE_API app_t* CUTE_CALL app_make(const char* window_title, int x, int y, int w, int h, uint32_t options = 0, const char* argv0 = NULL, void* user_allocator_context = NULL);
extern CUTE_API void CUTE_CALL app_destroy(app_t* app);

extern CUTE_API int CUTE_CALL is_running(app_t* app);
extern CUTE_API void CUTE_CALL stop_running(app_t* app);
extern CUTE_API void CUTE_CALL app_update(app_t* app, float dt);

}

#include <cute_error.h>
#include <cute_clipboard.h>

#endif // CUTE_H
