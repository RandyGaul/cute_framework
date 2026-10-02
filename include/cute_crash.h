/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info

	Crash reporting: the framework's face of cute_crash.h and cute_sym.h, which were originally
	written by bullno1 as crash-where (https://github.com/bullno1/crash-where).
*/

#ifndef CF_CRASH_H
#define CF_CRASH_H

#include "cute_defines.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @enum     CF_CrashMode
 * @category crash
 * @brief    How a crash is caught.
 * @remarks  In-process (the default) catches the crash in the crashing process, writes the report and dies.
 *           Watcher spawns a second copy of the executable at startup that watches the game from outside:
 *           on Windows it walks the crashed stack and writes the minidump from a healthy process and catches
 *           hangs and silent exits the moment they happen; on macOS and Linux it only uploads at once.
 * @related  CF_CrashConfig cf_crash_init
 */
#define CF_CRASH_MODE_DEFS \
	/* @entry Catch the crash inside the crashing process. The report is uploaded on the next launch, or by a child process spawned from the handler when `upload_on_crash` is set. */ \
	CF_ENUM(CRASH_MODE_INPROCESS, 0) \
	/* @entry A watcher process spawned by `cf_crash_init` reports from outside. Exits with the game; never respawns. */ \
	CF_ENUM(CRASH_MODE_WATCHER, 1) \
	/* @end */

typedef enum CF_CrashMode
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_CRASH_MODE_DEFS
	#undef CF_ENUM
} CF_CrashMode;

/**
 * @struct   CF_CrashConfig
 * @category crash
 * @brief    What a crash report says about the build, where it goes, and what the reporter watches for.
 * @remarks  Every field has a working default from `cf_crash_defaults`; a report needs only `version` and `upload_url`.
 *           A report holds the stack of the crashing thread as module-relative addresses plus the build id of
 *           every loaded module, the fault, the machine, breadcrumbs (`cf_crash_breadcrumb`), state
 *           (`cf_crash_set`), and on Windows a minidump. Reports are JSON files in `report_dir`. Before upload
 *           they are symbolicated against any symbol table found: embedded in the module (`cf_symbols(target EMBED)`
 *           in CMake), beside it as `<module>.sym`, or in `sym_dir`. Missing tables leave frames raw; the
 *           developer resolves them later with `cute-sym resolve` and the tables kept from the build.
 * @related  cf_crash_defaults cf_crash_init cf_crash_breadcrumb cf_crash_set
 */
typedef struct CF_CrashConfig
{
	/* @member The game's version string. Goes into every report. */
	const char* version;

	/* @member A build label, such as a git sha. Optional; the report carries each module's build id regardless. */
	const char* build;

	/* @member A build configuration label ("Debug", "Release"). Optional. */
	const char* config;

	/* @member Where reports wait on disk. NULL for the platform's per-user app data under the executable's name. */
	const char* report_dir;

	/* @member Where `<module>.sym` tables are looked for. NULL for beside each module. Embedded tables are found regardless. */
	const char* sym_dir;

	/* @member Full URL of the endpoint reports are POSTed to, e.g. "https://crash.example.com/v1/crash". NULL keeps reports on disk, unsent. */
	const char* upload_url;

	/* @member An optional bearer token sent in the Authorization header. */
	const char* upload_token;

	/* @member See `CF_CrashMode`. */
	CF_CrashMode mode;

	/* @member In-process mode: spawn a child from the crash handler that uploads at once. Otherwise reports upload on the next launch. */
	bool upload_on_crash;

	/* @member Windows: write a minidump beside the report, with stacks and the memory they reference, so locals are visible in a debugger. Default true. */
	bool minidump;

	/* @member Include a random per-installation id in reports so a service can count affected users. Default true. */
	bool install_id;

	/* @member Seconds without a frame before the main thread is sampled and a hang report is written. 0 (default) turns the watchdog off. CF calls the heartbeat from `cf_app_update`. */
	float hang_seconds;

	/* @member Ask the player before sending, once, and remember the answer. Default true. */
	bool ask_consent;

	/* @member A failed assert (the CF_ASSERT macro) records its expression, file and line (`state.assert`, and the last breadcrumb) before the assert handler runs; if the handler returns, the assert is written as a report of its own, with a stack. Default true. */
	bool assert_reports;

	/* @member Called inside the crash handler before the report is written: the last chance to call `cf_crash_set`. No allocation, no locks. */
	void (*on_crash)(void* udata);

	/* @member Called on the watchdog thread after a hang report is written. Default: nothing; the game keeps running and the watchdog keeps watching. */
	void (*on_hang)(void* udata);

	/* @member Passed to the callbacks. */
	void* udata;
} CF_CrashConfig;
// @end

/**
 * @function cf_crash_defaults
 * @category crash
 * @brief    Returns a `CF_CrashConfig` with every field at its default.
 * @related  CF_CrashConfig cf_crash_init
 */
CF_API CF_CrashConfig CF_CALL cf_crash_defaults(void);

/**
 * @function cf_crash_init
 * @category crash
 * @brief    Installs the crash reporter. Call it first thing in `main`, before `cf_make_app`.
 * @param    config     See `CF_CrashConfig`.
 * @param    argc       The program's arguments; the reporter's own flags (`--cc-upload`, `--cc-watch`, `--cc-test`) are handled here and never reach the game.
 * @param    argv       The program's arguments.
 * @return   Returns true when the reporter is active. False under a debugger, with `CC_DISABLE=1` in the environment, or when the report directory cannot be made.
 * @remarks  Also uploads any report left by an earlier run (after asking the player once, when `ask_consent` is set) on a background
 *           thread, and registers the clean-exit marker with `atexit`, so a run that ends without reaching `exit` is reported as an
 *           abnormal exit next time. Threads made with `cf_thread_create` are attached automatically; a thread made some other way
 *           gains stack-overflow coverage through `cf_crash_attach_thread`.
 * @related  CF_CrashConfig cf_crash_defaults cf_crash_breadcrumb cf_crash_set cf_crash_hang_pause
 */
CF_API bool CF_CALL cf_crash_init(CF_CrashConfig config, int argc, char** argv);

/**
 * @function cf_crash_breadcrumb
 * @category crash
 * @brief    Records one line of what the game is doing, into a ring of the most recent few hundred that every report carries.
 * @param    fmt        printf-style format.
 * @remarks  Cheap: a copy into a fixed slot. Use it where the game logs anyway: a scene loaded, a player joined, a turn advanced.
 * @related  cf_crash_set cf_crash_init
 */
CF_API void CF_CALL cf_crash_breadcrumb(const char* fmt, ...);

/**
 * @function cf_crash_report
 * @category crash
 * @brief    Writes a report now, with a message and the calling thread's stack, and returns: a report that is not a crash.
 * @param    fmt        printf-style format of the message.
 * @remarks  For a condition the game survived but wants a stack for: a failed assert in a build that does not stop on them, a state
 *           that should not happen. Uploaded like any report. At most a handful per run, so a loop cannot flood the directory.
 * @related  cf_crash_breadcrumb cf_crash_set cf_crash_init
 */
CF_API void CF_CALL cf_crash_report(const char* fmt, ...);

/**
 * @function cf_crash_set
 * @category crash
 * @brief    Sets one key to one value in the state every report carries: what is true now, as opposed to what happened.
 * @param    key        Up to 31 characters. Overwrites an existing key.
 * @param    value      Up to 127 characters, or NULL to clear the key.
 * @remarks  The keys "gpu", "gpu_driver", "backend", "display", "window" and "fullscreen" are copied into the report's machine
 *           section; CF sets them itself once the app exists.
 * @related  cf_crash_breadcrumb cf_crash_init
 */
CF_API void CF_CALL cf_crash_set(const char* key, const char* value);

/**
 * @function cf_crash_hang_pause
 * @category crash
 * @brief    Suspends the hang watchdog around work the game knows takes long, such as a load. Nestable; pair with `cf_crash_hang_resume`.
 * @related  cf_crash_hang_resume CF_CrashConfig
 */
CF_API void CF_CALL cf_crash_hang_pause(void);

/**
 * @function cf_crash_hang_resume
 * @category crash
 * @brief    Resumes the hang watchdog after `cf_crash_hang_pause`.
 * @related  cf_crash_hang_pause CF_CrashConfig
 */
CF_API void CF_CALL cf_crash_hang_resume(void);

/**
 * @function cf_crash_attach_thread
 * @category crash
 * @brief    Gives a thread not made by `cf_thread_create` stack-overflow coverage and a name in reports. Call it on that thread.
 * @param    name       The thread's name as reports show it.
 * @related  cf_crash_init
 */
CF_API void CF_CALL cf_crash_attach_thread(const char* name);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

using CrashConfig = CF_CrashConfig;
using CrashMode = CF_CrashMode;

CF_INLINE CrashConfig crash_defaults() { return cf_crash_defaults(); }
CF_INLINE bool crash_init(CrashConfig config, int argc, char** argv) { return cf_crash_init(config, argc, argv); }
CF_INLINE void crash_set(const char* key, const char* value) { cf_crash_set(key, value); }
CF_INLINE void crash_hang_pause() { cf_crash_hang_pause(); }
CF_INLINE void crash_hang_resume() { cf_crash_hang_resume(); }
CF_INLINE void crash_attach_thread(const char* name) { cf_crash_attach_thread(name); }

}

#endif // CF_CPP

#endif // CF_CRASH_H
