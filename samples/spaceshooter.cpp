#include <cute.h>
using namespace Cute;

inline float fade(float t, float lo, float hi) { return remap(1.0f - smoothstep(t), lo, hi); }

#define ROCKETS_MAX 16

struct RocketBarn
{
	int count;
	int capacity;
	bool alive[ROCKETS_MAX];
	float elapsed[ROCKETS_MAX];
	float hang_time[ROCKETS_MAX];
	Routine rt[ROCKETS_MAX];
	v2 p[ROCKETS_MAX];
	v2 c0[ROCKETS_MAX];
	v2 c1[ROCKETS_MAX];
	v2 target[ROCKETS_MAX];

	float prev_dt;
	int hit_count;
	v2 hits[ROCKETS_MAX];

	bool add(v2 start, v2 end, float duration);
	void update();
	void draw();
	bool try_pop_hit(v2* hit_out);
};

#define BULLETS_MAX 16
#define BULLETS_RADIUS 4

struct BulletBarn
{
	bool alive[BULLETS_MAX];
	v2 p[BULLETS_MAX];

	bool add();
	void hit(int i);
	void update();
	void draw();
};

#define ASTEROIDS_MAX 256

struct AsteroidBarn
{
	bool alive[ASTEROIDS_MAX];
	float angular_velocity[ASTEROIDS_MAX];
	v2 velocity[ASTEROIDS_MAX];
	CF_Poly poly[ASTEROIDS_MAX];
	v2 center_mass[ASTEROIDS_MAX];
	float slice_timeout[ASTEROIDS_MAX];

	void add(v2 p, v2 v, float size);
	void add(CF_Poly p, v2 v, float a, float timeout);
	void update();
	void slice(CF_Ray r);
	void draw();
	void hit(v2 hit, float radius);
	void explode(int i);
};

#define TRAIL_MAX 128

// Used for trails of rockets.
struct TrailBarn
{
	bool alive[TRAIL_MAX];
	float lifespan[TRAIL_MAX];
	float time_left[TRAIL_MAX];
	CF_Aabb shape[TRAIL_MAX];

	void add(CF_Aabb box, float duration);
	void update();
	void draw();
};

struct PlayerShip
{
	v2 p;
	CF_Aabb bounds;
	Routine rt_movement;
	Routine rt_weapons;
	Routine rt_trail;
	bool charging_laser;
	int charging_shot;
	bool fired_laser;
	bool firing_rockets;
	bool shielding;
	float shield_time;
	RocketBarn rockets;
	BulletBarn bullets;
	CF_Ray laser_trail;
	Sprite sprite;
	Sprite charge_sprite;
	int facing_index;
	float booster_time;
	float turn_time;
	v2 dir;
	v2 old_dir;
	int hp;
	bool hurt;
	float iframes;

	void reset();
};

// Stores a single sprite and plays it one time. Once played, the sprite the sprite gets removed.
struct AnimationBarn
{
	inline void add(Sprite sprite) { sprites.add(sprite); }
	inline void update()
	{
		for (int i = 0; i < sprites.count(); ++i) {
			sprites[i].update();
			if (sprites[i].loop_count) {
				sprites.unordered_remove(i);
				--i;
			}
		}
	}
	inline void draw()
	{
		for (int i = 0; i < sprites.count(); ++i) {
			sprites[i].draw();
		}
	}
	Array<Sprite> sprites;
};

// Draws a sprite fading over a time. Does not animate the sprite, but stays locked on that frame.
struct FadeBarn
{
	inline void add(Sprite sprite, float time = 0.5f) { times_original.add(time); times.add(time); sprites.add(sprite); }
	inline void update()
	{
		for (int i = 0; i < times.count(); ++i) {

			times[i] -= DELTA_TIME;
			if (times[i] <= 0) {
				times_original.unordered_remove(i);
				times.unordered_remove(i);
				sprites.unordered_remove(i);
				--i;
			}
		}
	}
	inline void draw()
	{
		for (int i = 0; i < sprites.count(); ++i) {
			float opacity = fade(1.0f - times[i] / times_original[i], 0.0f, 1.0f);
			sprites[i].opacity = opacity;
			sprites[i].draw();
		}
	}
	Array<float> times_original;
	Array<float> times;
	Array<Sprite> sprites;
};

// Animates circles with a velocity, fading out over time.
struct ParticleBarn
{
	struct Particle
	{
		Circle c;
		v2 v;
		float t0;
		float t;
	};
	inline void add(v2 at, float r, v2 v, float lifetime) { Particle p; p.c = make_circle(at, r); p.v = v; p.t0 = p.t = lifetime; particles.add(p); }
	inline void update_and_draw()
	{
		for (int i = 0; i < particles.count(); ++i) {
			particles[i].t -= DELTA_TIME;
			if (particles[i].t <= 0) {
				particles.unordered_remove(i);
				--i;
				continue;
			}
			particles[i].c.p += particles[i].v * DELTA_TIME;
			float opacity = particles[i].t / particles[i].t0;
			draw_push_color(color_white() * opacity);
			draw_circle_fill(particles[i].c);
		}
	}
	Array<Particle> particles;
};

struct ChargeShot
{
	Routine rt;
	Sprite shot_spawn;
	Sprite shot;
	v2 at;
	bool start = false;
	bool alive = false;
	inline void spawn(v2 at) { rt = { }; this->at = at; start = true; }
	void hit();
	void update_and_draw();
	#define CHARGE_SHOT_RADIUS 12
};

struct LineParticleBarn
{
	struct Particle
	{
		v2 a, b;
		v2 v;
		float spin;
		float t0, t;
	};
	inline void add(v2 a, v2 b, v2 v, float spin, float lifetime)
	{
		Particle p;
		p.a = a;
		p.b = b;
		p.v = v;
		p.spin = spin;
		p.t = p.t0 = lifetime;
		particles.add(p);
	}
	inline void add(Poly poly, v2 v, float spin, float lifetime)
	{
		v2 com = center_of_mass(poly);
		for (int i = 0; i < poly.count; ++i) {
			int j = i + 1 == poly.count ? 0 : i + 1;
			v2 a = poly.verts[i];
			v2 b = poly.verts[j];
			v2 c = (a + b) * 0.5f;
			v2 r = norm(c - com) * 100.0f;
			add(poly.verts[i], poly.verts[j], v + r, spin, lifetime);
		}
	}
	inline void update_and_draw()
	{
		for (int i = 0; i < particles.count(); ++i) {
			Particle* p = particles + i;
			p->t -= DELTA_TIME;
			if (p->t <= 0) {
				particles.unordered_remove(i);
				--i;
				continue;
			}
			p->a += p->v * DELTA_TIME;
			p->b += p->v * DELTA_TIME;
			v2 c = (p->a + p->b) * 0.5f;
			v2 a = p->a - c;
			v2 b = p->b - c;
			SinCos r = sincos(p->spin * DELTA_TIME);
			a = mul(r, a);
			b = mul(r, b);
			p->a = a + c;
			p->b = b + c;
			float opacity = p->t / p->t0;
			draw_push_color(color_white() * opacity);
			draw_line(p->a, p->b);
		}
	}
	Array<Particle> particles;
};

struct Game
{
	CF_RndState rnd;
	PlayerShip player;
	AsteroidBarn asteroids;
	TrailBarn trails;
	AnimationBarn animations;
	FadeBarn fades;
	ChargeShot charge_shot;
	ParticleBarn particles;
	LineParticleBarn line_particles;

	// Boss stuff.
	bool boss_hurt;
	bool boss_hurt_small;
	bool boss1;
	int boss_state;
	Routine rt;
	Routine rt_hurt;
	Routine rt_hurt_small;
	v2 boss_p, boss_last_p;
};

Game* g;

float rnd_range(float lo, float hi) { return rnd_range(g->rnd, lo, hi); }
v2 rnd_range(v2 lo, v2 hi) { return V2(rnd_range(g->rnd, lo.x, hi.x), rnd_range(g->rnd, lo.y, hi.y)); }

void TrailBarn::add(CF_Aabb box, float duration)
{
	for (int i = 0; i < TRAIL_MAX; ++i) {
		if (alive[i]) continue;
		alive[i] = true;
		shape[i] = box;
		lifespan[i] = duration;
		time_left[i] = duration;
		break;
	}
}

void TrailBarn::update()
{
	for (int i = 0; i < TRAIL_MAX; ++i) {
		if (!alive[i]) continue;
		time_left[i] -= DELTA_TIME;
		if (time_left[i] < 0) alive[i] = false;
	}
}

void TrailBarn::draw()
{
	for (int i = 0; i < TRAIL_MAX; ++i) {
		if (!alive[i]) continue;
		draw_push_color(color_white() * fade(1.0f - (time_left[i] / lifespan[i]), 0.4f, 1.0f));
		draw_box(shape[i]);
	}
}

const float player_shield_max = 3.0f;

void PlayerShip::reset()
{
	memset(this, 0, sizeof(*this));
	rockets.capacity = 5;
	sprite = make_sprite("ship.ase");
	charge_sprite = make_sprite("charge.ase");
	facing_index = 3;
	hp = 3;
}

bool RocketBarn::add(v2 start, v2 end, float duration)
{
	if (count >= capacity) return false;
	for (int i = 0; i < ROCKETS_MAX; ++i) {
		if (!alive[i]) {
			alive[i] = true;
			hang_time[i] = duration;
			elapsed[i] = 0;
			rt[i] = Routine();
			v2 d = norm(end - start);
			p[i] = start;
			c0[i] = (start - d * 150.0f + skew(d) * rnd_range(25.0f, 250.0f) * sign(rnd_range(-1.0f, 1.0f)));
			c1[i] = rnd_range((start + end) * 0.5f - V2(50,50), (start + end) * 0.5f+ V2(50,50));
			target[i] = end;
			++count;
			return true;
		}
	}
	return false;
}

void RocketBarn::update()
{
	prev_dt = DELTA_TIME;
	hit_count = 0;
	for (int i = 0; i < ROCKETS_MAX; ++i) {
		if (!alive[i]) continue;
		rt_begin(rt[i], DELTA_TIME);
		rt_seconds(hang_time[i])
		{
			elapsed[i] = rt.elapsed;
			if (on_interval(0.015f)) {
				float t = elapsed[i];
				v2 at = bezier(p[i], c0[i], c1[i], target[i], sin_in(t));
				g->trails.add(make_aabb(at - V2(1,1), at + V2(1,1)), 0.15f);
			}
		}
		rt_once()
		{
			alive[i] = false;
			Sprite s = make_sprite("explosion.ase");
			s.transform.p = target[i];
			g->animations.add(s);
			hits[hit_count++] = target[i];
		}
		rt_end();
	}
}

void RocketBarn::draw()
{
	for (int i = 0; i < ROCKETS_MAX; ++i) {
		if (!alive[i]) continue;
		float t0 = (elapsed[i] - prev_dt * 2 / hang_time[i]);
		float t1 = (elapsed[i] - prev_dt / hang_time[i]);
		float t = elapsed[i];
		v2 at0 = bezier(p[i], c0[i], c1[i], target[i], sin_in(t0));
		v2 at1 = bezier(p[i], c0[i], c1[i], target[i], sin_in(t1));
		v2 at = bezier(p[i], c0[i], c1[i], target[i], sin_in(t));
		draw_push_color(color_white());
		draw_circle_fill(make_circle(at, 3));
		draw_push_color(color_white() * 0.75f);
		draw_circle_fill(make_circle(at1, 4));
		draw_push_color(color_white() * 0.5f);
		draw_circle_fill(make_circle(at0, 4));
	}
}

bool RocketBarn::try_pop_hit(v2* hit_out)
{
	if (hit_count) {
		*hit_out = hits[--hit_count];
		return true;
	} else {
		return false;
	}
}

bool BulletBarn::add()
{
	for (int i = 0; i < BULLETS_MAX; ++i) {
		if (!alive[i]) {
			alive[i] = true;
			p[i] = top(g->player.bounds) + V2(0, 8);
			return true;
		}
	}
	return false;
}

void BulletBarn::hit(int i)
{
	Sprite s = make_sprite("bullet_pop.ase");
	s.transform.p = p[i];
	g->animations.add(s);
	alive[i] = false;
	g->particles.add(p[i], 2, V2(-200,0), 0.25f);
	g->particles.add(p[i], 2, V2( 200,0), 0.25f);
}

void BulletBarn::update()
{
	for (int i = 0; i < BULLETS_MAX; ++i) {
		if (!alive[i]) continue;
		p[i] += V2(0,1) * 300.0f * DELTA_TIME;
		if (p[i].y > 240.0f + 10.0f) {
			alive[i] = false;
		} else {
			for (int j = 0; j < ASTEROIDS_MAX; ++j) {
				if (!g->asteroids.alive[j]) continue;
				if (circle_to_poly(make_circle(p[i], BULLETS_RADIUS), g->asteroids.poly + j, NULL)) {
					hit(i);
				}
			}
		}
	}
}

void BulletBarn::draw()
{
	for (int i = 0; i < BULLETS_MAX; ++i) {
		if (!alive[i]) continue;
		v2 e = V2(2.0f, 5.0);
		draw_push_color(color_white());
		draw_box(make_aabb(p[i] - e, p[i] + e));
	}
}

inline float flicker(float interval, float lo, float hi)
{
	if (between_interval(interval)) return lo;
	else return hi;
}

CF_Poly make_asteroid_poly(v2 p0, float size)
{
	CF_Poly poly;
	for (int i = 0; i < CF_POLY_MAX_VERTS; ++i) {
		poly.verts[i] = rnd_range(V2(-size,-size), V2(size,size));
		poly.verts[i] += p0;
	}
	poly.count = CF_POLY_MAX_VERTS;
	cf_make_poly(&poly);
	return poly;
}

void AsteroidBarn::add(v2 p, v2 v, float size)
{
	for (int i = 0; i < ASTEROIDS_MAX; ++i) {
		if (alive[i]) continue;
		alive[i] = true;
		angular_velocity[i] = rnd_range(0.2f, 1.0f) * sign(rnd_range(-1,1));
		velocity[i] = v;
		poly[i] = make_asteroid_poly(p, size);
		center_mass[i] = center_of_mass(poly[i]);
		slice_timeout[i] = 0;
		return;
	}
}

void AsteroidBarn::add(CF_Poly p, v2 v, float a, float timeout)
{
	for (int i = 0; i < ASTEROIDS_MAX; ++i) {
		if (alive[i]) continue;
		alive[i] = true;
		angular_velocity[i] = a;
		velocity[i] = v;
		poly[i] = p;
		center_mass[i] = center_of_mass(poly[i]);
		slice_timeout[i] = timeout;
		return;
	}
}

void AsteroidBarn::update()
{
	for (int i = 0; i < ASTEROIDS_MAX; ++i) {
		if (!alive[i]) continue;

		// Delete tiny asteroids.
		if (calc_area(poly[i]) < 100.0f) {
			alive[i] = false;
			explode(i);
			continue;
		}

		// Integrate positions.
		v2 delta = velocity[i] * DELTA_TIME;
		for (int j = 0; j < poly[i].count; ++j) {
			poly[i].verts[j] += delta;
		}
		center_mass[i] += delta;

		// Rotate about center of mass.
		// Integrate orientation.
		float angular_delta = angular_velocity[i] * DELTA_TIME;
		CF_SinCos r = sincos(angular_delta);
		v2 c = center_mass[i];
		for (int j = 0; j < poly[i].count; ++j) {
			poly[i].verts[j] = mul(r, poly[i].verts[j] - c) + c;
		}
		norms(poly[i].verts, poly[i].norms, poly[i].count);

		if (slice_timeout[i] > 0) {
			slice_timeout[i] = max(0.0f, slice_timeout[i] - DELTA_TIME);
		}
	}
}

void AsteroidBarn::slice(CF_Ray r)
{
	CF_Halfspace h = plane(skew(r.d), r.p);
	for (int i = 0; i < ASTEROIDS_MAX; ++i) {
		if (!alive[i]) continue;
		if (slice_timeout[i] > 0) continue;
		if (!ray_to_poly(r, &poly[i]).hit) continue;
		SliceOutput sho = Cute::slice(h, poly[i]);
		if (!sho.front.count | !sho.back.count) continue;
		v2 c = center_mass[i];
		v2 v = velocity[i];
		float a = angular_velocity[i] * 2;
		v2 v_front = v + V2(-50.0f, 0);
		v2 v_back = v + V2(50.0f, 0);
		alive[i] = false;
		add(sho.front, v_front, a < 0 ? -a : a, 1.0f);
		add(sho.back, v_back, a < 0 ? a : -a, 1.0f);
	}
}

void AsteroidBarn::draw()
{
	draw_push_color(color_white());
	for (int i = 0; i < ASTEROIDS_MAX; ++i) {
		if (!alive[i]) continue;
		draw_polyline(poly[i].verts, poly[i].count, 1.0f, true);
	}
}

void AsteroidBarn::hit(v2 hit, float radius)
{
	Circle c = make_circle(hit, radius);
	for (int i = 0; i < ASTEROIDS_MAX; ++i) {
		if (!alive[i]) continue;
		if (circle_to_poly(c, poly + i, NULL)) {
			alive[i] = false;
			explode(i);
		}
	}
}

void AsteroidBarn::explode(int i)
{
	g->line_particles.add(poly[i], velocity[i], angular_velocity[i] * 3.0f, rnd_range(0.5f, 1.0f));
}

void player_movement_routine()
{
	g->player.iframes -= DELTA_TIME;
	rt_begin(g->player.rt_movement, DELTA_TIME)
	{
		if (g->player.fired_laser) {
			nav_goto("knockback");
		}

		if (g->player.iframes <= 0) {
			// Detect running into asteroids.
			for (int i = 0; i < ASTEROIDS_MAX; ++i) {
				if (!g->asteroids.alive[i]) continue;
				Circle c = make_circle(g->player.sprite.transform.p, 12.5f);
				if (circle_to_poly(c, g->asteroids.poly + i, NULL)) {
					nav_goto("hurt");
				}
			}
		}

		v2 dir = V2(0,0);
		if (key_down(KEY_LEFT)) {
			dir += V2(-1,0);
		}
		if (key_down(KEY_UP)) {
			dir += V2(0,1);
		}
		if (key_down(KEY_RIGHT)) {
			dir += V2(1,0);
		}
		if (key_down(KEY_DOWN)) {
			dir += V2(0,-1);
		}
		g->player.old_dir = g->player.dir;
		g->player.dir = dir;

		// Ship turning.
		// The ships images are stored in a sprite, but we don't call Sprite::update() on it.
		// Instead we track the frame index manually and increment ourselves in the direction
		// the player is pressing.
		// We store two sets of the player's ship animation and go toggle them frequently to
		// implement the flame booster effect.
		g->player.turn_time += DELTA_TIME;
		if (g->player.turn_time > 0.05f) {
			g->player.turn_time = 0;
			int target;
			if (g->player.dir.x == 0) {
				target = g->player.facing_index < 7 ? 3 : 10;
			} else if (g->player.dir.x > 0) {
				target = g->player.facing_index < 7 ? 6 : 13;
			} else {
				target = g->player.facing_index < 7 ? 0 : 7;
			}
			int diff = target - g->player.facing_index;
			int target_sign = diff != 0 ? sign(diff) : 0;
			g->player.facing_index = min(13, max(0, g->player.facing_index + target_sign));
			g->player.sprite.frame_index = g->player.facing_index;
		}

		bool go_slow = g->player.charging_laser | g->player.shielding | g->player.firing_rockets;
		g->player.p += safe_norm(dir) * (go_slow ? 100.0f : 200.0f) * DELTA_TIME;
		nav_restart();
	}

	rt_label("knockback")
	{
		v2 offset = V2(0,-15);
		int n = 5;
		Sprite s = g->player.sprite;
		v2 p = s.transform.p;
		for (int i = 0; i < n; ++i) {
			float t = (float)i / (float)n;
			s.transform.p = p + cf_lerp_v2(V2(0,0), offset, t);
			g->fades.add(s, t*0.5f);
		}
		g->player.p += offset;
	}
	rt_wait(0.25f)
	{
		nav_restart();
	}

	rt_label("hurt")
	{
		g->player.hurt = true;
		g->player.hp--;
	}
	rt_wait(0.25f)
	{
		g->player.hurt = false;
		g->player.iframes = 2.0f;
		nav_restart();
	}

	rt_end();

	g->player.bounds = make_aabb(g->player.p, 10, 20);

	// Toggle back and forth between two sets of animation frames to implement the flame
	// booster effect for the back of the player's ship.
	g->player.booster_time += DELTA_TIME;
	if (g->player.booster_time > 0.025f) {
		g->player.booster_time = 0;
		if (g->player.facing_index < 7) {
			g->player.facing_index += 7;
		} else {
			g->player.facing_index -= 7;
		}
		g->player.sprite.frame_index = g->player.facing_index;
	}
}

void player_weapons_routine()
{
	rt_begin(g->player.rt_weapons, DELTA_TIME)
	{
		if (key_down(KEY_X)) {
			nav_goto("laser");
		} else if (key_just_pressed(KEY_Z)) {
			nav_goto("bullet");
		} else if (key_just_pressed(KEY_C)) {
			nav_goto("rocket");
		} else if (key_just_pressed(KEY_SPACE)) {
			nav_goto("shield");
		} else {
			nav_restart();
		}
	}

	rt_label("laser")
	{
		g->player.charging_laser = true;
	}
	// Charging laser.
	rt_seconds(1.0f)
	{
		if (key_down(KEY_X)) {
			v2 p = top(g->player.bounds) + V2(0, 10.0f);
			draw_circle(make_circle(p, cf_lerp(3.0f, 10.0f, rt.elapsed)));
		} else {
			g->player.charging_laser = false;
			nav_restart();
		}
	}
	// Fire laser.
	rt_once()
	{
		v2 p = top(g->player.bounds) + V2(0, 10.0f);
		CF_Ray r = make_ray(p + V2(0,8), V2(0,1), 500);
		g->asteroids.slice(r);
		g->player.laser_trail = r;
		g->player.fired_laser = true;
	}
	// Laser Fade.
	rt_seconds(0.35f)
	{
		g->player.fired_laser = false;
		g->player.charging_laser = false;
		CF_Ray r = g->player.laser_trail;
		float alpha = fade(rt.elapsed, 0.4f, 1.0f);
		draw_push_color(color_white() * alpha);
		v2 p = r.p - V2(0,3);
		draw_line(p, endpoint(r));
		draw_circle_fill(make_circle(r.p - V2(0,9), 5));
	}
	rt_once()
	{
		nav_restart();
	}

	rt_label("bullet")
	{
		g->player.bullets.add();
	}
	rt_seconds(0.5f)
	{
		if (!key_down(KEY_Z)) {
			nav_restart();
		}
	}
	rt_seconds(1.25f)
	{
		g->player.charging_shot = 1;
		if (!key_down(KEY_Z)) {
			g->player.charging_shot = 0;
			g->player.bullets.add();
			nav_goto("bullet_cooldown");
		}
	}
	rt_while(key_down(KEY_Z))
	{
		g->player.charging_shot = 2;
	}
	rt_once()
	{
		// Do charge shot.
		g->player.charging_shot = 0;
		g->charge_shot.spawn(g->player.sprite.transform.p + V2(0,18));
		g->player.rt_movement.set_next("knockback");
		g->particles.add(g->player.sprite.transform.p + V2(0, 16), 3, V2(-200,0), 0.3f);
		g->particles.add(g->player.sprite.transform.p + V2(0, 16), 3, V2( 200,0), 0.3f);
		nav_restart();
	}

	rt_label("bullet_cooldown") { }
	rt_wait(0.05f)
	{
		nav_restart();
	}

	rt_label("rocket")
	{
		g->player.firing_rockets = true;
	}
	rt_wait(0.1f)
	{
		v2 target = V2(g->player.p.x, 200) + rnd_range(V2(-40,-40), V2(20,20));
		g->player.rockets.add(g->player.p, target, 1.5f);
		if (g->player.rockets.count < g->player.rockets.capacity) {
			nav_goto("rocket");
		}
		g->player.firing_rockets = false;
	}
	rt_wait(1.0f)
	{
		nav_restart();
	}

	rt_label("shield")
	{
		g->player.shielding = true;
		g->player.shield_time += DELTA_TIME;
		if (key_down(KEY_SPACE) && g->player.shield_time < player_shield_max) {
			// Collide with asteroids.
			for (int i = 0; i < ASTEROIDS_MAX; ++i) {
				if (!g->asteroids.alive[i]) continue;
				if (circle_to_poly(make_circle(g->player.sprite.transform.p, 25), g->asteroids.poly + i, NULL)) {
					g->asteroids.alive[i] = false;
					g->asteroids.explode(i);
				}
			}
			nav_redo();
		}
		g->player.shield_time = 0;
		g->player.shielding = false;
	}
	rt_wait(0.5f) { }
	rt_while(key_down(KEY_SPACE)) { }
	rt_once()
	{
		nav_restart();
	}
	rt_end();
}

void boss1()
{
	#define BOSS1_RADIUS 20
	#define BOSS1_ASTEROID_RADIUS 15

	rt_begin(g->rt, DELTA_TIME) { }
	rt_seconds(2)
	{
		g->boss1 = true;
		g->boss_p = cf_lerp_v2(V2(0,270), V2(0,180), smoothstep(rt.elapsed));
	}
	rt_wait(1)
	{
		nav_goto("asteroid");
	}

	rt_label("main")
	{
		g->boss_last_p = g->boss_p;
	}
	rt_wait(3)
	{
		if (g->boss_state == 0) {
			g->boss_state++;
			nav_goto("left");
		} else if (g->boss_state == 1) {
			g->boss_state++;
			nav_goto("right");
		} else {
			g->boss_state = 0;
			nav_goto("center");
		}
	}

	rt_label("left") { }
	rt_seconds(2.5f)
	{
		g->boss_p = bezier(g->boss_last_p, V2(-80, 50), V2(-180, 200), V2(-200,160), smoothstep(rt.elapsed));
	}
	rt_once()
	{
		nav_goto("asteroid");
	}

	rt_label("right") { }
	rt_seconds(2.5f)
	{
		g->boss_p = bezier(g->boss_last_p, V2(80, 50), V2(180, 200), V2(200,160), smoothstep(rt.elapsed));
	}
	rt_once()
	{
		nav_goto("asteroid");
	}

	rt_label("center") { }
	rt_seconds(2.5f)
	{
		g->boss_p = bezier(g->boss_last_p, V2(100, 100), V2(50, 260), V2(0,180), smoothstep(rt.elapsed));
	}
	rt_once()
	{
		nav_goto("asteroid");
	}

	rt_label("asteroid") { }
	rt_seconds(1.0f)
	{
		float t = smoothstep(rt.elapsed);
		draw_push_color(color_white() * t);
		draw_circle_fill(g->boss_p - V2(0,35), t * BOSS1_ASTEROID_RADIUS);
		draw_pop_color();
	}
	rt_once()
	{
		draw_circle_fill(g->boss_p - V2(0,35), BOSS1_ASTEROID_RADIUS);
		g->asteroids.add(g->boss_p - V2(0,35), -V2(0,30), BOSS1_ASTEROID_RADIUS);
	}
	rt_seconds(0.35f)
	{
		float t = smoothstep(1.0f - rt.elapsed);
		draw_push_color(color_white() * t);
		draw_circle_fill(g->boss_p - V2(0,35), t * BOSS1_ASTEROID_RADIUS);
		draw_pop_color();
	}
	rt_once()
	{
		nav_goto("main");
	}

	rt_end();
}

void boss_hurt()
{
	rt_begin(g->rt_hurt, DELTA_TIME) { }
	rt_wait(1)
	{
		g->rt.set_next("main");
		g->boss_hurt = false;
		nav_restart();
	}
	rt_end() { }
}

void boss_hurt_small()
{
	rt_begin(g->rt_hurt_small, DELTA_TIME) { }
	rt_wait(0.1f)
	{
		g->boss_hurt_small = false;
		nav_restart();
	}
	rt_end();
}

void update_bosses()
{
	if (g->boss1) {
		if (g->charge_shot.alive && !g->boss_hurt) {
			if (circle_to_circle(make_circle(g->charge_shot.at, CHARGE_SHOT_RADIUS), make_circle(g->boss_p, BOSS1_RADIUS))) {
				g->boss_hurt = true;
				g->charge_shot.hit();
				g->boss_p -= V2(0,-5);
				g->boss_last_p -= V2(0,-5);
			}
		}

		if (g->boss_hurt) {
			boss_hurt();
		} else {
			boss1();
		}

		if (g->boss_hurt_small) {
			boss_hurt_small();
		}

		for (int i = 0; i < BULLETS_MAX; ++i) {
			if (!g->player.bullets.alive[i]) continue;
			v2 p = g->player.bullets.p[i];
			if (circle_to_circle(make_circle(g->boss_p, BOSS1_RADIUS), make_circle(p, BULLETS_RADIUS))) {
				g->boss_hurt_small = true;
				g->player.bullets.hit(i);
				g->rt_hurt_small.reset();
			}
		}
	}
}

void draw_bosses()
{
	if (g->boss1) {
		if (!g->boss_hurt) {
			if (g->boss_hurt_small) {
				if (between_interval(0.05f)) draw_push_color(color_red());
				else draw_push_color(color_white());
			}
			draw_circle(make_circle(g->boss_p, BOSS1_RADIUS));
			if (g->boss_hurt_small) draw_pop_color();
			draw_line(g->boss_p + V2(-12,4), g->boss_p + V2(-5,4));
			draw_line(g->boss_p + V2(5,4), g->boss_p + V2(12,4));
		} else {
			if (between_interval(0.05f)) {
				draw_push_color(color_red());
			} else {
				draw_push_color(color_white());
			}
			draw_circle(make_circle(g->boss_p, BOSS1_RADIUS));
			draw_circle(g->boss_p + (V2(-12,4)+V2(-5,4)) * 0.5f, 4);
			draw_circle(g->boss_p + (V2( 5, 4)+V2(12,4)) * 0.5f, 4);
			draw_pop_color();
		}
	}
}

void ChargeShot::hit()
{
	Sprite s = make_sprite("explosion.ase");
	s.transform.p = shot.transform.p;
	g->animations.add(s);
	for (int i = 0; i < 10; ++i) {
		SinCos r = sincos(rnd_range(-CF_PI*0.25f, CF_PI*0.25f));
		g->particles.add(shot.transform.p, 3.0f, mul(r,V2(0,rnd_range(50,250))), rnd_range(0.25f, 0.5f));
	}
	alive = false;
	rt.reset();
}

void ChargeShot::update_and_draw()
{
	rt_begin(rt, DELTA_TIME)
	{
		if (!start) {
			nav_redo();
		}
		shot = make_sprite("shot.ase");
		shot.scale = V2(2,2);
		shot_spawn = make_sprite("shot_spawn.ase");
		shot_spawn.transform.p = g->charge_shot.at;
		shot.transform.p = g->charge_shot.at + V2(0,6);
		start = false;
	}
	rt_always()
	{
		shot_spawn.update();
		if (shot_spawn.loop_count) {
			nav_goto("shot");
		}
		shot_spawn.draw();
	}
	rt_label("shot")
	{
		// Draw the charge shot with trails.
		alive = true;
		shot.update();
		shot.draw();
		v2 v = V2(0,400);
		shot.transform.p += v * DELTA_TIME;
		at = shot.transform.p;
		if (on_interval(0.075f)) {
			v2 offset = rnd_range(V2(-5,0), V2(5,0));
			g->particles.add(shot.transform.p + offset, rnd_range(0.75f,3.5f), v * rnd_range(0.35f, 0.6f), rnd_range(0.5f,1.5f));
		}

		// Collide with asteroids.
		for (int i = 0; i < ASTEROIDS_MAX; ++i) {
			if (!g->asteroids.alive[i]) continue;
			if (circle_to_poly(make_circle(shot.transform.p, CHARGE_SHOT_RADIUS), g->asteroids.poly + i, NULL)) {
				g->asteroids.alive[i] = false;
				g->asteroids.explode(i);
				hit();
				nav_restart();
			}
		}

		nav_redo();
	}
	rt_end();
}

void mount_content_directory_as(const char* dir)
{
	Path path = fs_get_base_directory();
	path.normalize();
	path += "/spaceshooter_data";
	fs_mount(path.c_str(), dir);
}

void push_flash(CF_Color color)
{
	draw_push_vertex_attributes(color);
}

CF_Color pop_flash()
{
	return draw_pop_vertex_attributes();
}

int main(int argc, char* argv[])
{
	// Create a window with a resolution of 640 x 480.
	make_app("Space Shooter", 0, 0, 0, 640, 480, APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	cf_shader_directory("/spaceshooter_data");

	mount_content_directory_as("/");
	clear_color(color_black());

	g = (Game*)cf_calloc(sizeof(Game), 1);
	g->rnd = rnd_seed(0);
	g->player.reset();
	g->boss1 = true;

	CF_Shader flash_shader = cf_make_draw_shader("flash.shd");

	while (app_is_running()) {
		app_update();
		push_flash(color_invisible());

		// Draw hearts for HP.
		for (int i = 0; i < g->player.hp; ++i) {
			Sprite s = make_sprite("heart.ase");
			s.transform.p = V2(-300.0f + i * (s.w + 3.0f), 220);
			s.draw();
		}

		player_movement_routine();
		player_weapons_routine();

		if (g->player.charging_shot) {
			float opacity = g->player.charging_shot == 1 ? 0.5f : 1.0f;
			g->player.charge_sprite.opacity = opacity;
			g->player.charge_sprite.transform = g->player.sprite.transform;
			g->player.charge_sprite.transform.p += V2(0, 22);
			g->player.charge_sprite.update();
			g->player.charge_sprite.draw();
		}

		// Update + draw rockets.
		g->player.rockets.update();
		g->player.rockets.draw();
		v2 rocket_hit;
		while (g->player.rockets.try_pop_hit(&rocket_hit)) {
			g->player.rockets.count--;
			g->asteroids.hit(rocket_hit, 20);
		}

		// Update + draw bullets.
		g->player.bullets.update();
		g->player.bullets.draw();

		// Draw player.
		g->player.sprite.transform.p = g->player.p;
		if (g->player.hurt) {
			if (between_interval(0.05f)) {
				push_flash(color_red());
			} else {
				push_flash(color_invisible());
			}
		} else {
			push_flash(color_invisible());
		}
		g->player.sprite.draw();
		pop_flash();
		if (g->player.shielding) {
			float shield_alpha = remap(1.0f - g->player.shield_time / player_shield_max, 0.4f, 1.0f);
			draw_push_color(color_white() * shield_alpha);
			draw_circle(make_circle(g->player.p, flicker(0.075f, 25, 26)));
		}

		update_bosses();
		draw_bosses();

		// Update + draw asteroids.
		g->asteroids.update();
		g->asteroids.draw();

		// Update + draw all fade FX.
		g->fades.update();
		g->fades.draw();

		// Charge shot projectile.
		g->charge_shot.update_and_draw();

		// Update and draw particles.
		g->particles.update_and_draw();
		g->line_particles.update_and_draw();

		// Update + draw bomb trails.
		g->trails.update();
		g->trails.draw();

		// Draw any animation FX.
		g->animations.update();
		g->animations.draw();

		draw_push_shader(flash_shader);
		app_draw_onto_screen(true);
	}

	destroy_app();

	// [x] Bullet pop asteroid
	// [x] Shield bonk asteroid
	// [x] Ship can get hurt
	// [x] Ship iframes
	// [ ] Boss 1

	// Boss 1: shield (rotates)
	// --> beat normally w/ timing, stun with charge shot and spam minis, no stun lock
	// 3 max shots + barrage worth of hp.
	// 
	// Boss 2: laser (asteroids in between)
	// --> beat w/ shield reflect
	// 
	// Boss 3: missiles (hides behind asteroids/cover)
	// --> beat w/ laser penetration
	// 
	// Boss 4 (final?): fast + side-dash
	// --> beat w/ missile curve & AoE

	return 0;
}
