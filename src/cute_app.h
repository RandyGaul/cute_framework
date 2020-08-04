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

#ifndef CUTE_APP_H
#define CUTE_APP_H

#include <cute_defines.h>
#include <cute_error.h>

struct ImGuiContext;

namespace cute
{

#define CUTE_APP_OPTIONS_OPENGL_CONTEXT      (1 << 0)
#define CUTE_APP_OPTIONS_OPENG_GL_ES_CONTEXT (1 << 1)
#define CUTE_APP_OPTIONS_FULLSCREEN          (1 << 2)
#define CUTE_APP_OPTIONS_RESIZABLE           (1 << 3)
#define CUTE_APP_OPTIONS_HIDDEN              (1 << 4)
#define CUTE_APP_OPTIONS_WINDOW_POS_CENTERED (1 << 5)

extern CUTE_API app_t* CUTE_CALL app_make(const char* window_title, int x, int y, int w, int h, uint32_t options = 0, const char* argv0 = NULL, void* user_allocator_context = NULL);
extern CUTE_API void CUTE_CALL app_destroy(app_t* app);

extern CUTE_API bool CUTE_CALL app_is_running(app_t* app);
extern CUTE_API void CUTE_CALL app_stop_running(app_t* app);
extern CUTE_API void CUTE_CALL app_update(app_t* app, float dt);

extern CUTE_API error_t CUTE_CALL app_init_net(app_t* app);
extern CUTE_API error_t CUTE_CALL app_init_audio(app_t* app, int max_simultaneous_sounds = 5000);
extern CUTE_API error_t CUTE_CALL app_init_imgui(app_t* app, ImGuiContext** context = NULL);

}

#include <cute_error.h>
#include <cute_clipboard.h>

#endif // CUTE_APP_H
