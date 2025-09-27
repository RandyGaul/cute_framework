#ifdef IMGUI_ENABLE_FREETYPE
#ifndef CIMGUI_FREETYPE
#error "IMGUI_FREETYPE should be defined for Freetype linking"
#endif
#else
#ifdef CIMGUI_FREETYPE
#error "IMGUI_FREETYPE should not be defined without freetype generated cimgui"
#endif
#endif

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define IMGUI_STB_NAMESPACE ImStb

#undef NDEBUG