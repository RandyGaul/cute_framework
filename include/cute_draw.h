/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_DRAW_H
#define CF_DRAW_H

#include "cute_defines.h"
#include "cute_math.h"
#include "cute_result.h"
#include "cute_graphics.h"
#include "cute_sprite.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @function cf_draw_sprite
 * @category draw
 * @brief    Draws a sprite.
 * @param    sprite     The sprite.
 * @related  cf_draw_sprite cf_draw_quad camera_look_at cf_draw_to cf_app_draw_onto_screen
 */
CF_API void CF_CALL cf_draw_sprite(const CF_Sprite* sprite);

/**
 * @function cf_draw_quad
 * @category draw
 * @brief    Draws a quad wireframe.
 * @param    bb         The AABB (Axis-Aligned Bounding Box) to draw a quad over.
 * @param    thickness  The thickness of each line to draw.
 * @param    chubbiness Inflates the shape, similar to corner-rounding. Makes the shape chubbier.
 * @related  cf_draw_quad cf_draw_quad2 cf_draw_quad_fill cf_draw_quad_fill2
 */
CF_API void CF_CALL cf_draw_quad(CF_Aabb bb, float thickness, float chubbiness);

/**
 * @function cf_draw_quad2
 * @category draw
 * @brief    Draws a quad wireframe.
 * @param    p0         A corner of the quad.
 * @param    p1         A corner of the quad.
 * @param    p2         A corner of the quad.
 * @param    p3         A corner of the quad.
 * @param    thickness  The thickness of each line to draw.
 * @param    chubbiness Inflates the shape, similar to corner-rounding. Makes the shape chubbier.
 * @remarks  All points `p0` through `p3` are encouraged to be in counter-clockwise order.
 * @related  cf_draw_quad cf_draw_quad2 cf_draw_quad_fill cf_draw_quad_fill2
 */
CF_API void CF_CALL cf_draw_quad2(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, float thickness, float chubbiness);

/**
 * @function cf_draw_quad_fill
 * @category draw
 * @brief    Draws a quad.
 * @param    bb         The AABB (Axis-Aligned Bounding Box) to draw a quad over.
 * @param    chubbiness Inflates the shape, similar to corner-rounding. Makes the shape chubbier.
 * @related  cf_draw_quad cf_draw_quad2 cf_draw_quad_fill cf_draw_quad_fill2
 */
CF_API void CF_CALL cf_draw_quad_fill(CF_Aabb bb, float chubbiness);

/**
 * @function cf_draw_quad_fill2
 * @category draw
 * @brief    Draws a quad.
 * @param    p0         A corner of the quad.
 * @param    p1         A corner of the quad.
 * @param    p2         A corner of the quad.
 * @param    p3         A corner of the quad.
 * @param    chubbiness Inflates the shape, similar to corner-rounding. Makes the shape chubbier.
 * @remarks  All points `p0` through `p3` are encouraged to be in counter-clockwise order.
 * @related  cf_draw_quad cf_draw_quad2 cf_draw_quad_fill cf_draw_quad_fill2
 */
CF_API void CF_CALL cf_draw_quad_fill2(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, float chubbiness);

/**
* @function cf_draw_box
* @category draw
* @brief    Draws a quad wireframe.
* @param    bb         The AABB (Axis-Aligned Bounding Box) to draw a quad over.
* @param    thickness  The thickness of each line to draw.
* @param    chubbiness Inflates the shape, similar to corner-rounding. Makes the shape chubbier.
* @remarks  This is an alias for `cf_draw_quad`
* @related  cf_draw_quad cf_draw_quad2 cf_draw_quad_fill cf_draw_quad_fill2
*/
CF_INLINE void cf_draw_box(CF_Aabb bb, float thickness, float chubbiness) { cf_draw_quad(bb, thickness, chubbiness); }


/**
* @function cf_draw_box2
* @category draw
* @brief    Draws a quad wireframe.
* @param    p0         A corner of the quad.
* @param    p1         A corner of the quad.
* @param    p2         A corner of the quad.
* @param    p3         A corner of the quad.
* @param    thickness  The thickness of each line to draw.
* @param    chubbiness Inflates the shape, similar to corner-rounding. Makes the shape chubbier.
* @remarks  All points `p0` through `p3` are encouraged to be in counter-clockwise order. This is an alias for `cf_draw_quad2`
* @related  cf_draw_quad cf_draw_quad2 cf_draw_quad_fill cf_draw_quad_fill2
*/
CF_INLINE void cf_draw_box2(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, float thickness, float chubbiness) { cf_draw_quad2(p0, p1, p2, p3, thickness,  chubbiness); }

/**
* @function cf_draw_box_fill
* @category draw
* @brief    Draws a quad.
* @param    bb         The AABB (Axis-Aligned Bounding Box) to draw a quad over.
* @param    chubbiness Inflates the shape, similar to corner-rounding. Makes the shape chubbier.
* @remarks  This is an alias for `cf_draw_quad_fill`
* @related  cf_draw_quad cf_draw_quad2 cf_draw_quad_fill cf_draw_quad_fill2
*/
CF_INLINE void cf_draw_box_fill(CF_Aabb bb, float chubbiness) { cf_draw_quad_fill(bb, chubbiness); }

/**
* @function cf_draw_box_fill2
* @category draw
* @brief    Draws a quad.
* @param    p0         A corner of the quad.
* @param    p1         A corner of the quad.
* @param    p2         A corner of the quad.
* @param    p3         A corner of the quad.
* @param    chubbiness Inflates the shape, similar to corner-rounding. Makes the shape chubbier.
* @remarks  All points `p0` through `p3` are encouraged to be in counter-clockwise order. This is an alias for `cf_draw_quad_fill2`
* @related  cf_draw_quad cf_draw_quad2 cf_draw_quad_fill cf_draw_quad_fill2
*/
CF_INLINE void cf_draw_box_fill2(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, float chubbiness) { cf_draw_quad_fill2(p0, p1, p2, p3, chubbiness); }

/**
 * @function cf_draw_circle
 * @category draw
 * @brief    Draws a circle wireframe.
 * @param    circle     The circle.
 * @param    thickness  The thickness of each line to draw.
 * @related  cf_draw_circle cf_draw_circle2 cf_draw_circle_fill cf_draw_circle_fill2 cf_draw_arc
 */
CF_API void CF_CALL cf_draw_circle(CF_Circle circle, float thickness);

/**
 * @function cf_draw_circle2
 * @category draw
 * @brief    Draws a circle wireframe.
 * @param    p          Center of the circle.
 * @param    r          Radius of the circle.
 * @param    thickness  The thickness of each line to draw.
 * @related  cf_draw_circle cf_draw_circle2 cf_draw_circle_fill cf_draw_circle_fill2 cf_draw_arc
 */
CF_API void CF_CALL cf_draw_circle2(CF_V2 p, float r, float thickness);

/**
 * @function cf_draw_circle_fill
 * @category draw
 * @brief    Draws a circle.
 * @param    circle     The circle.
 * @related  cf_draw_circle cf_draw_circle2 cf_draw_circle_fill cf_draw_circle_fill2 cf_draw_arc
 */
CF_API void CF_CALL cf_draw_circle_fill(CF_Circle circle);

/**
 * @function cf_draw_circle_fill2
 * @category draw
 * @brief    Draws a circle.
 * @param    p          Center of the circle.
 * @param    r          Radius of the circle.
 * @related  cf_draw_circle cf_draw_circle2 cf_draw_circle_fill cf_draw_circle_fill2 cf_draw_arc
 */
CF_API void CF_CALL cf_draw_circle_fill2(CF_V2 p, float r);

/**
 * @function cf_draw_arc
 * @category draw
 * @brief    Draws an arc of a circle wireframe.
 * @param    p              Center of the arc.
 * @param    center_of_arc  Radius of the circle.
 * @param    range          Angle the arc covers.
 * @param    iters          Number of edges used for the circle. More looks smoother, but renders slower.
 * @param    thickness      The thickness of each line to draw.
 * @related  cf_draw_circle cf_draw_circle2 cf_draw_circle_fill cf_draw_circle_fill2 cf_draw_arc
 */
CF_API void CF_CALL cf_draw_arc(CF_V2 p, CF_V2 center_of_arc, float range, int iters, float thickness);

/**
 * @function cf_draw_capsule
 * @category draw
 * @brief    Draws a capsule wireframe.
 * @param    capsule    The capsule.
 * @param    thickness  The thickness of each line to draw.
 * @related  cf_draw_capsule cf_draw_capsule2 cf_draw_capsule_fill cf_draw_capsule_fill2
 */
CF_API void CF_CALL cf_draw_capsule(CF_Capsule capsule, float thickness);

/**
 * @function cf_draw_capsule2
 * @category draw
 * @brief    Draws a capsule wireframe.
 * @param    p0         An endpoint of the interior line-segment of the capsule (the center of one end-cap).
 * @param    p1         An endpoint of the interior line-segment of the capsule (the center of one end-cap).
 * @param    r          Radius of the capsule.
 * @param    thickness  The thickness of each line to draw.
 * @related  cf_draw_capsule cf_draw_capsule2 cf_draw_capsule_fill cf_draw_capsule_fill2
 */
CF_API void CF_CALL cf_draw_capsule2(CF_V2 p0, CF_V2 p1, float r, float thickness);

/**
 * @function cf_draw_capsule_fill
 * @category draw
 * @brief    Draws a capsule.
 * @param    capsule    The capsule.
 * @related  cf_draw_capsule cf_draw_capsule2 cf_draw_capsule_fill cf_draw_capsule_fill2
 */
CF_API void CF_CALL cf_draw_capsule_fill(CF_Capsule capsule);

/**
 * @function cf_draw_capsule_fill2
 * @category draw
 * @brief    Draws a capsule.
 * @param    p0         An endpoint of the interior line-segment of the capsule (the center of one end-cap).
 * @param    p1         An endpoint of the interior line-segment of the capsule (the center of one end-cap).
 * @param    r          Radius of the capsule.
 * @related  cf_draw_capsule cf_draw_capsule2 cf_draw_capsule_fill cf_draw_capsule_fill2
 */
CF_API void CF_CALL cf_draw_capsule_fill2(CF_V2 p0, CF_V2 p1, float r);

/**
 * @function cf_draw_tri
 * @category draw
 * @brief    Draws a triangle wireframe.
 * @param    p0         A corner of the triangle.
 * @param    p1         A corner of the triangle.
 * @param    p2         A corner of the triangle.
 * @param    thickness  The thickness of each line to draw.
 * @param    chubbiness Inflates the shape, similar to corner-rounding. Makes the shape chubbier.
 * @related  cf_draw_tri cf_draw_tri_fill
 */
CF_API void CF_CALL cf_draw_tri(CF_V2 p0, CF_V2 p1, CF_V2 p2, float thickness, float chubbiness);

/**
 * @function cf_draw_tri_fill
 * @category draw
 * @brief    Draws a triangle.
 * @param    p0         A corner of the triangle.
 * @param    p1         A corner of the triangle.
 * @param    p2         A corner of the triangle.
 * @param    chubbiness Inflates the shape, similar to corner-rounding. Makes the shape chubbier.
 * @related  cf_draw_tri cf_draw_tri_fill
 */
CF_API void CF_CALL cf_draw_tri_fill(CF_V2 p0, CF_V2 p1, CF_V2 p2, float chubbiness);

/**
 * @function cf_draw_line
 * @category draw
 * @brief    Draws a line.
 * @param    p0         An endpoint of the line.
 * @param    p1         An endpoint of the line.
 * @param    thickness  The thickness of the line to draw.
 * @related  cf_draw_line cf_draw_polyline cf_draw_bezier_line cf_draw_bezier_line2 cf_draw_arrow
 */
CF_API void CF_CALL cf_draw_line(CF_V2 p0, CF_V2 p1, float thickness);

/**
 * @function cf_draw_polyline
 * @category draw
 * @brief    Draws a chain of connected line segments.
 * @param    points       An array of line segment endpoints.
 * @param    count        The number of points in the polyline.
 * @param    thickness    The thickness of the line to draw.
 * @param    loop         True to connect the first and last point to form a loop. False otherwise.
 * @param    bevel_count  The number of edges used to smooth corners.
 * @related  cf_draw_line cf_draw_polyline cf_draw_bezier_line cf_draw_bezier_line2 cf_draw_arrow
 */
CF_API void CF_CALL cf_draw_polyline(CF_V2* points, int count, float thickness, bool loop);

/**
 * @function cf_draw_bezier_line
 * @category draw
 * @brief    Draws line segments over a quadratic bezier line.
 * @param    a          The starting point.
 * @param    c0         A bezier control point.
 * @param    b          The end point.
 * @param    thickness  The thickness of the line to draw.
 * @param    iters      The number of lines used to draw the bezier spline.
 * @related  cf_draw_line cf_draw_polyline cf_draw_bezier_line cf_draw_bezier_line2 cf_draw_arrow
 */
CF_API void CF_CALL cf_draw_bezier_line(CF_V2 a, CF_V2 c0, CF_V2 b, int iters, float thickness);

/**
 * @function cf_draw_bezier_line2
 * @category draw
 * @brief    Draws line segments over a cubic bezier line.
 * @param    a          The starting point.
 * @param    c0         A bezier control point.
 * @param    c1         A bezier control point.
 * @param    b          The end point.
 * @param    thickness  The thickness of the line to draw.
 * @param    iters      The number of lines used to draw the bezier spline.
 * @related  cf_draw_line cf_draw_polyline cf_draw_bezier_line cf_draw_bezier_line2 cf_draw_arrow
 */
CF_API void CF_CALL cf_draw_bezier_line2(CF_V2 a, CF_V2 c0, CF_V2 c1, CF_V2 b, int iters, float thickness);

/**
 * @function cf_draw_arrow
 * @category draw
 * @brief    Draws an arrow.
 * @param    a            The starting point.
 * @param    b            The end point.
 * @param    thickness    The thickness of the line to draw.
 * @param    arrow_width  The width of the arrow to draw.
 * @remarks  This function is intended only for debug purposes. It's implemented in naive way so the
 *           arrow shaft will overdraw atop the arrow head. This will become visible if the arrow is
 *           drawn with any transparency.
 * @related  cf_draw_line cf_draw_polyline cf_draw_bezier_line cf_draw_bezier_line2 cf_draw_arrow
 */
CF_API void CF_CALL cf_draw_arrow(CF_V2 a, CF_V2 b, float thickness, float arrow_width);

/**
 * @function cf_draw_push_layer
 * @category draw
 * @brief    Pushes a draw layer.
 * @param    layer      The layer.
 * @remarks  Draw layers are sorted before rendering. Lower numbers are rendered fast, while larger numbers are rendered last.
 *           This can be used to pick which sprites/shapes should draw on top of each other.
 * @related  cf_draw_push_layer cf_draw_pop_layer cf_draw_peek_layer
 */
CF_API void CF_CALL cf_draw_push_layer(int layer);

/**
 * @function cf_draw_pop_layer
 * @category draw
 * @brief    Pops and returns the last draw layer.
 * @remarks  Draw layers are sorted before rendering. Lower numbers are rendered fast, while larger numbers are rendered last.
 *           This can be used to pick which sprites/shapes should draw on top of each other.
 * @related  cf_draw_push_layer cf_draw_pop_layer cf_draw_peek_layer
 */
CF_API int CF_CALL cf_draw_pop_layer();

/**
 * @function cf_draw_peek_layer
 * @category draw
 * @brief    Returns the last draw layer.
 * @remarks  Draw layers are sorted before rendering. Lower numbers are rendered fast, while larger numbers are rendered last.
 *           This can be used to pick which sprites/shapes should draw on top of each other.
 * @related  cf_draw_push_layer cf_draw_pop_layer cf_draw_peek_layer
 */
CF_API int CF_CALL cf_draw_peek_layer();

/**
 * @function cf_draw_push_color
 * @category draw
 * @brief    Pushes a draw color.
 * @param    c          The color.
 * @remarks  Various draw functions do not specify a color. In these cases, the last color pushed will be used.
 * @related  cf_draw_push_color cf_draw_pop_color cf_draw_peek_color
 */
CF_API void CF_CALL cf_draw_push_color(CF_Color c);

/**
 * @function cf_draw_pop_color
 * @category draw
 * @brief    Pops and returns the last draw color.
 * @remarks  Various draw functions do not specify a color. In these cases, the last color pushed will be used.
 * @related  cf_draw_push_color cf_draw_pop_color cf_draw_peek_color
 */
CF_API CF_Color CF_CALL cf_draw_pop_color();

/**
 * @function cf_draw_peek_color
 * @category draw
 * @brief    Returns the last draw color.
 * @remarks  Various draw functions do not specify a color. In these cases, the last color pushed will be used.
 * @related  cf_draw_push_color cf_draw_pop_color cf_draw_peek_color
 */
CF_API CF_Color CF_CALL cf_draw_peek_color();

/**
 * @function cf_draw_push_tint
 * @category draw
 * @brief    Pushes a tint color.
 * @param    c          The color.
 * @remarks  Sprites and shapes can be tinted. This is useful for certain effects such as damage flashes, or
 *           dynamic color variations. Tint is implemented under the hood with an overlay operation. If you want
 *           to push a no-op, use `cf_color_grey` to apply no tinting at all.
 * @related  cf_draw_push_tint cf_draw_pop_tint cf_draw_peek_tint
 */
CF_API void CF_CALL cf_draw_push_tint(CF_Color c);

/**
 * @function cf_draw_pop_tint
 * @category draw
 * @brief    Pops and returns the last tint color.
 * @remarks  Sprites and shapes can be tinted. This is useful for certain effects such as damage flashes.
 *           Tint is implemented under the hood with an overlay operation. If you want to push a no-op, use
 *           `cf_color_grey` to apply no tinting at all.
 * @related  cf_draw_push_tint cf_draw_pop_tint cf_draw_peek_tint
 */
CF_API CF_Color CF_CALL cf_draw_pop_tint();

/**
 * @function cf_draw_peek_tint
 * @category draw
 * @brief    Returns the last tint color.
 * @remarks  Sprites and shapes can be tinted. This is useful for certain effects such as damage flashes.
 * @related  cf_draw_push_tint cf_draw_pop_tint cf_draw_peek_tint
 */
CF_API CF_Color CF_CALL cf_draw_peek_tint();

/**
 * @function cf_draw_push_antialias
 * @category draw
 * @brief    Pushes whether or not to apply antialiasing.
 * @param    antialias  True to antialias, false otherwise.
 * @remarks  Various shape drawing functions can be drawn in antialiased mode, or in plain mode. Antialiasing is slightly slower,
 *           but looks much smoother.
 * @related  cf_draw_push_antialias cf_draw_pop_antialias cf_draw_peek_antialias
 */
CF_API void CF_CALL cf_draw_push_antialias(bool antialias);

/**
 * @function cf_draw_pop_antialias
 * @category draw
 * @brief    Pops and returns the last antialias state.
 * @remarks  Various shape drawing functions can be drawn in antialiased mode, or in plain mode. Antialiasing is slightly slower,
 *           but looks much smoother.
 * @related  cf_draw_push_antialias cf_draw_pop_antialias cf_draw_peek_antialias
 */
CF_API bool CF_CALL cf_draw_pop_antialias();

/**
 * @function cf_draw_peek_antialias
 * @category draw
 * @brief    Returns the last antialias state.
 * @remarks  Various shape drawing functions can be drawn in antialiased mode, or in plain mode. Antialiasing is slightly slower,
 *           but looks much smoother.
 * @related  cf_draw_push_antialias cf_draw_pop_antialias cf_draw_peek_antialias
 */
CF_API bool CF_CALL cf_draw_peek_antialias();

/**
 * @function cf_draw_push_antialias_scale
 * @category draw
 * @brief    Returns the last antialias scale.
 * @remarks  Antialias scale controls how much antialiasing will be used. A larger number makes the borders of shapes blurry.
 *           The number must be greater than 0, but probably not more than 2 or 3 for most cases. The default is 1.5.
 * @related  cf_draw_push_antialias_scale cf_draw_pop_antialias_scale cf_draw_peek_antialias_scale
 */
CF_API void CF_CALL cf_draw_push_antialias_scale(float scale);

/**
 * @function cf_draw_pop_antialias_scale
 * @category draw
 * @brief    Pops and returns the last antialias scale.
 * @remarks  Antialias scale controls how much antialiasing will be used. A larger number makes the borders of shapes blurry.
 *           The number must be greater than 0, but probably not more than 2 or 3 for most cases. The default is 1.5.
 * @related  cf_draw_push_antialias_scale cf_draw_pop_antialias_scale cf_draw_peek_antialias_scale
 */
CF_API float CF_CALL cf_draw_pop_antialias_scale();

/**
 * @function cf_draw_peek_antialias_scale
 * @category draw
 * @brief    Returns the last antialias scale.
 * @remarks  Antialias scale controls how much antialiasing will be used. A larger number makes the borders of shapes blurry.
 *           The number must be greater than 0, but probably not more than 2 or 3 for most cases. The default is 1.5.
 * @related  cf_draw_push_antialias_scale cf_draw_pop_antialias_scale cf_draw_peek_antialias_scale
 */
CF_API float CF_CALL cf_draw_peek_antialias_scale();

/**
 * @function cf_draw_push_vertex_attributes
 * @category draw
 * @brief    Pushes a set of vertex parameters.
 * @related  TODO
 */
CF_API void CF_CALL cf_draw_push_vertex_attributes(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

/**
 * @function cf_draw_push_vertex_attributes2
 * @category draw
 * @brief    TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_draw_push_vertex_attributes2(CF_Pixel attributes);

/**
 * @function cf_draw_pop_vertex_attributes
 * @category draw
 * @brief    TODO
 * @related  TODO
 */
CF_API CF_Pixel CF_CALL cf_draw_pop_vertex_attributes();

/**
 * @function cf_draw_peek_vertex_attributes
 * @category draw
 * @brief    TODO
 * @related  TODO
 */
CF_API CF_Pixel CF_CALL cf_draw_peek_vertex_attributes();

/**
 * @function cf_make_font
 * @category text
 * @brief    Constructs a font for rendering text.
 * @param    path        A virtual path to the font file. See [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @param    font_name   A unique name for this font. Used by `cf_push_font` and friends.
 * @return   Returns any errors as `CF_Result`.
 * @remarks  Memory is only consumed when you draw a certain glyph (text character). Just loading up the font initially is
 *           a low-cost operation. You may load up many fonts with low overhead. Please note that bold, italic, etc. are actually
 *           _different fonts_ and each must be loaded up individually.
 * @related  cf_make_font cf_make_font_from_memory cf_destroy_font cf_push_font cf_push_font_size cf_push_font_blur cf_draw_text
 */
CF_API CF_Result CF_CALL cf_make_font(const char* path, const char* font_name);

/**
 * @function cf_make_font_from_memory
 * @category text
 * @brief    Constructs a font for rendering text from memory.
 * @param    data        A buffer containing the bytes of a font file in memory.
 * @param    size        The size of `data` in bytes.
 * @param    font_name   A unique name for this font. Used by `cf_push_font` and friends.
 * @return   Returns any errors as `CF_Result`.
 * @remarks  Memory is only consumed when you draw a certain glyph (text character). Just loading up the font initially is
 *           a low-cost operation. You may load up many fonts with low overhead. Please note that bold, italic, etc. are actually
 *           _different fonts_ and each must be loaded up individually.
 * @related  cf_make_font cf_make_font_from_memory cf_destroy_font cf_push_font cf_push_font_size cf_push_font_blur cf_draw_text
 */
CF_API CF_Result CF_CALL cf_make_font_from_memory(void* data, int size, const char* font_name);

/**
 * @function cf_destroy_font
 * @category text
 * @brief    Destroys a font previously made by `cf_make_font` or `cf_make_font_from_memory`.
 * @param    font_name   The unique name for this font.
 * @related  cf_make_font cf_make_font_from_memory cf_destroy_font cf_push_font cf_push_font_size cf_push_font_blur cf_draw_text
 */
CF_API void CF_CALL cf_destroy_font(const char* font_name);

/**
 * @function cf_font_add_backup_codepoints
 * @category text
 * @brief    When drawing text, and missing glyphs from the font will be replaced by any backup codepoints.
 * @param    font_name   The unique name for this font.
 * @param    codepoints  An array of backup codepoints. Highest priority comes first in the array.
 * @param    count       The number of elements in `codepoints`.
 * @related  cf_draw_text
 */
CF_API void CF_CALL cf_font_add_backup_codepoints(const char* font_name, int* codepoints, int count);

/**
 * @function cf_push_font
 * @category text
 * @brief    Pushes a font to use for text drawing.
 * @param    font_name   The unique name for this font.
 * @related  cf_make_font cf_push_font cf_pop_font cf_peek_font cf_push_font_size cf_push_font_blur cf_draw_text
 */
CF_API void CF_CALL cf_push_font(const char* font);

/**
 * @function cf_pop_font
 * @category text
 * @brief    Pops and returns the last font name used.
 * @related  cf_make_font cf_push_font cf_pop_font cf_peek_font cf_push_font_size cf_push_font_blur cf_draw_text
 */
CF_API const char* CF_CALL cf_pop_font();

/**
 * @function cf_peek_font
 * @category text
 * @brief    Returns the last font name used.
 * @related  cf_make_font cf_push_font cf_pop_font cf_peek_font cf_push_font_size cf_push_font_blur cf_draw_text
 */
CF_API const char* CF_CALL cf_peek_font();

/**
 * @function cf_push_font_size
 * @category text
 * @brief    Pushes a font size to use for text drawing.
 * @param    size       The size to use for text drawing.
 * @related  cf_make_font cf_push_font cf_push_font_size cf_pop_font_size cf_peek_font_size cf_push_font_blur cf_draw_text
 */
CF_API void CF_CALL cf_push_font_size(float size);

/**
 * @function cf_pop_font_size
 * @category text
 * @brief    Pops and returns the last font size.
 * @related  cf_make_font cf_push_font cf_push_font_size cf_pop_font_size cf_peek_font_size cf_push_font_blur cf_draw_text
 */
CF_API float CF_CALL cf_pop_font_size();

/**
 * @function cf_peek_font_size
 * @category text
 * @brief    Returns the last font size.
 * @related  cf_make_font cf_push_font cf_push_font_size cf_pop_font_size cf_peek_font_size cf_push_font_blur cf_draw_text
 */
CF_API float CF_CALL cf_peek_font_size();

/**
 * @function cf_push_font_blur
 * @category text
 * @brief    Pushes a font blur to use for text drawing.
 * @param    blur       The blur to use for text drawing.
 * @related  cf_make_font cf_push_font cf_push_font_size cf_push_font_blur cf_pop_font_blur cf_peek_font_blur cf_draw_text
 */
CF_API void CF_CALL cf_push_font_blur(int blur);

/**
 * @function cf_pop_font_blur
 * @category text
 * @brief    Pops and returns the last font blur.
 * @related  cf_make_font cf_push_font cf_push_font_size cf_push_font_blur cf_pop_font_blur cf_peek_font_blur cf_draw_text
 */
CF_API int CF_CALL cf_pop_font_blur();

/**
 * @function cf_peek_font_blur
 * @category text
 * @brief    Returns the last font blur.
 * @related  cf_make_font cf_push_font cf_push_font_size cf_push_font_blur cf_pop_font_blur cf_peek_font_blur cf_draw_text
 */
CF_API int CF_CALL cf_peek_font_blur();

/**
 * @function cf_push_text_wrap_width
 * @category text
 * @brief    Pushes a text wrap width to use for text drawing.
 * @param    width      The text wrap width to use for text drawing.
 * @related  cf_make_font cf_push_font cf_push_text_wrap_width cf_pop_text_wrap_width cf_peek_text_wrap_width cf_push_text_clip_box cf_draw_text
 */
CF_API void CF_CALL cf_push_text_wrap_width(float width);

/**
 * @function cf_pop_text_wrap_width
 * @category text
 * @brief    Pops and returns the last text wrap width.
 * @related  cf_make_font cf_push_font cf_push_text_wrap_width cf_pop_text_wrap_width cf_peek_text_wrap_width cf_push_text_clip_box cf_draw_text
 */
CF_API float CF_CALL cf_pop_text_wrap_width();

/**
 * @function cf_peek_text_wrap_width
 * @category text
 * @brief    Returns the last text wrap width.
 * @related  cf_make_font cf_push_font cf_push_text_wrap_width cf_pop_text_wrap_width cf_peek_text_wrap_width cf_push_text_clip_box cf_draw_text
 */
CF_API float CF_CALL cf_peek_text_wrap_width();

/**
 * @function cf_push_text_clip_box
 * @category text
 * @brief    Pushes a text clip box to use for text drawing.
 * @param    clip_box   The text clip box to use for text drawing.
 * @related  cf_make_font cf_push_font cf_push_text_wrap_width cf_push_text_clip_box cf_pop_text_clip_box cf_peek_text_clip_box cf_draw_text
 */
CF_API void CF_CALL cf_push_text_clip_box(CF_Aabb clip_box);

/**
 * @function cf_pop_text_clip_box
 * @category text
 * @brief    Pops and returns the last text clip box.
 * @related  cf_make_font cf_push_font cf_push_text_wrap_width cf_push_text_clip_box cf_pop_text_clip_box cf_peek_text_clip_box cf_draw_text
 */
CF_API CF_Aabb CF_CALL cf_pop_text_clip_box();

/**
 * @function cf_peek_text_clip_box
 * @category text
 * @brief    Returns the last text clip box.
 * @related  cf_make_font cf_push_font cf_push_text_wrap_width cf_push_text_clip_box cf_pop_text_clip_box cf_peek_text_clip_box cf_draw_text
 */
CF_API CF_Aabb CF_CALL cf_peek_text_clip_box();

/**
 * @function cf_push_text_vertical_layout
 * @category text
 * @brief    Pushes a whether or not to layout text vertically (as opposed to the default or horizontally).
 * @param    layout_vertically  True to layout vertically, false otherwise.
 * @related  cf_make_font cf_push_font cf_push_text_vertical_layout cf_pop_text_vertical_layout cf_peek_text_vertical_layout cf_draw_text
 */
CF_API void CF_CALL cf_push_text_vertical_layout(bool layout_vertically);

/**
 * @function cf_pop_text_vertical_layout
 * @category text
 * @brief    Pops and returns the last vertical layout state.
 * @related  cf_make_font cf_push_font cf_push_text_vertical_layout cf_pop_text_vertical_layout cf_peek_text_vertical_layout cf_draw_text
 */
CF_API bool CF_CALL cf_pop_text_vertical_layout();

/**
 * @function cf_peek_text_vertical_layout
 * @category text
 * @brief    Returns the last vertical layout state.
 * @related  cf_make_font cf_push_font cf_push_text_vertical_layout cf_pop_text_vertical_layout cf_peek_text_vertical_layout cf_draw_text
 */
CF_API bool CF_CALL cf_peek_text_vertical_layout();

/**
 * @function cf_text_width
 * @category text
 * @brief    Returns the width of a text given the currently pushed font.
 * @param    text      The text considered for rendering.
 * @param    num_chars_to_draw  The number of characters to draw `text`. Use -1 to draw the whole string.
 * @related  cf_make_font cf_text_width cf_text_height cf_draw_text cf_text_size
 */
CF_API float CF_CALL cf_text_width(const char* text, int num_chars_to_draw);

/**
 * @function cf_text_height
 * @category text
 * @brief    Returns the height of a text given the currently pushed font.
 * @param    text      The text considered for rendering.
 * @param    num_chars_to_draw  The number of characters to draw `text`. Use -1 to draw the whole string.
 * @related  cf_make_font cf_text_width cf_text_height cf_draw_text cf_text_size
 */
CF_API float CF_CALL cf_text_height(const char* text, int num_chars_to_draw);

/**
 * @function cf_text_size
 * @category text
 * @brief    Returns the width/height of a text given the currently pushed font.
 * @param    text      The text considered for rendering.
 * @param    num_chars_to_draw  The number of characters to draw `text`. Use -1 to draw the whole string.
 * @remarks  This function is slightly superior to `cf_text_width` or `cf_text_height` if you need both width/height, as
 *           it will run the layout code only a single time.
 * @related  cf_make_font cf_text_width cf_text_height cf_draw_text cf_text_size
 */
CF_API CF_V2 CF_CALL cf_text_size(const char* text, int num_chars_to_draw);

/**
 * @function cf_draw_text
 * @category text
 * @brief    Draws text.
 * @param    text               The text to draw.
 * @param    position           The top-left corner of the text.
 * @param    num_chars_to_draw  The number of characters to draw `text`. Use -1 to draw the whole string.
 * @remarks  `num_chars_to_draw` is a great way to control how many characters to draw for implementing a typewriter style effect.
 * @related  cf_make_font cf_draw_text cf_text_effect_register cf_draw_to cf_app_draw_onto_screen
 */
CF_API void CF_CALL cf_draw_text(const char* text, CF_V2 position, int num_chars_to_draw /*= -1*/);

/**
 * @struct   CF_TextEffect
 * @category text
 * @brief    A user-defined text effect that can be triggered with text codes.
 * @example > Quick example listing some valid strings using text effects. These are all built-in text effects, and not user-defined custom ones.
 *     "This text is white. And this is <color=0x55b6f2ff>blue text</color>!"
 *     "<fade>This text shows a fade example~</fade>"
 * @remarks  A text code is an XML-style markup for strings. See the above code example for what this looks like. See `CF_TextEffect` and
 *           `cf_text_effect_register` on registering a custom-made text effect. See `cf_text_effect_register` for a big list of built-in text effects
 *           that work out-of-the-box. Members of this struct that can be mutated freely within a custom text effect are noted with "User-modifiable"
 *           in their description.
 * @related  CF_TextEffect CF_TextEffectFn cf_text_effect_register
 */
typedef struct CF_TextEffect
{
	/* @member Name of this effect, as registered by `cf_text_effect_register`. */
	const char* effect_name;

	/* @member UTF8 codepoint of the current character. */
	int character;

	/* @member The index into the string in `cf_draw_text` currently affected. */
	int index_into_string;

	/* @member Starts at 0 and increments for each character affected. */
	int index_into_effect;

	/* @member The number of glyphs spanning the entire effect. */
	int glyph_count;

	/* @member How long this effect has persisted for. */
	float elapsed;

	/* @member Center of this glyp's space -- not the same as the center of the glyph quad. */
	CF_V2 center;

	/* @member User-modifiable. This glyph's renderable quad. q0 is the min vertex, while q1 is the max vertex. */
	CF_V2 q0, q1;

	/* @member Width and height of the glyph. */
	int w, h;

	/* @member User-modifiable. The color to render this glyph with. */
	CF_Color color;

	/* @member User-modifiable. The opacity to render this glyph with. */
	float opacity;

	/* @member User-modifiable. How far the text will advance along the x-axis (only applicable for non-vertical layout mode). */
	float xadvance;

	/* @member User-modifiable. Whether or not this glyph is visibly rendered (e.g. not visible for spaces ' '). */
	bool visible;

	/* @member The last size passed to `cf_push_font_size`. */
	float font_size;
} CF_TextEffect;
// @end

/**
 * @function CF_TextEffectFn
 * @category text
 * @brief    Implements a custom text effect, called once per glyph.
 * @param    fx        The text effect state.
 * @return   Return true to go to the next glyph. Return false to stop processing the string.
 * @example  > Internally the text shake effect is implemented something like this.
 *     // Given a string like this:
 *     "Some <shake freq=50 x=2.5 y=1>shaking text</shake> drawing!"
 *     
 *     static bool s_text_fx_shake(TextEffect* effect)
 *     {
 *         double freq = effect->get_number("freq", 35);
 *         int seed = (int)(effect->elapsed * freq);
 *         float x = (float)effect->get_number("x", 2);
 *         float y = (float)effect->get_number("y", 2);
 *         CF_Rnd rnd = cf_rnd_seed(seed);
 *         v2 offset = V2(rnd_next_range(rnd, -x, y), rnd_next_range(rnd, -x, y));
 *         effect->q0 += offset;
 *         effect->q1 += offset;
 *         return true;
 *     }
 * @remarks  The text between your custom text-code will get passed to `fn` for you, and called one time per glyph in
 *           the text just before it gets rendered. You have the chance to modify things such as the text color, size, scale,
 *           position, visibility, etc. You should use `cf_text_effect_get_number`, `cf_text_effect_get_color`, or
 *           `cf_text_effect_get_string` to fetch values from your codes. As a convenience, you can see if the current
 *           character is the first or last to render using `cf_text_effect_on_start` or `cf_text_effect_on_finish` respectively.
 * @related  CF_TextEffect CF_TextEffectFn cf_text_effect_register cf_text_effect_on_start cf_text_effect_on_finish cf_text_effect_get_number cf_text_effect_get_color cf_text_effect_get_string
 */
typedef bool (CF_TextEffectFn)(CF_TextEffect* fx);

/**
 * @function cf_text_effect_register
 * @category text
 * @brief    Registers a custom text effect.
 * @param    name      A unique name for your text effect.
 * @param    fn        The `CF_TextEffectFn` function you must implement to perform the custom effect.
 * @example  > Internally the text shake effect is implemented something like this.
 *     // Given a string like this:
 *     "Some <shake freq=50 x=2.5 y=1>shaking text</shake> drawing!"
 *     
 *     static bool s_text_fx_shake(TextEffect* effect)
 *     {
 *         double freq = effect->get_number("freq", 35);
 *         int seed = (int)(effect->elapsed * freq);
 *         float x = (float)effect->get_number("x", 2);
 *         float y = (float)effect->get_number("y", 2);
 *         CF_Rnd rnd = cf_rnd_seed(seed);
 *         v2 offset = V2(rnd_next_range(rnd, -x, y), rnd_next_range(rnd, -x, y));
 *         effect->q0 += offset;
 *         effect->q1 += offset;
 *         return true;
 *     }
 *     
 *     // Register it like so:
 *     cf_text_effect_register("shake", s_text_fx_shake);
 * @remarks  The `name` of the text effect will be used within the string text codes. For example, for the "shake" effect in the above
 *           example, the text code <shake> will be used.
 *           ```
 *           + color
 *                example : "Here's some <color=#2c5ee8>blue text</color>."
 *                        : default (white) - The color to render text with.
 *           + shake
 *                example : "<shake freq=30 x=3 y=3>This text is all shaky.</shake>"
 *                example : "<shake y=20>Shake this text with default values, but override for a big height.</shake>"
 *                freq    : default (35)    - Number of times per second to shake.
 *                x       : default (2)     - Max +/- distance to shake on x-axis.
 *                y       : default (2)     - Max +/- distance to shake on y-axis.
 *           + fade
 *                example : "<fade speed=10 span=3>Fading some text like a ghost~</fade>"
 *                example : "<fade>Fading some text like a ghost~</fade>"
 *                speed   : default (2)     - Number of times per second to find in and then out.
 *                span    : default (5)     - Number of characters long for the fade to loop.
 *           + wave
 *                example : "<wave>Wobbly wave text.</wave>"
 *                speed   : default (5)     - Number of times per second to bob up and down.
 *                span    : default (10)    - Number of characters long for the wave to loop.
 *                height. : default (5)     - How many characters high the wave will go.
 *           + strike
 *                example : "<strike>Strikethrough</strike>"
 *                example : "<strike=10>Thick Strikethrough</strike>"
 *                        : default (font_height / 20) - The thickness of the strike line.
 *           ```
 *           When registering a custom text effect, any parameters in the string will be stored for you
 *           automatically. You only need to fetch them with the appropriate cf_text_effect_get*** function.
 * @related  CF_TextEffect CF_TextEffectFn cf_text_effect_register cf_text_effect_on_start cf_text_effect_on_finish cf_text_effect_get_number cf_text_effect_get_color cf_text_effect_get_string
 */
CF_API void CF_CALL cf_text_effect_register(const char* name, CF_TextEffectFn* fn);

/**
 * @function cf_text_effect_on_start
 * @category text
 * @brief    Helper function to see if the current glyph is the beginning of the text, from within a custom text effect.
 * @param    fx        The text effect state.
 * @return   Return true to continue to the next glyph, false otherwise.
 * @related  CF_TextEffect CF_TextEffectFn cf_text_effect_register cf_text_effect_on_start cf_text_effect_on_finish cf_text_effect_get_number cf_text_effect_get_color cf_text_effect_get_string
 */
CF_API bool CF_CALL cf_text_effect_on_start(CF_TextEffect* fx);

/**
 * @function cf_text_effect_on_finish
 * @category text
 * @brief    Helper function to see if the current glyph is the end of the text, from within a custom text effect.
 * @param    fx        The text effect state.
 * @return   Return true to continue to the next glyph, false otherwise.
 * @related  CF_TextEffect CF_TextEffectFn cf_text_effect_register cf_text_effect_on_start cf_text_effect_on_finish cf_text_effect_get_number cf_text_effect_get_color cf_text_effect_get_string
 */
CF_API bool CF_CALL cf_text_effect_on_finish(CF_TextEffect* fx);

/**
 * @function cf_text_effect_get_number
 * @category text
 * @brief    Returns the text parameter as a number.
 * @param    fx           The text effect state.
 * @param    key          The name of the text code parameter
 * @param    default_val  A default value for the text code parameter if doesn't exist in the text.
 * @return   Returns the value of the text code parameter.
 * @related  CF_TextEffect CF_TextEffectFn cf_text_effect_register cf_text_effect_on_start cf_text_effect_on_finish cf_text_effect_get_number cf_text_effect_get_color cf_text_effect_get_string
 */
CF_API double CF_CALL cf_text_effect_get_number(CF_TextEffect* fx, const char* key, double default_val);

/**
 * @function cf_text_effect_get_color
 * @category text
 * @brief    Returns the text parameter as a color.
 * @param    fx           The text effect state.
 * @param    key          The name of the text code parameter
 * @param    default_val  A default value for the text code parameter if doesn't exist in the text.
 * @return   Returns the value of the text code parameter.
 * @related  CF_TextEffect CF_TextEffectFn cf_text_effect_register cf_text_effect_on_start cf_text_effect_on_finish cf_text_effect_get_number cf_text_effect_get_color cf_text_effect_get_string
 */
CF_API CF_Color CF_CALL cf_text_effect_get_color(CF_TextEffect* fx, const char* key, CF_Color default_val);

/**
 * @function cf_text_effect_get_string
 * @category text
 * @brief    Returns the text parameter as a string.
 * @param    fx           The text effect state.
 * @param    key          The name of the text code parameter
 * @param    default_val  A default value for the text code parameter if doesn't exist in the text.
 * @return   Returns the value of the text code parameter.
 * @related  CF_TextEffect CF_TextEffectFn cf_text_effect_register cf_text_effect_on_start cf_text_effect_on_finish cf_text_effect_get_number cf_text_effect_get_color cf_text_effect_get_string
 */
CF_API const char* CF_CALL cf_text_effect_get_string(CF_TextEffect* fx, const char* key, const char* default_val);

/**
 * @function cf_render_settings_filter
 * @category draw
 * @brief    Sets a rendering `CF_Filter`, used for sampling from textures.
 * @param    filter       The filter.
 * @related  cf_render_settings_filter cf_render_settings_push_viewport cf_render_settings_push_scissor cf_render_settings_push_render_state cf_render_to cf_app_draw_onto_screen
 */
CF_API void CF_CALL cf_render_settings_filter(CF_Filter filter);

/**
 * @function cf_render_settings_push_viewport
 * @category draw
 * @brief    Pushes a `CF_Rect` for the viewport to render within.
 * @param    viewport     The viewport.
 * @related  cf_render_settings_filter cf_render_settings_push_viewport cf_render_settings_pop_viewport cf_render_settings_peek_viewport cf_render_settings_push_scissor cf_render_settings_push_render_state cf_render_to cf_app_draw_onto_screen
 */
CF_API void CF_CALL cf_render_settings_push_viewport(CF_Rect viewport);

/**
 * @function cf_render_settings_pop_viewport
 * @category draw
 * @brief    Pops and returns the last `CF_Rect` for the viewport.
 * @related  cf_render_settings_filter cf_render_settings_push_viewport cf_render_settings_pop_viewport cf_render_settings_peek_viewport cf_render_settings_push_scissor cf_render_settings_push_render_state cf_render_to cf_app_draw_onto_screen
 */
CF_API CF_Rect CF_CALL cf_render_settings_pop_viewport();

/**
 * @function cf_render_settings_peek_viewport
 * @category draw
 * @brief    Returns the last `CF_Rect` for the viewport.
 * @related  cf_render_settings_filter cf_render_settings_push_viewport cf_render_settings_pop_viewport cf_render_settings_peek_viewport cf_render_settings_push_scissor cf_render_settings_push_render_state cf_render_to cf_app_draw_onto_screen
 */
CF_API CF_Rect CF_CALL cf_render_settings_peek_viewport();

/**
 * @function cf_render_settings_push_scissor
 * @category draw
 * @brief    Pushes a `CF_Rect` for the scissor to render within.
 * @param    scissor      The scissor box.
 * @related  cf_render_settings_filter cf_render_settings_push_viewport cf_render_settings_push_scissor cf_render_settings_pop_scissor cf_render_settings_peek_scissor cf_render_settings_push_render_state cf_render_to cf_app_draw_onto_screen
 */
CF_API void CF_CALL cf_render_settings_push_scissor(CF_Rect scissor);

/**
 * @function cf_render_settings_pop_scissor
 * @category draw
 * @brief    Pops and returns the last `CF_Rect` for the scissor box.
 * @related  cf_render_settings_filter cf_render_settings_push_viewport cf_render_settings_push_scissor cf_render_settings_pop_scissor cf_render_settings_peek_scissor cf_render_settings_push_render_state cf_render_to cf_app_draw_onto_screen
 */
CF_API CF_Rect CF_CALL cf_render_settings_pop_scissor();

/**
 * @function cf_render_settings_peek_scissor
 * @category draw
 * @brief    Returns the last `CF_Rect` for the scissor box.
 * @related  cf_render_settings_filter cf_render_settings_push_viewport cf_render_settings_push_scissor cf_render_settings_pop_scissor cf_render_settings_peek_scissor cf_render_settings_push_render_state cf_render_to cf_app_draw_onto_screen
 */
CF_API CF_Rect CF_CALL cf_render_settings_peek_scissor();

/**
 * @function cf_render_settings_push_render_state
 * @category draw
 * @brief    Pushes a `CF_RenderState` for controlling various rendering settings.
 * @param    render_state  Various types of rendering states.
 * @related  CF_RenderState cf_render_settings_filter cf_render_settings_push_viewport cf_render_settings_push_scissor cf_render_settings_push_render_state cf_render_settings_pop_render_state cf_render_settings_peek_render_state cf_render_to cf_app_draw_onto_screen
 */
CF_API void CF_CALL cf_render_settings_push_render_state(CF_RenderState render_state);

/**
 * @function cf_render_settings_pop_render_state
 * @category draw
 * @brief    Pops and returns the last `CF_RenderState`.* @related  CF_RenderState cf_render_settings_filter cf_render_settings_push_viewport cf_render_settings_push_scissor cf_render_settings_push_render_state cf_render_settings_pop_render_state cf_render_settings_peek_render_state cf_render_to cf_app_draw_onto_screen
 */
CF_API CF_RenderState CF_CALL cf_render_settings_pop_render_state();

/**
 * @function cf_render_settings_peek_render_state
 * @category draw
 * @brief    Returns the last `CF_RenderState`.* @related  CF_RenderState cf_render_settings_filter cf_render_settings_push_viewport cf_render_settings_push_scissor cf_render_settings_push_render_state cf_render_settings_pop_render_state cf_render_settings_peek_render_state cf_render_to cf_app_draw_onto_screen
 */
CF_API CF_RenderState CF_CALL cf_render_settings_peek_render_state();

/**
 * @function cf_render_settings_set_atlas_dimensions
 * @category draw
 * @brief    Sets the internal atlas size for batching sprites. The default is 2048x2048.
 * @remarks  This function will completely invalidate the current cache, causing a noticeable perf cost -- do not call this
 *           function frequently. It's intended as a one-time setup. Be careful not to pass in values too large as certain
 *           graphics backends have different texture size limits. A very safe max size is 2048x2048 (the default), but you
 *           can likely get away with 4096 on most devices. Larger internal atlases can be useful to decrease the number of
 *           draw calls used, and also enables support for high-res image rendering.
 *           
 *           Please not you should put in power of 2 atlases sizes to make the hardware happy. Here are the recommended range
 *           of sizes available:
 *           
 *           - 256
 *           - 512
 *           - 1024
 *           - 2048
 *           - 4096
 */
CF_API void CF_CALL cf_render_settings_set_atlas_dimensions(int width_in_pixels, int height_in_pixels);

/**
 * @function cf_render_settings_push_shader
 * @category draw
 * @brief    TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_render_settings_push_shader(CF_Shader shader);

/**
 * @function cf_render_settings_pop_shader
 * @category draw
 * @brief    TODO
 * @related  TODO
 */
CF_API CF_Shader CF_CALL cf_render_settings_pop_shader();

/**
 * @function cf_render_settings_peek_shader
 * @category draw
 * @brief    TODO
 * @related  TODO
 */
CF_API CF_Shader CF_CALL cf_render_settings_peek_shader();

/**
 * @function cf_render_settings_push_texture
 * @category draw
 * @brief    TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_render_settings_push_texture(const char* name, CF_Texture texture);

/**
 * @function cf_render_settings_push_uniform
 * @category draw
 * @brief    TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_render_settings_push_uniform(const char* name, void* data, CF_UniformType type, int array_length);

/**
 * @function cf_render_settings_push_uniform_int
 * @category draw
 * @brief    TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_render_settings_push_uniform_int(const char* name, int val);

/**
 * @function cf_render_settings_push_uniform_float
 * @category draw
 * @brief    TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_render_settings_push_uniform_float(const char* name, float val);

/**
 * @function cf_render_settings_push_uniform_v2
 * @category draw
 * @brief    TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_render_settings_push_uniform_v2(const char* name, CF_V2 val);

/**
 * @function cf_render_settings_push_uniform_color
 * @category draw
 * @brief    TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_render_settings_push_uniform_color(const char* name, CF_Color val);

/**
 * @function cf_camera_dimensions
 * @category camera
 * @brief    Sets the width and height of the camera's view.
 * @param    w          The width of the camera's view.
 * @param    h          The height of the camera's view.
 * @related  cf_camera_dimensions cf_camera_look_at cf_camera_rotate cf_camera_push cf_camera_pop cf_app_draw_onto_screen cf_render_to cf_camera_peek_position cf_camera_peek_dimensions cf_camera_peek_rotation cf_camera_peek
 */
CF_API void CF_CALL cf_camera_dimensions(float w, float h);

/**
 * @function cf_camera_look_at
 * @category camera
 * @brief    Sets where in the world the camera sees.
 * @param    x          The x position of the camera's view.
 * @param    y          The y position of the camera's view.
 * @related  cf_camera_dimensions cf_camera_look_at cf_camera_rotate cf_camera_push cf_camera_pop cf_app_draw_onto_screen cf_render_to cf_camera_peek_position cf_camera_peek_dimensions cf_camera_peek_rotation cf_camera_peek
 */
CF_API void CF_CALL cf_camera_look_at(float x, float y);

/**
 * @function cf_camera_rotate
 * @category camera
 * @brief    Rotates the camera.
 * @param    radians    The angle of camera rotation.
 * @related  cf_camera_dimensions cf_camera_look_at cf_camera_rotate cf_camera_push cf_camera_pop cf_app_draw_onto_screen cf_render_to cf_camera_peek_position cf_camera_peek_dimensions cf_camera_peek_rotation cf_camera_peek
 */
CF_API void CF_CALL cf_camera_rotate(float radians);

/**
 * @function cf_camera_push
 * @category camera
 * @brief    Pushes a copy of the camera state.
 * @related  cf_camera_dimensions cf_camera_look_at cf_camera_rotate cf_camera_push cf_camera_pop cf_app_draw_onto_screen cf_render_to cf_camera_peek_position cf_camera_peek_dimensions cf_camera_peek_rotation cf_camera_peek
 */
CF_API void CF_CALL cf_camera_push();

/**
 * @function cf_camera_pop
 * @category camera
 * @brief    Pops the current camera state, and uses the previously pushed camera state.
 * @related  cf_camera_dimensions cf_camera_look_at cf_camera_rotate cf_camera_push cf_camera_pop cf_app_draw_onto_screen cf_render_to cf_camera_peek_position cf_camera_peek_dimensions cf_camera_peek_rotation cf_camera_peek
 */
CF_API void CF_CALL cf_camera_pop();

/**
 * @function cf_camera_peek_position
 * @category camera
 * @brief    Returns the current camera position.
 * @remarks  See `cf_camera_look_at`.
 * @related  cf_camera_dimensions cf_camera_look_at cf_camera_rotate cf_camera_push cf_camera_pop cf_app_draw_onto_screen cf_render_to cf_camera_peek_position cf_camera_peek_dimensions cf_camera_peek_rotation cf_camera_peek
 */
CF_API CF_V2 CF_CALL cf_camera_peek_position();

/**
 * @function cf_camera_peek_dimensions
 * @category camera
 * @brief    Returns the current camera dimensions.
 * @remarks  See `cf_camera_dimensions`.
 * @related  cf_camera_dimensions cf_camera_look_at cf_camera_rotate cf_camera_push cf_camera_pop cf_app_draw_onto_screen cf_render_to cf_camera_peek_position cf_camera_peek_dimensions cf_camera_peek_rotation cf_camera_peek
 */
CF_API CF_V2 CF_CALL cf_camera_peek_dimensions();

/**
 * @function cf_camera_peek_rotation
 * @category camera
 * @brief    Returns the current camera rotation.
 * @remarks  See `cf_camera_rotate`.
 * @related  cf_camera_dimensions cf_camera_look_at cf_camera_rotate cf_camera_push cf_camera_pop cf_app_draw_onto_screen cf_render_to cf_camera_peek_position cf_camera_peek_dimensions cf_camera_peek_rotation cf_camera_peek
 */
CF_API float CF_CALL cf_camera_peek_rotation();

/**
 * @function cf_camera_peek
 * @category camera
 * @brief    Returns the current camera as a `CF_M3x2`.
 * @remarks  Multiplying this matrix against a vector will transform the vector to "cam space" or "eye space".
 * @related  cf_camera_dimensions cf_camera_look_at cf_camera_rotate cf_camera_push cf_camera_pop cf_app_draw_onto_screen cf_render_to cf_camera_peek_position cf_camera_peek_dimensions cf_camera_peek_rotation cf_camera_peek
 */
CF_API CF_M3x2 CF_CALL cf_camera_peek();

/**
 * @function cf_render_to
 * @category draw
 * @brief    Renders to a `CF_Canvas`.
 * @param    canvas     The canvas to render to.
 * @param    clear      True to clear the canvas's previous contents, false otherwise.
 * @remarks  This is advanced function. It's useful for off-screen rendering for certain rendering effects, such as multi-pass
 *           effects like reflections, or advanced lighting techniques. By default, everything will get renderered to the app's
 *           canvas, so this function is not necessary to call at all. Instead, calling `cf_app_draw_onto_screen` should be the go-to.
 * @related  cf_camera_dimensions cf_camera_look_at cf_camera_rotate cf_camera_push cf_camera_pop cf_app_draw_onto_screen cf_render_to
 */
CF_API void CF_CALL cf_render_to(CF_Canvas canvas, bool clear);

/**
 * @struct   CF_TemporaryImage
 * @category draw
 * @brief    Returns temporal information about a sprite's rendering internals.
 * @remarks  Useful to render a sprite in an external system, e.g. Dear ImGui. This struct is only valid until the next time `cf_render_to` or
 *           `cf_app_draw_onto_screen` is called.
 * @related  CF_TemporaryImage cf_fetch_image
 */
typedef struct CF_TemporaryImage
{
	/* @member A handle representing the texture for this image. */
	CF_Texture tex;

	/* @member Width in pixels of the image. */
	int w;

	/* @member Height in pixels of the image. */
	int h;

	/* @member u coordinate of the image in the texture. */
	CF_V2 u;

	/* @member v coordinate of the image in the texture. */
	CF_V2 v;
} CF_TemporaryImage;
// @end

/**
 * @function cf_fetch_image
 * @category draw
 * @brief    Returns a `CF_TemporaryImage` for a given sprite.
 * @param    sprite     The sprite.
 * @remarks  Useful to render a sprite in an external system, e.g. Dear ImGui. This struct is only valid until the next time `cf_render_to` or
 *           `cf_app_draw_onto_screen` is called.
 * @related  CF_TemporaryImage cf_fetch_image
 */
CF_API CF_TemporaryImage CF_CALL cf_fetch_image(const CF_Sprite* sprite);

//--------------------------------------------------------------------------------------------------
// "Hidden" API -- Just here for some inline C++ functions below.

enum CF_TextCodeValType
{
	CF_TEXT_CODE_VAL_TYPE_NONE,
	CF_TEXT_CODE_VAL_TYPE_COLOR,
	CF_TEXT_CODE_VAL_TYPE_NUMBER,
	CF_TEXT_CODE_VAL_TYPE_STRING,
};

struct CF_TextCodeVal
{
	enum CF_TextCodeValType type;
	union
	{
		CF_Color color;
		double number;
		const char* string;
	} u;
};

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

using TemporaryImage = CF_TemporaryImage;

CF_INLINE void draw_sprite(const Sprite* sprite) { cf_draw_sprite(sprite); }
CF_INLINE void draw_sprite(const Sprite& sprite) { cf_draw_sprite(&sprite); }
CF_INLINE void draw_quad(Aabb bb, float thickness = 1.0f, float chubbiness = 0) { cf_draw_quad(bb, thickness, chubbiness); }
CF_INLINE void draw_quad(v2 p0, v2 p1, v2 p2, v2 p3, float thickness = 1.0f, float chubbiness = 0) { cf_draw_quad2(p0, p1, p2, p3, thickness, chubbiness); }
CF_INLINE void draw_quad_fill(Aabb bb, float chubbiness = 0) { cf_draw_quad_fill(bb, chubbiness); }
CF_INLINE void draw_quad_fill(v2 p0, v2 p1, v2 p2, v2 p3, float chubbiness = 0) { cf_draw_quad_fill2(p0, p1, p2, p3, chubbiness); }
CF_INLINE void draw_box(Aabb bb, float thickness = 1.0f, float chubbiness = 0) { cf_draw_quad(bb, thickness, chubbiness); }
CF_INLINE void draw_box(v2 p0, v2 p1, v2 p2, v2 p3, float thickness = 1.0f, float chubbiness = 0) { cf_draw_quad2(p0, p1, p2, p3, thickness, chubbiness); }
CF_INLINE void draw_box(v2 p, float w, float h, float thickness = 1.0f, float chubbiness = 0) { cf_draw_quad(make_aabb(p, w, h), thickness, chubbiness); }
CF_INLINE void draw_box_fill(Aabb bb, float chubbiness = 0) { cf_draw_quad_fill(bb, chubbiness); }
CF_INLINE void draw_box_fill(v2 p0, v2 p1, v2 p2, v2 p3, float chubbiness = 0) { cf_draw_quad_fill2(p0, p1, p2, p3, chubbiness); }
CF_INLINE void draw_circle(Circle circle, float thickness = 1.0f) { cf_draw_circle(circle, thickness); }
CF_INLINE void draw_circle(v2 p, float r, float thickness = 1.0f) { cf_draw_circle2(p, r, thickness); }
CF_INLINE void draw_circle_fill(Circle circle) { cf_draw_circle_fill(circle); }
CF_INLINE void draw_circle_fill(v2 p, float r) { cf_draw_circle_fill2(p, r); }
CF_INLINE void draw_arc(v2 p, v2 center_of_arc, float range, int iters = 20, float thickness = 1.0f) { cf_draw_arc(p, center_of_arc, range, iters, thickness); }
CF_INLINE void draw_capsule(Capsule capsule, float thickness = 1.0f) { cf_draw_capsule(capsule, thickness); }
CF_INLINE void draw_capsule(v2 p0, v2 p1, float r, float thickness = 1.0f) { cf_draw_capsule2(p0, p1, r, thickness); }
CF_INLINE void draw_capsule_fill(Capsule capsule) { cf_draw_capsule_fill(capsule); }
CF_INLINE void draw_capsule_fill(v2 p0, v2 p1, float r) { cf_draw_capsule_fill2(p0, p1, r); }
CF_INLINE void draw_tri(v2 p0, v2 p1, v2 p2, float thickness = 1.0f, float chubbiness = 0) { cf_draw_tri(p0, p1, p2, thickness, chubbiness); }
CF_INLINE void draw_tri_fill(v2 p0, v2 p1, v2 p2, float chubbiness = 0) { cf_draw_tri_fill(p0, p1, p2, chubbiness); }
CF_INLINE void draw_line(v2 p0, v2 p1, float thickness = 1.0f) { cf_draw_line(p0, p1, thickness); }
CF_INLINE void draw_polyline(v2* points, int count, float thickness = 1.0f, bool loop = false) { cf_draw_polyline(points, count, thickness, loop); }
CF_INLINE void draw_bezier_line(v2 a, v2 c0, v2 b, int iters, float thickness) { cf_draw_bezier_line(a, c0, b, iters, thickness); }
CF_INLINE void draw_bezier_line(v2 a, v2 c0, v2 c1, v2 b, int iters, float thickness) { cf_draw_bezier_line2(a, c0, c1, b, iters, thickness); }
CF_INLINE void draw_arrow(v2 a, v2 b, float thickness, float arrow_width) { cf_draw_arrow(a, b, thickness, arrow_width); }

CF_INLINE void draw_push_layer(int layer) { cf_draw_push_layer(layer); }
CF_INLINE int draw_pop_layer() { return cf_draw_pop_layer(); }
CF_INLINE int draw_peek_layer() { return cf_draw_peek_layer(); }
CF_INLINE void draw_push_color(CF_Color c) { cf_draw_push_color(c); }
CF_INLINE CF_Color draw_pop_color() { return cf_draw_pop_color(); }
CF_INLINE CF_Color draw_peek_color() { return cf_draw_peek_color(); }
CF_INLINE void draw_push_tint(CF_Color c) { cf_draw_push_tint(c); }
CF_INLINE CF_Color draw_pop_tint() { return cf_draw_pop_tint(); }
CF_INLINE CF_Color draw_peek_tint() { return cf_draw_peek_tint(); }
CF_INLINE void draw_push_antialias(bool antialias) { cf_draw_push_antialias(antialias); }
CF_INLINE bool draw_pop_antialias() { return cf_draw_pop_antialias(); }
CF_INLINE bool draw_peek_antialias() { return cf_draw_peek_antialias(); }
CF_INLINE void draw_push_antialias_scale(float scale) { return cf_draw_push_antialias_scale(scale); }
CF_INLINE float draw_pop_antialias_scale() { return cf_draw_pop_antialias_scale(); }
CF_INLINE float draw_peek_antialias_scale() { return cf_draw_peek_antialias_scale(); }
CF_INLINE void draw_push_vertex_attributes(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { cf_draw_push_vertex_attributes(r, g, b, a); }
CF_INLINE void draw_push_vertex_attributes(CF_Pixel attributes) { cf_draw_push_vertex_attributes2(attributes); }
CF_INLINE CF_Pixel draw_pop_vertex_attributes() { return cf_draw_pop_vertex_attributes(); }
CF_INLINE CF_Pixel draw_peek_vertex_attributes() { return cf_draw_peek_vertex_attributes(); }

CF_INLINE CF_Result make_font(const char* path, const char* font_name) { return cf_make_font(path, font_name); }
CF_INLINE CF_Result make_font_from_memory(void* data, int size, const char* font_name) { return cf_make_font_from_memory(data, size, font_name); }
CF_INLINE void destroy_font(const char* font_name) { cf_destroy_font(font_name); }
CF_INLINE void font_add_backup_codepoints(const char* font_name, int* codepoints, int count) { cf_font_add_backup_codepoints(font_name, codepoints, count); }
CF_INLINE void push_font(const char* font_name) { cf_push_font(font_name); }
CF_INLINE const char* pop_font() { return cf_pop_font(); }
CF_INLINE const char* peek_font() { return cf_peek_font(); }
CF_INLINE void push_font_size(float size) { cf_push_font_size(size); }
CF_INLINE float pop_font_size() { return cf_pop_font_size(); }
CF_INLINE float peek_font_size() { return cf_peek_font_size(); }
CF_INLINE void push_font_blur(int blur) { cf_push_font_blur(blur); }
CF_INLINE int pop_font_blur() { return cf_pop_font_blur(); }
CF_INLINE int peek_font_blur() { return cf_peek_font_blur(); }
CF_INLINE void push_text_wrap_width(float width) { cf_push_text_wrap_width(width); }
CF_INLINE float pop_text_wrap_width() { return cf_pop_text_wrap_width(); }
CF_INLINE float peek_text_wrap_width() { return cf_peek_text_wrap_width(); }
CF_INLINE void push_text_clip_box(CF_Aabb clip_box) { cf_push_text_clip_box(clip_box); }
CF_INLINE CF_Aabb pop_text_clip_box() { return cf_pop_text_clip_box(); }
CF_INLINE CF_Aabb peek_text_clip_box() { return cf_peek_text_clip_box(); }
CF_INLINE float text_width(const char* text, int num_chars_to_render = -1) { return cf_text_width(text, num_chars_to_render); }
CF_INLINE float text_height(const char* text, int num_chars_to_render = -1) { return cf_text_height(text, num_chars_to_render); }
CF_INLINE v2 text_size(const char* text, int num_chars_to_render = -1) { return cf_text_size(text, num_chars_to_render); }
CF_INLINE void draw_text(const char* text, CF_V2 position, int num_chars_to_render = -1) { cf_draw_text(text, position, num_chars_to_render); }

struct TextEffect : public CF_TextEffect
{
	CF_INLINE bool on_start() const { return index_into_effect == 0; }
	CF_INLINE bool on_finish() const { return index_into_effect == glyph_count - 1; }

	CF_INLINE double get_number(const char* key, double default_val = 0)
	{
		const CF_TextCodeVal* v = params->try_find(sintern(key));
		if (v && v->type == CF_TEXT_CODE_VAL_TYPE_NUMBER) {
			return v->u.number;
		} else {
			return default_val;
		}
	}
	
	CF_INLINE CF_Color get_color(const char* key, CF_Color default_val = cf_color_white())
	{
		const CF_TextCodeVal* v = params->try_find(sintern(key));
		if (v && v->type == CF_TEXT_CODE_VAL_TYPE_COLOR) {
			return v->u.color;
		} else {
			return default_val;
		}
	}
	
	CF_INLINE const char* get_string(const char* key, const char* default_val = NULL)
	{
		const CF_TextCodeVal* v = params->try_find(sintern(key));
		if (v && v->type == CF_TEXT_CODE_VAL_TYPE_STRING) {
			return v->u.string;
		} else {
			return default_val;
		}
	}

	// "private" state -- don't touch.
	const Cute::Map<const char*, CF_TextCodeVal>* params;
	CF_TextEffectFn* fn;
};

typedef bool (TextEffectFn)(TextEffect* fx);

CF_INLINE void text_effect_register(const char* name, TextEffectFn* fn) { cf_text_effect_register(name, (CF_TextEffectFn*)fn); }

CF_INLINE void render_settings_filter(CF_Filter filter) { cf_render_settings_filter(filter); }
CF_INLINE void render_settings_push_viewport(CF_Rect viewport) { cf_render_settings_push_viewport(viewport); }
CF_INLINE CF_Rect render_settings_pop_viewport() { return cf_render_settings_pop_viewport(); }
CF_INLINE CF_Rect render_settings_peek_viewport() { return cf_render_settings_peek_viewport(); }
CF_INLINE void render_settings_push_scissor(CF_Rect scissor) { cf_render_settings_push_scissor(scissor); }
CF_INLINE CF_Rect render_settings_pop_scissor() { return cf_render_settings_pop_scissor(); }
CF_INLINE CF_Rect render_settings_peek_scissor() { return cf_render_settings_peek_scissor(); }
CF_INLINE void render_settings_push_render_state(CF_RenderState render_state) { render_settings_push_render_state(render_state); }
CF_INLINE CF_RenderState render_settings_pop_render_state() { return render_settings_pop_render_state(); }
CF_INLINE CF_RenderState render_settings_peek_render_state() { return render_settings_peek_render_state(); }
CF_INLINE void render_settings_push_shader(CF_Shader shader) { cf_render_settings_push_shader(shader); }
CF_INLINE CF_Shader render_settings_pop_shader() { return cf_render_settings_pop_shader(); }
CF_INLINE CF_Shader render_settings_peek_shader() { return cf_render_settings_peek_shader(); }
CF_INLINE void render_settings_push_texture(const char* name, CF_Texture texture) { cf_render_settings_push_texture(name, texture); }
CF_INLINE void render_settings_push_uniform(const char* name, void* data, CF_UniformType type, int array_length) { cf_render_settings_push_uniform(name, data, type, array_length); }
CF_INLINE void render_settings_push_uniform(const char* name, int val) { cf_render_settings_push_uniform_int(name, val); }
CF_INLINE void render_settings_push_uniform(const char* name, float val) { cf_render_settings_push_uniform_float(name, val); }
CF_INLINE void render_settings_push_uniform(const char* name, v2 val) { cf_render_settings_push_uniform_v2(name, val); }
CF_INLINE void render_settings_push_uniform(const char* name, CF_Color val) { cf_render_settings_push_uniform_color(name, val); }

CF_INLINE void camera_dimensions(float w, float h) { cf_camera_dimensions(w, h); }
CF_INLINE void camera_look_at(float x, float y) { cf_camera_look_at(x, y); }
CF_INLINE void camera_look_at(v2 pos) { cf_camera_look_at(pos.x, pos.y); }
CF_INLINE void camera_rotate(float radians) { cf_camera_rotate(radians); }
CF_INLINE CF_V2 camera_peek_position() { return cf_camera_peek_position(); }
CF_INLINE CF_V2 camera_peek_dimensions() { return cf_camera_peek_dimensions(); }
CF_INLINE float camera_peek_rotation() { return cf_camera_peek_rotation(); }
CF_INLINE CF_M3x2 camera_peek() { return cf_camera_peek(); }
CF_INLINE void camera_push() { cf_camera_push(); }
CF_INLINE void camera_pop() { cf_camera_pop(); }

CF_INLINE void render_to(CF_Canvas canvas, bool clear = false) { cf_render_to(canvas, clear); }

CF_INLINE CF_TemporaryImage fetch_image(const CF_Sprite* sprite) { return cf_fetch_image(sprite); }
CF_INLINE CF_TemporaryImage fetch_image(const CF_Sprite& sprite) { return cf_fetch_image(&sprite); }

}

#endif // CF_CPP

#endif // CF_DRAW_H
