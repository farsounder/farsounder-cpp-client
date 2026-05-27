# C++ API Client for Argos data

This repository provides a C++ client to communicate with an
Argos sonar.

>[!NOTE]
> This is still under active development and testing. But may serve as an example for
> integration, beyond the lower level example that's given in [sdk repo](https://github.com/farsounder/SDK-Integration-Examples). At the moment the docs are the headers and the example in examples.

## What is this?

This is a simple wrapper around the APIs that FarSounder exposes to access Argos sonar data. You can use the APIs directly (protobuf over zeromq or json over http) if you prefer. They are documented in the [sdk repo](https://github.com/farsounder/SDK-Integration-Examples).

The purpose of this project is to offer a simpler way to interface with the Argos sonar and start getting data. The dependencies are abstracted away and some basic threading is implemented to handle receiving data, etc. For streaming data sources, a callback can be registered that will either run in the context of the receive thread, or run on a thread pool (default). Simple get/set functions are exposed to make the request/reply endpoints easier to use ([example](examples/basic_client.cpp)).

This wrapper uses the zermq/protobuf api mostly, but uses the http/json api for history data as it's not currently exposed via the zmq api.

## Installing
There's a 64 bit Windows MSVC version and an Ubuntu 24 GCC version so far.

You can install by downloading the zip package from [releases](https://github.com/farsounder/farsounder-cpp-client/releases) and using the dll and headers in your project. 

A very minimal example of this using `CMake` to pull the package is avaiable [here](https://github.com/farsounder/SDK-Integration-Examples/tree/master/using_sdk/cpp/simple_example)

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

Ubuntu 24.04:
```
CMAKE_BUILD_TYPE:STRING=Release
CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/c++
CMAKE_CXX_COMPILER_AR:FILEPATH=/usr/bin/gcc-ar-13
CMAKE_CXX_COMPILER_RANLIB:FILEPATH=/usr/bin/gcc-ranlib-13
CMAKE_C_COMPILER:FILEPATH=/usr/bin/cc
CMAKE_C_COMPILER_AR:FILEPATH=/usr/bin/gcc-ar-13
CMAKE_C_COMPILER_RANLIB:FILEPATH=/usr/bin/gcc-ranlib-13
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
