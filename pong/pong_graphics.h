// pong_graphics.h
// 
// Note:	Graphics features for use in game (not system-level).
// 			QoL UI-layer for sprite handling (syntactic sugar).
// 			Possibly some display & vfx features as well.

#ifndef PONG_GRAPHICS
#define PONG_GRAPHICS

// -------------------------------------------------------------------------- //

#include <cute.h>
#include <pong_global.h>
// #include <pong_utils.h>
using namespace cute;

// -------------------------------------------------------------------------- //

// -- GLOBAlS -- //

//@STUB:	constants, enums, global-vars


// -------------------------------------------------------------------------- //

// -- OBJECTS -- //

//@STUB:	structs, fixed arrays, etc.

struct Spriter
{
	//@TODO:	functionality
	
	aseprite_cache_t* cache;
	batch_t* batch;
	//
	matrix_t projection;
};

// -------------------------------------------------------------------------- //

// -- FUNC SIGS -- //

//@STUB:	function signatures (fwd-eclrations)
//			(unnecessary but can serve as a ToC)

// Spriter sigs
CUTE_INLINE Spriter spriter_make(matrix_t projection);
CUTE_INLINE Spriter spriter_make(int width, int height, int x, int y);


// -------------------------------------------------------------------------- //

// -- FUNC DEFINITIONS -- //

//@STUB:	function definitions

// Spriter defs
Spriter spriter_make(matrix_t projection)
{
	Spriter spr;
	spr.projection = projection;
	spr.cache = aseprite_cache_make(app);	//global (app)
	spr.batch = batch_make(aseprite_cache_get_pixels_fn(spr.cache), spr.cache);
	batch_set_projection(spr.batch, spr.projection);
	
	return spr;
}
Spriter spriter_make(int width = SCREEN_WIDTH, 	//global
					  int height = SCREEN_HEIGHT, 	//global
					  int x = 0, int y = 0			//x,y = center-based
					 )
{
	return spriter_make(matrix_ortho_2d(width, height, x, y));
}


// -------------------------------------------------------------------------- //

#endif	// PONG_GRAPHICS
