/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_H
#define CF_H

#include "cute_aabb_tree.h"
#include "cute_alloc.h"
#include "cute_app.h"
#include "cute_array.h"
#include "cute_audio.h"
#include "cute_base64.h"
#include "cute_circular_buffer.h"
#include "cute_clipboard.h"
#include "cute_color.h"
#include "cute_multithreading.h"
#include "cute_coroutine.h"
#include "cute_defer.h"
#include "cute_doubly_list.h"
#include "cute_draw.h"
#include "cute_ecs.h"
#include "cute_file_system.h"
#include "cute_graphics.h"
#include "cute_haptics.h"
#include "cute_hashtable.h"
#include "cute_https.h"
#include "cute_image.h"
#include "cute_input.h"
#include "cute_joypad.h"
#include "cute_json.h"
#include "cute_math.h"
#include "cute_networking.h"
#include "cute_noise.h"
#include "cute_png_cache.h"
#include "cute_rnd.h"
#include "cute_sprite.h"
#include "cute_string.h"
#include "cute_time.h"
#include "cute_version.h"
#include "cute_routine.h"

#ifndef CF_EMSCRIPTEN
#	include <SDL_main.h>
#endif

#endif // CF_H
