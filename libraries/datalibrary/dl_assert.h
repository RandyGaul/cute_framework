#ifndef DL_ASSERT_H_INCLUDED
#define DL_ASSERT_H_INCLUDED

// TODO: replace this with a real assert, that can be comipled out!

#include <stdio.h>  // for vsnprintf

#define DL_ASSERT( expr, ... ) do { if(!(expr)) printf("ASSERT FAIL! %s %s %u\n", #expr, __FILE__, __LINE__); } while( false ) // TODO: implement me plox!

#endif // DL_ASSERT_H_INCLUDED
