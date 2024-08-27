#include <cute.h>

using namespace Cute;

#define GRID_CELL_CAPACITY 4
typedef int grid_cell_id_t;

struct grid_cell_t
{
	int entities_count;
	grid_cell_id_t entities[GRID_CELL_CAPACITY];
};

void grid_cell_add(grid_cell_t* cell, grid_cell_id_t id)
{
	cell->entities[cell->entities_count] = id;
	if (cell->entities_count < (GRID_CELL_CAPACITY - 1)) {
		cell->entities_count++;
	}
}

void grid_cell_clear(grid_cell_t* cell)
{
	cell->entities_count = 0;
}

// NOTE: This grid assumes all entities have a diameter of 1.0
struct grid_t
{
	int width;
	int height;

	Array<grid_cell_t> cells;
};

void grid_init(grid_t* grid, int width, int height)
{
	grid->width       = width;
	grid->height      = height;
	grid->cells.ensure_count(width * height);
}

int grid_get_index(grid_t* grid, int x, int y)
{
	const int width  = grid->width;
	const int height = grid->height;

	x = (((x % width) + width) % width);
	y = (((y % height) + height) % height);

	return y * width + x;
}

grid_cell_t* grid_at(grid_t* grid, int x, int y)
{
	return grid->cells + grid_get_index(grid, x, y);
}

grid_cell_t* grid_at(grid_t* grid, v2 pos)
{
	return grid_at(grid, (int)pos.x, (int)pos.y);
}

void grid_add(grid_t* grid, int x, int y, grid_cell_id_t id)
{
	grid_cell_add(grid_at(grid, x, y), id);
}

void grid_add(grid_t* grid, v2 pos, grid_cell_id_t id)
{
	grid_cell_add(grid_at(grid, pos), id);
}

void grid_clear(grid_t* grid)
{
	for (int i = 0; i < grid->cells.count(); i++) {
		grid_cell_clear(grid->cells + i);
	}
}

struct particle_t
{
	v2 prev_pos;
	v2 curr_pos;
	float r;
	CF_Color color;
};

#define PHYSICS_SUBSTEPS 8
struct physics_t
{
	int width;
	int height;
	grid_t grid;
	Array<particle_t> particles;

	CF_Threadpool* tp;
};

void physics_init(physics_t* physics, int width, int height, CF_Threadpool* tp)
{
	physics->width     = width;
	physics->height    = height;
	grid_init(&physics->grid, width, height);
	physics->particles = {};
	physics->tp        = tp;
}

void physics_update_grid(physics_t* physics)
{
	grid_t* grid = &physics->grid;

	grid_clear(grid);
	for (int i = 0; i < physics->particles.count(); i++) {
		v2 pos = physics->particles[i].curr_pos;
		grid_add(grid, pos, i);
	}
}

void physics_solve_collisions_entity(physics_t* physics, grid_cell_id_t id, int cell_index)
{
	grid_t* grid = &physics->grid;

	if (cell_index < 0 || cell_index >= grid->cells.count()) {
		return;
	}

	grid_cell_t* cell = grid->cells + cell_index;

	particle_t* a = physics->particles + id;
	for (int i = 0; i < cell->entities_count; i++) {
		particle_t* b = physics->particles + cell->entities[i];
		if (a == b) {
			continue;
		}

		v2* a_pos = &a->curr_pos;
		v2* b_pos = &b->curr_pos;

		v2 d = cf_sub_v2(*b_pos, *a_pos);

		float d2 = cf_dot(d, d);
		float r  = a->r + b->r;
		if (d2 < r * r) {
			float l = CF_SQRTF(d2);
			v2 n = l != 0 ? cf_mul_v2_f(d, 1.0f / l) : v2{ 0.0f, 1.0f };

			float depth = r - l;
			v2 response = cf_mul_v2_f(n, depth * 0.5f);

			*a_pos = cf_sub_v2(*a_pos, response);
			*b_pos = cf_add_v2(*b_pos, response);
		}
	}
}

struct physics_solve_task_params_t
{
	int start, end;
	physics_t* physics;
};

void physics_solve_task(void* udata)
{
	physics_solve_task_params_t* params = (physics_solve_task_params_t*)udata;
	const int end = params->end;

	physics_t* p = params->physics;
	grid_t* g    = &p->grid;

	const int width = g->width;

	for (int cell_index = params->start; cell_index < end; cell_index++) {
		grid_cell_t* cell = g->cells + cell_index;
		for (int i = 0; i < cell->entities_count; i++) {
			grid_cell_id_t id = cell->entities[i];
			// Check 2 rows worth of cells.
			physics_solve_collisions_entity(p, id, cell_index - 1);
			physics_solve_collisions_entity(p, id, cell_index);
			physics_solve_collisions_entity(p, id, cell_index + 1);

			// It's important to subtract here to remain deterministic,
			// because we are incrementing when queueing tasks.
			physics_solve_collisions_entity(p, id, cell_index - width - 1);
			physics_solve_collisions_entity(p, id, cell_index - width);
			physics_solve_collisions_entity(p, id, cell_index - width + 1);
		}
	}
}

void physics_solve(physics_t* physics)
{
	grid_t* grid = &physics->grid;
	CF_Threadpool* tp = physics->tp;

	const int task_count = cf_core_count();

	const int slice_count = task_count * 2;
	const int slice_size = (int)CF_CEILF(grid->height / (float)slice_count) * grid->width;

	static physics_solve_task_params_t params[64] = {};
	CF_ASSERT(task_count < 64);

	for (int pass = 0; pass < 2; pass++) {
		int i = 0, start, end;
		for (;;) {
			start = (2 * i + pass) * slice_size;
			end   = cf_min(start + slice_size, (int)grid->cells.count());
			if (start >= end) {
				break;
			}
			params[i] = {
					.start   = start,
					.end     = end,
					.physics = physics,
			};
			cf_threadpool_add_task(tp, physics_solve_task, params + i);
			++i;
		}
		cf_threadpool_kick_and_wait(tp);
	}
}

struct physics_update_entities_task_params_t
{
	int start, end;
	physics_t* physics;
	float dt;
};

void physics_update_entities_task(void* udata)
{
	physics_update_entities_task_params_t* params = (physics_update_entities_task_params_t*)udata;
	physics_t* physics = params->physics;

	v2 gravity   = { 0.f, -50.f };
	float margin = 2.0f;

	for (int i = params->start; i < params->end; i++) {
		particle_t* p = physics->particles + i;

		const v2 vel            = (p->curr_pos - p->prev_pos);
		const float damping = 1.0 - 0.0005f;

		const v2 new_position = p->curr_pos + (vel + (gravity * params->dt * params->dt)) * damping;

		p->prev_pos = p->curr_pos;
		p->curr_pos  = new_position;

		p->curr_pos = cf_clamp_v2(p->curr_pos, {margin, margin}, {physics->width - margin, physics->height - margin});
	}
}

void physics_update_entities(physics_t* physics, float dt)
{
	const int task_count = cf_core_count();
	const int slice_size = cf_max((int)CF_CEILF(physics->particles.count() / (float)task_count), 1000);

	static physics_update_entities_task_params_t params[64] = {};
	CF_ASSERT(task_count <= 64);

	for (int i = 0; i < task_count; i++) {
		const int start = slice_size * i;
		const int end   = cf_min(start + slice_size, physics->particles.count());
		if (start >= end) {
			break;
		}
		params[i] = {
				.start = start,
				.end = end,
				.physics = physics,
				.dt = dt
		};

		cf_threadpool_add_task(physics->tp, physics_update_entities_task, params + i);
	}
	cf_threadpool_kick_and_wait(physics->tp);
}

void physics_update(physics_t* physics, float dt)
{
	const float sub_dt = dt / (float)PHYSICS_SUBSTEPS;
	for (int i = 0; i < PHYSICS_SUBSTEPS; i++) {
		physics_update_grid(physics);
		physics_solve(physics);
		physics_update_entities(physics, sub_dt);
	}
}

CF_Color get_rainbow(float t)
{
	const float r = CF_SINF(t);
	const float g = CF_SINF(t + 0.33f * 2.0f * CF_PI);
	const float b = CF_SINF(t + 0.66f * 2.0f * CF_PI);
	return cf_make_color_rgb_f(r * r, g * g, b * b);
}

void update(void* udata)
{
	physics_t* physics = (physics_t*)udata;

	// Add particles every frame
	int particles_per_frame = 10;
	int max_particles = physics->width * physics->height;
	if (physics->particles.count() < max_particles) {
		for (int i = 0; i < particles_per_frame; i++) {
			v2 vel = {1.5f / (float)PHYSICS_SUBSTEPS, 0.f};

			particle_t p = {};
			p.curr_pos = {2.f, physics->height - 2.f - 1.5f * i};
			p.prev_pos = p.curr_pos - vel;
			p.r = 0.5f;
			p.color = get_rainbow(physics->particles.count() * 0.001f);

			physics->particles.add(p);
		}
	}

	physics_update(physics, CF_DELTA_TIME_FIXED);
}

int main(int argc, char* argv[])
{
	int w = 150, h = 150;
	float scale = 5;

	int options = 0;
	//options = APP_OPTIONS_GFX_VULKAN_BIT;
	//options = APP_OPTIONS_GFX_D3D11_BIT;
	//options = APP_OPTIONS_GFX_D3D12_BIT;
	make_app("Liquid Rainbow", 0, 0, 0, (int)(w * scale), (int)(h * scale), options | APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	cf_set_fixed_timestep(60);
	cf_set_target_framerate(60);
	cf_app_set_vsync(false);

	CF_Threadpool* tp = cf_make_threadpool(cf_core_count());
	physics_t physics = {};
	physics_init(&physics, w, h, tp);
	cf_set_update_udata(&physics);

	while (cf_app_is_running()) {
		cf_app_update(update);

		// Render
		cf_app_get_size(&w, &h);

		cf_draw_push();
		cf_draw_scale(scale, scale);
		cf_draw_translate(-physics.width / 2.f, -physics.height / 2.f);

		cf_draw_push_color(cf_make_color_hex(0xFFFFFF));
		CF_Aabb bounds = cf_make_aabb(v2{ 0.f, 0.f }, v2{(float)physics.width, (float)physics.height});
		cf_draw_box(bounds, 1.f, 0.f);
		cf_draw_pop_color();

		for (int i = 0; i < physics.particles.count(); i++) {
			particle_t p = physics.particles[i];
			CF_Color color = p.color;
			cf_draw_push_color(color);

			v2 pos = p.curr_pos;
			CF_Aabb rect = cf_make_aabb(pos - v2{ 0.5f, 0.5f }, pos + v2{ 0.5f, 0.5f });
			cf_draw_box_fill(rect, 0.0f);
			cf_draw_pop_color();
		}

		cf_draw_pop();
		cf_app_draw_onto_screen(true);
	}

	cf_destroy_app();
	return 0;
}
