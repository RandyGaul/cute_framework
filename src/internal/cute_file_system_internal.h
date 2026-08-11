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

// Walks one real (platform) directory in a single pass, filling `out` with its entries keyed by
// interned name; returns false, leaving `out` alone, if the directory could not be walked at all.
// The metadata matches what PHYSFS_stat would report, quirks included, so a caller can mix these
// entries with ones it stat'd itself.
bool cf_fs_scan_native_directory(const char* platform_directory, Cute::Map<CF_DirEntry>* out);

struct CF_DirSignature
{
	uint64_t name_xor = 0;
	uint64_t name_sum = 0;
	int count = -1;  // -1: the directory could not be walked.
};

CF_INLINE bool cf_fs_dir_signature_equal(CF_DirSignature a, CF_DirSignature b)
{
	return a.name_xor == b.name_xor && a.name_sum == b.name_sum && a.count == b.count;
}

// Reduces a walk to a value two scans can compare. Interned names are already stable unique
// pointers, so there is nothing to hash: accumulating them is enough to identify the set. XOR and
// sum are both commutative, so the result does not depend on the order the walk returned entries
// in, and keeping them apart rather than folding them together is what makes the pair strong --
// for any two entries, an XOR and a sum together pin down the pair exactly.
CF_INLINE CF_DirSignature cf_fs_dir_signature(Cute::Map<CF_DirEntry>* entries)
{
	CF_DirSignature sig;
	sig.count = entries->count();
	const uint64_t* keys = entries->keys();
	for (int i = 0; i < sig.count; ++i) {
		sig.name_xor ^= keys[i];
		sig.name_sum += keys[i];
	}
	return sig;
}

// Returns a counter bumped by cf_fs_mount and cf_fs_dismount, the only calls that change the
// search path. Anything caching a view derived from the virtual filesystem compares against it
// instead of trying to notice a mount change after the fact.
uint64_t cf_fs_generation();

// Fills `out` with the real directory each mount would serve `virtual_directory` from, in
// search-path order, for every mount that could contribute to it. These are candidates, not facts:
// PHYSFS_setRoot's prefix has no getter, so a mount using it resolves somewhere other than this
// arithmetic suggests, and a caller must prove whatever it takes from one.
//
// Fills `out_virtual_names` with the entries that exist only because a mount point passes through
// the directory, and so have no real directory anywhere to walk.
void cf_fs_mount_candidates(const char* virtual_directory, Cute::Array<Cute::String>* out,
                            Cute::Array<Cute::String>* out_virtual_names);

#endif // CF_FILE_SYSTEM_INTERNAL_H
