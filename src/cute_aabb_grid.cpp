/*
	Cute Framework
	Copyright (C) 2023 Randy Gaul https://randygaul.github.io/

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

#include <cute_aabb_grid.h>

using namespace Cute;

void aabb_grid_insert(AabbGrid *grid, Cute::Aabb aabb, void *udata)
{
	v2 half_extents = grid->extents * 0.5f;

	int max_tiles_x = (int)grid->extents.x / grid->tile_size;
	int max_tiles_y = (int)grid->extents.y / grid->tile_size;

	Aabb relative_aabb = aabb;
	relative_aabb.min += half_extents - grid->center;
	relative_aabb.max += half_extents - grid->center;

	int min_x = (int)relative_aabb.min.x / grid->tile_size;
	int min_y = (int)relative_aabb.min.y / grid->tile_size;

	int max_x = (int)relative_aabb.max.x / grid->tile_size;
	int max_y = (int)relative_aabb.max.y / grid->tile_size;

	if (min_x < 0 || min_y < 0 || max_x >= max_tiles_x || max_y >= max_tiles_y)
	{
		return;
	}

	int id = ++grid->count;

	for (int x = min_x; x <= max_x; x++)
	{
		for (int y = min_y; y <= max_y; y++)
		{
			int index = y * max_tiles_x + x;
			grid->map[index].add({id, aabb, udata});
		}
	}
}

void aabb_grid_clear(AabbGrid *grid)
{
	for (int i = 0; i < grid->map.count(); i++)
	{
		grid->map[i].clear();
	}

	grid->count = 0;
}

void aabb_grid_query(
		AabbGrid *grid,
		Cute::Aabb a_aabb,
		bool (*on_hit)(int id, Cute::Aabb aabb, void *leaf_udata, void *fn_udata),
		void *udata
)
{
	static Array<int> visited;
	visited.clear();

	v2 half_extents = grid->extents * 0.5f;

	int max_tiles_x = (int)grid->extents.x / grid->tile_size;
	int max_tiles_y = (int)grid->extents.y / grid->tile_size;

	a_aabb.min += half_extents - grid->center;
	a_aabb.max += half_extents - grid->center;

	int min_x = (int)a_aabb.min.x / grid->tile_size;
	int min_y = (int)a_aabb.min.y / grid->tile_size;

	int max_x = (int)a_aabb.max.x / grid->tile_size;
	int max_y = (int)a_aabb.max.y / grid->tile_size;

	if (min_x < 0 || min_y < 0 || max_x >= max_tiles_x || max_y >= max_tiles_y)
	{
		return;
	}

	for (int x = min_x; x <= max_x; x++)
	{
		for (int y = min_y; y <= max_y; y++)
		{
			int index = y * max_tiles_x + x;

			for (auto b : grid->map[index])
			{
				bool already_visited = false;
				for (auto v : visited)
				{
					if (v == b.id)
					{
						already_visited = true;
						break;
					}
				}

				if (already_visited)
				{
					continue;
				}

				auto b_aabb = b.aabb;
				b_aabb.min += half_extents - grid->center;
				b_aabb.max += half_extents - grid->center;

				if (aabb_to_aabb(a_aabb, b_aabb))
				{
					visited.add(b.id);
					if (!on_hit(b.id, b.aabb, b.udata, udata))
					{
						return;
					}
				}
			}
		}
	}
}

AabbGrid make_aabb_grid(Aabb bounds, int tile_size)
{
	AabbGrid grid;

	grid.extents = extents(bounds);
	grid.center = center(bounds);
	grid.tile_size = tile_size;

	int max_tiles_x = (int)grid.extents.x / tile_size;
	int max_tiles_y = (int)grid.extents.y / tile_size;

	grid.map.ensure_count(max_tiles_x * max_tiles_y);

	return grid;
}

