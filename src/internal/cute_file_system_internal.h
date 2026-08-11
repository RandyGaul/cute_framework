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

// Fast directory polling, for code that must notice a file changing without paying for it every
// frame. cf_shader_watch is why this exists.
//
// The expensive operation is a per-file metadata query. PHYSFS_stat is one, and so is every entry
// of PHYSFS_enumerateFiles -- it stats each entry internally to filter out symlinks it will not
// report. A bulk directory walk (FindFirstFileW, readdir) hands back the same metadata for a whole
// directory for roughly the cost of one query, so polling is built out of walks, and PhysFS is
// asked as rarely as it can be.
//
// PhysFS still decides *what exists*. Several mounts can serve one directory at once and only
// PhysFS knows the merged result, at any depth and after any later cf_fs_mount; a walk sees a
// single real directory. So a walk is a metadata lookup table, never the file list. Take the list
// from a walk instead and a file shadowed by a lower mount silently stops being watched.
//
// What is left is knowing when PhysFS's answer went stale, which is decided without asking it:
//
//   - the merged list changes only if the mount table changes, or if some contributing real
//     directory gains, loses or renames an entry;
//   - cf_fs_generation() tracks the first; one walk per contributing mount tracks the second,
//     reduced to a CF_DirSignature a scan compares against the previous one;
//   - writing to a file does neither, so editing a watched file never re-asks PhysFS.
//
// Anything derived from a mount's real path is a candidate to be proven rather than a fact. See
// cf_fs_mount_candidates.

// One entry of a bulk directory walk.
struct CF_DirEntry
{
	const char* name = NULL;  // Interned, so comparing two names is a pointer compare.
	uint64_t modified_time = 0;
	uint64_t size = 0;
	bool is_directory = false;
	bool stat_ok = false;
};

// Walks one real (platform) directory, keying `out` by interned name. False if it could not be
// walked at all. The metadata is deliberately bug-compatible with PHYSFS_stat, so a caller may mix
// entries from here with entries it stat'd itself.
bool cf_fs_scan_native_directory(const char* platform_directory, Cute::Map<CF_DirEntry>* out);

struct CF_DirSignature
{
	uint64_t hash = 0;
	int count = -1;  // -1: the directory could not be walked.
};

CF_INLINE bool cf_fs_dir_signature_equal(CF_DirSignature a, CF_DirSignature b)
{
	return a.hash == b.hash && a.count == b.count;
}

// Interned keys are stable unique pointers, so the set of them identifies the listing without
// hashing any characters. XOR and sum accumulate separately so the result does not depend on the
// order the walk returned entries in.
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

// Bumped by cf_fs_mount and cf_fs_dismount, the only calls that change the search path.
uint64_t cf_fs_generation();

// The real directory each mount would serve `virtual_directory` from, in search-path order, for
// every mount that could contribute to it. Candidates, not facts: PHYSFS_setRoot's prefix has no
// getter, so a mount using it resolves somewhere other than this arithmetic suggests.
//
// `out_virtual_names` receives entries that exist only because a mount point passes through the
// directory, which therefore have no real directory anywhere to walk.
void cf_fs_mount_candidates(const char* virtual_directory, Cute::Array<Cute::String>* out,
                            Cute::Array<Cute::String>* out_virtual_names);

#endif // CF_FILE_SYSTEM_INTERNAL_H
