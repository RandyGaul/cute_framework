#include <cute.h>
#include "imgui.h"

#define CREATURE_BLOCK_HALF_WIDTH 9.0f
#define CREATURE_BLOCK_HALF_HEIGHT 9.0f

#define BLOCK_HALF_WIDTH 10.0f
#define BLOCK_HALF_HEIGHT 5.0f

#define STAR_KEY CF_KEY_Z
#define JUMP_KEY CF_KEY_X
#define STAR_RECALL_KEY CF_KEY_C

#define UP_KEY CF_KEY_UP
#define DOWN_KEY CF_KEY_DOWN
#define RIGHT_KEY CF_KEY_RIGHT
#define LEFT_KEY CF_KEY_LEFT

// since we're using fixed updates, the times below are amount of total frames
#define JUMP_VARIABLE_MAX_TIME (CF_DELTA_TIME_FIXED * 15)
#define STAR_CHARGE_TIME (CF_DELTA_TIME_FIXED * 16)

#define STAR_LIFE_TIME (CF_DELTA_TIME_FIXED * 600)
#define STAR_IMPULSE 200.0f

#define COYOTE_JUMP_FRAME_COUNT 5

#define GRAVITY_X 0
#define GRAVITY_Y -1000.0f

#define SKIN_FACTOR 0.005f
#define INVALID_RIDE_INDEX -1

struct Level
{
    CF_Aabb bounds;
};

struct BlockShape
{
    union
    {
        CF_Aabb aabb;
        CF_Poly poly;
        void* _data;
    };
    CF_ShapeType type;
};

struct Block
{
    CF_Transform transform;
    
    BlockShape shape;
    
    CF_Color color;
};

enum PlayerState
{
    PlayerState_Grounded,
    PlayerState_Jumping,
    PlayerState_Falling,
};

enum PlayerStarState
{
    PlayerStarState_None,
    PlayerStarState_Charging,
    PlayerStarState_Ready,
};

struct Player
{
    CF_Transform transform;
    CF_Transform prev_transform;
    
    CF_V2 velocity;
    
    float ground_acceleration;
    float air_acceleration;
    
    float jump_acceleration;
    float jump_impulse;
    float jump_remaining_time;
    
    CF_Aabb collision_aabb;
    CF_Aabb hit_box_aabb;
    
    PlayerStarState star_state;
    float star_charge_time;
    
    int coyote_grounded[COYOTE_JUMP_FRAME_COUNT];
    int coyote_ground_index;
    
    bool is_riding;
    int riding_index;
    CF_V2 ride_offset;
    
    int thrown_star_index;
    bool is_overlapping_with_thrown_star;
    
    int jump_queued;
    
    PlayerState state;
};

struct Star
{
    CF_Transform transform;
    CF_Transform prev_transform;
    
    CF_V2 velocity;
    
    CF_Aabb collision_aabb;
    CF_Aabb hurt_aabb;
    CF_Aabb player_walkable_aabb;
    
    float bounciness;
    
    float life_time;
};

// stars in gimmick linger, so no need to do velocity
struct StarParticle
{
    CF_Transform transform;
    CF_Transform prev_transform;
    
    CF_V2 size;
    CF_V2 prev_size;
    
    CF_Color color;
    
    float life_time;
};

struct Game
{
    dyna Block *blocks;
    dyna Player *players;
    dyna Star *stars;
    dyna StarParticle *star_particles;
    
    Level level;
};

static inline CF_Aabb shape_get_aabb(BlockShape shape)
{
    if (shape.type == CF_SHAPE_TYPE_AABB)
    {
        return shape.aabb;
    }
    else
    {
        CF_V2 min = cf_v2(FLT_MAX, FLT_MAX);
        CF_V2 max = cf_v2(-FLT_MAX, -FLT_MAX);
        for (int index = 0; index < shape.poly.count; ++index)
        {
            min = cf_min(min, shape.poly.verts[index]);
            max = cf_max(max, shape.poly.verts[index]);
        }
        return cf_make_aabb(min, max);
    }
}

static inline CF_Aabb aabb_set_center(CF_Aabb bb, CF_V2 center)
{
    CF_V2 half_extents = cf_half_extents(bb);
    CF_Aabb new_bb = cf_make_aabb_center_half_extents(center, half_extents);
    return new_bb;
}

static inline BlockShape shape_set_center(BlockShape shape, CF_V2 center)
{
    BlockShape new_shape = shape;
    if (shape.type == CF_SHAPE_TYPE_AABB)
    {
        new_shape.aabb = aabb_set_center(shape.aabb, center);
    }
    else
    {
        for (int index = 0; index < new_shape.poly.count; ++index)
        {
            CF_V2 p = new_shape.poly.verts[index];
            p = cf_sub_v2(center, p);
            new_shape.poly.verts[index] = p;
            cf_make_poly(&new_shape.poly);
        }
    }
    return new_shape;
}

static inline int aabb_to_shape(CF_Aabb aabb, BlockShape shape)
{
    if (shape.type == CF_SHAPE_TYPE_AABB)
    {
        return cf_aabb_to_aabb(aabb, shape.aabb);
    }
    else
    {
        return cf_aabb_to_poly(aabb, &shape.poly, nullptr);
    }
}

static inline CF_Manifold aabb_to_shape_manifold(CF_Aabb aabb, BlockShape shape)
{
    if (shape.type == CF_SHAPE_TYPE_AABB)
    {
        return cf_aabb_to_aabb_manifold(aabb, shape.aabb);
    }
    else
    {
        return cf_aabb_to_poly_manifold(aabb, &shape.poly, nullptr);
    }
}

static inline Player make_player(CF_V2 position)
{
    Player player = {};
    CF_V2 player_half_extents = cf_v2(CREATURE_BLOCK_HALF_WIDTH, CREATURE_BLOCK_HALF_HEIGHT);
    player.collision_aabb = cf_make_aabb_center_half_extents(cf_v2(0.0f, 0.0f), player_half_extents);
    player.hit_box_aabb = cf_expand_aabb(player.collision_aabb, cf_v2(-1.0f, -1.0f));
    player.transform.r = cf_sincos_f(0);
    player.transform.p = position;
    player.prev_transform = player.transform;
    
    // high enough to start reaching max speed within a few frames
    player.ground_acceleration = 300.0f;
    player.air_acceleration = 170.0f;
    player.jump_acceleration = 200.0f;
    player.jump_impulse = 100.0f;
    
    player.riding_index = INVALID_RIDE_INDEX;
    player.thrown_star_index = INVALID_RIDE_INDEX;
    
    return player;
}

static inline Star make_star(CF_V2 position)
{
    Star star = {};
    CF_V2 star_half_extents = cf_v2(CREATURE_BLOCK_HALF_WIDTH, CREATURE_BLOCK_HALF_HEIGHT);
    float star_radius = 10.0f;
    
    star.collision_aabb = cf_make_aabb_center_half_extents(cf_v2(0.0f, 0.0f), star_half_extents);
    star.hurt_aabb = star.collision_aabb;
    star.player_walkable_aabb = star.collision_aabb;
    
    star.transform.r = cf_sincos_f(0);
    star.transform.p = position;
    star.prev_transform = star.transform;
    
    // anything above 1 will start to gain speed
    // 0+..1 will lose speed over time
    // 0 will slide
    // < 0 will try to go into collision, this may get stuck as the star will be pushed out of collision blocks
    star.bounciness = 0.8f;
    star.life_time = STAR_LIFE_TIME;
    
    return star;
}

static inline StarParticle make_star_particle(CF_V2 position, CF_V2 half_extents, float life_time)
{
    StarParticle particle = {};
    particle.transform.p = position;
    particle.prev_transform.p = position;
    
    particle.size = cf_mul_v2_f(half_extents, 2.0f);
    particle.prev_size = particle.size;
    
    particle.color = cf_color_yellow();
    particle.life_time = life_time;
    
    return particle;
}

static inline Block make_block(CF_V2 position)
{
    Block block = {};
    CF_V2 block_half_extents = cf_v2(BLOCK_HALF_WIDTH, BLOCK_HALF_HEIGHT);
    block.shape.aabb = cf_make_aabb_center_half_extents(cf_v2(0.0f, 0.0f), block_half_extents);
    block.shape.type = CF_SHAPE_TYPE_AABB;
    block.transform.p = position;
    block.color = cf_color_white();
    
    return block;
}

static inline Block make_block_right_slope(CF_V2 position)
{
    Block block = {};
    block.shape.poly.verts[0] = cf_v2(BLOCK_HALF_WIDTH, BLOCK_HALF_HEIGHT);
    block.shape.poly.verts[1] = cf_v2(-BLOCK_HALF_WIDTH, BLOCK_HALF_HEIGHT);
    block.shape.poly.verts[2] = cf_v2(-BLOCK_HALF_WIDTH, -BLOCK_HALF_HEIGHT);
    block.shape.poly.count = 3;
    cf_make_poly(&block.shape.poly);
    block.shape.type = CF_SHAPE_TYPE_POLY;
    block.transform.p = position;
    block.color = cf_color_white();
    
    return block;
}

static inline Block make_block_left_slope(CF_V2 position)
{
    Block block = {};
    block.shape.poly.verts[0] = cf_v2(BLOCK_HALF_WIDTH, BLOCK_HALF_HEIGHT);
    block.shape.poly.verts[1] = cf_v2(-BLOCK_HALF_WIDTH, BLOCK_HALF_HEIGHT);
    block.shape.poly.verts[2] = cf_v2(BLOCK_HALF_WIDTH, -BLOCK_HALF_HEIGHT);
    block.shape.poly.count = 3;
    cf_make_poly(&block.shape.poly);
    block.shape.type = CF_SHAPE_TYPE_POLY;
    block.transform.p = position;
    block.color = cf_color_white();
    
    return block;
}

static inline void draw_star(CF_V2 position, CF_V2 size, float chubbiness)
{
    //           p0
    //
    //
    //  p2    p1    p9    p8
    // 
    //       p3      p7
    //           p5
    //
    //    p4            p6
    //
    
    CF_V2 vertices[10] = 
    {
        cf_v2(0.5f ,  1.0f),
        cf_v2(0.35f, 0.65f),
        cf_v2(0.15f, 0.65f),
        cf_v2(0.4f , 0.45f),
        cf_v2(0.25f, 0.15f),
        cf_v2(0.5f , 0.35f),
        cf_v2(0.75f, 0.15f),
        cf_v2(0.6f , 0.45f),
        cf_v2(0.85f, 0.65f),
        cf_v2(0.65f, 0.65f),
    };
    
    CF_V2 half_size = cf_mul_v2_f(size, 0.5f);
    CF_V2 p0;
    CF_V2 p1;
    CF_V2 p2;
    
    for (int index = 0; index < CF_ARRAY_SIZE(vertices); ++index)
    {
        vertices[index] = cf_mul_v2(vertices[index], size);
        vertices[index] = cf_add_v2(vertices[index], position);
        vertices[index] = cf_sub_v2(vertices[index], half_size);
    }
    
    // T triangle
    {
        p0 = vertices[0];
        p1 = vertices[1];
        p2 = vertices[9];
        cf_draw_tri_fill(p0, p1, p2, chubbiness);
    }
    // L triangle
    {
        p0 = vertices[1];
        p1 = vertices[2];
        p2 = vertices[3];
        cf_draw_tri_fill(p0, p1, p2, chubbiness);
    }
    // R triangle
    {
        p0 = vertices[9];
        p1 = vertices[7];
        p2 = vertices[8];
        cf_draw_tri_fill(p0, p1, p2, chubbiness);
    }
    // BL triangle
    {
        p0 = vertices[3];
        p1 = vertices[4];
        p2 = vertices[5];
        cf_draw_tri_fill(p0, p1, p2, chubbiness);
    }
    // BR triangle
    {
        p0 = vertices[5];
        p1 = vertices[6];
        p2 = vertices[7];
        cf_draw_tri_fill(p0, p1, p2, chubbiness);
    }
    // split center into 3 parts
    // C0 triangle
    {
        p0 = vertices[1];
        p1 = vertices[3];
        p2 = vertices[9];
        cf_draw_tri_fill(p0, p1, p2, chubbiness);
    }
    // C1 triangle
    {
        p0 = vertices[3];
        p1 = vertices[5];
        p2 = vertices[9];
        cf_draw_tri_fill(p0, p1, p2, chubbiness);
    }
    // C2 triangle
    {
        p0 = vertices[9];
        p1 = vertices[5];
        p2 = vertices[7];
        cf_draw_tri_fill(p0, p1, p2, chubbiness);
    }
}

// didn't want to embed a sprite sheet or asprite file so we get this
static inline void draw_gimmick(CF_V2 position, float radius, CF_V2 facing_direction)
{
    float quarter_radius = radius * 0.25f;
    
    float upper_body_radius = radius * 0.65f;
    float upper_body_height = radius * 0.05f;
    
    float lower_body_radius = radius * 0.45f;
    float lower_body_height = radius * 0.45f;
    float lower_body_0_distance = -radius * 0.225f;
    float lower_body_1_distance = radius * 0.3f;
    float lower_body_distance = radius * 0.1f;
    
    float eye_0_distance = -radius * 0.05f;
    float eye_1_distance = radius * 0.5f;
    float eye_height = radius * 0.1f;
    float eye_radius = radius * 0.2f;
    
    float pupil_distance = eye_radius * 0.5f;
    float pupil_radius = radius * 0.075f;
    
    float iris_distance = pupil_distance - eye_radius * 1.1f;
    float iris_radius = pupil_radius * 0.015f;
    
    float mouth_radius = radius * 0.025f;
    
    float foot_0_0_distance = -radius * 0.45f;
    float foot_0_1_distance = -radius * 0.3f;
    float foot_1_0_distance = radius * 0.6f;
    float foot_1_1_distance = radius * 0.75f;
    float foot_height = -radius * 0.4f;
    float foot_radius = radius * 0.05f;
    
    float horn_height = upper_body_radius * 0.8f;
    float horn_distance = radius * 0.1f;
    
    CF_V2 upper_body = cf_v2(0.0f, upper_body_height);
    upper_body = cf_add_v2(position, upper_body);
    
    CF_V2 lower_body = cf_mul_v2_f(facing_direction, lower_body_distance);
    lower_body.y -= lower_body_height;
    lower_body = cf_add_v2(position, lower_body);
    CF_V2 lower_body_0 = cf_add_v2(lower_body, cf_mul_v2_f(facing_direction, lower_body_0_distance));
    CF_V2 lower_body_1 = cf_add_v2(lower_body, cf_mul_v2_f(facing_direction, lower_body_1_distance));
    
    CF_V2 eye_0 = cf_mul_v2_f(facing_direction, eye_0_distance);
    eye_0 = cf_add_v2(eye_0, cf_v2(0.0f, eye_height));
    eye_0 = cf_add_v2(eye_0, upper_body);
    
    CF_V2 eye_1 = cf_mul_v2_f(facing_direction, eye_1_distance);
    eye_1 = cf_add_v2(eye_1, cf_v2(0.0f, eye_height));
    eye_1 = cf_add_v2(eye_1, upper_body);
    
    CF_V2 pupil_0 = cf_add_v2(eye_0, cf_mul_v2_f(facing_direction, pupil_distance));
    CF_V2 pupil_1 = cf_add_v2(eye_1, cf_mul_v2_f(facing_direction, pupil_distance));
    
    CF_V2 iris_0 = cf_add_v2(pupil_0, cf_mul_v2_f(facing_direction, iris_distance));
    CF_V2 iris_1 = cf_add_v2(pupil_1, cf_mul_v2_f(facing_direction, iris_distance));
    
    CF_V2 mouth_0 = pupil_0;
    CF_V2 mouth_1 = iris_1;
    mouth_0.y = lower_body_0.y;
    mouth_1.y = lower_body_0.y;
    
    CF_V2 foot_0_0 = cf_mul_v2_f(facing_direction, foot_0_0_distance);
    foot_0_0 = cf_add_v2(foot_0_0, cf_v2(0.0f, foot_height));
    foot_0_0 = cf_add_v2(foot_0_0, lower_body);
    CF_V2 foot_0_1 = cf_mul_v2_f(facing_direction, foot_0_1_distance);
    foot_0_1 = cf_add_v2(foot_0_1, cf_v2(0.0f, foot_height));
    foot_0_1 = cf_add_v2(foot_0_1, lower_body);
    
    CF_V2 foot_1_0 = cf_mul_v2_f(facing_direction, foot_1_0_distance);
    foot_1_0 = cf_add_v2(foot_1_0, cf_v2(0.0f, foot_height));
    foot_1_0 = cf_add_v2(foot_1_0, lower_body);
    CF_V2 foot_1_1 = cf_mul_v2_f(facing_direction, foot_1_1_distance);
    foot_1_1 = cf_add_v2(foot_1_1, cf_v2(0.0f, foot_height));
    foot_1_1 = cf_add_v2(foot_1_1, lower_body);
    
    CF_V2 horn_p0 = cf_add_v2(cf_v2(0.0f, horn_height + radius * 0.05f), cf_mul_v2_f(facing_direction, horn_distance - radius * 0.35f));
    CF_V2 horn_p1 = cf_add_v2(cf_v2(0.0f, horn_height + radius * 0.05f), cf_mul_v2_f(facing_direction, horn_distance - radius * 0.05f));
    CF_V2 horn_p2 = cf_add_v2(cf_v2(0.0f, horn_height + radius * 0.35f), cf_mul_v2_f(facing_direction, horn_distance - radius * 0.4f));
    
    horn_p0 = cf_add_v2(horn_p0, upper_body);
    horn_p1 = cf_add_v2(horn_p1, upper_body);
    horn_p2 = cf_add_v2(horn_p2, upper_body);
    
    cf_draw_push_color(cf_color_orange());
    // horn
    cf_draw_tri_fill(horn_p0, horn_p1, horn_p2, 0.0f);
    cf_draw_pop_color();
    
    cf_draw_push_color(cf_color_green());
    // upper body
    cf_draw_circle_fill2(upper_body, upper_body_radius);
    // lower body
    cf_draw_capsule_fill2(lower_body_0, lower_body_1, lower_body_radius);
    cf_draw_pop_color();
    
    cf_draw_push_color(cf_color_white());
    // eye 0
    cf_draw_circle_fill2(eye_0, eye_radius);
    // eye 1
    cf_draw_circle_fill2(eye_1, eye_radius);
    cf_draw_pop_color();
    
    cf_draw_push_color(cf_color_black());
    // pupil 0
    cf_draw_circle_fill2(pupil_0, pupil_radius);
    // pupil 1
    cf_draw_circle_fill2(pupil_1, pupil_radius);
    cf_draw_pop_color();
    
    cf_draw_push_color(cf_color_white());
    // iris 0
    cf_draw_circle_fill2(iris_0, pupil_radius);
    // iris 1
    cf_draw_circle_fill2(iris_1, pupil_radius);
    cf_draw_pop_color();
    
    cf_draw_push_color(cf_color_grey());
    // mouth
    cf_draw_line(mouth_0, mouth_1, mouth_radius);
    cf_draw_pop_color();
    
    cf_draw_push_color(cf_color_white());
    // foot 0
    cf_draw_capsule_fill2(foot_0_0, foot_0_1, foot_radius);
    // foot 1
    cf_draw_capsule_fill2(foot_1_0, foot_1_1, foot_radius);
    cf_draw_pop_color();
}

// TODO: stars rotating in to a focal point
static inline void get_star_positions(CF_V2 positions[5], CF_V2 position, float distance)
{
    positions[0] = cf_v2(0.5f ,  1.0f);
    positions[1] = cf_v2(0.15f, 0.65f);
    positions[2] = cf_v2(0.25f, 0.15f);
    positions[3] = cf_v2(0.75f, 0.15f);
    positions[4] = cf_v2(0.85f, 0.65f);
    
    CF_V2 offset = cf_v2(distance * 0.5f, distance * 0.5f);
    
    for (int index = 0; index < 5; ++index)
    {
        CF_V2 p = positions[index];
        p = cf_mul_v2_f(p, distance);
        p = cf_add_v2(p, position);
        p = cf_sub_v2(p, offset);
        positions[index] = p;
    }
}

// this was moved out to test if we can skip step_move() and do resolve_overlaps() when only riding
// but it made things a bit jittery
CF_V2 resolve_overlaps(dyna Block *blocks, CF_V2 p, CF_V2 dp, CF_Aabb mover_aabb, CF_V2 normals[3], int *out_normal_count)
{
    CF_V2 new_position = p;
    
    CF_Aabb cull_aabb = aabb_set_center(mover_aabb, p);
    cf_inflate(&cull_aabb, CF_SHAPE_TYPE_AABB, cf_len(dp) + 1.0f);
    
    CF_Aabb padded_aabb = mover_aabb;
    cf_inflate(&padded_aabb, CF_SHAPE_TYPE_AABB, SKIN_FACTOR);
    int nudge_count = 0;
    CF_V2 sample_p = cf_add_v2(p, dp);
    
    int normal_count = 0;
    
    for (int index = 0; index < 10; ++index)
    {
        for (int block_index = 0; block_index < cf_array_count(blocks); ++block_index)
        {
            Block *block = blocks + block_index;
            BlockShape block_shape = shape_set_center(block->shape, block->transform.p);
            CF_Aabb aabb = aabb_set_center(padded_aabb, sample_p);
            
            if (aabb_to_shape(cull_aabb, block_shape))
            {
                CF_Manifold m = aabb_to_shape_manifold(aabb, block_shape);
                if (m.count && cf_abs(m.depths[0]) > 0)
                {
                    CF_V2 offset = cf_mul_v2_f(m.n, m.depths[0]);
                    sample_p = cf_sub_v2(sample_p, offset);
                    
                    CF_V2 n = cf_neg_v2(m.n);
                    
                    int has_normal = false;
                    for (int normal_index = 0; normal_index < normal_count; ++normal_index)
                    {
                        CF_V2 dn = cf_sub_v2(normals[index], n);
                        if (cf_len_sq(dn) < 0.0001f)
                        {
                            has_normal = true;
                            break;
                        }
                    }
                    
                    if (!has_normal && normal_count < CF_ARRAY_SIZE(normals))
                    {
                        normals[normal_count++] = n;
                    }
                    ++nudge_count;
                }
            }
        }
    }
    
    if (nudge_count)
    {
        new_position = sample_p;
    }
    
    if (out_normal_count)
    {
        *out_normal_count = normal_count;
    }
    
    return new_position;
}

enum
{
    StepMoveResult_None = 0,
    StepMoveResult_Grounded = 1 << 0,
    StepMoveResult_Hit_Head = 1 << 1,
};

// TODO: actually do a step height/slope check
// BUG:  there are still collision bugs like penetrating too much then jittering against some tiles
//       but ran out of time to fix
int step_move(dyna Block *blocks, CF_Transform *transform, CF_V2 *velocity, CF_Aabb mover_aabb, float bounciness)
{
    int result = StepMoveResult_None;
    
    float remaining_move_time = CF_DELTA_TIME_FIXED;
    
    CF_V2 p = transform->p;
    CF_V2 v = *velocity;
    
    CF_V2 normals[3] = {};
    int normal_count = 0;
    
    while (remaining_move_time > 0.0001f)
    {
        CF_V2 dp = cf_mul_v2_f(v, remaining_move_time);
        
        float toi = 1.0f;
        CF_ToiResult shortest_result = {};
        float shortest_toi = 1.0f;
        normal_count = 0;
        
        // TODO: do real spatial query for which blocks to do hit tests against instead of doing this
        CF_Aabb cull_aabb = aabb_set_center(mover_aabb, p);
        cf_inflate(&cull_aabb, CF_SHAPE_TYPE_AABB, cf_len(dp) + 1.0f);
        
        {
            CF_Aabb aabb = aabb_set_center(mover_aabb, p);
            cf_inflate(&aabb, CF_SHAPE_TYPE_AABB, -SKIN_FACTOR);
            
            for (int block_index = 0; block_index < cf_array_count(blocks); ++block_index)
            {
                Block *block = blocks + block_index;
                BlockShape block_shape = shape_set_center(block->shape, block->transform.p);
                
                if (aabb_to_shape(cull_aabb, block_shape))
                {
                    CF_ToiResult result = cf_toi(&aabb, CF_SHAPE_TYPE_AABB, nullptr, dp, 
                                                 &block_shape._data, block_shape.type, nullptr, cf_v2(0.0f, 0.0f),
                                                 false);
                    int collided = cf_collided(&aabb, nullptr, CF_SHAPE_TYPE_AABB, &block_shape._data, nullptr, block_shape.type);
                    
                    if (result.hit && !collided)
                    {
                        if (result.toi < shortest_toi)
                        {
                            shortest_toi = result.toi;
                            shortest_result = result;
                        }
                    }
                }
            }
        }
        
        if (shortest_result.hit)
        {
            toi = shortest_result.toi;
        }
        
        dp = cf_mul_v2_f(v, remaining_move_time * toi);
        p = resolve_overlaps(blocks, p, dp, mover_aabb, normals, &normal_count);
        
        for (int normal_index = 0; normal_index < normal_count; ++normal_index)
        {
            CF_V2 normal = normals[normal_index];
            float clip_amount = cf_dot(v, normal);
            CF_V2 clip = cf_mul_v2_f(normal, clip_amount * (1.0f + bounciness));
            v = cf_sub_v2(v, clip);
        }
        
        dp = cf_mul_v2_f(v, remaining_move_time * toi);
        p = cf_add_v2(p, dp);
        
        remaining_move_time = remaining_move_time - remaining_move_time * toi;
        if (toi == 0.0f)
        {
            remaining_move_time = 0.0f;
        }
        
        for (int normal_index = 0; normal_index < normal_count; ++normal_index)
        {
            CF_V2 normal = normals[normal_index];
            float ground_dot = cf_dot(normal, cf_v2(0.0f, 1.0f));
            if (ground_dot > 0)
            {
                result |= StepMoveResult_Grounded;
            }
            else
            {
                if (ground_dot < 0)
                {
                    result |= StepMoveResult_Hit_Head;
                }
            }
        }
    }
    
    transform->p = p;
    *velocity = v;
    
    return result;
}

void imgui_render(Game *game)
{
    const char *states[] = 
    {
        "Grounded",
        "Jumping",
        "Falling",
    };
    
    ImGui::Begin("Debug");
    {
        Player *player = game->players;
        ImGui::Text("Z - Recall/Charge/Fire Star | X - Jump | C - Recall Star");
        ImGui::Text("R - Restart");
        ImGui::Text("%-10s | Is Riding: %d", states[player->state], player->is_riding);
        ImGui::Text("Velocity: %0.3f, %0.3f", player->velocity.x, player->velocity.y);
    }
    ImGui::End();
}

void update_blocks(Game *game);
void render_blocks(Game *game);

void update_stars(Game *game);
void render_stars(Game *game);

void update_players(Game *game);
void render_players(Game *game);

void update_star_particles(Game *game);
void render_star_particles(Game *game);

void load_level(Game *game);
void init(void* udata);
void update(void* udata);
void render(void* udata);

void update_blocks(Game *game)
{
    // nothing to do for now
}

void render_blocks(Game *game)
{
    for (int index = 0; index < cf_array_count(game->blocks); ++index)
    {
        Block *block = game->blocks + index;
        BlockShape shape = shape_set_center(block->shape, block->transform.p);
        
        cf_draw_push_color(block->color);
        if (shape.type == CF_SHAPE_TYPE_AABB)
        {
            cf_draw_box(shape.aabb, 0.0f, 0.0f);
        }
        else
        {
            cf_draw_polyline(shape.poly.verts, shape.poly.count, 0.0f, true);
        }
        cf_draw_pop_color();
    }
}

void update_stars(Game *game)
{
    float death_y = game->level.bounds.min.y - BLOCK_HALF_HEIGHT * 2;
    
    dyna Star *stars = game->stars;
    dyna Block *blocks = game->blocks;
    
    CF_V2 gravity = cf_v2(GRAVITY_X, GRAVITY_Y);
    CF_V2 g = cf_mul_v2_f(gravity, CF_DELTA_TIME_FIXED);
    
    for (int index = 0; index < cf_array_count(stars); ++index)
    {
        Star *star = stars + index;
        
        star->prev_transform = star->transform;
        
        CF_V2 v = star->velocity;
        v = cf_add_v2(v, g);
        
        int move_result = step_move(blocks, &star->transform, &v, star->collision_aabb, star->bounciness); 
        
        star->velocity = v;
        
        star->life_time -= CF_DELTA_TIME_FIXED;
        
        if (cf_on_interval(CF_DELTA_TIME_FIXED * 4, 0.0f))
        {
            CF_V2 half_extents = cf_half_extents(star->collision_aabb);
            StarParticle particle = make_star_particle(star->transform.p, half_extents, CF_DELTA_TIME_FIXED * 8);
            particle.color.a = 0.5f;
            cf_array_push(game->star_particles, particle);
        }
    }
    
    // check if out of bounds or dead star
    {
        int index = 0;
        while (index < cf_array_count(stars))
        {
            Star *star = game->stars + index;
            bool is_out_of_bounds = star->transform.p.y < death_y;
            bool is_out_of_time = star->life_time <= 0.0f;
            if (is_out_of_bounds || is_out_of_time)
            {
                int last_index = cf_array_count(game->stars) - 1;
                CF_MEMCPY(game->stars + index, game->stars + last_index, sizeof(stars[0]));
                CF_UNUSED(cf_array_pop(game->stars));
                continue;
            }
            
            index++;
        }
    }
}

void render_stars(Game *game)
{
    cf_draw_push_color(cf_color_yellow());
    for (int index = 0; index < cf_array_count(game->stars); ++index)
    {
        Star *star = game->stars + index;
        CF_V2 position = cf_lerp_v2(star->prev_transform.p, star->transform.p, CF_DELTA_TIME_INTERPOLANT);
        CF_Aabb star_walkable_aabb = aabb_set_center(star->player_walkable_aabb, position);
        
        draw_star(position, cf_v2(CREATURE_BLOCK_HALF_WIDTH * 2, CREATURE_BLOCK_HALF_HEIGHT * 2), 0.0f);
        //cf_draw_box(star_walkable_aabb, 0.0f, 0.0f);
    }
    cf_draw_pop_color();
}

void update_players(Game *game)
{
    float death_y = game->level.bounds.min.y - CREATURE_BLOCK_HALF_HEIGHT * 4;
    
    CF_V2 level_extents = cf_extents(game->level.bounds);
    
    dyna Player *players = game->players;
    dyna Star *stars = game->stars;
    dyna StarParticle *star_particles = game->star_particles;
    dyna Block *blocks = game->blocks;
    
    CF_V2 gravity = cf_v2(GRAVITY_X, GRAVITY_Y);
    CF_V2 g = cf_mul_v2_f(gravity, CF_DELTA_TIME_FIXED);
    
    float ground_friction = 3.75f;
    
    bool input_jump_pressed = cf_key_just_pressed(JUMP_KEY);
    bool input_jump_down = cf_key_down(JUMP_KEY);
    bool input_star_pressed = cf_key_just_pressed(STAR_KEY);
    bool input_star_down = cf_key_down(STAR_KEY);
    bool input_star_up = cf_key_up(STAR_KEY);
    bool input_recall_pressed = cf_key_just_pressed(STAR_RECALL_KEY);

    CF_V2 input_direction = cf_v2(0.0f, 0.0f);
    if (cf_key_down(UP_KEY))
    {
        input_direction.y += 1.0f;
    }
    if (cf_key_down(DOWN_KEY))
    {
        input_direction.y -= 1.0f;
    }
    if (cf_key_down(RIGHT_KEY))
    {
        input_direction.x += 1.0f;
    }
    if (cf_key_down(LEFT_KEY))
    {
        input_direction.x -= 1.0f;
    }
    
    for (int index = 0; index < cf_array_count(players); ++index)
    {
        Player *player = players + index;
        
        player->prev_transform = player->transform;
        
        CF_V2 v = player->velocity;
        
        int hit_head = false;
        
        float acceleration = player->ground_acceleration;
        float friction = ground_friction;
        
        if (player->state != PlayerState_Grounded)
        {
            acceleration = player->air_acceleration;
            friction = 0.0f;
        }
        
        // apply gravity only when falling
        if (player->state != PlayerState_Jumping)
        {
            v = cf_add_v2(v, g);
        }
        
        // update facing direction
        if (input_direction.x > 0)
        {
            player->transform.r = cf_sincos_f(0);
        }
        else if (input_direction.x < 0)
        {
            player->transform.r = cf_sincos_f(CF_PI);
        }
        
        v.x += input_direction.x * acceleration * CF_DELTA_TIME_FIXED;
        
        // friction
        {
            float new_ground_speed = cf_abs(v.x);
            new_ground_speed = new_ground_speed - new_ground_speed * friction * CF_DELTA_TIME_FIXED;
            if (new_ground_speed < 0)
            {
                new_ground_speed = 0.0f;
            }
            v.x = cf_sign(v.x) * new_ground_speed;
        }
        
        // speed is not capped since we want the player to be able to move fast when able to (slopes, star riding, knockbacks, etc)
        // speed ground capping is handled by a high ground friction, this is high enough so when normally moving back and forth to
        // stop the player within a few frames
        
        // check for star rides
        // prioritize jumping away before starting rides, feels bad when trying to jump to platform but you start riding a star instead
        int can_ride = player->state != PlayerState_Jumping;
        
        // don't allow riding the newly thrown star until it's no longer overlapping
        if (player->is_overlapping_with_thrown_star)
        {
            CF_Aabb player_aabb = aabb_set_center(player->collision_aabb, player->transform.p);
            
            if (player->thrown_star_index != INVALID_RIDE_INDEX &&
                player->thrown_star_index < cf_array_count(stars))
            {
                Star *star = stars + player->thrown_star_index;
                CF_Aabb star_aabb = aabb_set_center(star->player_walkable_aabb, star->transform.p);
                
                if (cf_aabb_to_aabb(player_aabb, star_aabb))
                {
                    can_ride = false;
                }
                else
                {
                    player->is_overlapping_with_thrown_star = false;
                    player->thrown_star_index = INVALID_RIDE_INDEX;
                }
            }
        }
        
        if (can_ride)
        {
            bool is_star_riding = false;
            
            CF_V2 inherit_velocity = cf_v2(0.0f, 0.0f);
            CF_Aabb player_aabb = aabb_set_center(player->collision_aabb, player->transform.p);
            
            if (player->is_riding)
            {
                float ride_skin_factor = 1.0f;
                cf_inflate(&player_aabb, CF_SHAPE_TYPE_AABB, ride_skin_factor);
            }
            
            CF_V2 new_p = player->transform.p;
            
            int star_riding_index = INVALID_RIDE_INDEX;
            CF_V2 star_position;
            CF_V2 star_offset;
            
            for (int star_index = 0; star_index < cf_array_count(stars); ++star_index)
            {
                Star *star = stars + star_index;
                CF_Aabb star_aabb = aabb_set_center(star->player_walkable_aabb, star->transform.p);
                
                CF_Manifold m = cf_aabb_to_aabb_manifold(player_aabb, star_aabb);
                if (m.count)
                {
                    bool is_star_under_player = cf_dot(m.n, cf_v2(0.0f, -1.0f)) > 0;
                    if (is_star_under_player)
                    {
                        inherit_velocity = star->velocity;
                        CF_V2 offset = cf_mul_v2_f(m.n, m.depths[0]);
                        new_p = cf_sub_v2(player->transform.p, offset);
                        is_star_riding = true;
                        
                        star_riding_index = star_index;
                        star_offset = cf_sub_v2(new_p, star->transform.p);
                        star_position = star->transform.p;
                    }
                }
            }
            
            if (is_star_riding)
            {
                v = inherit_velocity;
                if (star_riding_index != player->riding_index)
                {
                    player->riding_index = star_riding_index;
                    player->ride_offset = star_offset;
                    player->transform.p = new_p;
                    
                    // clear out initial ride velocity, otherwise player will fly off
                    v = cf_v2(0.0f, 0.0f);
                }
                else
                {
                    // try to maintain roughly same offset
                    player->transform.p = cf_add_v2(star_position, player->ride_offset);
                }
            }
            else
            {
                player->riding_index = INVALID_RIDE_INDEX;
            }
            player->is_riding = is_star_riding;
        }
        
        int move_result = step_move(blocks, &player->transform, &v, player->collision_aabb, 0); 
        if (move_result & StepMoveResult_Grounded)
        {
            player->state = PlayerState_Grounded;
        }
        else if (player->state != PlayerState_Jumping)
        {
            player->state = PlayerState_Falling;
        }
        
        if (player->is_riding)
        {
            if (player->state == PlayerState_Falling)
            {
                player->state = PlayerState_Grounded;
            }
        }
        
        // coyote jump check
        int can_jump = false;
        for (int coyote_grounded_index = 0; coyote_grounded_index < CF_ARRAY_SIZE(player->coyote_grounded); ++coyote_grounded_index)
        {
            if (player->coyote_grounded[coyote_grounded_index])
            {
                can_jump = true;
                break;
            }
        }
        
        // don't allow initial jumps when all ready jumping
        if (can_jump)
        {
            if (player->state == PlayerState_Jumping)
            {
                can_jump = false;
            }
        }
        
        // jump input buffering
        if (input_jump_pressed)
        {
            if (!player->jump_queued)
            {
                if (player->state != PlayerState_Jumping)
                {
                    player->jump_queued = true;
                }
            }
        }
        else if (!input_jump_down)
        {
            player->jump_queued = false;
        }
        
        // variable jumping
        {
            if (can_jump)
            {
                if (player->jump_queued)
                {
                    // always try to get the most possible initial jump height
                    v.y = cf_max(v.y + player->jump_impulse, player->jump_impulse);
                    player->jump_remaining_time = JUMP_VARIABLE_MAX_TIME;
                    player->state = PlayerState_Jumping;
                    
                    player->jump_queued = false;
                }
            }
            
            if (player->state == PlayerState_Jumping)
            {
                if (player->jump_remaining_time > 0)
                {
                    if (move_result & StepMoveResult_Hit_Head)
                    {
                        player->jump_remaining_time = 0.0f;
                    }
                    else if (input_jump_down)
                    {
                        v = cf_add_v2(v, cf_v2(0.0f, player->jump_acceleration * CF_DELTA_TIME_FIXED));
                        player->jump_remaining_time -= CF_DELTA_TIME_FIXED;
                    }
                    else
                    {
                        player->jump_remaining_time = 0.0f;
                    }
                }
                else
                {
                    player->state = PlayerState_Falling;
                }
            }
        }
        
        // star throwing
        {
            if (input_recall_pressed)
            {
                cf_array_clear(stars);
            }
            
            if (player->star_state == PlayerStarState_None)
            {
                if (input_star_pressed)
                {
                    player->star_charge_time = STAR_CHARGE_TIME;
                    player->star_state = PlayerStarState_Charging;
                    
                    // destroy old star(s) when throwing new one
                    cf_array_clear(stars);
                }
            }
            else if (player->star_state == PlayerStarState_Charging)
            {
                float min_particle_distance = CREATURE_BLOCK_HALF_WIDTH * 2;
                float max_particle_distance = CREATURE_BLOCK_HALF_WIDTH * 2 * 6;
                
                if (cf_on_interval(CF_DELTA_TIME_FIXED * 2, 0.0f))
                {
                    float lerp_value = cf_clamp01(player->star_charge_time / STAR_CHARGE_TIME);
                    float particle_distance = cf_lerp(min_particle_distance, max_particle_distance, lerp_value);
                    CF_V2 particle_positions[5];
                    CF_V2 particle_half_extents = cf_v2(CREATURE_BLOCK_HALF_WIDTH * 0.5f, CREATURE_BLOCK_HALF_HEIGHT * 0.5f);
                    CF_V2 particle_focal_point = cf_v2(0.0f, CREATURE_BLOCK_HALF_HEIGHT);
                    
                    particle_focal_point = cf_add_v2(player->transform.p, particle_focal_point);
                    
                    get_star_positions(particle_positions, particle_focal_point, particle_distance);
                    
                    for (int particle_index = 0; particle_index < CF_ARRAY_SIZE(particle_positions); ++particle_index)
                    {
                        StarParticle particle = make_star_particle(particle_positions[particle_index], particle_half_extents, CF_DELTA_TIME_FIXED * 4);
                        cf_array_push(star_particles, particle);
                    }
                }
                
                if (input_star_down)
                {
                    player->star_charge_time -= CF_DELTA_TIME_FIXED;
                }
                else
                {
                    player->star_state = PlayerStarState_None;
                }
                
                if (player->star_charge_time <= 0)
                {
                    player->star_state = PlayerStarState_Ready;
                }
            }
            else if (player->star_state == PlayerStarState_Ready)
            {
                if (input_star_up)
                {
                    CF_V2 star_position = player->transform.p;
                    CF_V2 star_offset = cf_v2(0.0f, CREATURE_BLOCK_HALF_HEIGHT);
                    star_position = cf_add_v2(star_position, star_offset);
                    
                    Star star = make_star(star_position);
                    star.transform.r = player->transform.r;
                    
                    CF_V2 star_velocity = cf_x_axis(star.transform.r);
                    star_velocity.y = -1.0f;
                    star_velocity = cf_mul_v2_f(star_velocity, STAR_IMPULSE);
                    star.velocity = star_velocity;
                    
                    cf_array_push(stars, star);
                    player->thrown_star_index = cf_array_count(stars) - 1;
                    player->is_overlapping_with_thrown_star = true;
                    
                    player->star_state = PlayerStarState_None;
                }
            }
        }
        
        // update coyote grounded frames
        {
            player->coyote_grounded[player->coyote_ground_index] = player->state == PlayerState_Grounded;
            player->coyote_ground_index = (player->coyote_ground_index + 1) % CF_ARRAY_SIZE(player->coyote_grounded);
        }
        
        player->velocity = v;
    }
    
    // check if out of bounds
    {
        int index = 0;
        while (index < cf_array_count(players))
        {
            Player *player = game->players + index;
            bool is_out_of_bounds = player->transform.p.y < death_y;
            if (is_out_of_bounds)
            {
                int last_index = cf_array_count(game->players) - 1;
                CF_MEMCPY(players + index, players + last_index, sizeof(players[0]));
                CF_UNUSED(cf_array_pop(game->players));
                continue;
            }
            
            index++;
        }
    }
}

void render_players(Game *game)
{
    for (int index = 0; index < cf_array_count(game->players); ++index)
    {
        Player *player = game->players + index;
        CF_V2 position = cf_lerp_v2(player->prev_transform.p, player->transform.p, CF_DELTA_TIME_INTERPOLANT);
        CF_V2 half_extents = cf_half_extents(player->collision_aabb);
        CF_V2 facing_direction = cf_x_axis(player->transform.r);
        float radius = cf_max(half_extents.x, half_extents.y);
        
        draw_gimmick(player->transform.p, radius, facing_direction);
        
        if (player->star_state == PlayerStarState_Ready)
        {
            CF_V2 star_size = cf_v2(CREATURE_BLOCK_HALF_WIDTH * 2, CREATURE_BLOCK_HALF_HEIGHT * 2);
            CF_V2 star_position = cf_v2(0.0f, CREATURE_BLOCK_HALF_HEIGHT);
            star_position = cf_add_v2(position, star_position);
            cf_draw_push_color(cf_color_yellow());
            draw_star(star_position, star_size, 0.0f);
            cf_draw_pop_color();
        }
    }
}

void update_star_particles(Game *game)
{
    dyna StarParticle *particles = game->star_particles;
    
    for (int index = 0; index < cf_array_count(particles); ++index)
    {
        StarParticle *particle = particles + index;
        
        particle->prev_transform = particle->transform;
        particle->prev_size = particle->size;
        
        particle->life_time -= CF_DELTA_TIME_FIXED;
    }
    
    // check if dead particle
    {
        int index = 0;
        while (index < cf_array_count(particles))
        {
            StarParticle *particle = particles + index;
            bool is_out_of_time = particle->life_time <= 0.0f;
            if (is_out_of_time)
            {
                int last_index = cf_array_count(particles) - 1;
                CF_MEMCPY(particles + index, particles + last_index, sizeof(particles[0]));
                CF_UNUSED(cf_array_pop(particles));
                continue;
            }
            
            index++;
        }
    }
}

void render_star_particles(Game *game)
{
    dyna StarParticle *particles = game->star_particles;
    for (int index = 0; index < cf_array_count(particles); ++index)
    {
        StarParticle *particle = particles + index;
        CF_V2 position = cf_lerp_v2(particle->prev_transform.p, particle->transform.p, CF_DELTA_TIME_INTERPOLANT);
        CF_V2 size = cf_lerp_v2(particle->prev_size, particle->size, CF_DELTA_TIME_INTERPOLANT);
        
        cf_draw_push_color(particle->color);
        draw_star(position, size, 0.0f);
        cf_draw_pop_color();
    }
}

void load_level(Game *game)
{
    cf_array_clear(game->players);
    cf_array_clear(game->stars);
    cf_array_clear(game->blocks);
    
    int width = 20;
    int height = 20;
    
    const char *level = 
        "#..................#"
        "#..................#"
        "#..................#"
        "#..................#"
        "#..................#"
        "#.......p..........#"
        "#..................#"
        "#..................#"
        "#..................#"
        "#......###.###.....#"
        "#................###"
        "#................###"
        "#.....#.........####"
        "#...............####"
        "###.............####"
        "###.............####"
        "####l.........r#####"
        "#####l.......r######"
        "####################"
        "####################"
        ;
    
    CF_V2 tile_extents = cf_v2(BLOCK_HALF_WIDTH * 2, BLOCK_HALF_HEIGHT * 2);
    CF_V2 min = cf_v2(FLT_MAX, FLT_MAX);
    CF_V2 max = cf_v2(FLT_MIN, FLT_MIN);
    
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            char c = level[x + y * width];
            int tile_x = x;
            // flip y, otherwise map is inverted
            int tile_y = (height - y);
            
            CF_V2 position = cf_v2(tile_extents.x * tile_x, tile_extents.y * tile_y);
            CF_Aabb aabb = {};
            
            if (c == '#')
            {
                Block block = make_block(position);
                cf_array_push(game->blocks, block);
                
                BlockShape shape = shape_set_center(block.shape, position);
                aabb = shape_get_aabb(shape);
            }
            else if (c == 'p')
            {
                Player player = make_player(position);
                cf_array_push(game->players, player);
                
                aabb = aabb_set_center(player.collision_aabb, position);
            }
            else if (c == 'r')
            {
                Block block = make_block_right_slope(position);
                cf_array_push(game->blocks, block);
                
                BlockShape shape = shape_set_center(block.shape, position);
                aabb = shape_get_aabb(shape);
            }
            else if (c == 'l')
            {
                Block block = make_block_left_slope(position);
                cf_array_push(game->blocks, block);
                
                BlockShape shape = shape_set_center(block.shape, position);
                aabb = shape_get_aabb(shape);
            }
            
            min = cf_min_v2(min, aabb.min);
            max = cf_max_v2(max, aabb.max);
        }
    }
    
    game->level.bounds = cf_make_aabb(min, max);
    game->level.bounds.min.y += BLOCK_HALF_HEIGHT;
};

void init(void* udata)
{
    Game *game = (Game*)udata;
    
    cf_array_fit(game->players, 1);
    cf_array_fit(game->stars, 1);
    cf_array_fit(game->blocks, 256);
    cf_array_fit(game->star_particles, 256);
    
    load_level(game);
};

void update(void* udata)
{
    Game *game = (Game*)udata;
    
    update_blocks(game);
    update_stars(game);
    update_players(game);
    update_star_particles(game);
    
    if (cf_key_just_pressed(CF_KEY_R))
    {
        load_level(game);
    }
}

void render(void* udata)
{
    Game *game = (Game*)udata;
    
    CF_Aabb level_bounds = game->level.bounds;
    CF_V2 camera_offset = cf_center(level_bounds);
    // move this up a bit
    camera_offset.y -= BLOCK_HALF_HEIGHT * 2 * 4;
    camera_offset = cf_neg_v2(camera_offset);
    
    cf_draw_push();
    cf_draw_scale(1.5f, 1.5f);
    cf_draw_translate_v2(camera_offset);
    
    render_blocks(game);
    render_stars(game);
    render_players(game);
    render_star_particles(game);
    
    // level bounds
    {
        cf_draw_push_color(cf_color_grey());
        cf_draw_box(level_bounds, 0.0f, 0.0f);
        cf_draw_pop_color();
    }
    
    cf_draw_pop();
}

int main(int argc, char **argv)
{
    int display_index = cf_default_display();
    int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT;
    int window_x = 0;
    int window_y = 0;
    int window_width = 640;
    int window_height = 480;
    float display_refresh_rate = cf_display_refresh_rate(display_index);
    Game game = {};
    
	CF_Result result = cf_make_app("cute gimmick", display_index, window_x, window_y, window_width, window_height, options, argv[0]);
    
    cf_set_fixed_timestep(60);
    cf_set_target_framerate((int)display_refresh_rate);
    
    cf_app_init_imgui();
    cf_set_update_udata(&game);
    
    init(&game);
    
    while (cf_app_is_running())
    {
        cf_app_update(update);
        render(&game);
        imgui_render(&game);
        cf_app_draw_onto_screen(true);
    }
    
    cf_destroy_app();
    
    return 0;
}