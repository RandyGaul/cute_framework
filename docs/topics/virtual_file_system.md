[](../header.md ':include')

<br>

Cute Framework uses a Virtual File System (VFS) to access the disk. You can of course simply call `fopen` and `fclose` (or similar) functions on your own and entirely bypass the VFS, but, all of CF's functions that access the disk make use of the VFS. It works by _mounting_ folders under new names. You game can then operate purely on _mounted_ folders and file paths. This has some major benefits:

- More portable
- More secure
- More versatile

## Mounting

Mounting a folder gives it a new alias and optionally appends it to the search path. Call [`cf_fs_mount`](https://randygaul.github.io/cute_framework/#/file/cf_fs_mount).

```cpp
bool append_to_path = true;
cf_fs_mount("C:/Users/Randy/Documents/data", "/data", append_to_path);
```

The above snippet will give the folder `"C:/Users/Randy/Documents/data"`, which is specified in platform-dependent notation, a new alias `"/data"`.

?> **Platform-dependent notation** means a full folder path, and not a relative path, according to the platform your game is currently running on. On Windows machines folders start with the drive like, such as `C:/`, while on Linux machines a path will start simply with a slash `/`.

 The alias folder `/data` is called a virtual path, and is specified in platform-independent notation. This means:

- No drive letters
- No relative paths `.` or `..`
- No Windows style slashes `\\`
- No colons `:`

?> **Normalizing** a path is the process of removing relative directories, removing redundant or Windows style slashes, and attempting to convert the string to a more platform-independant form. You can still normalize platform-dependent paths too though. Call [`spnorm`](https://randygaul.github.io/cute_framework/#/path/spnorm) to normalize a string path.

By mounting we achieve great portability by using platform-independent paths within our game. The paths are also more secure by removing relative paths (which reduce the chances of anyone accessing unanticipated directories), and most important of all grants versatility.

## Mounting Archives

Since a folder can be mounted, wouldn't it be cool if you could also mount an archive, such as a .zip or .7z file? Turns out, you can! The first parameter of [`cf_fs_mount`](https://randygaul.github.io/cute_framework/#/file/cf_fs_mount) can be a number of different archive files, and not just a plain folder.

- .ZIP (pkZip/WinZip/Info-ZIP compatible)
- .7Z  (7zip archives)
- .ISO (ISO9660 files, CD-ROM images)
- .GRP (Build Engine groupfile archives)
- .PAK (Quake I/II archive format)
- .HOG (Descent I/II HOG file archives)
- .MVL (Descent II movielib archives)
- .WAD (DOOM engine archives)
- .VDF (Gothic I/II engine archives)
- .SLB (Independence War archives)

This grants a lot of flexibility. We can move entire directories around on disk and then rename the mount point without changing the rest of the game code. Whenever an archive is mounted the file system treats it like a normal directory. No extra work is needed. This lets us do really cool things, like deploy patches by downloading new archive files and appending them to an earlier place in the search path. This also works to add mod support to your game, and provides a simple way of storing multiple versions of a single file without overwriting each other on the actual disk.

By default CF mounts the base directory (mentioned in the next section). when you call [`cf_make_app`](https://randygaul.github.io/cute_framework/#/app/cf_make_app). This can be disabled by passing the [`CF_APP_OPTIONS_FILE_SYSTEM_DONT_DEFAULT_MOUNT`](https://randygaul.github.io/cute_framework/#/app/app_options) flag to [`cf_make_app`](https://randygaul.github.io/cute_framework/#/app/cf_make_app).

### Search Path

The search path of the VFS is defined as a list of directories. When attempting to locate a file (or directory) you can mount multiple real folders/archives onto a single alias. This alias then represents the list of mounts in the order they were mounted, so long as you append to the path when mounting (true in the third parameter of [`cf_fs_mount`](https://randygaul.github.io/cute_framework/#/file/cf_fs_mount)). If any two files have the same path after mounting, they are added to a list. When searching for any file, only the most recently added file will be seen by the VFS. This allows for an easy way to support downloadable patches or mods. You may mount additional archives onto the same alias and "hide" any older previously mounted files with overlapping paths.

## Base and User Directories

The base directory is the actual path to the directory the executable for your game was run from. This is not a virtual path, but the actual OS-path in platform-dependent notation. This might not be the working directory, but probably is. You should probably mount the base directory with [`cf_fs_mount`](https://randygaul.github.io/cute_framework/#/file/cf_fs_mount). A very common strategy is to mount the base folder as `"/"`. You can fetch the base directory be calling [`cf_fs_get_base_directory`](https://randygaul.github.io/cute_framework/#/file/cf_fs_get_base_directory).

The user directory is a safe place for your game to write files. It's unique per user and per application. On Windows it will probably live in AppData/Roaming, on Linux in user/.local, and so on. You should assume this directory is the only safe place to write files. You can fetch the user directory with [`cf_fs_get_user_directory`](https://randygaul.github.io/cute_framework/#/file/cf_fs_get_user_directory).

## The Write Directory

Your application gets a single write directory. You set it with [`cf_fs_set_write_directory`](https://randygaul.github.io/cute_framework/#/file/cf_fs_set_write_directory). This greatly aids security and keeps writing operations locked within a single directory for simplicity. It's highly recommended to setup your write directory as the user directory from [`cf_fs_get_user_directory`](https://randygaul.github.io/cute_framework/#/file/cf_fs_get_user_directory). This directory is guaranteed to be a write-enabled and safe place to store game-specific files for your player.

> Setting the user directory as the write directory as recommended.

```cpp
CF_Result result = cf_fs_set_write_directory(cf_fs_get_user_directory("cool-game-studio", "awesome-game"));
CF_ASSERT(!cf_is_error(result));
```

## Editors or Asset Hotloading

Sometimes it is necessary to convert a virtual path to an actual platform-dependant path. For example, if you're building an editor and wish to modify assets on disk, you'll of course need access to the real path. Similarly, if you want to watch files on disk and see when they're modified to perform asset hotloading, real paths are again going to be necessary.

You can convert a virtual path to a real path with [`cf_fs_get_actual_path`](https://randygaul.github.io/cute_framework/#/file/cf_fs_get_actual_path).

## Just Open a File

If you just want to read an entire file's contents to memory then try using [`cf_fs_read_entire_file_to_memory`](https://randygaul.github.io/cute_framework/#/file/cf_fs_read_entire_file_to_memory); it does just that. Similarly, if you want to read an entire file's contents to memory as a string try using [`cf_fs_read_entire_file_to_memory_and_nul_terminate`](https://randygaul.github.io/cute_framework/#/file/cf_fs_read_entire_file_to_memory_and_nul_terminate).

## Enumerating Directories

The function [`cf_fs_enumerate_directory`](https://randygaul.github.io/cute_framework/#/file/cf_fs_enumerate_directory) returns an array of strings of all filse within a directory. This is great for looping over the names of files within a particular folder.

```cpp
const char** dirs_ptr = cf_fs_enumerate_directory("/data");
for (const char** dirs = dirs_ptr; *dirs; ++dirs) {
	const char* dir = *dirs;
	printf("Found %s\n", dir);
}
cf_fs_free_enumerated_directory(dirs_ptr);
```

If you're using C++ you can instead use the `Directory` wrapper.

```cpp
Directory dir = Directory::open("/data");
for (const char* file = dir.next(); file; file = dir.next()) {
	printf("Found %s\n", file);
}
```
