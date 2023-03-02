/*
	Cute Framework
	Copyright (C) 2023 Randy Gaul https://randygaul.github.io/

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

#include <cute_symbol.h>

#include <SDL.h>

CF_SharedLibrary* cf_load_shared_library(const char* path)
{
	return SDL_LoadObject(path);
}

void cf_unload_shared_library(CF_SharedLibrary* library)
{
	SDL_UnloadObject(library);
}

void* cf_load_function(CF_SharedLibrary* library, const char* function_name)
{
	return SDL_LoadFunction(library, function_name);
}
