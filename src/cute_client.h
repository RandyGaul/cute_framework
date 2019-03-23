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

#ifndef CUTE_CLIENT_H
#define CUTE_CLIENT_H

#include <cute_defines.h>

#define CUTE_CLIENT_RECONNECT_SECONDS 5.0f

namespace cute
{

struct client_t;
struct endpoint_t;
struct crypto_key_t;

extern CUTE_API client_t* CUTE_CALL client_alloc(void* user_allocator_context = NULL);
extern CUTE_API void CUTE_CALL client_destroy(client_t* client);

extern CUTE_API int CUTE_CALL client_connect(client_t* client, endpoint_t endpoint, const crypto_key_t* server_public_key);
extern CUTE_API void CUTE_CALL client_disconnect(client_t* client);

enum client_state_t : int
{
	CLIENT_STATE_CONNECTING,
	CLIENT_STATE_CONNECTED,
	CLIENT_STATE_DISCONNECTED,
	CLIENT_STATE_CONNECTION_DENIED,
};

extern CUTE_API client_state_t CUTE_CALL client_state_get(const client_t* client);
extern CUTE_API void CUTE_CALL client_update(client_t* client, float dt);

extern CUTE_API void CUTE_CALL client_get_packet(client_t* client, void* data, int* size);

extern CUTE_API int CUTE_CALL client_send_data(client_t* client, const void* data, int size);
extern CUTE_API int CUTE_CALL client_send_data_unreliable(client_t* client, const void* data, int size);

}

#endif // CUTE_CLIENT_H
