/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_VIDEO_H
#define CF_VIDEO_H

#include "cute_defines.h"
#include "cute_result.h"
#include "cute_image.h"
#include "cute_graphics.h"
#include "cute_sprite.h"

//--------------------------------------------------------------------------------------------------
// C API
//
// Playing and recording H.264 video. The codec itself lives in libraries/cute/cute_h264.h and does
// no file IO of its own; these functions wire it to CF's virtual file system, its allocator, its
// images and its textures.
//
// Both halves are CF's own code -- no system codec, no patent-encumbered third party library, the
// same dual zlib/Unlicense terms as the rest of the framework. That is the reason this exists: a
// game can ship a cutscene, or record one, without dragging in something it cannot redistribute.
//
// Playing a file is three lines:
//
//     CF_Video* video = cf_make_video("/intro.mp4");
//     ...
//     cf_video_update(video, CF_DELTA_TIME);
//     cf_draw_box... / cf_material_set_texture_fs(material, "tex", cf_video_texture(video));
//
// Recording is four:
//
//     CF_VideoEncoder* encoder = cf_make_video_encoder(w, h, 30);
//     for (each frame) cf_video_encoder_add_frame(encoder, image);
//     cf_video_encoder_save(encoder, "/replay.mp4");
//     cf_destroy_video_encoder(encoder);
//
// Decoding is a few milliseconds of CPU per frame at 720p and there is no hardware path, so this
// suits cutscenes, replays and title screens rather than a dozen videos at once.

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @struct   CF_Video
 * @category video
 * @brief    An opened video, decoded one picture at a time.
 * @remarks  Made by `cf_make_video`. Holds the whole file in memory along with the decoder's
 *           reference pictures, so a long video costs roughly its file size plus a few frames.
 * @related  CF_Video cf_make_video cf_destroy_video cf_video_update cf_video_texture
 */
typedef struct CF_Video CF_Video;
// @end

/**
 * @struct   CF_VideoEncoder
 * @category video
 * @brief    Turns a sequence of images into an H.264 video file.
 * @remarks  Made by `cf_make_video_encoder`. Frames are compressed as they arrive and accumulate
 *           in memory until `cf_video_encoder_save` writes them out.
 * @related  CF_VideoEncoder cf_make_video_encoder cf_video_encoder_add_frame cf_video_encoder_save
 */
typedef struct CF_VideoEncoder CF_VideoEncoder;
// @end

// -------------------------------------------------------------------------------------------------
// Playback.

/**
 * @function cf_make_video
 * @category video
 * @brief    Opens a video file for playback.
 * @param    virtual_path  A virtual path to an .mp4 or raw .h264 file. See [Virtual File System](https://randygaul.github.io/cute_framework/topics/virtual_file_system).
 * @return   Returns the video, or `NULL` on failure -- call `cf_video_error` for the reason.
 * @remarks  Both container forms are accepted and told apart by their contents, so a file saved by
 *           `cf_video_encoder_save` reopens whichever extension it was given. Nothing is decoded
 *           yet; call `cf_video_update` or `cf_video_next_frame` to advance. Free it with
 *           `cf_destroy_video`.
 * @related  CF_Video cf_make_video_from_memory cf_destroy_video cf_video_update cf_video_error
 */
CF_API CF_Video* CF_CALL cf_make_video(const char* virtual_path);

/**
 * @function cf_make_video_from_memory
 * @category video
 * @brief    Opens a video already in memory.
 * @param    data  The file's bytes, .mp4 or raw .h264.
 * @param    size  How many bytes `data` holds.
 * @return   Returns the video, or `NULL` on failure -- call `cf_video_error` for the reason.
 * @remarks  The bytes are copied, so the caller's buffer may be freed immediately.
 * @related  CF_Video cf_make_video cf_destroy_video cf_video_error
 */
CF_API CF_Video* CF_CALL cf_make_video_from_memory(const void* data, int size);

/**
 * @function cf_destroy_video
 * @category video
 * @brief    Frees a video and its texture.
 * @param    video  The video from `cf_make_video`. `NULL` is allowed.
 * @related  CF_Video cf_make_video cf_video_texture
 */
CF_API void CF_CALL cf_destroy_video(CF_Video* video);

/**
 * @function cf_video_error
 * @category video
 * @brief    Returns a human-readable reason for the last video failure.
 * @remarks  Static storage, valid until the next call into this API. Meaningful after
 *           `cf_make_video` returns `NULL`, or after `cf_video_update` stops early.
 * @related  cf_make_video cf_video_update
 */
CF_API const char* CF_CALL cf_video_error(void);

/**
 * @function cf_video_width
 * @category video
 * @brief    The width of the video in pixels.
 * @param    video  The video.
 * @remarks  Known as soon as the video is opened, before any frame is decoded.
 * @related  CF_Video cf_video_height cf_video_fps
 */
CF_API int CF_CALL cf_video_width(CF_Video* video);

/**
 * @function cf_video_height
 * @category video
 * @brief    The height of the video in pixels.
 * @param    video  The video.
 * @related  CF_Video cf_video_width cf_video_fps
 */
CF_API int CF_CALL cf_video_height(CF_Video* video);

/**
 * @function cf_video_fps
 * @category video
 * @brief    How many frames per second the file says it runs at.
 * @param    video  The video.
 * @return   Returns the rate, or 0 if the file does not state one.
 * @remarks  The rate is optional in H.264 and some encoders leave it out. `cf_video_update` falls
 *           back to 30 when it is missing; use `cf_video_next_frame` and your own clock if you
 *           need something else.
 * @related  CF_Video cf_video_update cf_video_next_frame
 */
CF_API int CF_CALL cf_video_fps(CF_Video* video);

/**
 * @function cf_video_update
 * @category video
 * @brief    Advances playback by an elapsed time, decoding a frame when one is due.
 * @param    video  The video.
 * @param    dt     Seconds elapsed, usually `CF_DELTA_TIME`.
 * @return   Returns true if a new frame became current on this call.
 * @remarks  Call this once a frame and draw `cf_video_texture` regardless of what it returns --
 *           a video slower than the display simply keeps showing the picture it is on. A single
 *           call never decodes more than a few frames, so a long stall drops behind rather than
 *           freezing the game catching up. At the end of the file this stops, unless
 *           `cf_video_set_looped` says otherwise.
 * @related  CF_Video cf_video_next_frame cf_video_texture cf_video_set_looped cf_video_is_finished
 */
CF_API bool CF_CALL cf_video_update(CF_Video* video, float dt);

/**
 * @function cf_video_next_frame
 * @category video
 * @brief    Decodes exactly one frame, ignoring any clock.
 * @param    video  The video.
 * @return   Returns true if a frame was decoded, false at the end of the file or on a broken one.
 * @remarks  This is the one to use to walk a video frame by frame -- to pull thumbnails out of it,
 *           or to drive playback off something other than wall-clock time. Looping does not apply
 *           here; the end of the file is the end.
 * @related  CF_Video cf_video_update cf_video_frame cf_video_texture
 */
CF_API bool CF_CALL cf_video_next_frame(CF_Video* video);

/**
 * @function cf_video_frame
 * @category video
 * @brief    The picture currently being shown, as pixels.
 * @param    video  The video.
 * @return   Returns a `CF_Image` whose pixels belong to the video.
 * @remarks  Do NOT call `cf_image_free` on it, and do not keep the pointer: it is overwritten by
 *           the next decode. Before the first frame is decoded the image is zero sized. To get it
 *           onto the screen use `cf_video_texture` instead, which does the upload for you.
 * @related  CF_Video cf_video_texture cf_video_next_frame
 */
CF_API CF_Image CF_CALL cf_video_frame(CF_Video* video);

/**
 * @function cf_video_sprite
 * @category video
 * @brief    The picture currently being shown, as a sprite ready to draw.
 * @param    video  The video.
 * @return   Returns a sprite owned by the video, sized to the video.
 * @remarks  This is the short way to get a video on screen:
 *
 *           ```c
 *           cf_video_update(video, CF_DELTA_TIME);
 *           CF_Sprite sprite = cf_video_sprite(video);
 *           cf_draw_sprite(&sprite);
 *           ```
 *
 *           Everything the draw API can do to a sprite works here -- transforms, tint, layers. The
 *           sprite is re-uploaded only when the frame actually changes. Do not call
 *           `cf_easy_sprite_unload` on it; `cf_destroy_video` handles it. Note what
 *           `cf_easy_sprite_update_pixels` says about cost: the first few updates after the image
 *           packs rebuild an atlas page, after which it migrates to a texture of its own and
 *           settles. If that matters, or if the frames are headed for a shader rather than the
 *           draw API, use `cf_video_texture` instead.
 * @related  CF_Video cf_video_texture cf_video_update cf_draw_sprite
 */
CF_API CF_Sprite CF_CALL cf_video_sprite(CF_Video* video);

/**
 * @function cf_video_texture
 * @category video
 * @brief    The picture currently being shown, as a texture ready to draw.
 * @param    video  The video.
 * @return   Returns a texture owned by the video.
 * @remarks  Created on the first call and re-uploaded only when the frame actually changes, so
 *           calling this every frame costs nothing extra. Do not call `cf_destroy_texture` on it;
 *           `cf_destroy_video` handles it. Requires the app to be running, like any other texture.
 *
 *           This is the one to feed a custom shader or a 3D draw. To simply put the video on
 *           screen with the 2D draw API, `cf_video_sprite` is fewer steps.
 * @related  CF_Video cf_video_sprite cf_video_update cf_video_frame CF_Texture
 */
CF_API CF_Texture CF_CALL cf_video_texture(CF_Video* video);

/**
 * @function cf_video_set_looped
 * @category video
 * @brief    Sets whether the video starts over when it reaches the end.
 * @param    video   The video.
 * @param    looped  True to loop, false to stop on the last frame. Off by default.
 * @related  CF_Video cf_video_update cf_video_is_finished cf_video_restart
 */
CF_API void CF_CALL cf_video_set_looped(CF_Video* video, bool looped);

/**
 * @function cf_video_is_finished
 * @category video
 * @brief    Returns true once the last frame has been shown.
 * @param    video  The video.
 * @remarks  Never true for a looping video.
 * @related  CF_Video cf_video_set_looped cf_video_restart
 */
CF_API bool CF_CALL cf_video_is_finished(CF_Video* video);

/**
 * @function cf_video_restart
 * @category video
 * @brief    Starts the video over from the beginning.
 * @param    video  The video.
 * @remarks  The file is already in memory, so this is cheap -- it throws away the decoder's state
 *           and builds it again.
 * @related  CF_Video cf_video_update cf_video_set_looped cf_video_is_finished
 */
CF_API void CF_CALL cf_video_restart(CF_Video* video);

// -------------------------------------------------------------------------------------------------
// Recording.

/**
 * @function cf_make_video_encoder
 * @category video
 * @brief    Creates an encoder that turns images into an H.264 video.
 * @param    w    Width in pixels. Any size works; odd sizes are padded internally and cropped back.
 * @param    h    Height in pixels.
 * @param    fps  Frames per second the result should play at.
 * @return   Returns the encoder, or `NULL` on failure -- call `cf_video_error` for the reason.
 * @remarks  Every frame handed over must be exactly `w` by `h`. Free it with
 *           `cf_destroy_video_encoder`.
 * @related  CF_VideoEncoder cf_video_encoder_add_frame cf_video_encoder_save cf_video_encoder_quality
 */
CF_API CF_VideoEncoder* CF_CALL cf_make_video_encoder(int w, int h, int fps);

/**
 * @function cf_destroy_video_encoder
 * @category video
 * @brief    Frees an encoder and everything it has compressed so far.
 * @param    encoder  The encoder from `cf_make_video_encoder`. `NULL` is allowed.
 * @related  CF_VideoEncoder cf_make_video_encoder cf_video_encoder_save
 */
CF_API void CF_CALL cf_destroy_video_encoder(CF_VideoEncoder* encoder);

/**
 * @function cf_video_encoder_quality
 * @category video
 * @brief    Trades file size against picture quality.
 * @param    encoder  The encoder.
 * @param    quality  0 to 100. 50 is the default and a sane middle; 100 is exactly lossless.
 * @remarks  Call this before the first frame -- changing it partway through applies from that
 *           point on, which is legal but rarely what anyone means. Lossless is not "very good", it
 *           is bit-for-bit perfect and enormous, tens of times the size of quality 99. It exists
 *           for intermediate files that will be re-encoded, not for shipping.
 * @related  CF_VideoEncoder cf_make_video_encoder cf_video_encoder_add_frame
 */
CF_API void CF_CALL cf_video_encoder_quality(CF_VideoEncoder* encoder, int quality);

/**
 * @function cf_video_encoder_add_frame
 * @category video
 * @brief    Compresses one frame.
 * @param    encoder  The encoder.
 * @param    frame    The picture, sized exactly as the encoder was created with.
 * @return   Returns any error that stopped the frame being added.
 * @remarks  Frames go in the order they should play. The alpha channel is discarded -- H.264 has
 *           nowhere to put it. This is the expensive call in this API: it runs a motion search over
 *           the whole picture, so it is not something to do inside a frame budget.
 * @related  CF_VideoEncoder cf_video_encoder_save cf_video_encoder_quality
 */
CF_API CF_Result CF_CALL cf_video_encoder_add_frame(CF_VideoEncoder* encoder, CF_Image frame);

/**
 * @function cf_video_encoder_save
 * @category video
 * @brief    Writes everything compressed so far as an .mp4 file.
 * @param    encoder       The encoder.
 * @param    virtual_path  Where to write it. See [Virtual File System](https://randygaul.github.io/cute_framework/topics/virtual_file_system).
 * @return   Returns any error that stopped the file being written.
 * @remarks  The encoder is still usable afterwards; adding more frames and saving again writes a
 *           longer file. A path ending in .h264 or .264 writes the raw stream instead of an MP4,
 *           which is what a command line tool expects to be piped.
 * @related  CF_VideoEncoder cf_video_encoder_add_frame cf_video_encoder_data cf_make_video
 */
CF_API CF_Result CF_CALL cf_video_encoder_save(CF_VideoEncoder* encoder, const char* virtual_path);

/**
 * @function cf_video_encoder_data
 * @category video
 * @brief    The .mp4 bytes, without writing a file.
 * @param    encoder  The encoder.
 * @param    size     Out parameter for how many bytes.
 * @return   Returns bytes owned by the encoder, or `NULL` on failure.
 * @remarks  Invalidated by the next call to `cf_video_encoder_add_frame` or
 *           `cf_video_encoder_data`. Useful for uploading a replay rather than saving it.
 * @related  CF_VideoEncoder cf_video_encoder_save cf_make_video_from_memory
 */
CF_API const void* CF_CALL cf_video_encoder_data(CF_VideoEncoder* encoder, int* size);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

CF_INLINE CF_Video* make_video(const char* virtual_path) { return cf_make_video(virtual_path); }
CF_INLINE CF_Video* make_video_from_memory(const void* data, int size) { return cf_make_video_from_memory(data, size); }
CF_INLINE void destroy_video(CF_Video* video) { cf_destroy_video(video); }
CF_INLINE const char* video_error() { return cf_video_error(); }
CF_INLINE int video_width(CF_Video* video) { return cf_video_width(video); }
CF_INLINE int video_height(CF_Video* video) { return cf_video_height(video); }
CF_INLINE int video_fps(CF_Video* video) { return cf_video_fps(video); }
CF_INLINE bool video_update(CF_Video* video, float dt) { return cf_video_update(video, dt); }
CF_INLINE bool video_next_frame(CF_Video* video) { return cf_video_next_frame(video); }
CF_INLINE CF_Image video_frame(CF_Video* video) { return cf_video_frame(video); }
CF_INLINE CF_Sprite video_sprite(CF_Video* video) { return cf_video_sprite(video); }
CF_INLINE CF_Texture video_texture(CF_Video* video) { return cf_video_texture(video); }
CF_INLINE void video_set_looped(CF_Video* video, bool looped) { cf_video_set_looped(video, looped); }
CF_INLINE bool video_is_finished(CF_Video* video) { return cf_video_is_finished(video); }
CF_INLINE void video_restart(CF_Video* video) { cf_video_restart(video); }

CF_INLINE CF_VideoEncoder* make_video_encoder(int w, int h, int fps) { return cf_make_video_encoder(w, h, fps); }
CF_INLINE void destroy_video_encoder(CF_VideoEncoder* encoder) { cf_destroy_video_encoder(encoder); }
CF_INLINE void video_encoder_quality(CF_VideoEncoder* encoder, int quality) { cf_video_encoder_quality(encoder, quality); }
CF_INLINE CF_Result video_encoder_add_frame(CF_VideoEncoder* encoder, CF_Image frame) { return cf_video_encoder_add_frame(encoder, frame); }
CF_INLINE CF_Result video_encoder_save(CF_VideoEncoder* encoder, const char* virtual_path) { return cf_video_encoder_save(encoder, virtual_path); }
CF_INLINE const void* video_encoder_data(CF_VideoEncoder* encoder, int* size) { return cf_video_encoder_data(encoder, size); }

}

#endif // CF_CPP

#endif // CF_VIDEO_H
