/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_array.h>
#include <cute_a_star.h>
#include <cute_math.h>
#include <cute_priority_queue.h>
#include <float.h>

#include <internal/cute_alloc_internal.h>

using namespace Cute;

struct CF_iv2
{
	int x, y;
};

struct CF_AStarNodeInternal
{
	CF_iv2 p;
	float h; // Cost from the heuristic function to the end.
	float g; // Accumulated cost of the path (from `cell_to_cost`).
	float f; // h + g
	bool visited;
	CF_AStarNodeInternal* parent;
};

struct CF_AStarGridInternal
{
	int w = 0;
	int h = 0;
	float* cell_costs = NULL;
	Array<CF_AStarNodeInternal> nodes;
	PriorityQueue<CF_AStarNodeInternal*> open_list;

	void reset()
	{
		for (int i = 0; i < w; ++i) {
			for (int j = 0; j < h; ++j) {
				CF_AStarNodeInternal* n = nodes + (j * w + i);
				n->p = { i, j };
				n->visited = false;
				n->f = FLT_MAX;
				n->g = 0;
				n->h = 0;
				n->parent = NULL;
			}
		}
		open_list.clear();
	}
};

static float cf_internal_s_heuristic(CF_iv2 a, CF_iv2 b, float allow_diagonals)
{
	float dx = (float)cf_abs_int(a.x - b.x);
	float dy = (float)cf_abs_int(a.y - b.y);
	float diagonal = cf_min(dx, dy) * allow_diagonals;
	float manhattan = dx + dy;
	float chebyshev = 1.4142135f * diagonal + (manhattan - 2.0f * diagonal);
	return chebyshev;
}


CF_AStarGrid cf_make_a_star_grid(int w, int h, float* cell_costs)
{
	CF_AStarGridInternal* grid = CF_NEW(CF_AStarGridInternal);
	grid->w = w;
	grid->h = h;
	grid->cell_costs = cell_costs;
	grid->nodes.ensure_count(w * h);
	CF_AStarGrid result;
	result.id = (uint64_t)grid;
	return result;
}

float cf_a_star_grid_get_cost(CF_AStarGrid grid_handle, int x, int y)
{
	CF_AStarGridInternal* grid = (CF_AStarGridInternal*)grid_handle.id;
	float cost = grid->cell_costs[y * grid->w + x];
	return cost;
}

void cf_a_star_grid_set_cost(CF_AStarGrid grid_handle, int x, int y, float cost)
{
	CF_AStarGridInternal* grid = (CF_AStarGridInternal*)grid_handle.id;
	grid->cell_costs[y * grid->w + x] = cost;
}

void cf_destroy_a_star_grid(CF_AStarGrid grid_handle)
{
	CF_AStarGridInternal* grid = (CF_AStarGridInternal*)grid_handle.id;
	grid->~CF_AStarGridInternal();
	CF_FREE(grid);
}

bool cf_a_star(CF_AStarGrid grid_handle, int start_x, int start_y, int end_x, int end_y, bool allow_diagonal_movement, CF_AStarOutput* out)
{
	CF_AStarGridInternal* grid = (CF_AStarGridInternal*)grid_handle.id;
	grid->reset();
	PriorityQueue<CF_AStarNodeInternal*>& open_list = grid->open_list;
	CF_iv2 s = { start_x, start_y };
	CF_iv2 e = { end_x, end_y };
	float allow_diagonals = allow_diagonal_movement ? 1.0f : 0;
	CF_AStarNodeInternal* nodes = grid->nodes.data();
	const float* cell_costs = grid->cell_costs;
	dyna int* out_x = NULL;
	dyna int* out_y = NULL;

	if (s.x == e.x && s.y == e.y) {
		if (out) {
			apush(out_x, s.x);
			apush(out_y, s.y);
			out->count = 1;
			out->x = out_x;
			out->y = out_y;
		}
		return true;
	}

	int w = grid->w;
	int h = grid->h;
	int index = s.y * w + s.x;
	CF_AStarNodeInternal* initial = nodes + index;
	initial->g = 0;
	initial->h = cf_internal_s_heuristic(s, e, allow_diagonals);
	initial->f = initial->h;
	initial->visited = true;
	open_list.push_min(initial, initial->f);

	while (open_list.count()) {
		CF_AStarNodeInternal* q;
		open_list.pop_min(&q);
		CF_iv2 qp = q->p;

		if (qp.x == e.x && qp.y == e.y) {
			if (out) {
				while (q->parent) {
					apush(out_x, q->p.x);
					apush(out_y, q->p.y);
					q = q->parent;
				}

				arev(out_x);
				arev(out_y);
				out->count = acount(out_x);
				CF_ASSERT(acount(out_x) == acount(out_y));
			}

			return true;
		}

		int next_count = 0;
		CF_AStarNodeInternal* next[8];

		#define CF_A_STAR_ADD_SUCCESSOR(x, y) \
			if ((x) >= 0 && (x) < w && (y) >= 0 && (y) < h) { \
				next[next_count++] = nodes + ((y) * w + (x)); \
			}

		CF_A_STAR_ADD_SUCCESSOR(qp.x + 1, qp.y);
		CF_A_STAR_ADD_SUCCESSOR(qp.x, qp.y + 1);
		CF_A_STAR_ADD_SUCCESSOR(qp.x - 1, qp.y);
		CF_A_STAR_ADD_SUCCESSOR(qp.x, qp.y - 1);
		if (allow_diagonal_movement) {
			CF_A_STAR_ADD_SUCCESSOR(qp.x + 1, qp.y + 1);
			CF_A_STAR_ADD_SUCCESSOR(qp.x - 1, qp.y + 1);
			CF_A_STAR_ADD_SUCCESSOR(qp.x + 1, qp.y - 1);
			CF_A_STAR_ADD_SUCCESSOR(qp.x - 1, qp.y - 1);
		}

		for (int i = 0; i < next_count; ++i) {
			CF_AStarNodeInternal* n = next[i];
			index = n->p.y * w + n->p.x;
			float cell_cost = cell_costs ? cell_costs[index] : 1.0f;
			bool non_traversable = cell_cost <= 0;
			float g = n->g + cell_cost;
			if (n->visited) continue;
			if (non_traversable) continue;
			if (n->g <= g) continue;

			float h = cf_internal_s_heuristic(n->p, e, allow_diagonals);
			n->g = g;
			n->h = h;
			n->f = g + h;
			n->parent = q;
			n->visited = true;
			open_list.push_min(n, n->f);
		}
	}

	return false;
}

CF_API void CF_CALL cf_free_a_star_output(CF_AStarOutput* out)
{
	afree(out->x);
	afree(out->y);
}
