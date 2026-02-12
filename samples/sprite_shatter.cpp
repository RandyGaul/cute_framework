#include <cute.h>
#include "proggy.h"

#define STR(X) #X

const char* shader_str = STR(
vec4 shader(vec4 color, vec2 pos, vec2 screen_uv, vec4 params)
{
    return texture(u_image, smooth_uv(v_uv, u_texture_size));
}
);

#ifndef CF_RUNTIME_SHADER_COMPILATION
#include "sprite_shatter_shd.h"
#endif

struct
{
    CF_Shader shader;
    CF_Material material;
    CF_Mesh mesh;
    CF_M3x2 projection;
    CF_Vertex* verts;
} draw;

struct SpriteChunk
{
    CF_V2 verts[4];
    CF_V2 uvs[4];
    CF_V2 velocity;
    uint8_t opacity;
    CF_Pixel color;
};

void add_vertex_attribute(CF_VertexAttribute* attrs, const char* name, CF_VertexFormat format, int offset);
void init(int w, int h);
void shatter_sprite(CF_Sprite* sprite, int steps, SpriteChunk* chunks);
void update_sprite_chunks(SpriteChunk* chunks);
void draw_sprite_chunks(SpriteChunk* chunks);

void add_vertex_attribute(CF_VertexAttribute* attrs, const char* name, CF_VertexFormat format, int offset)
{
    CF_VertexAttribute vert = {};
    vert.name = name;
    vert.format = format;
    vert.offset = offset;
    
    cf_array_push(attrs, vert);
}

void init(int w, int h)
{
#ifdef CF_RUNTIME_SHADER_COMPILATION
    draw.shader = cf_make_draw_shader_from_source(shader_str);
#else
    draw.shader = cf_make_draw_shader_from_bytecode(s_sprite_shatter_shd_bytecode);
#endif
    draw.material = cf_make_material();
    
	CF_VertexAttribute* attrs = NULL;
    cf_array_fit(attrs, 32);
    
    add_vertex_attribute(attrs, "in_pos", CF_VERTEX_FORMAT_FLOAT2, CF_OFFSET_OF(CF_Vertex, p));
    add_vertex_attribute(attrs, "in_posH", CF_VERTEX_FORMAT_FLOAT2, CF_OFFSET_OF(CF_Vertex, posH));
    add_vertex_attribute(attrs, "in_n", CF_VERTEX_FORMAT_INT, CF_OFFSET_OF(CF_Vertex, n));
    add_vertex_attribute(attrs, "in_ab", CF_VERTEX_FORMAT_FLOAT4, CF_OFFSET_OF(CF_Vertex, shape[0]));
    add_vertex_attribute(attrs, "in_cd", CF_VERTEX_FORMAT_FLOAT4, CF_OFFSET_OF(CF_Vertex, shape[2]));
    add_vertex_attribute(attrs, "in_ef", CF_VERTEX_FORMAT_FLOAT4, CF_OFFSET_OF(CF_Vertex, shape[4]));
    add_vertex_attribute(attrs, "in_gh", CF_VERTEX_FORMAT_FLOAT4, CF_OFFSET_OF(CF_Vertex, shape[6]));
    add_vertex_attribute(attrs, "in_uv", CF_VERTEX_FORMAT_FLOAT2, CF_OFFSET_OF(CF_Vertex, uv));
	add_vertex_attribute(attrs, "in_col", CF_VERTEX_FORMAT_UBYTE4_NORM, CF_OFFSET_OF(CF_Vertex, color));
	add_vertex_attribute(attrs, "in_radius", CF_VERTEX_FORMAT_FLOAT, CF_OFFSET_OF(CF_Vertex, radius));
	add_vertex_attribute(attrs, "in_stroke", CF_VERTEX_FORMAT_FLOAT, CF_OFFSET_OF(CF_Vertex, stroke));
	add_vertex_attribute(attrs, "in_aa", CF_VERTEX_FORMAT_FLOAT, CF_OFFSET_OF(CF_Vertex, aa));
    add_vertex_attribute(attrs, "in_params", CF_VERTEX_FORMAT_UBYTE4_NORM, CF_OFFSET_OF(CF_Vertex, type));
    add_vertex_attribute(attrs, "in_user_params", CF_VERTEX_FORMAT_FLOAT4, CF_OFFSET_OF(CF_Vertex, attributes));
    draw.mesh = cf_make_mesh(CF_MB * 5, attrs, cf_array_count(attrs), sizeof(CF_Vertex));
    
    draw.verts = NULL;
    cf_array_fit(draw.verts, 1024);
    
    draw.projection = cf_ortho_2d(0, 0, (float)w, (float)h);
    cf_draw_projection(draw.projection);
    
    cf_array_free(attrs);
}

void shatter_sprite(CF_Sprite* sprite, int steps, SpriteChunk* chunks)
{
    float split_step = (float)steps;
    
    CF_Pixel color = cf_color_to_pixel(cf_draw_peek_color());
    uint8_t opacity = (uint8_t)(sprite->opacity * 255.0f);
    
    CF_V2 sprite_min = sprite->transform.p;
    CF_V2 sprite_max = sprite_min;
    float w = sprite->w * sprite->scale.x;
    float h = sprite->h * sprite->scale.y;
    
    sprite_min.x -= w / 2;
    sprite_min.y -= h / 2;
    sprite_max.x += w / 2;
    sprite_max.y += h / 2;
    
    CF_Aabb aabb = cf_make_aabb(sprite_min, sprite_max);
    CF_V2 step = cf_sub(sprite_max, sprite_min);
    step = cf_div(step, split_step);
    
    CF_M3x2 mvp = cf_draw_peek();
    mvp = cf_mul_m32(draw.projection, mvp);
    
    CF_Vertex verts[6] = {};
    CF_TemporaryImage temporary_image = cf_fetch_image(sprite);
    CF_V2 u = cf_min(temporary_image.v, temporary_image.u);
    CF_V2 v = cf_max(temporary_image.v, temporary_image.u);
    CF_V2 duv = cf_sub(v, u);
    CF_V2 uv_step = cf_div(duv, split_step);
    
    CF_V2 center = cf_center(aabb);
    float push_strength = 10.0f;
    
    // y is down
    for (int y = 0; y < split_step; ++y)
    {
        for (int x = 0; x < split_step; ++x)
        {
            SpriteChunk chunk = {};
            
            CF_V2 chunk_top_left = cf_top_left(aabb);
            chunk_top_left.x += step.x * x;
            chunk_top_left.y -= step.y * y;
            
            CF_V2 chunk_center = chunk_top_left;
            chunk_center.x += step.x * 0.5f;
            chunk_center.y -= step.y * 0.5f;
            
            // uniform force push to each chunk from the center
            CF_V2 velocity = cf_sub(chunk_center, center);
            velocity = cf_norm(velocity);
            velocity = cf_mul_v2_f(velocity, push_strength);
            
            CF_Aabb chunk_aabb = cf_make_aabb_from_top_left(chunk_top_left, step.x, step.y);
            
            CF_V2 chunk_u = u;
            chunk_u.x += uv_step.x * x;
            chunk_u.y += uv_step.y * y;
            
            CF_V2 uv0 = chunk_u;
            CF_V2 uv1 = uv0;
            CF_V2 uv2 = uv0;
            CF_V2 uv3 = uv0;
            
            uv1.y += uv_step.y;
            
            uv2.x += uv_step.x;
            
            uv3.x += uv_step.x;
            uv3.y += uv_step.y;
            
            chunk.verts[0] = cf_top_left(chunk_aabb);
            chunk.verts[1] = cf_bottom_left(chunk_aabb);
            chunk.verts[2] = cf_top_right(chunk_aabb);
            chunk.verts[3] = cf_bottom_right(chunk_aabb);
            
            chunk.uvs[0] = uv0;
            chunk.uvs[1] = uv1;
            chunk.uvs[2] = uv2;
            chunk.uvs[3] = uv3;
            
            chunk.opacity = opacity;
            chunk.color = color;
            
            chunk.velocity = velocity;
            
            cf_array_push(chunks, chunk);
        }
    }
}

void update_sprite_chunks(SpriteChunk* chunks)
{
    for (int index = 0; index < cf_array_count(chunks); ++index)
    {
        SpriteChunk* chunk = chunks + index;
        
        CF_V2 dp = cf_mul_v2_f(chunk->velocity, CF_DELTA_TIME);
        
        chunk->verts[0] = cf_add(chunk->verts[0], dp);
        chunk->verts[1] = cf_add(chunk->verts[1], dp);
        chunk->verts[2] = cf_add(chunk->verts[2], dp);
        chunk->verts[3] = cf_add(chunk->verts[3], dp);
    }
}

void draw_sprite_chunks(SpriteChunk* chunks)
{
    CF_M3x2 mvp = cf_draw_peek();
    mvp = cf_mul_m32(draw.projection, mvp);
    
    CF_Vertex verts[6] = {};
    
    for (int index = 0; index < cf_array_count(chunks); ++index)
    {
        SpriteChunk* chunk = chunks + index;
        
        verts[0].p = chunk->verts[0];
        verts[1].p = chunk->verts[1];
        verts[2].p = chunk->verts[2];
        verts[3].p = chunk->verts[2];
        verts[4].p = chunk->verts[1];
        verts[5].p = chunk->verts[3];
        
        verts[0].posH = cf_mul_m32_v2(mvp, chunk->verts[0]);
        verts[1].posH = cf_mul_m32_v2(mvp, chunk->verts[1]);
        verts[2].posH = cf_mul_m32_v2(mvp, chunk->verts[2]);
        verts[3].posH = cf_mul_m32_v2(mvp, chunk->verts[2]);
        verts[4].posH = cf_mul_m32_v2(mvp, chunk->verts[1]);
        verts[5].posH = cf_mul_m32_v2(mvp, chunk->verts[3]);
        
        verts[0].uv = chunk->uvs[0];
        verts[1].uv = chunk->uvs[1];
        verts[2].uv = chunk->uvs[2];
        verts[3].uv = chunk->uvs[2];
        verts[4].uv = chunk->uvs[1];
        verts[5].uv = chunk->uvs[3];
        
        verts[0].aa = false;
        verts[1].aa = false;
        verts[2].aa = false;
        verts[3].aa = false;
        verts[4].aa = false;
        verts[5].aa = false;
        
        verts[0].alpha = chunk->opacity;
        verts[1].alpha = chunk->opacity;
        verts[2].alpha = chunk->opacity;
        verts[3].alpha = chunk->opacity;
        verts[4].alpha = chunk->opacity;
        verts[5].alpha = chunk->opacity;
        
        verts[0].color = chunk->color;
        verts[1].color = chunk->color;
        verts[2].color = chunk->color;
        verts[3].color = chunk->color;
        verts[4].color = chunk->color;
        verts[5].color = chunk->color;
        
        cf_array_push(draw.verts, verts[0]);
        cf_array_push(draw.verts, verts[1]);
        cf_array_push(draw.verts, verts[2]);
        cf_array_push(draw.verts, verts[3]);
        cf_array_push(draw.verts, verts[4]);
        cf_array_push(draw.verts, verts[5]);
    }
}

int main(int argc, char *argv[])
{
    int w = 640;
    int h = 480;
    
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT;
	CF_Result result = cf_make_app("Sprite Shatter", 0, 0, 0, w, h, options, argv[0]);
	if (cf_is_error(result)) return -1;
    
    // need to know the atlas size otherwise without smooth_uv() the sprites will look blurry
    int atlas_w = 2048;
    int atlas_h = 2048;
    CF_V2 atlas_dims = cf_v2((float)atlas_w, (float)atlas_h);
    cf_draw_set_atlas_dimensions(atlas_w, atlas_h);
    
    cf_make_font_from_memory(proggy_data, proggy_sz, "ProggyClean");
    init(w, h);
    
    CF_V2 normal_position = cf_v2(-100, 0);
    CF_V2 shatter_position = cf_v2(100, 0);
    
    CF_Sprite demo_sprite = cf_make_demo_sprite();
    cf_sprite_play(&demo_sprite, "spin");
    int frame_count = cf_array_count(demo_sprite.animation->frames);
    demo_sprite.scale = cf_v2(4, 4);
    
    CF_Canvas app_canvas = cf_app_get_canvas();
    
    dyna SpriteChunk* chunks = NULL;
    cf_array_fit(chunks, 32);
    
	while (cf_app_is_running())
	{
		cf_app_update(NULL);
        
        cf_array_clear(draw.verts);
        
        if (cf_key_just_pressed(CF_KEY_R))
        {
            cf_sprite_reset(&demo_sprite);
        }
        if (cf_key_just_pressed(CF_KEY_SPACE))
        {
            cf_sprite_pause(&demo_sprite);
            cf_array_clear(chunks);
            shatter_sprite(&demo_sprite, 4, chunks);
        }
        
        cf_sprite_update(&demo_sprite);
        update_sprite_chunks(chunks);
        
        // normal
        {
            demo_sprite.transform.p = normal_position;
            cf_draw_sprite(&demo_sprite);
        }
        
        // shattered
        {
            if (demo_sprite.paused)
            {
                draw_sprite_chunks(chunks);
                
                CF_TemporaryImage image = cf_fetch_image(&demo_sprite);
                
                cf_apply_canvas(app_canvas, true);
                cf_draw_push_shader(draw.shader);
                
                // still relying on base shader so need to setup `u_image` and `u_texture_size`
                // TODO: you might have more than 1 texture so this would cause
                //       you to get bad sampling if you have a lot of sprites
                cf_material_set_texture_fs(draw.material, "u_image", image.tex);
                cf_material_set_uniform_fs(draw.material, "u_texture_size", &atlas_dims, CF_UNIFORM_TYPE_FLOAT2, 1);
                
                cf_mesh_update_vertex_data(draw.mesh, draw.verts, cf_array_count(draw.verts));
                cf_apply_mesh(draw.mesh);
                
                cf_apply_shader(draw.shader, draw.material);
                cf_draw_elements();
                
                cf_draw_pop_shader();
            }
            else
            {
                demo_sprite.transform.p = shatter_position;
                cf_draw_sprite(&demo_sprite);
                cf_render_to(app_canvas, true);
            }
        }
        
        const char* text = "Press SPACE to shatter Sprite\nR to reset";
        
        cf_push_font("ProggyClean");
        cf_push_font_size(26.0f);
        CF_V2 text_size = cf_text_size(text, -1);
        cf_draw_text(text, cf_v2(-text_size.x * 0.5f, h * -0.25f), -1);
        cf_pop_font_size();
        cf_pop_font();
        
        cf_app_draw_onto_screen(false);
    }
    
    cf_destroy_app();
    return 0;
}