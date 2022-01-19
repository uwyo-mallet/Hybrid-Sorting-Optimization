# Using libc++ instead of libstdc++

It is useful to be able to benchmark not only gcc's `libstdc++` (also known as
`glibc++`), but also LLVM's implementation of the standard C++ library, `libc++`.
The unfortunate consequence of switching standard libraries is having to
recompile any and all libraries built with `libstdc++`.

This document aims to explain the compilation process of the two major libraries
utilized, `boost` and `google benchmark`, and how to build `QST` and `GB_QST`.

## Assumptions

If you are following this guide, the following assumptions are made about your
environment.

- You are using Linux.
- The default standard library on your distribution is `libstdc++`.
- `clang` and `libc++` (and corresponding development headers) are installed.
- You have gotten either just `QST` or both `QST` and `GB_QST` to run
  successfully at least once.

Using `libc++` with `gcc` takes considerably more effort than `clang`, so we
will not cover it here.

## General Setup

To make this a bit easier to follow, throughout this guide we will have the
following shell variables set:

```sh
CC=clang                                     # Clang C compiler
CXX=clang++                                  # Clang C++ compiler
QST_DIR=~/quicksort-tuning                   # Location of wherever this repository is cloned.
LIBCPP_PREFIX_PATH="${QST_DIR}/libcpp_libs"  # Place to install newly compiled libraries.
```

Ensure the destination for newly compiled libraries, `LIBCPP_PREFIX_PATH`, exists.

```sh
$ mkdir "$LIBCPP_PREFIX_PATH"
```

## Compiling and Installing Dependencies

In order to avoid conflicting libraries with those distributed from package
managers, or the distribution itself, all our builds will be installed to their
own dedicated installation directory within the `quicksort-tuning` repository.

Cmake doesn't have any built-in way to specify a different standard library,
nor a way to pick sets of compiled libraries corresponding to those standard
libraries. So instead, we simply tell cmake to only look for libraries in our
dedicated prefix path. We take care of ensuring all the dependencies are built
correctly, then cmake doesn't have any issue building `QST` or `GB_QST`.

### Boost

Boost requires some configuration at build time to support `libc++`.

1. Download the latest version of boost from [here](https://www.boost.org/users/download/),
   extract it, and `cd` into the resulting directory.

2. Build `b2` with the `clang` toolset using the provided bootstrap script.

   ```sh
   $ ./bootstrap.sh --prefix="$LIB_CPP_PREFIX_PATH" --with-toolset=clang
   ```

3. Build boost with `b2`, ensuring to target `libc++`.

   ```sh
   $ ./b2 toolset=clang cxxflags="-stdlib=libc++" stage=release --prefix="$LIB_CPP_PREFIX_PATH" --build-type=complete --layout=versioned
   ```

4. Ensure no errors occur during the build, then install to our prefix path,
   which we set earlier.

   ```sh
   $ ./b2 install toolset=clang cxxflags="-stdlib=libc++" stage=release --prefix="$LIB_CPP_PREFIX_PATH" --build-type=complete --layout=versioned
   ```

### Google Benchmark

Google benchmark, like boost, requires some special configuration. If you are
not using the `GB_QST` executable these steps can be skipped.

1. Clone and `cd` into the Google benchmark repository.

   ```sh
   $ git clone https://github.com/google/benchmark.git
   $ cd benchmark
   ```

2. Download and build required dependencies.

   ```sh
   $ mkdir build
   $ cd build
   $ cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DBENCHMARK_DOWNLOAD_DEPENDENCIES=on -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS=-stdlib=libc++ -DCMAKE_INSTALL_PREFIX="$LIB_CPP_PREFIX_PATH" ..
   ```

3. Build and Install

   ```sh
   $ cd ..
   $ cmake --build ./build --config Release
   $ cmake --build ./build --config Release --install
   ```

## Compiling QST and GB_QST

1. Navigate back to the `quicksort-tuning` repository.

2. If you have built previously using `libstdc++`, ensure you delete the contents
   of the `build/` directory, otherwise stale cmake configurations will cause
   builds to fail.

3. Configure the build.

   The included `CMakeLists.txt` file uses the `-DUSE_LIBCPP` definition to
   decide to link against `libc++`.

   ```sh
   $ cd "$QST_DIR"
   $ rm -rf ./build/*
   $ cd build/

   $ cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -DUSE_LIBCPP=ON -DCMAKE_PREFIX_PATH="$LIB_CPP_PREFIX_PATH"
   ```

   You should receive a warning about `libc++`, and the configuration should succeed.

   ```txt
    -- The C compiler identification is Clang 13.0.0
    -- The CXX compiler identification is Clang 13.0.0
    -- The ASM compiler identification is Clang with GNU-like command-line
    -- Found assembler: /usr/bin/clang
    -- Detecting C compiler ABI info
    -- Detecting C compiler ABI info - done
    -- Check for working C compiler: /usr/bin/clang - skipped
    -- Detecting C compile features
    -- Detecting C compile features - done
    -- Detecting CXX compiler ABI info
    -- Detecting CXX compiler ABI info - done
    -- Check for working CXX compiler: /usr/bin/clang++ - skipped
    -- Detecting CXX compile features
    -- Detecting CXX compile features - done
    CMake Warning at CMakeLists.txt:23 (message):
    Using libc++ instead of libstdc++.

    Only clang is currently supported.

    Ensure you set CMAKE_PREFIX_PATH correctly!

    CMAKE_PREFIX_PATH: ~/workRepos/quicksort-tuning/libcpp_libs


    C Compiler Path: /usr/bin/clang
    C Compiler ID: Clang
    CXX Compiler Path: /usr/bin/clang++
    CXX Compiler ID: Clang
    -- Found Boost: /home/joshua/workRepos/quicksort-tuning/libcpp_libs/lib/cmake/Boost-1.78.0/BoostConfig.cmake (found suitable version "1.78.0", minimum required is "1.67.0") found components: iostreams filesystem timer
    -- Looking for pthread.h
    -- Looking for pthread.h - found
    -- Performing Test CMAKE_HAVE_LIBC_PTHREAD
    -- Performing Test CMAKE_HAVE_LIBC_PTHREAD - Success
    -- Found Threads: TRUE
    Build type: Debug
    -- Found Doxygen: /usr/bin/doxygen (found version "1.9.1") found components: doxygen dot
    -- Configuring done
    -- Generating done
    -- Build files have been written to: /home/joshua/workRepos/quicksort-tuning/build
   ```

4. Build.

   ```
   $ make
   ```

   > If Google Benchmark is not installed, an error will be thrown, but `QST`
   > will still be built, as long as boost is successfully found.

At this point, binaries for `QST` and (if Google benchmark is installed)
`GB_QST` should exist in the build directory.

```sh
$ ./QST -V
2.0.3
	C COMPILER: Clang 13.0.0
	CXX COMPILER: Clang 13.0.0
	Type: Debug
	ASM Methods: [+]
	Using GLIBCXX: [-]

$ ./GB_QST -V
1.0.4
	C COMPILER: Clang 13.0.0
	CXX COMPILER: Clang 13.0.0
	Type: Debug
	ASM Methods: [+]
	Using GLIBCXX: [-]
```

As you can see, the `Using GLIBCXX` field has a `-`, indicating the binary
was successfully built using `libc++`.
