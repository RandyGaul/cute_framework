/*
	Cute Framework
	Copyright (C) 2019 Randy Gaul https://randygaul.net

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

#include <cute_a_star.h>
#include <cute_math.h>
#include <cute_priority_queue.h>

#include <float.h>

namespace cute
{

struct iv2
{
	int x, y;
};

struct node_t
{
	iv2 p;
	float h; // Cost from the heuristic function to the end.
	float g; // Accumulated cost of the path (from `cell_to_cost`).
	float f; // h + g
	bool visited;
	node_t* parent;
};

struct a_star_grid_t
{
	int w = 0;
	int h = 0;
	const int* cells = NULL;
	array<node_t> nodes;
	priority_queue<node_t*> open_list;

	void reset()
	{
		for (int i = 0; i < w; ++i) {
			for (int j = 0; j < h; ++j) {
				node_t* n = nodes + (j * w + i);
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

static float s_heuristic(iv2 a, iv2 b, float allow_diagonals)
{
	float dx = (float)abs(a.x - b.x);
	float dy = (float)abs(a.y - b.y);
	float diagonal = min(dx, dy) * allow_diagonals;
	float manhattan = dx + dy;
	float chebyshev = 1.4142135f * diagonal + (manhattan - 2.0f * diagonal);
	return chebyshev;
}

const a_star_grid_t* a_star_make_grid(int w, int h, const int* cells)
{
	a_star_grid_t* grid = CUTE_NEW(a_star_grid_t, NULL);
	grid->w = w;
	grid->h = h;
	grid->cells = cells;
	grid->nodes.ensure_count(w * h);
	return grid;
}

void a_star_destroy_grid(a_star_grid_t* grid)
{
	grid->~a_star_grid_t();
	CUTE_FREE(grid, NULL);
}

bool a_star(const a_star_grid_t* const_grid, const a_star_input_t* input, a_star_output_t* output)
{
	a_star_grid_t* grid = (a_star_grid_t*)const_grid;
	grid->reset();
	priority_queue<node_t*>& open_list = grid->open_list;
	iv2 s = { input->start_x, input->start_y };
	iv2 e = { input->end_x, input->end_y };
	bool allow_diagonal_movement = input->allow_diagonal_movement;
	float allow_diagonals = allow_diagonal_movement ? 1.0f : 0;
	node_t* nodes = grid->nodes.data();
	const float* cell_to_cost = input->cell_to_cost;
	const int* cells = grid->cells;
	if (output) {
		output->x.clear();
		output->y.clear();
	}

	if (s.x == e.x && s.y == e.y)
	{
		if (output) {
			output->x.add(s.x);
			output->y.add(s.y);
		}
		return true;
	}

	int w = grid->w;
	int h = grid->h;
	int index = s.y * w + s.x;
	node_t* initial = nodes + index;
	initial->g = 0;
	initial->h = s_heuristic(s, e, allow_diagonals);
	initial->f = initial->h;
	initial->visited = true;
	open_list.push_min(initial, initial->f);

	while (open_list.count()) {
		node_t* q;
		open_list.pop_min(&q);
		iv2 qp = q->p;

		if (qp.x == e.x && qp.y == e.y) {
			if (output) {
				while (q->parent) {
					output->x.add(q->p.x);
					output->y.add(q->p.y);
					q = q->parent;
				}

				output->x.reverse();
				output->y.reverse();
			}

			return true;
		}

		int next_count = 0;
		node_t* next[8];

		#define CUTE_A_STAR_ADD_SUCCESSOR(x, y) \
			if ((x) >= 0 && (x) < w && (y) >= 0 && (y) < h) { \
				next[next_count++] = nodes + ((y) * w + (x)); \
			}
		
		CUTE_A_STAR_ADD_SUCCESSOR(qp.x + 1, qp.y);
		CUTE_A_STAR_ADD_SUCCESSOR(qp.x, qp.y + 1);
		CUTE_A_STAR_ADD_SUCCESSOR(qp.x - 1, qp.y);
		CUTE_A_STAR_ADD_SUCCESSOR(qp.x, qp.y - 1);
		if (allow_diagonal_movement) {
			CUTE_A_STAR_ADD_SUCCESSOR(qp.x + 1, qp.y + 1);
			CUTE_A_STAR_ADD_SUCCESSOR(qp.x - 1, qp.y + 1);
			CUTE_A_STAR_ADD_SUCCESSOR(qp.x + 1, qp.y - 1);
			CUTE_A_STAR_ADD_SUCCESSOR(qp.x - 1, qp.y - 1);
		}
		
		for (int i = 0; i < next_count; ++i) {
			node_t* n = next[i];
			index = n->p.y * w + n->p.x;
			float cell_cost = cell_to_cost[cells[index]];
			bool non_traversable = cell_cost <= 0;
			float g = n->g + cell_cost;
			if (n->visited) continue;
			if (non_traversable) continue;
			if (n->g <= g) continue;

			float h = s_heuristic(n->p, e, allow_diagonals);
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

}
