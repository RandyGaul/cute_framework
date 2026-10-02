/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_CRASH_INTERNAL_H
#define CF_CRASH_INTERNAL_H

// The reporter's hooks into the framework, all no-ops until cf_crash_init succeeded:
// the frame heartbeat (cf_app_update), threads made by cf_thread_create, and the machine
// section once the app exists.
void cf_crash_app_update_internal();
void cf_crash_thread_attach_internal(const char* name);
void cf_crash_app_made_internal();
bool cf_crash_active_internal();

#endif // CF_CRASH_INTERNAL_H
