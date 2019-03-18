/*
	Cute Framework
	Copyright (C) 2019 Randy Gaul https://randygaul.net

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

#ifndef CUTE_NET_H
#define CUTE_NET_H

#include <cute_defines.h>

namespace cute
{

struct cute_t;

/*
	mock api

	endpoint
		public key
		ip address

	generate server endpoint
		for player hosted servers
		also for my own use
		also generates decryption key

	load decryption key

	endpoint server_endpoint = load_endpoint("path");
	connection c = connect(server_endpoint)

	// Also want high perf channel for server?
	c.send(msg);
	msg m = c.poll();
*/

namespace internal
{
	int crypto_init(cute_t* cute);
}

}

#endif // CUTE_NET_H
