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

namespace cute
{

template<typename T>
class scope_exit
{
public:
	explicit scope_exit(const T& func) : F(func) { }
	~scope_exit() { F(); }

private:
	T F;
};

template <typename T>
static scope_exit<T> s_create_scope_helper(T func)
{
	return scope_exit<T>(func);
}

#define CUTE_TOKEN_PASTE_HELPER(X, Y) X ## Y
#define CUTE_TOKEN_PASTE(X, Y) CUTE_TOKEN_PASTE_HELPER(X, Y)

/**
 * Runs a single line of code `L` whenever the scope the `CUTE_DEFER` macro resides within
 * exits. This is useful for closing files or cleaning things up when returning from a
 * function, especially when there are many different places to return from.
 *
 * Here is an example.
 *
 * error_code_t read_file(const char* path)
 * {
 *     FILE* fp = fopen(path, "rb");
 *     CUTE_DEFER(fclose(fp));
 * 
 *     // read from file...
 * 
 *     if (error) {
 *         // The defer line will run here!
 *         return error_code(error);
 *     }
 * 
 *     // read more from the file ...
 * 
 *     if (other_error) {
 *         // The defer line will also run here!
 *         return error_code(other_error);
 *     }
 * 
 *     // And finally, the defer line can run here too.
 * }
 */
#define CUTE_DEFER(L) const auto& CUTE_TOKEN_PASTE(scope_exit, __LINE__) = s_create_scope_helper([&]() { L; })

}
