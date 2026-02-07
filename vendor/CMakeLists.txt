# PhysicsFS for virtual file system and archive support.
if(CF_FRAMEWORK_STATIC)
	set(PHYSFS_BUILD_SHARED OFF CACHE BOOL "Build PhysFS as shared library")
	set(PHYSFS_BUILD_STATIC ON CACHE BOOL "Build PhysFS as static library")
else()
	set(PHYSFS_BUILD_SHARED ON CACHE BOOL "Build PhysFS as shared library")
	set(PHYSFS_BUILD_STATIC OFF CACHE BOOL "Build PhysFS as static library")
endif()
set(PHYSFS_BUILD_TEST OFF CACHE BOOL "Do not build PhysFS tests.")
set(PHYSFS_BUILD_DOCS OFF CACHE BOOL "Do not build PhysFS docs.")
FetchContent_Declare(
	physfs
	GIT_REPOSITORY https://github.com/icculus/physfs.git
	GIT_TAG        9d18d36b5a5207b72f473f05e1b2834e347d8144
	UPDATE_DISCONNECTED TRUE
	EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(physfs)
