/**
    @file pico_unit.h
    @brief A bare-bones unit testing framework written in C99.

    ----------------------------------------------------------------------------
    Licensing information at end of header
    ----------------------------------------------------------------------------

    Features:
    ---------

    * Written in C99 and compatible with C++
    * Single-header for easy integration into any build system
    * Tiny memory and code footprint
    * Simple and minimalistic API
    * All unit tests are run during execution and failures are indicated
    * On demand setup and teardown function support
    * Ability to group tests into test suites
    * Ability to print test statistics
    * Optional color coded output
    * Optional time measurement
    * Permissive licensing (zlib or public domain)

    Summary:
    --------

    This library is a minimal unit testing framework. It should compile and run
    on just about any platform with a standard C99 compiler.

    Writing tests is simple: 1) Use the TEST_CASE macro and define the test using
    using REQUIRE to test boolean expressions; 2) Run the test inside the
    body of a test suite or other function (e.g. main) using RUN_TEST_CASE. How
    you group tests and test suites is entirely up to you.

    In order to keep the library portable and nimble, certain features were
    dispensed with, namely automatic test registration and predicates more
    complex than REQUIRE. Experience has shown these are not serious defects.

    There are a number of display options available: color coded output, test
    elapsed time (unless PICO_UNIT_NO_CLOCK is defined), and printing test statistics.

    A test suite is simply a group of tests. These contain calls to RUN_TEST_CASE.
    The advantage of using test suites is that it divides unit tests into
    logical groups. There are helper macros for declaring and defining test
    suites. Test suites help with formatting the output and the number of suites
    shows up in the test statistics. There is also the option to flexibly define
    setup and teardown functions for groups of tests.

    Please see the examples for more details.

    Usage:
    ------

    To use this library in your project, add the following

    > #define PICO_UNIT_IMPLEMENTATION
    > #include "pico_unit.h"

    to a source file (once), then simply include the header normally.
*/

#ifndef PICO_UNIT_H
#define PICO_UNIT_H

#include <stdbool.h> /* bool, true, false */
#include <stddef.h>  /* NULL */
#include <string.h>  /* strcmp */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Defines a unit test.
 *
 * @param name The name of the test. Must be a valid C function name
 */
#define TEST_CASE(name) static bool name()

/**
 * @brief Asserts that a condition is true

 * Asserts that the given expression evaluates to `true`. If the expression
 * evalutes to `false`, execution of the current test aborts and an error
 * message is displayed.
 *
 * @param expr The expression to evaluate
 */
#define REQUIRE(expr) \
    do  { \
        if (!pu_require((expr) ? true : false, (#expr), __FILE__, __LINE__)) \
            return false; \
    } while(false)

/**
 * @brief Runs a unit test function.
 *
 * IMPORTANT: The function `test_fp` must return `true`. The test function has
 * the signature, `bool test_func(void)`.
 *
 * @param test_fp The test function to execute
 */
#define RUN_TEST_CASE(test_fp) (pu_run_test(#test_fp, test_fp))

/**
 * @brief Declares a test suite
 *
 * @param name The name of the test suite
 */
#define TEST_SUITE(name) void name()

/**
 * @brief Runs a series of unit tests. The test suite function has the signature,
 * `void suite_func(void)`.
 *
 * @param suite_fp The test suite function to run
 */
#define RUN_TEST_SUITE(suite_fp) pu_run_suite(#suite_fp, suite_fp)

/**
 * @brief Functions that are run before or after a number of unit tests execute.
 */
typedef void (*pu_setup_fn)(void);

/**
 * @brief Sets the current setup and teardown functions.
 *
 * Sets the current setup and teardown functions. The setup function is called
 * prior to each unit test and the teardown function after. Either of these
 * functions can be `NULL`. The setup and teardown functions have the signature,
 * `void func(void)`.
 *
 * @param setup_fp The setup function
 * @param teardown_fp The teardown function
 */
void pu_setup(pu_setup_fn setup_fp, pu_setup_fn teardown_fp);

/**
 * @brief Disables the setup and teardown functions by setting them to `NULL`.
 */
void pu_clear_setup(void);

/**
 * @brief Turns on terminal colors. NOTE: Off by default.
 */
void pu_display_colors(bool enabled);

/**
 * @brief Turns on time measurement. NOTE: Off by default.
 */
void pu_display_time(bool enabled);

/**
 * @brief Prints test statistics.
 */
void pu_print_stats(void);

/**
 * @brief Check whether at least one test failed.
 *
 * @return true if any test failed, false if they all passed
 */
bool pu_test_failed(void);

/*
 * WARNING: These functions are not meant to be called directly. Use the macros
 * instead.
 */
typedef bool (*pu_test_fn)(void);
typedef void (*pu_suite_fn)(void);

/**
 * @brief Used internally
 */
bool pu_require(bool passed,
                const char* const expr,
                const char* const file,
                int line);

/**
 * @brief Used internally
 */
void pu_run_test(const char* const name, pu_test_fn test_fp);

/**
 * @brief Used internally
 */
void pu_run_suite(const char* const name, pu_suite_fn suite_fp);

#ifdef __cplusplus
}
#endif

#endif // PICO_UNIT_H

#ifdef PICO_UNIT_IMPLEMENTATION

#include <stdio.h> /* printf */

#ifndef PICO_UNIT_NO_CLOCK
#include <time.h>  /* clock_t, clock */
#endif // PICO_UNIT_NO_CLOCK

#define TERM_COLOR_CODE   0x1B
#define TERM_COLOR_RED   "[1;31m"
#define TERM_COLOR_GREEN "[1;32m"
#define TERM_COLOR_BOLD  "[1m"
#define TERM_COLOR_RESET "[0m"

static unsigned pu_num_asserts  = 0;
static unsigned pu_num_passed   = 0;
static unsigned pu_num_failed   = 0;
static unsigned pu_num_suites   = 0;
static bool     pu_colors      = false;
static bool     pu_time        = false;

static pu_setup_fn pu_setup_fp    = NULL;
static pu_setup_fn pu_teardown_fp = NULL;

void
pu_setup (pu_setup_fn fp_setup, pu_setup_fn fp_teardown)
{
    pu_setup_fp = fp_setup;
    pu_teardown_fp = fp_teardown;
}

void
pu_clear_setup (void)
{
    pu_setup_fp = NULL;
    pu_teardown_fp = NULL;
}

void
pu_display_colors (bool enabled)
{
    pu_colors = enabled;
}

void
pu_display_time (bool enabled)
{
    pu_time = enabled;
}

bool
pu_test_failed(void)
{
    return (pu_num_failed != 0);
}

bool
pu_require(bool passed,
           const char* const expr,
           const char* const file,
           int line)
{
    pu_num_asserts++;

    if (passed)
    {
        return true;
    }

    if (pu_colors)
    {
        printf("(%c%sFAILED%c%s: %s (%d): %s)\n",
               TERM_COLOR_CODE, TERM_COLOR_RED,
               TERM_COLOR_CODE, TERM_COLOR_RESET,
               file, line, expr);
    }
    else
    {
        printf("(FAILED: %s (%d): %s)\n", file, line, expr);
    }

    return false;
}

void
pu_run_test (const char* const name, pu_test_fn test_fp)
{
    if (NULL != pu_setup_fp)
    {
        pu_setup_fp();
    }

    printf("Running: %s ", name);

    #ifndef PICO_UNIT_NO_CLOCK

    clock_t start_time = 0;
    clock_t end_time = 0;

    if (pu_time)
    {
        start_time = clock();
    }

    #endif // PICO_UNIT_NO_CLOCK

    if (!test_fp())
    {
        pu_num_failed++;

        if (NULL != pu_teardown_fp)
        {
            pu_teardown_fp();
        }

        return;
    }

    #ifndef PICO_UNIT_NO_CLOCK

    if (pu_time)
    {
        end_time = clock();
    }

    #endif // PICO_UNIT_NO_CLOCK

    if (pu_colors)
    {
        printf("(%c%sOK%c%s)", TERM_COLOR_CODE, TERM_COLOR_GREEN,
                               TERM_COLOR_CODE, TERM_COLOR_RESET);
    }
    else
    {
        printf("(OK)");
    }

    #ifndef PICO_UNIT_NO_CLOCK

    if (pu_time)
    {
        printf(" (%f secs)", (double)(end_time - start_time) / CLOCKS_PER_SEC);
    }

    #endif // PICO_UNIT_NO_CLOCK

    printf("\n");

    pu_num_passed++;

    if (NULL != pu_teardown_fp)
    {
        pu_teardown_fp();
    }
}

void
pu_run_suite (const char* const name, pu_suite_fn suite_fp)
{
    printf("===============================================================\n");

    if (pu_colors)
    {
        printf("%c%sRunning: %s%c%s\n", TERM_COLOR_CODE, TERM_COLOR_BOLD,
                                        name,
                                        TERM_COLOR_CODE, TERM_COLOR_RESET);
    }
    else
    {
        printf("Running: %s\n", name);
    }

    printf("---------------------------------------------------------------\n");
    suite_fp();
    pu_num_suites++;
}

void
pu_print_stats (void)
{
    printf("===============================================================\n");

    if (pu_colors)
    {
        printf("Summary: Passed: %c%s%u%c%s "
               "Failed: %c%s%u%c%s "
               "Total: %u Suites: %u "
               "Asserts: %u\n",
                TERM_COLOR_CODE, TERM_COLOR_GREEN, pu_num_passed,
                TERM_COLOR_CODE, TERM_COLOR_RESET,
                TERM_COLOR_CODE, TERM_COLOR_RED, pu_num_failed,
                TERM_COLOR_CODE, TERM_COLOR_RESET,
                pu_num_passed + pu_num_failed,
                pu_num_suites,  pu_num_asserts);
    }
    else
    {
        printf("Summary: Passed: %u "
               "Failed: %u "
               "Total: %u Suites: %u "
               "Asserts: %u\n",
                pu_num_passed,
                pu_num_failed,
                pu_num_passed + pu_num_failed,
                pu_num_suites,  pu_num_asserts);
    }
}


#endif // PICO_UNIT_IMPLEMENTATION

/*
    ----------------------------------------------------------------------------
    This software is available under two licenses (A) or (B). You may choose
    either one as you wish:
    ----------------------------------------------------------------------------

    (A) The zlib License

    Copyright (c) 2021 James McLean

    This software is provided 'as-is', without any express or implied warranty.
    In no event will the authors be held liable for any damages arising from the
    use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software in a
    product, an acknowledgment in the product documentation would be appreciated
    but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.

    ----------------------------------------------------------------------------

    (B) Public Domain (www.unlicense.org)

    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or distribute
    this software, either in source code form or as a compiled binary, for any
    purpose, commercial or non-commercial, and by any means.

    In jurisdictions that recognize copyright laws, the author or authors of
    this software dedicate any and all copyright interest in the software to the
    public domain. We make this dedication for the benefit of the public at
    large and to the detriment of our heirs and successors. We intend this
    dedication to be an overt act of relinquishment in perpetuity of all present
    and future rights to this software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
    ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

// EoF