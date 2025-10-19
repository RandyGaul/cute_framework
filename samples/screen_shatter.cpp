#include <cute.h>
#include "proggy.h"

#define STR(X) #X

const char* shader_str = STR(
vec4 shader(vec4 color, vec2 pos, vec2 screen_uv, vec4 params)
{
    return texture(u_image, smooth_uv(v_uv, u_texture_size));
}
);

struct
{
    CF_Shader shader;
    CF_Material material;
    CF_Canvas copy_canvas;
    CF_Mesh mesh;
    CF_M3x2 projection;
    CF_Vertex* verts;
} draw;

CF_Rnd rnd;

struct ScreenChunk
{
    CF_V2 verts[3];
    CF_V2 uvs[3];
    CF_V2 velocity;
};

void add_vertex_attribute(CF_VertexAttribute* attrs, const char* name, CF_VertexFormat format, int offset);
void init(int w, int h);
void shattered_screen(CF_V2 screen_dims, ScreenChunk* chunks);
void update_screen_chunk(ScreenChunk* chunks);
void draw_screen_chunk(ScreenChunk* chunks);

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
    
    CF_CanvasParams params = cf_canvas_defaults(w, h);
    draw.copy_canvas = cf_make_canvas(params);
    
    draw.projection = cf_ortho_2d(0, 0, (float)w, (float)h);
    cf_draw_projection(draw.projection);
    
    cf_array_free(attrs);
    
    rnd = cf_rnd_seed(0);
}

void screen_chunk(CF_V2 screen_dims, ScreenChunk* chunks)
{
    CF_V2 center = cf_v2(0, 0);
    float push_strength = 10.0f;
    
    CF_V2 screen_min = cf_neg(cf_mul(screen_dims, 0.5f));
    CF_V2 screen_max = cf_mul(screen_dims, 0.5f);
    CF_Aabb screen_aabb = cf_make_aabb(screen_min, screen_max);
    CF_V2 screen_top_left = cf_top_left(screen_aabb);
    
    CF_Aabb* aabbs = NULL;
    cf_array_fit(aabbs, 128);
    cf_array_push(aabbs, screen_aabb);
    CF_V2 min_size = cf_v2(96, 96);
    
    while (cf_array_count(aabbs) > 0)
    {
        int count = cf_array_count(aabbs);
        int index = cf_rnd_range_int(&rnd, 0, count - 1);
        
        CF_Aabb aabb = aabbs[index];
        aabbs[index] = aabbs[count - 1];
        cf_array_pop(aabbs);
        
        CF_V2 extents = cf_extents(aabb);
        CF_V2 ds = cf_sub(extents, min_size);
        bool discard = ds.x <= min_size.x && ds.y <= min_size.y;
        
        if (discard)
        {
            CF_V2 p0 = cf_top_left(aabb);
            CF_V2 p1 = cf_bottom_left(aabb);
            CF_V2 p2 = cf_top_right(aabb);
            CF_V2 p3 = cf_bottom_right(aabb);
            
            CF_V2 uv0 = cf_sub(screen_top_left, p0);
            CF_V2 uv1 = cf_sub(screen_top_left, p1);
            CF_V2 uv2 = cf_sub(screen_top_left, p2);
            CF_V2 uv3 = cf_sub(screen_top_left, p3);
            
            uv0.x = cf_abs(uv0.x / screen_dims.x);
            uv0.y = cf_abs(uv0.y / screen_dims.y);
            uv1.x = cf_abs(uv1.x / screen_dims.x);
            uv1.y = cf_abs(uv1.y / screen_dims.y);
            uv2.x = cf_abs(uv2.x / screen_dims.x);
            uv2.y = cf_abs(uv2.y / screen_dims.y);
            uv3.x = cf_abs(uv3.x / screen_dims.x);
            uv3.y = cf_abs(uv3.y / screen_dims.y);
            
            CF_V2 centroid;
            
            // left tri
            {
                ScreenChunk chunk = {};
                chunk.verts[0] = p0;
                chunk.verts[1] = p1;
                chunk.verts[2] = p2;
                
                chunk.uvs[0] = uv0;
                chunk.uvs[1] = uv1;
                chunk.uvs[2] = uv2;
                
                centroid = cf_centroid(chunk.verts, CF_ARRAY_SIZE(chunk.verts));
                chunk.velocity = cf_sub(centroid, center);
                chunk.velocity = cf_norm(chunk.velocity);
                chunk.velocity = cf_mul(chunk.velocity, push_strength);
                
                cf_array_push(chunks, chunk);
            }
            
            // right tri
            {
                ScreenChunk chunk = {};
                chunk.verts[0] = p2;
                chunk.verts[1] = p1;
                chunk.verts[2] = p3;
                
                chunk.uvs[0] = uv2;
                chunk.uvs[1] = uv1;
                chunk.uvs[2] = uv3;
                
                centroid = cf_centroid(chunk.verts, CF_ARRAY_SIZE(chunk.verts));
                chunk.velocity = cf_sub(centroid, center);
                chunk.velocity = cf_norm(chunk.velocity);
                chunk.velocity = cf_mul(chunk.velocity, push_strength);
                
                cf_array_push(chunks, chunk);
            }
        }
        else
        {
            CF_V2 new_extents = extents;
            if (ds.x > min_size.x)
            {
                new_extents.x = cf_rnd_range_float(&rnd, min_size.x, extents.x);
            }
            if (ds.y > min_size.y)
            {
                new_extents.y = cf_rnd_range_float(&rnd, min_size.y, extents.y);
            }
            
            // p0----p1--------p2
            // | q0  | q1      |
            // |     |         |
            // p3----p4--------p5
            // | q2  | q3      |
            // p6----p7--------p8
            
            CF_V2 p0 = cf_top_left(aabb);
            CF_V2 p1 = cf_v2(p0.x + new_extents.x, p0.y);
            CF_V2 p2 = cf_top_right(aabb);
            CF_V2 p3 = cf_v2(p0.x, p0.y - new_extents.y);
            CF_V2 p4 = cf_v2(p1.x, p1.y - new_extents.y);
            CF_V2 p5 = cf_v2(p2.x, p3.y);
            CF_V2 p6 = cf_bottom_left(aabb);
            CF_V2 p7 = cf_v2(p1.x, p0.y - extents.y);
            CF_V2 p8 = cf_bottom_right(aabb);
            
            CF_Aabb q0 = cf_make_aabb(p3, p1);
            CF_Aabb q1 = cf_make_aabb(p4, p2);
            CF_Aabb q2 = cf_make_aabb(p6, p4);
            CF_Aabb q3 = cf_make_aabb(p7, p5);
            
            cf_array_push(aabbs, q0);
            cf_array_push(aabbs, q1);
            cf_array_push(aabbs, q2);
            cf_array_push(aabbs, q3);
        }
    }
    
    cf_array_free(aabbs);
}

void update_screen_chunk(ScreenChunk* chunks)
{
    for (int index = 0; index < cf_array_count(chunks); ++index)
    {
        ScreenChunk* chunk = chunks + index;
        CF_V2 dp = cf_mul_v2_f(chunk->velocity, CF_DELTA_TIME);
        chunk->verts[0] = cf_add(chunk->verts[0], dp);
        chunk->verts[1] = cf_add(chunk->verts[1], dp);
        chunk->verts[2] = cf_add(chunk->verts[2], dp);
    }
}

void draw_screen_chunk(ScreenChunk* chunks)
{
    CF_M3x2 mvp = cf_draw_peek();
    mvp = cf_mul_m32(draw.projection, mvp);
    
    CF_Vertex verts[3] = {};
    
    verts[0].aa = false;
    verts[1].aa = false;
    verts[2].aa = false;
    verts[0].alpha = 255;
    verts[1].alpha = 255;
    verts[2].alpha = 255;
    
    for (int index = 0; index < cf_array_count(chunks); ++index)
    {
        ScreenChunk* chunk = chunks + index;
        
        verts[0].p = chunk->verts[0];
        verts[1].p = chunk->verts[1];
        verts[2].p = chunk->verts[2];
        
        verts[0].posH = cf_mul_m32_v2(mvp, chunk->verts[0]);
        verts[1].posH = cf_mul_m32_v2(mvp, chunk->verts[1]);
        verts[2].posH = cf_mul_m32_v2(mvp, chunk->verts[2]);
        
        verts[0].uv = chunk->uvs[0];
        verts[1].uv = chunk->uvs[1];
        verts[2].uv = chunk->uvs[2];
        
        cf_array_push(draw.verts, verts[0]);
        cf_array_push(draw.verts, verts[1]);
        cf_array_push(draw.verts, verts[2]);
    }
}

int main(int argc, char *argv[])
{
    int w = 640;
    int h = 480;
    
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT;
	CF_Result result = cf_make_app("Screen Shatter", 0, 0, 0, w, h, options, argv[0]);
	if (cf_is_error(result)) return -1;
    
    // need to know the atlas size otherwise without smooth_uv() the sprites will look blurry
    CF_V2 canvas_dims = cf_v2((float)w, (float)h);
    
    cf_make_font_from_memory(proggy_data, proggy_sz, "ProggyClean");
    init(w, h);
    
    CF_Sprite demo_sprite = cf_make_demo_sprite();
    cf_sprite_play(&demo_sprite, "spin");
    int frame_count = cf_array_count(demo_sprite.animation->frames);
    demo_sprite.scale = cf_v2(12, 12);
    
    CF_Canvas app_canvas = cf_app_get_canvas();
    
    dyna ScreenChunk* chunks = NULL;
    cf_array_fit(chunks, 1024);
    
	while (cf_app_is_running())
	{
		cf_app_update(NULL);
        
        cf_array_clear(draw.verts);
        
        if (cf_key_just_pressed(CF_KEY_R))
        {
            cf_sprite_reset(&demo_sprite);
            cf_array_clear(chunks);
        }
        if (cf_key_just_pressed(CF_KEY_SPACE))
        {
            cf_sprite_pause(&demo_sprite);
            cf_array_clear(chunks);
            
            // copy last drawn screen
            cf_draw_canvas(app_canvas, cf_v2(0, 0), canvas_dims);
            cf_render_to(draw.copy_canvas, true);
            screen_chunk(canvas_dims, chunks);
        }
        
        cf_sprite_update(&demo_sprite);
        update_screen_chunk(chunks);
        
        if (cf_array_count(chunks))
        {
            draw_screen_chunk(chunks);
            
            cf_apply_canvas(app_canvas, true);
            cf_draw_push_shader(draw.shader);
            
            cf_material_set_texture_fs(draw.material, "u_image", cf_canvas_get_target(draw.copy_canvas));
            cf_material_set_uniform_fs(draw.material, "u_texture_size", &canvas_dims, CF_UNIFORM_TYPE_FLOAT2, 1);
            
            cf_mesh_update_vertex_data(draw.mesh, draw.verts, cf_array_count(draw.verts));
            
            cf_apply_shader(draw.shader, draw.material, draw.mesh);
            cf_draw_elements(draw.mesh);
            
            cf_draw_pop_shader();
        }
        else
        {
            cf_draw_push_color(cf_color_grey());
            CF_Aabb background_aabb = cf_make_aabb(cf_neg(canvas_dims), canvas_dims);
            cf_draw_box_fill(background_aabb, 0.0f);
            cf_draw_pop_color();
            
            cf_draw_sprite(&demo_sprite);
            
            const char* text = "Press SPACE to shatter screen\nR to reset";
            
            cf_push_font("ProggyClean");
            cf_push_font_size(26.0f);
            CF_V2 text_size = cf_text_size(text, -1);
            cf_draw_text(text, cf_v2(-text_size.x * 0.5f, h * -0.35f), -1);
            cf_pop_font_size();
            cf_pop_font();
            
            cf_render_to(app_canvas, true);
        }
        
        cf_app_draw_onto_screen(false);
    }
    
    cf_destroy_app();
    return 0;
}