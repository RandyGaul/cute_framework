/*
	cute-sym: the command line face of libraries/cute/cute_sym.h.

	    cute-sym <binary> [--debug <pdb|dSYM>] [-o <out.sym>] [--embed]
	    cute-sym resolve <report.json> [--symbols <dir>]
	    cute-sym print <report.json>
	    cute-sym dump <table.sym | binary with a slot>
*/

#define CUTE_SYM_IMPLEMENTATION
#define CUTE_SYM_MAIN
#include <cute/cute_sym.h>
