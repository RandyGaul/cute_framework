/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
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
