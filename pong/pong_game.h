// pong_game.h
// 
// Note:	Game objects & features. Top level.
// 			

#ifndef PONG_GAME
#define PONG_GAME

// -------------------------------------------------------------------------- //

#include <cute.h>
#include <pong_global.h>
#include <pong_utils.h>
using namespace cute;

// -------------------------------------------------------------------------- //

// -- GLOBAlS -- //

//@STUB:	constants, enums, global-vars


// -------------------------------------------------------------------------- //

// -- OBJECTS -- //

//@STUB:	structs, fixed arrays, etc.

struct HUD
{
	//@TODO:	HUD display object
};

struct Player
{
	//@TODO:	player state
	int id;
	int lives;
	//
	Input_map inputs;
};

struct Paddle
{
	enum Buff {fast=1, big=2, stricky=3, bar=4};
	enum Debuff {slow=1, small=2, inverted=3, stun=4};
	//
	static float base_accel;
	static float base_max_speed;
	
	int id;				//match to player-ID
	//
	int x;
	int y;
	int width;
	int height;
	//
	float dx;
	float dy;
	float accel;
	float max_speed;
	//
	array<Buff> buffs;			//@CONSIDER:	using a bitfield
	array<Debuff> debuffs; 		//@CONSIDER:	using a bitfield
};

struct Ball
{
	static float base_speed;	// = 1.0f;		//baseline ball speed
	static float speed_mult;	// = 2.0f;		//multiplier (on speed-up)
	
	int x;
	int y;
	float speed;
	v2 dir;
};


// -------------------------------------------------------------------------- //

// -- FUNC SIGS -- //

//@STUB:	function signatures (fwd-eclrations)
//			(unnecessary but can serve as a ToC)


// -------------------------------------------------------------------------- //

// -- FUNC DEFINITIONS -- //

//@STUB:	function definitions


// -------------------------------------------------------------------------- //

#endif	// PONG_GAME
