#include <cute.h>

#define MAX_FIREWORKS 256

typedef struct Firework
{
	CF_Sprite sprite;
	CF_V2 position;
} Firework;

static Firework fireworks[MAX_FIREWORKS];
static int firework_count;

int main(int argc, char* argv[])
{
	cf_make_app("Custom Sprite", 0, 0, 0, 640, 480, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);

	// Mount data directory using full path from base directory.
	char* path = cf_path_normalize(cf_fs_get_base_directory());
	sappend(path, "/custom_sprite_data");
	cf_fs_mount(path, "/", true);
	sfree(path);

	// Load PNG frames.
	CF_Png bullet_pngs[3], explosion_pngs[5];
	cf_custom_sprite_load_png("/bullet_pop1.png", &bullet_pngs[0]);
	cf_custom_sprite_load_png("/bullet_pop2.png", &bullet_pngs[1]);
	cf_custom_sprite_load_png("/bullet_pop3.png", &bullet_pngs[2]);
	for (int i = 0; i < 5; i++) {
		char path[32];
		snprintf(path, sizeof(path), "/explosion%d.png", i + 1);
		cf_custom_sprite_load_png(path, &explosion_pngs[i]);
	}

	// Create animations.
	float bullet_delays[] = { 0.05f, 0.05f, 0.05f };
	float explosion_delays[] = { 0.05f, 0.05f, 0.05f, 0.05f, 0.05f };
	const CF_Animation* bullet_anim = cf_make_custom_sprite_animation("bullet_pop", bullet_pngs, 3, bullet_delays, 3);
	const CF_Animation* explosion_anim = cf_make_custom_sprite_animation("explosion", explosion_pngs, 5, explosion_delays, 5);

	// Animation table + template sprite.
	const CF_Animation* anims[] = { bullet_anim, explosion_anim };
	const CF_AnimationTable* table = cf_make_custom_sprite_animation_table("firework", anims, 2);
	CF_Sprite fireworks_sprite = cf_make_custom_sprite("firework", table);

	// Main loop.
	CF_Rnd rnd = cf_rnd_seed(0);
	float spawn_timer = 0;

	while (cf_app_is_running()) {
		cf_app_update(NULL);
		float dt = CF_DELTA_TIME;

		// Spawn fireworks periodically.
		spawn_timer += dt;
		if (spawn_timer >= 0.1f && firework_count < MAX_FIREWORKS) {
			spawn_timer = 0;
			Firework* fw = &fireworks[firework_count++];
			fw->sprite = fireworks_sprite;
			fw->position = cf_v2(cf_rnd_range_float(&rnd, -200, 200), cf_rnd_range_float(&rnd, -100, 100));
			const char* anim = cf_rnd_range_int(&rnd, 0, 1) ? "explosion" : "bullet_pop";
			cf_sprite_play(&fw->sprite, anim);
			fw->sprite.loop = false;
		}

		// Update, draw, and cull finished fireworks.
		cf_draw_scale(2, 2);
		for (int i = 0; i < firework_count; ) {
			Firework* fw = &fireworks[i];
			cf_sprite_update(&fw->sprite);
			fw->sprite.transform.p = fw->position;
			cf_draw_sprite(&fw->sprite);

			if (fw->sprite.finished) {
				fireworks[i] = fireworks[--firework_count];
			} else {
				i++;
			}
		}

		cf_app_draw_onto_screen(true);
	}

	cf_destroy_app();
	return 0;
}
