# FarSounder C++ SDK

This repository provides a C++ client to communicate with an
Argos sonar.

>![NOTE]
> This is still under active development and testing. But may serve as an example for
> integration, beyond the lower level example in [sdk repo]().

## Build

```sh
cmake -S . -B build
cmake --build build
```

## Run example client

```sh
./build/farsounder_example # (./build/Debug|Release/farsounder_example.exe on windows)
```

## Run a dummy server
//TODO

## Include in a project
1. Build the repo (or add as a subdir etc and build)
2. Link against the farsounder target
3. Include headers in include/

In cmake (NOT TESTED YET):

```
FetchContent_Declare(
  farsounder
  GIT_REPOSITORY <url>
  GIT_TAG <tag/commit>
)
FetchContent_MakeAvailable(farsounder)

target_link_libraries(my_app PRIVATE farsounder)
```



## Development

Autoformat with: 
```
clang-format -i src/*.cpp include/farsounder/*.hpp examples/*.cpp
```




