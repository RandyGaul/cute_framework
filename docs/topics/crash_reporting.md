# Crash Reporting

When your game crashes on a player's machine, CF can write down what happened and send it to you: the stack trace with file and line numbers, what the game was doing, and the player's OS, GPU and driver.

## Why bother

Ship a game on Steam and thousands of people run it on hardware you have never seen. Some of them will crash. Almost none will tell you, and the ones who do will say "it crashed when I opened the map."

A crash reporter turns that into something you can fix. Every crash arrives with the exact line it happened on, and identical crashes are grouped and counted, so you see "this crash, 312 times, 40 players, only on version 1.0.2." You fix the biggest one first, ship a patch, and watch it stop coming in. The first week after launch is when this matters most.

## Step 1: turn it on

Add three lines at the top of `main`, before `cf_make_app`:

```cpp
int main(int argc, char* argv[])
{
	CF_CrashConfig cc = cf_crash_defaults();
	cc.version = "1.0.2";                                   // Your game's version.
	cc.upload_url = "https://crash.example.com/v1/crash";   // Your server (step 4).
	cf_crash_init(cc, argc, argv);

	cf_make_app(...);
	// The rest of your game, unchanged.
}
```

From now on a crash writes a report to disk. The next time the player starts the game, they are asked once whether to send crash reports, and the report is uploaded.

While you run the game under a debugger, the reporter stays off so crashes still stop in the debugger.

## Step 2: get file and line numbers

In your `CMakeLists.txt`, after `add_executable`:

```cmake
cf_symbols(mygame EMBED)
```

This stores a small table of function names and line numbers inside your executable when it is built. Without it, reports still arrive, but show addresses like `mygame.exe+0x3b9d5` instead of `player.cpp:118`.

Also keep a copy of the `.pdb` file (Windows) or `.dSYM` folder (macOS) from every build you ship. You need it later to open a crash dump in a debugger.

## Step 3: try it

Run your game with `--cc-test null` on the command line. It crashes on purpose. Reports land in a folder per game:

| OS | Folder |
|---|---|
| Windows | `%LOCALAPPDATA%\mygame\crash\` |
| macOS | `~/Library/Application Support/mygame/crash/` |
| Linux | `~/.local/share/mygame/crash/` |

Each report is a `.json` file, plus a `.dmp` on Windows. To read one as plain text:

```
cute-sym print crash-20261002-101500-1234.json
```

`cute-sym` is built with CF. The `crashme` sample does the same thing with a menu of different crashes to try.

## Step 4: a server to collect reports

Reports need somewhere to go. CF comes with a small one, `tools/crashbox`, that receives reports and shows them on a web page, biggest crash at the top.

You need a Linux server with a domain name pointing at it (any small VPS is enough). On that server:

1. Install [Go](https://go.dev/doc/install) and [Caddy](https://caddyserver.com/docs/install). Caddy gives you HTTPS for free, which the game requires.
2. Copy `tools/crashbox` from CF to the server and build it:
   ```
   cd crashbox
   go build -o crashbox .
   ```
3. Create `crashbox.conf` next to it:
   ```
   addr = 127.0.0.1:8445
   tls =
   data = ./data
   token = pick-a-long-random-string
   user = admin
   password = pick-a-password
   max_age_days = 90
   ```
4. Create `/etc/caddy/Caddyfile` with your domain:
   ```
   crash.example.com {
   	reverse_proxy 127.0.0.1:8445
   }
   ```
   Then run `sudo systemctl reload caddy`.
5. Start crashbox: `./crashbox -config crashbox.conf`. To keep it running after you log out, see the crashbox README.
6. In your game, set the URL and the token from the config:
   ```cpp
   cc.upload_url = "https://crash.example.com/v1/crash";
   cc.upload_token = "pick-a-long-random-string";
   ```

Open `https://crash.example.com/` in a browser and log in with the user and password from the config.

Any other server works too, as long as it accepts a `multipart/form-data` POST with a `report` part (the JSON) and an optional `crash.dmp` part. Any reply counts as received.

## Step 5: fix crashes

On the crashbox page:

- Each row is one crash location, with how many times it happened, to how many players, and on which versions.
- Click a row to see the stack trace, and click a report for everything it holds.
- On Windows, download the `.dmp`, open it in Visual Studio, and point Visual Studio at the `.pdb` you kept in step 2. You see the local variables at the moment of the crash.
- When the fix has shipped, press **fixed: delete all reports**. If the crash comes back, it shows up as a new row.

## Optional extras

```cpp
cf_crash_breadcrumb("loaded level %s", name);   // A log line carried in the report.
cf_crash_set("level", name);                     // A value that is true right now.
cc.hang_seconds = 20;                            // Report a game frozen for 20 seconds.
cf_crash_report("inventory out of sync");        // Send a report without crashing.
```

A failed `CF_ASSERT` puts the assert's text into the report automatically.

## Under the hood

The reporter is two single-file libraries: [`cute_crash.h`](https://github.com/RandyGaul/cute_framework/blob/master/libraries/cute/cute_crash.h) catches crashes, and [`cute_sym.h`](https://github.com/RandyGaul/cute_framework/blob/master/libraries/cute/cute_sym.h) builds the symbol tables. Both work without the rest of CF. They were originally written by bullno1 as [crash-where](https://github.com/bullno1/crash-where).
