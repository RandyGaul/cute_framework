/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// The framework's face of cute_crash.h: cc_ does the catching, the report, the tables and the
// upload flow; this file supplies what cc_ does not assume -- HTTPS through cf_https, the consent
// box through SDL, the heartbeat from the app loop, thread attachment from cf_thread_create, and
// the machine section once the app exists.

#include <cute_crash.h>
#include <cute_https.h>
#include <cute_app.h>
#include <cute_graphics.h>
#include <cute_string.h>
#include <cute_alloc.h>
#include <cute_time.h>
#include <internal/cute_app_internal.h>
#include <internal/cute_graphics_internal.h>
#include <internal/cute_crash_internal.h>

#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define CUTE_CRASH_IMPLEMENTATION
#include <cute/cute_crash.h>

using namespace Cute;

#define CF_CRASH_ATTACHMENT_MAX (8 * 1024 * 1024)

static CF_CrashConfig s_config;
static bool s_active;
static char s_host[256];
static char s_uri[1024];
static int s_port = 443;
static char s_app_name[128] = "The game";
static cf_assert_fn* s_prev_assert;

// "https://host[:port]/path" into host, port, uri. Only https; a report never travels in the clear.
static bool s_parse_url(const char* url)
{
	const char* p = url;
	if (strncmp(p, "https://", 8) != 0) return false;
	p += 8;
	const char* slash = strchr(p, '/');
	const char* colon = strchr(p, ':');
	size_t host_len = slash ? (size_t)(slash - p) : strlen(p);
	if (colon && (!slash || colon < slash)) {
		host_len = (size_t)(colon - p);
		s_port = atoi(colon + 1);
	}
	if (host_len == 0 || host_len >= sizeof(s_host)) return false;
	memcpy(s_host, p, host_len);
	s_host[host_len] = 0;
	snprintf(s_uri, sizeof(s_uri), "%s", slash ? slash : "/");
	return true;
}

static bool s_read_file(const char* path, Array<char>* out)
{
	FILE* f = fopen(path, "rb");
	if (!f) return false;
	fseek(f, 0, SEEK_END);
	long n = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (n < 0) { fclose(f); return false; }
	out->set_count((int)n);
	size_t got = n ? fread(out->data(), 1, (size_t)n, f) : 0;
	fclose(f);
	return got == (size_t)n;
}

static const char* s_file_name(const char* path)
{
	const char* a = strrchr(path, '/');
	const char* b = strrchr(path, '\\');
	const char* s = a > b ? a : b;
	return s ? s + 1 : path;
}

#ifndef CF_EMSCRIPTEN
// One multipart POST: the report as "report", the attachment as "crash.dmp" when there is one.
// Any reply at all is the ack; only a dead connection is a failure.
static bool s_send(void* udata, const char* report_path, const char* attachment_path)
{
	(void)udata;
	Array<char> report, dump;
	if (!s_read_file(report_path, &report)) return false;
	// Over the cap the dump stays home and the report goes alone: a server's limit is the same by default.
	bool has_dump = attachment_path && s_read_file(attachment_path, &dump) && dump.count() <= CF_CRASH_ATTACHMENT_MAX;

	const char* boundary = "----cute_crash_7f3a9c1e";
	String body;
	body.fmt_append("--%s\r\nContent-Disposition: form-data; name=\"report\"; filename=\"%s\"\r\nContent-Type: application/json\r\n\r\n", boundary, s_file_name(report_path));
	body.append(report.data(), report.data() + report.count());
	body.append("\r\n");
	if (has_dump) {
		body.fmt_append("--%s\r\nContent-Disposition: form-data; name=\"crash.dmp\"; filename=\"%s\"\r\nContent-Type: application/octet-stream\r\n\r\n", boundary, s_file_name(attachment_path));
		body.append(dump.data(), dump.data() + dump.count());
		body.append("\r\n");
	}
	body.fmt_append("--%s--\r\n", boundary);

	CF_HttpsRequest request = cf_https_post(s_host, s_port, s_uri, body.c_str(), body.len(), true);
	// cf_https keeps header POINTERS, so every value lives until the request is done.
	String content_type = String::fmt("multipart/form-data; boundary=%s", boundary);
	String auth = String::fmt("Bearer %s", s_config.upload_token ? s_config.upload_token : "");
	cf_https_add_header(request, "Content-Type", content_type.c_str());
	cf_https_add_header(request, "User-Agent", "cute_crash/1");
	if (s_config.upload_token) cf_https_add_header(request, "Authorization", auth.c_str());
	CF_HttpsResult state;
	while ((state = cf_https_process(request)) == CF_HTTPS_RESULT_PENDING) cf_sleep(5);
	cf_https_destroy(request);
	return state == CF_HTTPS_RESULT_OK;
}
#else
static bool s_send(void* udata, const char* report_path, const char* attachment_path) { (void)udata; (void)report_path; (void)attachment_path; return false; }
#endif

static cc_consent s_ask(void* udata, int pending_count)
{
	(void)udata;
	SDL_MessageBoxButtonData buttons[3] = {
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Send" },
		{ 0, 1, "Always send" },
		{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 2, "Don't send" },
	};
	String text = String::fmt("%s ended unexpectedly last time. Send the crash report%s to the developer?\n\nThe report holds the program's state at the crash, not your files.", s_app_name, pending_count > 1 ? "s" : "");
	SDL_MessageBoxData data = { SDL_MESSAGEBOX_INFORMATION, NULL, "Crash report", text.c_str(), 3, buttons, NULL };
	int id = 2;
	if (!SDL_ShowMessageBox(&data, &id)) return CC_CONSENT_ASK;
	return id == 0 ? CC_CONSENT_ONCE : id == 1 ? CC_CONSENT_SEND : CC_CONSENT_NEVER;
}

// A failed assert is the message the crash is about: into the report, then on to whatever the
// assert handler does (CF's default breaks, which the reporter catches like any crash).
static void s_assert(bool expr, const char* message, const char* file, int line)
{
	if (!expr) {
		char text[256];
		snprintf(text, sizeof(text), "%s (%s:%d)", message, s_file_name(file), line);
		cc_set("assert", text);
		cc_breadcrumb("assert: %s", text);
		if (s_prev_assert) s_prev_assert(expr, message, file, line);
		cc_report(text); // The handler came back: the assert is a report of its own.
		return;
	}
	if (s_prev_assert) s_prev_assert(expr, message, file, line);
}

CF_CrashConfig cf_crash_defaults()
{
	CF_CrashConfig c = { 0 };
	cc_config d = cc_defaults();
	c.mode = CF_CRASH_MODE_INPROCESS;
	c.minidump = d.minidump;
	c.install_id = d.install_id;
	c.ask_consent = true;
	c.assert_reports = true;
	return c;
}

bool cf_crash_init(CF_CrashConfig config, int argc, char** argv)
{
	s_config = config;
	if (argc > 0 && argv && argv[0]) {
		snprintf(s_app_name, sizeof(s_app_name), "%s", s_file_name(argv[0]));
		char* dot = strrchr(s_app_name, '.');
		if (dot && dot != s_app_name) *dot = 0;
	}
	cc_config c = cc_defaults();
	c.version = config.version;
	c.build = config.build;
	c.config = config.config;
	c.report_dir = config.report_dir;
	c.sym_dir = config.sym_dir;
	c.mode = config.mode == CF_CRASH_MODE_WATCHER ? CC_MODE_WATCHER : CC_MODE_INPROCESS;
	c.upload_on_crash = config.upload_on_crash;
	c.minidump = config.minidump;
	c.install_id = config.install_id;
	c.hang_seconds = config.hang_seconds;
	c.send = config.upload_url && s_parse_url(config.upload_url) ? s_send : NULL;
	c.ask = config.ask_consent ? s_ask : NULL;
	c.on_crash = config.on_crash;
	c.on_hang = config.on_hang;
	c.udata = config.udata;
	s_active = cc_init(c, argc, argv);
	if (s_active && config.assert_reports && g_assert_fn != s_assert) {
		s_prev_assert = g_assert_fn;
		cf_set_assert_handler(s_assert);
	}
	return s_active;
}

void cf_crash_breadcrumb(const char* fmt, ...)
{
	if (!s_active) return;
	char buf[CC_BREADCRUMB_BYTES];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	cc_breadcrumb("%s", buf);
}

void cf_crash_report(const char* fmt, ...)
{
	if (!s_active) return;
	char buf[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	cc_report(buf);
}

void cf_crash_set(const char* key, const char* value)
{
	if (s_active) cc_set(key, value);
}

void cf_crash_hang_pause()
{
	if (s_active) cc_hang_pause();
}

void cf_crash_hang_resume()
{
	if (s_active) cc_hang_resume();
}

void cf_crash_attach_thread(const char* name)
{
	if (s_active) cc_attach_thread(name);
}

bool cf_crash_active_internal()
{
	return s_active;
}

void cf_crash_app_update_internal()
{
	if (s_active) cc_heartbeat();
}

void cf_crash_thread_attach_internal(const char* name)
{
	if (s_active) cc_attach_thread(name);
}

// The machine section's keys the reporter cannot know on its own: what the frame is drawn with.
void cf_crash_app_made_internal()
{
	if (!s_active || !app) return;
#ifdef CF_WEBGPU
	if (app->gfx_enabled && app->gfx_backend_type == CF_BACKEND_TYPE_WEBGPU) {
		cc_set("backend", cf_backend_type_to_string(app->gfx_backend_type));
		const char* gpu = cf_webgpu_adapter_name();
		if (gpu && *gpu) cc_set("gpu", gpu);
	}
#endif
#ifndef CF_EMSCRIPTEN
	if (app->gfx_enabled && app->gfx_backend_type != CF_BACKEND_TYPE_WEBGPU) {
		cc_set("backend", cf_backend_type_to_string(app->gfx_backend_type));
		SDL_GPUDevice* device = cf_sdlgpu_get_device();
		if (device) {
			SDL_PropertiesID props = SDL_GetGPUDeviceProperties(device);
			const char* gpu = SDL_GetStringProperty(props, SDL_PROP_GPU_DEVICE_NAME_STRING, NULL);
			const char* driver = SDL_GetStringProperty(props, SDL_PROP_GPU_DEVICE_DRIVER_VERSION_STRING, NULL);
			if (gpu) cc_set("gpu", gpu);
			if (driver) cc_set("gpu_driver", driver);
		}
	}
#endif
	if (app->window) {
		int w, h;
		cf_app_get_size(&w, &h);
		cc_set("window", String::fmt("%dx%d", w, h).c_str());
		CF_DisplayID display = cf_default_display();
		cc_set("display", String::fmt("%dx%d@%.0f", cf_display_width(display), cf_display_height(display), cf_display_refresh_rate(display)).c_str());
		cc_set("fullscreen", (SDL_GetWindowFlags(app->window) & SDL_WINDOW_FULLSCREEN) ? "true" : "false");
	}
}
