# Crash Reporting

When a shipped game crashes, hangs, or quits without saying goodbye, CF can write a report of what happened and send it to you. A report holds the crashing thread's stack, every loaded module's build id, the fault, the machine, the game's own breadcrumbs and state, and on Windows a minidump that shows local variables in a debugger. It is a JSON file on disk first, uploaded later, so nothing fragile runs inside the crash.

The reporter is [`cute_crash.h`](https://github.com/RandyGaul/cute_framework/blob/master/libraries/cute/cute_crash.h), a self-contained single-file library with no dependencies, and [`cute_sym.h`](https://github.com/RandyGaul/cute_framework/blob/master/libraries/cute/cute_sym.h), its symbol tool. CF wraps them as the `cf_crash_*` API. The design is bullno1's, from [crash-where](https://github.com/bullno1/crash-where).

## Turning it on

Two lines before the app exists:

```cpp
int main(int argc, char* argv[])
{
	CF_CrashConfig cc = cf_crash_defaults();
	cc.version = "1.0.3";
	cc.upload_url = "https://crash.example.com/v1/crash";
	cf_crash_init(cc, argc, argv);

	// ... cf_make_app and the game ...
}
```

That is a working reporter. Everything else is optional:

- [`cf_crash_breadcrumb`](../crash/function/cf_crash_breadcrumb.md) records a line of what the game is doing into a ring the report carries: a scene loaded, a player joined, a turn advanced. Cheap enough to call wherever you log.
- [`cf_crash_set`](../crash/function/cf_crash_set.md) sets a key to a value: what is true right now, such as the map or the player count.
- `hang_seconds` in [`CF_CrashConfig`](../crash/struct/cf_crashconfig.md) turns on a watchdog thread that samples the main thread when frames stop coming and writes a hang report. CF sends the heartbeat from `cf_app_update`; wrap long loads with [`cf_crash_hang_pause`](../crash/function/cf_crash_hang_pause.md).
- `mode` chooses between catching the crash in-process (the default) and a watcher process that reports from outside. The watcher exits with the game and never respawns.
- [`cf_crash_report`](../crash/function/cf_crash_report.md) writes a report that is not a crash: a message and the calling thread's stack, for a condition the game survived but wants to know about. A failed `CF_ASSERT` does this on its own: its expression, file and line go into the report, and if the assert handler returns instead of stopping, the assert becomes a report of its own.

Under a debugger, or with `CC_DISABLE=1` in the environment, the reporter installs nothing and your crashes break into the debugger as usual.

## Names and line numbers

A report is written with raw addresses, relative to each module, plus the module's build id. Turning those into function names, files and lines needs a symbol table made from the build's debug info. CMake makes one for you:

```cmake
cf_symbols(mygame EMBED)   # The table is patched into the executable itself. Nothing ships beside it.
cf_symbols(mygame FILE)    # The table is written beside the executable as mygame.exe.sym.
```

`cf_symbols` turns on debug info for the target and for CF itself (a PDB on MSVC, `-g` elsewhere; the shipped code is unchanged) and runs `cute-sym` after every link. With a table present, reports are symbolicated on the player's machine before upload and arrive readable. A table holds names and lines only, not types or locals.

If you would rather ship no symbols at all, skip `cf_symbols` and keep the tables and debug files from each build. Reports then arrive raw, and `cute-sym resolve report.json --symbols <dir>` decodes them on your machine with the same code. The minidump needs the full PDB either way; open it in Visual Studio with the symbol path pointing at the PDBs you kept.

The tool reads PDBs, Mach-O debug maps and dSYMs, and ELF DWARF itself, with no dependency on the toolchain that made the binary.

## The endpoint

Reports are POSTed as `multipart/form-data` with two parts: `report`, the JSON document, and `crash.dmp`, the minidump when there is one. Any reply at all is the acknowledgment; the client deletes the report when the server answered and keeps it for the next launch when the connection failed. The simplest possible server is a script that writes the parts to disk. A fuller one symbolicates against tables by build id, stores the documents, and groups them by the `signature` field the report carries.

The player is asked once before the first upload, with Send, Always send and Don't send, and the answer is remembered.

[`tools/crashbox`](https://github.com/RandyGaul/cute_framework/tree/master/tools/crashbox) is such a server, the minimal one: a few hundred lines of Go with no dependencies, files on disk, a page of groups sorted by count, and size, rate and disk limits on by default. Its `tools/setup.sh` and `tools/deploy.sh` put it on a Linux box as a systemd service.

## Trying it

The `crashme` sample crashes on request, by key or with `--cc-test null|overflow|abort|throw|thread|hang`, and prints the report it left at the next launch. `cute-sym print report.json` renders any report as text.
