/*
	------------------------------------------------------------------------------
		Licensing information can be found at the end of the file.
	------------------------------------------------------------------------------

	cute_crash.h - v0.01

	To create implementation (the function definitions)
		#define CUTE_CRASH_IMPLEMENTATION
	in *one* C/CPP file (translation unit) that includes this file


	SUMMARY:

		A crash reporter in a single header, for games. When the process crashes, hangs,
		or vanishes without a clean exit, a report is written to disk: the stack of the
		thread that died as module-relative addresses, every loaded module with its build
		id, what the game said it was doing (breadcrumbs), what it said was true (state),
		the machine, and on Windows a minidump holding every thread's stack and the
		locals in it. At the next launch, or at once from a child process, the report is
		symbolicated against a table beside or inside the executable (cute_sym.h makes
		those) and handed to a callback to send wherever it should go. Any reply is the
		ack.

		Original author: bullno1 -- the design is crash-where's
		(https://github.com/bullno1/crash-where): module-relative frames keyed by build
		id, a symbol table made at build time, a watcher process that unwinds from
		outside. This is a self-contained rewrite for Cute Framework by Randy Gaul, with
		in-process capture as the default and the watcher as an option.

		Platforms: Windows (x64, arm64, x86), macOS, Linux. Nothing is required of the
		build: no frame pointers, no flags. No dependencies beyond libc and the OS.

		What happens inside the crash: no heap, no locks, no stdio. Everything the
		handler writes into was allocated by cc_init. The report is bytes in a buffer
		written with one system call, and the process then ends. Symbolication, the
		consent question, and the upload never run in the crashed process: they run at
		the next cc_init, or in a child spawned from the handler with upload_on_crash,
		or in the watcher.

		A clean exit is the atexit hook (or cc_shutdown). A run that ends any other way
		leaves the `running` marker behind, and the next cc_init reports it as
		"abnormal_exit" with whatever state the marker held.


	USAGE:

		int main(int argc, char** argv)
		{
			cc_config cfg = cc_defaults();
			cfg.version = "0.3.1";
			cfg.send = my_post;        // bool (void* udata, const char* report_path, const char* dump_path)
			cfg.hang_seconds = 20;
			cc_init(cfg, argc, argv);  // Before anything that can crash. Handles --cc-upload and --cc-test itself.

			while (running) {
				cc_heartbeat();
				cc_set("level", level_name);
				cc_breadcrumb("turn %d", turn);
				...
			}
			return 0;                  // atexit marks the clean exit.
		}

		Reports live in cc_config.report_dir (default: the platform's per-user app data
		under the app's name) as crash-<time>-<pid>.json plus crash-<time>-<pid>.dmp on
		Windows. `cute_sym print report.json` renders one for a human.


	CUSTOMIZATION:

		Define before including, in the implementation translation unit:

			CC_REPORT_BYTES       Size of the report buffer (256 KB). A report that does not fit is truncated at a safe point.
			CC_BREADCRUMBS        Ring size in lines (256).
			CC_BREADCRUMB_BYTES   Bytes per line (160).
			CC_STATE_SLOTS        Key/value slots (32).
			CC_STACK_BYTES        Raw stack copied into a POSIX report (64 KB).
			CC_MAX_FRAMES         Frames walked (128).
			CC_MAX_MODULES        Modules listed (256).
			CUTE_CRASH_SYM_RESERVE
			                      Bytes reserved inside the executable for an embedded symbol table.
			                      `cute_sym <exe> --embed` fills it. Undefined: no slot.

	Revision history:
		0.01 (10/01/2026) first version
*/

#if !defined(CUTE_CRASH_H)

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum cc_mode
{
	CC_MODE_INPROCESS, // The crashed process writes its own report. Default.
	CC_MODE_WATCHER,   // A second copy of the executable, spawned by cc_init, writes it from outside (Windows), or watches and uploads (POSIX).
} cc_mode;

typedef enum cc_consent
{
	CC_CONSENT_ASK,    // No decision: nothing is sent now and the question comes back next time.
	CC_CONSENT_ONCE,   // Send what is pending now; ask again for later reports. Never stored.
	CC_CONSENT_SEND,   // Send, now and from now on.
	CC_CONSENT_NEVER,  // Delete everything pending, write nothing more.
} cc_consent;

typedef struct cc_config
{
	const char* app;         // NULL: the executable's basename. Names the report directory.
	const char* version;
	const char* build;       // Optional label (a git sha) beside the build ids every module carries.
	const char* config;      // Optional ("Debug", "RelWithDebInfo").
	const char* report_dir;  // NULL: %LOCALAPPDATA%\<app>\crash\, ~/Library/Application Support/<app>/crash/, $XDG_DATA_HOME|~/.local/share/<app>/crash/.
	const char* sym_dir;     // NULL: beside each module. A table embedded in a module is found regardless.
	cc_mode mode;
	bool upload_on_crash;    // CC_MODE_INPROCESS: spawn the uploader child from the handler. Else the next cc_init uploads.
	bool minidump;           // Windows. Default true.
	bool install_id;         // A random per-install id in every report, so a server can count users. Default true.
	float hang_seconds;      // 0: no watchdog.
	// Post one report. ANY reply from the server is the ack: return true. Return false only when nothing answered.
	// NULL: reports stay on disk, never sent.
	bool (*send)(void* udata, const char* report_path, const char* attachment_path);
	// Ask the player when no decision is stored: ONCE sends what is pending and asks again later, SEND and
	// NEVER are remembered, ASK sends nothing now. NULL: send without asking.
	cc_consent (*ask)(void* udata, int pending_count);
	void (*on_crash)(void* udata); // Inside the handler, before the report is written: the last chance for cc_set. No heap, no locks.
	void (*on_hang)(void* udata);  // On the watchdog thread after a hang report is written. Default: nothing; the game keeps waiting.
	void* udata;
} cc_config;

cc_config cc_defaults(void);

// Installs everything. Handles `--cc-upload <dir>` (uploads pending reports, exits), `--cc-watch ...`
// (the watcher, never returns) and `--cc-test <kind>` (crashes on purpose: null, overflow, abort,
// throw, thread, hang). Returns false, installing nothing, under a debugger or with CC_DISABLE=1.
bool cc_init(cc_config config, int argc, char** argv);

// A clean exit. Registered with atexit by cc_init; call it yourself before _exit.
void cc_shutdown(void);

// Once a frame when hang_seconds > 0.
void cc_heartbeat(void);

// From any thread that should survive its own stack overflow, and to name it in reports. The thread
// that called cc_init is attached as "main".
void cc_attach_thread(const char* name);

// Around work the game knows is long. Nestable.
void cc_hang_pause(void);
void cc_hang_resume(void);

// The last CC_BREADCRUMBS lines, in the report. Any thread. Cheap: a formatted copy into a ring.
void cc_breadcrumb(const char* fmt, ...);

// A report that is not a crash: a failed assert the program survived, a condition worth a stack. Returns.
void cc_report(const char* message);

// What is true now, in the report. CC_STATE_SLOTS keys; a repeated key overwrites; NULL clears. Keys
// "gpu", "gpu_driver", "backend", "display", "window", "fullscreen" also appear under "machine".
void cc_set(const char* key, const char* value);

// The `--cc-test null` crash site. Exported so a test can expect it by name at the top of the stack.
void cc_test_null_site(void);

// Defines the slot `cute_sym <binary> --embed` fills: a read-only array of `bytes` zeros behind a
// 16-byte header. Expand it once in any translation unit of the executable or shared library
// that should carry its own symbol table. Discovery is by scanning the module for the header,
// so the array's name is nothing to anyone. CUTE_CRASH_SYM_RESERVE expands it in the
// implementation translation unit.
#if defined(__cplusplus)
#	define CC_SYM_SLOT_LINKAGE extern
#else
#	define CC_SYM_SLOT_LINKAGE
#endif
#if defined(_MSC_VER)
#	define CC_SYM_SLOT_KEEP
#else
#	define CC_SYM_SLOT_KEEP __attribute__((used))
#endif
#define CC_SYM_SLOT_DEFINE(bytes) \
	CC_SYM_SLOT_LINKAGE CC_SYM_SLOT_KEEP const unsigned char cc_sym_slot[16 + (bytes)] = { \
		'C', 'U', 'T', 'E', 'S', 'Y', 'M', 'S', 'L', 'O', 'T', 0, \
		(unsigned char)((bytes) & 0xff), (unsigned char)(((bytes) >> 8) & 0xff), (unsigned char)(((bytes) >> 16) & 0xff), (unsigned char)(((bytes) >> 24) & 0xff) }

#ifdef __cplusplus
}
#endif

#define CUTE_CRASH_H
#endif // CUTE_CRASH_H

#if defined(CUTE_CRASH_IMPLEMENTATION)
#if !defined(CUTE_CRASH_IMPLEMENTATION_ONCE)
#define CUTE_CRASH_IMPLEMENTATION_ONCE

// The sizes a user may set before including the implementation; the web stub needs them too.
#if !defined(CC_REPORT_BYTES)
#	define CC_REPORT_BYTES (256 * 1024)
#endif
#if !defined(CC_BREADCRUMBS)
#	define CC_BREADCRUMBS 256
#endif
#if !defined(CC_BREADCRUMB_BYTES)
#	define CC_BREADCRUMB_BYTES 160
#endif
#if !defined(CC_STATE_SLOTS)
#	define CC_STATE_SLOTS 32
#endif
#if !defined(CC_STACK_BYTES)
#	define CC_STACK_BYTES (64 * 1024)
#endif

#if defined(__EMSCRIPTEN__)
// The web: no signals to catch, no process to spawn, no file to leave. Every call is a no-op and
// cc_init says so by returning false; the browser's console is the crash report there.
#include <stdarg.h>
#include <string.h>
cc_config cc_defaults(void) { cc_config c; memset(&c, 0, sizeof(c)); c.minidump = true; c.install_id = true; return c; }
bool cc_init(cc_config config, int argc, char** argv) { (void)config; (void)argc; (void)argv; return false; }
void cc_shutdown(void) {}
void cc_heartbeat(void) {}
void cc_attach_thread(const char* name) { (void)name; }
void cc_hang_pause(void) {}
void cc_hang_resume(void) {}
void cc_breadcrumb(const char* fmt, ...) { (void)fmt; }
void cc_report(const char* message) { (void)message; }
void cc_set(const char* key, const char* value) { (void)key; (void)value; }
#else

#if !defined(CC_REPORT_BYTES)
#	define CC_REPORT_BYTES (256 * 1024)
#endif
#if !defined(CC_BREADCRUMBS)
#	define CC_BREADCRUMBS 256
#endif
#if !defined(CC_BREADCRUMB_BYTES)
#	define CC_BREADCRUMB_BYTES 160
#endif
#if !defined(CC_STATE_SLOTS)
#	define CC_STATE_SLOTS 32
#endif
#if !defined(CC_STACK_BYTES)
#	define CC_STACK_BYTES (64 * 1024)
#endif
#if !defined(CC_MAX_FRAMES)
#	define CC_MAX_FRAMES 128
#endif
#if !defined(CC_MAX_MODULES)
#	define CC_MAX_MODULES 256
#endif
#define CC_STATE_KEY 32
#define CC_STATE_VALUE 128
#define CC_PATH 1024
#define CC_MODULE_PATH 512
#define CC_MAX_THREADS 64
#define CC_MARKER_CRUMBS 32
#define CC_UPLOAD_ATTEMPTS 5
#define CC_SIGNATURE_FRAMES 8

#if defined(CUTE_CRASH_SYM_RESERVE) && (CUTE_CRASH_SYM_RESERVE > 0)
CC_SYM_SLOT_DEFINE(CUTE_CRASH_SYM_RESERVE);
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include <time.h>

#if defined(_WIN32)
#	if !defined(WIN32_LEAN_AND_MEAN)
#		define WIN32_LEAN_AND_MEAN
#	endif
#	if !defined(NOMINMAX)
#		define NOMINMAX
#	endif
#	include <windows.h>
#	include <winternl.h>
#	include <dbghelp.h>
#	include <intrin.h>
#	if defined(_MSC_VER)
#		pragma comment(lib, "advapi32.lib")
#	endif
#else
#	include <unistd.h>
#	include <fcntl.h>
#	include <errno.h>
#	include <pthread.h>
#	include <dirent.h>
#	include <dlfcn.h>
#	include <execinfo.h>
#	include <sys/stat.h>
#	include <sys/types.h>
#	include <sys/wait.h>
#	include <sys/utsname.h>
#	if defined(__APPLE__)
#		include <mach-o/dyld.h>
#		include <mach-o/loader.h>
#		include <sys/sysctl.h>
#	else
#		include <link.h>
#		include <elf.h>
#		include <sys/sysinfo.h>
#		include <sys/syscall.h>
#	endif
#endif

#if defined(__cplusplus)
#	include <exception>
#	include <typeinfo>
#endif

#if defined(_MSC_VER)
#	define CC_NOINLINE __declspec(noinline)
#	define CC_THREAD_LOCAL __declspec(thread)
#else
#	define CC_NOINLINE __attribute__((noinline))
#	define CC_THREAD_LOCAL __thread
#endif

// --- atomics -------------------------------------------------------------------------------------

#if defined(_MSC_VER) && !defined(__clang__)
static uint32_t s_atomic_load32(volatile uint32_t* p) { return (uint32_t)_InterlockedCompareExchange((volatile long*)p, 0, 0); }
static void s_atomic_store32(volatile uint32_t* p, uint32_t v) { _InterlockedExchange((volatile long*)p, (long)v); }
static bool s_atomic_cas32(volatile uint32_t* p, uint32_t expect, uint32_t v) { return (uint32_t)_InterlockedCompareExchange((volatile long*)p, (long)v, (long)expect) == expect; }
static uint32_t s_atomic_inc32(volatile uint32_t* p) { return (uint32_t)_InterlockedIncrement((volatile long*)p) - 1; }
static int64_t s_atomic_load64(volatile int64_t* p) { return _InterlockedCompareExchange64(p, 0, 0); }
static void s_atomic_store64(volatile int64_t* p, int64_t v) { _InterlockedExchange64(p, v); }
#else
static uint32_t s_atomic_load32(volatile uint32_t* p) { return __atomic_load_n(p, __ATOMIC_SEQ_CST); }
static void s_atomic_store32(volatile uint32_t* p, uint32_t v) { __atomic_store_n(p, v, __ATOMIC_SEQ_CST); }
static bool s_atomic_cas32(volatile uint32_t* p, uint32_t expect, uint32_t v) { return __atomic_compare_exchange_n(p, &expect, v, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); }
static uint32_t s_atomic_inc32(volatile uint32_t* p) { return __atomic_fetch_add(p, 1, __ATOMIC_SEQ_CST); }
static int64_t s_atomic_load64(volatile int64_t* p) { return __atomic_load_n(p, __ATOMIC_SEQ_CST); }
static void s_atomic_store64(volatile int64_t* p, int64_t v) { __atomic_store_n(p, v, __ATOMIC_SEQ_CST); }
#endif

// --- the shared block: what the handler, the watchdog, and (in watcher mode) the watcher read -----
// In CC_MODE_INPROCESS this is a static; in CC_MODE_WATCHER it is a file mapping both processes see.

typedef struct cc_crumb { double t; char msg[CC_BREADCRUMB_BYTES]; } cc_crumb;
typedef struct cc_thread_name { volatile uint32_t tid; char name[32]; } cc_thread_name; // tid 0: free; CC_TID_CLAIMED: being written.
#define CC_TID_CLAIMED 0xffffffffu
typedef struct cc_slot { volatile uint32_t seq; char key[CC_STATE_KEY]; char value[CC_STATE_VALUE]; } cc_slot;

#define CC_CRASH_IDLE 0
#define CC_CRASH_WRITING 1
#define CC_CRASH_DONE 2

typedef struct cc_shared
{
	uint32_t magic;
	uint32_t pid;
	uint32_t main_tid;
	volatile uint32_t clean_shutdown;
	volatile int64_t heartbeat_ns;
	int64_t start_ns;
	int64_t start_unix;
	volatile uint32_t pause_depth;
	float hang_seconds;      // The game's config, for the watcher: its own argv carries none of it.
	uint32_t minidump;
	volatile uint32_t crumb_next;
	cc_crumb crumbs[CC_BREADCRUMBS];
	cc_slot state[CC_STATE_SLOTS];
	cc_thread_name threads[CC_MAX_THREADS];
	volatile uint32_t crash_state;
	uint32_t crash_tid;
	uint32_t crash_kind;     // 1 exception, 2 abort-like (name in crash_name)
	char crash_name[48];
#if defined(_WIN32)
	uint64_t crash_pointers; // EXCEPTION_POINTERS* in the game.
	EXCEPTION_RECORD crash_record;
	CONTEXT crash_context;
#endif
} cc_shared;

#define CC_SHARED_MAGIC 0x48534343u // "CCSH"

typedef struct cc_module
{
	uint64_t base;           // Where it is loaded.
	uint64_t size;
	uint64_t offset_base;    // What a pc subtracts to become the table's address: base (PE, RVA), the slide (Mach-O), the load bias (ELF).
	char path[CC_MODULE_PATH];
	int name_at;             // Basename offset in path.
	unsigned char build_id[20];
	int build_id_len;
	bool system;
} cc_module;

typedef struct cc_fault
{
	const char* kind;        // "crash", "hang", "abnormal_exit"
	const char* name;        // Exception or signal name, or NULL.
	uint64_t code;           // Windows exception code.
	const char* code_name;   // POSIX si_code name.
	const char* access;      // "read", "write", "execute", NULL.
	uint64_t address;
	bool has_address;
	uint32_t tid;
	double hang_seconds;
	const char* message;     // "report": what the program had to say.
} cc_fault;

static struct
{
	bool inited;
	bool enabled;
	cc_config cfg;
	char app[64];
	char version[64];
	char build[64];
	char config_name[32];
	char report_dir[CC_PATH];   // With the trailing separator.
	char sym_dir[CC_PATH];
	char exe_path[CC_PATH];
	int argc;                // The game's arguments, as given to cc_init: the children get them too.
	char** argv;
	char marker_path[CC_PATH];
	char consent_path[CC_PATH];
	char install_path[CC_PATH];
	char install[40];
	char report_path[CC_PATH];
	char dump_path[CC_PATH];
	char* report;               // CC_REPORT_BYTES.
	cc_shared* sh;
	cc_shared sh_private;
	cc_module modules[CC_MAX_MODULES];
	int module_count;
	uint64_t pcs[CC_MAX_FRAMES];
	int pc_count;
	char os[96];
	char cpu[96];
	char arch[16];
	int cpu_threads;
	uint64_t ram_mb;
	unsigned char entropy[64];
	volatile uint32_t handler_depth;
	uint32_t handler_tid;
	bool watchdog_on;
	volatile uint32_t hang_reported;
	uint64_t sample_pcs[2][CC_MAX_FRAMES];
	int sample_count[2];
	volatile uint32_t sample_done;
	int64_t marker_written_ns;
	bool has_dump;
	uint64_t dump_bytes;
	bool in_child;
	bool exit_after_hang;       // --cc-test hang: the process ends once the hang report is written.
	bool marker_on;             // This process keeps the `running` marker (in-process mode, or any POSIX mode).
	bool hang_on;               // This process's watchdog samples for hangs (in-process mode; the watcher does it otherwise).
	char* marker;               // Its own buffer: the watchdog writes it while a crash may own `report`.
#if defined(_WIN32)
	wchar_t report_dir_w[CC_PATH];
	wchar_t exe_w[CC_PATH];
	wchar_t child_cmd[CC_PATH * 4];
	HANDLE main_thread;
	HANDLE watchdog_thread;
	HANDLE upload_thread;
	DWORD fls_index;            // Per-thread exit callback (registry slot release). FLS_OUT_OF_INDEXES: none.
	HMODULE dbghelp;
	BOOL (WINAPI *MiniDumpWriteDump)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE, PMINIDUMP_EXCEPTION_INFORMATION, PMINIDUMP_USER_STREAM_INFORMATION, PMINIDUMP_CALLBACK_INFORMATION);
	LPTOP_LEVEL_EXCEPTION_FILTER filter;
	PVOID veh;
	// Watcher mode.
	HANDLE region_mapping;
	HANDLE ev_crash;
	HANDLE ev_done;
	HANDLE watcher_process;
	HANDLE game_process;
#else
	char* child_argv[64];
	char child_flag[16];
	pthread_t main_pthread;
	pthread_t watchdog_thread;
	pthread_t upload_thread;
	bool upload_thread_live;
	pthread_key_t thread_key;   // Per-thread exit destructor (alternate stack and registry slot release).
	bool thread_key_ok;
	unsigned char* stack_copy;
	size_t stack_copy_len;
	int watch_pipe[2];
#endif
} s_cc;

// --- small utilities: nothing here allocates ---------------------------------------------------

static size_t s_strlen(const char* s) { size_t n = 0; if (s) while (s[n]) ++n; return n; }

static void s_strcpy(char* dst, size_t cap, const char* src)
{
	size_t i = 0;
	if (!cap) return;
	if (src) for (; src[i] && i + 1 < cap; ++i) dst[i] = src[i];
	dst[i] = 0;
}

static void s_strcat(char* dst, size_t cap, const char* src)
{
	size_t n = s_strlen(dst);
	if (n < cap) s_strcpy(dst + n, cap - n, src);
}

static bool s_streq(const char* a, const char* b) { return a && b && strcmp(a, b) == 0; }

static bool s_starts_with_nocase(const char* s, const char* prefix)
{
	for (; *prefix; ++s, ++prefix) {
		char a = *s, b = *prefix;
		if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
		if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
		if (a != b) return false;
	}
	return true;
}

static const char* s_basename(const char* path)
{
	const char* b = path;
	for (const char* p = path; *p; ++p) if (*p == '/' || *p == '\\') b = p + 1;
	return b;
}

// Unsigned to decimal, returns length. `out` holds at least 21 bytes.
static int s_utoa(uint64_t v, char* out)
{
	char tmp[24];
	int n = 0;
	do { tmp[n++] = (char)('0' + (int)(v % 10)); v /= 10; } while (v);
	for (int i = 0; i < n; ++i) out[i] = tmp[n - 1 - i];
	out[n] = 0;
	return n;
}

static int s_itoa(int64_t v, char* out)
{
	if (v < 0) { out[0] = '-'; return 1 + s_utoa((uint64_t)(-(v + 1)) + 1, out + 1); }
	return s_utoa((uint64_t)v, out);
}

// Lowercase hex without a prefix, `digits` wide (0: as many as needed).
static int s_hex(uint64_t v, int digits, char* out)
{
	static const char* hexd = "0123456789abcdef";
	char tmp[17];
	int n = 0;
	do { tmp[n++] = hexd[v & 15]; v >>= 4; } while (v || n < digits);
	for (int i = 0; i < n; ++i) out[i] = tmp[n - 1 - i];
	out[n] = 0;
	return n;
}

static void s_hex_bytes(const unsigned char* b, int n, char* out)
{
	static const char* hexd = "0123456789abcdef";
	for (int i = 0; i < n; ++i) { out[i * 2] = hexd[b[i] >> 4]; out[i * 2 + 1] = hexd[b[i] & 15]; }
	out[n * 2] = 0;
}

static int s_unhex_bytes(const char* s, unsigned char* out, int cap)
{
	int n = 0;
	for (; s[0] && s[1] && n < cap; s += 2, ++n) {
		int hi, lo;
		char a = s[0], b = s[1];
		hi = a >= '0' && a <= '9' ? a - '0' : a >= 'a' && a <= 'f' ? a - 'a' + 10 : a >= 'A' && a <= 'F' ? a - 'A' + 10 : -1;
		lo = b >= '0' && b <= '9' ? b - '0' : b >= 'a' && b <= 'f' ? b - 'a' + 10 : b >= 'A' && b <= 'F' ? b - 'A' + 10 : -1;
		if (hi < 0 || lo < 0) break;
		out[n] = (unsigned char)(hi * 16 + lo);
	}
	return n;
}

// Fixed point with `decimals` places. Enough for seconds.
static int s_ftoa(double v, int decimals, char* out)
{
	int n = 0;
	if (v < 0) { out[n++] = '-'; v = -v; }
	if (v > 1.0e15) v = 1.0e15;
	uint64_t whole = (uint64_t)v;
	double frac = v - (double)whole;
	n += s_utoa(whole, out + n);
	if (decimals > 0) {
		out[n++] = '.';
		for (int i = 0; i < decimals; ++i) { frac *= 10.0; int d = (int)frac; if (d > 9) d = 9; out[n++] = (char)('0' + d); frac -= d; }
		out[n] = 0;
	}
	return n;
}

// Monotonic nanoseconds and wall-clock Unix seconds. Both are safe inside a handler.
static int64_t s_now_ns(void)
{
#if defined(_WIN32)
	static LARGE_INTEGER freq;
	LARGE_INTEGER c;
	if (!freq.QuadPart) QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&c);
	return (int64_t)((double)c.QuadPart * (1.0e9 / (double)freq.QuadPart));
#else
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (int64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec;
#endif
}

static int64_t s_now_unix(void)
{
#if defined(_WIN32)
	FILETIME ft;
	ULARGE_INTEGER u;
	GetSystemTimeAsFileTime(&ft);
	u.LowPart = ft.dwLowDateTime; u.HighPart = ft.dwHighDateTime;
	return (int64_t)(u.QuadPart / 10000000ULL) - 11644473600LL;
#else
	return (int64_t)time(NULL);
#endif
}

// Days since 1970-01-01 to y/m/d (Howard Hinnant's civil_from_days): no libc, no locale.
static void s_civil(int64_t unix_seconds, int* y, int* m, int* d, int* hh, int* mm, int* ss)
{
	int64_t z = unix_seconds / 86400;
	int64_t rem = unix_seconds - z * 86400;
	if (rem < 0) { rem += 86400; z -= 1; }
	*hh = (int)(rem / 3600); *mm = (int)(rem / 60 % 60); *ss = (int)(rem % 60);
	z += 719468;
	int64_t era = (z >= 0 ? z : z - 146096) / 146097;
	int64_t doe = z - era * 146097;
	int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
	int64_t yy = yoe + era * 400;
	int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
	int64_t mp = (5 * doy + 2) / 153;
	*d = (int)(doy - (153 * mp + 2) / 5 + 1);
	*m = (int)(mp < 10 ? mp + 3 : mp - 9);
	*y = (int)(yy + (*m <= 2));
}

static void s_pad2(int v, char* out) { out[0] = (char)('0' + v / 10 % 10); out[1] = (char)('0' + v % 10); }

// "2026-10-01T22:14:07Z" (20 chars) and "20261001-221407" (15 chars).
static void s_iso_time(int64_t unix_seconds, char* out)
{
	int y, m, d, hh, mm, ss;
	s_civil(unix_seconds, &y, &m, &d, &hh, &mm, &ss);
	s_utoa((uint64_t)y, out); s_pad2(y / 100 % 100, out); s_pad2(y % 100, out + 2);
	out[4] = '-'; s_pad2(m, out + 5); out[7] = '-'; s_pad2(d, out + 8); out[10] = 'T';
	s_pad2(hh, out + 11); out[13] = ':'; s_pad2(mm, out + 14); out[16] = ':'; s_pad2(ss, out + 17); out[19] = 'Z'; out[20] = 0;
}

static void s_stamp_time(int64_t unix_seconds, char* out)
{
	int y, m, d, hh, mm, ss;
	s_civil(unix_seconds, &y, &m, &d, &hh, &mm, &ss);
	s_pad2(y / 100 % 100, out); s_pad2(y % 100, out + 2); s_pad2(m, out + 4); s_pad2(d, out + 6);
	out[8] = '-'; s_pad2(hh, out + 9); s_pad2(mm, out + 11); s_pad2(ss, out + 13); out[15] = 0;
}

static uint32_t s_current_tid(void)
{
#if defined(_WIN32)
	return (uint32_t)GetCurrentThreadId();
#elif defined(__APPLE__)
	uint64_t tid = 0;
	pthread_threadid_np(NULL, &tid);
	return (uint32_t)tid;
#else
	return (uint32_t)syscall(SYS_gettid);
#endif
}

static uint32_t s_current_pid(void)
{
#if defined(_WIN32)
	return (uint32_t)GetCurrentProcessId();
#else
	return (uint32_t)getpid();
#endif
}

// --- sha1 ----------------------------------------------------------------------------------------

typedef struct cc_sha1 { uint32_t h[5]; unsigned char block[64]; uint64_t len; int fill; } cc_sha1;

static void s_sha1_init(cc_sha1* s) { s->h[0] = 0x67452301u; s->h[1] = 0xEFCDAB89u; s->h[2] = 0x98BADCFEu; s->h[3] = 0x10325476u; s->h[4] = 0xC3D2E1F0u; s->len = 0; s->fill = 0; }

static void s_sha1_block(cc_sha1* s)
{
	uint32_t w[80];
	for (int i = 0; i < 16; ++i) w[i] = ((uint32_t)s->block[i * 4] << 24) | ((uint32_t)s->block[i * 4 + 1] << 16) | ((uint32_t)s->block[i * 4 + 2] << 8) | s->block[i * 4 + 3];
	for (int i = 16; i < 80; ++i) { uint32_t x = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16]; w[i] = (x << 1) | (x >> 31); }
	uint32_t a = s->h[0], b = s->h[1], c = s->h[2], d = s->h[3], e = s->h[4];
	for (int i = 0; i < 80; ++i) {
		uint32_t f, k;
		if (i < 20) { f = (b & c) | (~b & d); k = 0x5A827999u; }
		else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1u; }
		else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDCu; }
		else { f = b ^ c ^ d; k = 0xCA62C1D6u; }
		uint32_t t = ((a << 5) | (a >> 27)) + f + e + k + w[i];
		e = d; d = c; c = (b << 30) | (b >> 2); b = a; a = t;
	}
	s->h[0] += a; s->h[1] += b; s->h[2] += c; s->h[3] += d; s->h[4] += e;
}

static void s_sha1_update(cc_sha1* s, const void* data, size_t len)
{
	const unsigned char* p = (const unsigned char*)data;
	s->len += len;
	while (len--) {
		s->block[s->fill++] = *p++;
		if (s->fill == 64) { s_sha1_block(s); s->fill = 0; }
	}
}

static void s_sha1_final(cc_sha1* s, unsigned char out[20])
{
	uint64_t bits = s->len * 8;
	unsigned char pad = 0x80;
	s_sha1_update(s, &pad, 1);
	pad = 0;
	while (s->fill != 56) s_sha1_update(s, &pad, 1);
	for (int i = 7; i >= 0; --i) { unsigned char b = (unsigned char)(bits >> (i * 8)); s_sha1_update(s, &b, 1); }
	for (int i = 0; i < 5; ++i) { out[i * 4] = (unsigned char)(s->h[i] >> 24); out[i * 4 + 1] = (unsigned char)(s->h[i] >> 16); out[i * 4 + 2] = (unsigned char)(s->h[i] >> 8); out[i * 4 + 3] = (unsigned char)s->h[i]; }
}

static void s_sha1_str(cc_sha1* s, const char* str) { s_sha1_update(s, str, s_strlen(str)); }

// --- the JSON writer: into a fixed buffer, pretty, nothing else -----------------------------------

typedef struct cc_jw
{
	char* p;
	char* end;
	int depth;
	bool first[64];
	bool pending_value;
	bool overflow;
} cc_jw;

static void s_jw_init(cc_jw* w, char* buf, size_t cap) { memset(w, 0, sizeof(*w)); w->p = buf; w->end = buf + cap - 1; w->first[0] = true; }
static void s_jw_putc(cc_jw* w, char c) { if (w->p < w->end) *w->p++ = c; else w->overflow = true; }
static void s_jw_puts(cc_jw* w, const char* s) { while (*s) s_jw_putc(w, *s++); }
static void s_jw_indent(cc_jw* w) { s_jw_putc(w, '\n'); for (int i = 0; i < w->depth; ++i) { s_jw_putc(w, ' '); s_jw_putc(w, ' '); } }

static void s_jw_sep(cc_jw* w)
{
	if (w->pending_value) { w->pending_value = false; return; }
	if (!w->first[w->depth]) s_jw_putc(w, ',');
	w->first[w->depth] = false;
	if (w->depth > 0) s_jw_indent(w);
}

static void s_jw_quoted(cc_jw* w, const char* s)
{
	static const char* hexd = "0123456789abcdef";
	s_jw_putc(w, '"');
	for (; s && *s; ++s) {
		unsigned char c = (unsigned char)*s;
		if (c == '"') s_jw_puts(w, "\\\"");
		else if (c == '\\') s_jw_puts(w, "\\\\");
		else if (c == '\n') s_jw_puts(w, "\\n");
		else if (c == '\r') s_jw_puts(w, "\\r");
		else if (c == '\t') s_jw_puts(w, "\\t");
		else if (c < 0x20) { s_jw_puts(w, "\\u00"); s_jw_putc(w, hexd[c >> 4]); s_jw_putc(w, hexd[c & 15]); }
		else s_jw_putc(w, (char)c);
	}
	s_jw_putc(w, '"');
}

static void s_jw_key(cc_jw* w, const char* key) { s_jw_sep(w); s_jw_quoted(w, key); s_jw_puts(w, ": "); w->pending_value = true; }
static void s_jw_open(cc_jw* w, char c) { s_jw_sep(w); s_jw_putc(w, c); w->depth++; if (w->depth < 64) w->first[w->depth] = true; }
static void s_jw_close(cc_jw* w, char c) { bool empty = w->depth < 64 && w->first[w->depth]; w->depth--; if (!empty) s_jw_indent(w); s_jw_putc(w, c); }
static void s_jw_begin_obj(cc_jw* w) { s_jw_open(w, '{'); }
static void s_jw_end_obj(cc_jw* w) { s_jw_close(w, '}'); }
static void s_jw_begin_arr(cc_jw* w) { s_jw_open(w, '['); }
static void s_jw_end_arr(cc_jw* w) { s_jw_close(w, ']'); }
static void s_jw_str(cc_jw* w, const char* s) { s_jw_sep(w); s_jw_quoted(w, s); }
static void s_jw_raw(cc_jw* w, const char* s) { s_jw_sep(w); s_jw_puts(w, s); }
static void s_jw_int(cc_jw* w, int64_t v) { char b[24]; s_itoa(v, b); s_jw_raw(w, b); }
static void s_jw_uint(cc_jw* w, uint64_t v) { char b[24]; s_utoa(v, b); s_jw_raw(w, b); }
static void s_jw_float(cc_jw* w, double v, int decimals) { char b[48]; s_ftoa(v, decimals, b); s_jw_raw(w, b); }
static void s_jw_bool(cc_jw* w, bool v) { s_jw_raw(w, v ? "true" : "false"); }
static void s_jw_hexstr(cc_jw* w, uint64_t v) { char b[24]; b[0] = '0'; b[1] = 'x'; s_hex(v, 0, b + 2); s_jw_str(w, b); }
static void s_jw_kv_str(cc_jw* w, const char* k, const char* v) { s_jw_key(w, k); s_jw_str(w, v); }
static void s_jw_kv_int(cc_jw* w, const char* k, int64_t v) { s_jw_key(w, k); s_jw_int(w, v); }
static void s_jw_kv_uint(cc_jw* w, const char* k, uint64_t v) { s_jw_key(w, k); s_jw_uint(w, v); }
static void s_jw_kv_bool(cc_jw* w, const char* k, bool v) { s_jw_key(w, k); s_jw_bool(w, v); }
static void s_jw_kv_hex(cc_jw* w, const char* k, uint64_t v) { s_jw_key(w, k); s_jw_hexstr(w, v); }
static size_t s_jw_len(const cc_jw* w, const char* buf) { return (size_t)(w->p - buf); }

static const char* s_b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void s_jw_base64(cc_jw* w, const unsigned char* data, size_t len)
{
	s_jw_sep(w);
	s_jw_putc(w, '"');
	for (size_t i = 0; i < len; i += 3) {
		uint32_t v = (uint32_t)data[i] << 16;
		if (i + 1 < len) v |= (uint32_t)data[i + 1] << 8;
		if (i + 2 < len) v |= data[i + 2];
		s_jw_putc(w, s_b64[(v >> 18) & 63]);
		s_jw_putc(w, s_b64[(v >> 12) & 63]);
		s_jw_putc(w, i + 1 < len ? s_b64[(v >> 6) & 63] : '=');
		s_jw_putc(w, i + 2 < len ? s_b64[v & 63] : '=');
	}
	s_jw_putc(w, '"');
}

// --- files, handler-safe --------------------------------------------------------------------------

#if defined(_WIN32)
static void s_utf8_to_wide(const char* s, wchar_t* out, int cap)
{
	int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, out, cap);
	if (n <= 0 && cap > 0) out[0] = 0;
}

static void s_wide_to_utf8(const wchar_t* s, char* out, int cap)
{
	int n = WideCharToMultiByte(CP_UTF8, 0, s, -1, out, cap, NULL, NULL);
	if (n <= 0 && cap > 0) out[0] = 0;
}
#endif

// Writes a whole file. Only system calls; the path is ASCII past the (pre-converted) directory.
static bool s_file_write(const char* path, const void* data, size_t len)
{
#if defined(_WIN32)
	wchar_t wpath[CC_PATH];
	s_utf8_to_wide(path, wpath, CC_PATH);
	HANDLE h = CreateFileW(wpath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) return false;
	bool ok = true;
	const char* p = (const char*)data;
	while (len) {
		DWORD chunk = len > 0x7fffffff ? 0x7fffffff : (DWORD)len, wrote = 0;
		if (!WriteFile(h, p, chunk, &wrote, NULL) || wrote == 0) { ok = false; break; }
		p += wrote; len -= wrote;
	}
	CloseHandle(h);
	return ok;
#else
	int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) return false;
	bool ok = true;
	const char* p = (const char*)data;
	while (len) {
		ssize_t wrote = write(fd, p, len);
		if (wrote < 0 && errno == EINTR) continue;
		if (wrote <= 0) { ok = false; break; }
		p += wrote; len -= (size_t)wrote;
	}
	close(fd);
	return ok;
#endif
}

static bool s_file_exists(const char* path)
{
#if defined(_WIN32)
	wchar_t wpath[CC_PATH];
	s_utf8_to_wide(path, wpath, CC_PATH);
	return GetFileAttributesW(wpath) != INVALID_FILE_ATTRIBUTES;
#else
	struct stat st;
	return stat(path, &st) == 0;
#endif
}

// Environment, without the CRT's deprecation theatre on Windows.
static bool s_env_is(const char* name, char value)
{
#if defined(_WIN32)
	char buf[8];
	DWORD n = GetEnvironmentVariableA(name, buf, sizeof(buf));
	return n > 0 && n < sizeof(buf) && buf[0] == value;
#else
	const char* v = getenv(name);
	return v && v[0] == value;
#endif
}

static void s_file_delete(const char* path)
{
#if defined(_WIN32)
	wchar_t wpath[CC_PATH];
	s_utf8_to_wide(path, wpath, CC_PATH);
	DeleteFileW(wpath);
#else
	unlink(path);
#endif
}

// Not handler-safe (malloc): for the resolve and upload paths.
static char* s_file_read(const char* path, size_t* out_len)
{
	FILE* f = NULL;
#if defined(_WIN32)
	wchar_t wpath[CC_PATH];
	s_utf8_to_wide(path, wpath, CC_PATH);
	if (_wfopen_s(&f, wpath, L"rb") != 0) f = NULL;
#else
	f = fopen(path, "rb");
#endif
	if (!f) return NULL;
	fseek(f, 0, SEEK_END);
	long n = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (n < 0) { fclose(f); return NULL; }
	char* buf = (char*)malloc((size_t)n + 1);
	if (!buf) { fclose(f); return NULL; }
	size_t got = fread(buf, 1, (size_t)n, f);
	fclose(f);
	buf[got] = 0;
	if (out_len) *out_len = got;
	return buf;
}

static void s_mkdir_p(const char* path)
{
	char tmp[CC_PATH];
	s_strcpy(tmp, CC_PATH, path);
	size_t n = s_strlen(tmp);
	for (size_t i = 1; i < n; ++i) {
		if (tmp[i] == '/' || tmp[i] == '\\') {
			char c = tmp[i];
			tmp[i] = 0;
#if defined(_WIN32)
			{ wchar_t w[CC_PATH]; s_utf8_to_wide(tmp, w, CC_PATH); CreateDirectoryW(w, NULL); }
#else
			mkdir(tmp, 0755);
#endif
			tmp[i] = c;
		}
	}
#if defined(_WIN32)
	{ wchar_t w[CC_PATH]; s_utf8_to_wide(tmp, w, CC_PATH); CreateDirectoryW(w, NULL); }
#else
	mkdir(tmp, 0755);
#endif
}

// Calls fn(path, udata) for each file in `dir` whose name ends with `suffix`. Not handler-safe.
static void s_dir_each(const char* dir, const char* suffix, void (*fn)(const char* path, void* udata), void* udata)
{
	size_t sl = s_strlen(suffix);
#if defined(_WIN32)
	wchar_t pattern[CC_PATH];
	WIN32_FIND_DATAW fd;
	char glob[CC_PATH];
	s_strcpy(glob, CC_PATH, dir);
	s_strcat(glob, CC_PATH, "*");
	s_utf8_to_wide(glob, pattern, CC_PATH);
	HANDLE h = FindFirstFileW(pattern, &fd);
	if (h == INVALID_HANDLE_VALUE) return;
	do {
		char name[CC_PATH], path[CC_PATH];
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
		s_wide_to_utf8(fd.cFileName, name, CC_PATH);
		size_t nl = s_strlen(name);
		if (nl < sl || strcmp(name + nl - sl, suffix) != 0) continue;
		s_strcpy(path, CC_PATH, dir);
		s_strcat(path, CC_PATH, name);
		fn(path, udata);
	} while (FindNextFileW(h, &fd));
	FindClose(h);
#else
	DIR* d = opendir(dir);
	if (!d) return;
	struct dirent* e;
	while ((e = readdir(d)) != NULL) {
		char path[CC_PATH];
		size_t nl = s_strlen(e->d_name);
		if (nl < sl || strcmp(e->d_name + nl - sl, suffix) != 0) continue;
		s_strcpy(path, CC_PATH, dir);
		s_strcat(path, CC_PATH, e->d_name);
		fn(path, udata);
	}
	closedir(d);
#endif
}

// CC_DEBUG=1: the library's own notes, appended to <report_dir>/cc_debug.log. Not for handlers.
static void s_debugf(const char* fmt, ...)
{
	if (!s_env_is("CC_DEBUG", '1')) return;
	char path[CC_PATH], line[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(line, sizeof(line), fmt, args);
	va_end(args);
	s_strcpy(path, CC_PATH, s_cc.report_dir);
	s_strcat(path, CC_PATH, "cc_debug.log");
	FILE* f = NULL;
#if defined(_WIN32)
	wchar_t w[CC_PATH];
	s_utf8_to_wide(path, w, CC_PATH);
	if (_wfopen_s(&f, w, L"ab") != 0) f = NULL;
#else
	f = fopen(path, "ab");
#endif
	if (!f) return;
	fprintf(f, "[%u] %s\n", (unsigned)s_current_pid(), line);
	fclose(f);
}

// --- breadcrumbs, state, heartbeat: any thread, no locks ------------------------------------------

static double s_uptime(void) { return (double)(s_now_ns() - s_cc.sh->start_ns) / 1.0e9; }

void cc_breadcrumb(const char* fmt, ...)
{
	if (!s_cc.sh) return;
	uint32_t i = s_atomic_inc32(&s_cc.sh->crumb_next) % CC_BREADCRUMBS;
	cc_crumb* c = &s_cc.sh->crumbs[i];
	va_list args;
	va_start(args, fmt);
	vsnprintf(c->msg, CC_BREADCRUMB_BYTES, fmt, args);
	va_end(args);
	c->msg[CC_BREADCRUMB_BYTES - 1] = 0;
	c->t = s_uptime();
}

// A slot is claimed by writing its key under an odd sequence number and published under an even one;
// a reader that sees an odd number, or a number that changed while it copied, skips the slot.
void cc_set(const char* key, const char* value)
{
	if (!s_cc.sh || !key || !key[0]) return;
	cc_slot* free_slot = NULL;
	for (int i = 0; i < CC_STATE_SLOTS; ++i) {
		cc_slot* s = &s_cc.sh->state[i];
		if (s->key[0] == 0) { if (!free_slot) free_slot = s; continue; }
		if (s_streq(s->key, key)) { free_slot = s; break; }
	}
	if (!free_slot) return;
	uint32_t seq = s_atomic_load32(&free_slot->seq);
	s_atomic_store32(&free_slot->seq, seq | 1);
	if (value && value[0]) {
		s_strcpy(free_slot->key, CC_STATE_KEY, key);
		s_strcpy(free_slot->value, CC_STATE_VALUE, value);
	} else {
		free_slot->value[0] = 0;
		free_slot->key[0] = 0;
	}
	s_atomic_store32(&free_slot->seq, (seq | 1) + 1);
}

static const char* s_state_get(const char* key)
{
	for (int i = 0; i < CC_STATE_SLOTS; ++i) {
		cc_slot* s = &s_cc.sh->state[i];
		if ((s_atomic_load32(&s->seq) & 1) == 0 && s_streq(s->key, key)) return s->value;
	}
	return NULL;
}

#if !defined(_WIN32)
static void s_modules_refresh(void);
#endif

void cc_heartbeat(void)
{
	if (!s_cc.sh) return;
#if !defined(_WIN32)
	s_modules_refresh();
#endif
	s_atomic_store64(&s_cc.sh->heartbeat_ns, s_now_ns());
	if (s_cc.hang_reported && s_atomic_cas32(&s_cc.hang_reported, 1, 0)) {
		cc_breadcrumb("cute_crash: recovered from the reported hang");
	}
}

void cc_hang_pause(void) { if (s_cc.sh) s_atomic_inc32(&s_cc.sh->pause_depth); }

void cc_hang_resume(void)
{
	if (!s_cc.sh) return;
	uint32_t d = s_atomic_load32(&s_cc.sh->pause_depth);
	if (d > 0) s_atomic_store32(&s_cc.sh->pause_depth, d - 1);
	s_atomic_store64(&s_cc.sh->heartbeat_ns, s_now_ns());
}

// The thread registry: CC_MAX_THREADS slots, reused. A slot is claimed by CAS (0 -> CLAIMED), named,
// then published under its tid; a thread's exit clears it back to 0. Readers match on the tid
// alone, so a slot cleared or claimed under them is simply a miss.
static const char* s_thread_name(uint32_t tid)
{
	for (uint32_t i = 0; i < CC_MAX_THREADS; ++i) {
		if (s_atomic_load32(&s_cc.sh->threads[i].tid) == tid) return s_cc.sh->threads[i].name;
	}
	return "";
}

static void s_thread_register(const char* name)
{
	uint32_t tid = s_current_tid();
	for (uint32_t i = 0; i < CC_MAX_THREADS; ++i) {
		if (s_atomic_load32(&s_cc.sh->threads[i].tid) == tid) { s_strcpy(s_cc.sh->threads[i].name, 32, name ? name : ""); return; }
	}
	for (uint32_t i = 0; i < CC_MAX_THREADS; ++i) {
		if (!s_atomic_cas32(&s_cc.sh->threads[i].tid, 0, CC_TID_CLAIMED)) continue;
		s_strcpy(s_cc.sh->threads[i].name, 32, name ? name : "");
		s_atomic_store32(&s_cc.sh->threads[i].tid, tid);
		return;
	}
}

static void s_thread_unregister(uint32_t tid)
{
	for (uint32_t i = 0; i < CC_MAX_THREADS; ++i) {
		if (s_atomic_load32(&s_cc.sh->threads[i].tid) != tid) continue;
		s_cc.sh->threads[i].name[0] = 0;
		s_atomic_store32(&s_cc.sh->threads[i].tid, 0);
		return;
	}
}

// --- modules -------------------------------------------------------------------------------------
// A frame is (module, offset): offset = pc - offset_base, which is the table's address space. The
// build id is read from the mapped image: PE CodeView record, Mach-O LC_UUID, ELF build-id note.

static bool s_is_system_path(const char* path)
{
#if defined(_WIN32)
	static char windir[CC_PATH];
	if (!windir[0]) {
		wchar_t w[CC_PATH];
		UINT n = GetWindowsDirectoryW(w, CC_PATH);
		if (n && n < CC_PATH) s_wide_to_utf8(w, windir, CC_PATH); else s_strcpy(windir, CC_PATH, "C:\\Windows");
	}
	return s_starts_with_nocase(path, windir);
#elif defined(__APPLE__)
	return s_starts_with_nocase(path, "/usr/lib/") || s_starts_with_nocase(path, "/System/") || s_starts_with_nocase(path, "/usr/libexec/");
#else
	return s_starts_with_nocase(path, "/lib") || s_starts_with_nocase(path, "/usr/lib") || s_starts_with_nocase(path, "linux-vdso") || s_starts_with_nocase(path, "/usr/local/lib");
#endif
}

static cc_module* s_module_add(void)
{
	if (s_cc.module_count >= CC_MAX_MODULES) return NULL;
	cc_module* m = &s_cc.modules[s_cc.module_count++];
	memset(m, 0, sizeof(*m));
	return m;
}

static void s_module_finish(cc_module* m)
{
	m->name_at = (int)(s_basename(m->path) - m->path);
	m->system = s_is_system_path(m->path);
}

#if defined(_WIN32)

static bool s_cv_build_id(const unsigned char* cv, cc_module* m);

// The PE's CodeView debug entry, from the mapped image. Build id = GUID in printed order + age big-endian.
static void s_pe_build_id(const unsigned char* base, cc_module* m)
{
	const IMAGE_DOS_HEADER* dos = (const IMAGE_DOS_HEADER*)base;
	if (dos->e_magic != IMAGE_DOS_SIGNATURE) return;
	const IMAGE_NT_HEADERS* nt = (const IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
	if (nt->Signature != IMAGE_NT_SIGNATURE) return;
	m->size = nt->OptionalHeader.SizeOfImage;
	const IMAGE_DATA_DIRECTORY* dir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
	if (!dir->VirtualAddress || !dir->Size) return;
	const IMAGE_DEBUG_DIRECTORY* dbg = (const IMAGE_DEBUG_DIRECTORY*)(base + dir->VirtualAddress);
	int count = (int)(dir->Size / sizeof(IMAGE_DEBUG_DIRECTORY));
	for (int i = 0; i < count; ++i) {
		if (dbg[i].Type != IMAGE_DEBUG_TYPE_CODEVIEW || !dbg[i].AddressOfRawData || dbg[i].SizeOfData < 24) continue;
		const unsigned char* cv = base + dbg[i].AddressOfRawData;
		if (s_cv_build_id(cv, m)) return;
	}
}

// The 24-byte RSDS record: 'RSDS', GUID, age.
static bool s_cv_build_id(const unsigned char* cv, cc_module* m)
{
	if (memcmp(cv, "RSDS", 4) != 0) return false;
	{
		const unsigned char* g = cv + 4;
		uint32_t d1 = (uint32_t)g[0] | ((uint32_t)g[1] << 8) | ((uint32_t)g[2] << 16) | ((uint32_t)g[3] << 24);
		uint16_t d2 = (uint16_t)(g[4] | (g[5] << 8)), d3 = (uint16_t)(g[6] | (g[7] << 8));
		uint32_t age = (uint32_t)cv[20] | ((uint32_t)cv[21] << 8) | ((uint32_t)cv[22] << 16) | ((uint32_t)cv[23] << 24);
		unsigned char* b = m->build_id;
		b[0] = (unsigned char)(d1 >> 24); b[1] = (unsigned char)(d1 >> 16); b[2] = (unsigned char)(d1 >> 8); b[3] = (unsigned char)d1;
		b[4] = (unsigned char)(d2 >> 8); b[5] = (unsigned char)d2;
		b[6] = (unsigned char)(d3 >> 8); b[7] = (unsigned char)d3;
		memcpy(b + 8, g + 8, 8);
		b[16] = (unsigned char)(age >> 24); b[17] = (unsigned char)(age >> 16); b[18] = (unsigned char)(age >> 8); b[19] = (unsigned char)age;
		m->build_id_len = 20;
		return true;
	}
}

// The loader's own list, read without a lock: the only module enumeration that is safe to run on
// a thread that may already hold the loader lock.
static void s_modules_enumerate(void)
{
	s_cc.module_count = 0;
	PEB* peb = NtCurrentTeb()->ProcessEnvironmentBlock;
	PEB_LDR_DATA* ldr = peb->Ldr;
	LIST_ENTRY* head = &ldr->InMemoryOrderModuleList;
	int guard = 0;
	for (LIST_ENTRY* e = head->Flink; e && e != head && guard++ < 4096; e = e->Flink) {
		LDR_DATA_TABLE_ENTRY* entry = CONTAINING_RECORD(e, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
		if (!entry->DllBase) continue;
		cc_module* m = s_module_add();
		if (!m) break;
		m->base = (uint64_t)(uintptr_t)entry->DllBase;
		m->offset_base = m->base;
		if (entry->FullDllName.Buffer && entry->FullDllName.Length) {
			wchar_t w[CC_MODULE_PATH];
			int n = entry->FullDllName.Length / 2;
			if (n >= CC_MODULE_PATH) n = CC_MODULE_PATH - 1;
			memcpy(w, entry->FullDllName.Buffer, (size_t)n * 2);
			w[n] = 0;
			s_wide_to_utf8(w, m->path, CC_MODULE_PATH);
		}
		s_pe_build_id((const unsigned char*)entry->DllBase, m);
		s_module_finish(m);
	}
}

#elif defined(__APPLE__)

// Unverified on this host: the dyld image list, read at init and refreshed on the heartbeat, so the
// handler never calls into dyld.
static void s_modules_enumerate(void)
{
	s_cc.module_count = 0;
	uint32_t count = _dyld_image_count();
	for (uint32_t i = 0; i < count; ++i) {
		const struct mach_header* mh = _dyld_get_image_header(i);
		const char* name = _dyld_get_image_name(i);
		intptr_t slide = _dyld_get_image_vmaddr_slide(i);
		if (!mh) continue;
		cc_module* m = s_module_add();
		if (!m) break;
		m->base = (uint64_t)(uintptr_t)mh;
		m->offset_base = (uint64_t)slide;
		s_strcpy(m->path, CC_MODULE_PATH, name ? name : "");
		if (mh->magic == MH_MAGIC_64) {
			const struct mach_header_64* h = (const struct mach_header_64*)mh;
			const unsigned char* p = (const unsigned char*)(h + 1);
			uint64_t vmsize = 0;
			for (uint32_t c = 0; c < h->ncmds; ++c) {
				const struct load_command* lc = (const struct load_command*)p;
				if (lc->cmd == LC_UUID) { memcpy(m->build_id, ((const struct uuid_command*)lc)->uuid, 16); m->build_id_len = 16; }
				else if (lc->cmd == LC_SEGMENT_64) {
					const struct segment_command_64* seg = (const struct segment_command_64*)lc;
					if (strcmp(seg->segname, "__PAGEZERO") != 0) vmsize += seg->vmsize;
				}
				p += lc->cmdsize;
			}
			m->size = vmsize;
		}
		s_module_finish(m);
	}
}

#else

// Unverified on this host: dl_iterate_phdr, read at init and refreshed on the heartbeat.
static int s_phdr_callback(struct dl_phdr_info* info, size_t size, void* udata)
{
	(void)size; (void)udata;
	cc_module* m = s_module_add();
	if (!m) return 1;
	uint64_t lo = UINT64_MAX, hi = 0;
	for (int i = 0; i < info->dlpi_phnum; ++i) {
		const ElfW(Phdr)* ph = &info->dlpi_phdr[i];
		if (ph->p_type == PT_LOAD) {
			uint64_t a = info->dlpi_addr + ph->p_vaddr;
			if (a < lo) lo = a;
			if (a + ph->p_memsz > hi) hi = a + ph->p_memsz;
		} else if (ph->p_type == PT_NOTE) {
			const unsigned char* p = (const unsigned char*)(info->dlpi_addr + ph->p_vaddr);
			const unsigned char* end = p + ph->p_memsz;
			while (p + sizeof(ElfW(Nhdr)) <= end) {
				const ElfW(Nhdr)* nh = (const ElfW(Nhdr)*)p;
				const unsigned char* name = p + sizeof(ElfW(Nhdr));
				const unsigned char* desc = name + ((nh->n_namesz + 3) & ~3u);
				if (nh->n_type == NT_GNU_BUILD_ID && nh->n_namesz == 4 && memcmp(name, "GNU", 4) == 0 && desc + nh->n_descsz <= end) {
					int n = nh->n_descsz > 20 ? 20 : (int)nh->n_descsz;
					memcpy(m->build_id, desc, (size_t)n);
					m->build_id_len = n;
					break;
				}
				p = desc + ((nh->n_descsz + 3) & ~3u);
			}
		}
	}
	m->base = lo == UINT64_MAX ? info->dlpi_addr : lo;
	m->size = hi > lo ? hi - lo : 0;
	m->offset_base = info->dlpi_addr;
	if (info->dlpi_name && info->dlpi_name[0]) s_strcpy(m->path, CC_MODULE_PATH, info->dlpi_name);
	else s_strcpy(m->path, CC_MODULE_PATH, s_cc.exe_path);
	s_module_finish(m);
	return 0;
}

static void s_modules_enumerate(void)
{
	s_cc.module_count = 0;
	dl_iterate_phdr(s_phdr_callback, NULL);
}

#endif

static int s_module_of(uint64_t pc)
{
	for (int i = 0; i < s_cc.module_count; ++i) {
		const cc_module* m = &s_cc.modules[i];
		if (pc >= m->base && pc < m->base + m->size) return i;
	}
	return -1;
}

// --- machine, once at init -----------------------------------------------------------------------

static void s_machine_init(void)
{
#if defined(_WIN32)
	{
		typedef LONG (WINAPI *rtl_get_version_fn)(PRTL_OSVERSIONINFOW);
		HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
		rtl_get_version_fn rgv = ntdll ? (rtl_get_version_fn)(void*)GetProcAddress(ntdll, "RtlGetVersion") : NULL;
		RTL_OSVERSIONINFOW v;
		memset(&v, 0, sizeof(v));
		v.dwOSVersionInfoSize = sizeof(v);
		if (rgv && rgv(&v) == 0) {
			char b[24];
			s_strcpy(s_cc.os, sizeof(s_cc.os), v.dwMajorVersion >= 10 && v.dwBuildNumber >= 22000 ? "Windows 11 " : "Windows ");
			s_utoa(v.dwMajorVersion, b); s_strcat(s_cc.os, sizeof(s_cc.os), b); s_strcat(s_cc.os, sizeof(s_cc.os), ".");
			s_utoa(v.dwMinorVersion, b); s_strcat(s_cc.os, sizeof(s_cc.os), b); s_strcat(s_cc.os, sizeof(s_cc.os), ".");
			s_utoa(v.dwBuildNumber, b); s_strcat(s_cc.os, sizeof(s_cc.os), b);
		} else {
			s_strcpy(s_cc.os, sizeof(s_cc.os), "Windows");
		}
	}
	{
#if defined(_M_X64) || defined(_M_IX86)
		int regs[4];
		char brand[49];
		memset(brand, 0, sizeof(brand));
		__cpuid(regs, 0x80000000);
		if ((unsigned)regs[0] >= 0x80000004u) {
			for (int i = 0; i < 3; ++i) { __cpuid(regs, (int)(0x80000002u + (unsigned)i)); memcpy(brand + i * 16, regs, 16); }
		}
		const char* b = brand;
		while (*b == ' ') ++b;
		s_strcpy(s_cc.cpu, sizeof(s_cc.cpu), b[0] ? b : "x86");
#else
		s_strcpy(s_cc.cpu, sizeof(s_cc.cpu), "arm64");
#endif
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		s_cc.cpu_threads = (int)si.dwNumberOfProcessors;
		MEMORYSTATUSEX ms;
		ms.dwLength = sizeof(ms);
		if (GlobalMemoryStatusEx(&ms)) s_cc.ram_mb = ms.ullTotalPhys / (1024 * 1024);
	}
#if defined(_M_X64)
	s_strcpy(s_cc.arch, sizeof(s_cc.arch), "x86_64");
#elif defined(_M_ARM64)
	s_strcpy(s_cc.arch, sizeof(s_cc.arch), "aarch64");
#else
	s_strcpy(s_cc.arch, sizeof(s_cc.arch), "x86");
#endif
#elif defined(__APPLE__)
	{
		// Unverified on this host.
		char buf[128];
		size_t n = sizeof(buf);
		if (sysctlbyname("kern.osproductversion", buf, &n, NULL, 0) == 0) { s_strcpy(s_cc.os, sizeof(s_cc.os), "macOS "); s_strcat(s_cc.os, sizeof(s_cc.os), buf); }
		else s_strcpy(s_cc.os, sizeof(s_cc.os), "macOS");
		n = sizeof(buf);
		if (sysctlbyname("machdep.cpu.brand_string", buf, &n, NULL, 0) == 0) s_strcpy(s_cc.cpu, sizeof(s_cc.cpu), buf);
		int threads = 0; n = sizeof(threads);
		if (sysctlbyname("hw.logicalcpu", &threads, &n, NULL, 0) == 0) s_cc.cpu_threads = threads;
		uint64_t mem = 0; n = sizeof(mem);
		if (sysctlbyname("hw.memsize", &mem, &n, NULL, 0) == 0) s_cc.ram_mb = mem / (1024 * 1024);
	}
#if defined(__aarch64__)
	s_strcpy(s_cc.arch, sizeof(s_cc.arch), "aarch64");
#else
	s_strcpy(s_cc.arch, sizeof(s_cc.arch), "x86_64");
#endif
#else
	{
		// Unverified on this host.
		struct utsname u;
		if (uname(&u) == 0) { s_strcpy(s_cc.os, sizeof(s_cc.os), u.sysname); s_strcat(s_cc.os, sizeof(s_cc.os), " "); s_strcat(s_cc.os, sizeof(s_cc.os), u.release); }
		else s_strcpy(s_cc.os, sizeof(s_cc.os), "Linux");
		FILE* f = fopen("/proc/cpuinfo", "r");
		if (f) {
			char line[256];
			while (fgets(line, sizeof(line), f)) {
				if (strncmp(line, "model name", 10) == 0) {
					const char* c = strchr(line, ':');
					if (c) { ++c; while (*c == ' ') ++c; s_strcpy(s_cc.cpu, sizeof(s_cc.cpu), c); size_t l = s_strlen(s_cc.cpu); if (l && s_cc.cpu[l - 1] == '\n') s_cc.cpu[l - 1] = 0; }
					break;
				}
			}
			fclose(f);
		}
		s_cc.cpu_threads = (int)sysconf(_SC_NPROCESSORS_ONLN);
		s_cc.ram_mb = (uint64_t)sysconf(_SC_PHYS_PAGES) * (uint64_t)sysconf(_SC_PAGESIZE) / (1024 * 1024);
	}
#if defined(__aarch64__)
	s_strcpy(s_cc.arch, sizeof(s_cc.arch), "aarch64");
#elif defined(__x86_64__)
	s_strcpy(s_cc.arch, sizeof(s_cc.arch), "x86_64");
#else
	s_strcpy(s_cc.arch, sizeof(s_cc.arch), "x86");
#endif
#endif
}

// --- the report ----------------------------------------------------------------------------------

// crash-<stamp>-<pid>.json and .dmp, in the report directory.
static void s_report_paths(void)
{
	char stamp[16], pid[24];
	s_stamp_time(s_now_unix(), stamp);
	s_utoa(s_current_pid(), pid);
	s_strcpy(s_cc.report_path, CC_PATH, s_cc.report_dir);
	s_strcat(s_cc.report_path, CC_PATH, "crash-");
	s_strcat(s_cc.report_path, CC_PATH, stamp);
	s_strcat(s_cc.report_path, CC_PATH, "-");
	s_strcat(s_cc.report_path, CC_PATH, pid);
	s_strcpy(s_cc.dump_path, CC_PATH, s_cc.report_path);
	s_strcat(s_cc.report_path, CC_PATH, ".json");
	s_strcat(s_cc.dump_path, CC_PATH, ".dmp");
}

// A uuid-shaped id from the entropy pool, the clock, and the pid: unique, not secret.
static void s_report_id(char out[40])
{
	cc_sha1 h;
	unsigned char d[20];
	int64_t t = s_now_ns();
	uint32_t pid = s_current_pid();
	s_sha1_init(&h);
	s_sha1_update(&h, s_cc.entropy, sizeof(s_cc.entropy));
	s_sha1_update(&h, &t, sizeof(t));
	s_sha1_update(&h, &pid, sizeof(pid));
	s_sha1_final(&h, d);
	char hex[41];
	s_hex_bytes(d, 16, hex);
	int o = 0;
	for (int i = 0; i < 32; ++i) {
		if (i == 8 || i == 12 || i == 16 || i == 20) out[o++] = '-';
		out[o++] = hex[i];
	}
	out[o] = 0;
}

// Every report carries one: the kind, the fault's name, and the top frames as (build id, offset);
// a report with no stack (abnormal_exit) is keyed by the main module's build id instead, so a
// server can still group it by build.
static void s_signature_raw(const char* kind, const char* name, const uint64_t* pcs, int count, char out[41])
{
	cc_sha1 h;
	unsigned char d[20];
	s_sha1_init(&h);
	s_sha1_str(&h, kind ? kind : "");
	s_sha1_str(&h, "|");
	if (name) { s_sha1_str(&h, name); s_sha1_str(&h, "|"); }
	if (count == 0 && s_cc.module_count > 0) {
		char hex[48];
		s_hex_bytes(s_cc.modules[0].build_id, s_cc.modules[0].build_id_len, hex);
		s_sha1_str(&h, hex);
	}
	int used = 0;
	for (int i = 0; i < count && used < CC_SIGNATURE_FRAMES; ++i) {
		int mi = s_module_of(pcs[i]);
		if (mi < 0 || s_cc.modules[mi].system) continue;
		char hex[48], num[24];
		s_hex_bytes(s_cc.modules[mi].build_id, s_cc.modules[mi].build_id_len, hex);
		s_utoa(pcs[i] - s_cc.modules[mi].offset_base, num);
		s_sha1_str(&h, hex); s_sha1_str(&h, ":"); s_sha1_str(&h, num); s_sha1_str(&h, ";");
		++used;
	}
	s_sha1_final(&h, d);
	s_hex_bytes(d, 20, out);
}

static void s_write_frames(cc_jw* w, const uint64_t* pcs, int count)
{
	s_jw_begin_arr(w);
	for (int i = 0; i < count; ++i) {
		int mi = s_module_of(pcs[i]);
		s_jw_begin_obj(w);
		s_jw_kv_int(w, "module", mi);
		s_jw_kv_uint(w, "offset", mi >= 0 ? pcs[i] - s_cc.modules[mi].offset_base : pcs[i]);
		s_jw_kv_hex(w, "pc", pcs[i]);
		s_jw_end_obj(w);
	}
	s_jw_end_arr(w);
}

static void s_write_machine_value(cc_jw* w, const char* key)
{
	const char* v = s_state_get(key);
	if (v) s_jw_kv_str(w, key, v);
}

// The whole document. `pcs` is the stack of the thread the report is about (none for abnormal_exit);
// `stack`/`stack_len` the raw stack copy (POSIX). Returns the byte count written to s_cc.report.
static size_t s_write_report_json(const cc_fault* f, const uint64_t* pcs, int count, const unsigned char* stack, size_t stack_len)
{
	cc_jw w;
	char buf[64];
	s_jw_init(&w, s_cc.report, CC_REPORT_BYTES);
	s_jw_begin_obj(&w);
	s_jw_kv_int(&w, "format", 1);
	s_report_id(buf);
	s_jw_kv_str(&w, "id", buf);
	s_jw_kv_str(&w, "kind", f->kind);
	if (f->message) s_jw_kv_str(&w, "message", f->message);
	s_jw_kv_str(&w, "app", s_cc.app);
	if (s_cc.version[0]) s_jw_kv_str(&w, "version", s_cc.version);
	if (s_cc.build[0]) s_jw_kv_str(&w, "build", s_cc.build);
	if (s_cc.config_name[0]) s_jw_kv_str(&w, "config", s_cc.config_name);
	s_iso_time(s_now_unix(), buf);
	s_jw_kv_str(&w, "time", buf);
	s_jw_key(&w, "uptime"); s_jw_float(&w, s_uptime(), 2);
	if (s_cc.install[0]) s_jw_kv_str(&w, "install", s_cc.install);

	s_jw_key(&w, "machine");
	s_jw_begin_obj(&w);
	s_jw_kv_str(&w, "os", s_cc.os);
	s_jw_kv_str(&w, "arch", s_cc.arch);
	s_jw_kv_str(&w, "cpu", s_cc.cpu);
	s_jw_kv_int(&w, "threads", s_cc.cpu_threads);
	s_jw_kv_uint(&w, "ram_mb", s_cc.ram_mb);
	s_write_machine_value(&w, "gpu");
	s_write_machine_value(&w, "gpu_driver");
	s_write_machine_value(&w, "backend");
	s_write_machine_value(&w, "display");
	s_write_machine_value(&w, "window");
	{
		const char* fs = s_state_get("fullscreen");
		if (fs) s_jw_kv_bool(&w, "fullscreen", s_streq(fs, "true") || s_streq(fs, "1"));
	}
	s_jw_end_obj(&w);

	if (!s_streq(f->kind, "abnormal_exit") && !s_streq(f->kind, "report")) {
		s_jw_key(&w, "fault");
		s_jw_begin_obj(&w);
		if (s_streq(f->kind, "hang")) {
			s_jw_kv_int(&w, "thread", f->tid);
			s_jw_kv_str(&w, "thread_name", s_thread_name(f->tid));
			s_jw_key(&w, "seconds"); s_jw_float(&w, f->hang_seconds, 1);
		} else {
#if defined(_WIN32)
			s_jw_kv_str(&w, "exception", f->name ? f->name : "");
			if (f->code) s_jw_kv_hex(&w, "code", f->code);
#else
			s_jw_kv_str(&w, "signal", f->name ? f->name : "");
			if (f->code_name) s_jw_kv_str(&w, "code", f->code_name);
#endif
			if (f->access) s_jw_kv_str(&w, "access", f->access);
			if (f->has_address) s_jw_kv_hex(&w, "address", f->address);
			s_jw_kv_int(&w, "thread", f->tid);
			s_jw_kv_str(&w, "thread_name", s_thread_name(f->tid));
		}
		s_jw_end_obj(&w);
	}

	s_jw_key(&w, "modules");
	s_jw_begin_arr(&w);
	for (int i = 0; i < s_cc.module_count; ++i) {
		const cc_module* m = &s_cc.modules[i];
		char hex[48];
		s_hex_bytes(m->build_id, m->build_id_len, hex);
		s_jw_begin_obj(&w);
		s_jw_kv_str(&w, "name", m->path + m->name_at);
		s_jw_kv_str(&w, "path", m->path);
		s_jw_kv_hex(&w, "base", m->base);
		s_jw_kv_uint(&w, "size", m->size);
		s_jw_kv_str(&w, "build_id", hex);
		s_jw_kv_str(&w, "arch", s_cc.arch);
		if (m->system) s_jw_kv_bool(&w, "system", true);
		s_jw_end_obj(&w);
	}
	s_jw_end_arr(&w);

	if (pcs) {
		s_jw_key(&w, "stack");
		s_write_frames(&w, pcs, count);
	}

	s_jw_key(&w, "signature");
	s_jw_begin_obj(&w);
	{
		char sig[41];
		s_signature_raw(f->kind, f->name ? f->name : f->message, pcs, pcs ? count : 0, sig);
		s_jw_kv_str(&w, "raw", sig);
	}
	s_jw_end_obj(&w);

	s_jw_key(&w, "state");
	s_jw_begin_obj(&w);
	for (int i = 0; i < CC_STATE_SLOTS; ++i) {
		cc_slot* s = &s_cc.sh->state[i];
		if ((s_atomic_load32(&s->seq) & 1) || !s->key[0]) continue;
		s_jw_kv_str(&w, s->key, s->value);
	}
	s_jw_end_obj(&w);

	s_jw_key(&w, "breadcrumbs");
	s_jw_begin_arr(&w);
	{
		uint32_t next = s_atomic_load32(&s_cc.sh->crumb_next);
		uint32_t n = next < CC_BREADCRUMBS ? next : CC_BREADCRUMBS;
		uint32_t first = next - n;
		for (uint32_t i = 0; i < n; ++i) {
			const cc_crumb* c = &s_cc.sh->crumbs[(first + i) % CC_BREADCRUMBS];
			if (!c->msg[0]) continue;
			s_jw_begin_obj(&w);
			s_jw_key(&w, "t"); s_jw_float(&w, c->t, 2);
			s_jw_kv_str(&w, "msg", c->msg);
			s_jw_end_obj(&w);
		}
	}
	s_jw_end_arr(&w);

	if (s_cc.has_dump) {
		s_jw_key(&w, "attachments");
		s_jw_begin_arr(&w);
		s_jw_begin_obj(&w);
		s_jw_kv_str(&w, "name", s_basename(s_cc.dump_path));
		s_jw_kv_str(&w, "type", "minidump");
		s_jw_kv_uint(&w, "bytes", s_cc.dump_bytes);
		s_jw_end_obj(&w);
		s_jw_end_arr(&w);
	}

	if (stack && stack_len) {
		s_jw_key(&w, "stack_bytes");
		s_jw_base64(&w, stack, stack_len);
	}

	s_jw_end_obj(&w);
	s_jw_putc(&w, '\n');
	return s_jw_len(&w, s_cc.report);
}

static bool s_write_report(const cc_fault* f, const uint64_t* pcs, int count, const unsigned char* stack, size_t stack_len)
{
	size_t n = s_write_report_json(f, pcs, count, stack, stack_len);
	return s_file_write(s_cc.report_path, s_cc.report, n);
}

// --- the marker: this run is alive; its mirror of state and breadcrumbs is all an abnormal_exit knows.

#define CC_MARKER_BYTES (16 * 1024)

static void s_marker_write(void)
{
	cc_jw w;
	char buf[32];
	s_jw_init(&w, s_cc.marker, CC_MARKER_BYTES);
	s_jw_begin_obj(&w);
	s_jw_kv_int(&w, "start", s_cc.sh->start_unix);
	s_jw_kv_uint(&w, "pid", s_current_pid());
	s_iso_time(s_now_unix(), buf);
	s_jw_kv_str(&w, "written", buf);
	s_jw_key(&w, "uptime"); s_jw_float(&w, s_uptime(), 2);
	s_jw_key(&w, "state");
	s_jw_begin_obj(&w);
	for (int i = 0; i < CC_STATE_SLOTS; ++i) {
		cc_slot* s = &s_cc.sh->state[i];
		if ((s_atomic_load32(&s->seq) & 1) || !s->key[0]) continue;
		s_jw_kv_str(&w, s->key, s->value);
	}
	s_jw_end_obj(&w);
	s_jw_key(&w, "breadcrumbs");
	s_jw_begin_arr(&w);
	{
		uint32_t next = s_atomic_load32(&s_cc.sh->crumb_next);
		uint32_t n = next < CC_MARKER_CRUMBS ? next : CC_MARKER_CRUMBS;
		for (uint32_t i = next - n; i < next; ++i) {
			const cc_crumb* c = &s_cc.sh->crumbs[i % CC_BREADCRUMBS];
			if (!c->msg[0]) continue;
			s_jw_begin_obj(&w);
			s_jw_key(&w, "t"); s_jw_float(&w, c->t, 2);
			s_jw_kv_str(&w, "msg", c->msg);
			s_jw_end_obj(&w);
		}
	}
	s_jw_end_arr(&w);
	s_jw_end_obj(&w);
	s_file_write(s_cc.marker_path, s_cc.marker, s_jw_len(&w, s_cc.marker));
	s_cc.marker_written_ns = s_now_ns();
}

// --- the crash path, shared by every entry (exception filter, abort, hang sampler, watcher) --------

static void s_spawn_uploader(void);
static void s_exit_process(int code);
static void s_sleep_ms(int ms);
#if defined(_WIN32)
static void WINAPI s_thread_exit_cb(PVOID p);
#endif

// A reporter flag the child must not inherit: the count of arguments that follow it, or -1.
static int s_own_flag_args(const char* arg)
{
	if (s_streq(arg, "--cc-upload") || s_streq(arg, "--cc-test")) return 1;
	if (s_streq(arg, "--cc-watch")) return 5;
	return -1;
}
#if defined(_WIN32)
static void s_region_publish_crash(uint32_t kind, const char* name, const void* record, const void* context, uint64_t pointers);
#endif

// Claims the one report a process gets to write. A second crashing thread parks forever; the same
// thread crashing again inside the handler ends the process with whatever is on disk.
static bool s_crash_claim(void)
{
	uint32_t tid = s_current_tid();
	if (!s_atomic_cas32(&s_cc.sh->crash_state, CC_CRASH_IDLE, CC_CRASH_WRITING)) {
		if (s_cc.handler_tid == tid) return false;
		for (;;) {
#if defined(_WIN32)
			Sleep(INFINITE);
#else
			pause();
#endif
		}
	}
	s_cc.handler_tid = tid;
	s_cc.sh->crash_tid = tid;
	return true;
}

#if defined(_WIN32)

static const char* s_exception_name(DWORD code)
{
	switch (code) {
	case EXCEPTION_ACCESS_VIOLATION: return "EXCEPTION_ACCESS_VIOLATION";
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
	case EXCEPTION_BREAKPOINT: return "EXCEPTION_BREAKPOINT";
	case EXCEPTION_DATATYPE_MISALIGNMENT: return "EXCEPTION_DATATYPE_MISALIGNMENT";
	case EXCEPTION_FLT_DENORMAL_OPERAND: return "EXCEPTION_FLT_DENORMAL_OPERAND";
	case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
	case EXCEPTION_FLT_INEXACT_RESULT: return "EXCEPTION_FLT_INEXACT_RESULT";
	case EXCEPTION_FLT_INVALID_OPERATION: return "EXCEPTION_FLT_INVALID_OPERATION";
	case EXCEPTION_FLT_OVERFLOW: return "EXCEPTION_FLT_OVERFLOW";
	case EXCEPTION_FLT_STACK_CHECK: return "EXCEPTION_FLT_STACK_CHECK";
	case EXCEPTION_FLT_UNDERFLOW: return "EXCEPTION_FLT_UNDERFLOW";
	case EXCEPTION_ILLEGAL_INSTRUCTION: return "EXCEPTION_ILLEGAL_INSTRUCTION";
	case EXCEPTION_IN_PAGE_ERROR: return "EXCEPTION_IN_PAGE_ERROR";
	case EXCEPTION_INT_DIVIDE_BY_ZERO: return "EXCEPTION_INT_DIVIDE_BY_ZERO";
	case EXCEPTION_INT_OVERFLOW: return "EXCEPTION_INT_OVERFLOW";
	case EXCEPTION_INVALID_DISPOSITION: return "EXCEPTION_INVALID_DISPOSITION";
	case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
	case EXCEPTION_PRIV_INSTRUCTION: return "EXCEPTION_PRIV_INSTRUCTION";
	case EXCEPTION_SINGLE_STEP: return "EXCEPTION_SINGLE_STEP";
	case EXCEPTION_STACK_OVERFLOW: return "EXCEPTION_STACK_OVERFLOW";
	case EXCEPTION_INVALID_HANDLE: return "EXCEPTION_INVALID_HANDLE";
	case 0xE06D7363: return "CXX_EXCEPTION";
	case 0xC0000409: return "STATUS_STACK_BUFFER_OVERRUN";
	case 0xC0000374: return "STATUS_HEAP_CORRUPTION";
	default: return "EXCEPTION";
	}
}

// Walks `ctx` by the unwind tables the compiler always emits (x64, arm64) or the frame chain (x86).
// Reads the stack directly, so a torn stack is caught by the __try around the loop.
static int s_walk_context_unguarded(CONTEXT* ctx, uint64_t* pcs, int cap)
{
	int n = 0;
#if defined(_M_X64) || defined(_M_ARM64)
	while (n < cap) {
#if defined(_M_X64)
		DWORD64 pc = ctx->Rip;
#else
		DWORD64 pc = ctx->Pc;
#endif
		if (!pc) break;
		pcs[n++] = pc;
		DWORD64 base = 0;
		PRUNTIME_FUNCTION fn = RtlLookupFunctionEntry(pc, &base, NULL);
		if (!fn) {
#if defined(_M_X64)
			ctx->Rip = *(DWORD64*)(uintptr_t)ctx->Rsp;
			ctx->Rsp += 8;
#else
			if (ctx->Lr == pc) break;
			ctx->Pc = ctx->Lr;
#endif
			continue;
		}
		PVOID handler_data = NULL;
		DWORD64 establisher = 0;
		RtlVirtualUnwind(UNW_FLAG_NHANDLER, base, pc, fn, ctx, &handler_data, &establisher, NULL);
	}
#else
	DWORD ebp = ctx->Ebp, esp = ctx->Esp;
	pcs[n++] = ctx->Eip;
	while (n < cap && ebp >= esp && ebp < esp + (1 << 20)) {
		DWORD ret = *(DWORD*)(uintptr_t)(ebp + 4);
		DWORD next = *(DWORD*)(uintptr_t)ebp;
		if (!ret) break;
		pcs[n++] = ret;
		if (next <= ebp) break;
		ebp = next;
	}
#endif
	return n;
}

static int s_walk_context(CONTEXT* ctx, uint64_t* pcs, int cap)
{
	int n = 0;
#if defined(_MSC_VER) || defined(__clang__)
	__try {
		n = s_walk_context_unguarded(ctx, pcs, cap);
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		if (n == 0) n = 1;
	}
#else
	n = s_walk_context_unguarded(ctx, pcs, cap);
#endif
	return n;
}

static void s_write_dump(EXCEPTION_POINTERS* ep)
{
	if (!s_cc.MiniDumpWriteDump || !s_cc.cfg.minidump) return;
	wchar_t wpath[CC_PATH];
	s_utf8_to_wide(s_cc.dump_path, wpath, CC_PATH);
	HANDLE h = CreateFileW(wpath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) return;
	MINIDUMP_EXCEPTION_INFORMATION mei;
	mei.ThreadId = GetCurrentThreadId();
	mei.ExceptionPointers = ep;
	mei.ClientPointers = FALSE;
	// Stacks, what they point at, and the threads: locals in a debugger at a few MB. Not the data
	// segments: every global of a static build, the symbol slot included, is tens of MB.
	MINIDUMP_TYPE type = (MINIDUMP_TYPE)(MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithThreadInfo | MiniDumpWithHandleData);
	BOOL ok = s_cc.MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), h, type, ep ? &mei : NULL, NULL, NULL);
	LARGE_INTEGER size;
	size.QuadPart = 0;
	if (ok) GetFileSizeEx(h, &size);
	CloseHandle(h);
	if (ok && size.QuadPart > 0) { s_cc.has_dump = true; s_cc.dump_bytes = (uint64_t)size.QuadPart; }
	else DeleteFileW(wpath);
}

// Every Windows crash ends here with a CONTEXT to walk. Never returns.
static void s_crash_windows(cc_fault* f, CONTEXT* ctx, EXCEPTION_POINTERS* ep, DWORD exit_code)
{
	if (!s_crash_claim()) TerminateProcess(GetCurrentProcess(), exit_code);
	f->tid = s_current_tid();
	if (s_cc.cfg.mode == CC_MODE_WATCHER && s_cc.ev_crash) {
		s_region_publish_crash(ep ? 1 : 2, f->name, ep ? (const void*)ep->ExceptionRecord : NULL, ctx, (uint64_t)(uintptr_t)ep);
		if (SetEvent(s_cc.ev_crash)) WaitForSingleObject(s_cc.ev_done, 30000);
		TerminateProcess(GetCurrentProcess(), exit_code);
	}
	if (s_cc.cfg.on_crash) s_cc.cfg.on_crash(s_cc.cfg.udata);
	s_report_paths();
	s_modules_enumerate();
	CONTEXT walk = *ctx;
	s_cc.pc_count = s_walk_context(&walk, s_cc.pcs, CC_MAX_FRAMES);
	s_write_dump(ep);
	s_write_report(f, s_cc.pcs, s_cc.pc_count, NULL, 0);
	s_file_delete(s_cc.marker_path);
	s_atomic_store32(&s_cc.sh->crash_state, CC_CRASH_DONE);
	if (s_cc.cfg.upload_on_crash) s_spawn_uploader();
	TerminateProcess(GetCurrentProcess(), exit_code);
}

// An MSVC C++ throw that nobody caught reaches the filter as 0xE06D7363 with the throw info in the
// record: the catchable type array names the object's type (mangled, which is still legible).
static void s_cxx_exception_type(const EXCEPTION_RECORD* r)
{
#if defined(_M_X64) || defined(_M_ARM64)
	if (r->NumberParameters < 4 || r->ExceptionInformation[0] != 0x19930520) return;
	const unsigned char* base = (const unsigned char*)r->ExceptionInformation[3];
	const uint32_t* throw_info = (const uint32_t*)r->ExceptionInformation[2];
	if (!base || !throw_info) return;
	const uint32_t* catchable = (const uint32_t*)(base + throw_info[3]);
	if (catchable[0] < 1) return;
	const uint32_t* type = (const uint32_t*)(base + catchable[1]);
	const char* name = (const char*)(base + type[1]) + 2 * sizeof(void*);
	cc_set("exception_type", name);
#else
	(void)r;
#endif
}

static LONG WINAPI s_exception_filter(EXCEPTION_POINTERS* ep)
{
	cc_fault f;
	EXCEPTION_RECORD* r = ep->ExceptionRecord;
	memset(&f, 0, sizeof(f));
	f.kind = "crash";
	f.name = s_exception_name(r->ExceptionCode);
	f.code = r->ExceptionCode;
	if (r->ExceptionCode == 0xE06D7363) s_cxx_exception_type(r);
	if ((r->ExceptionCode == EXCEPTION_ACCESS_VIOLATION || r->ExceptionCode == EXCEPTION_IN_PAGE_ERROR) && r->NumberParameters >= 2) {
		f.access = r->ExceptionInformation[0] == 0 ? "read" : r->ExceptionInformation[0] == 1 ? "write" : r->ExceptionInformation[0] == 8 ? "execute" : "unknown";
		f.address = (uint64_t)r->ExceptionInformation[1];
		f.has_address = true;
	} else {
		f.address = (uint64_t)(uintptr_t)r->ExceptionAddress;
		f.has_address = true;
	}
	s_crash_windows(&f, ep->ContextRecord, ep, r->ExceptionCode);
	return EXCEPTION_EXECUTE_HANDLER;
}

// Runs first on every exception and only puts the filter back if another module took the slot.
static LONG CALLBACK s_first_chance(EXCEPTION_POINTERS* ep)
{
	(void)ep;
	LPTOP_LEVEL_EXCEPTION_FILTER prev = SetUnhandledExceptionFilter(s_exception_filter);
	if (prev != s_exception_filter) {
		static volatile uint32_t warned;
		if (s_atomic_cas32(&warned, 0, 1)) cc_breadcrumb("cute_crash: another module replaced the exception filter; put back");
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

// abort(), purecall, invalid parameter, and an uncaught C++ exception arrive without an exception
// record: the stack is captured here.
static CC_NOINLINE void s_crash_named(const char* name)
{
	cc_fault f;
	CONTEXT ctx;
	memset(&f, 0, sizeof(f));
	memset(&ctx, 0, sizeof(ctx));
	f.kind = "crash";
	f.name = name;
	RtlCaptureContext(&ctx);
	s_crash_windows(&f, &ctx, NULL, 3);
}

static void __cdecl s_on_sigabrt(int sig) { (void)sig; s_crash_named("SIGABRT"); }
static void __cdecl s_on_purecall(void) { s_crash_named("purecall"); }
static void __cdecl s_on_invalid_parameter(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t reserved)
{
	(void)expression; (void)function; (void)file; (void)line; (void)reserved;
	s_crash_named("invalid_parameter");
}

static bool s_debugger_present(void) { return IsDebuggerPresent() != 0; }

static void s_install_handlers(void)
{
	s_cc.dbghelp = LoadLibraryW(L"dbghelp.dll");
	if (s_cc.dbghelp) {
		s_cc.MiniDumpWriteDump = (BOOL (WINAPI *)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE, PMINIDUMP_EXCEPTION_INFORMATION, PMINIDUMP_USER_STREAM_INFORMATION, PMINIDUMP_CALLBACK_INFORMATION))(void*)GetProcAddress(s_cc.dbghelp, "MiniDumpWriteDump");
	}
	s_cc.filter = SetUnhandledExceptionFilter(s_exception_filter);
	s_cc.veh = AddVectoredExceptionHandler(1, s_first_chance);
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
	signal(SIGABRT, s_on_sigabrt);
	_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
	_set_purecall_handler(s_on_purecall);
	_set_invalid_parameter_handler(s_on_invalid_parameter);
	DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &s_cc.main_thread, THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, 0);
	s_cc.fls_index = FlsAlloc(s_thread_exit_cb); // Declared above the handlers, defined with the attach below.
}

// Thread exit, through fiber-local storage: the registry slot goes back. The guarantee needs nothing.
static void WINAPI s_thread_exit_cb(PVOID p)
{
	(void)p;
	if (s_cc.sh) s_thread_unregister(s_current_tid());
}

static void s_attach_thread_platform(void)
{
	ULONG size = 64 * 1024;
	SetThreadStackGuarantee(&size);
	if (s_cc.fls_index != FLS_OUT_OF_INDEXES) FlsSetValue(s_cc.fls_index, (PVOID)1);
}

// The report directory, quoted for a command line: without its trailing separator, which a
// closing quote would otherwise swallow (\" is an escaped quote to the CRT's parser).
static void s_cat_quoted_dir(wchar_t* cmd, size_t cap)
{
	wchar_t dir[CC_PATH];
	s_utf8_to_wide(s_cc.report_dir, dir, CC_PATH);
	size_t n = wcslen(dir);
	while (n > 1 && (dir[n - 1] == L'\\' || dir[n - 1] == L'/')) dir[--n] = 0;
	wcscat_s(cmd, cap, L"\"");
	wcscat_s(cmd, cap, dir);
	wcscat_s(cmd, cap, L"\"");
}

// "<exe>" <the game's own arguments> --cc-upload "<dir>", prepared here so the handler formats
// nothing. The game's arguments ride along so its main builds the same config in the child; the
// reporter's own flags from this run are dropped.
static void s_prepare_uploader(void)
{
	const size_t cap = CC_PATH * 4;
	s_cc.child_cmd[0] = 0;
	wcscat_s(s_cc.child_cmd, cap, L"\"");
	wcscat_s(s_cc.child_cmd, cap, s_cc.exe_w);
	wcscat_s(s_cc.child_cmd, cap, L"\"");
	for (int i = 1; i < s_cc.argc; ++i) {
		int skip = s_own_flag_args(s_cc.argv[i]);
		if (skip >= 0) { i += skip; continue; }
		wchar_t w[CC_PATH];
		s_utf8_to_wide(s_cc.argv[i], w, CC_PATH);
		if (wcslen(s_cc.child_cmd) + wcslen(w) + 4 >= cap) break;
		wcscat_s(s_cc.child_cmd, cap, L" \"");
		wcscat_s(s_cc.child_cmd, cap, w);
		wcscat_s(s_cc.child_cmd, cap, L"\"");
	}
	wcscat_s(s_cc.child_cmd, cap, L" --cc-upload ");
	s_cat_quoted_dir(s_cc.child_cmd, cap);
}

static void s_spawn_uploader(void)
{
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	if (CreateProcessW(s_cc.exe_w, s_cc.child_cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
}

// The watchdog's view of the main thread: suspended just long enough to copy its context and walk it.
static int s_sample_main_thread(uint64_t* pcs, int cap)
{
	if (!s_cc.main_thread) return 0;
	if (SuspendThread(s_cc.main_thread) == (DWORD)-1) return 0;
	CONTEXT ctx;
	memset(&ctx, 0, sizeof(ctx));
	ctx.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
	int n = 0;
	if (GetThreadContext(s_cc.main_thread, &ctx)) n = s_walk_context(&ctx, pcs, cap);
	ResumeThread(s_cc.main_thread);
	return n;
}

static void s_exit_process(int code) { TerminateProcess(GetCurrentProcess(), (UINT)code); }

static void s_sleep_ms(int ms) { Sleep((DWORD)ms); }

#endif // _WIN32

// --- the crash sites behind --cc-test -------------------------------------------------------------

CC_NOINLINE void cc_test_null_site(void)
{
	volatile int* p = (volatile int*)(uintptr_t)0x18;
	*p = 1;
}

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable: 4717) // Recursive on all paths: that is the test.
#endif
static CC_NOINLINE int s_test_overflow_site(int depth)
{
	volatile char pad[256];
	pad[depth & 255] = (char)depth;
	return s_test_overflow_site(depth + 1) + pad[(depth + 1) & 255];
}
#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#if defined(__cplusplus)
struct cc_test_thrown { int code; };
static CC_NOINLINE void s_test_throw_site(void) { throw cc_test_thrown{ 42 }; }
#endif

static CC_NOINLINE void s_test_hang_site(void)
{
	for (;;) {
#if defined(_WIN32)
		Sleep(1000);
#else
		sleep(1);
#endif
	}
}

#if defined(_WIN32)
static DWORD WINAPI s_test_thread_main(LPVOID p) { (void)p; cc_attach_thread("cc-test"); cc_test_null_site(); return 0; }
#else
static void* s_test_thread_main(void* p) { (void)p; cc_attach_thread("cc-test"); cc_test_null_site(); return NULL; }
#endif

static void s_run_test(const char* kind)
{
	cc_breadcrumb("cute_crash: --cc-test %s", kind);
	cc_set("test", kind);
	if (s_streq(kind, "null")) cc_test_null_site();
	else if (s_streq(kind, "overflow")) s_test_overflow_site(0);
	else if (s_streq(kind, "abort")) abort();
	else if (s_streq(kind, "throw")) {
#if defined(__cplusplus)
		s_test_throw_site();
#else
		abort();
#endif
	} else if (s_streq(kind, "thread")) {
#if defined(_WIN32)
		HANDLE t = CreateThread(NULL, 0, s_test_thread_main, NULL, 0, NULL);
		if (t) WaitForSingleObject(t, INFINITE);
#else
		pthread_t t;
		if (pthread_create(&t, NULL, s_test_thread_main, NULL) == 0) pthread_join(t, NULL);
#endif
	} else if (s_streq(kind, "spawn")) {
		s_spawn_uploader();
	} else if (s_streq(kind, "hang")) {
		s_cc.exit_after_hang = true;
		s_test_hang_site();
	}
	s_exit_process(2);
}

#if defined(__cplusplus)
// An uncaught C++ exception: its type and message go into state, then abort() takes the usual path.
static void s_on_terminate()
{
	const char* type_name = "unknown";
	const char* what = "";
	try {
		std::exception_ptr ep = std::current_exception();
		if (ep) std::rethrow_exception(ep);
	} catch (const std::exception& e) {
#if defined(__cpp_rtti) || defined(_CPPRTTI) || defined(__GXX_RTTI)
		type_name = typeid(e).name();
#else
		type_name = "std::exception";
#endif
		what = e.what();
	} catch (...) {
	}
	cc_set("exception_type", type_name);
	cc_set("exception_what", what);
	abort();
}

static void s_install_terminate(void) { std::set_terminate(s_on_terminate); }
#endif

#if !defined(_WIN32)

// --- POSIX capture: macOS and Linux. Every function here is unverified on this (Windows) host. ----

#include <sys/ucontext.h>
#if defined(__linux__)
#	include <sys/syscall.h>
#endif
extern char** environ;

// Per attached thread: its stack bounds for the raw copy and its alternate signal stack, which its
// exit gives back. Slots reuse the registry's rule: tid 0 free, CLAIMED while written.
typedef struct cc_thread_stack { volatile uint32_t tid; uintptr_t lo, hi; void* alt; } cc_thread_stack;
static cc_thread_stack s_thread_stacks[CC_MAX_THREADS];
static int64_t s_modules_refreshed_ns;

static const char* s_signal_name(int sig)
{
	switch (sig) {
	case SIGSEGV: return "SIGSEGV";
	case SIGBUS: return "SIGBUS";
	case SIGILL: return "SIGILL";
	case SIGFPE: return "SIGFPE";
	case SIGABRT: return "SIGABRT";
	case SIGTRAP: return "SIGTRAP";
	case SIGSYS: return "SIGSYS";
	default: return "SIGNAL";
	}
}

static const char* s_si_code_name(int sig, int code)
{
	if (sig == SIGSEGV) return code == SEGV_MAPERR ? "SEGV_MAPERR" : code == SEGV_ACCERR ? "SEGV_ACCERR" : "";
	if (sig == SIGBUS) return code == BUS_ADRALN ? "BUS_ADRALN" : code == BUS_ADRERR ? "BUS_ADRERR" : code == BUS_OBJERR ? "BUS_OBJERR" : "";
	if (sig == SIGILL) return code == ILL_ILLOPC ? "ILL_ILLOPC" : code == ILL_ILLOPN ? "ILL_ILLOPN" : code == ILL_PRVOPC ? "ILL_PRVOPC" : "";
	if (sig == SIGFPE) return code == FPE_INTDIV ? "FPE_INTDIV" : code == FPE_INTOVF ? "FPE_INTOVF" : code == FPE_FLTDIV ? "FPE_FLTDIV" : code == FPE_FLTOVF ? "FPE_FLTOVF" : code == FPE_FLTINV ? "FPE_FLTINV" : "";
	return "";
}

// Unverified on this host: the faulting pc and sp out of the signal's ucontext.
static void s_fault_registers(void* uctx, uintptr_t* pc, uintptr_t* sp)
{
	*pc = 0; *sp = 0;
	if (!uctx) return;
	ucontext_t* uc = (ucontext_t*)uctx;
#if defined(__APPLE__)
#	if defined(__arm64__) || defined(__aarch64__)
#		if defined(__arm64e__)
	*pc = (uintptr_t)__darwin_arm_thread_state64_get_pc(uc->uc_mcontext->__ss);
	*sp = (uintptr_t)__darwin_arm_thread_state64_get_sp(uc->uc_mcontext->__ss);
#		else
	*pc = (uintptr_t)uc->uc_mcontext->__ss.__pc;
	*sp = (uintptr_t)uc->uc_mcontext->__ss.__sp;
#		endif
#	else
	*pc = (uintptr_t)uc->uc_mcontext->__ss.__rip;
	*sp = (uintptr_t)uc->uc_mcontext->__ss.__rsp;
#	endif
#elif defined(__aarch64__)
	*pc = (uintptr_t)uc->uc_mcontext.pc;
	*sp = (uintptr_t)uc->uc_mcontext.sp;
#elif defined(__x86_64__)
	*pc = (uintptr_t)uc->uc_mcontext.gregs[16]; // REG_RIP
	*sp = (uintptr_t)uc->uc_mcontext.gregs[15]; // REG_RSP
#else
	*pc = (uintptr_t)uc->uc_mcontext.gregs[14]; // REG_EIP
	*sp = (uintptr_t)uc->uc_mcontext.gregs[7];  // REG_ESP
#endif
}

// Unverified on this host: backtrace() from inside the handler lists the handler and the kernel's
// trampoline first; the frames from the faulting pc onward are the ones kept. When the pc is not
// in the list (a jump into nowhere), it is put at the top and the whole list follows.
static int s_posix_frames(uintptr_t fault_pc, uint64_t* pcs, int cap)
{
	void* raw[CC_MAX_FRAMES];
	int n = backtrace(raw, CC_MAX_FRAMES);
	int start = -1;
	for (int i = 0; i < n; ++i) {
		uintptr_t a = (uintptr_t)raw[i];
		if (fault_pc && a >= fault_pc && a <= fault_pc + 16) { start = i; break; }
	}
	int out = 0;
	if (start < 0) {
		if (fault_pc) pcs[out++] = fault_pc;
		start = 0;
	}
	for (int i = start; i < n && out < cap; ++i) pcs[out++] = (uint64_t)(uintptr_t)raw[i];
	if (out > 0 && fault_pc) pcs[0] = fault_pc;
	return out;
}

static size_t s_copy_stack(uintptr_t sp)
{
	if (!s_cc.stack_copy || !sp) return 0;
	uint32_t tid = s_current_tid();
	for (uint32_t i = 0; i < CC_MAX_THREADS; ++i) {
		const cc_thread_stack* t = &s_thread_stacks[i];
		if (s_atomic_load32(&t->tid) != tid || sp < t->lo || sp >= t->hi) continue;
		size_t len = (size_t)(t->hi - sp);
		if (len > CC_STACK_BYTES) len = CC_STACK_BYTES;
		memcpy(s_cc.stack_copy, (const void*)sp, len);
		return len;
	}
	return 0;
}

static void s_spawn_uploader(void)
{
	pid_t pid = fork();
	if (pid == 0) {
		execve(s_cc.child_argv[0], s_cc.child_argv, environ);
		_exit(127);
	}
}

// Unverified on this host.
static void s_signal_handler(int sig, siginfo_t* si, void* uctx)
{
	cc_fault f;
	uintptr_t pc = 0, sp = 0;
	memset(&f, 0, sizeof(f));
	f.kind = "crash";
	f.name = s_signal_name(sig);
	f.code_name = si ? s_si_code_name(sig, si->si_code) : "";
	if (si && (sig == SIGSEGV || sig == SIGBUS || sig == SIGILL || sig == SIGFPE)) { f.address = (uint64_t)(uintptr_t)si->si_addr; f.has_address = true; }
	f.tid = s_current_tid();
	if (s_crash_claim()) {
		s_fault_registers(uctx, &pc, &sp);
		if (s_cc.cfg.on_crash) s_cc.cfg.on_crash(s_cc.cfg.udata);
		s_report_paths();
		s_cc.pc_count = s_posix_frames(pc, s_cc.pcs, CC_MAX_FRAMES);
		s_cc.stack_copy_len = s_copy_stack(sp);
		s_write_report(&f, s_cc.pcs, s_cc.pc_count, s_cc.stack_copy, s_cc.stack_copy_len);
		s_file_delete(s_cc.marker_path);
		s_atomic_store32(&s_cc.sh->crash_state, CC_CRASH_DONE);
		if (s_cc.cfg.upload_on_crash) s_spawn_uploader();
	}
	// The default action now: the OS exit status, core, and Apple's own report all happen.
	signal(sig, SIG_DFL);
	if (sig == SIGABRT || sig == SIGTRAP || sig == SIGSYS) raise(sig);
}

// Unverified on this host: the watchdog's sampler, run on the main thread by a signal.
static void s_sample_handler(int sig, siginfo_t* si, void* uctx)
{
	(void)sig; (void)si;
	uintptr_t pc = 0, sp = 0;
	s_fault_registers(uctx, &pc, &sp);
	int slot = (int)(s_atomic_load32(&s_cc.sample_done) & 1);
	s_cc.sample_count[slot] = s_posix_frames(pc, s_cc.sample_pcs[slot], CC_MAX_FRAMES);
	s_atomic_store32(&s_cc.sample_done, 2 | (uint32_t)slot);
}

static int s_sample_main_thread(uint64_t* pcs, int cap)
{
	static uint32_t slot;
	slot ^= 1;
	s_atomic_store32(&s_cc.sample_done, slot);
	if (pthread_kill(s_cc.main_pthread, SIGURG) != 0) return 0;
	for (int i = 0; i < 1000; ++i) {
		if (s_atomic_load32(&s_cc.sample_done) & 2) {
			int n = s_cc.sample_count[slot] < cap ? s_cc.sample_count[slot] : cap;
			memcpy(pcs, s_cc.sample_pcs[slot], (size_t)n * sizeof(uint64_t));
			return n;
		}
		usleep(1000);
	}
	return 0;
}

static bool s_debugger_present(void)
{
#if defined(__APPLE__)
	struct kinfo_proc info;
	size_t size = sizeof(info);
	int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, (int)getpid() };
	memset(&info, 0, sizeof(info));
	if (sysctl(mib, 4, &info, &size, NULL, 0) != 0) return false;
	return (info.kp_proc.p_flag & P_TRACED) != 0;
#else
	FILE* f = fopen("/proc/self/status", "r");
	if (!f) return false;
	char line[128];
	bool traced = false;
	while (fgets(line, sizeof(line), f)) {
		if (strncmp(line, "TracerPid:", 10) == 0) { traced = atoi(line + 10) != 0; break; }
	}
	fclose(f);
	return traced;
#endif
}

static void s_install_handlers(void)
{
	static const int sigs[] = { SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT, SIGTRAP, SIGSYS };
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = s_signal_handler;
	sa.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_NODEFER;
	sigemptyset(&sa.sa_mask);
	for (size_t i = 0; i < sizeof(sigs) / sizeof(sigs[0]); ++i) sigaction(sigs[i], &sa, NULL);
	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = s_sample_handler;
	sa.sa_flags = SA_SIGINFO | SA_RESTART;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGURG, &sa, NULL);
	{
		void* warm[8];
		backtrace(warm, 8);
	}
	s_cc.main_pthread = pthread_self();
	s_cc.thread_key_ok = pthread_key_create(&s_cc.thread_key, s_thread_exit_cb) == 0;
	s_cc.stack_copy = (unsigned char*)malloc(CC_STACK_BYTES);
}

// Unverified on this host: thread exit, through the key's destructor. The alternate stack is
// disabled and freed, the slots go back. The main thread keeps its own for the process.
static void s_thread_exit_cb(void* p)
{
	(void)p;
	uint32_t tid = s_current_tid();
	if (!s_cc.sh || tid == s_cc.sh->main_tid) return;
	for (uint32_t i = 0; i < CC_MAX_THREADS; ++i) {
		cc_thread_stack* t = &s_thread_stacks[i];
		if (s_atomic_load32(&t->tid) != tid) continue;
		stack_t off;
		memset(&off, 0, sizeof(off));
		off.ss_flags = SS_DISABLE;
		sigaltstack(&off, NULL);
		void* alt = t->alt;
		t->alt = NULL;
		t->lo = t->hi = 0;
		s_atomic_store32(&t->tid, 0);
		free(alt);
		break;
	}
	s_thread_unregister(tid);
}

// Unverified on this host: an alternate stack, so a stack overflow still reports, and the thread's
// stack bounds for the raw copy.
static void s_attach_thread_platform(void)
{
	stack_t ss;
	ss.ss_sp = malloc(64 * 1024);
	ss.ss_size = 64 * 1024;
	ss.ss_flags = 0;
	if (ss.ss_sp) sigaltstack(&ss, NULL);
	if (s_cc.thread_key_ok) pthread_setspecific(s_cc.thread_key, (void*)1);
	uintptr_t lo = 0, hi = 0;
#if defined(__APPLE__)
	pthread_t self = pthread_self();
	hi = (uintptr_t)pthread_get_stackaddr_np(self);
	lo = hi - (uintptr_t)pthread_get_stacksize_np(self);
#else
	pthread_attr_t attr;
	if (pthread_getattr_np(pthread_self(), &attr) == 0) {
		void* addr = NULL;
		size_t size = 0;
		if (pthread_attr_getstack(&attr, &addr, &size) == 0) { lo = (uintptr_t)addr; hi = lo + size; }
		pthread_attr_destroy(&attr);
	}
#endif
	for (uint32_t i = 0; i < CC_MAX_THREADS; ++i) {
		if (!s_atomic_cas32(&s_thread_stacks[i].tid, 0, CC_TID_CLAIMED)) continue;
		s_thread_stacks[i].lo = lo;
		s_thread_stacks[i].hi = hi;
		s_thread_stacks[i].alt = ss.ss_sp;
		s_atomic_store32(&s_thread_stacks[i].tid, s_current_tid());
		return;
	}
	free(ss.ss_sp); // No slot: the stack stays disabled for this thread, nothing leaks.
	{ stack_t off; memset(&off, 0, sizeof(off)); off.ss_flags = SS_DISABLE; sigaltstack(&off, NULL); }
}

// <exe> <the game's own arguments> --cc-upload <dir>: the game's main builds the same config in
// the child; the reporter's own flags from this run are dropped.
static void s_prepare_uploader(void)
{
	int n = 0;
	s_strcpy(s_cc.child_flag, sizeof(s_cc.child_flag), "--cc-upload");
	s_cc.child_argv[n++] = s_cc.exe_path;
	for (int i = 1; i < s_cc.argc && n < 60; ++i) {
		int skip = s_own_flag_args(s_cc.argv[i]);
		if (skip >= 0) { i += skip; continue; }
		s_cc.child_argv[n++] = s_cc.argv[i];
	}
	s_cc.child_argv[n++] = s_cc.child_flag;
	s_cc.child_argv[n++] = s_cc.report_dir;
	s_cc.child_argv[n] = NULL;
}

static void s_exit_process(int code) { _exit(code); }
static void s_sleep_ms(int ms) { usleep((useconds_t)ms * 1000); }

// The module list is read outside the handler: at init and, at most once a second, on the heartbeat.
static void s_modules_refresh(void)
{
	int64_t now = s_now_ns();
	if (now - s_modules_refreshed_ns < 1000000000LL) return;
	s_modules_refreshed_ns = now;
	s_modules_enumerate();
}

#endif // !_WIN32

static cc_consent s_pending_consent;

// --- the symbol table reader (CUTESYM v1): the same bytes cute_sym.h writes ---------------------
// Records are little-endian with a trailing pad to 8: funcs 16 bytes { u64 start; u32 size;
// u32 name; }, lines 24 { u64 start; u32 len; u32 file; u32 line; u32 pad; }, inlines 32 { u64 start;
// u32 len; u32 callee; u32 call_file; u32 call_line; u32 parent; u32 pad; }. Read by byte, never
// overlaid with a struct: an embedded slot is not necessarily aligned.

#define CC_SYM_HEADER 96
#define CC_SYM_FUNC 16
#define CC_SYM_LINE 24
#define CC_SYM_INLINE 32
#define CC_SYM_NO_PARENT 0xffffffffu
#define CC_SYM_SLOT_MAGIC "CUTESYMSLOT"
#define CC_MAX_INLINES 16

typedef struct cc_sym_table
{
	const unsigned char* bytes;
	size_t len;
	uint32_t arch;
	unsigned char build_id[20];
	int build_id_len;
	uint32_t func_count, line_count, inline_count, file_count, strings_len;
	uint32_t off_funcs, off_lines, off_inlines, off_files, off_strings;
} cc_sym_table;

typedef struct cc_sym_frame { const char* function; const char* file; uint32_t line; } cc_sym_frame;

static uint32_t s_le32(const unsigned char* p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }
static uint64_t s_le64(const unsigned char* p) { return (uint64_t)s_le32(p) | ((uint64_t)s_le32(p + 4) << 32); }

static bool s_sym_read(const unsigned char* bytes, size_t len, cc_sym_table* t)
{
	memset(t, 0, sizeof(*t));
	if (!bytes || len < CC_SYM_HEADER || memcmp(bytes, "CUTESYM\0", 8) != 0 || s_le32(bytes + 8) != 1) return false;
	t->bytes = bytes;
	t->arch = s_le32(bytes + 12);
	memcpy(t->build_id, bytes + 16, 20);
	t->build_id_len = bytes[36];
	if (t->build_id_len > 20) return false;
	t->func_count = s_le32(bytes + 40);
	t->line_count = s_le32(bytes + 44);
	t->inline_count = s_le32(bytes + 48);
	t->file_count = s_le32(bytes + 52);
	t->strings_len = s_le32(bytes + 56);
	t->off_funcs = s_le32(bytes + 60);
	t->off_lines = s_le32(bytes + 64);
	t->off_inlines = s_le32(bytes + 68);
	t->off_files = s_le32(bytes + 72);
	t->off_strings = s_le32(bytes + 76);
	uint32_t total = s_le32(bytes + 80);
	if (total > len) return false;
	t->len = total;
	if ((uint64_t)t->off_funcs + (uint64_t)t->func_count * CC_SYM_FUNC > total) return false;
	if ((uint64_t)t->off_lines + (uint64_t)t->line_count * CC_SYM_LINE > total) return false;
	if ((uint64_t)t->off_inlines + (uint64_t)t->inline_count * CC_SYM_INLINE > total) return false;
	if ((uint64_t)t->off_files + (uint64_t)t->file_count * 4 > total) return false;
	if ((uint64_t)t->off_strings + t->strings_len > total || t->strings_len == 0) return false;
	if (bytes[t->off_strings + t->strings_len - 1] != 0) return false;
	return true;
}

static const char* s_sym_str(const cc_sym_table* t, uint32_t off)
{
	if (off >= t->strings_len) return "";
	return (const char*)t->bytes + t->off_strings + off;
}

// `file` in a line or inline record is a STRING offset (the files section is a list of the distinct
// file strings, for tools; lookups never go through it).
static const char* s_sym_file(const cc_sym_table* t, uint32_t file)
{
	return s_sym_str(t, file);
}

// Index of the last record whose start <= addr, or -1. Records are `stride` bytes with a u64 start first.
static int64_t s_sym_floor(const unsigned char* recs, uint32_t count, uint32_t stride, uint64_t addr)
{
	int64_t lo = 0, hi = (int64_t)count - 1, best = -1;
	while (lo <= hi) {
		int64_t mid = lo + (hi - lo) / 2;
		if (s_le64(recs + mid * stride) <= addr) { best = mid; lo = mid + 1; } else hi = mid - 1;
	}
	return best;
}

static bool s_sym_lookup(const cc_sym_table* t, uint64_t addr, cc_sym_frame* out, cc_sym_frame* inlined, int inlined_cap, int* inlined_count)
{
	memset(out, 0, sizeof(*out));
	*inlined_count = 0;
	const unsigned char* funcs = t->bytes + t->off_funcs;
	const unsigned char* lines = t->bytes + t->off_lines;
	const unsigned char* inls = t->bytes + t->off_inlines;
	bool found = false;
	uint64_t func_start = 0;
	int64_t fi = s_sym_floor(funcs, t->func_count, CC_SYM_FUNC, addr);
	if (fi >= 0) {
		const unsigned char* r = funcs + fi * CC_SYM_FUNC;
		func_start = s_le64(r);
		if (addr < func_start + s_le32(r + 8)) { out->function = s_sym_str(t, s_le32(r + 12)); found = true; }
	}
	int64_t li = s_sym_floor(lines, t->line_count, CC_SYM_LINE, addr);
	if (li >= 0) {
		const unsigned char* r = lines + li * CC_SYM_LINE;
		if (addr < s_le64(r) + s_le32(r + 8)) { out->file = s_sym_file(t, s_le32(r + 12)); out->line = s_le32(r + 16); found = true; }
	}
	// The innermost inline containing addr: scanning back from the last start <= addr, the first
	// container met is the deepest, since containers nest and a deeper one starts no earlier.
	int64_t ii = s_sym_floor(inls, t->inline_count, CC_SYM_INLINE, addr);
	int64_t inner = -1;
	for (int64_t i = ii; i >= 0; --i) {
		const unsigned char* r = inls + i * CC_SYM_INLINE;
		uint64_t start = s_le64(r);
		if (found && start < func_start) break;
		if (addr < start + s_le32(r + 8)) { inner = i; break; }
	}
	if (inner >= 0 && found) {
		// Innermost first: the callee that ran, at the line table's row; then each caller inline at the
		// call site of the one inside it; the function itself ends up at the outermost call site.
		int n = 0;
		int64_t i = inner;
		const char* file = out->file;
		uint32_t line = out->line;
		int guard = 0;
		while (i >= 0 && n < inlined_cap && guard++ < 64) {
			const unsigned char* r = inls + i * CC_SYM_INLINE;
			inlined[n].function = s_sym_str(t, s_le32(r + 12));
			inlined[n].file = file;
			inlined[n].line = line;
			file = s_sym_file(t, s_le32(r + 16));
			line = s_le32(r + 20);
			uint32_t parent = s_le32(r + 24);
			i = parent == CC_SYM_NO_PARENT || parent >= t->inline_count ? -1 : (int64_t)parent;
			++n;
		}
		out->file = file;
		out->line = line;
		*inlined_count = n;
	}
	return found;
}

// --- a small JSON tree: for the report this header wrote and the marker it wrote --------------------
// Not handler-safe: heap. Values live in the tree's own strings.

typedef struct cc_jnode
{
	int type;         // 0 null, 1 bool, 2 number, 3 string, 4 array, 5 object
	char* key;        // Owned, when a child of an object.
	char* str;        // Owned, type 3. Numbers keep their text here too.
	bool b;
	int first;        // Child index chain, -1 ends.
	int last;
	int next;
} cc_jnode;

typedef struct cc_jtree { cc_jnode* nodes; int count; int cap; } cc_jtree;

static int s_jt_new(cc_jtree* t, int type)
{
	if (t->count == t->cap) {
		int cap = t->cap ? t->cap * 2 : 256;
		cc_jnode* n = (cc_jnode*)realloc(t->nodes, (size_t)cap * sizeof(cc_jnode));
		if (!n) return -1;
		t->nodes = n;
		t->cap = cap;
	}
	cc_jnode* n = &t->nodes[t->count];
	memset(n, 0, sizeof(*n));
	n->type = type;
	n->first = n->last = n->next = -1;
	return t->count++;
}

static char* s_jt_strdup(const char* s, size_t n)
{
	char* d = (char*)malloc(n + 1);
	if (!d) return NULL;
	memcpy(d, s, n);
	d[n] = 0;
	return d;
}

static void s_jt_free(cc_jtree* t)
{
	for (int i = 0; i < t->count; ++i) { free(t->nodes[i].key); free(t->nodes[i].str); }
	free(t->nodes);
	memset(t, 0, sizeof(*t));
}

static void s_jt_append(cc_jtree* t, int parent, int child)
{
	cc_jnode* p = &t->nodes[parent];
	if (p->first < 0) p->first = child; else t->nodes[p->last].next = child;
	p->last = child;
}

static const char* s_jp_skip(const char* p) { while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') ++p; return p; }

static const char* s_jp_string(const char* p, char** out)
{
	if (*p != '"') return NULL;
	++p;
	size_t cap = 64, n = 0;
	char* s = (char*)malloc(cap);
	if (!s) return NULL;
	while (*p && *p != '"') {
		char c = *p++;
		if (c == '\\') {
			char e = *p++;
			if (e == 'n') c = '\n'; else if (e == 't') c = '\t'; else if (e == 'r') c = '\r';
			else if (e == 'u') { unsigned v = 0; for (int i = 0; i < 4 && *p; ++i, ++p) { char h = *p; v = v * 16 + (unsigned)(h >= '0' && h <= '9' ? h - '0' : h >= 'a' && h <= 'f' ? h - 'a' + 10 : h >= 'A' && h <= 'F' ? h - 'A' + 10 : 0); } c = (char)(v < 128 ? v : '?'); }
			else c = e;
		}
		if (n + 2 > cap) { cap *= 2; char* g = (char*)realloc(s, cap); if (!g) { free(s); return NULL; } s = g; }
		s[n++] = c;
	}
	if (*p != '"') { free(s); return NULL; }
	s[n] = 0;
	*out = s;
	return p + 1;
}

static const char* s_jp_value(cc_jtree* t, const char* p, int* out);

static const char* s_jp_container(cc_jtree* t, const char* p, int node, char close)
{
	++p;
	for (;;) {
		p = s_jp_skip(p);
		if (*p == close) return p + 1;
		if (*p == ',') { ++p; continue; }
		char* key = NULL;
		if (close == '}') {
			p = s_jp_string(p, &key);
			if (!p) return NULL;
			p = s_jp_skip(p);
			if (*p != ':') { free(key); return NULL; }
			++p;
		}
		int child = -1;
		p = s_jp_value(t, s_jp_skip(p), &child);
		if (!p) { free(key); return NULL; }
		t->nodes[child].key = key;
		s_jt_append(t, node, child);
	}
}

static const char* s_jp_value(cc_jtree* t, const char* p, int* out)
{
	int n;
	if (*p == '{' || *p == '[') {
		n = s_jt_new(t, *p == '{' ? 5 : 4);
		if (n < 0) return NULL;
		*out = n;
		return s_jp_container(t, p, n, *p == '{' ? '}' : ']');
	}
	if (*p == '"') {
		char* s = NULL;
		p = s_jp_string(p, &s);
		if (!p) return NULL;
		n = s_jt_new(t, 3);
		if (n < 0) { free(s); return NULL; }
		t->nodes[n].str = s;
		*out = n;
		return p;
	}
	if (strncmp(p, "true", 4) == 0) { n = s_jt_new(t, 1); if (n < 0) return NULL; t->nodes[n].b = true; *out = n; return p + 4; }
	if (strncmp(p, "false", 5) == 0) { n = s_jt_new(t, 1); if (n < 0) return NULL; *out = n; return p + 5; }
	if (strncmp(p, "null", 4) == 0) { n = s_jt_new(t, 0); if (n < 0) return NULL; *out = n; return p + 4; }
	const char* e = p;
	while ((*e >= '0' && *e <= '9') || *e == '-' || *e == '+' || *e == '.' || *e == 'e' || *e == 'E') ++e;
	if (e == p) return NULL;
	n = s_jt_new(t, 2);
	if (n < 0) return NULL;
	t->nodes[n].str = s_jt_strdup(p, (size_t)(e - p));
	*out = n;
	return e;
}

static int s_jt_parse(cc_jtree* t, const char* text)
{
	int root = -1;
	memset(t, 0, sizeof(*t));
	if (!s_jp_value(t, s_jp_skip(text), &root)) { s_jt_free(t); return -1; }
	return root;
}

static int s_jt_get(const cc_jtree* t, int obj, const char* key)
{
	if (obj < 0 || t->nodes[obj].type != 5) return -1;
	for (int c = t->nodes[obj].first; c >= 0; c = t->nodes[c].next) if (s_streq(t->nodes[c].key, key)) return c;
	return -1;
}

static const char* s_jt_string(const cc_jtree* t, int obj, const char* key)
{
	int c = s_jt_get(t, obj, key);
	return c >= 0 && t->nodes[c].type == 3 ? t->nodes[c].str : NULL;
}

static int64_t s_jt_int(const cc_jtree* t, int obj, const char* key, int64_t fallback)
{
	int c = s_jt_get(t, obj, key);
	if (c < 0 || t->nodes[c].type != 2 || !t->nodes[c].str) return fallback;
	return (int64_t)strtoll(t->nodes[c].str, NULL, 10);
}

static int s_jt_add(cc_jtree* t, int obj, const char* key, int type)
{
	int n = s_jt_new(t, type);
	if (n < 0) return -1;
	t->nodes[n].key = key ? s_jt_strdup(key, s_strlen(key)) : NULL;
	s_jt_append(t, obj, n);
	return n;
}

// Sets key to a string or number (text), replacing an existing child.
static void s_jt_set_text(cc_jtree* t, int obj, const char* key, const char* text, int type)
{
	int c = s_jt_get(t, obj, key);
	if (c < 0) c = s_jt_add(t, obj, key, type);
	if (c < 0) return;
	free(t->nodes[c].str);
	t->nodes[c].str = s_jt_strdup(text, s_strlen(text));
	t->nodes[c].type = type;
	t->nodes[c].first = t->nodes[c].last = -1;
}

static void s_jt_set_str(cc_jtree* t, int obj, const char* key, const char* s) { s_jt_set_text(t, obj, key, s, 3); }
static void s_jt_set_int(cc_jtree* t, int obj, const char* key, int64_t v) { char b[24]; s_itoa(v, b); s_jt_set_text(t, obj, key, b, 2); }

static void s_jt_write(const cc_jtree* t, int node, cc_jw* w)
{
	const cc_jnode* n = &t->nodes[node];
	switch (n->type) {
	case 0: s_jw_raw(w, "null"); break;
	case 1: s_jw_bool(w, n->b); break;
	case 2: s_jw_raw(w, n->str ? n->str : "0"); break;
	case 3: s_jw_str(w, n->str ? n->str : ""); break;
	case 4:
		s_jw_begin_arr(w);
		for (int c = n->first; c >= 0; c = t->nodes[c].next) s_jt_write(t, c, w);
		s_jw_end_arr(w);
		break;
	default:
		s_jw_begin_obj(w);
		for (int c = n->first; c >= 0; c = t->nodes[c].next) { s_jw_key(w, t->nodes[c].key ? t->nodes[c].key : ""); s_jt_write(t, c, w); }
		s_jw_end_obj(w);
		break;
	}
}

static bool s_jt_save(const cc_jtree* t, int root, const char* path)
{
	size_t cap = 1 << 20;
	char* buf = (char*)malloc(cap);
	if (!buf) return false;
	cc_jw w;
	s_jw_init(&w, buf, cap);
	s_jt_write(t, root, &w);
	s_jw_putc(&w, '\n');
	bool ok = !w.overflow && s_file_write(path, buf, s_jw_len(&w, buf));
	free(buf);
	return ok;
}

// --- finding a module's table: the slot inside it, then <file>.sym beside it, then sym_dir --------

typedef struct cc_found_table
{
	unsigned char* owned;   // malloc'd file bytes, or NULL when the table is the mapped slot.
	cc_sym_table table;
	const char* where;      // "embedded", "file", "none"
} cc_found_table;

static const unsigned char* s_find_slot(const unsigned char* p, size_t len)
{
	const unsigned char* end = p + len;
	while (p + 16 + CC_SYM_HEADER <= end) {
		const unsigned char* m = (const unsigned char*)memchr(p, 'C', (size_t)(end - p));
		if (!m || m + 16 + CC_SYM_HEADER > end) return NULL;
		if (memcmp(m, CC_SYM_SLOT_MAGIC, 12) == 0) return m;
		p = m + 1;
	}
	return NULL;
}

static bool s_table_matches(const cc_sym_table* t, const unsigned char* build_id, int build_id_len)
{
	return build_id_len > 0 && t->build_id_len == build_id_len && memcmp(t->build_id, build_id, (size_t)build_id_len) == 0;
}

static bool s_slot_table(const unsigned char* slot, size_t avail, const unsigned char* build_id, int build_id_len, cc_sym_table* out)
{
	uint32_t cap = s_le32(slot + 12);
	if ((uint64_t)cap + 16 > avail) cap = (uint32_t)(avail - 16);
	return s_sym_read(slot + 16, cap, out) && s_table_matches(out, build_id, build_id_len);
}

#if defined(_WIN32)
// The readable, committed pages of a loaded module, scanned for the slot header.
static bool s_mapped_slot(uint64_t base, uint64_t size, const unsigned char* build_id, int build_id_len, cc_sym_table* out)
{
	const unsigned char* p = (const unsigned char*)(uintptr_t)base;
	const unsigned char* end = p + size;
	while (p < end) {
		MEMORY_BASIC_INFORMATION mbi;
		if (!VirtualQuery(p, &mbi, sizeof(mbi))) break;
		const unsigned char* rend = (const unsigned char*)mbi.BaseAddress + mbi.RegionSize;
		if (rend > end) rend = end;
		bool readable = mbi.State == MEM_COMMIT && !(mbi.Protect & PAGE_GUARD) && (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_WRITECOPY));
		if (readable) {
			const unsigned char* q = p;
			while (q < rend) {
				const unsigned char* slot = s_find_slot(q, (size_t)(rend - q));
				if (!slot) break;
				if (s_slot_table(slot, (size_t)(rend - slot), build_id, build_id_len, out)) return true;
				q = slot + 1;
			}
		}
		p = rend;
	}
	return false;
}
#endif

static bool s_file_table(const char* path, const unsigned char* build_id, int build_id_len, bool scan_for_slot, cc_found_table* f)
{
	size_t len = 0;
	unsigned char* bytes = (unsigned char*)s_file_read(path, &len);
	if (!bytes) return false;
	if (scan_for_slot) {
		const unsigned char* q = bytes;
		for (;;) {
			const unsigned char* slot = s_find_slot(q, len - (size_t)(q - bytes));
			if (!slot) break;
			if (s_slot_table(slot, len - (size_t)(slot - bytes), build_id, build_id_len, &f->table)) { f->owned = bytes; f->where = "embedded"; return true; }
			q = slot + 1;
		}
	} else if (s_sym_read(bytes, len, &f->table) && s_table_matches(&f->table, build_id, build_id_len)) {
		f->owned = bytes;
		f->where = "file";
		return true;
	}
	free(bytes);
	return false;
}

static bool s_find_table(const char* module_path, const unsigned char* build_id, int build_id_len, cc_found_table* f)
{
	char path[CC_PATH];
	memset(f, 0, sizeof(*f));
	f->where = "none";
	if (build_id_len <= 0) return false;
#if defined(_WIN32)
	for (int i = 0; i < s_cc.module_count; ++i) {
		const cc_module* m = &s_cc.modules[i];
		if (m->build_id_len == build_id_len && memcmp(m->build_id, build_id, (size_t)build_id_len) == 0) {
			if (s_mapped_slot(m->base, m->size, build_id, build_id_len, &f->table)) { f->where = "embedded"; return true; }
			break;
		}
	}
#endif
	if (module_path && module_path[0]) {
		if (s_file_table(module_path, build_id, build_id_len, true, f)) return true;
		s_strcpy(path, CC_PATH, module_path);
		s_strcat(path, CC_PATH, ".sym");
		if (s_file_table(path, build_id, build_id_len, false, f)) return true;
	}
	if (s_cc.sym_dir[0]) {
		char hex[48];
		s_strcpy(path, CC_PATH, s_cc.sym_dir);
		s_strcat(path, CC_PATH, s_basename(module_path ? module_path : ""));
		s_strcat(path, CC_PATH, ".sym");
		if (s_file_table(path, build_id, build_id_len, false, f)) return true;
		s_hex_bytes(build_id, build_id_len, hex);
		s_strcpy(path, CC_PATH, s_cc.sym_dir);
		s_strcat(path, CC_PATH, hex);
		s_strcat(path, CC_PATH, ".sym");
		if (s_file_table(path, build_id, build_id_len, false, f)) return true;
	}
	return false;
}

static void s_free_table(cc_found_table* f) { free(f->owned); f->owned = NULL; }

// --- resolving one report in place ------------------------------------------------------------------

typedef struct cc_resolve_module
{
	bool tried;
	bool found;
	bool system;
	cc_found_table table;
} cc_resolve_module;

static void s_resolve_frames(cc_jtree* t, int stack, cc_resolve_module* mods, int mod_count, cc_sha1* sig, int* sig_used, const char* fault_name)
{
	int index = 0;
	if (sig && *sig_used == 0 && fault_name) s_sha1_str(sig, fault_name);
	for (int fr = t->nodes[stack].first; fr >= 0; fr = t->nodes[fr].next, ++index) {
		int mi = (int)s_jt_int(t, fr, "module", -1);
		if (mi < 0 || mi >= mod_count || !mods[mi].found) continue;
		uint64_t offset = (uint64_t)s_jt_int(t, fr, "offset", 0);
		uint64_t addr = index > 0 && offset > 0 ? offset - 1 : offset;
		cc_sym_frame frame, inl[CC_MAX_INLINES];
		int inl_count = 0;
		if (!s_sym_lookup(&mods[mi].table.table, addr, &frame, inl, CC_MAX_INLINES, &inl_count)) continue;
		if (frame.function && frame.function[0]) s_jt_set_str(t, fr, "function", frame.function);
		if (frame.file && frame.file[0]) { s_jt_set_str(t, fr, "file", frame.file); s_jt_set_int(t, fr, "line", frame.line); }
		if (inl_count > 0) {
			int existing = s_jt_get(t, fr, "inlined");
			if (existing >= 0) { t->nodes[existing].first = t->nodes[existing].last = -1; t->nodes[existing].type = 4; }
			int arr = existing >= 0 ? existing : s_jt_add(t, fr, "inlined", 4);
			for (int i = 0; i < inl_count && arr >= 0; ++i) {
				int o = s_jt_add(t, arr, NULL, 5);
				if (o < 0) break;
				s_jt_set_str(t, o, "function", inl[i].function ? inl[i].function : "");
				if (inl[i].file && inl[i].file[0]) { s_jt_set_str(t, o, "file", inl[i].file); s_jt_set_int(t, o, "line", inl[i].line); }
			}
		}
		if (sig && *sig_used < CC_SIGNATURE_FRAMES && !mods[mi].system && frame.function && frame.function[0]) {
			s_sha1_str(sig, inl_count > 0 && inl[0].function ? inl[0].function : frame.function);
			s_sha1_str(sig, ";");
			(*sig_used)++;
		}
	}
}

static bool s_resolve_report(const char* path)
{
	size_t len = 0;
	char* text = s_file_read(path, &len);
	if (!text) return false;
	cc_jtree t;
	int root = s_jt_parse(&t, text);
	free(text);
	if (root < 0) return false;
	int modules = s_jt_get(&t, root, "modules");
	int mod_count = 0;
	for (int m = modules >= 0 ? t.nodes[modules].first : -1; m >= 0; m = t.nodes[m].next) ++mod_count;
	cc_resolve_module* mods = (cc_resolve_module*)calloc((size_t)(mod_count > 0 ? mod_count : 1), sizeof(cc_resolve_module));
	if (!mods) { s_jt_free(&t); return false; }
#if defined(_WIN32)
	s_modules_enumerate();
#endif
	int i = 0;
	for (int m = modules >= 0 ? t.nodes[modules].first : -1; m >= 0; m = t.nodes[m].next, ++i) {
		const char* hex = s_jt_string(&t, m, "build_id");
		const char* mpath = s_jt_string(&t, m, "path");
		int sys = s_jt_get(&t, m, "system");
		unsigned char id[20];
		int id_len = hex ? s_unhex_bytes(hex, id, 20) : 0;
		mods[i].system = sys >= 0 && t.nodes[sys].b;
		mods[i].tried = true;
		mods[i].found = !mods[i].system && s_find_table(mpath, id, id_len, &mods[i].table);
		s_jt_set_str(&t, m, "symbols", mods[i].found ? mods[i].table.where : "none");
	}
	int fault = s_jt_get(&t, root, "fault");
	const char* fault_name = fault >= 0 ? s_jt_string(&t, fault, "exception") : NULL;
	if (!fault_name && fault >= 0) fault_name = s_jt_string(&t, fault, "signal");
	if (!fault_name) fault_name = s_jt_string(&t, root, "kind");
	cc_sha1 sig;
	int sig_used = 0;
	s_sha1_init(&sig);
	int stack = s_jt_get(&t, root, "stack");
	if (stack >= 0) s_resolve_frames(&t, stack, mods, mod_count, &sig, &sig_used, fault_name);
	int threads = s_jt_get(&t, root, "threads");
	for (int th = threads >= 0 ? t.nodes[threads].first : -1; th >= 0; th = t.nodes[th].next) {
		int st = s_jt_get(&t, th, "stack");
		if (st >= 0) s_resolve_frames(&t, st, mods, mod_count, NULL, &sig_used, NULL);
	}
	if (stack >= 0 && sig_used > 0) {
		unsigned char d[20];
		char hex[41];
		s_sha1_final(&sig, d);
		s_hex_bytes(d, 20, hex);
		int signature = s_jt_get(&t, root, "signature");
		if (signature < 0) signature = s_jt_add(&t, root, "signature", 5);
		if (signature >= 0) s_jt_set_str(&t, signature, "symbolic", hex);
	}
	bool ok = s_jt_save(&t, root, path);
	for (i = 0; i < mod_count; ++i) s_free_table(&mods[i].table);
	free(mods);
	s_jt_free(&t);
	return ok;
}

// --- the upload flow: pending reports in, nothing left on disk out -----------------------------------

typedef struct cc_pending { char paths[64][CC_PATH]; int count; } cc_pending;

static void s_collect_pending(const char* path, void* udata)
{
	cc_pending* p = (cc_pending*)udata;
	if (p->count < 64) s_strcpy(p->paths[p->count++], CC_PATH, path);
}

static void s_dump_path_of(const char* report_path, char* out)
{
	s_strcpy(out, CC_PATH, report_path);
	size_t n = s_strlen(out);
	if (n > 5) s_strcpy(out + n - 5, 6, ".dmp");
}

static cc_consent s_consent_load(void)
{
	size_t len = 0;
	char* text = s_file_read(s_cc.consent_path, &len);
	if (!text) return CC_CONSENT_ASK;
	cc_consent c = strncmp(text, "send", 4) == 0 ? CC_CONSENT_SEND : strncmp(text, "never", 5) == 0 ? CC_CONSENT_NEVER : CC_CONSENT_ASK;
	free(text);
	return c;
}

static void s_consent_store(cc_consent c)
{
	if (c == CC_CONSENT_SEND) s_file_write(s_cc.consent_path, "send\n", 5);
	else if (c == CC_CONSENT_NEVER) s_file_write(s_cc.consent_path, "never\n", 6);
}

static void s_upload_flow(const char* dir, bool may_ask)
{
	cc_pending* pending = (cc_pending*)calloc(1, sizeof(cc_pending));
	if (!pending) return;
	s_dir_each(dir, ".json", s_collect_pending, pending);
	if (pending->count == 0) { free(pending); return; }
	cc_consent consent = s_consent_load();
	if (consent == CC_CONSENT_ASK && may_ask) {
		consent = s_cc.cfg.ask ? s_cc.cfg.ask(s_cc.cfg.udata, pending->count) : CC_CONSENT_SEND;
		s_consent_store(consent);
	}
	if (consent == CC_CONSENT_ASK && !may_ask) consent = s_pending_consent;
	for (int i = 0; i < pending->count; ++i) {
		const char* path = pending->paths[i];
		char dump[CC_PATH];
		s_dump_path_of(path, dump);
		bool has_dump = s_file_exists(dump);
		if (consent == CC_CONSENT_NEVER) { s_file_delete(path); if (has_dump) s_file_delete(dump); continue; }
		s_resolve_report(path);
		if ((consent != CC_CONSENT_SEND && consent != CC_CONSENT_ONCE) || !s_cc.cfg.send) continue;
		if (s_cc.cfg.send(s_cc.cfg.udata, path, has_dump ? dump : NULL)) {
			s_file_delete(path);
			if (has_dump) s_file_delete(dump);
			continue;
		}
		size_t len = 0;
		char* text = s_file_read(path, &len);
		if (!text) continue;
		cc_jtree t;
		int root = s_jt_parse(&t, text);
		free(text);
		if (root < 0) continue;
		int64_t attempts = s_jt_int(&t, root, "attempts", 0) + 1;
		if (attempts > CC_UPLOAD_ATTEMPTS) { s_file_delete(path); if (has_dump) s_file_delete(dump); }
		else { s_jt_set_int(&t, root, "attempts", attempts); s_jt_save(&t, root, path); }
		s_jt_free(&t);
	}
	free(pending);
}

typedef struct cc_pid_match { uint32_t pid; bool found; } cc_pid_match;

static void s_pid_match_cb(const char* path, void* udata)
{
	cc_pid_match* m = (cc_pid_match*)udata;
	const char* b = s_basename(path);
	const char* dash = strrchr(b, '-');
	if (dash && (uint32_t)strtoul(dash + 1, NULL, 10) == m->pid) m->found = true;
}

// A run that never reached cc_shutdown: its marker becomes the report. State and breadcrumbs are
// loaded into the (still empty) shared block, written out, and cleared again. A run that crashed
// wrote its own report (named by its pid) and may have left the marker behind: that one is stale.
static void s_report_abnormal_exit(void)
{
	size_t len = 0;
	char* text = s_file_read(s_cc.marker_path, &len);
	if (!text) return;
	cc_jtree t;
	int root = s_jt_parse(&t, text);
	free(text);
	if (root < 0) return;
	cc_pid_match match;
	match.pid = (uint32_t)s_jt_int(&t, root, "pid", 0);
	match.found = false;
	s_dir_each(s_cc.report_dir, ".json", s_pid_match_cb, &match);
	if (match.found) { s_jt_free(&t); s_file_delete(s_cc.marker_path); return; }
	int state = s_jt_get(&t, root, "state");
	for (int c = state >= 0 ? t.nodes[state].first : -1; c >= 0; c = t.nodes[c].next) {
		if (t.nodes[c].type == 3 && t.nodes[c].key) cc_set(t.nodes[c].key, t.nodes[c].str);
	}
	int crumbs = s_jt_get(&t, root, "breadcrumbs");
	for (int c = crumbs >= 0 ? t.nodes[crumbs].first : -1; c >= 0; c = t.nodes[c].next) {
		const char* msg = s_jt_string(&t, c, "msg");
		if (!msg) continue;
		uint32_t i = s_atomic_inc32(&s_cc.sh->crumb_next) % CC_BREADCRUMBS;
		s_strcpy(s_cc.sh->crumbs[i].msg, CC_BREADCRUMB_BYTES, msg);
		int tn = s_jt_get(&t, c, "t");
		s_cc.sh->crumbs[i].t = tn >= 0 && t.nodes[tn].str ? strtod(t.nodes[tn].str, NULL) : 0.0;
	}
	s_jt_free(&t);
	cc_fault f;
	memset(&f, 0, sizeof(f));
	f.kind = "abnormal_exit";
	s_report_paths();
	s_modules_enumerate(); // This process is the same executable: its build ids key the report.
	s_write_report(&f, NULL, 0, NULL, 0);
	memset(s_cc.sh->state, 0, sizeof(s_cc.sh->state));
	memset(s_cc.sh->crumbs, 0, sizeof(s_cc.sh->crumbs));
	s_cc.sh->crumb_next = 0;
}

// --- the watchdog: one sleeping thread; a hang is two matching samples of the main thread ---------

static bool s_samples_match(const uint64_t* a, int na, const uint64_t* b, int nb)
{
	if (na < 1 || nb < 1) return false;
	int n = na < nb ? na : nb;
	if (n > 4) n = 4;
	for (int i = 0; i < n; ++i) if (a[i] != b[i]) return false;
	return true;
}

static void s_write_hang_report(const uint64_t* pcs, int count, double since)
{
	if (!s_atomic_cas32(&s_cc.sh->crash_state, CC_CRASH_IDLE, CC_CRASH_WRITING)) return;
	cc_fault f;
	memset(&f, 0, sizeof(f));
	f.kind = "hang";
	f.tid = s_cc.sh->main_tid;
	f.hang_seconds = since;
	s_report_paths();
#if defined(_WIN32)
	s_modules_enumerate();
#endif
	s_cc.has_dump = false;
	s_write_report(&f, pcs, count, NULL, 0);
	s_atomic_store32(&s_cc.sh->crash_state, CC_CRASH_IDLE);
	s_atomic_store32(&s_cc.hang_reported, 1);
	if (s_cc.cfg.on_hang) s_cc.cfg.on_hang(s_cc.cfg.udata);
	if (s_cc.cfg.upload_on_crash) s_spawn_uploader(); // A healthy process: the hang can go out now.
	if (s_cc.exit_after_hang) { cc_shutdown(); s_exit_process(0); } // The test kind: a clean exit, marker and all.
}

// --- cc_report: a report from a healthy thread, which continues ---------------------------------

#if !defined(CC_REPORTS_PER_RUN)
#	define CC_REPORTS_PER_RUN 8
#endif
static volatile uint32_t s_reports_made;
static volatile uint32_t s_report_guard; // Serializes the shared buffers; apart from the crash handlers' state word.

// The caller's stack: this function and cc_report are the two frames on top, dropped.
static CC_NOINLINE int s_self_frames(uint64_t* pcs, int cap)
{
	uint64_t raw[CC_MAX_FRAMES];
	int n;
#if defined(_WIN32)
	CONTEXT ctx;
	memset(&ctx, 0, sizeof(ctx));
	RtlCaptureContext(&ctx);
	n = s_walk_context(&ctx, raw, CC_MAX_FRAMES);
#else
	void* bt[CC_MAX_FRAMES];
	n = backtrace(bt, CC_MAX_FRAMES);
	for (int i = 0; i < n; ++i) raw[i] = (uint64_t)(uintptr_t)bt[i];
#endif
	int skip = n > 2 ? 2 : 0;
	int out = 0;
	for (int i = skip; i < n && out < cap; ++i) pcs[out++] = raw[i];
	return out;
}

void cc_report(const char* message)
{
	if (!s_cc.enabled || s_cc.in_child || !message) return;
	if (s_atomic_inc32(&s_reports_made) >= CC_REPORTS_PER_RUN) return;
	if (s_atomic_load32(&s_cc.sh->crash_state) != CC_CRASH_IDLE) return;
	if (!s_atomic_cas32(&s_report_guard, 0, 1)) return;
	cc_fault f;
	memset(&f, 0, sizeof(f));
	f.kind = "report";
	f.message = message;
	f.tid = s_current_tid();
	s_report_paths();
	// Its own file per call: report-<stamp>-<n>-<pid>.json, the pid last as every report name has it.
	{
		char seq[24];
		size_t n = s_strlen(s_cc.report_dir);
		s_strcpy(s_cc.report_path + n, CC_PATH - n, "report-");
		s_stamp_time(s_now_unix(), s_cc.report_path + n + 7);
		s_strcat(s_cc.report_path, CC_PATH, "-");
		s_utoa(s_atomic_load32(&s_reports_made), seq);
		s_strcat(s_cc.report_path, CC_PATH, seq);
		s_strcat(s_cc.report_path, CC_PATH, "-");
		s_utoa(s_current_pid(), seq);
		s_strcat(s_cc.report_path, CC_PATH, seq);
		s_strcat(s_cc.report_path, CC_PATH, ".json");
	}
	s_modules_enumerate();
	s_cc.has_dump = false;
	int count = s_self_frames(s_cc.pcs, CC_MAX_FRAMES);
	s_write_report(&f, s_cc.pcs, count, NULL, 0);
	s_atomic_store32(&s_report_guard, 0);
	if (s_cc.cfg.upload_on_crash) s_spawn_uploader();
}

static void s_watchdog_tick(void)
{
	int64_t now = s_now_ns();
	if (s_cc.marker_on && now - s_cc.marker_written_ns > 2000000000LL && !s_atomic_load32(&s_cc.sh->clean_shutdown) && s_atomic_load32(&s_cc.sh->crash_state) == CC_CRASH_IDLE) s_marker_write();
	if (s_cc.cfg.hang_seconds <= 0.0f || !s_cc.hang_on) return;
	if (s_atomic_load32(&s_cc.sh->pause_depth) > 0) return;
	if (s_atomic_load32(&s_cc.hang_reported)) return;
	int64_t beat = s_atomic_load64(&s_cc.sh->heartbeat_ns);
	double since = (double)(now - beat) / 1.0e9;
	if (since < (double)s_cc.cfg.hang_seconds) return;
	s_cc.sample_count[0] = s_sample_main_thread(s_cc.sample_pcs[0], CC_MAX_FRAMES);
	s_sleep_ms(3000);
	if (s_atomic_load64(&s_cc.sh->heartbeat_ns) != beat) return;
	s_cc.sample_count[1] = s_sample_main_thread(s_cc.sample_pcs[1], CC_MAX_FRAMES);
	if (!s_samples_match(s_cc.sample_pcs[0], s_cc.sample_count[0], s_cc.sample_pcs[1], s_cc.sample_count[1])) return;
	s_write_hang_report(s_cc.sample_pcs[1], s_cc.sample_count[1], (double)(s_now_ns() - beat) / 1.0e9);
}

#if defined(_WIN32)
static DWORD WINAPI s_watchdog_main(LPVOID p)
{
	(void)p;
	for (;;) { s_sleep_ms(500); s_watchdog_tick(); }
}
#else
static void* s_watchdog_main(void* p)
{
	(void)p;
	for (;;) { s_sleep_ms(500); s_watchdog_tick(); }
	return NULL;
}
#endif

static void s_watchdog_start(void)
{
	if (s_cc.watchdog_on) return;
	s_cc.watchdog_on = true;
#if defined(_WIN32)
	s_cc.watchdog_thread = CreateThread(NULL, 64 * 1024, s_watchdog_main, NULL, 0, NULL);
#else
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&s_cc.watchdog_thread, &attr, s_watchdog_main, NULL);
	pthread_attr_destroy(&attr);
#endif
}

// --- init ----------------------------------------------------------------------------------------

cc_config cc_defaults(void)
{
	cc_config c;
	memset(&c, 0, sizeof(c));
	c.minidump = true;
	c.install_id = true;
	return c;
}

static void s_entropy_init(void)
{
#if defined(_WIN32)
	typedef BOOLEAN (WINAPI *gen_random_fn)(PVOID, ULONG);
	HMODULE adv = LoadLibraryW(L"advapi32.dll");
	gen_random_fn gen = adv ? (gen_random_fn)(void*)GetProcAddress(adv, "SystemFunction036") : NULL;
	if (!gen || !gen(s_cc.entropy, sizeof(s_cc.entropy))) {
		LARGE_INTEGER c;
		QueryPerformanceCounter(&c);
		memcpy(s_cc.entropy, &c, sizeof(c));
	}
#else
	int fd = open("/dev/urandom", O_RDONLY);
	ssize_t got = fd >= 0 ? read(fd, s_cc.entropy, sizeof(s_cc.entropy)) : -1;
	if (fd >= 0) close(fd);
	if (got <= 0) { int64_t t = s_now_ns(); memcpy(s_cc.entropy, &t, sizeof(t)); }
#endif
}

static void s_exe_path_init(void)
{
#if defined(_WIN32)
	GetModuleFileNameW(NULL, s_cc.exe_w, CC_PATH);
	s_wide_to_utf8(s_cc.exe_w, s_cc.exe_path, CC_PATH);
#elif defined(__APPLE__)
	uint32_t size = CC_PATH;
	if (_NSGetExecutablePath(s_cc.exe_path, &size) != 0) s_cc.exe_path[0] = 0;
#else
	ssize_t n = readlink("/proc/self/exe", s_cc.exe_path, CC_PATH - 1);
	s_cc.exe_path[n > 0 ? n : 0] = 0;
#endif
}

static void s_default_app_name(void)
{
	s_strcpy(s_cc.app, sizeof(s_cc.app), s_basename(s_cc.exe_path));
	size_t n = s_strlen(s_cc.app);
	if (n > 4 && s_starts_with_nocase(s_cc.app + n - 4, ".exe")) s_cc.app[n - 4] = 0;
	if (!s_cc.app[0]) s_strcpy(s_cc.app, sizeof(s_cc.app), "app");
}

static void s_report_dir_init(void)
{
	char dir[CC_PATH];
	dir[0] = 0;
	if (s_cc.cfg.report_dir && s_cc.cfg.report_dir[0]) {
		s_strcpy(dir, CC_PATH, s_cc.cfg.report_dir);
	} else {
#if defined(_WIN32)
		wchar_t w[CC_PATH];
		DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", w, CC_PATH);
		if (n && n < CC_PATH) s_wide_to_utf8(w, dir, CC_PATH); else s_strcpy(dir, CC_PATH, ".");
		s_strcat(dir, CC_PATH, "\\");
		s_strcat(dir, CC_PATH, s_cc.app);
		s_strcat(dir, CC_PATH, "\\crash");
#elif defined(__APPLE__)
		const char* home = getenv("HOME");
		s_strcpy(dir, CC_PATH, home ? home : ".");
		s_strcat(dir, CC_PATH, "/Library/Application Support/");
		s_strcat(dir, CC_PATH, s_cc.app);
		s_strcat(dir, CC_PATH, "/crash");
#else
		const char* xdg = getenv("XDG_DATA_HOME");
		const char* home = getenv("HOME");
		if (xdg && xdg[0]) s_strcpy(dir, CC_PATH, xdg);
		else { s_strcpy(dir, CC_PATH, home ? home : "."); s_strcat(dir, CC_PATH, "/.local/share"); }
		s_strcat(dir, CC_PATH, "/");
		s_strcat(dir, CC_PATH, s_cc.app);
		s_strcat(dir, CC_PATH, "/crash");
#endif
	}
	size_t n = s_strlen(dir);
	if (n && dir[n - 1] != '/' && dir[n - 1] != '\\') {
#if defined(_WIN32)
		s_strcat(dir, CC_PATH, "\\");
#else
		s_strcat(dir, CC_PATH, "/");
#endif
	}
	s_strcpy(s_cc.report_dir, CC_PATH, dir);
	s_mkdir_p(s_cc.report_dir);
#if defined(_WIN32)
	s_utf8_to_wide(s_cc.report_dir, s_cc.report_dir_w, CC_PATH);
#endif
	s_strcpy(s_cc.marker_path, CC_PATH, s_cc.report_dir); s_strcat(s_cc.marker_path, CC_PATH, "running");
	s_strcpy(s_cc.consent_path, CC_PATH, s_cc.report_dir); s_strcat(s_cc.consent_path, CC_PATH, "consent");
	s_strcpy(s_cc.install_path, CC_PATH, s_cc.report_dir); s_strcat(s_cc.install_path, CC_PATH, "install");
	if (s_cc.cfg.sym_dir && s_cc.cfg.sym_dir[0]) {
		s_strcpy(s_cc.sym_dir, CC_PATH, s_cc.cfg.sym_dir);
		n = s_strlen(s_cc.sym_dir);
		if (n && s_cc.sym_dir[n - 1] != '/' && s_cc.sym_dir[n - 1] != '\\') s_strcat(s_cc.sym_dir, CC_PATH, "/");
	}
}

static void s_install_id_init(void)
{
	if (!s_cc.cfg.install_id) return;
	size_t len = 0;
	char* text = s_file_read(s_cc.install_path, &len);
	if (text && len >= 32) { s_strcpy(s_cc.install, 33, text); free(text); return; }
	free(text);
	cc_sha1 h;
	unsigned char d[20];
	s_sha1_init(&h);
	s_sha1_update(&h, s_cc.entropy, sizeof(s_cc.entropy));
	s_sha1_final(&h, d);
	s_hex_bytes(d, 16, s_cc.install);
	s_file_write(s_cc.install_path, s_cc.install, 32);
}

static void s_shared_init(cc_shared* sh)
{
	memset(sh, 0, sizeof(*sh));
	sh->magic = CC_SHARED_MAGIC;
	sh->pid = s_current_pid();
	sh->main_tid = s_current_tid();
	sh->start_ns = s_now_ns();
	sh->start_unix = s_now_unix();
	sh->heartbeat_ns = sh->start_ns;
}

static void s_pending_count_cb(const char* path, void* udata) { (void)path; (*(int*)udata)++; }

#if defined(_WIN32)
static DWORD WINAPI s_upload_thread(LPVOID p) { (void)p; s_upload_flow(s_cc.report_dir, false); return 0; }
#else
static void* s_upload_thread(void* p) { (void)p; s_upload_flow(s_cc.report_dir, false); return NULL; }
#endif

// Pending reports: consent on this thread (a dialog belongs here), the rest in the background.
static void s_upload_pending_async(void)
{
	int count = 0;
	s_dir_each(s_cc.report_dir, ".json", s_pending_count_cb, &count);
	if (count == 0) return;
	cc_consent consent = s_consent_load();
	if (consent == CC_CONSENT_ASK) {
		consent = s_cc.cfg.ask ? s_cc.cfg.ask(s_cc.cfg.udata, count) : CC_CONSENT_SEND;
		s_consent_store(consent);
	}
	s_pending_consent = consent;
#if defined(_WIN32)
	s_cc.upload_thread = CreateThread(NULL, 0, s_upload_thread, NULL, 0, NULL);
#else
	s_cc.upload_thread_live = pthread_create(&s_cc.upload_thread, NULL, s_upload_thread, NULL) == 0;
#endif
}

// An exit right after launch still gets its reports out: a bounded wait for the upload thread.
static void s_upload_join(void)
{
#if defined(_WIN32)
	if (s_cc.upload_thread) { WaitForSingleObject(s_cc.upload_thread, 3000); CloseHandle(s_cc.upload_thread); s_cc.upload_thread = NULL; }
#else
	if (s_cc.upload_thread_live) { pthread_join(s_cc.upload_thread, NULL); s_cc.upload_thread_live = false; }
#endif
}

static bool s_watcher_spawn(void);
static void s_watcher_main(int argc, char** argv);
static void s_watcher_notify_clean(void);

bool cc_init(cc_config config, int argc, char** argv)
{
	if (s_cc.inited) return s_cc.enabled;
	s_cc.inited = true;
	s_cc.cfg = config;
	s_cc.sh = &s_cc.sh_private;
	s_cc.argc = argc;
	s_cc.argv = argv;
	s_shared_init(s_cc.sh);
	s_exe_path_init();
	if (config.app && config.app[0]) s_strcpy(s_cc.app, sizeof(s_cc.app), config.app); else s_default_app_name();
	s_strcpy(s_cc.version, sizeof(s_cc.version), config.version);
	s_strcpy(s_cc.build, sizeof(s_cc.build), config.build);
	s_strcpy(s_cc.config_name, sizeof(s_cc.config_name), config.config);
	const char* test_kind = NULL;
	const char* upload_dir = NULL;
	int watch_at = -1;
	for (int i = 1; i < argc; ++i) {
		if (s_streq(argv[i], "--cc-test") && i + 1 < argc) test_kind = argv[++i];
		else if (s_streq(argv[i], "--cc-upload") && i + 1 < argc) upload_dir = argv[++i];
		else if (s_streq(argv[i], "--cc-watch")) { watch_at = i; break; }
	}
	if (upload_dir) s_cc.cfg.report_dir = upload_dir;
	if (watch_at >= 0 && watch_at + 5 < argc) s_cc.cfg.report_dir = argv[watch_at + 5];
	if (test_kind && s_streq(test_kind, "hang") && s_cc.cfg.hang_seconds <= 0.0f) s_cc.cfg.hang_seconds = 2.0f;
	s_cc.sh->hang_seconds = s_cc.cfg.hang_seconds;
	s_cc.sh->minidump = s_cc.cfg.minidump ? 1u : 0u;
	s_report_dir_init();
	s_cc.report = (char*)malloc(CC_REPORT_BYTES);
	s_cc.marker = (char*)malloc(CC_MARKER_BYTES);
	if (!s_cc.report || !s_cc.marker) return false;
	s_entropy_init();
	s_machine_init();
	s_install_id_init();
	if (upload_dir) {
		s_cc.in_child = true;
		s_cc.enabled = true;
		s_upload_flow(s_cc.report_dir, true);
		exit(0);
	}
	if (watch_at >= 0) {
		s_cc.in_child = true;
		s_cc.enabled = true;
		s_watcher_main(argc - watch_at, argv + watch_at);
		exit(0);
	}
	if (!test_kind) {
		if (s_env_is("CC_DISABLE", '1') || s_debugger_present()) return false;
	}
	s_cc.enabled = true;
	if (s_cc.cfg.mode == CC_MODE_WATCHER) {
		if (!s_watcher_spawn()) s_cc.cfg.mode = CC_MODE_INPROCESS;
	}
	// The marker is the in-process record of a run; on Windows the watcher's region replaces it.
#if defined(_WIN32)
	s_cc.marker_on = s_cc.cfg.mode == CC_MODE_INPROCESS;
#else
	s_cc.marker_on = true;
#endif
	s_cc.hang_on = s_cc.cfg.mode == CC_MODE_INPROCESS;
	if (s_cc.marker_on) {
		if (s_file_exists(s_cc.marker_path)) s_report_abnormal_exit();
		s_marker_write();
	}
	atexit(cc_shutdown);
	s_install_handlers();
	cc_attach_thread("main");
#if !defined(_WIN32)
	s_modules_enumerate();
	s_modules_refreshed_ns = s_now_ns();
#endif
	s_prepare_uploader();
#if defined(__cplusplus)
	s_install_terminate();
#endif
	s_watchdog_start();
	s_upload_pending_async();
	if (test_kind) s_run_test(test_kind);
	return true;
}

void cc_shutdown(void)
{
	if (!s_cc.enabled || s_cc.in_child) return;
	s_atomic_store32(&s_cc.sh->clean_shutdown, 1);
	if (s_cc.marker_on) s_file_delete(s_cc.marker_path);
	s_watcher_notify_clean();
	s_upload_join();
}

void cc_attach_thread(const char* name)
{
	if (!s_cc.enabled) return;
	s_thread_register(name);
	s_attach_thread_platform();
}

// --- the watcher (CC_MODE_WATCHER) ----------------------------------------------------------------
// Windows: the game's handler copies the exception into the shared region and signals; the watcher
// walks the game's stack from outside, writes the dump and the report, and uploads. Hangs and a death
// without cc_shutdown are seen from outside too. POSIX: the game reports in-process as always; the
// watcher only notices the pipe closing and uploads at once.

#if defined(_WIN32)

static void s_region_publish_crash(uint32_t kind, const char* name, const void* record, const void* context, uint64_t pointers)
{
	cc_shared* sh = s_cc.sh;
	sh->crash_kind = kind;
	s_strcpy(sh->crash_name, sizeof(sh->crash_name), name ? name : "");
	if (record) sh->crash_record = *(const EXCEPTION_RECORD*)record;
	if (context) sh->crash_context = *(const CONTEXT*)context;
	sh->crash_pointers = pointers;
	s_atomic_store32(&sh->crash_state, CC_CRASH_DONE);
}

static bool s_watcher_spawn(void)
{
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	s_cc.region_mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0, (DWORD)sizeof(cc_shared), NULL);
	if (!s_cc.region_mapping) return false;
	cc_shared* view = (cc_shared*)MapViewOfFile(s_cc.region_mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (!view) { CloseHandle(s_cc.region_mapping); s_cc.region_mapping = NULL; return false; }
	memcpy(view, &s_cc.sh_private, sizeof(cc_shared));
	s_cc.sh = view;
	s_cc.ev_crash = CreateEventW(&sa, FALSE, FALSE, NULL);
	s_cc.ev_done = CreateEventW(&sa, FALSE, FALSE, NULL);
	wchar_t cmd[CC_PATH * 2];
	wchar_t num[32];
	cmd[0] = 0;
	wcscat_s(cmd, CC_PATH * 2, L"\"");
	wcscat_s(cmd, CC_PATH * 2, s_cc.exe_w);
	wcscat_s(cmd, CC_PATH * 2, L"\" --cc-watch ");
	_ui64tow_s(GetCurrentProcessId(), num, 32, 10); wcscat_s(cmd, CC_PATH * 2, num); wcscat_s(cmd, CC_PATH * 2, L" ");
	_ui64tow_s((uintptr_t)s_cc.region_mapping, num, 32, 10); wcscat_s(cmd, CC_PATH * 2, num); wcscat_s(cmd, CC_PATH * 2, L" ");
	_ui64tow_s((uintptr_t)s_cc.ev_crash, num, 32, 10); wcscat_s(cmd, CC_PATH * 2, num); wcscat_s(cmd, CC_PATH * 2, L" ");
	_ui64tow_s((uintptr_t)s_cc.ev_done, num, 32, 10); wcscat_s(cmd, CC_PATH * 2, num);
	wcscat_s(cmd, CC_PATH * 2, L" ");
	s_cat_quoted_dir(cmd, CC_PATH * 2);
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	if (!CreateProcessW(s_cc.exe_w, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) { s_debugf("watcher spawn failed: %lu", (unsigned long)GetLastError()); return false; }
	s_debugf("watcher spawned: pid %lu", (unsigned long)pi.dwProcessId);
	CloseHandle(pi.hThread);
	s_cc.watcher_process = pi.hProcess;
	return true;
}

static void s_watcher_notify_clean(void) {}

// dbghelp in the watcher: a healthy process, so the symbol engine is fair game here.
static struct
{
	BOOL (WINAPI *StackWalk64)(DWORD, HANDLE, HANDLE, LPSTACKFRAME64, PVOID, PREAD_PROCESS_MEMORY_ROUTINE64, PFUNCTION_TABLE_ACCESS_ROUTINE64, PGET_MODULE_BASE_ROUTINE64, PTRANSLATE_ADDRESS_ROUTINE64);
	BOOL (WINAPI *SymInitializeW)(HANDLE, PCWSTR, BOOL);
	BOOL (WINAPI *SymCleanup)(HANDLE);
	PVOID (WINAPI *SymFunctionTableAccess64)(HANDLE, DWORD64);
	DWORD64 (WINAPI *SymGetModuleBase64)(HANDLE, DWORD64);
	BOOL (WINAPI *EnumProcessModulesEx)(HANDLE, HMODULE*, DWORD, LPDWORD, DWORD);
	DWORD (WINAPI *GetModuleFileNameExW)(HANDLE, HMODULE, LPWSTR, DWORD);
} s_wd;

static BOOL CALLBACK s_read_remote(HANDLE process, DWORD64 base, PVOID buf, DWORD size, LPDWORD read)
{
	SIZE_T got = 0;
	BOOL ok = ReadProcessMemory(process, (LPCVOID)(uintptr_t)base, buf, size, &got);
	*read = (DWORD)got;
	return ok;
}

static void s_watcher_load_dbghelp(void)
{
	HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
	s_wd.EnumProcessModulesEx = (BOOL (WINAPI *)(HANDLE, HMODULE*, DWORD, LPDWORD, DWORD))(void*)GetProcAddress(k32, "K32EnumProcessModulesEx");
	s_wd.GetModuleFileNameExW = (DWORD (WINAPI *)(HANDLE, HMODULE, LPWSTR, DWORD))(void*)GetProcAddress(k32, "K32GetModuleFileNameExW");
	if (!s_cc.dbghelp) s_cc.dbghelp = LoadLibraryW(L"dbghelp.dll");
	if (!s_cc.dbghelp) return;
	s_cc.MiniDumpWriteDump = (BOOL (WINAPI *)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE, PMINIDUMP_EXCEPTION_INFORMATION, PMINIDUMP_USER_STREAM_INFORMATION, PMINIDUMP_CALLBACK_INFORMATION))(void*)GetProcAddress(s_cc.dbghelp, "MiniDumpWriteDump");
	s_wd.StackWalk64 = (BOOL (WINAPI *)(DWORD, HANDLE, HANDLE, LPSTACKFRAME64, PVOID, PREAD_PROCESS_MEMORY_ROUTINE64, PFUNCTION_TABLE_ACCESS_ROUTINE64, PGET_MODULE_BASE_ROUTINE64, PTRANSLATE_ADDRESS_ROUTINE64))(void*)GetProcAddress(s_cc.dbghelp, "StackWalk64");
	s_wd.SymInitializeW = (BOOL (WINAPI *)(HANDLE, PCWSTR, BOOL))(void*)GetProcAddress(s_cc.dbghelp, "SymInitializeW");
	s_wd.SymCleanup = (BOOL (WINAPI *)(HANDLE))(void*)GetProcAddress(s_cc.dbghelp, "SymCleanup");
	s_wd.SymFunctionTableAccess64 = (PVOID (WINAPI *)(HANDLE, DWORD64))(void*)GetProcAddress(s_cc.dbghelp, "SymFunctionTableAccess64");
	s_wd.SymGetModuleBase64 = (DWORD64 (WINAPI *)(HANDLE, DWORD64))(void*)GetProcAddress(s_cc.dbghelp, "SymGetModuleBase64");
}

// The game's modules, read through its memory.
static void s_modules_enumerate_remote(HANDLE process)
{
	s_cc.module_count = 0;
	if (!s_wd.EnumProcessModulesEx) return;
	static HMODULE mods[CC_MAX_MODULES];
	DWORD needed = 0;
	if (!s_wd.EnumProcessModulesEx(process, mods, sizeof(mods), &needed, 0x03)) return;
	int count = (int)(needed / sizeof(HMODULE));
	if (count > CC_MAX_MODULES) count = CC_MAX_MODULES;
	for (int i = 0; i < count; ++i) {
		cc_module* m = s_module_add();
		if (!m) break;
		m->base = (uint64_t)(uintptr_t)mods[i];
		m->offset_base = m->base;
		wchar_t w[CC_MODULE_PATH];
		if (s_wd.GetModuleFileNameExW && s_wd.GetModuleFileNameExW(process, mods[i], w, CC_MODULE_PATH)) s_wide_to_utf8(w, m->path, CC_MODULE_PATH);
		unsigned char hdr[4096];
		SIZE_T got = 0;
		if (ReadProcessMemory(process, mods[i], hdr, sizeof(hdr), &got) && got >= 1024) {
			const IMAGE_DOS_HEADER* dos = (const IMAGE_DOS_HEADER*)hdr;
			if (dos->e_magic == IMAGE_DOS_SIGNATURE && dos->e_lfanew > 0 && (size_t)dos->e_lfanew + sizeof(IMAGE_NT_HEADERS) <= got) {
				const IMAGE_NT_HEADERS* nt = (const IMAGE_NT_HEADERS*)(hdr + dos->e_lfanew);
				m->size = nt->OptionalHeader.SizeOfImage;
				const IMAGE_DATA_DIRECTORY* dir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
				IMAGE_DEBUG_DIRECTORY dbg[16];
				int n = (int)(dir->Size / sizeof(IMAGE_DEBUG_DIRECTORY));
				if (n > 16) n = 16;
				if (dir->VirtualAddress && n > 0 && ReadProcessMemory(process, (LPCVOID)(uintptr_t)(m->base + dir->VirtualAddress), dbg, (SIZE_T)n * sizeof(IMAGE_DEBUG_DIRECTORY), &got)) {
					for (int d = 0; d < n; ++d) {
						unsigned char cv[64];
						if (dbg[d].Type != IMAGE_DEBUG_TYPE_CODEVIEW || !dbg[d].AddressOfRawData || dbg[d].SizeOfData < 24) continue;
						if (!ReadProcessMemory(process, (LPCVOID)(uintptr_t)(m->base + dbg[d].AddressOfRawData), cv, 24, &got) || got < 24) continue;
						s_cv_build_id(cv, m);
						break;
					}
				}
			}
		}
		s_module_finish(m);
	}
}

static int s_walk_remote(HANDLE process, HANDLE thread, CONTEXT* ctx, uint64_t* pcs, int cap)
{
	if (!s_wd.StackWalk64) return 0;
	STACKFRAME64 frame;
	DWORD machine;
	memset(&frame, 0, sizeof(frame));
#if defined(_M_X64)
	machine = IMAGE_FILE_MACHINE_AMD64;
	frame.AddrPC.Offset = ctx->Rip; frame.AddrFrame.Offset = ctx->Rbp; frame.AddrStack.Offset = ctx->Rsp;
#elif defined(_M_ARM64)
	machine = IMAGE_FILE_MACHINE_ARM64;
	frame.AddrPC.Offset = ctx->Pc; frame.AddrFrame.Offset = ctx->Fp; frame.AddrStack.Offset = ctx->Sp;
#else
	machine = IMAGE_FILE_MACHINE_I386;
	frame.AddrPC.Offset = ctx->Eip; frame.AddrFrame.Offset = ctx->Ebp; frame.AddrStack.Offset = ctx->Esp;
#endif
	frame.AddrPC.Mode = AddrModeFlat; frame.AddrFrame.Mode = AddrModeFlat; frame.AddrStack.Mode = AddrModeFlat;
	int n = 0;
	while (n < cap) {
		if (!s_wd.StackWalk64(machine, process, thread, &frame, ctx, s_read_remote, s_wd.SymFunctionTableAccess64, s_wd.SymGetModuleBase64, NULL)) break;
		if (!frame.AddrPC.Offset) break;
		pcs[n++] = frame.AddrPC.Offset;
	}
	return n;
}

static int s_sample_remote_main(HANDLE process, uint64_t* pcs, int cap)
{
	HANDLE t = OpenThread(THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION, FALSE, s_cc.sh->main_tid);
	if (!t) { s_debugf("watcher: OpenThread(%u) failed %lu", s_cc.sh->main_tid, (unsigned long)GetLastError()); return 0; }
	int n = 0;
	if (SuspendThread(t) != (DWORD)-1) {
		CONTEXT ctx;
		memset(&ctx, 0, sizeof(ctx));
		ctx.ContextFlags = CONTEXT_FULL;
		if (GetThreadContext(t, &ctx)) n = s_walk_remote(process, t, &ctx, pcs, cap);
		ResumeThread(t);
	}
	CloseHandle(t);
	return n;
}

static void s_watcher_write_dump(HANDLE process, DWORD pid)
{
	if (!s_cc.MiniDumpWriteDump || !s_cc.sh->minidump) return;
	wchar_t wpath[CC_PATH];
	s_utf8_to_wide(s_cc.dump_path, wpath, CC_PATH);
	HANDLE h = CreateFileW(wpath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) return;
	MINIDUMP_EXCEPTION_INFORMATION mei;
	mei.ThreadId = s_cc.sh->crash_tid;
	mei.ExceptionPointers = (PEXCEPTION_POINTERS)(uintptr_t)s_cc.sh->crash_pointers;
	mei.ClientPointers = TRUE;
	// Stacks, what they point at, and the threads: locals in a debugger at a few MB. Not the data
	// segments: every global of a static build, the symbol slot included, is tens of MB.
	MINIDUMP_TYPE type = (MINIDUMP_TYPE)(MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithThreadInfo | MiniDumpWithHandleData);
	BOOL ok = s_cc.MiniDumpWriteDump(process, pid, h, type, mei.ExceptionPointers ? &mei : NULL, NULL, NULL);
	LARGE_INTEGER size;
	size.QuadPart = 0;
	if (ok) GetFileSizeEx(h, &size);
	CloseHandle(h);
	if (ok && size.QuadPart > 0) { s_cc.has_dump = true; s_cc.dump_bytes = (uint64_t)size.QuadPart; }
	else DeleteFileW(wpath);
}

static void s_watcher_on_crash(HANDLE process, DWORD pid)
{
	cc_shared* sh = s_cc.sh;
	cc_fault f;
	memset(&f, 0, sizeof(f));
	f.kind = "crash";
	f.tid = sh->crash_tid;
	if (sh->crash_kind == 1) {
		const EXCEPTION_RECORD* r = &sh->crash_record;
		f.name = s_exception_name(r->ExceptionCode);
		f.code = r->ExceptionCode;
		if ((r->ExceptionCode == EXCEPTION_ACCESS_VIOLATION || r->ExceptionCode == EXCEPTION_IN_PAGE_ERROR) && r->NumberParameters >= 2) {
			f.access = r->ExceptionInformation[0] == 0 ? "read" : r->ExceptionInformation[0] == 1 ? "write" : r->ExceptionInformation[0] == 8 ? "execute" : "unknown";
			f.address = (uint64_t)r->ExceptionInformation[1];
		} else {
			f.address = (uint64_t)(uintptr_t)r->ExceptionAddress;
		}
		f.has_address = true;
	} else {
		f.name = sh->crash_name;
	}
	s_report_paths();
	s_modules_enumerate_remote(process);
	HANDLE t = OpenThread(THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, sh->crash_tid);
	CONTEXT ctx = sh->crash_context;
	s_cc.pc_count = s_walk_remote(process, t, &ctx, s_cc.pcs, CC_MAX_FRAMES);
	if (t) CloseHandle(t);
	s_watcher_write_dump(process, pid);
	s_write_report(&f, s_cc.pcs, s_cc.pc_count, NULL, 0);
}

static void s_watcher_hang_tick(HANDLE process, int64_t* last_beat, bool* reported)
{
	cc_shared* sh = s_cc.sh;
	if (sh->hang_seconds <= 0.0f) return;
	int64_t beat = s_atomic_load64(&sh->heartbeat_ns);
	if (beat != *last_beat) { *last_beat = beat; *reported = false; }
	if (*reported || s_atomic_load32(&sh->pause_depth) > 0) return;
	double since = (double)(s_now_ns() - beat) / 1.0e9;
	if (since < (double)sh->hang_seconds) return;
	s_cc.sample_count[0] = s_sample_remote_main(process, s_cc.sample_pcs[0], CC_MAX_FRAMES);
	s_sleep_ms(3000);
	if (s_atomic_load64(&sh->heartbeat_ns) != beat) return;
	s_cc.sample_count[1] = s_sample_remote_main(process, s_cc.sample_pcs[1], CC_MAX_FRAMES);
	s_debugf("watcher: hang %.1fs samples %d %d top %llx %llx", since, s_cc.sample_count[0], s_cc.sample_count[1], (unsigned long long)s_cc.sample_pcs[0][0], (unsigned long long)s_cc.sample_pcs[1][0]);
	if (!s_samples_match(s_cc.sample_pcs[0], s_cc.sample_count[0], s_cc.sample_pcs[1], s_cc.sample_count[1])) return;
	cc_fault f;
	memset(&f, 0, sizeof(f));
	f.kind = "hang";
	f.tid = sh->main_tid;
	f.hang_seconds = (double)(s_now_ns() - beat) / 1.0e9;
	s_report_paths();
	s_modules_enumerate_remote(process);
	s_cc.has_dump = false;
	s_write_report(&f, s_cc.sample_pcs[1], s_cc.sample_count[1], NULL, 0);
	*reported = true;
}

static DWORD WINAPI s_watcher_deadline(LPVOID p) { (void)p; Sleep(60000); TerminateProcess(GetCurrentProcess(), 0); return 0; }

static void s_watcher_main(int argc, char** argv)
{
	if (argc < 6) exit(1);
	DWORD pid = (DWORD)strtoul(argv[1], NULL, 10);
	HANDLE mapping = (HANDLE)(uintptr_t)strtoull(argv[2], NULL, 10);
	HANDLE ev_crash = (HANDLE)(uintptr_t)strtoull(argv[3], NULL, 10);
	HANDLE ev_done = (HANDLE)(uintptr_t)strtoull(argv[4], NULL, 10);
	HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | SYNCHRONIZE, FALSE, pid);
	cc_shared* view = (cc_shared*)MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	s_debugf("watcher: pid %lu process %p view %p magic %08x err %lu dir %s", (unsigned long)pid, (void*)process, (void*)view, view ? view->magic : 0, (unsigned long)GetLastError(), s_cc.report_dir);
	if (!process || !view || view->magic != CC_SHARED_MAGIC) exit(1);
	s_cc.sh = view;
	s_watcher_load_dbghelp();
	if (s_wd.SymInitializeW) s_wd.SymInitializeW(process, NULL, TRUE);
	HANDLE waits[2];
	waits[0] = process;
	waits[1] = ev_crash;
	int64_t last_beat = 0;
	bool reported = false;
	for (;;) {
		DWORD r = WaitForMultipleObjects(2, waits, FALSE, 500);
		if (r != WAIT_TIMEOUT) s_debugf("watcher: wait %lu clean %u crash_state %u", (unsigned long)r, view->clean_shutdown, view->crash_state);
		if (r == WAIT_OBJECT_0 + 1) {
			s_watcher_on_crash(process, pid);
			SetEvent(ev_done);
			break;
		}
		if (r == WAIT_OBJECT_0) {
			if (!s_atomic_load32(&view->clean_shutdown) && s_atomic_load32(&view->crash_state) != CC_CRASH_DONE) {
				cc_fault f;
				memset(&f, 0, sizeof(f));
				f.kind = "abnormal_exit";
				s_report_paths();
				s_modules_enumerate(); // The watcher is the same executable: its build ids key the report.
				s_cc.has_dump = false;
				s_write_report(&f, NULL, 0, NULL, 0);
			}
			break;
		}
		if (r == WAIT_TIMEOUT) s_watcher_hang_tick(process, &last_beat, &reported);
		else break;
	}
	if (s_wd.SymCleanup) s_wd.SymCleanup(process);
	HANDLE deadline = CreateThread(NULL, 0, s_watcher_deadline, NULL, 0, NULL);
	if (deadline) CloseHandle(deadline);
	s_debugf("watcher: report %s dump %d", s_cc.report_path, (int)s_cc.has_dump);
	s_cc.sh = &s_cc.sh_private;
	s_upload_flow(s_cc.report_dir, true);
	exit(0);
}

#else

// Unverified on this host. The game keeps the pipe's write end; the watcher reads until it closes.
// A 'c' before the close is cc_shutdown; silence is a death the marker file describes.
static bool s_watcher_spawn(void)
{
	if (pipe(s_cc.watch_pipe) != 0) return false;
	fcntl(s_cc.watch_pipe[1], F_SETFD, FD_CLOEXEC);
	char fd[16];
	s_utoa((uint64_t)s_cc.watch_pipe[0], fd);
	pid_t pid = fork();
	if (pid < 0) { close(s_cc.watch_pipe[0]); close(s_cc.watch_pipe[1]); return false; }
	if (pid == 0) {
		char flag[] = "--cc-watch";
		char* argv[4];
		argv[0] = s_cc.exe_path; argv[1] = flag; argv[2] = fd; argv[3] = NULL;
		execve(s_cc.exe_path, argv, environ);
		_exit(127);
	}
	close(s_cc.watch_pipe[0]);
	return true;
}

static void s_watcher_notify_clean(void)
{
	if (s_cc.cfg.mode != CC_MODE_WATCHER) return;
	ssize_t w = write(s_cc.watch_pipe[1], "c", 1);
	(void)w;
	close(s_cc.watch_pipe[1]);
}

static void s_watcher_main(int argc, char** argv)
{
	if (argc < 2) exit(1);
	int fd = atoi(argv[1]);
	char last = 0, c;
	for (;;) {
		ssize_t n = read(fd, &c, 1);
		if (n < 0 && errno == EINTR) continue;
		if (n <= 0) break;
		last = c;
	}
	if (last != 'c' && s_file_exists(s_cc.marker_path)) {
		s_report_abnormal_exit();
		s_file_delete(s_cc.marker_path);
	}
	s_upload_flow(s_cc.report_dir, true);
	exit(0);
}

#endif

#endif // __EMSCRIPTEN__
#endif // CUTE_CRASH_IMPLEMENTATION_ONCE
#endif // CUTE_CRASH_IMPLEMENTATION

/*
	------------------------------------------------------------------------------
	This software is available under 2 licenses - you may choose the one you like.
	------------------------------------------------------------------------------
	ALTERNATIVE A - zlib license
	Copyright (c) 2026 bullno1, Randy Gaul
	This software is provided 'as-is', without any express or implied warranty.
	In no event will the authors be held liable for any damages arising from
	the use of this software.
	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:
	  1. The origin of this software must not be misrepresented; you must not
	     claim that you wrote the original software. If you use this software
	     in a product, an acknowledgment in the product documentation would be
	     appreciated but is not required.
	  2. Altered source versions must be plainly marked as such, and must not
	     be misrepresented as being the original software.
	  3. This notice may not be removed or altered from any source distribution.
	------------------------------------------------------------------------------
	ALTERNATIVE B - Public Domain (www.unlicense.org)
	This is free and unencumbered software released into the public domain.
	Anyone is free to copy, modify, publish, use, compile, sell, or distribute
	this software, either in source code form or as a compiled binary, for any
	purpose, commercial or non-commercial, and by any means.
	In jurisdictions that recognize copyright laws, the author or authors of
	this software dedicate any and all copyright interest in the software to
	the public domain. We make this dedication for the benefit of the public
	at large and to the detriment of our heirs and successors. We intend this
	dedication to be an overt act of relinquishment in perpetuity of all
	present and future rights to this software under copyright law.
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
	ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
	------------------------------------------------------------------------------
*/
