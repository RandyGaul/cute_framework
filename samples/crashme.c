// crashme: the crash reporter end to end. Press a key to crash the way it names; the report
// lands in the report directory (--report-dir, else the per-user default), and the next launch
// of crashme resolves it against the table cute-sym embedded in this binary (cf_symbols(crashme
// EMBED) in samples/CMakeLists.txt) and prints it. Pass --upload <https url> to see it posted.
//
// The test harness drives it two ways: the reporter's own flag, `--cc-test <kind>`, which crashes
// inside cf_crash_init before the window exists, and `--crash <n>`, which presses key n on the
// third frame so the crash site is a known line in this file reached through the app loop.

#include <cute.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int s_worker(void* udata)
{
	(void)udata;
	volatile int* p = (volatile int*)0x18;
	*p = 1;
	return 0;
}

static int s_overflow(int n)
{
	volatile char pad[512];
	pad[0] = (char)n;
	return s_overflow(n + 1) + pad[0];
}

static void s_crash(int kind)
{
	cf_crash_breadcrumb("crashme: key %d", kind);
	switch (kind) {
	case 1: { volatile int* p = (volatile int*)0x18; *p = 1; } break;
	case 2: s_overflow(0); break;
	case 3: abort(); break;
	case 4: cf_thread_detach(cf_thread_create(s_worker, "worker", NULL)); break;
	case 5: for (;;) cf_sleep(100); // No heartbeat: the watchdog reports a hang after hang_seconds.
	case 6: { volatile int players = 9; CF_ASSERT(players <= 8); } break; // The assert's text rides in the report.
	case 7: cf_crash_report("a report that is not a crash: key %d", kind); break; // A stack and a message; the game goes on.
	default: break;
	}
}

int main(int argc, char* argv[])
{
	CF_CrashConfig cc = cf_crash_defaults();
	cc.version = "1.0";
	cc.build = "crashme";
	cc.hang_seconds = 5.0f;
	for (int i = 1; i + 1 < argc; ++i) {
		if (!strcmp(argv[i], "--report-dir")) cc.report_dir = argv[++i];
		else if (!strcmp(argv[i], "--upload")) cc.upload_url = argv[++i];
		else if (!strcmp(argv[i], "--hang-seconds")) cc.hang_seconds = (float)atof(argv[++i]);
		else if (!strcmp(argv[i], "--watcher")) cc.mode = CF_CRASH_MODE_WATCHER;
		else if (!strcmp(argv[i], "--token")) cc.upload_token = argv[++i];
	}
	int crash_on_frame = 0, frames = 0; // --frames n: exit after n frames (the harness's clean run).
	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "--no-ask")) cc.ask_consent = false;
		if (!strcmp(argv[i], "--crash") && i + 1 < argc) crash_on_frame = atoi(argv[i + 1]);
		if (!strcmp(argv[i], "--frames") && i + 1 < argc) frames = atoi(argv[i + 1]);
		if (!strcmp(argv[i], "--upload-on-crash")) cc.upload_on_crash = true;
	}
	bool active = cf_crash_init(cc, argc, argv);
	printf("crash reporter %s\n", active ? "active" : "inactive (debugger or CC_DISABLE)");

	CF_Result result = cf_make_app("crashme", 0, 0, 0, 640, 300, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | (crash_on_frame || frames ? CF_APP_OPTIONS_HIDDEN_BIT : 0), argv[0]);
	if (cf_is_error(result)) return -1;
	cf_crash_set("scene", "menu");

	int frame = 0;
	while (cf_app_is_running()) {
		cf_app_update(NULL);
		++frame;
		if (crash_on_frame && frame == 3) s_crash(crash_on_frame);
		if (frames && frame >= frames) cf_app_signal_shutdown();
		for (int k = 1; k <= 7; ++k) {
			if (cf_key_just_pressed((CF_KeyButton)(CF_KEY_0 + k))) s_crash(k);
		}
		cf_push_font_size(18);
		cf_draw_text("crashme: press a key", cf_v2(-300, 110), -1);
		cf_draw_text("1  null write", cf_v2(-300, 70), -1);
		cf_draw_text("2  stack overflow", cf_v2(-300, 40), -1);
		cf_draw_text("3  abort", cf_v2(-300, 10), -1);
		cf_draw_text("4  crash on a worker thread", cf_v2(-300, -20), -1);
		cf_draw_text("5  hang (reported after 5 s, the window stays dead)", cf_v2(-300, -50), -1);
		cf_draw_text("6  failed CF_ASSERT", cf_v2(-300, -80), -1);
		cf_draw_text("7  a report without a crash", cf_v2(-300, -110), -1);
		cf_draw_text(active ? "reporter active" : "reporter inactive", cf_v2(-300, -140), -1);
		cf_pop_font_size();
		cf_app_draw_onto_screen(true);
	}

	cf_destroy_app();
	return 0;
}
