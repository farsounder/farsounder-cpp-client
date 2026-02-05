# C++ API Client for Argos data

This repository provides a C++ client to communicate with an
Argos sonar.

>[!NOTE]
> This is still under active development and testing. But may serve as an example for
> integration, beyond the lower level example that's given in [sdk repo](https://github.com/farsounder/SDK-Integration-Examples). At the moment the docs are the headers and the example in examples.

## Build the SDK

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

## Example usage:
There is an example in [basic_client.cpp](examples/basic_client.cpp).

## Development

Autoformat with: 
```
clang-format -i src/*.cpp include/farsounder/*.hpp examples/*.cpp
```

So far we've built on

On Windows 11:
```
Visual Studio 17 2022, Windows SDK version 10.0.26100.0 to target Windows 10.0.26200.
The CXX compiler identification is MSVC 19.44.35209.0
```

## TODO:
- example server for testing without the SDK demo running
- tests --> make transport layer injectable for easier testing?
  - message conversion (especially hydrophone)
- create example to test/demo async stuff
- docs
