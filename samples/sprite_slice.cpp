#include <cute.h>
#include "proggy.h"

#define STR(X) #X

const char* shader_str = STR(
vec4 shader(vec4 color, ShaderParams params)
{
    return texture(u_image, smooth_uv(v_uv, u_texture_size));
}
);

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
    CF_Poly poly;
    CF_V2 uvs[8];
    CF_V2 velocity;
    uint8_t opacity;
    CF_Pixel color;
};

void add_vertex_attribute(CF_VertexAttribute* attrs, const char* name, CF_VertexFormat format, int offset);
void init(int w, int h);
void set_sprite_chunk(SpriteChunk* chunk, CF_Poly* poly, CF_Sprite* sprite, float push_strength);
void slice_sprite(CF_Sprite* sprite, CF_V2 start, CF_V2 end, SpriteChunk* chunks);
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
    draw.shader = cf_make_draw_shader_from_source(shader_str);
    draw.material = cf_make_material();
    
	CF_VertexAttribute* attrs = NULL;
    cf_array_fit(attrs, 32);
    
    add_vertex_attribute(attrs, "in_pos_uv",     CF_VERTEX_FORMAT_FLOAT4,      CF_OFFSET_OF(CF_Vertex, p));
    add_vertex_attribute(attrs, "in_n",          CF_VERTEX_FORMAT_INT,          CF_OFFSET_OF(CF_Vertex, n));
    add_vertex_attribute(attrs, "in_ab",         CF_VERTEX_FORMAT_FLOAT4,      CF_OFFSET_OF(CF_Vertex, shape[0]));
    add_vertex_attribute(attrs, "in_cd",         CF_VERTEX_FORMAT_FLOAT4,      CF_OFFSET_OF(CF_Vertex, shape[2]));
    add_vertex_attribute(attrs, "in_ef",         CF_VERTEX_FORMAT_FLOAT4,      CF_OFFSET_OF(CF_Vertex, shape[4]));
    add_vertex_attribute(attrs, "in_gh",         CF_VERTEX_FORMAT_FLOAT4,      CF_OFFSET_OF(CF_Vertex, shape[6]));
    add_vertex_attribute(attrs, "in_col",        CF_VERTEX_FORMAT_UBYTE4_NORM, CF_OFFSET_OF(CF_Vertex, color));
    add_vertex_attribute(attrs, "in_shape",      CF_VERTEX_FORMAT_FLOAT4,      CF_OFFSET_OF(CF_Vertex, radius));
    add_vertex_attribute(attrs, "in_blend_posH", CF_VERTEX_FORMAT_FLOAT4,      CF_OFFSET_OF(CF_Vertex, alpha));
    add_vertex_attribute(attrs, "in_user",       CF_VERTEX_FORMAT_FLOAT4,      CF_OFFSET_OF(CF_Vertex, attributes));
    add_vertex_attribute(attrs, "in_uv_bounds",  CF_VERTEX_FORMAT_FLOAT4,      CF_OFFSET_OF(CF_Vertex, uv_bounds));
    draw.mesh = cf_make_mesh(CF_MB * 5, attrs, cf_array_count(attrs), sizeof(CF_Vertex));
    
    draw.verts = NULL;
    cf_array_fit(draw.verts, 1024);
    
    draw.projection = cf_ortho_2d(0, 0, (float)w, (float)h);
    cf_draw_projection(draw.projection);
    
    cf_array_free(attrs);
}

void set_sprite_chunk(SpriteChunk* chunk, CF_Poly* poly, CF_Sprite* sprite, float push_strength)
{
    CF_V2 sprite_top_left = sprite->transform.p;
    float w = sprite->w * sprite->scale.x;
    float h = sprite->h * sprite->scale.y;
    sprite_top_left.x -= w * 0.5f;
    sprite_top_left.y += h * 0.5f;
    
    CF_TemporaryImage temporary_image = cf_fetch_image(sprite);
    CF_V2 u = cf_min(temporary_image.v, temporary_image.u);
    CF_V2 v = cf_max(temporary_image.v, temporary_image.u);
    CF_V2 duv = cf_sub(v, u);
    
    chunk->poly = *poly;
    for (int index = 0; index < chunk->poly.count; ++index)
    {
        CF_V2 uv = cf_sub(sprite_top_left, chunk->poly.verts[index]);
        uv.x = cf_abs(uv.x / w);
        uv.y = cf_abs(uv.y / h);
        uv = cf_mul(uv, duv);
        uv = cf_add(u, uv);
        
        chunk->uvs[index] = uv;
    }
    
    chunk->velocity = cf_center_of_mass(chunk->poly);
    chunk->velocity = cf_sub(chunk->velocity, sprite->transform.p);
    chunk->velocity = cf_norm(chunk->velocity);
    chunk->velocity = cf_mul(chunk->velocity, push_strength);
}

void slice_sprite(CF_Sprite* sprite, CF_V2 start, CF_V2 end, SpriteChunk* chunks)
{
    CF_Pixel color = cf_color_to_pixel(cf_draw_peek_color());
    
    CF_V2 sprite_min = sprite->transform.p;
    CF_V2 sprite_max = sprite_min;
    float w = sprite->w * sprite->scale.x;
    float h = sprite->h * sprite->scale.y;
    
    sprite_min.x -= w / 2;
    sprite_min.y -= h / 2;
    sprite_max.x += w / 2;
    sprite_max.y += h / 2;
    CF_Aabb aabb = cf_make_aabb(sprite_min, sprite_max);
    CF_V2 sprite_top_left = cf_top_left(aabb);
    
    float push_strength = 10.0f;
    
    CF_Ray ray;
    ray.p = start;
    ray.d = cf_sub(end, start);
    ray.d = cf_safe_norm(ray.d);
    ray.t = cf_distance(end, start);
    
    if (ray.t > 0)
    {
        CF_Raycast hit_result = cf_ray_to_aabb(ray, aabb);
        if (hit_result.hit)
        {
            cf_array_clear(chunks);
        
            CF_V2 hit_point = cf_mul(ray.d, hit_result.t);
            hit_point = cf_add(start, hit_point);
        
            CF_Poly sprite_poly;
            sprite_poly.verts[0] = sprite_min;
            sprite_poly.verts[1] = cf_v2(sprite_min.x, sprite_max.y);
            sprite_poly.verts[2] = cf_v2(sprite_max.x, sprite_min.y);
            sprite_poly.verts[3] = sprite_max;
            sprite_poly.count = 4;
            cf_make_poly(&sprite_poly);
        
            const float epsilon = (float)1e-4;
            CF_V2 n = cf_perp(ray.d);
        
            CF_Halfspace slice_plane = cf_plane(n, hit_point);
            CF_SliceOutput output = cf_slice(slice_plane, sprite_poly, epsilon);
        
            SpriteChunk chunk = {};
            chunk.opacity = (uint8_t)(sprite->opacity * 255.0f);
            chunk.color = color;
        
            set_sprite_chunk(&chunk, &output.front, sprite, push_strength);
            cf_array_push(chunks, chunk);
        
            set_sprite_chunk(&chunk, &output.back, sprite, push_strength);
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
        
        for (int vertex = 0; vertex < chunk->poly.count; ++vertex)
        {
            chunk->poly.verts[vertex] = cf_add(chunk->poly.verts[vertex], dp);
        }
    }
}

void draw_sprite_chunks(SpriteChunk* chunks)
{
    CF_M3x2 mvp = cf_draw_peek();
    mvp = cf_mul_m32(draw.projection, mvp);
    CF_Vertex verts[8] = {};
    
    for (int index = 0; index < cf_array_count(chunks); ++index)
    {
        SpriteChunk* chunk = chunks + index;
        
        for (int vertex = 0; vertex < chunk->poly.count; ++vertex)
        {
            CF_Vertex* vert = verts + vertex;
            vert->p = chunk->poly.verts[vertex];
            vert->posH = cf_mul_m32_v2(mvp, vert->p);
            vert->uv = chunk->uvs[vertex];
            vert->aa = false;
            vert->alpha = chunk->opacity / 255.0f;
            vert->color = chunk->color;
        }
        
        for (int vertex = 0; vertex < chunk->poly.count - 1; ++vertex)
        {
            cf_array_push(draw.verts, verts[0]);
            cf_array_push(draw.verts, verts[vertex]);
            cf_array_push(draw.verts, verts[vertex + 1]);
        }
    }
    
    for (int index = 0; index < cf_array_count(chunks); ++index)
    {
        SpriteChunk* chunk = chunks + index;
        cf_draw_polyline(chunk->poly.verts, chunk->poly.count, 1.0f, true);
    }
}

int main(int argc, char *argv[])
{
    int w = 640;
    int h = 480;
    
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT;
	CF_Result result = cf_make_app("Sprite Slice", 0, 0, 0, w, h, options, argv[0]);
	if (cf_is_error(result)) return -1;
    
    // need to know the atlas size otherwise without smooth_uv() the sprites will look blurry
    int atlas_w = 2048;
    int atlas_h = 2048;
    CF_V2 atlas_dims = cf_v2((float)atlas_w, (float)atlas_h);
    cf_draw_set_atlas_dimensions(atlas_w, atlas_h);
    
    cf_make_font_from_memory(proggy_data, proggy_sz, "ProggyClean");
    init(w, h);
    
    CF_Sprite demo_sprite = cf_make_demo_sprite();
    cf_sprite_play(&demo_sprite, "spin");
    int frame_count = cf_sprite_frame_count(&demo_sprite);
    demo_sprite.scale = cf_v2(8, 8);
    
    CF_Canvas app_canvas = cf_app_get_canvas();
    
    dyna SpriteChunk* chunks = NULL;
    cf_array_fit(chunks, 8);
    
    CF_V2 slice_start = {};
    CF_V2 slice_end = {};
    
	while (cf_app_is_running())
	{
		cf_app_update(NULL);
        
        cf_array_clear(draw.verts);
        
        // input
        {
            CF_V2 mouse = cf_v2(cf_mouse_x(), cf_mouse_y());
            
            if (cf_key_just_pressed(CF_KEY_R))
            {
                cf_array_clear(chunks);
                cf_sprite_reset(&demo_sprite);
            }
            if(cf_mouse_just_pressed(CF_MOUSE_BUTTON_LEFT))
            {
                slice_start = cf_screen_to_world(mouse);
                cf_sprite_pause(&demo_sprite);
            }
            if (cf_mouse_down(CF_MOUSE_BUTTON_LEFT))
            {
                slice_end = cf_screen_to_world(mouse);
                cf_draw_line(slice_start, slice_end, 2.0f);
            }
            if (cf_mouse_just_released(CF_MOUSE_BUTTON_LEFT))
            {
                slice_sprite(&demo_sprite, slice_start, slice_end, chunks);
                
                if (cf_array_count(chunks) == 0)
                {
                    cf_sprite_unpause(&demo_sprite);
                }
            }
        }
        
        cf_sprite_update(&demo_sprite);
        update_sprite_chunks(chunks);
        
        if (cf_array_count(chunks))
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
            CF_V2 sprite_min = demo_sprite.transform.p;
            CF_V2 sprite_max = sprite_min;
            float w = demo_sprite.w * demo_sprite.scale.x;
            float h = demo_sprite.h * demo_sprite.scale.y;
            
            sprite_min.x -= w / 2;
            sprite_min.y -= h / 2;
            sprite_max.x += w / 2;
            sprite_max.y += h / 2;
            CF_Aabb aabb = cf_make_aabb(sprite_min, sprite_max);
            
            cf_draw_box(aabb, 1.0f, 0.0f);
            
            cf_draw_sprite(&demo_sprite);
            cf_render_to(app_canvas, true);
        }
        
        const char* text = "Press Left Click to start to slice\nR to reset";
        
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