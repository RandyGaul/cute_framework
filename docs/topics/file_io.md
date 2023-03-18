[](../header.md ':include')

<br>

CF uses a virtual file system (VFS). Here's the [page on VFS](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system) to learn in-depth details. In contrast, the page here gets you up and running very quickly with the VFS so you can practice opening sprites or font files in the next section: [Drawing](https://randygaul.github.io/cute_framework/#/topics/drawing).

## Mounting

The VFS adds a layer of indirection from actual file or asset paths, to virtual paths. This grants boosted security, portability, and most importantly, versatility. Let us imagine our game's folder structure like so:

```
/super_cool_game
    ├─ build/
    |  └─ game.exe
    └─ content
    |  ├─ sprites/
    |  |    ├─ player.ase
    |  |    └─ bomb.ase
    │  └─ music/
    |       ├─ song1.ogg
    |       ├─ song2.ogg
    |       └─ song3.ogg
    └─ src/
        ├─ main.cpp
        └─ file.cpp
```

All of the games assets are in the `content` folder, including sprites, songs, etc. Source files go in the `src` folder, while all build related things go into the `build` folder. This is a good way to organize your project structure. The simplest way to get the VFS setup is to _mount_ the `content` folder. This means taking an absolute path to the `content` folder and giving it an aliased path. The game can from then on only refer to the aliased path, thus achieving all the benefits of the VFS.

To mount the content folder you may implement a function like this:

```cpp
void mount_content_folder()
{
	char* path = spnorm(fs_get_base_dir());
	int n = 1;
#ifdef _MSC_VER
	// Visual studio places .exe into one-level deeper /Debug or /Release folders.
	n = 2;
#endif
	path = sppopn(path, n);
	scat(path, "/content");
	fs_mount(path, "/");
	sfree(path);
}
```

Simply call this function from `main` before your main loop. [`cf_fs_get_base_directory`](https://randygaul.github.io/cute_framework/#/file/cf_fs_get_base_directory) returns the full path to the folder the game was run from. If you are already familiar with the _working directory_ this is not quite the same -- it's merely where the game's executable resides.

This renames the content folder to `"/"`, an empty path. To load up a music file, the virtual path the game will use now becomes `"/music/song1.ogg"`, which could be passed to [`cf_audio_load_ogg`](https://randygaul.github.io/cute_framework/#/audio/cf_audio_load_ogg). To load up a sprite call [`cf_make_sprite`](https://randygaul.github.io/cute_framework/#/sprite/cf_make_sprite) with a path like `"/sprites/player.ase"` or `"/sprites/bomb.ase"`.

## Benefits

The VFS provides extra portability. You may not have `:` in any virtual path name, as they are not allowed at all on Windows. You may not have drive letters like `C:\` on Windows. No Windows style slashes `\\`.

Extra security is achieved by disallowing relative paths such as `.` or `..`. You must also explicitly mount folders before accessing them, making it difficult to access unantipicated areas.

Boosted versatility comes with the VFS -- you may at any time change the folder structure by moving the `content` folder somwhere else. All that would need to change is the `mount_content_folder` from the previous section, and the rest of the game can continue functioning without _any changes at all_.

## Mounting Archives

You may also mount archive files, and not just folders. The VFS will auto-magically open the archive and treat it like any other normal folder. This works for all popular archive formats. For a full list, see the [detailed VFS page](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).

## Write Directory

The application gets a single directory to write files to (along with its subdirectories). This grants extra security. You must set up a write directory before opening any files with the VFS for writing (e.g. [`cf_fs_open_file_for_write`](https://randygaul.github.io/cute_framework/#/file/cf_fs_open_file_for_write)). To setup your write directory, call [`cf_fs_set_write_directory`](https://randygaul.github.io/cute_framework/#/file/cf_fs_set_write_directory).

It's highly recommended to call [`cf_fs_set_write_directory`](https://randygaul.github.io/cute_framework/#/file/cf_fs_set_write_directory) by passing in [`cf_fs_get_user_directory`](https://randygaul.github.io/cute_framework/#/file/cf_fs_get_user_directory), like so:

```cpp
const char* write_directory = cf_fs_get_user_directory("my_game_company_name", "my_game_name");
cf_fs_set_write_directory(write_directory);
```
