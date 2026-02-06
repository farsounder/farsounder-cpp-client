# C++ API Client for Argos data

This repository provides a C++ client to communicate with an
Argos sonar.

>[!NOTE]
> This is still under active development and testing. But may serve as an example for
> integration, beyond the lower level example that's given in [sdk repo](https://github.com/farsounder/SDK-Integration-Examples). At the moment the docs are the headers and the example in examples.

## Installing
There's a 64 bit Windows MSVC version available so far.

You can install by downloading the zip package from [releases](https://github.com/farsounder/farsounder-cpp-client/releases) and using the dll and headers in your project. There's an example of how to do this in [examples/CMakeLists.txt](examples/CMakeLists.txt) with `CMake`. With the pre-built package, start from step 2 (don't need to build the sdk) and point CMAKE_PREFIX_PATH at where you extracted the `farsounder-sdk` package. That file is a bit complicated to support building the example in both cases, in reality a simpler make file is possible - something like:

``` cmake
cmake_minimum_required(VERSION 3.16)

project(farsounder_client_example LANGUAGES CXX)
# Point to where you unpacked the SDK or use cmdline
# argument -DFARSOUNDER_SDK_DIR=<path>
set(FARSOUNDER_SDK_DIR "vendor/farsounder-sdk")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_PREFIX_PATH ${FARSOUNDER_SDK_DIR})

find_package(farsounder REQUIRED)

add_executable(farsounder_client_example basic_client.cpp)
target_link_libraries(farsounder_client_example PRIVATE farsounder::farsounder)

if(WIN32)
    # Copy farsounder.dll next to the executable
    add_custom_command(TARGET farsounder_client_example POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:farsounder_client_example>
                $<TARGET_FILE_DIR:farsounder_client_example>
        COMMAND_EXPAND_LISTS
    )
endif()
```

If you need another configuration / version / platform etc, you will need to build the sdk from source.

## Build the SDK from source

For testing, build and link to the example client in [examples/basic_client.cpp](examples/basic_client.cpp):
``` sh
cmake -S . -B build
cmake --build build
```
On Windows - this will build a dll in `build/Debug/farsounder.dll` and an exe
in `build/examples/Debug/farsounder_example.exe`.

Build the SDK package (headers, binary, and cmake config) for redist:
``` sh
cmake -S . -B build
cmake --build build --config Release
cmake --install build --prefix C:/farsounder-sdk --config Release  # puts in C:/farsounder-sdk
```

Build the example, linking against the separately build dll instead of directly
against the target:

``` sh
cmake -S examples -B build-example -DCMAKE_PREFIX_PATH="C:/farsounder-sdk"
cmake --build build-example --config Release
```

```
build-example/Release/farsounder_example.exe
```

## Dependencies
This project uses the following dependencies, all of which are fetched and built automatically via CMake FetchContent in the top-level CMakeLists.txt:

- [protobuf](https://github.com/protocolbuffers/protobuf) v33.5 (parse/serialize messages)
- [abseil-cpp](https://github.com/abseil/abseil-cpp) --> pulled in by protobuf
- [zeromq/libzmq](https://github.com/zeromq/libzmq) v4.3.5 (sockets)
- [cppzmq](https://github.com/zeromq/cppzmq) v4.11.0 (cpp bindings)
- [cpr](https://github.com/libcpr/cpr) v1.11.0 (curl for http request)
- [nlohmann/json](https://github.com/nlohmann/json) v3.11.3 (parse json)

These are configured to be statically linked into the main DLL, so end users only need the built farsounder DLL and public headers for integration.

## Development

Autoformat with: 
```
clang-format -i src/*.cpp include/farsounder/*.hpp examples/*.cpp tests/*.cpp
```

So far we've built on

On Windows 11:
```
Visual Studio 17 2022, Windows SDK version 10.0.26100.0 to target Windows 10.0.26200.
The CXX compiler identification is MSVC 19.44.35209.0
```

### Tests
Configure and build with tests enabled:
``` sh
cmake -S . -B build -DFARSOUNDER_BUILD_TESTS=ON
cmake --build build
```

Run tests:
``` sh
ctest --test-dir build --output-on-failure
```

On Windows (multi-config generators), specify a config:
``` sh
ctest --test-dir build -C Debug --output-on-failure
```

## TODO:
- example server for testing without the SDK demo running
- tests --> make transport layer injectable for easier testing?
- create example to test/demo async stuff
- docs
