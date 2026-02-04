# CPP API Client for Argos data

This repository provides a C++ client to communicate with an
Argos sonar.

>[!NOTE]
> This is still under active development and testing. But may serve as an example for
> integration, beyond the lower level example that's given in [sdk repo](https://github.com/farsounder/SDK-Integration-Examples).

## Build

```sh
cmake -S . -B build
cmake --build build
```

## Run example client:

```sh
./build/farsounder_example # (./build/Debug|Release/farsounder_example.exe on windows)
```

## Include in a project
1. Build the repo (or add as a subdir etc and build)
2. Link against the farsounder target
3. Include headers in include/

CMAKE example: In your CMakeLists.txt, you can add this repo to fetch from - or clone it manually into /vendor for example. Something like:

```
FetchContent_Declare(
  farsounder
  GIT_REPOSITORY <url>
  GIT_TAG <tag/commit>
)
FetchContent_MakeAvailable(farsounder)

target_link_libraries(<your_app> PRIVATE farsounder)
```
Should get it fetched and built. When I tested that in standalone I still had to copy the the zmq lib next to the test exe, I'm sure the path could be updated instead. Same thing we're doing here in the last line of the `CMakeLists.txt` file.

## Example usage:
There is an example in [basic_client.cpp](examples/basic_client.cpp).

## Development

Autoformat with: 
```
clang-format -i src/*.cpp include/farsounder/*.hpp examples/*.cpp
```

So far we've built:

On Windows 11:
```
Visual Studio 17 2022, Windows SDK version 10.0.26100.0 to target Windows 10.0.26200.
The CXX compiler identification is MSVC 19.44.35209.0
```

On WSL2 Ubuntu 24.04 w/ gcc/g++
```
The CXX compiler identification is GNU 13.3.0
```

## TODO:
- create built lib/dll instead
- example server for testing without the SDK demo
- tag version
- create example to test async stuff
- docs?