/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_IMGUI_INTERNAL_H
#define CF_IMGUI_INTERNAL_H

#include <SDL3/SDL.h>

#include <imgui/backends/imgui_impl_sdlgpu3.h>
#include <SDL3_shadercross/SDL_shadercross.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <imgui/backends/imgui_impl_sdl3.h>

void cf_imgui_init();
void cf_imgui_shutdown();
void cf_imgui_draw();

#endif // CF_IMGUI_INTERNAL_H
