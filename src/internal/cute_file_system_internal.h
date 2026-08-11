/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_FILE_SYSTEM_INTERNAL_H
#define CF_FILE_SYSTEM_INTERNAL_H

#include <cute_defines.h>
#include <cute_array.h>
#include <cute_map.h>
#include <cute_string.h>

// One entry from a native (non-PhysFS) directory walk. See cf_fs_scan_native_directory.
//
// `modified_time` and `size` are only meaningful when `is_directory` is false. The POSIX walk
// types directories straight from `d_type` and never stats them, so a directory's mtime is
// whatever the platform happened to hand back -- possibly zero. Nothing reads it: the shader
// watcher only recurses into directories.
struct CF_DirEntry
{
	// Interned, so it is a stable unique pointer for the life of the process: comparing two names
	// is a pointer compare, and an entry owns no allocation. A walk runs per directory per scan,
	// so a heap string per entry here was most of what the walk cost.
	const char* name = NULL;
	uint64_t modified_time = 0;  // UNIX seconds, matching PHYSFS_Stat::modtime.
	uint64_t size = 0;
	bool is_directory = false;
	bool stat_ok = false;
};

// Reads one real (platform) directory in a single walk and fills `out` with its entries, keyed
// by `sintern`'d name. Returns false if the directory could not be walked, in which case `out`
// is left as-is and the caller must fall back to per-file stats.
//
// This is a metadata accelerator, not a replacement for PhysFS enumeration: it knows nothing
// about mounts, so it sees only whichever real directory it is pointed at. The metadata it
// reports is deliberately bug-compatible with `PHYSFS_stat` -- see the notes on the Windows
// time conversion and on symlink typing in the implementation.
bool cf_fs_scan_native_directory(const char* platform_directory, Cute::Map<CF_DirEntry>* out);

// A directory's entry set reduced to something cheap to compare from one scan to the next.
// Adding, removing or renaming an entry changes it; writing to a file does not -- which is exactly
// the distinction a cached directory *listing* needs.
struct CF_DirSignature
{
	uint64_t hash = 0;
	int count = -1;  // -1 means the directory could not be walked at all.
};

CF_INLINE bool cf_fs_dir_signature_equal(CF_DirSignature a, CF_DirSignature b)
{
	return a.hash == b.hash && a.count == b.count;
}

// Entries are keyed by `sintern`'d name, and an interned string is a stable unique pointer for the
// life of the process -- so the set of keys identifies the listing without hashing any characters.
// XOR and sum accumulate separately so the result does not depend on the order the walk happened to
// return entries in.
CF_INLINE CF_DirSignature cf_fs_dir_signature(Cute::Map<CF_DirEntry>* entries)
{
	uint64_t x = 0, sum = 0;
	int n = entries->count();
	const uint64_t* keys = entries->keys();
	for (int i = 0; i < n; ++i) {
		x ^= keys[i];
		sum += keys[i];
	}
	CF_DirSignature sig;
	sig.hash = x ^ (sum * 1099511628211ULL);
	sig.count = n;
	return sig;
}

// Monotonic counter bumped by every call that can change the PhysFS search path -- cf_fs_mount and
// cf_fs_dismount are the only two. Anything caching a view derived from the virtual filesystem
// compares against this instead of trying to notice a mount change after the fact.
uint64_t cf_fs_generation();

// The real directory each mounted archive would serve `virtual_directory` from, in search-path
// order, for every mount that could contribute entries to it. Derived from the mount table alone --
// no filesystem calls -- by subtracting each archive's mount point from the virtual path.
//
// These are *candidates*, not facts. PHYSFS_setRoot's prefix has no getter, so a mount using it
// resolves elsewhere than the arithmetic here suggests; such a candidate normally just fails to
// walk. Callers must prove what they take from a candidate rather than trusting it.
//
// `out_virtual_names` collects the entries in this directory that exist only because a mount point
// passes through it, and so have no real directory anywhere to walk. They come from the mount table
// alone, which cf_fs_generation already tracks.
void cf_fs_mount_candidates(const char* virtual_directory, Cute::Array<Cute::String>* out,
                            Cute::Array<Cute::String>* out_virtual_names);

#endif // CF_FILE_SYSTEM_INTERNAL_H
