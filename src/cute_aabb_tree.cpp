/*
	Cute Framework
	Copyright (C) 2021 Randy Gaul https://randygaul.net

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

#include <cute_aabb_tree.h>
#include <cute_array.h>
#include <cute_alloc.h>

#include <internal/cute_serialize_internal.h>

#include <float.h>

#define AABB_TREE_EXPAND_CONSTANT 2.0f
#define AABB_TREE_STACK_QUERY_CAPACITY 256
#define AABB_TREE_NULL_NODE_INDEX -1
#define AABB_TREE_MOVE_CONSTANT 4.0f

using namespace Cute;

struct CF_Aabbree_node_t
{
	int index_a;
	int index_b;
	int index_parent;
	int height;
};

struct CF_AabbTree
{
	int root = AABB_TREE_NULL_NODE_INDEX;
	int freelist = 0;
	int node_capacity = 0;
	int node_count = 0;
	Array<CF_Aabbree_node_t> nodes;
	Array<CF_Aabb> aabbs;
	Array<void*> udatas;
};

static int s_balance(CF_AabbTree* tree, int index_a)
{
	//      a
	//    /   \
	//   b     c
	//  / \   / \
	// d   e f   g

	CF_Aabbree_node_t* nodes = tree->nodes.data();
	CF_Aabb* aabbs = tree->aabbs.data();
	CF_Aabbree_node_t* a = nodes + index_a;
	int index_b = a->index_a;
	int index_c = a->index_b;
	if ((a->index_a == AABB_TREE_NULL_NODE_INDEX) | (a->height < 2)) return index_a;

	CF_Aabbree_node_t* b = nodes + index_b;
	CF_Aabbree_node_t* c = nodes + index_c;
	int balance = c->height - b->height;

	// Rotate c up.
	if (balance > 1) {
		int index_f = c->index_a;
		int index_g = c->index_b;
		CF_Aabbree_node_t* f = nodes + index_f;
		CF_Aabbree_node_t* g = nodes + index_g;

		// Swap a and c.
		c->index_a = index_a;
		c->index_parent = a->index_parent;
		a->index_parent = index_c;

		// Hookup a's old parent to c.
		if (c->index_parent != AABB_TREE_NULL_NODE_INDEX) {
			if (nodes[c->index_parent].index_a == index_a) nodes[c->index_parent].index_a = index_c;
			else {
				CUTE_ASSERT(nodes[c->index_parent].index_b == index_a);
				nodes[c->index_parent].index_b = index_c;
			}
		} else {
			tree->root = index_c;
		}

		// Rotation, picking f or g to go under a or c respectively.
		//       c
		//      / \
		//     a   ? (f or g)
		//    / \
		//   b   ? (f or g)
		//  / \
		// d   e
		if (f->height > g->height) {
			c->index_b = index_f;
			a->index_b = index_g;
			g->index_parent = index_a;
			aabbs[index_a] = cf_combine(aabbs[index_b], aabbs[index_g]);
			aabbs[index_c] = cf_combine(aabbs[index_a], aabbs[index_f]);

			a->height = cf_max_int(b->height, g->height) + 1;
			c->height = cf_max_int(a->height, f->height) + 1;
		} else {
			c->index_b = index_g;
			a->index_b = index_f;
			f->index_parent = index_a;
			aabbs[index_a] = cf_combine(aabbs[index_b], aabbs[index_f]);
			aabbs[index_c] = cf_combine(aabbs[index_a], aabbs[index_g]);

			a->height = cf_max_int(b->height, f->height) + 1;
			c->height = cf_max_int(a->height, g->height) + 1;
		}

		return index_c;
	}

	// Rotate b up.
	else if (balance < -1) {
		int index_d = b->index_a;
		int index_e = b->index_b;
		CF_Aabbree_node_t* d = nodes + index_d;
		CF_Aabbree_node_t* e = nodes + index_e;

		// Swap a and b.
		b->index_a = index_a;
		b->index_parent = a->index_parent;
		a->index_parent = index_b;

		// Hookup a's old parent to b.
		if (b->index_parent != AABB_TREE_NULL_NODE_INDEX) {
			if (nodes[b->index_parent].index_a == index_a) nodes[b->index_parent].index_a = index_b;
			else {
				CUTE_ASSERT(nodes[b->index_parent].index_b == index_a);
				nodes[b->index_parent].index_b = index_b;
			}
		} else {
			tree->root = index_b;
		}

		// Rotation, picking d or e to go under a or b respectively.
		//            b
		//           / \
		// (d or e) ?   a
		//             / \
		//   (d or e) ?   c
		//               / \
		//              f   g
		if (d->height > e->height) {
			b->index_b = index_d;
			a->index_a = index_e;
			e->index_parent = index_a;
			aabbs[index_a] = cf_combine(aabbs[index_c], aabbs[index_e]);
			aabbs[index_b] = cf_combine(aabbs[index_a], aabbs[index_d]);

			a->height = cf_max_int(c->height, e->height) + 1;
			b->height = cf_max_int(a->height, d->height) + 1;
		} else {
			b->index_b = index_e;
			a->index_a = index_d;
			d->index_parent = index_a;
			aabbs[index_a] = cf_combine(aabbs[index_c], aabbs[index_d]);
			aabbs[index_b] = cf_combine(aabbs[index_a], aabbs[index_e]);

			a->height = cf_max_int(c->height, d->height) + 1;
			b->height = cf_max_int(a->height, e->height) + 1;
		}

		return index_b;
	}

	return index_a;
}

static inline void s_sync_node(CF_AabbTree* tree, int index)
{
	CF_Aabb* aabbs = tree->aabbs.data();
	CF_Aabbree_node_t* nodes = tree->nodes.data();
	CF_Aabbree_node_t* node = nodes + index;
	int index_a = node->index_a;
	int index_b = node->index_b;
	node->height = cf_max_int(nodes[index_a].height, nodes[index_b].height) + 1;
	aabbs[index] = cf_combine(aabbs[index_a], aabbs[index_b]);
}

static inline void s_refit_hierarchy(CF_AabbTree* tree, int index)
{
	while (1) {
		if (index == AABB_TREE_NULL_NODE_INDEX) break;
		index = s_balance(tree, index);
		s_sync_node(tree, index);
		index = tree->nodes[index].index_parent;
	}
}

static inline int s_pop_freelist(CF_AabbTree* tree, CF_Aabb aabb, void* udata = NULL)
{
	int new_index = tree->freelist;

	if (new_index == AABB_TREE_NULL_NODE_INDEX) {
		int new_capacity = tree->node_capacity * 2;
		tree->nodes.ensure_count(new_capacity);
		tree->aabbs.ensure_count(new_capacity);
		tree->udatas.ensure_count(new_capacity);

		// Link up new freelist and attach it to pre-existing freelist.
		CF_Aabbree_node_t* new_nodes = tree->nodes.data() + tree->node_capacity;
		for (int i = 0; i < tree->node_capacity - 1; ++i) new_nodes[i].index_a = i + tree->node_capacity + 1;
		new_nodes[tree->node_capacity - 1].index_a = AABB_TREE_NULL_NODE_INDEX;
		tree->freelist = tree->node_capacity;
		new_index = tree->freelist;
		tree->node_capacity = new_capacity;
	}

	tree->freelist = tree->nodes[new_index].index_a;
	tree->nodes[new_index].index_a = AABB_TREE_NULL_NODE_INDEX;
	tree->nodes[new_index].index_b = AABB_TREE_NULL_NODE_INDEX;
	tree->nodes[new_index].index_parent = AABB_TREE_NULL_NODE_INDEX;
	tree->nodes[new_index].height = 0;
	tree->aabbs[new_index] = aabb;
	tree->udatas[new_index] = udata;

	tree->node_count++;

	return new_index;
}

void s_push_freelist(CF_AabbTree* tree, int index)
{
	CUTE_ASSERT(index != AABB_TREE_NULL_NODE_INDEX);
	tree->nodes[index].index_a = tree->freelist;
	tree->freelist = index;
	tree->node_count--;
}

struct CF_Aabbree_priority_queue
{
	inline void init(int* indices, float* costs, int capacity)
	{
		m_count = 0;
		m_capacity = capacity;
		m_indices = indices;
		m_costs = costs;
	}

	inline void push(int index, float cost)
	{
		CUTE_ASSERT(m_count < m_capacity);
		m_indices[m_count] = index;
		m_costs[m_count] = cost;
		++m_count;

		int i = m_count;
		while (i > 1 && predicate(i - 1, i / 2 - 1) > 0) {
			swap(i - 1, i / 2 - 1);
			i /= 2;
		}
	}

	inline bool try_pop(int* index, float* cost)
	{
		if (!m_count) return false;
		*index = m_indices[0];
		*cost = m_costs[0];

		m_count--;
		m_indices[0] = m_indices[m_count];
		m_costs[0] = m_costs[m_count];

		int u = 0, v = 1;
		while (u != v) {
			u = v;
			if (2 * u + 1 <= m_count) {
				if (predicate(u - 1, 2 * u - 1) <= 0) v = 2 * u;
				if (predicate(v - 1, 2 * u + 1 - 1) <= 0) v = 2 * u + 1;
			} else if (2 * u <= m_count) {
				if (predicate(u - 1, 2 * u - 1) <= 0) v = 2 * u;
			}

			if (u != v) {
				swap(u - 1, v - 1);
			}
		}

		return true;
	}

private:
	inline int predicate(int index_a, int index_b)
	{
		float cost_a = m_costs[index_a];
		float cost_b = m_costs[index_b];
		return cost_a < cost_b ? -1 : cost_a > cost_b ? 1 : 0;
	}

	inline void swap(int index_a, int index_b)
	{
		int ival = m_indices[index_a];
		m_indices[index_a] = m_indices[index_b];
		m_indices[index_b] = ival;

		float fval = m_costs[index_a];
		m_costs[index_a] = m_costs[index_b];
		m_costs[index_b] = fval;
	}

	int m_count = 0;
	int m_capacity = 0;
	int* m_indices = NULL;
	float* m_costs = NULL;
};

static inline float s_delta_cost(CF_Aabb to_insert, CF_Aabb candidate)
{
	return cf_surface_area_aabb(cf_combine(to_insert, candidate)) - cf_surface_area_aabb(candidate);
}

// https://en.wikipedia.org/wiki/Branch_and_bound#Generic_version
static int s_branch_and_bound_find_optimal_sibling(CF_AabbTree* tree, CF_Aabb to_insert)
{
	CF_Aabbree_priority_queue queue;
	int indices[AABB_TREE_STACK_QUERY_CAPACITY];
	float costs[AABB_TREE_STACK_QUERY_CAPACITY];
	queue.init(indices, costs, AABB_TREE_STACK_QUERY_CAPACITY);
	queue.push(tree->root, s_delta_cost(to_insert, tree->aabbs[tree->root]));

	float to_insert_sa = cf_surface_area_aabb(to_insert);
	float best_cost = FLT_MAX;
	int best_index = AABB_TREE_NULL_NODE_INDEX;
	int search_index;
	float search_delta_cost;
	while (queue.try_pop(&search_index, &search_delta_cost)) {
		// Track the best candidate so far.
		CF_Aabb search_aabb = tree->aabbs[search_index];
		float cost = cf_surface_area_aabb(cf_combine(to_insert, search_aabb)) + search_delta_cost;
		if (cost < best_cost) {
			best_cost = cost;
			best_index = search_index;
		}

		// Consider pushing the candidate's children onto the priority queue.
		// Cull subtrees with lower bound metric.
		float delta_cost = s_delta_cost(to_insert, search_aabb) + search_delta_cost;
		float lower_bound = to_insert_sa + delta_cost;
		if (lower_bound < best_cost) {
			int index_a = tree->nodes[search_index].index_a;
			int index_b = tree->nodes[search_index].index_b;
			if (index_a != AABB_TREE_NULL_NODE_INDEX) {
				CUTE_ASSERT(index_b != AABB_TREE_NULL_NODE_INDEX);
				queue.push(index_a, delta_cost);
				queue.push(index_b, delta_cost);
			}
		}
	}

	return best_index;
}

static void s_write_v2(uint8_t** p, CF_V2 value)
{
	cf_write_float(p, value.x);
	cf_write_float(p, value.y);
}

static CF_V2 s_read_v2(uint8_t** p)
{
	CF_V2 value;
	value.x = cf_read_float(p);
	value.y = cf_read_float(p);
	return value;
}

static void s_init_from_buffer(CF_AabbTree* tree, uint8_t** p, int height)
{
	bool is_leaf = !!cf_read_uint8(p);
	CF_V2 min = s_read_v2(p);
	CF_V2 max = s_read_v2(p);
	int index_a = AABB_TREE_NULL_NODE_INDEX;
	int index_b = AABB_TREE_NULL_NODE_INDEX;
	if (!is_leaf) {
		index_a = cf_read_uint32(p);
		index_b = cf_read_uint32(p);
	}
	int index_parent = cf_read_uint32(p);

	CF_Aabb aabb = cf_make_aabb(min, max);
	CF_Aabbree_node_t node;
	node.index_a = index_a;
	node.index_b = index_b;
	node.index_parent = index_parent;
	node.height = height;
	tree->aabbs.add(aabb);
	tree->nodes.add(node);
	tree->udatas.add(NULL);

	if (!is_leaf) {
		s_init_from_buffer(tree, p, height + 1);
		s_init_from_buffer(tree, p, height + 1);
	}
}

static inline int s_raycast(CF_Aabb aabb, CF_Ray ray_inv)
{
	CF_V2 d0 = (aabb.min - ray_inv.p) * ray_inv.d;
	CF_V2 d1 = (aabb.min - ray_inv.p) * ray_inv.d;
	CF_V2 v0 = cf_min_v2(d0, d1);
	CF_V2 v1 = cf_max_v2(d0, d1);
	float tmin = cf_hmax(v0);
	float tmax = cf_hmin(v1);
	return (tmax >= 0) && (tmax >= tmin) && (tmin <= ray_inv.t);
}

static float s_tree_cost(const CF_AabbTree* tree, int index)
{
	if (index == AABB_TREE_NULL_NODE_INDEX) return 0;
	float cost_a = s_tree_cost(tree, tree->nodes[index].index_a);
	float cost_b = s_tree_cost(tree, tree->nodes[index].index_b);
	float my_cost = cf_surface_area_aabb(tree->aabbs[index]);
	return cost_a + cost_b + my_cost;
}

static void s_build_index_map(const CF_AabbTree* tree, Array<int>* map, int* i, int index)
{
	if (map->operator[](index) == AABB_TREE_NULL_NODE_INDEX) {
		CUTE_ASSERT(*i < (int)tree->nodes.size());
		map->operator[](index) = *i;
		*i = *i + 1;
	}

	if (tree->nodes[index].index_a != AABB_TREE_NULL_NODE_INDEX) {
		CUTE_ASSERT(tree->nodes[index].index_b != AABB_TREE_NULL_NODE_INDEX);
		s_build_index_map(tree, map, i, tree->nodes[index].index_a);
		s_build_index_map(tree, map, i, tree->nodes[index].index_b);
	}
}

CF_AabbTree* s_remap(const CF_AabbTree* tree)
{
	if (tree->root == AABB_TREE_NULL_NODE_INDEX) {
		return NULL;
	}

	// Build a map of old to new indices.
	Array<int> map;
	map.ensure_count(tree->node_count);
	CUTE_MEMSET(map.data(), AABB_TREE_NULL_NODE_INDEX, sizeof(int) * tree->node_count);
	int i = 0;
	s_build_index_map(tree, &map, &i, tree->root);
	CUTE_ASSERT(i == tree->node_count);

	// Create a copy of the input tree.
	CF_AabbTree* result = cf_make_aabb_tree(tree->node_count);
	result->root = tree->root;
	result->nodes = tree->nodes;
	result->aabbs = tree->aabbs;
	result->udatas = tree->udatas;
	result->freelist = tree->freelist;
	result->node_capacity = tree->node_capacity;
	result->node_count = tree->node_count;
	Array<CF_Aabbree_node_t>& nodes = result->nodes;
	Array<CF_Aabb>& aabbs = result->aabbs;
	Array<void*>& udatas = result->udatas;

	// Swap all values to a contiguous form within the copy.
	for (int i = 0; i < (int)map.size(); ++i) {
		int new_index = map[i];
		if (new_index == AABB_TREE_NULL_NODE_INDEX) continue;
		nodes[new_index] = nodes[i];
		aabbs[new_index] = aabbs[i];
		udatas[new_index] = udatas[i];

		// Reassign all indices within each node.
		int index_a = nodes[new_index].index_a;
		int index_b = nodes[new_index].index_b;
		int index_parent = nodes[new_index].index_parent;

		nodes[new_index].index_a = index_a != AABB_TREE_NULL_NODE_INDEX ? map[index_a] : AABB_TREE_NULL_NODE_INDEX;
		nodes[new_index].index_b = index_b != AABB_TREE_NULL_NODE_INDEX ? map[index_b] : AABB_TREE_NULL_NODE_INDEX;
		nodes[new_index].index_parent = index_parent != AABB_TREE_NULL_NODE_INDEX ? map[index_parent] : AABB_TREE_NULL_NODE_INDEX;
	}

	// Return the copy.
	CUTE_ASSERT(map[result->root] == 0);
	result->root = 0;
	return result;
}

static void s_serialize_to_buffer(CF_AabbTree* tree, uint8_t** p, int index)
{
	CF_Aabbree_node_t* nodes = tree->nodes.data();
	CF_Aabbree_node_t* node = nodes + index;
	bool is_leaf = node->index_a == AABB_TREE_NULL_NODE_INDEX;

	if (is_leaf) {
		cf_write_uint8(p, 1);
	} else {
		cf_write_uint8(p, 0);
	}

	CF_Aabb aabb = tree->aabbs[index];
	s_write_v2(p, aabb.min);
	s_write_v2(p, aabb.max);

	if (!is_leaf) {
		cf_write_uint32(p, node->index_a);
		cf_write_uint32(p, node->index_b);
	}
	cf_write_uint32(p, node->index_parent);

	if (!is_leaf) {
		CUTE_ASSERT(node->index_b != AABB_TREE_NULL_NODE_INDEX);
		s_serialize_to_buffer(tree, p, node->index_a);
		s_serialize_to_buffer(tree, p, node->index_b);
	}
}

static int s_validate(const CF_AabbTree* tree, int index, int depth)
{
	if (index == AABB_TREE_NULL_NODE_INDEX) return depth - 1;
	int depthA = s_validate(tree, tree->nodes[index].index_a, depth + 1);
	int depthB = s_validate(tree, tree->nodes[index].index_b, depth + 1);
	int max_depth = cf_max_int(depthA, depthB);
	int height = max_depth - depth;
	CUTE_ASSERT(height == tree->nodes[index].height);
	return max_depth;
}

static void s_validate(const CF_AabbTree* tree, int index)
{
	CUTE_ASSERT(index != AABB_TREE_NULL_NODE_INDEX);
	if (tree->nodes[index].index_a != AABB_TREE_NULL_NODE_INDEX) CUTE_ASSERT(tree->nodes[index].index_b != AABB_TREE_NULL_NODE_INDEX);
	if (tree->nodes[index].index_a == AABB_TREE_NULL_NODE_INDEX) CUTE_ASSERT(tree->nodes[index].index_b == AABB_TREE_NULL_NODE_INDEX);
	if (tree->nodes[index].index_a != AABB_TREE_NULL_NODE_INDEX) {
		s_validate(tree, tree->nodes[index].index_a);
		s_validate(tree, tree->nodes[index].index_b);
		CF_Aabb parent_aabb = tree->aabbs[index];
		CF_Aabb a = tree->aabbs[tree->nodes[index].index_a];
		CF_Aabb b = tree->aabbs[tree->nodes[index].index_b];
		CUTE_ASSERT(cf_contains_aabb(parent_aabb, a));
		CUTE_ASSERT(cf_contains_aabb(parent_aabb, b));
	}
}

//--------------------------------------------------------------------------------------------------

CF_AabbTree* cf_make_aabb_tree(int initial_capacity)
{
	CF_AabbTree* tree = CUTE_NEW(CF_AabbTree);
	if (!initial_capacity) initial_capacity = 64;
	tree->node_capacity = initial_capacity;
	tree->nodes.ensure_count(initial_capacity);
	tree->aabbs.ensure_count(initial_capacity);
	tree->udatas.ensure_count(initial_capacity);
	for (int i = 0; i < tree->nodes.count() - 1; ++i) tree->nodes[i].index_a = i + 1;
	tree->nodes[tree->nodes.count() - 1].index_a = AABB_TREE_NULL_NODE_INDEX;
	return tree;
}

CF_AabbTree* cf_make_aabb_tree_from_memory(const void* buffer, size_t size)
{
	uint8_t* p = (uint8_t*)buffer;
	uint8_t fourcc[4];
	cf_read_fourcc(&p, fourcc);
	if (CUTE_MEMCMP(fourcc, "aabb", 4)) return NULL;

	int node_count = (int)cf_read_uint32(&p);
	CF_AabbTree* tree = cf_make_aabb_tree(0);
	tree->nodes.ensure_count(node_count);
	tree->aabbs.ensure_count(node_count);
	tree->udatas.ensure_count(node_count);
	s_init_from_buffer(tree, &p, 0);

	tree->node_count = node_count;
	tree->root = 0;
	tree->freelist = AABB_TREE_NULL_NODE_INDEX;

	return tree;
}

void cf_destroy_aabb_tree(CF_AabbTree* tree)
{
	tree->~CF_AabbTree();
	CUTE_FREE(tree);
}

static CF_Leaf s_insert(CF_AabbTree* tree, CF_Aabb aabb, void* udata)
{
	// Make a new node.
	int new_index = s_pop_freelist(tree, aabb, udata);
	int search_index = tree->root;

	// Empty tree, make new root.
	if (search_index == AABB_TREE_NULL_NODE_INDEX) {
		tree->root = new_index;
	} else {
		search_index = s_branch_and_bound_find_optimal_sibling(tree, aabb);

		// Make new branch node.
		int branch_index = s_pop_freelist(tree, cf_combine(aabb, tree->aabbs[search_index]));
		CF_Aabbree_node_t* branch = tree->nodes.data() + branch_index;
		int parent_index = tree->nodes[search_index].index_parent;

		if (parent_index == AABB_TREE_NULL_NODE_INDEX) {
			tree->root = branch_index;
		} else {
			CF_Aabbree_node_t* parent = tree->nodes.data() + parent_index;

			// Hookup parent to the new branch.
			if (parent->index_a == search_index) {
				parent->index_a = branch_index;
			} else {
				CUTE_ASSERT(parent->index_b == search_index);
				parent->index_b = branch_index;
			}
		}

		// Assign branch children and parent.
		branch->index_a = search_index;
		branch->index_b = new_index;
		branch->index_parent = parent_index;
		branch->height = tree->nodes[search_index].height + 1;

		// Assign parent pointers for new the leaf pair, and heights.
		tree->nodes[search_index].index_parent = branch_index;
		tree->nodes[new_index].index_parent = branch_index;

		// The new node should have no children.
		CUTE_ASSERT(tree->nodes[new_index].index_a == AABB_TREE_NULL_NODE_INDEX);
		CUTE_ASSERT(tree->nodes[new_index].index_b == AABB_TREE_NULL_NODE_INDEX);

		// Branches should never have udata.
		CUTE_ASSERT(tree->udatas[branch_index] == NULL);

		// Children should be contained by the parent branch.
		CUTE_ASSERT(cf_contains_aabb(tree->aabbs[branch_index], tree->aabbs[search_index]));
		CUTE_ASSERT(cf_contains_aabb(tree->aabbs[branch_index], tree->aabbs[new_index]));

		s_refit_hierarchy(tree, parent_index);
	}

	CF_Aabbree_validate(tree);

	return { new_index };
}

CF_Leaf CF_Aabbree_insert(CF_AabbTree* tree, CF_Aabb aabb, void* udata)
{
	aabb = cf_expand_aabb_f(aabb, AABB_TREE_EXPAND_CONSTANT);
	return s_insert(tree, aabb, udata);
}

void CF_Aabbree_remove(CF_AabbTree* tree, CF_Leaf leaf)
{
	int index = leaf.id;
	if (tree->root == index) {
		tree->root = AABB_TREE_NULL_NODE_INDEX;
	} else {
		CF_Aabbree_node_t* nodes = tree->nodes.data();
		CF_Aabbree_node_t* node = nodes + index;
		int index_parent = node->index_parent;
		CF_Aabbree_node_t* parent = nodes + index_parent;

		// Can only remove leaves.
		CUTE_ASSERT(node->index_a == AABB_TREE_NULL_NODE_INDEX);
		CUTE_ASSERT(node->index_b == AABB_TREE_NULL_NODE_INDEX);

		if (index_parent == tree->root) {
			if (parent->index_a == index) tree->root = parent->index_b;
			else tree->root = parent->index_a;
			nodes[tree->root].index_parent = AABB_TREE_NULL_NODE_INDEX;
		} else {
			int index_grandparent = parent->index_parent;
			CF_Aabbree_node_t* grandparent = nodes + index_grandparent;

			if (parent->index_a == index) {
				CF_Aabbree_node_t* sindex_bling = nodes + parent->index_b;
				sindex_bling->index_parent = index_grandparent;
				if (grandparent->index_a == index_parent) grandparent->index_a = parent->index_b;
				else grandparent->index_b = parent->index_b;
			} else {
				CUTE_ASSERT(parent->index_b == index);
				CF_Aabbree_node_t* sindex_bling = nodes + parent->index_a;
				sindex_bling->index_parent = index_grandparent;
				if (grandparent->index_a == index_parent) grandparent->index_a = parent->index_a;
				else grandparent->index_b = parent->index_a;
			}

			s_refit_hierarchy(tree, index_grandparent);
		}

		s_push_freelist(tree, index_parent);
	}

	s_push_freelist(tree, index);
}

bool CF_Aabbree_update_leaf(CF_AabbTree* tree, CF_Leaf leaf, CF_Aabb aabb)
{
	// Can only update leaves.
	CUTE_ASSERT(tree->nodes[leaf.id].index_a == AABB_TREE_NULL_NODE_INDEX);
	CUTE_ASSERT(tree->nodes[leaf.id].index_b == AABB_TREE_NULL_NODE_INDEX);

	if (cf_contains_aabb(tree->aabbs[leaf.id], aabb)) {
		tree->aabbs[leaf.id] = aabb;
		return false;
	}

	void* udata = tree->udatas[leaf.id];
	CF_Aabbree_remove(tree, leaf);
	CF_Aabbree_insert(tree, aabb, udata);

	return true;
}

bool CF_Aabbree_move(CF_AabbTree* tree, CF_Leaf leaf, CF_Aabb aabb, CF_V2 offset)
{
	// Can only update leaves.
	CUTE_ASSERT(tree->nodes[leaf.id].index_a == AABB_TREE_NULL_NODE_INDEX);
	CUTE_ASSERT(tree->nodes[leaf.id].index_b == AABB_TREE_NULL_NODE_INDEX);

	aabb = cf_expand_aabb_f(aabb, AABB_TREE_EXPAND_CONSTANT);
	CF_V2 delta = offset * AABB_TREE_MOVE_CONSTANT;

	if (delta.x < 0) {
		aabb.min.x += delta.x;
	} else {
		aabb.max.x += delta.x;
	}

	if (delta.y < 0) {
		aabb.min.y += delta.y;
	} else {
		aabb.max.y += delta.y;
	}

	CF_Aabb old_aabb = tree->aabbs[leaf.id];
	if (cf_contains_aabb(old_aabb, aabb)) {
		CF_Aabb big_aabb = cf_expand_aabb_f(aabb, AABB_TREE_MOVE_CONSTANT);
		bool old_aabb_is_not_way_too_huge = cf_contains_aabb(big_aabb, old_aabb);
		if (old_aabb_is_not_way_too_huge) {
			return false;
		}
	}

	void* udata = tree->udatas[leaf.id];
	CF_Aabbree_remove(tree, leaf);
	s_insert(tree, aabb, udata);

	return true;
}

CF_Aabb CF_Aabbree_get_aabb(CF_AabbTree* tree, CF_Leaf leaf)
{
	return tree->aabbs[leaf.id];
}

void* CF_Aabbree_get_udata(CF_AabbTree* tree, CF_Leaf leaf)
{
	return tree->udatas[leaf.id];
}

void CF_Aabbree_query_aabb(const CF_AabbTree* tree, CF_AabbTreeQueryFn* fn, CF_Aabb aabb, void* fn_udata)
{
	if (tree->root == AABB_TREE_NULL_NODE_INDEX) return;
	int index_stack[AABB_TREE_STACK_QUERY_CAPACITY];
	int sp = 1;
	const CF_Aabbree_node_t* nodes = tree->nodes.data();
	const CF_Aabb* aabbs = tree->aabbs.data();
	void* const* udatas = tree->udatas.data();
	index_stack[0] = tree->root;

	while (sp) {
		CUTE_ASSERT(sp < AABB_TREE_STACK_QUERY_CAPACITY);
		int index = index_stack[--sp];
		CF_Aabb search_aabb = aabbs[index];

		if (cf_collide_aabb(aabb, search_aabb)) {
			const CF_Aabbree_node_t* node = nodes + index;

			if (node->index_a == AABB_TREE_NULL_NODE_INDEX) {
				CF_Leaf leaf = { index };
				if (!fn(leaf, search_aabb, udatas[index], fn_udata)) {
					return;
				}
			} else {
				index_stack[sp++] = node->index_a;
				index_stack[sp++] = node->index_b;
			}
		}
	}
}

void CF_Aabbree_query_ray(const CF_AabbTree* tree, CF_AabbTreeQueryFn* fn, CF_Ray ray, void* fn_udata)
{
	if (tree->root == AABB_TREE_NULL_NODE_INDEX) return;
	int index_stack[AABB_TREE_STACK_QUERY_CAPACITY];
	int sp = 1;
	const CF_Aabbree_node_t* nodes = tree->nodes.data();
	const CF_Aabb* aabbs = tree->aabbs.data();
	void* const* udatas = tree->udatas.data();
	index_stack[0] = tree->root;

	CF_Ray ray_inv = ray;
	ray_inv.d = cf_invert_safe_v2(ray.d);
	CF_V2 ray_end = cf_endpoint(ray);
	CF_Aabb ray_aabb;
	ray_aabb.min = cf_min_v2(ray.p, ray_end);
	ray_aabb.max = cf_min_v2(ray.p, ray_end);

	while (sp) {
		CUTE_ASSERT(sp < AABB_TREE_STACK_QUERY_CAPACITY);
		int index = index_stack[--sp];
		CF_Aabb search_aabb = aabbs[index];

		if (!cf_collide_aabb(ray_aabb, search_aabb)) {
			continue;
		}

		if (s_raycast(search_aabb, ray_inv)) {
			const CF_Aabbree_node_t* node = nodes + index;

			if (node->index_a != AABB_TREE_NULL_NODE_INDEX) {
				CF_Leaf leaf = { index };
				if (!fn(leaf, search_aabb, udatas[index], fn_udata)) {
					return;
				}
			} else {
				index_stack[sp++] = node->index_a;
				index_stack[sp++] = node->index_b;
			}
		}
	}
}

float CF_Aabbree_cost(const CF_AabbTree* tree)
{
	return s_tree_cost(tree, tree->root);
}

void CF_Aabbree_validate(const CF_AabbTree* tree)
{
	if (tree->root != AABB_TREE_NULL_NODE_INDEX) {
		s_validate(tree, tree->root, 0);
		s_validate(tree, tree->root);
	}
}

size_t CF_Aabbree_serialized_size(const CF_AabbTree* tree)
{
	size_t fourcc_size = 4;
	size_t node_count_size = sizeof(uint32_t);
	size_t overhead_per_node = 1;
	size_t aabb_size = sizeof(float) * 2 * 2;
	size_t internal_node_size = sizeof(uint32_t);
	size_t leaf_node_size = sizeof(uint32_t) * 3;
	size_t leaf_node_count = ((size_t)tree->node_count + 1) / 2;
	size_t internal_node_count = (size_t)tree->node_count - leaf_node_count;
	size_t all_internal_nodes_size = (aabb_size + internal_node_size + overhead_per_node) * internal_node_count;
	size_t all_leaf_nodes_size = (aabb_size + leaf_node_size + overhead_per_node) * leaf_node_count;
	size_t needed_size = fourcc_size + node_count_size + all_internal_nodes_size + all_leaf_nodes_size;
	return needed_size;
}

bool CF_Aabbree_serialize(const CF_AabbTree* tree, void* buffer, size_t size)
{
	size_t needed_size = CF_Aabbree_serialized_size(tree);
	if (needed_size < size) return false;
	CF_AabbTree* copy = s_remap(tree);

	uint8_t* p = (uint8_t*)buffer;
	cf_write_fourcc(&p, "aabb");
	cf_write_uint32(&p, (uint32_t)copy->node_count);
	s_serialize_to_buffer(copy, &p, copy->root);
	cf_destroy_aabb_tree(copy);

	size_t size_written = p - (uint8_t*)buffer;
	CUTE_ASSERT(size_written <= size);
	if (size_written > size) {
		return false;
	}

	return true;
}