# Android Builds

Android is a Java-based platform. Apps are packaged as APK files containing Java bytecode, resources, and metadata. Native C/C++ code cannot run directly - it must be compiled into a shared library (.so) and loaded by a Java app via JNI (Java Native Interface).

Cute Framework is built on top of SDL3, which handles this complexity by providing a Java activity (`SDLActivity`) that initializes the Android environment, creates a window/surface, and loads your native code. Your game compiles to `libmain.so`, which SDL's Java code loads at startup. This is why Android builds require all this project structure - it's fundamentally a Java app that hosts your native code.

SDL3 provides an `android-project` template with the necessary Java scaffolding. We use this template as our starting point, adding CF and your game code to it. The build process uses Gradle (Android's build system) to compile the Java wrapper, invoke CMake/NDK to cross-compile your C code for ARM, and package everything into an APK.

> [!NOTE]
> Cute Framework supports Android 7.0 (API level 24) and newer. Vulkan is preferred with automatic fallback to OpenGL ES 3.0.

## Prerequisites

Install via Android Studio's SDK Manager (Tools > SDK Manager > SDK Tools):
- Android SDK (API 24+)
- NDK (Side by side)
- CMake 3.31+ (the default 3.22 has bugs with FetchContent)
- Build-Tools

To install CMake 3.31+ from command line:
```
sdkmanager "cmake;3.31.6"
```

Environment variables JAVA_HOME and ANDROID_HOME must be set for Gradle to work. Android Studio typically sets these, but if building from command line you may need to set them manually.

## Project Structure

The android project is based on SDL3's `android-project` template with CF and your game added:

```
android/
├── build.gradle                 <- top-level gradle config
├── settings.gradle              <- project settings
├── gradle.properties            <- gradle properties
├── gradlew                      <- gradle wrapper (linux/mac)
├── gradlew.bat                  <- gradle wrapper (windows)
├── gradle/
│   └── wrapper/                 <- gradle wrapper JAR
├── app/
│   ├── build.gradle             <- app build config (edit cmake version here)
│   ├── proguard-rules.pro       <- proguard rules (required)
│   ├── jni/
│   │   ├── CMakeLists.txt       <- entry point, just calls add_subdirectory(src)
│   │   ├── cute_framework/      <- COPY of cute_framework (exclude build folders)
│   │   │   ├── CMakeLists.txt
│   │   │   ├── include/
│   │   │   ├── src/
│   │   │   └── libraries/
│   │   └── src/
│   │       ├── CMakeLists.txt   <- your game's cmake config
│   │       ├── main.c           <- COPY of your game source
│   │       └── *.c, *.h         <- COPY of other source files
│   └── src/main/
│       ├── AndroidManifest.xml  <- app manifest
│       ├── assets/              <- COPY of your game assets
│       ├── java/                <- SDL Java wrapper (from template)
│       │   └── org/libsdl/app/
│       │       ├── SDLActivity.java
│       │       └── ...
│       └── res/                 <- android resources
│           ├── mipmap-*/        <- app icons
│           └── values/          <- strings.xml, styles.xml
```

Paths marked COPY are synced from your main project before each build. The library in `jni/src/` must be named `main` for SDL's Java activity to load it.

## Source Control

The project contains scaffolding (version control) and copied source/assets (ignore).

**Check in:**
- `build.gradle`, `settings.gradle`, `gradle.properties`
- `gradlew`, `gradlew.bat`, `gradle/`
- `app/build.gradle`, `app/proguard-rules.pro`
- `app/jni/CMakeLists.txt`
- `app/jni/src/CMakeLists.txt`
- `app/src/main/AndroidManifest.xml`
- `app/src/main/java/`
- `app/src/main/res/`

**Add to `.gitignore`:**

```
# Build artifacts
android/.gradle/
android/build/
android/app/.cxx/
android/app/build/

# Copied before build - sync from main project
android/app/jni/cute_framework/
android/app/jni/src/*.c
android/app/jni/src/*.h
android/app/src/main/assets/
```

Create a build script that syncs these paths before running gradle. This keeps your repo lean and ensures Android always builds from current source.

## Setup

### 1. Locate SDL3's android-project template

Run CMake once for your desktop build - this fetches SDL3 which contains the template:
```
cmake -B build
```

Then find the template at:
```
build/_deps/sdl3-src/android-project/
```
Copy this folder as your Android project base.

### 2. Copy cute_framework

Copy `cute_framework` into `app/jni/cute_framework/`. Exclude any `build*` folders - they're large and unnecessary.

### 3. Copy your game source and assets

- Copy source files to `app/jni/src/`
- Copy assets to `app/src/main/assets/`

### 4. Edit app/jni/CMakeLists.txt

Replace contents with:
```cmake
cmake_minimum_required(VERSION 3.22)
project(GAME)
add_subdirectory(src)
```

### 5. Edit app/jni/src/CMakeLists.txt

Replace contents with:
```cmake
cmake_minimum_required(VERSION 3.22)
project(mygame)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../cute_framework cf_build)

add_library(main SHARED
    main.c
)

target_include_directories(main PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(main PRIVATE cute)
```
Add all your source files to the `add_library` call. The library must be named `main` to work with SDL's Android activity.

### 6. Edit app/build.gradle

Find the `externalNativeBuild` > `cmake` block and add the version:
```gradle
cmake {
    path 'jni/CMakeLists.txt'
    version '3.31.6'
}
```

### 7. Edit app name

Change the app name in `app/src/main/res/values/strings.xml`.

## Building

On Linux/Mac, make gradlew executable first: `chmod +x gradlew`

```
./gradlew assembleDebug -PBUILD_WITH_CMAKE=1
```

APK output: `app/build/outputs/apk/debug/app-debug.apk`

## Installing

```
adb install app/build/outputs/apk/debug/app-debug.apk
```

## Assets

Files in `app/src/main/assets/` are accessible via CF's virtual filesystem (e.g. `/textures/player.ase`).

## Common Issues

**FetchContent/URL errors**: Install CMake 3.31+ via SDK Manager. The bundled 3.22 has bugs that breaks FetchContent.

**Crash on startup**: Check `adb logcat`. Usually missing assets or library not named `main`.