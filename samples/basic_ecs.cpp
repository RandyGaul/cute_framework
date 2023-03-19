#include <cute.h>
#include <stdio.h>

struct C_HitPoints
{
	int max_hp;
	int hp;
};

struct C_Poison
{
	bool active;
	float elapsed;
	float duration;
};

struct C_Enemy
{
};

void hitpoints_system(CF_ComponentList component_list, int entity_count, void* udata)
{
	C_HitPoints* hitpoints = CF_GET_COMPONENTS(component_list, C_HitPoints);
}

int main(int argc, char* argv[])
{
	int options = CF_APP_OPTIONS_DEFAULT_GFX_CONTEXT | CF_APP_OPTIONS_WINDOW_POS_CENTERED;
	CF_Result result = cf_make_app("Basic Input", 0, 0, 640, 480, options, argv[0]);
	if (cf_is_error(result)) return -1;

	while (cf_app_is_running())
	{
		cf_app_update(NULL);


		cf_app_draw_onto_screen();
	}

	cf_destroy_app();

	return 0;
}
