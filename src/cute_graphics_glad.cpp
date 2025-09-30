#include <glad/glad.h>
#include <SDL3/SDL_video.h>
#include <stdio.h>

// A separate compilation unit is needed because when glad.h and GLES3/gl3.h
// are included in the same file, there are too many warnings about redefined
// macro

void cf_load_gles()
{
	gladLoadGLES2Loader((GLADloadproc)SDL_GL_GetProcAddress);
	printf("Loaded GLES %d.%d\n", GLVersion.major, GLVersion.minor);
}
