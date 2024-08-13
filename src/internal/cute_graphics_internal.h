/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_GRAPHICS_INTERNAL_H
#define CF_GRAPHICS_INTERNAL_H

CF_Shader cf_make_draw_shader_internal(const char* path);
void cf_load_internal_shaders();
void cf_unload_shader_compiler();

#endif // CF_GRAPHICS_INTERNAL_H
