/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_aabb_tree.h>
#include <cute_array.h>
#include <cute_alloc.h>
#include <cute_defer.h>

#include <internal/cute_alloc_internal.h>
#include <internal/cute_serialize_internal.h>

#include <float.h>

#define AABB_TREE_EXPAND_CONSTANT 2.0f
#define AABB_TREE_STACK_QUERY_CAPACITY 256
#define AABB_TREE_NULL_NODE_INDEX -1
#define AABB_TREE_MOVE_CONSTANT 4.0f

using namespace Cute;

struct CF_AabbTreeNode
{
	int index_a;
	int index_b;
	int index_parent;
	int height;
};

struct CF_AabbTreeInternal
{
	int root = AABB_TREE_NULL_NODE_INDEX;
	int freelist = 0;
	int node_capacity = 0;
	int node_count = 0;
	Array<CF_AabbTreeNode> nodes;
	Array<CF_Aabb> aabbs;
	Array<void*> udatas;
};

static int s_balance(CF_AabbTreeInternal* tree, int index_a)
{
	//      a
	//    /   \
	//   b     c
	//  / \   / \
	// d   e f   g

	CF_AabbTreeNode* nodes = tree->nodes.data();
	CF_Aabb* aabbs = tree->aabbs.data();
	CF_AabbTreeNode* a = nodes + index_a;
	int index_b = a->index_a;
	int index_c = a->index_b;
	if ((a->index_a == AABB_TREE_NULL_NODE_INDEX) | (a->height < 2)) return index_a;

	CF_AabbTreeNode* b = nodes + index_b;
	CF_AabbTreeNode* c = nodes + index_c;
	int balance = c->height - b->height;

	// Rotate c up.
	if (balance > 1) {
		int index_f = c->index_a;
		int index_g = c->index_b;
		CF_AabbTreeNode* f = nodes + index_f;
		CF_AabbTreeNode* g = nodes + index_g;

		// Swap a and c.
		c->index_a = index_a;
		c->index_parent = a->index_parent;
		a->index_parent = index_c;

		// Hookup a's old parent to c.
		if (c->index_parent != AABB_TREE_NULL_NODE_INDEX) {
			if (nodes[c->index_parent].index_a == index_a) nodes[c->index_parent].index_a = index_c;
			else {
				CF_ASSERT(nodes[c->index_parent].index_b == index_a);
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

			a->height = cf_max(b->height, g->height) + 1;
			c->height = cf_max(a->height, f->height) + 1;
		} else {
			c->index_b = index_g;
			a->index_b = index_f;
			f->index_parent = index_a;
			aabbs[index_a] = cf_combine(aabbs[index_b], aabbs[index_f]);
			aabbs[index_c] = cf_combine(aabbs[index_a], aabbs[index_g]);

			a->height = cf_max(b->height, f->height) + 1;
			c->height = cf_max(a->height, g->height) + 1;
		}

		return index_c;
	}

	// Rotate b up.
	else if (balance < -1) {
		int index_d = b->index_a;
		int index_e = b->index_b;
		CF_AabbTreeNode* d = nodes + index_d;
		CF_AabbTreeNode* e = nodes + index_e;

		// Swap a and b.
		b->index_a = index_a;
		b->index_parent = a->index_parent;
		a->index_parent = index_b;

		// Hookup a's old parent to b.
		if (b->index_parent != AABB_TREE_NULL_NODE_INDEX) {
			if (nodes[b->index_parent].index_a == index_a) nodes[b->index_parent].index_a = index_b;
			else {
				CF_ASSERT(nodes[b->index_parent].index_b == index_a);
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

			a->height = cf_max(c->height, e->height) + 1;
			b->height = cf_max(a->height, d->height) + 1;
		} else {
			b->index_b = index_e;
			a->index_a = index_d;
			d->index_parent = index_a;
			aabbs[index_a] = cf_combine(aabbs[index_c], aabbs[index_d]);
			aabbs[index_b] = cf_combine(aabbs[index_a], aabbs[index_e]);

			a->height = cf_max(c->height, d->height) + 1;
			b->height = cf_max(a->height, e->height) + 1;
		}

		return index_b;
	}

	return index_a;
}

static inline void s_sync_node(CF_AabbTreeInternal* tree, int index)
{
	CF_Aabb* aabbs = tree->aabbs.data();
	CF_AabbTreeNode* nodes = tree->nodes.data();
	CF_AabbTreeNode* node = nodes + index;
	int index_a = node->index_a;
	int index_b = node->index_b;
	node->height = cf_max(nodes[index_a].height, nodes[index_b].height) + 1;
	aabbs[index] = cf_combine(aabbs[index_a], aabbs[index_b]);
}

static inline void s_refit_hierarchy(CF_AabbTreeInternal* tree, int index)
{
	while (1) {
		if (index == AABB_TREE_NULL_NODE_INDEX) break;
		index = s_balance(tree, index);
		s_sync_node(tree, index);
		index = tree->nodes[index].index_parent;
	}
}

static inline int s_pop_freelist(CF_AabbTreeInternal* tree, CF_Aabb aabb, void* udata = NULL)
{
	int new_index = tree->freelist;

	if (new_index == AABB_TREE_NULL_NODE_INDEX) {
		int new_capacity = tree->node_capacity * 2;
		tree->nodes.ensure_count(new_capacity);
		tree->aabbs.ensure_count(new_capacity);
		tree->udatas.ensure_count(new_capacity);

		// Link up new freelist and attach it to pre-existing freelist.
		CF_AabbTreeNode* new_nodes = tree->nodes.data() + tree->node_capacity;
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

void s_push_freelist(CF_AabbTreeInternal* tree, int index)
{
	CF_ASSERT(index != AABB_TREE_NULL_NODE_INDEX);
	tree->nodes[index].index_a = tree->freelist;
	tree->freelist = index;
	tree->node_count--;
}

struct CF_AabbTreePriorityQueue
{
	~CF_AabbTreePriorityQueue()
	{
		afree(m_indices);
		afree(m_costs);
	}

	inline void init(int* indices, float* costs, int capacity)
	{
		m_count = 0;
		astatic(m_indices, indices, capacity);
		astatic(m_costs, costs, capacity);
	}

	inline void push(int index, float cost)
	{
		apush(m_indices, index);
		apush(m_costs, cost);
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
		m_indices[0] = apop(m_indices);
		m_costs[0] = apop(m_costs);

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
	dyna int* m_indices = NULL;
	dyna float* m_costs = NULL;
};

static inline float s_delta_cost(CF_Aabb to_insert, CF_Aabb candidate)
{
	return cf_surface_area_aabb(cf_combine(to_insert, candidate)) - cf_surface_area_aabb(candidate);
}

// https://en.wikipedia.org/wiki/Branch_and_bound#Generic_version
static int s_branch_and_bound_find_optimal_sibling(CF_AabbTreeInternal* tree, CF_Aabb to_insert)
{
	CF_AabbTreePriorityQueue queue;
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
				CF_ASSERT(index_b != AABB_TREE_NULL_NODE_INDEX);
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

static void s_init_from_buffer(CF_AabbTreeInternal* tree, uint8_t** p, int height)
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
	CF_AabbTreeNode node;
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

static float s_tree_cost(CF_AabbTreeInternal* tree, int index)
{
	if (index == AABB_TREE_NULL_NODE_INDEX) return 0;
	float cost_a = s_tree_cost(tree, tree->nodes[index].index_a);
	float cost_b = s_tree_cost(tree, tree->nodes[index].index_b);
	float my_cost = cf_surface_area_aabb(tree->aabbs[index]);
	return cost_a + cost_b + my_cost;
}

static void s_build_index_map(CF_AabbTreeInternal* tree, Array<int>* map, int* i, int index)
{
	if (map->operator[](index) == AABB_TREE_NULL_NODE_INDEX) {
		CF_ASSERT(*i < (int)tree->nodes.size());
		map->operator[](index) = *i;
		*i = *i + 1;
	}

	if (tree->nodes[index].index_a != AABB_TREE_NULL_NODE_INDEX) {
		CF_ASSERT(tree->nodes[index].index_b != AABB_TREE_NULL_NODE_INDEX);
		s_build_index_map(tree, map, i, tree->nodes[index].index_a);
		s_build_index_map(tree, map, i, tree->nodes[index].index_b);
	}
}

CF_AabbTreeInternal* s_remap(CF_AabbTreeInternal* tree)
{
	if (tree->root == AABB_TREE_NULL_NODE_INDEX) {
		return NULL;
	}

	// Build a map of old to new indices.
	Array<int> map;
	map.ensure_count(tree->node_count);
	CF_MEMSET(map.data(), AABB_TREE_NULL_NODE_INDEX, sizeof(int) * tree->node_count);
	int i = 0;
	s_build_index_map(tree, &map, &i, tree->root);
	CF_ASSERT(i == tree->node_count);

	// Create a copy of the input tree.
	CF_AabbTreeInternal* result = (CF_AabbTreeInternal*)cf_make_aabb_tree(tree->node_count).id;
	result->root = tree->root;
	result->nodes = tree->nodes;
	result->aabbs = tree->aabbs;
	result->udatas = tree->udatas;
	result->freelist = tree->freelist;
	result->node_capacity = tree->node_capacity;
	result->node_count = tree->node_count;
	Array<CF_AabbTreeNode>& nodes = result->nodes;
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
	CF_ASSERT(map[result->root] == 0);
	result->root = 0;
	return result;
}

static void s_serialize_to_buffer(CF_AabbTreeInternal* tree, uint8_t** p, int index)
{
	CF_AabbTreeNode* nodes = tree->nodes.data();
	CF_AabbTreeNode* node = nodes + index;
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
		CF_ASSERT(node->index_b != AABB_TREE_NULL_NODE_INDEX);
		s_serialize_to_buffer(tree, p, node->index_a);
		s_serialize_to_buffer(tree, p, node->index_b);
	}
}

static int s_validate(CF_AabbTreeInternal* tree, int index, int depth)
{
	if (index == AABB_TREE_NULL_NODE_INDEX) return depth - 1;
	int depthA = s_validate(tree, tree->nodes[index].index_a, depth + 1);
	int depthB = s_validate(tree, tree->nodes[index].index_b, depth + 1);
	int max_depth = cf_max(depthA, depthB);
	int height = max_depth - depth;
	CF_ASSERT(height == tree->nodes[index].height);
	return max_depth;
}

static void s_validate(CF_AabbTreeInternal* tree, int index)
{
	CF_ASSERT(index != AABB_TREE_NULL_NODE_INDEX);
	if (tree->nodes[index].index_a != AABB_TREE_NULL_NODE_INDEX) CF_ASSERT(tree->nodes[index].index_b != AABB_TREE_NULL_NODE_INDEX);
	if (tree->nodes[index].index_a == AABB_TREE_NULL_NODE_INDEX) CF_ASSERT(tree->nodes[index].index_b == AABB_TREE_NULL_NODE_INDEX);
	if (tree->nodes[index].index_a != AABB_TREE_NULL_NODE_INDEX) {
		s_validate(tree, tree->nodes[index].index_a);
		s_validate(tree, tree->nodes[index].index_b);
		CF_Aabb parent_aabb = tree->aabbs[index];
		CF_Aabb a = tree->aabbs[tree->nodes[index].index_a];
		CF_Aabb b = tree->aabbs[tree->nodes[index].index_b];
		CF_ASSERT(cf_contains_aabb(parent_aabb, a));
		CF_ASSERT(cf_contains_aabb(parent_aabb, b));
	}
}

//--------------------------------------------------------------------------------------------------

CF_AabbTree cf_make_aabb_tree(int initial_capacity)
{
	CF_AabbTreeInternal* tree = CF_NEW(CF_AabbTreeInternal);
	if (!initial_capacity) initial_capacity = 64;
	tree->node_capacity = initial_capacity;
	tree->nodes.ensure_count(initial_capacity);
	tree->aabbs.ensure_count(initial_capacity);
	tree->udatas.ensure_count(initial_capacity);
	for (int i = 0; i < tree->nodes.count() - 1; ++i) tree->nodes[i].index_a = i + 1;
	tree->nodes[tree->nodes.count() - 1].index_a = AABB_TREE_NULL_NODE_INDEX;
	CF_AabbTree result;
	result.id = (uint64_t)tree;
	return result;
}

CF_AabbTree cf_make_aabb_tree_from_memory(const void* buffer, size_t size)
{
	CF_AabbTreeInternal* tree = CF_NEW(CF_AabbTreeInternal);
	uint8_t* p = (uint8_t*)buffer;
	uint8_t fourcc[4];
	cf_read_fourcc(&p, fourcc);
	if (CF_MEMCMP(fourcc, "aabb", 4)) return { 0 };

	int node_count = (int)cf_read_uint32(&p);
	CF_AabbTree tree_handle = cf_make_aabb_tree(0);
	tree->nodes.ensure_count(node_count);
	tree->aabbs.ensure_count(node_count);
	tree->udatas.ensure_count(node_count);
	s_init_from_buffer(tree, &p, 0);

	tree->node_count = node_count;
	tree->root = 0;
	tree->freelist = AABB_TREE_NULL_NODE_INDEX;

	CF_AabbTree result;
	result.id = (uint64_t)tree;
	return result;
}

void cf_destroy_aabb_tree(CF_AabbTree tree_handle)
{
	CF_AabbTreeInternal* tree = (CF_AabbTreeInternal*)tree_handle.id;
	tree->~CF_AabbTreeInternal();
	CF_FREE(tree);
}

static CF_Leaf s_insert(CF_AabbTreeInternal* tree, CF_Aabb aabb, void* udata)
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
		CF_AabbTreeNode* branch = tree->nodes.data() + branch_index;
		int parent_index = tree->nodes[search_index].index_parent;

		if (parent_index == AABB_TREE_NULL_NODE_INDEX) {
			tree->root = branch_index;
		} else {
			CF_AabbTreeNode* parent = tree->nodes.data() + parent_index;

			// Hookup parent to the new branch.
			if (parent->index_a == search_index) {
				parent->index_a = branch_index;
			} else {
				CF_ASSERT(parent->index_b == search_index);
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
		CF_ASSERT(tree->nodes[new_index].index_a == AABB_TREE_NULL_NODE_INDEX);
		CF_ASSERT(tree->nodes[new_index].index_b == AABB_TREE_NULL_NODE_INDEX);

		// Branches should never have udata.
		CF_ASSERT(tree->udatas[branch_index] == NULL);

		// Children should be contained by the parent branch.
		CF_ASSERT(cf_contains_aabb(tree->aabbs[branch_index], tree->aabbs[search_index]));
		CF_ASSERT(cf_contains_aabb(tree->aabbs[branch_index], tree->aabbs[new_index]));

		s_refit_hierarchy(tree, parent_index);
	}

	CF_AabbTree tree_handle;
	tree_handle.id = (uint64_t)tree;
	cf_aabb_tree_validate(tree_handle);

	return { new_index };
}

CF_Leaf cf_aabb_tree_insert(CF_AabbTree tree_handle, CF_Aabb aabb, void* udata)
{
	CF_AabbTreeInternal* tree = (CF_AabbTreeInternal*)tree_handle.id;
	aabb = cf_expand_aabb_f(aabb, AABB_TREE_EXPAND_CONSTANT);
	return s_insert(tree, aabb, udata);
}

void cf_aabb_tree_remove(CF_AabbTree tree_handle, CF_Leaf leaf)
{
	CF_AabbTreeInternal* tree = (CF_AabbTreeInternal*)tree_handle.id;
	int index = leaf.id;
	if (tree->root == index) {
		tree->root = AABB_TREE_NULL_NODE_INDEX;
	} else {
		CF_AabbTreeNode* nodes = tree->nodes.data();
		CF_AabbTreeNode* node = nodes + index;
		int index_parent = node->index_parent;
		CF_AabbTreeNode* parent = nodes + index_parent;

		// Can only remove leaves.
		CF_ASSERT(node->index_a == AABB_TREE_NULL_NODE_INDEX);
		CF_ASSERT(node->index_b == AABB_TREE_NULL_NODE_INDEX);

		if (index_parent == tree->root) {
			if (parent->index_a == index) tree->root = parent->index_b;
			else tree->root = parent->index_a;
			nodes[tree->root].index_parent = AABB_TREE_NULL_NODE_INDEX;
		} else {
			int index_grandparent = parent->index_parent;
			CF_AabbTreeNode* grandparent = nodes + index_grandparent;

			if (parent->index_a == index) {
				CF_AabbTreeNode* sindex_bling = nodes + parent->index_b;
				sindex_bling->index_parent = index_grandparent;
				if (grandparent->index_a == index_parent) grandparent->index_a = parent->index_b;
				else grandparent->index_b = parent->index_b;
			} else {
				CF_ASSERT(parent->index_b == index);
				CF_AabbTreeNode* sindex_bling = nodes + parent->index_a;
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

bool cf_aabb_tree_update_leaf(CF_AabbTree tree_handle, CF_Leaf leaf, CF_Aabb aabb)
{
	CF_AabbTreeInternal* tree = (CF_AabbTreeInternal*)tree_handle.id;

	// Can only update leaves.
	CF_ASSERT(tree->nodes[leaf.id].index_a == AABB_TREE_NULL_NODE_INDEX);
	CF_ASSERT(tree->nodes[leaf.id].index_b == AABB_TREE_NULL_NODE_INDEX);

	if (cf_contains_aabb(tree->aabbs[leaf.id], aabb)) {
		tree->aabbs[leaf.id] = aabb;
		return false;
	}

	void* udata = tree->udatas[leaf.id];
	cf_aabb_tree_remove(tree_handle, leaf);
	cf_aabb_tree_insert(tree_handle, aabb, udata);

	return true;
}

bool cf_aabb_tree_move(CF_AabbTree tree_handle, CF_Leaf leaf, CF_Aabb aabb, CF_V2 offset)
{
	CF_AabbTreeInternal* tree = (CF_AabbTreeInternal*)tree_handle.id;

	// Can only update leaves.
	CF_ASSERT(tree->nodes[leaf.id].index_a == AABB_TREE_NULL_NODE_INDEX);
	CF_ASSERT(tree->nodes[leaf.id].index_b == AABB_TREE_NULL_NODE_INDEX);

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
	cf_aabb_tree_remove(tree_handle, leaf);
	s_insert(tree, aabb, udata);

	return true;
}

CF_Aabb cf_aabb_tree_get_aabb(CF_AabbTree tree_handle, CF_Leaf leaf)
{
	CF_AabbTreeInternal* tree = (CF_AabbTreeInternal*)tree_handle.id;
	return tree->aabbs[leaf.id];
}

void* cf_aabb_tree_get_udata(CF_AabbTree tree_handle, CF_Leaf leaf)
{
	CF_AabbTreeInternal* tree = (CF_AabbTreeInternal*)tree_handle.id;
	return tree->udatas[leaf.id];
}

void cf_aabb_tree_query_aabb(const CF_AabbTree tree_handle, CF_AabbTreeQueryFn* fn, CF_Aabb aabb, void* fn_udata)
{
	CF_AabbTreeInternal* tree = (CF_AabbTreeInternal*)tree_handle.id;
	if (tree->root == AABB_TREE_NULL_NODE_INDEX) return;
	int index_stack_buf[AABB_TREE_STACK_QUERY_CAPACITY];
	int* index_stack = NULL;
	astatic(index_stack, index_stack_buf, CF_ARRAY_SIZE(index_stack_buf));
	CF_DEFER(afree(index_stack));
	const CF_AabbTreeNode* nodes = tree->nodes.data();
	const CF_Aabb* aabbs = tree->aabbs.data();
	void* const* udatas = tree->udatas.data();
	apush(index_stack, tree->root);

	while (alen(index_stack)) {
		int index = apop(index_stack);
		CF_Aabb search_aabb = aabbs[index];

		if (cf_collide_aabb(aabb, search_aabb)) {
			const CF_AabbTreeNode* node = nodes + index;

			if (node->index_a == AABB_TREE_NULL_NODE_INDEX) {
				CF_Leaf leaf = { index };
				if (!fn(leaf, search_aabb, udatas[index], fn_udata)) {
					return;
				}
			} else {
				apush(index_stack, node->index_a);
				apush(index_stack, node->index_b);
			}
		}
	}
}

void cf_aabb_tree_query_ray(const CF_AabbTree tree_handle, CF_AabbTreeQueryFn* fn, CF_Ray ray, void* fn_udata)
{
	CF_AabbTreeInternal* tree = (CF_AabbTreeInternal*)tree_handle.id;
	if (tree->root == AABB_TREE_NULL_NODE_INDEX) return;
	int index_stack_buf[AABB_TREE_STACK_QUERY_CAPACITY];
	int* index_stack = NULL;
	astatic(index_stack, index_stack_buf, CF_ARRAY_SIZE(index_stack_buf));
	CF_DEFER(afree(index_stack));
	const CF_AabbTreeNode* nodes = tree->nodes.data();
	const CF_Aabb* aabbs = tree->aabbs.data();
	void* const* udatas = tree->udatas.data();
	apush(index_stack, tree->root);

	CF_Ray ray_inv = ray;
	ray_inv.d = cf_safe_invert_v2(ray.d);
	CF_V2 ray_end = cf_endpoint(ray);
	CF_Aabb ray_aabb;
	ray_aabb.min = cf_min_v2(ray.p, ray_end);
	ray_aabb.max = cf_min_v2(ray.p, ray_end);

	while (alen(index_stack)) {
		int index = apop(index_stack);
		CF_Aabb search_aabb = aabbs[index];

		if (!cf_collide_aabb(ray_aabb, search_aabb)) {
			continue;
		}

		if (s_raycast(search_aabb, ray_inv)) {
			const CF_AabbTreeNode* node = nodes + index;

			if (node->index_a != AABB_TREE_NULL_NODE_INDEX) {
				CF_Leaf leaf = { index };
				if (!fn(leaf, search_aabb, udatas[index], fn_udata)) {
					return;
				}
			} else {
				apush(index_stack, node->index_a);
				apush(index_stack, node->index_b);
			}
		}
	}
}

float cf_aabb_tree_cost(const CF_AabbTree tree_handle)
{
	CF_AabbTreeInternal* tree = (CF_AabbTreeInternal*)tree_handle.id;
	return s_tree_cost(tree, tree->root);
}

void cf_aabb_tree_validate(const CF_AabbTree tree_handle)
{
	CF_AabbTreeInternal* tree = (CF_AabbTreeInternal*)tree_handle.id;
	if (tree->root != AABB_TREE_NULL_NODE_INDEX) {
		s_validate(tree, tree->root, 0);
		s_validate(tree, tree->root);
	}
}

size_t cf_aabb_tree_serialized_size(const CF_AabbTree tree_handle)
{
	CF_AabbTreeInternal* tree = (CF_AabbTreeInternal*)tree_handle.id;
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

bool cf_aabb_tree_serialize(const CF_AabbTree tree_handle, void* buffer, size_t size)
{
	CF_AabbTreeInternal* tree = (CF_AabbTreeInternal*)tree_handle.id;
	size_t needed_size = cf_aabb_tree_serialized_size(tree_handle);
	if (needed_size < size) return false;
	CF_AabbTreeInternal* copy = s_remap(tree);

	uint8_t* p = (uint8_t*)buffer;
	cf_write_fourcc(&p, "aabb");
	cf_write_uint32(&p, (uint32_t)copy->node_count);
	s_serialize_to_buffer(copy, &p, copy->root);
	CF_AabbTree copy_handle;
	copy_handle.id = (uint64_t)copy;
	cf_destroy_aabb_tree(copy_handle);

	size_t size_written = p - (uint8_t*)buffer;
	CF_ASSERT(size_written <= size);
	if (size_written > size) {
		return false;
	}

	return true;
}
