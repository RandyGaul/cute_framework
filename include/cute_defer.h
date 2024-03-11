/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "cute_defines.h"

#ifdef CF_CPP

template<typename T>
class CF_ScopeExit
{
public:
	explicit CF_ScopeExit(const T& func) : F(func) {}
	~CF_ScopeExit() { F(); }

private:
	T F;
};

template <typename T>
static CF_ScopeExit<T> s_create_scope_helper(T func)
{
	return CF_ScopeExit<T>(func);
}

#define CF_TOKEN_PASTE_HELPER(X, Y) X ## Y
#define CF_TOKEN_PASTE(X, Y) CF_TOKEN_PASTE_HELPER(X, Y)

/**
 * Runs a single line of code `L` whenever the scope the `CF_DEFER` macro resides within
 * exits. This is useful for closing files or cleaning things up when returning from a
 * function, especially when there are many different places to return from.
 *
 * Here is an example.
 *
 * error_code_t read_file(const char* path)
 * {
 *     FILE* fp = fopen(path, "rb");
 *     CF_DEFER(fclose(fp));
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
#define CF_DEFER(L) const auto& CF_TOKEN_PASTE(CF_ScopeExit, __LINE__) = s_create_scope_helper([&]() { L; })

#endif // CF_CPP
