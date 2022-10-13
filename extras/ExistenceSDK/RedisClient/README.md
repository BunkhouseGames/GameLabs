# RedisClient

#### Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

*A packaging, configuration and basic Unreal Engine integration module for accessing Redis database.*

This plugin contains pre-built static libraries of HiRedis and some configuration settings for Unreal Engine projects.
See https://github.com/redis/hiredis/tree/master

Additionally it contains the Redis++ wrapper library.
https://github.com/sewenew/redis-plus-plus

## Usage

To use the library from your C++ module, add `RedisClient` as a module dependency in your
`module.Build.cs` file.  You can then `#inlcude <RedisPubSub.h>` from your source code.

### Redis++

If you use the Redis++ library you need to in-line the source code to your project as well. Place this somewhere in a .cpp file in your game project:

``` c++
#if WITH_EDITOR
#include <Redis++Library.inl>
#endif
```

To use the library, `#include <redis++/redis++.h>`

## Updating the source code and building static libraries:

### HiRedis Library
 
* Get the latest release as a zip file from here: https://github.com/redis/hiredis/releases
* Unzip the files into `Source/ThirdParty/hiredis`  (Do not include the root folder from the zip file)
* Create a `Source/Lib` folder
* CD into the Source/ThirdParty/hiredis folder
* Windows:
  * Delete any `CMakeCache.txt` file
  * Run `cmake CMakeLists.txt`
  * Run `MSBuild.exe hiredis_static.vcxproj /p:Configuration=Debug`
  * Run `MSBuild.exe hiredis_static.vcxproj /p:Configuration=Release`
  * Create the `..\Lib\Win64` folder
  * Run `copy Debug\hiredis_staticd.lib Release\hiredis_static.lib ..\Lib\Win64
* Lunix:
  * in WSL or similar, CD into the `hiredis` folder
  * Run `mkdir -p ../Lib/Linux;
  * Run `make clean; make && cp libhiredis.a ../Lib/Linux/`
  * Run `make clean; make OPTIMIZATION=-O0 && cp libhiredis.a ../Lib/Linux/libhiredisd.a`
  
#### Windows prerequisites
* **cmake**:  Using chocolatey, it can be installed using `choco install cmake --installargs "ADD_CMAKE_TO_PATH=System"`
* **msbuild**:  Installed as part of visual studio, one needs to set the correct build environment to run it.  run something akin to
  `"C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsMSBuildCmd.bat"` to set the proper environment
  to run MSBuild.exe (your install location or edition of visual studio may vary.)

#### Including symbols in the .lib on windows
The default `CMakeLists.txt` does not add the `/Z7` flag to the library.  To do that, edit the file, chaning the relevant
section to look something like

```
IF(WIN32)
SET_TARGET_PROPERTIES(hiredis_static
    PROPERTIES COMPILE_FLAGS /Z7)
ENDIF()
```

This ensures that debug information is added to the .lib file and not a separate .pdb file

#### Cross-compiling for LinuxArm64
Cross compilation for ARM is possible on linux.  This requires the `gcc-aarch64-linux-gnu` package to be installed. 
Note that on some distros, it cannot co-exist with the `gcc-multilib` package

The steps are the same as for Linux above, with the following changes:

 - Set the CC env var to `aarch64-linux-gnu-gcc` so that the correct toolchain is used
 - Copy the resulting archives to the `../Lib/LinuxArm64` folder

### Redis++ Library

The source code can be fetched as zip file from https://github.com/sewenew/redis-plus-plus/tags

Unzip `/src/sw/redis++` into `kamo/Source/ThirdParty/redis++`.

Additinally get the [RedLock recipes code branch](https://github.com/sewenew/redis-plus-plus/archive/recipes.zip) and extract the *recipes* folder into `redis++/recipes`.

