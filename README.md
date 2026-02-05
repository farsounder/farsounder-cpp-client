# CPP API Client for Argos data

This repository provides a C++ client to communicate with an
Argos sonar.

>[!NOTE]
> This is still under active development and testing. But may serve as an example for
> integration, beyond the lower level example that's given in [sdk repo](https://github.com/farsounder/SDK-Integration-Examples).

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
- example server for testing without the SDK demo
- tag version
- create example to test async stuff
- docs
