#include <cute.h>
#include <imgui.h>
#include <stdio.h>

using namespace Cute;

// Mimics League of Legends armor/magic defense system. It supports shields, physical/magic/true damage, and
// damage over time. Inputs are just raw-damage, so there's not attacker stats, only defender stats.

struct Shield
{
	float hp;
	float expiration;
};

struct C_HitPoints
{
	float max_hp;
	float hp;
	Array<Shield> shields;
	float armor;
	float mage_defense;
	bool dead;

	void add_shield(float hp, float expiration) { Shield shield; shield.hp = hp; shield.expiration = expiration; shields.add(shield); }
	float calc_shields() const { float result = 0; for (int i = 0; i < shields.size(); ++i) { result += shields[i].hp; } return result; }
};

enum Dbf_Type
{
	DBF_HIT_PHYSICAL,
	DBF_HIT_MAGIC,
	DBF_HIT_TRUE,
};

struct Dbf_Hit
{
	Dbf_Type type;
	float damage;
};

// Dot = Damage Over Time.
struct Dbf_Dot
{
	Dbf_Type type;
	float tick_damage;
	float tick_interval;
	float tick_offset;
	int tick_count;
};

struct C_Debuff
{
	Array<Dbf_Hit> hits;
	Array<Dbf_Dot> dots;

	void add_hit(Dbf_Type type, float damage) { Dbf_Hit hit; hit.type = type; hit.damage = damage; this->hits.add(hit); }
	void add_dot(Dbf_Type type, float tick_damage, float interval, int ticks)
	{
		Dbf_Dot dot;
		dot.type = type;
		dot.tick_damage = tick_damage;
		dot.tick_interval = interval;
		dot.tick_offset = cf_mod((float)CF_SECONDS, interval);
		dot.tick_count = ticks;
		this->dots.add(dot);
	}
};

void hitpoints_system(CF_ComponentList component_list, int entity_count, void* udata)
{
	CF_UNUSED(udata);
	C_HitPoints* hitpoints = CF_GET_COMPONENTS(component_list, C_HitPoints);

	for (int i = 0; i < entity_count; ++i) {
		C_HitPoints* hp = hitpoints + i;

		// Remove depleted or expired shields.
		for (int i = 0; i < hp->shields.count();) {
			if (cf_on_timestamp(hp->shields[i].expiration)) {
				hp->shields.unordered_remove(i);
			} else if (hp->shields[i].hp == 0) {
				hp->shields.unordered_remove(i);
			} else {
				 ++i;
			}
		}

		if (hp->hp <= 0) {
			hp->dead = true;
		}
	}
}

void hit_raw(C_HitPoints* hp, float damage)
{
	// Absorb damage into shields.
	for (int i = 0; i < hp->shields.size(); ++i) {
		Shield* shield = hp->shields + i;
		if (shield->hp > 0) {
			float hit = min(shield->hp, damage);
			shield->hp = max(0.0f, shield->hp - hit);
			damage = max(0.0f, damage - hit);
		}
		if (damage == 0) {
			break;
		}
	}

	hp->hp -= damage;
}

void hit_physical(C_HitPoints* hp, float damage)
{
	float mitigated_damage = damage * (100.0f / (100.0f + hp->armor));
	hit_raw(hp, mitigated_damage);
}

void hit_magic(C_HitPoints* hp, float damage)
{
	float mitigated_damage = damage * (100.0f / (100.0f + hp->mage_defense));
	hit_raw(hp, mitigated_damage);
}

void hit_true(C_HitPoints* hp, float damage)
{
	hit_raw(hp, damage);
}

void debuff_system(CF_ComponentList component_list, int entity_count, void* udata)
{
	CF_UNUSED(udata);
	C_Debuff* debuffs = CF_GET_COMPONENTS(component_list, C_Debuff);
	C_HitPoints* hitpoints = CF_GET_COMPONENTS(component_list, C_HitPoints);

	for (int i = 0; i < entity_count; ++i) {
		C_Debuff* dbf = debuffs + i;
		C_HitPoints* hp = hitpoints + i;

		// Update dots.
		for (int i = 0; i < dbf->dots.size();) {
			Dbf_Dot* dot = dbf->dots + i;
			if (cf_on_interval(dot->tick_interval, dot->tick_offset)) {
				dot->tick_count--;
				dbf->add_hit(dot->type, dot->tick_damage);
			}

			// Remove expired dots.
			if (dot->tick_count == 0) {
				dbf->dots.unordered_remove(i);
			} else {
				++i;
			}
		}

		// Apply hits.
		for (int i = 0; i < dbf->hits.size(); ++i) {
			Dbf_Hit* hit = dbf->hits + i;
			switch (hit->type) {
			case DBF_HIT_PHYSICAL: hit_physical(hp, hit->damage); break;
			case DBF_HIT_MAGIC: hit_magic(hp, hit->damage); break;
			case DBF_HIT_TRUE: hit_true(hp, hit->damage); break;
			}
		}
		dbf->hits.clear();
	}
}

int main(int argc, char* argv[])
{
	int options = CF_APP_OPTIONS_DEFAULT_GFX_CONTEXT | CF_APP_OPTIONS_WINDOW_POS_CENTERED;
	CF_Result result = cf_make_app("Hitpoints ECS", 0, 0, 800, 800, options, argv[0]);
	if (cf_is_error(result)) return -1;

	// Register component types.
	cf_component_begin();
	cf_component_set_name(CF_STRINGIZE(C_HitPoints));
	cf_component_set_size(sizeof(C_HitPoints));
	cf_component_set_optional_cleanup(
		[](CF_Entity entity, void* component, void* udata) {
			CF_UNUSED(entity); CF_UNUSED(udata);
			C_HitPoints* c = (C_HitPoints*)component;
			c->~C_HitPoints();
		},
		NULL
	);
	cf_component_end();

	cf_component_begin();
	cf_component_set_name(CF_STRINGIZE(C_Debuff));
	cf_component_set_size(sizeof(C_Debuff));
	cf_component_set_optional_cleanup(
		[](CF_Entity entity, void* component, void* udata) {
			CF_UNUSED(entity); CF_UNUSED(udata);
			C_Debuff* c = (C_Debuff*)component;
			c->~C_Debuff();
		},
		NULL
	);
	cf_component_end();

	// Register entity types.
	cf_entity_begin();
	cf_entity_set_name("Enemy");
	cf_entity_add_component(CF_STRINGIZE(C_HitPoints));
	cf_entity_add_component(CF_STRINGIZE(C_Debuff));
	cf_entity_end();

	// Register systems.
	cf_system_begin();
	cf_system_set_name("HP");
	cf_system_set_update(hitpoints_system);
	cf_system_require_component(CF_STRINGIZE(C_HitPoints));
	cf_system_end();

	cf_system_begin();
	cf_system_set_name("Debuff");
	cf_system_set_update(debuff_system);
	cf_system_require_component(CF_STRINGIZE(C_HitPoints));
	cf_system_require_component(CF_STRINGIZE(C_Debuff));
	cf_system_end();

	// Create 4 "Enemy" entities.
	int n = 4;
	Array<Entity> entities;
	for (int i = 0; i < n; ++i) {
		Entity e = cf_make_entity("Enemy", NULL);
		entities.add(e);

		C_HitPoints* hp = (C_HitPoints*)cf_entity_get_component(e, CF_STRINGIZE(C_HitPoints));
		hp->hp = hp->max_hp = 250;

		if (i == 1) {
			hp->armor = 20;
			hp->mage_defense = 30;
		} else if (i == 2) {
			hp->armor = 100;
			hp->mage_defense = 50;
		} else if (i == 3) {
			hp->armor = 250;
			hp->mage_defense = 250;
		}
	}

	// Initialize imgui.
	cf_app_init_imgui(false);

	while (cf_app_is_running()) {
		cf_app_update(NULL);

		// React to user-input.
		ImGui::SetWindowPos(V2(406,214), ImGuiCond_FirstUseEver);
		ImGui::SetWindowSize(V2(264,323), ImGuiCond_FirstUseEver);
		ImGui::Begin("Apply Hits/Shields");
		const char* entity_combos[] = { "#1", "#2", "#3", "#4", "All" };
		static int entity_index = 0;
		ImGui::Text("Select Enemy");
		ImGui::Combo("Enemy", &entity_index, entity_combos, CF_ARRAY_SIZE(entity_combos));
		ImGui::Separator();
		ImGui::Text("Hit Damage");
		static float damage = 10.0f;
		ImGui::SliderFloat("Damage", &damage, 0, 100);
		const char* damage_types[] = { "Physical", "Magic", "True" };
		static int damage_index = 0;
		ImGui::Combo("Type", &damage_index, damage_types, CF_ARRAY_SIZE(damage_types));
		static bool dot = false;
		ImGui::Checkbox("Damage over time", &dot);
		static float tick_interval = 0.2f;
		static int tick_count = 5;
		if (dot) {
			ImGui::SliderFloat("Interval", &tick_interval, 1.0f/60.0f, 5);
			ImGui::SliderInt("Ticks", &tick_count, 0, 10);
		}
		if (ImGui::Button("Apply Hit")) {
			Array<Entity> entities_to_hit;
			if (entity_index == n) {
				entities_to_hit = entities;
			} else {
				entities_to_hit.add(entities[entity_index]);
			}
			for (int i = 0; i < entities_to_hit.size(); ++i) {
				C_Debuff* dbf = (C_Debuff*)cf_entity_get_component(entities_to_hit[i], CF_STRINGIZE(C_Debuff));
				if (dot) {
					dbf->add_dot((Dbf_Type)damage_index, damage, tick_interval, tick_count);
				} else {
					dbf->add_hit((Dbf_Type)damage_index, damage);
				}
			}
		}
		ImGui::Separator();
		ImGui::Text("Shields");
		static float shield_hp = 50;
		static float shield_duration = 2.5f;
		ImGui::SliderFloat("HP", &shield_hp, 0, 100);
		ImGui::SliderFloat("Duration", &shield_duration, 1, 10);
		if (ImGui::Button("Give Shield")) {
			Array<Entity> entities_to_hit;
			if (entity_index == n) {
				entities_to_hit = entities;
			} else {
				entities_to_hit.add(entities[entity_index]);
			}
			for (int i = 0; i < entities_to_hit.size(); ++i) {
				C_HitPoints* hp = (C_HitPoints*)cf_entity_get_component(entities_to_hit[i], CF_STRINGIZE(C_HitPoints));
				hp->add_shield(shield_hp, (float)(CF_SECONDS + shield_duration));
			}
		}
		ImGui::End();

		// Update the ECS.
		cf_run_systems();

		// Visualization.
		ImGui::SetWindowPos(V2(125,119), ImGuiCond_FirstUseEver);
		ImGui::SetWindowSize(V2(200,523), ImGuiCond_FirstUseEver);
		ImGui::Begin("Enemy Viewer");
		for (int i = 0; i < n; ++i) {
			Entity e = entities[i];
			C_HitPoints* hp = (C_HitPoints*)cf_entity_get_component(e, CF_STRINGIZE(C_HitPoints));
			ImGui::Text("Enemy #%d", i);
			ImGui::Text("Max HP       : %4.1f", hp->max_hp);
			ImGui::Text("HP           : %4.1f", hp->hp);
			ImGui::Text("Armor        : %4.1f", hp->armor);
			ImGui::Text("Mage Defense : %4.1f", hp->mage_defense);
			ImGui::Text("Shields      : %4.1f", hp->calc_shields());
			ImGui::Text("Status       : %s", hp->dead ? "DEAD" : "ALIVE");
			if (i != n - 1) ImGui::Separator();
		}
		ImGui::End();

		cf_app_draw_onto_screen();
	}

	cf_destroy_app();

	return 0;
}
