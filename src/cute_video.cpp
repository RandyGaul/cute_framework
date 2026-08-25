/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// The Media Foundation hardware-transform attributes need the Windows 7+ API level. Raise it before
// any header pins it lower (CF's defines otherwise bump it only up to Vista's 0x0600).
#if defined(_WIN32) && (!defined(_WIN32_WINNT) || _WIN32_WINNT < 0x0601)
#	undef _WIN32_WINNT
#	define _WIN32_WINNT 0x0601
#endif

#include <cute_alloc.h>

// Buffers cross the boundary in both directions -- the decoder hands out pictures this file copies
// into textures, and ch_mp4_unwrap hands back a stream this file frees -- so both sides must be on
// CF's allocator.
#define CUTE_H264_ALLOC(size) cf_alloc(size)
#define CUTE_H264_FREE(mem) cf_free(mem)
#define CUTE_H264_REALLOC(mem, size) cf_realloc(mem, size)
#define CUTE_H264_NO_STDIO // CF writes through its VFS.
#define CUTE_H264_IMPLEMENTATION
#include <cute/cute_h264.h>

#include <cute_video.h>
#include <cute_file_system.h>
#include <cute_c_runtime.h>
#include <cute/cute_sync.h>

// Hardware encode backend headers.
#if defined(CF_WINDOWS)
#	define WIN32_LEAN_AND_MEAN
#	define NOMINMAX
#	include <windows.h>
#	include <mfapi.h>
#	include <mfidl.h>
#	include <mfreadwrite.h>
#	include <mferror.h>
#elif defined(__APPLE__)
#	include <VideoToolbox/VideoToolbox.h>
#	include <CoreMedia/CoreMedia.h>
#	include <CoreVideo/CoreVideo.h>
#	include <CoreFoundation/CoreFoundation.h>
#endif

#include <internal/cute_alloc_internal.h>

// The codec reports its own failures through ch_error_reason and ch_decoder_error; this covers the
// ones that happen before it is reached, and gives every entry point one place to look.
static const char* s_video_error;

struct CF_Video
{
	uint8_t* annexb;       // the elementary stream, kept so the video can be restarted
	int annexb_size;
	ch_decoder_t* decoder;
	int w, h, fps;
	bool looped;
	bool finished;
	bool has_frame;
	int frame;             // how many frames have been decoded
	float clock;           // seconds of playback owed but not yet taken
	CF_Texture texture;
	bool has_texture;
	int texture_frame;     // which frame the texture holds, so it is not re-uploaded needlessly
	CF_Sprite sprite;
	bool has_sprite;
	int sprite_frame;
	int total_frames;      // counted from the stream when the video is opened
	int* key_offsets;      // byte offsets decoding can start cold at: the SPS ahead of a keyframe
	int* key_bases;        // the display index of each such keyframe
	int key_count;
};

// How many canvas captures may be crossing back from the GPU at once. Harvested every update, so
// reaching the cap means the GPU is a full 8 frames behind -- at that point skipping a capture is
// the right call anyway.
#define CF_VIDEO_GRABS 8

// Which encode backend a CF_VideoEncoder is using. Hardware backends drive the OS's dedicated
// video-encode silicon (fast, real-time); the software backend is cute_h264, portable everywhere.
typedef enum CF_VideoBackendKind
{
	CF_VIDEO_BACKEND_SOFTWARE,   // cute_h264 on a worker thread. Universal fallback.
	CF_VIDEO_BACKEND_MF,         // Windows Media Foundation (hardware H.264).
	CF_VIDEO_BACKEND_VT,         // macOS VideoToolbox (hardware H.264).
} CF_VideoBackendKind;

// One captured frame waiting to be encoded on the worker thread.
typedef struct CF_VideoJob
{
	uint8_t* pixels;             // Owned; freed once encoded.
	int repeats;                 // Encode this picture this many times (a stall repeats a frame).
	struct CF_VideoJob* next;
} CF_VideoJob;

struct CF_VideoEncoder
{
	ch_encoder_t* encoder;
	int w, h, fps;
	const void* mp4;       // owned by this, handed out by cf_video_encoder_data
	CF_Readback grabs[CF_VIDEO_GRABS];   // captures in flight, oldest at grab_first
	int grab_repeats[CF_VIDEO_GRABS];    // how many encoded frames each capture becomes
	int grab_first, grab_num;
	float clock;           // seconds of recording owed but not yet captured

	// Async encode. Captures are read back on the caller's thread, then handed to a worker thread
	// that runs the CPU-bound H.264 encode in the background, so recording never blocks the frame
	// loop. The worker is the ONLY thing that touches `encoder` (the codec), which keeps it
	// single-threaded and its inter-frame prediction intact -- capture and save synchronize with it
	// through the queue below rather than touching the codec concurrently.
	cute_thread_t* worker;
	cute_mutex_t mutex;
	cute_cv_t cv_work;     // Wakes the worker when a job is queued or on shutdown.
	cute_cv_t cv_done;     // Wakes drain waiters when a job finishes.
	CF_VideoJob* head;
	CF_VideoJob* tail;
	int jobs_in_flight;    // Queued-or-encoding count; a drain waits for this to reach zero.
	bool stop;
	bool worker_error;

	// Backend selection. Defaults to the OS hardware encoder when one initializes; otherwise the
	// software fields above (cute_h264 + worker) carry the load. `hw` points at backend-specific
	// state (e.g. the Media Foundation writer).
	CF_VideoBackendKind backend;
	void* hw;
	int quality;
	uint8_t* scratch;      // Reused w*h*4 buffer for synchronous hardware backends.
};

static int s_encode_worker(void* udata);

//--------------------------------------------------------------------------------------------------
// Media Foundation backend (Windows) -- drives the OS hardware H.264 encoder and muxes MP4 itself.

#if defined(CF_WINDOWS)
typedef struct CF_MF
{
	IMFSinkWriter* writer;
	DWORD stream;
	int64_t frame_index;
	wchar_t temp_path[MAX_PATH]; // Sink writer streams here; copied to the user's path on save.
	bool com_inited;
	bool mf_inited;
	bool finalized;
} CF_MF;

static void s_mf_free(CF_MF* mf)
{
	if (!mf) return;
	if (mf->writer) mf->writer->Release();
	if (mf->mf_inited) MFShutdown();
	if (mf->com_inited) CoUninitialize();
	if (mf->temp_path[0]) DeleteFileW(mf->temp_path);
	CF_FREE(mf);
}

// Returns NULL if MF or a hardware encoder is unavailable -- the caller then falls back to software.
static CF_MF* s_mf_make(int w, int h, int fps, int quality)
{
	CF_MF* mf = (CF_MF*)CF_CALLOC(sizeof(CF_MF));
	if (!mf) return NULL;

	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (SUCCEEDED(hr)) mf->com_inited = true;
	else if (hr != RPC_E_CHANGED_MODE) { s_mf_free(mf); return NULL; }
	if (FAILED(MFStartup(MF_VERSION, MFSTARTUP_LITE))) { s_mf_free(mf); return NULL; }
	mf->mf_inited = true;

	wchar_t dir[MAX_PATH];
	DWORD n = GetTempPathW(MAX_PATH, dir);
	if (n == 0 || n >= MAX_PATH) { s_mf_free(mf); return NULL; }
	swprintf(mf->temp_path, MAX_PATH, L"%scf_video_%lu_%p.mp4", dir, (unsigned long)GetCurrentProcessId(), (void*)mf);

	IMFAttributes* attrs = NULL;
	MFCreateAttributes(&attrs, 1);
	attrs->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
	hr = MFCreateSinkWriterFromURL(mf->temp_path, NULL, attrs, &mf->writer);
	if (attrs) attrs->Release();
	if (FAILED(hr)) { s_mf_free(mf); return NULL; }

	// Output: H.264, bitrate scaled by resolution/fps and the requested quality.
	int bitrate = (int)((double)w * h * fps * (0.03 + 0.10 * (quality / 100.0)));
	if (bitrate < 1000000) bitrate = 1000000;
	IMFMediaType* out = NULL; MFCreateMediaType(&out);
	out->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	out->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
	out->SetUINT32(MF_MT_AVG_BITRATE, (UINT32)bitrate);
	out->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
	MFSetAttributeSize(out, MF_MT_FRAME_SIZE, w, h);
	MFSetAttributeRatio(out, MF_MT_FRAME_RATE, fps, 1);
	MFSetAttributeRatio(out, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
	hr = mf->writer->AddStream(out, &mf->stream);
	out->Release();
	if (FAILED(hr)) { s_mf_free(mf); return NULL; }

	// Input: 32-bit RGB (BGRA in memory). The sink writer auto-inserts a converter to NV12.
	IMFMediaType* in = NULL; MFCreateMediaType(&in);
	in->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	in->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
	in->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
	MFSetAttributeSize(in, MF_MT_FRAME_SIZE, w, h);
	MFSetAttributeRatio(in, MF_MT_FRAME_RATE, fps, 1);
	MFSetAttributeRatio(in, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
	hr = mf->writer->SetInputMediaType(mf->stream, in, NULL);
	in->Release();
	if (FAILED(hr)) { s_mf_free(mf); return NULL; }

	if (FAILED(mf->writer->BeginWriting())) { s_mf_free(mf); return NULL; }
	return mf;
}

// Feed one captured frame. `rgba` is the readback: CF_Pixel order (R,G,B,A), top row first. We
// convert to BGRA and flip to bottom-up (RGB32's convention) straight into the sample buffer.
static bool s_mf_write(CF_MF* mf, const uint8_t* rgba, int w, int h, int fps)
{
	int size = w * h * 4, stride = w * 4;
	IMFMediaBuffer* buf = NULL;
	if (FAILED(MFCreateMemoryBuffer(size, &buf))) return false;
	BYTE* dst = NULL;
	if (FAILED(buf->Lock(&dst, NULL, NULL))) { buf->Release(); return false; }
	for (int y = 0; y < h; ++y) {
		const uint8_t* s = rgba + (size_t)y * stride;
		uint8_t* d = dst + (size_t)(h - 1 - y) * stride;
		for (int x = 0; x < w; ++x) { d[0] = s[2]; d[1] = s[1]; d[2] = s[0]; d[3] = 255; s += 4; d += 4; }
	}
	buf->Unlock();
	buf->SetCurrentLength(size);

	IMFSample* sample = NULL;
	if (FAILED(MFCreateSample(&sample))) { buf->Release(); return false; }
	sample->AddBuffer(buf);
	sample->SetSampleTime(mf->frame_index * 10000000LL / fps);
	sample->SetSampleDuration(10000000LL / fps);
	HRESULT hr = mf->writer->WriteSample(mf->stream, sample);
	sample->Release();
	buf->Release();
	if (SUCCEEDED(hr)) mf->frame_index++;
	return SUCCEEDED(hr);
}

static bool s_mf_finalize(CF_MF* mf)
{
	if (mf->finalized) return true;
	mf->finalized = true;
	return SUCCEEDED(mf->writer->Finalize());
}

// Slurp a real (non-VFS) Windows file into an owned buffer.
static uint8_t* s_win_read_file(const wchar_t* path, int* out_size)
{
	*out_size = 0;
	HANDLE f = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (f == INVALID_HANDLE_VALUE) return NULL;
	LARGE_INTEGER sz;
	if (!GetFileSizeEx(f, &sz)) { CloseHandle(f); return NULL; }
	int n = (int)sz.QuadPart;
	uint8_t* data = (uint8_t*)CF_ALLOC((size_t)(n > 0 ? n : 1));
	DWORD read = 0;
	BOOL ok = ReadFile(f, data, (DWORD)n, &read, NULL);
	CloseHandle(f);
	if (!ok || (int)read != n) { CF_FREE(data); return NULL; }
	*out_size = n;
	return data;
}
#endif // CF_WINDOWS

//--------------------------------------------------------------------------------------------------
// VideoToolbox backend (macOS) -- hardware H.264 via VTCompressionSession. Its output is a raw
// H.264 stream, so we accumulate Annex B and hand it to ch_mp4_wrap (the same muxer the software
// backend uses).
//
// NOTE: written against the VideoToolbox/CoreMedia C API but UNTESTED on this machine (no Mac in
// the loop). It compiles only under __APPLE__, so it cannot affect the Windows/Linux builds; treat
// it as a reviewed starting point to verify on macOS.

#if defined(__APPLE__)
typedef struct CF_VT
{
	VTCompressionSessionRef session;
	uint8_t* annexb;      // Accumulated Annex B (start-code-prefixed) output.
	int annexb_size;
	int annexb_cap;
	int w, h, fps;
	int64_t frame_index;
	bool error;
} CF_VT;

static void s_vt_append(CF_VT* vt, const void* data, int n)
{
	if (vt->annexb_size + n > vt->annexb_cap) {
		int cap = vt->annexb_cap ? vt->annexb_cap * 2 : (1 << 20);
		while (cap < vt->annexb_size + n) cap *= 2;
		uint8_t* nb = (uint8_t*)CF_ALLOC((size_t)cap);
		if (!nb) { vt->error = true; return; }
		if (vt->annexb_size) memcpy(nb, vt->annexb, (size_t)vt->annexb_size);
		CF_FREE(vt->annexb);
		vt->annexb = nb;
		vt->annexb_cap = cap;
	}
	memcpy(vt->annexb + vt->annexb_size, data, (size_t)n);
	vt->annexb_size += n;
}

static void s_vt_startcode(CF_VT* vt)
{
	static const uint8_t sc[4] = { 0, 0, 0, 1 };
	s_vt_append(vt, sc, 4);
}

// The compression session hands finished frames here. We convert AVCC (4-byte length prefixes) to
// Annex B, and on each keyframe first emit the SPS/PPS from the format description.
static void s_vt_output(void* refcon, void* src, OSStatus status, VTEncodeInfoFlags flags, CMSampleBufferRef sb)
{
	(void)src; (void)flags;
	CF_VT* vt = (CF_VT*)refcon;
	if (status != noErr || !sb || !CMSampleBufferDataIsReady(sb)) { if (status != noErr) vt->error = true; return; }

	bool keyframe = true;
	CFArrayRef attachments = CMSampleBufferGetSampleAttachmentsArray(sb, false);
	if (attachments && CFArrayGetCount(attachments)) {
		CFDictionaryRef d = (CFDictionaryRef)CFArrayGetValueAtIndex(attachments, 0);
		CFBooleanRef not_sync = (CFBooleanRef)CFDictionaryGetValue(d, kCMSampleAttachmentKey_NotSync);
		if (not_sync && CFBooleanGetValue(not_sync)) keyframe = false;
	}

	if (keyframe) {
		CMFormatDescriptionRef fmt = CMSampleBufferGetFormatDescription(sb);
		size_t count = 0;
		if (CMVideoFormatDescriptionGetH264ParameterSetAtIndex(fmt, 0, NULL, NULL, &count, NULL) == noErr) {
			for (size_t i = 0; i < count; ++i) {
				const uint8_t* ps = NULL; size_t ps_size = 0;
				if (CMVideoFormatDescriptionGetH264ParameterSetAtIndex(fmt, i, &ps, &ps_size, NULL, NULL) == noErr) {
					s_vt_startcode(vt);
					s_vt_append(vt, ps, (int)ps_size);
				}
			}
		}
	}

	CMBlockBufferRef block = CMSampleBufferGetDataBuffer(sb);
	size_t total = 0; char* ptr = NULL;
	if (CMBlockBufferGetDataPointer(block, 0, NULL, &total, &ptr) != noErr) { vt->error = true; return; }
	size_t off = 0;
	while (off + 4 <= total) {
		uint32_t nal = ((uint8_t)ptr[off] << 24) | ((uint8_t)ptr[off + 1] << 16) | ((uint8_t)ptr[off + 2] << 8) | (uint8_t)ptr[off + 3];
		off += 4;
		if (off + nal > total) break;
		s_vt_startcode(vt);
		s_vt_append(vt, ptr + off, (int)nal);
		off += nal;
	}
}

static void s_vt_free(CF_VT* vt)
{
	if (!vt) return;
	if (vt->session) { VTCompressionSessionInvalidate(vt->session); CFRelease(vt->session); }
	CF_FREE(vt->annexb);
	CF_FREE(vt);
}

static CF_VT* s_vt_make(int w, int h, int fps, int quality)
{
	CF_VT* vt = (CF_VT*)CF_CALLOC(sizeof(CF_VT));
	if (!vt) return NULL;
	vt->w = w; vt->h = h; vt->fps = fps;

	// Ask specifically for a hardware encoder; fall back to software cleanly if none.
	CFMutableDictionaryRef spec = CFDictionaryCreateMutable(NULL, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	CFDictionarySetValue(spec, kVTVideoEncoderSpecification_EnableHardwareAcceleratedVideoEncoder, kCFBooleanTrue);
	OSStatus st = VTCompressionSessionCreate(kCFAllocatorDefault, w, h, kCMVideoCodecType_H264, spec, NULL, NULL, s_vt_output, vt, &vt->session);
	CFRelease(spec);
	if (st != noErr || !vt->session) { s_vt_free(vt); return NULL; }

	VTSessionSetProperty(vt->session, kVTCompressionPropertyKey_RealTime, kCFBooleanTrue);
	VTSessionSetProperty(vt->session, kVTCompressionPropertyKey_ProfileLevel, kVTProfileLevel_H264_High_AutoLevel);
	VTSessionSetProperty(vt->session, kVTCompressionPropertyKey_AllowFrameReordering, kCFBooleanTrue);
	int bitrate = (int)((double)w * h * fps * (0.03 + 0.10 * (quality / 100.0)));
	if (bitrate < 1000000) bitrate = 1000000;
	CFNumberRef br = CFNumberCreate(NULL, kCFNumberIntType, &bitrate);
	VTSessionSetProperty(vt->session, kVTCompressionPropertyKey_AverageBitRate, br);
	CFRelease(br);
	int kf = fps * 2;
	CFNumberRef kfn = CFNumberCreate(NULL, kCFNumberIntType, &kf);
	VTSessionSetProperty(vt->session, kVTCompressionPropertyKey_MaxKeyFrameInterval, kfn);
	CFRelease(kfn);
	VTCompressionSessionPrepareToEncodeFrames(vt->session);
	return vt;
}

// Feed one frame. CVPixelBuffer is top-down, so unlike Media Foundation's RGB32 we only swap
// R<->B (into BGRA) and do not flip.
static bool s_vt_write(CF_VT* vt, const uint8_t* rgba)
{
	CVPixelBufferRef pb = NULL;
	if (CVPixelBufferCreate(NULL, vt->w, vt->h, kCVPixelFormatType_32BGRA, NULL, &pb) != kCVReturnSuccess) return false;
	CVPixelBufferLockBaseAddress(pb, 0);
	uint8_t* dst = (uint8_t*)CVPixelBufferGetBaseAddress(pb);
	size_t stride = CVPixelBufferGetBytesPerRow(pb);
	for (int y = 0; y < vt->h; ++y) {
		const uint8_t* s = rgba + (size_t)y * vt->w * 4;
		uint8_t* d = dst + (size_t)y * stride;
		for (int x = 0; x < vt->w; ++x) { d[0] = s[2]; d[1] = s[1]; d[2] = s[0]; d[3] = 255; s += 4; d += 4; }
	}
	CVPixelBufferUnlockBaseAddress(pb, 0);
	CMTime pts = CMTimeMake(vt->frame_index, vt->fps);
	CMTime dur = CMTimeMake(1, vt->fps);
	OSStatus st = VTCompressionSessionEncodeFrame(vt->session, pb, pts, dur, NULL, NULL, NULL);
	CVPixelBufferRelease(pb);
	if (st == noErr) vt->frame_index++;
	return st == noErr;
}

static bool s_vt_finish(CF_VT* vt)
{
	VTCompressionSessionCompleteFrames(vt->session, kCMTimeInvalid);
	return !vt->error && vt->annexb_size > 0;
}
#endif // __APPLE__


// Walks the stream once, counting pictures and remembering every keyframe that can be decoded
// cold -- one with its parameter sets right in front of it, which is how the CF encoder writes
// them. cf_video_seek starts at the nearest of these; a stream carrying none still seeks, it just
// decodes forward from the beginning. A picture is a slice NAL whose first_mb_in_slice is 0, and
// ue(0) is the single bit 1, so the top bit of the first payload byte is the whole test. Nothing
// coded after a keyframe shows before it -- the encoder flushes any held picture first -- so a
// keyframe's display index is simply how many pictures precede it.
static void s_scan(CF_Video* video)
{
	const uint8_t* s = video->annexb;
	int n = video->annexb_size;
	int sps_at = -1;       // start code of an SPS seen since the last picture
	bool pps_after = false;
	int cap = 0;
	for (int i = 0; i + 4 < n; ++i) {
		if (!(s[i] == 0 && s[i + 1] == 0 && s[i + 2] == 1)) continue;
		int sc = (i > 0 && s[i - 1] == 0) ? i - 1 : i;
		int type = s[i + 3] & 0x1f;
		if (type == 7) {                             // sequence parameter set
			sps_at = sc;
			pps_after = false;
		} else if (type == 8) {                      // picture parameter set
			if (sps_at >= 0) pps_after = true;
		} else if ((type == 1 || type == 5) && (s[i + 4] & 0x80)) {
			if (type == 5 && sps_at >= 0 && pps_after) {
				if (video->key_count == cap) {
					cap = cap ? cap * 2 : 16;
					video->key_offsets = (int*)CF_REALLOC(video->key_offsets, sizeof(int) * (size_t)cap);
					video->key_bases = (int*)CF_REALLOC(video->key_bases, sizeof(int) * (size_t)cap);
				}
				video->key_offsets[video->key_count] = sps_at;
				video->key_bases[video->key_count] = video->total_frames;
				++video->key_count;
			}
			++video->total_frames;
			sps_at = -1;
			pps_after = false;
		}
	}
}

// A file is either an elementary stream, which starts with a NAL start code, or a container, which
// starts with a box. Nothing else gets this far.
static bool s_is_annexb(const uint8_t* d, int n)
{
	if (n < 4) return false;
	if (d[0] == 0 && d[1] == 0 && d[2] == 1) return true;
	return d[0] == 0 && d[1] == 0 && d[2] == 0 && d[3] == 1;
}

const char* cf_video_error()
{
	if (s_video_error) return s_video_error;
	if (ch_decoder_error) return ch_decoder_error;
	if (ch_error_reason) return ch_error_reason;
	return "No error.";
}

CF_Video* cf_make_video_from_memory(const void* data, int size)
{
	s_video_error = NULL;
	ch_decoder_error = NULL;
	ch_error_reason = NULL;
	if (!data || size <= 0) {
		s_video_error = "Null or empty video data.";
		return NULL;
	}

	const uint8_t* bytes = (const uint8_t*)data;
	uint8_t* annexb = NULL;
	int annexb_size = 0;
	if (s_is_annexb(bytes, size)) {
		annexb = (uint8_t*)CF_ALLOC((size_t)size);
		if (!annexb) { s_video_error = "Out of memory."; return NULL; }
		CF_MEMCPY(annexb, data, (size_t)size);
		annexb_size = size;
	} else {
		// A container. ch_mp4_unwrap allocates through CF, so the buffer is ours either way.
		annexb = (uint8_t*)ch_mp4_unwrap(data, size, &annexb_size);
		if (!annexb) return NULL;   // ch_error_reason says why
	}

	CF_Video* video = (CF_Video*)CF_CALLOC(sizeof(CF_Video));
	if (!video) { CF_FREE(annexb); s_video_error = "Out of memory."; return NULL; }
	video->annexb = annexb;
	video->annexb_size = annexb_size;
	video->decoder = ch_decoder_make(annexb, annexb_size);
	if (!video->decoder) { CF_FREE(annexb); CF_FREE(video); return NULL; }

	// The size and rate come out of the parameter sets, which sit ahead of the first picture, so
	// one decode is enough to answer them -- and it is a decode the caller was going to need.
	if (!ch_decoder_next(video->decoder)) {
		s_video_error = ch_decoder_error ? ch_decoder_error : "The file carries no pictures.";
		cf_destroy_video(video);
		return NULL;
	}
	ch_decoder_size(video->decoder, &video->w, &video->h);
	video->fps = ch_decoder_fps(video->decoder);
	video->has_frame = true;
	video->frame = 1;
	video->texture_frame = -1;
	video->sprite_frame = -1;
	s_scan(video);
	return video;
}

CF_Video* cf_make_video(const char* virtual_path)
{
	s_video_error = NULL;
	size_t size = 0;
	void* data = cf_fs_read_entire_file_to_memory(virtual_path, &size);
	if (!data) {
		s_video_error = "Unable to read the video file from the virtual file system.";
		return NULL;
	}
	if (size > 0x7fffffff) {
		CF_FREE(data);
		s_video_error = "Video files over 2GB are not supported.";
		return NULL;
	}
	CF_Video* video = cf_make_video_from_memory(data, (int)size);
	CF_FREE(data);
	return video;
}

void cf_destroy_video(CF_Video* video)
{
	if (!video) return;
	if (video->has_sprite) cf_easy_sprite_unload(&video->sprite);
	if (video->has_texture) cf_destroy_texture(video->texture);
	if (video->decoder) ch_decoder_destroy(video->decoder);
	CF_FREE(video->key_offsets);
	CF_FREE(video->key_bases);
	CF_FREE(video->annexb);
	CF_FREE(video);
}

int cf_video_width(CF_Video* video) { return video ? video->w : 0; }
int cf_video_height(CF_Video* video) { return video ? video->h : 0; }
int cf_video_fps(CF_Video* video) { return video ? video->fps : 0; }
bool cf_video_is_finished(CF_Video* video) { return video ? video->finished : true; }
void cf_video_set_looped(CF_Video* video, bool looped) { if (video) video->looped = looped; }
int cf_video_frame_count(CF_Video* video) { return video ? video->total_frames : 0; }
int cf_video_frame_index(CF_Video* video) { return video && video->has_frame ? video->frame - 1 : 0; }
float cf_video_duration(CF_Video* video) { return video && video->fps > 0 ? (float)video->total_frames / (float)video->fps : 0; }

bool cf_video_seek(CF_Video* video, int frame)
{
	if (!video) return false;
	if (frame < 0 || frame >= video->total_frames) {
		s_video_error = "No such frame.";
		return false;
	}
	// The nearest cold-start keyframe at or before the target; a stream without any decodes
	// forward from the top.
	int base = 0, offset = 0;
	for (int i = 0; i < video->key_count && video->key_bases[i] <= frame; ++i) {
		base = video->key_bases[i];
		offset = video->key_offsets[i];
	}
	ch_decoder_error = NULL;
	ch_decoder_t* decoder = ch_decoder_make(video->annexb + offset, video->annexb_size - offset);
	for (int i = frame - base + 1; decoder && i > 0; --i) {
		if (!ch_decoder_next(decoder)) {
			ch_decoder_destroy(decoder);
			decoder = NULL;
		}
	}
	if (!decoder) {
		s_video_error = ch_decoder_error ? ch_decoder_error : "Unable to seek.";
		return false;   // the old decoder was never touched, so playback stands where it was
	}
	if (video->decoder) ch_decoder_destroy(video->decoder);
	video->decoder = decoder;
	video->frame = frame + 1;
	video->has_frame = true;
	video->finished = false;
	video->clock = 0;
	return true;
}

void cf_video_restart(CF_Video* video)
{
	if (!video) return;
	ch_decoder_error = NULL;
	if (video->decoder) ch_decoder_destroy(video->decoder);
	video->decoder = ch_decoder_make(video->annexb, video->annexb_size);
	video->finished = false;
	video->has_frame = false;
	video->frame = 0;
	video->clock = 0;
	if (video->decoder && ch_decoder_next(video->decoder)) {
		video->has_frame = true;
		video->frame = 1;
	}
}

bool cf_video_next_frame(CF_Video* video)
{
	if (!video || !video->decoder) return false;
	ch_decoder_error = NULL;
	if (!ch_decoder_next(video->decoder)) {
		if (ch_decoder_error) s_video_error = ch_decoder_error;
		video->finished = true;
		return false;
	}
	video->has_frame = true;
	++video->frame;
	return true;
}

bool cf_video_update(CF_Video* video, float dt)
{
	if (!video || !video->decoder) return false;
	int fps = video->fps > 0 ? video->fps : 30;
	float step = 1.0f / (float)fps;
	video->clock += dt;
	// A stall must not turn into a long catch-up that stalls the game in turn, so the clock is
	// clamped rather than being allowed to owe an unbounded number of frames.
	if (video->clock > step * 4) video->clock = step * 4;

	bool advanced = false;
	while (video->clock >= step) {
		video->clock -= step;
		if (!cf_video_next_frame(video)) {
			if (!video->looped) { video->clock = 0; break; }
			cf_video_restart(video);
			advanced = true;
			break;
		}
		advanced = true;
	}
	return advanced;
}

CF_Image cf_video_frame(CF_Video* video)
{
	CF_Image image = { 0, 0, NULL };
	if (!video || !video->has_frame) return image;
	const void* rgba = ch_decoder_rgba(video->decoder);
	if (!rgba) return image;
	image.w = video->w;
	image.h = video->h;
	image.pix = (CF_Pixel*)rgba;
	return image;
}

CF_Sprite cf_video_sprite(CF_Video* video)
{
	if (!video) return cf_sprite_defaults();
	CF_Image image = cf_video_frame(video);
	if (!video->has_sprite) {
		if (!image.pix) return cf_sprite_defaults();
		video->sprite = cf_make_easy_sprite_from_pixels(image.pix, image.w, image.h);
		video->has_sprite = true;
		video->sprite_frame = video->frame;
		return video->sprite;
	}
	if (video->sprite_frame != video->frame && image.pix) {
		cf_easy_sprite_update_pixels(&video->sprite, image.pix);
		video->sprite_frame = video->frame;
	}
	return video->sprite;
}

CF_Texture cf_video_texture(CF_Video* video)
{
	CF_Texture none = { 0 };
	if (!video) return none;
	if (!video->has_texture) {
		CF_TextureParams params = cf_texture_defaults(video->w, video->h);
		params.filter = CF_FILTER_LINEAR;
		video->texture = cf_make_texture(params);
		video->has_texture = true;
		video->texture_frame = -1;
	}
	if (video->texture_frame != video->frame) {
		CF_Image image = cf_video_frame(video);
		if (image.pix) {
			cf_texture_update(video->texture, image.pix, image.w * image.h * (int)sizeof(CF_Pixel));
			video->texture_frame = video->frame;
		}
	}
	return video->texture;
}

// -------------------------------------------------------------------------------------------------

CF_VideoEncoder* cf_make_video_encoder(int w, int h, int fps)
{
	s_video_error = NULL;
	ch_error_reason = NULL;
	CF_VideoEncoder* encoder = (CF_VideoEncoder*)CF_CALLOC(sizeof(CF_VideoEncoder));
	if (!encoder) { s_video_error = "Out of memory."; return NULL; }
	encoder->w = w;
	encoder->h = h;
	encoder->fps = fps;
	encoder->quality = 50;

	// Prefer the OS hardware encoder (real-time, GPU-backed). Falls back to software below if it is
	// unavailable (VM, headless, blocked driver, unsupported platform). Hardware encoders reject or
	// stall on tiny resolutions (their fixed-function blocks have minimum dimensions), so only reach
	// for hardware at sizes a real recording actually uses; small canvases go to software.
	bool hw_ok_size = (w >= 128 && h >= 128);
#if defined(CF_WINDOWS)
	if (hw_ok_size) {
		CF_MF* mf = s_mf_make(w, h, fps, encoder->quality);
		if (mf) {
			encoder->backend = CF_VIDEO_BACKEND_MF;
			encoder->hw = mf;
			encoder->scratch = (uint8_t*)CF_ALLOC((size_t)w * h * 4);
			return encoder;
		}
	}
#elif defined(__APPLE__)
	if (hw_ok_size) {
		CF_VT* vt = s_vt_make(w, h, fps, encoder->quality);
		if (vt) {
			encoder->backend = CF_VIDEO_BACKEND_VT;
			encoder->hw = vt;
			encoder->scratch = (uint8_t*)CF_ALLOC((size_t)w * h * 4);
			return encoder;
		}
	}
#else
	(void)hw_ok_size;
#endif

	// Linux hardware encode (VA-API) is a future backend -- it's a low-level interface (the caller
	// manages sequence/picture/slice params, references and rate control), best done by a maintainer
	// who can iterate on a Linux box. Until then Linux uses the software fallback below.

	// Software fallback: cute_h264 on a background worker thread.
	encoder->backend = CF_VIDEO_BACKEND_SOFTWARE;
	encoder->encoder = ch_encoder_make(w, h, fps);
	if (!encoder->encoder) { CF_FREE(encoder); return NULL; }
	// The defaults this API promises: the better entropy coder, and pictures that predict from
	// both sides. Both cost encode time and neither costs compatibility with anything modern.
	ch_encoder_cabac(encoder->encoder, 1);
	ch_encoder_bframes(encoder->encoder, 1);
	cf_video_encoder_set_quality(encoder, 50);

	// Spin up the background encode worker last, once the codec is fully configured.
	encoder->mutex = cute_mutex_create();
	encoder->cv_work = cute_cv_create();
	encoder->cv_done = cute_cv_create();
	encoder->worker = cute_thread_create(s_encode_worker, "cf_video_encode", encoder);
	return encoder;
}

void cf_destroy_video_encoder(CF_VideoEncoder* encoder)
{
	if (!encoder) return;
	if (encoder->backend == CF_VIDEO_BACKEND_SOFTWARE) {
		// Stop the encode worker and wait for it to unwind. It finishes any job it had already
		// popped, then exits; jobs still queued are freed below rather than encoded.
		if (encoder->worker) {
			cute_lock(&encoder->mutex);
			encoder->stop = true;
			cute_cv_wake_all(&encoder->cv_work);
			cute_unlock(&encoder->mutex);
			cute_thread_wait(encoder->worker);
		}
		for (CF_VideoJob* job = encoder->head; job; ) {
			CF_VideoJob* next = job->next;
			CF_FREE(job->pixels);
			CF_FREE(job);
			job = next;
		}
		cute_cv_destroy(&encoder->cv_work);
		cute_cv_destroy(&encoder->cv_done);
		cute_mutex_destroy(&encoder->mutex);
		if (encoder->encoder) ch_encoder_destroy(encoder->encoder);
	}
#if defined(CF_WINDOWS)
	else if (encoder->backend == CF_VIDEO_BACKEND_MF) {
		s_mf_free((CF_MF*)encoder->hw);
	}
#elif defined(__APPLE__)
	else if (encoder->backend == CF_VIDEO_BACKEND_VT) {
		s_vt_free((CF_VT*)encoder->hw);
	}
#endif
	// Captures still in flight are simply dropped -- their pixels have nowhere to go.
	for (int i = 0; i < encoder->grab_num; ++i) {
		cf_destroy_readback(encoder->grabs[(encoder->grab_first + i) % CF_VIDEO_GRABS]);
	}
	CF_FREE(encoder->scratch);
	CF_FREE((void*)encoder->mp4);
	CF_FREE(encoder);
}

void cf_video_encoder_set_quality(CF_VideoEncoder* encoder, int quality)
{
	if (!encoder) return;
	if (quality < 0) quality = 0;
	if (quality > 100) quality = 100;
	encoder->quality = quality;
	// Hardware backends bake their bitrate at creation time (before any frame), so there is no
	// quantizer to turn here -- the stored value above is informational for them.
	if (encoder->backend != CF_VIDEO_BACKEND_SOFTWARE) return;
	// The codec's knob is a quantizer: 51 throws nearly everything away, 0 keeps nearly all of it,
	// and -1 is the separate lossless path. 100 is the only value that reaches it, because
	// lossless is a different kind of thing rather than the top of the same scale.
	if (quality == 100) {
		ch_encoder_qp(encoder->encoder, -1);
		// Lossless codes every macroblock as raw samples, where B pictures have nothing to offer
		// and cost a frame of delay for it.
		ch_encoder_bframes(encoder->encoder, 0);
	} else {
		ch_encoder_qp(encoder->encoder, 51 - quality * 51 / 100);
		// Coming back from lossless has to undo the line above, or a dip to 100 and back would
		// leave B pictures off for the rest of the recording.
		ch_encoder_bframes(encoder->encoder, 1);
	}
}

// Compresses one finished capture, feeding it in `repeats` times when the recording owes more
// than one frame of time -- the pictures are identical so the copies land as P_Skip and cost a
// few bytes each, which is what keeps the file true to the wall clock.
// Hand an owned pixel buffer to the worker thread. Non-blocking.
static void s_enqueue(CF_VideoEncoder* encoder, uint8_t* pixels, int repeats)
{
	CF_VideoJob* job = (CF_VideoJob*)CF_ALLOC(sizeof(CF_VideoJob));
	job->pixels = pixels;
	job->repeats = repeats;
	job->next = NULL;
	cute_lock(&encoder->mutex);
	if (encoder->tail) encoder->tail->next = job; else encoder->head = job;
	encoder->tail = job;
	encoder->jobs_in_flight++;
	cute_cv_wake_one(&encoder->cv_work);
	cute_unlock(&encoder->mutex);
}

// The background encode loop: pop a job, run the (slow) codec on it outside the lock, repeat.
static int s_encode_worker(void* udata)
{
	CF_VideoEncoder* encoder = (CF_VideoEncoder*)udata;
	for (;;) {
		cute_lock(&encoder->mutex);
		while (!encoder->head && !encoder->stop) cute_cv_wait(&encoder->cv_work, &encoder->mutex);
		if (!encoder->head && encoder->stop) { cute_unlock(&encoder->mutex); break; }
		CF_VideoJob* job = encoder->head;
		encoder->head = job->next;
		if (!encoder->head) encoder->tail = NULL;
		cute_unlock(&encoder->mutex);

		for (int i = 0; i < job->repeats; ++i) {
			ch_error_reason = NULL;
			if (!ch_encoder_frame(encoder->encoder, job->pixels)) { encoder->worker_error = true; break; }
		}
		CF_FREE(job->pixels);
		CF_FREE(job);

		cute_lock(&encoder->mutex);
		encoder->jobs_in_flight--;
		cute_cv_wake_all(&encoder->cv_done);
		cute_unlock(&encoder->mutex);
	}
	return 0;
}

// Read a completed GPU capture back into an owned buffer and queue it for the worker. The readback
// stays on the caller's thread (it needs the GPU); only the encode itself runs in the background.
static int s_feed(CF_VideoEncoder* encoder, CF_Readback readback, int repeats)
{
	int size = encoder->w * encoder->h * (int)sizeof(CF_Pixel);
	if (cf_readback_size(readback) != size) {
		s_video_error = "The canvas does not match the size the encoder was created with.";
		return 0;
	}
#if defined(CF_WINDOWS)
	if (encoder->backend == CF_VIDEO_BACKEND_MF) {
		if (cf_readback_data(readback, encoder->scratch, size) != size) {
			s_video_error = "Unable to read the capture back.";
			return 0;
		}
		CF_MF* mf = (CF_MF*)encoder->hw;
		int added = 0;
		for (int i = 0; i < repeats; ++i) {
			if (!s_mf_write(mf, encoder->scratch, encoder->w, encoder->h, encoder->fps)) {
				s_video_error = "The hardware encoder rejected a frame.";
				break;
			}
			++added;
		}
		return added;
	}
#elif defined(__APPLE__)
	if (encoder->backend == CF_VIDEO_BACKEND_VT) {
		if (cf_readback_data(readback, encoder->scratch, size) != size) {
			s_video_error = "Unable to read the capture back.";
			return 0;
		}
		CF_VT* vt = (CF_VT*)encoder->hw;
		int added = 0;
		for (int i = 0; i < repeats; ++i) {
			if (!s_vt_write(vt, encoder->scratch)) { s_video_error = "The hardware encoder rejected a frame."; break; }
			++added;
		}
		return added;
	}
#endif
	// Software: own a copy per frame and hand it to the worker.
	uint8_t* pixels = (uint8_t*)CF_ALLOC((size_t)size);
	if (!pixels) { s_video_error = "Out of memory."; return 0; }
	if (cf_readback_data(readback, pixels, size) != size) {
		CF_FREE(pixels);
		s_video_error = "Unable to read the capture back.";
		return 0;
	}
	s_enqueue(encoder, pixels, repeats);
	return repeats;
}

// Block until the worker has encoded everything queued so far. After this returns the worker is
// idle, so the caller may safely touch the codec (flush/read the bitstream).
static void s_wait_encoded(CF_VideoEncoder* encoder)
{
	cute_lock(&encoder->mutex);
	while (encoder->jobs_in_flight > 0) cute_cv_wait(&encoder->cv_done, &encoder->mutex);
	cute_unlock(&encoder->mutex);
	if (encoder->worker_error && !s_video_error) s_video_error = "The encoder failed on a background frame.";
}

// Compresses every capture the GPU has finished with, in the order they were asked for. Stops at
// the first one still in flight -- order is part of what is being recorded.
static int s_harvest(CF_VideoEncoder* encoder)
{
	int added = 0;
	while (encoder->grab_num) {
		CF_Readback readback = encoder->grabs[encoder->grab_first];
		if (!cf_readback_ready(readback)) break;
		added += s_feed(encoder, readback, encoder->grab_repeats[encoder->grab_first]);
		cf_destroy_readback(readback);
		encoder->grab_first = (encoder->grab_first + 1) % CF_VIDEO_GRABS;
		--encoder->grab_num;
	}
	return added;
}

// The blocking version, for save: everything in flight is waited for, so the file carries every
// frame the recording owes. The wait is bounded -- a readback whose fence never signals (a lost
// device, a driver fault) drops the remaining captures rather than hanging the save.
static int s_drain(CF_VideoEncoder* encoder)
{
	int added = 0;
	while (encoder->grab_num) {
		CF_Readback readback = encoder->grabs[encoder->grab_first];
		for (int64_t spin = 0; !cf_readback_ready(readback) && spin < ((int64_t)1 << 28); ++spin) {}
		if (!cf_readback_ready(readback)) {
			for (int i = 0; i < encoder->grab_num; ++i) {
				cf_destroy_readback(encoder->grabs[(encoder->grab_first + i) % CF_VIDEO_GRABS]);
			}
			encoder->grab_num = 0;
			s_video_error = "A capture never came back from the GPU.";
			break;
		}
		added += s_feed(encoder, readback, encoder->grab_repeats[encoder->grab_first]);
		cf_destroy_readback(readback);
		encoder->grab_first = (encoder->grab_first + 1) % CF_VIDEO_GRABS;
		--encoder->grab_num;
	}
	// Everything captured has been queued; wait for the worker to actually finish encoding it so
	// the bitstream is complete before the caller reads it out.
	s_wait_encoded(encoder);
	return added;
}

int cf_video_encoder_update(CF_VideoEncoder* encoder, CF_Canvas canvas, float dt)
{
	if (!encoder) { s_video_error = "Null encoder."; return 0; }
	int added = s_harvest(encoder);
	float step = 1.0f / (float)encoder->fps;
	encoder->clock += dt;
	// A stall must not turn into a burst of catch-up frames, so the recording drops behind
	// instead -- the mirror of the clamp in cf_video_update.
	if (encoder->clock > step * 4) encoder->clock = step * 4;
	int owed = (int)(encoder->clock / step);
	if (owed > 0 && encoder->grab_num < CF_VIDEO_GRABS) {
		CF_Readback readback = cf_canvas_readback(canvas);
		if (!readback.id) {
			s_video_error = "Canvas readback failed, or is unsupported on this platform.";
			return added;
		}
		int at = (encoder->grab_first + encoder->grab_num) % CF_VIDEO_GRABS;
		encoder->grabs[at] = readback;
		encoder->grab_repeats[at] = owed;
		++encoder->grab_num;
		encoder->clock -= step * (float)owed;
	}
	return added;
}

CF_Result cf_video_encoder_add_frame(CF_VideoEncoder* encoder, CF_Image frame)
{
	if (!encoder) return cf_result_error("Null encoder.");
	if (!frame.pix) return cf_result_error("Null pixels.");
	if (frame.w != encoder->w || frame.h != encoder->h) {
		return cf_result_error("Frame size does not match the encoder.");
	}
#if defined(CF_WINDOWS)
	if (encoder->backend == CF_VIDEO_BACKEND_MF) {
		return s_mf_write((CF_MF*)encoder->hw, (const uint8_t*)frame.pix, encoder->w, encoder->h, encoder->fps)
			? cf_result_success() : cf_result_error("The hardware encoder rejected a frame.");
	}
#elif defined(__APPLE__)
	if (encoder->backend == CF_VIDEO_BACKEND_VT) {
		return s_vt_write((CF_VT*)encoder->hw, (const uint8_t*)frame.pix)
			? cf_result_success() : cf_result_error("The hardware encoder rejected a frame.");
	}
#endif
	// Software: queue a copy for the worker rather than encoding here -- the codec is only ever
	// touched by the worker thread.
	int size = encoder->w * encoder->h * (int)sizeof(CF_Pixel);
	uint8_t* pixels = (uint8_t*)CF_ALLOC((size_t)size);
	if (!pixels) return cf_result_error("Out of memory.");
	CF_MEMCPY(pixels, frame.pix, (size_t)size);
	s_enqueue(encoder, pixels, 1);
	return cf_result_success();
}

const void* cf_video_encoder_data(CF_VideoEncoder* encoder, int* size)
{
	if (size) *size = 0;
	if (!encoder) { s_video_error = "Null encoder."; return NULL; }
	s_drain(encoder);
#if defined(CF_WINDOWS)
	if (encoder->backend == CF_VIDEO_BACKEND_MF) {
		CF_MF* mf = (CF_MF*)encoder->hw;
		if (!s_mf_finalize(mf)) { s_video_error = "The hardware encoder failed to finalize."; return NULL; }
		int n = 0;
		uint8_t* data = s_win_read_file(mf->temp_path, &n);
		if (!data) { s_video_error = "Could not read the encoded video."; return NULL; }
		CF_FREE((void*)encoder->mp4);
		encoder->mp4 = data;
		if (size) *size = n;
		return encoder->mp4;
	}
#endif
	ch_error_reason = NULL;
	int raw_size = 0;
	const void* raw = NULL;
#if defined(__APPLE__)
	if (encoder->backend == CF_VIDEO_BACKEND_VT) {
		CF_VT* vt = (CF_VT*)encoder->hw;
		if (!s_vt_finish(vt)) { s_video_error = "The hardware encoder produced no data."; return NULL; }
		raw = vt->annexb;
		raw_size = vt->annexb_size;
	} else
#endif
	{
		raw = ch_encoder_data(encoder->encoder, &raw_size);
	}
	if (!raw || !raw_size) { s_video_error = "The encoder has no frames in it."; return NULL; }
	CF_FREE((void*)encoder->mp4);
	int mp4_size = 0;
	encoder->mp4 = ch_mp4_wrap(raw, raw_size, encoder->w, encoder->h,
		encoder->fps, &mp4_size);
	if (!encoder->mp4) return NULL;
	if (size) *size = mp4_size;
	return encoder->mp4;
}

CF_Result cf_video_encoder_save(CF_VideoEncoder* encoder, const char* virtual_path)
{
	if (!encoder) return cf_result_error("Null encoder.");
	if (!virtual_path) return cf_result_error("Null path.");
	if (encoder->backend != CF_VIDEO_BACKEND_SOFTWARE) {
		// Hardware backends emit MP4; write it out via the data path.
		int size = 0;
		const void* mp4 = cf_video_encoder_data(encoder, &size);
		if (!mp4) return cf_result_error(cf_video_error());
		return cf_fs_write_entire_buffer_to_file(virtual_path, mp4, (size_t)size);
	}
	size_t len = CF_STRLEN(virtual_path);
	bool raw_stream = (len > 5 && !CF_STRCMP(virtual_path + len - 5, ".h264"))
	               || (len > 4 && !CF_STRCMP(virtual_path + len - 4, ".264"));
	if (raw_stream) {
		s_drain(encoder);
		ch_error_reason = NULL;
		int size = 0;
		const void* data = ch_encoder_data(encoder->encoder, &size);
		if (!data || !size) return cf_result_error("The encoder has no frames in it.");
		return cf_fs_write_entire_buffer_to_file(virtual_path, data, (size_t)size);
	}
	int size = 0;
	const void* mp4 = cf_video_encoder_data(encoder, &size);
	if (!mp4) return cf_result_error(cf_video_error());
	return cf_fs_write_entire_buffer_to_file(virtual_path, mp4, (size_t)size);
}
