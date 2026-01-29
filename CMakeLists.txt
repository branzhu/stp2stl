cmake_minimum_required(VERSION 3.23)

# Keep MSVC runtime selection consistent across subprojects.
if (POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW)
endif()

project(stp2stl LANGUAGES C CXX)

# MSVC defaults to the active system code page when reading sources/headers without BOM.
# This project contains UTF-8 Chinese comments; enforce UTF-8 to avoid C4819 and mis-parsing.
if (MSVC)
  add_compile_options(/utf-8)
endif()

# Superbuild:
# - ON  : build OCCT from submodule via ExternalProject, then build this project against that install tree
# - OFF : use an existing OCCT install (via CMAKE_PREFIX_PATH)
option(STP2STL_SUPERBUILD "Build with Superbuild (build OCCT first, then this project)" ON)

# Configuration name used by the Superbuild for both OCCT and this project.
# - Multi-config generators (VS/Xcode): mapped to CMAKE_CONFIGURATION_TYPES + --config
# - Single-config generators (Ninja/Makefiles): mapped to CMAKE_BUILD_TYPE
set(STP2STL_SUPERBUILD_CONFIG "Release" CACHE STRING "Superbuild build type / configuration name (e.g. Release, Debug)")

if (STP2STL_SUPERBUILD)
  include(ExternalProject)

  # OCCT source is expected as a Git submodule.
  set(occt_source_dir "${CMAKE_SOURCE_DIR}/third_party/occt")
  if (NOT EXISTS "${occt_source_dir}/CMakeLists.txt")
    message(FATAL_ERROR
      "OCCT submodule not found: ${occt_source_dir}\n"
      "Please initialize submodules first:\n"
      "  git submodule update --init --recursive\n"
      "And ensure it is OCCT 7.9.3.")
  endif()

  # Hard check the OCCT submodule version to avoid subtle ABI/API mismatches.
  set(occt_version_header "${occt_source_dir}/src/Standard/Standard_Version.hxx")
  if (EXISTS "${occt_version_header}")
    file(READ "${occt_version_header}" _occt_ver_h)
    string(REGEX MATCH "#define[ \t]+OCC_VERSION_MAJOR[ \t]+([0-9]+)" _m_major "${_occt_ver_h}")
    set(_occt_major "${CMAKE_MATCH_1}")
    string(REGEX MATCH "#define[ \t]+OCC_VERSION_MINOR[ \t]+([0-9]+)" _m_minor "${_occt_ver_h}")
    set(_occt_minor "${CMAKE_MATCH_1}")
    string(REGEX MATCH "#define[ \t]+OCC_VERSION_MAINTENANCE[ \t]+([0-9]+)" _m_patch "${_occt_ver_h}")
    set(_occt_patch "${CMAKE_MATCH_1}")
    if (NOT (_occt_major STREQUAL "7" AND _occt_minor STREQUAL "9" AND _occt_patch STREQUAL "3"))
      message(FATAL_ERROR
        "OCCT submodule version mismatch: expected 7.9.3 but detected ${_occt_major}.${_occt_minor}.${_occt_patch}.\n"
        "Please run in third_party/occt:\n"
        "  git checkout V7_9_3\n"
        "Then delete the build directory and rebuild.")
    endif()
  endif()

  # Install tree used by the inner build to find OCCT (find_package(OpenCASCADE)).
  set(occt_install_dir "${CMAKE_BINARY_DIR}/_deps/occt-install")
  set(occt_build_dir   "${CMAKE_BINARY_DIR}/_deps/occt-build")

  # Superbuild configuration mapping for OCCT.
  if (CMAKE_CONFIGURATION_TYPES)
    set(_stp2stl_occt_cmake_config_args "-DCMAKE_CONFIGURATION_TYPES=${STP2STL_SUPERBUILD_CONFIG}")
    set(_stp2stl_occt_build_config_args --config "${STP2STL_SUPERBUILD_CONFIG}")
  else()
    set(_stp2stl_occt_cmake_config_args "-DCMAKE_BUILD_TYPE=${STP2STL_SUPERBUILD_CONFIG}")
    set(_stp2stl_occt_build_config_args)
  endif()

  # OCCT build configuration:
  # - build static libs only
  # - disable optional third-party dependencies (keep runtime footprint minimal)
  # - enable PIC so the static OCCT libs can be linked into our shared library on ELF/Mach-O platforms
  set(occt_cmake_args
    "-DCMAKE_INSTALL_PREFIX=${occt_install_dir}"
    "-DCMAKE_POLICY_DEFAULT_CMP0091=NEW"
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
    "-DBUILD_LIBRARY_TYPE=Static"
    "-DBUILD_SHARED_LIBS=OFF"
    ${_stp2stl_occt_cmake_config_args}
    "-DBUILD_DOC_Overview=OFF"
    "-DBUILD_SAMPLES_QT=OFF"
    "-DBUILD_TESTING=OFF"
    "-DUSE_TBB=OFF"
    "-DUSE_OPENGL=OFF"
    "-DUSE_FREETYPE=OFF"
    "-DUSE_TCL=OFF"
    "-DUSE_VTK=OFF"
    "-DUSE_RAPIDJSON=OFF"
    "-DUSE_DRACO=OFF"
    "-DUSE_FREEIMAGE=OFF"
    "-DUSE_TK=OFF"
    "-DUSE_QT=OFF"
    "-DUSE_XLIB=OFF"
    "-DBUILD_MODULE_Draw=OFF"
    "-DBUILD_MODULE_Visualization=ON"
    "-DBUILD_MODULE_Samples=OFF"
    "-DBUILD_MODULE_FoundationClasses=ON"
    "-DBUILD_MODULE_ModelingData=ON"
    "-DBUILD_MODULE_ModelingAlgorithms=ON"
    "-DBUILD_MODULE_ApplicationFramework=ON"
    "-DBUILD_MODULE_DataExchange=ON"
    "-DBUILD_MODULE_DETools=ON"
  )
  if (MSVC)
    # Force /MT for OCCT when building with MSVC.
    list(APPEND occt_cmake_args "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded")
  endif()

  # Propagate the same generator/platform/toolset into ExternalProject builds.
  set(ep_generator_args
    "-G" "${CMAKE_GENERATOR}"
  )
  if (CMAKE_GENERATOR_PLATFORM)
    list(APPEND ep_generator_args "-A" "${CMAKE_GENERATOR_PLATFORM}")
  endif()
  if (CMAKE_GENERATOR_TOOLSET)
    list(APPEND ep_generator_args "-T" "${CMAKE_GENERATOR_TOOLSET}")
  endif()

  # Build and install OCCT into the local install tree.
  ExternalProject_Add(occt
    SOURCE_DIR  "${occt_source_dir}"
    BINARY_DIR  "${occt_build_dir}"
    INSTALL_DIR "${occt_install_dir}"
    CMAKE_ARGS
      ${ep_generator_args}
      ${occt_cmake_args}
    BUILD_COMMAND "${CMAKE_COMMAND}" --build <BINARY_DIR> ${_stp2stl_occt_build_config_args}
    INSTALL_COMMAND "${CMAKE_COMMAND}" --build <BINARY_DIR> ${_stp2stl_occt_build_config_args} --target install
  )

  # Superbuild configuration mapping for the inner build.
  if (CMAKE_CONFIGURATION_TYPES)
    set(_stp2stl_inner_cmake_config_args "-DCMAKE_CONFIGURATION_TYPES=${STP2STL_SUPERBUILD_CONFIG}")
    set(_stp2stl_inner_build_config_args --config "${STP2STL_SUPERBUILD_CONFIG}")
  else()
    set(_stp2stl_inner_cmake_config_args "-DCMAKE_BUILD_TYPE=${STP2STL_SUPERBUILD_CONFIG}")
    set(_stp2stl_inner_build_config_args)
  endif()

  # Inner build: build this project against the OCCT install tree created above.
  ExternalProject_Add(stp2stl_inner
    SOURCE_DIR "${CMAKE_SOURCE_DIR}"
    BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/stp2stl-build"
    DEPENDS occt
    CMAKE_ARGS
      ${ep_generator_args}
      "-DSTP2STL_SUPERBUILD=OFF"
      "-DCMAKE_PREFIX_PATH=${occt_install_dir}"
      "-DCMAKE_POLICY_DEFAULT_CMP0091=NEW"
      ${_stp2stl_inner_cmake_config_args}
    BUILD_COMMAND "${CMAKE_COMMAND}" --build <BINARY_DIR> ${_stp2stl_inner_build_config_args}
    INSTALL_COMMAND ""
  )

  # Stop processing here in Superbuild mode: targets are produced by ExternalProject builds.
  return()
endif()

# ---- Non-superbuild (direct build) ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (MSVC)
  # Force /MT and ensure UTF-8 source encoding for MSVC.
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
  #add_compile_options(/utf-8)
endif()

# Components needed for STEP import and STL export.
set(stp2stl_occt_components
  FoundationClasses
  ModelingData
  ModelingAlgorithms
  Visualization
  ApplicationFramework
  DataExchange
  DETools
)

find_package(OpenCASCADE REQUIRED COMPONENTS ${stp2stl_occt_components})

# Ensure the detected OpenCASCADE matches the expected version (7.9.3).
if (NOT (OpenCASCADE_MAJOR_VERSION STREQUAL "7"
     AND OpenCASCADE_MINOR_VERSION STREQUAL "9"
     AND OpenCASCADE_MAINTENANCE_VERSION STREQUAL "3"))
  message(FATAL_ERROR
    "OpenCASCADE version mismatch: expected 7.9.3 but found "
    "${OpenCASCADE_MAJOR_VERSION}.${OpenCASCADE_MINOR_VERSION}.${OpenCASCADE_MAINTENANCE_VERSION}.\n"
    "Please ensure third_party/occt is checked out to V7_9_3, then clean build and retry.")
endif()

# Shared library: conversion implementation.
add_library(stp2stl SHARED
  src/stp2stl.cpp
  include/stp2stl/stp2stl.h
)

target_compile_definitions(stp2stl PRIVATE STP2STL_BUILD_DLL=1)

target_include_directories(stp2stl
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>"
)

# OpenCASCADE include directories can be exposed via different variables depending on the package config.
if (DEFINED OpenCASCADE_INCLUDE_DIR AND OpenCASCADE_INCLUDE_DIR)
  target_include_directories(stp2stl PRIVATE "${OpenCASCADE_INCLUDE_DIR}")
elseif (DEFINED OpenCASCADE_INCLUDE_DIRS)
  target_include_directories(stp2stl PRIVATE ${OpenCASCADE_INCLUDE_DIRS})
endif()

# Link the minimal OCCT libraries needed for STEP import and STL export.
set(stp2stl_occt_libs
  TKernel
  TKMath
  TKG2d
  TKG3d
  TKGeomBase
  TKBRep
  TKGeomAlgo
  TKTopAlgo
  TKMesh
  TKXSBase
  TKDE
  TKDESTEP
  TKDESTL
  TKRWMesh
)

target_link_libraries(stp2stl PRIVATE ${stp2stl_occt_libs})

target_compile_definitions(stp2stl PRIVATE UNICODE _UNICODE)

# CLI tool.
add_executable(stp2stl_cli
  src/stp2stl_cli.cpp
)

target_link_libraries(stp2stl_cli PRIVATE stp2stl)
target_compile_definitions(stp2stl_cli PRIVATE UNICODE _UNICODE)

# Keep outputs in a simple bin/lib layout under the build directory.
set_target_properties(stp2stl PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

set_target_properties(stp2stl_cli PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  OUTPUT_NAME "stp2stl"
)
