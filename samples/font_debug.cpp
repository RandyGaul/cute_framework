#include <cute.h>
#include "dcimgui.h"
#include "internal/cute_font_internal.h"
#include "proggy.h"

enum
{
    font_state_text_aabb = 1 << 0,
    font_state_glyph = 1 << 1,
};

int string_glyph_length(const char* str)
{
    int string_glyph_length = 0;
    const char* s = str;
    while (true)
    {
        int codepoint = 0;
        s = cf_decode_UTF8(s, &codepoint);
        if (codepoint == 0)
        {
            break;
        }
        string_glyph_length++;
    }
    return string_glyph_length;
}

int main(int argc, char* argv[])
{
    int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT;
    int display_index = 0;
    int x = 0;
    int y = 0;
    int w = 640;
    int h = 480;
    CF_Result result = cf_make_app("Cute Font Debug", display_index, x, y, w, h, options, argv[0]);
    if (cf_is_error(result)) return -1;

    cf_app_init_imgui();

    cf_make_font_from_memory(proggy_data, proggy_sz, cf_sintern("ProggyClean"));

    dyna const char** font_names = NULL;
    cf_array_push(font_names, cf_sintern("Calibri"));
    cf_array_push(font_names, cf_sintern("ProggyClean"));
    int font_index = 0;

    char text[1024];
    CF_SNPRINTF(text, sizeof(text), "Some Text");

    int font_state = 0;
    float font_size = 26.0f;
    float font_size_min = font_size * 0.5f;
    float font_size_max = font_size * 4.0f;
    int glyph_index = 0;
    int text_glyph_length = 0;

    int new_line_codepoint = 0;
    cf_decode_UTF8("\n", &new_line_codepoint);

    while (cf_app_is_running())
    {
        cf_app_update(NULL);

        int prev_codepoint = 0;
        int codepoint = 0;

        const char* font_name = font_names[font_index];
        CF_Font* font = cf_font_get(font_name);
        float scale = stbtt_ScaleForPixelHeight(&font->info, font_size);
        float ascent = font->ascent * scale;
        float descent = font->descent * scale;
        float line_gap = font->line_gap * scale;
        float line_height = font->line_height * scale;
        float kerning = 0;

        cf_push_font(font_name);
        cf_push_font_size(font_size);
        CF_V2 text_size = cf_text_size(text, -1);
        CF_V2 text_position = cf_v2(0, 0);
        text_position.x -= text_size.x * 0.5f;

        CF_Aabb text_aabb = cf_make_aabb_from_top_left(text_position, text_size.x, text_size.y);

        cf_draw_push_color(cf_color_white());
        cf_draw_text(text, text_position, -1);
        cf_draw_pop_color();

        if (font_state & font_state_text_aabb)
        {
            cf_draw_push_color(cf_color_grey());
            cf_draw_box(text_aabb, 0.0f, 1.0f);
            cf_draw_pop_color();
        }
        if (font_state & font_state_glyph)
        {
            int line_count = 0;
            float x_offset = 0;
            // find glyph to draw glyph metric lines
            int current_glyph_index = 0;
            {
                const char* s = text;
                while (true)
                {
                    prev_codepoint = codepoint;
                    s = cf_decode_UTF8(s, &codepoint);
                    bool is_new_line = new_line_codepoint == codepoint;

                    if (current_glyph_index == glyph_index)
                    {
                        break;
                    }

                    if (codepoint == 0)
                    {
                        break;
                    }

                    CF_Glyph* glyph = cf_font_get_glyph(font, codepoint, font_size, 0);

                    if (is_new_line)
                    {
                        line_count++;
                        x_offset = 0;
                    }
                    else
                    {
                        x_offset += glyph->xadvance;
                    }

                    current_glyph_index++;
                }
            }

            if (codepoint)
            {
                CF_Glyph* glyph = cf_font_get_glyph(font, codepoint, font_size, 0);
                kerning = cf_font_get_kern(font, font_size, prev_codepoint, codepoint);

                CF_V2 glyph_position = text_position;
                glyph_position.y -= ascent + line_count * line_height;
                glyph_position.x += x_offset;

                CF_V2 glyph_min = glyph_position;
                CF_V2 glyph_max = glyph_position;

                glyph_min = cf_add(glyph_min, glyph->q0);
                glyph_max = cf_add(glyph_max, glyph->q1);

                CF_Aabb glyph_aabb = cf_make_aabb(glyph_min, glyph_max);

                // origin
                {
                    CF_V2 p0 = cf_v2(glyph_min.x, glyph_position.y);
                    CF_V2 p1 = cf_v2(glyph_max.x, glyph_position.y);

                    cf_draw_push_color(cf_color_red());
                    cf_draw_line(p0, p1, 1.0f);
                    cf_draw_pop_color();
                }
                // ascent
                {
                    CF_V2 p0 = cf_v2(glyph_min.x, glyph_position.y + ascent);
                    CF_V2 p1 = cf_v2(glyph_max.x, glyph_position.y + ascent);

                    cf_draw_push_color(cf_color_cyan());
                    cf_draw_line(p0, p1, 1.0f);
                    cf_draw_pop_color();
                }
                // descent
                {
                    CF_V2 p0 = cf_v2(glyph_min.x, glyph_position.y + descent);
                    CF_V2 p1 = cf_v2(glyph_max.x, glyph_position.y + descent);

                    cf_draw_push_color(cf_color_orange());
                    cf_draw_line(p0, p1, 1.0f);
                    cf_draw_pop_color();
                }
                // xadvance
                {
                    // offset y a bit so it doesn't overlap with origin line
                    CF_V2 p0 = cf_v2(glyph_min.x                  , glyph_position.y - descent);
                    CF_V2 p1 = cf_v2(glyph_min.x + glyph->xadvance, glyph_position.y - descent);

                    cf_draw_push_color(cf_color_yellow());
                    cf_draw_line(p0, p1, 1.0f);
                    cf_draw_pop_color();
                }
                // line height
                {
                    CF_V2 p0 = cf_v2(glyph_min.x, glyph_position.y + descent - line_gap);
                    CF_V2 p1 = cf_v2(glyph_min.x, glyph_position.y + descent - line_gap + line_height);

                    cf_draw_push_color(cf_color_orange());
                    cf_draw_line(p0, p1, 1.0f);
                    cf_draw_pop_color();
                }
                // line gap
                {
                    CF_V2 p0 = cf_v2(glyph_min.x, glyph_position.y + descent);
                    CF_V2 p1 = cf_v2(glyph_min.x, glyph_position.y + descent - line_gap);

                    cf_draw_push_color(cf_color_green());
                    cf_draw_line(p0, p1, 1.0f);
                    cf_draw_pop_color();
                }
                if (kerning > 0 || kerning < 0)
                {
                    // offset y a bit so it doesn't overlap with origin line
                    CF_V2 p0 = cf_v2(glyph_min.x + kerning, glyph_position.y - descent);
                    CF_V2 p1 = cf_v2(glyph_min.x          , glyph_position.y - descent);

                    cf_draw_push_color(cf_color_blue());
                    cf_draw_line(p0, p1, 1.0f);
                    cf_draw_pop_color();
                }

                cf_draw_push_color(cf_color_grey());
                cf_draw_box(glyph_aabb, 0.0f, 1.0f);
                cf_draw_pop_color();
            }
        }

        cf_pop_font_size();
        cf_pop_font();

        ImGui_Begin("Font", 0, ImGuiWindowFlags_None);
        {
            ImGui_BeginChild("##font_input", { 250.0f, 0.0f }, ImGuiChildFlags_None, ImGuiWindowFlags_None);
            {
                ImGui_InputTextMultilineEx("Text", text, sizeof(text), {250.0f, 100.0f}, ImGuiInputTextFlags_None, NULL, NULL);
                ImGui_ListBox("Font", &font_index, font_names, cf_array_count(font_names), 2);

                ImGui_CheckboxFlagsIntPtr("Text Aabb", &font_state, font_state_text_aabb);
                ImGui_CheckboxFlagsIntPtr("Glyph", &font_state, font_state_glyph);

                text_glyph_length = string_glyph_length(text);

                ImGui_SliderIntEx("Glyph Index", &glyph_index, 0, text_glyph_length - 1, "%d", ImGuiSliderFlags_ClampOnInput);
                ImGui_SliderFloatEx("Font Size", &font_size, font_size_min, font_size_max, "%.0f", ImGuiSliderFlags_ClampOnInput);
            }
            ImGui_EndChild();

            ImGui_SameLineEx(0, 5);

            ImGui_BeginChild("##font_info", { 0 }, ImGuiChildFlags_None, ImGuiWindowFlags_None);
            {
                ImGui_Text("Ascent: %.2f", ascent);
                ImGui_Text("Descent: %.2f", descent);
                ImGui_Text("Line Height: %.2f", line_height);
                ImGui_Text("Line Gap: %.2f", line_gap);
                ImGui_Text("Kerning: %.2f", kerning);
            }
            ImGui_EndChild();
        }
        ImGui_End();

        cf_app_draw_onto_screen(true);
    }

    cf_destroy_app();

    return 0;
}