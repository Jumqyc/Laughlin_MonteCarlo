cmake_minimum_required(VERSION 3.14)
project(Laughlin LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose build type: Debug, Release, RelWithDebInfo" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(Laughlin_ENABLE_NATIVE_OPT "Enable native CPU optimization (-march=native)" ON)
option(Laughlin_ENABLE_LTO "Enable Link Time Optimization" ON)
option(Laughlin_ENABLE_AVX2 "Enable AVX2 vectorization" ON)
option(Laughlin_ENABLE_FAST_MATH "Enable fast math optimizations" ON)

include(CheckCXXCompilerFlag)

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

add_compile_options(
    "$<$<CONFIG:Release>:-fomit-frame-pointer>"
    "$<$<CONFIG:Release>:-funroll-loops>"
    "$<$<CONFIG:Release>:-floop-optimize>"
    "$<$<CONFIG:Release>:-fstrict-aliasing>"
    "$<$<CONFIG:Release>:-fno-semantic-interposition>"
)

if(Laughlin_ENABLE_AVX2)
    add_compile_options(
        "$<$<CONFIG:Release>:-mavx2>"
        "$<$<CONFIG:Release>:-mfma>"
        "$<$<CONFIG:Release>:-ftree-vectorize>"
    )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DENABLE_VECTORIZATION=ON")
endif()
if(Laughlin_ENABLE_NATIVE_OPT)
    add_compile_options("$<$<CONFIG:Release>:-march=native>")
    message(STATUS "Enabled native CPU optimization (-march=native)")
else()
    add_compile_options("$<$<CONFIG:Release>:-march=x86-64-v2>")
endif()

if(Laughlin_ENABLE_FAST_MATH)
    add_compile_options(
        "$<$<CONFIG:Release>:-ffast-math>"
        "$<$<CONFIG:Release>:-fno-math-errno>"
    )
    message(STATUS "Enabled fast math optimizations")
endif()

if(Laughlin_ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_OUTPUT)
    if(IPO_SUPPORTED)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
        message(STATUS "Enabled Link Time Optimization (LTO)")
    else()
        message(WARNING "LTO not supported: ${IPO_OUTPUT}")
    endif()
endif()


if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(
        "$<$<CONFIG:Release>:-fno-stack-protector>"
        "$<$<CONFIG:Release>:-fno-plt>"
    )
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -Wl,-O3 -Wl,--as-needed -Wl,--strip-all")
    
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(
        "$<$<CONFIG:Release>:-fvectorize>"
        "$<$<CONFIG:Release>:-fslp-vectorize>"
    )
    if(Laughlin_ENABLE_AVX2)
        add_compile_options("$<$<CONFIG:Release>:-mllvm -vectorize-loops>")
    endif()
    
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(
        "$<$<CONFIG:Release>:/O2 /Ob2 /GL /fp:fast /arch:AVX2>"
    )
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
endif()

include(FetchContent)
FetchContent_Declare(
    pybind11
    GIT_REPOSITORY  https://github.com/pybind/pybind11.git
    GIT_TAG         v2.13.6
    GIT_SHALLOW     TRUE
)
FetchContent_MakeAvailable(pybind11)

pybind11_add_module(Laughlin
    bind.cpp
    Laughlin.hpp
    Laughlin.cpp
)

install(TARGETS Laughlin DESTINATION .)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Native optimization: ${Laughlin_ENABLE_NATIVE_OPT}")
message(STATUS "LTO: ${Laughlin_ENABLE_LTO}")
message(STATUS "AVX2: ${Laughlin_ENABLE_AVX2}")
message(STATUS "Fast math: ${Laughlin_ENABLE_FAST_MATH}")
