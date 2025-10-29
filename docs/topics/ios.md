# macOS and iOS Builds

Building a game for iOS requires you to setup a developer account, and slightly complicates a few CMake commands. However, once setup it's very easy to build and install your game onto iOS devices through Xcode. Building your game for MacOS is even simpler than on iOS!

> [!NOTE]
> Just a friendly reminder that CF only supports 64-bit builds. There is no support for 32-bit builds.

## Setup your MacOS Device

Here are the recommended steps to setup your MacOS device for building a game with Cute Framework. You may have already performed some of them if you're an experienced developer.

- Install Xcode. You can do this from the app store. You must have opened Xcode at least once after the initial download.
- Install [Brew](https://brew.sh/). You should only have to open a terminal and copy + paste the command seen on the Brew homepage.
- Install CMake. You can do this with brew by typing `brew install cmake` into your terminal, or you can manually download [CMake from their website](https://cmake.org/download/).
- Install git. You can do this with brew by typing `brew install git` into your terminal, or you can manually download [Git from their website](https://git-scm.com/downloads).
- Enter `sudo xcode-select --reset` into your terminal.

## Generating an Xcode Project with CMake

If you are a beginner to CMake it's best to use [CF's project template](https://github.com/RandyGaul/cute_framework_project_template) to setup your game. Your project's directory layout will look like this:

```
/super_cool_game
    ├─ .gitignore
    ├─ CMakeLists.txt
    ├─ README.md
    └─ src/
        └─ main.cpp
```

> [!NOTE]
> The `.gitignore` file is there just to ignore any `.DS_Store` files, and anything in any folder starting with the name `build*`. This helps prevent git from capturing any unwanted files, especially our build files we will be generating shortly. You can add any other folder or files here you wish to ignore in source control. Usually you will want to ignore generated files.

Once downloaded all your source code can live in the `src` folder. To start with you should already have a basic `main.cpp` to open a window with CF.

Before we generate an Xcode project with CMake we must enter in some information about your game into CMakeLists.txt. Open up CMakeLists.txt and find + replace "mygame" with the name of your game. No spaces, underscores, or special characters allowed!

You can now open up a terminal in your top-level directory of your project.

## Generating for MacOS

Simply make a folder called `build_macos` in your project's top-level directory, then run a special CMake command. Here's an example of both steps you can run in a terminal opened in the projects top-level directory:

```
mkdir build_macos
cmake -G "Xcode" -Bbuild_macos .
```

CMake will print out all kinds of information to the terminal. Your project folder should now look something like this once CMake finishes:

```
/super_cool_game
    ├─ .gitignore
    ├─ build_macos/
    |  └─ lots of stuff!
    ├─ CMakeLists.txt
    ├─ README.md
    └─ src/
        └─ main.cpp
```

## Actually Building the Code for MacOS

There are two major ways to actually build your game at this point. One is through another CMake command (not recommended for beginners), and the other is to open Xcode and build your game from there (recommended).

### Building through CMake

You can build your game by running this command from your terminal (opened in the top-level directory of your project):

```
cmake --build build_macos
```

This may build in Debug mode by default, so you may prefer the following to build in Release mode:

```
cmake --build build_macos --config Release
```

### Building with Xcode

Inside the folder `build_macos` you can see a generated .xcodeproject file. Double-click it to open it. From here you can use Xcode to build your game.

## Generating for iOS

Before you can generate a proper build setup for iOS it's highly recommended to setup a developer account with Apple. This costs $99 a year, but you get access to generating certificates for your iOS apps, which allows you install your app onto your phone for testing, and allows you to distribute your app on the App store. Nowadays Xcode can auto-magically setup signing of apps so long as you provide your Apple Developer ID through CMake (details on this later).

### Setting up an Apple Developer Account

Visit [developer.apple.com](https://developer.apple.com/) and sign up for a developer account. Once completed, you need to request a certificate for both Apple Development and Distribution. You can do this by opening KeyChain Access on your Mac developer machine. Go to Keychain Access > Certificate Assistant > Request a Certificate from a Certificate Authority. Check the “Saved to disk” option. This will save a CertificateSigningRequest.certSigningRequest file.

Then head over to your developer account at [developer.apple.com](https://developer.apple.com/) and go to Account > Certificates. Click the blue plus sign + to add a certificate. This will ask for your CertificateSigningRequest.certSigningRequest file. You’ll be asked about what entitlements the certificate will allow — you probably just want to select Game Center and Fonts for now. At the end you can download your final .cer file. Double click on it to add it to your keychain access.

The final step is to open up Xcode and add your developer account to Xcode. Open Xcode and goo to Preferences > Accounts and add your Apple ID. Then click on Manage Certificates and all of the certificates you created through developer.apple.com.

Now Xcode can auto-magically setup code signing for you through CMake! We will do this with a special CMake command in the next section of this article.

### Using your Apple Developer ID

Simply make a folder called `build_ios` in your project's top-level directory, then run a special CMake command. Here's an example of both steps you can run in a terminal opened in the projects top-level directory:

```
mkdir build_ios
cmake -G "Xcode" DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphoneos -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=XXXXXXXXXX -Bbuild_ios .
```

Notice that `-DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=XXXXXXXXXX` needs to be filled out. You can find your Apple Developer ID at [developer.apple.com/account/resources/certificates/list](https://developer.apple.com/account/resources/certificates/list) in the top-right. Copy this value into the command replacing the `XXXXXXXXXX` placeholder value, and run it in your terminal.

CMake will print out all kinds of information to the terminal. Your project folder should now look something like this once CMake finishes:

```
/super_cool_game
    ├─ .gitignore
    ├─ build_ios/
    |  └─ lots of stuff!
    ├─ CMakeLists.txt
    ├─ README.md
    └─ src/
        └─ main.cpp
```

Inside the folder `build_macos` you can see a generated .xcodeproject file. Double-click it to open it. From here you can use Xcode to build your game. CMake should have auto-magically setup your Xcode project so it can fetch your certificate and sign your iOS app when building.

## Advanced CMake Commands

You can generate a build setup for either iOS or the iOS Simulator by changing the value for `CMAKE_OSX_SYSROOT` (to `iphoneos` or `iphonesimulator`) during the generate step. Here's are examples:

Generate a build setup for Xcode on iOS (non-simulator).

```
cmake -G "Xcode" DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphoneos -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=XXXXXXXXXX -Bbuild_ios .
```

Generate a build setup for Xcode on iOS Simulator (not actual devices).

```
cmake -G "Xcode" DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphonesimulator -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=XXXXXXXXXX -Bbuild_ios .
```

Similarly you can choose a CPU architecture to build for `CMAKE_OSX_ARCHITECTURES` with one of: `arm64` or `x86_64`. The `x86_64` value is for Apple devices with Intel CPUs, while `arm64` is for Apple devices with Arm CPUs (such as M1/M2 chips, or for iOS devices). Here's some examples:

Generate a build setup for Xcode on MacOS for Intel CPUs.

```
cmake -G "Xcode" -DCMAKE_OSX_ARCHITECTURES=x86_64 -Bbuild_macos .
```

Generate a build setup for Xcode on MacOS for Arm CPUs.

```
cmake -G "Xcode" -DCMAKE_OSX_ARCHITECTURES=arm64 -Bbuild_macos .
```

Generate a build setup for Xcode on iOS for Arm CPUs.

```
cmake -G "Xcode" -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphoneos -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=XXXXXXXXXX  -Bbuild_ios .
```

Generate a build setup for Xcode on iOS Simulator for Arm CPUs.

```
cmake -G "Xcode" -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphonesimulator -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=XXXXXXXXXX  -Bbuild_ios .
```

## Embedding Resources on iOS

TODO
