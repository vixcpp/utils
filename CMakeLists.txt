cmake_minimum_required(VERSION 3.20)

#  @file CMakeLists.txt
#  @author Gaspard Kirira
#
#  Copyright 2025, Gaspard Kirira.  All rights reserved.
#  https://github.com/vixcpp/vix
#  Use of this source code is governed by a MIT license
#  that can be found in the License file.
#
# ====================================================================
# Vix.cpp - Utils Module
# ====================================================================
# Purpose:
#   Cross-cutting utilities used by other Vix modules and end-user apps:
#   logging, validation helpers, environment/time/UUID utilities, etc.
#
#   Builds as STATIC by default, or as a header-only INTERFACE target
#   when VIX_HEADER_ONLY=ON.
#
# Public Targets:
#   - vix_utils  : The actual library target (STATIC or INTERFACE)
#   - vix::utils : Namespaced alias for consumers
#
# Public Dependencies:
#   - spdlog
#   - fmt
#
# Installation/Export:
#   - Umbrella build exports into: VixTargets
#   - Standalone build exports into: vix_utilsTargets
# ====================================================================

project(vix_utils VERSION 1.5.2 LANGUAGES CXX)

include(GNUInstallDirs)
include(CheckCXXCompilerFlag)
include(CMakePackageConfigHelpers)
include(FindPkgConfig)

# --------------------------------------------------------------------
# Global settings
# --------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# --------------------------------------------------------------------
# Export set selection
# --------------------------------------------------------------------
if (DEFINED VIX_UMBRELLA_BUILD AND VIX_UMBRELLA_BUILD)
  set(VIX_UTILS_EXPORT_SET VixTargets)
else()
  set(VIX_UTILS_EXPORT_SET vix_utilsTargets)
endif()

# --------------------------------------------------------------------
# Options
# --------------------------------------------------------------------
option(VIX_STRICT               "Treat warnings as errors"                  OFF)
option(VIX_ENABLE_LTO           "Enable link-time optimization (Release)"   OFF)
option(VIX_HEADER_ONLY          "Build vix_utils as header-only INTERFACE"  OFF)
option(VIX_UTILS_BUILD_EXAMPLES "Build utils examples"                      OFF)

# --------------------------------------------------------------------
# Git / Build metadata
# --------------------------------------------------------------------
execute_process(
  COMMAND git rev-parse --short HEAD
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE VIX_GIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)

if (NOT VIX_GIT_HASH)
  set(VIX_GIT_HASH "unknown")
endif()

string(TIMESTAMP VIX_BUILD_DATE "%Y-%m-%d %H:%M:%S UTC" UTC)

# --------------------------------------------------------------------
# Dependency resolution: spdlog + fmt
# Resolution policy:
#   1) Reuse existing targets if already available
#   2) Try installed packages via find_package(... CONFIG QUIET)
#   3) Try pkg-config fallback
#   4) Fail with a clear message if nothing is found
# --------------------------------------------------------------------
set(_VIX_SPDLOG_TARGET "")
set(_VIX_FMT_TARGET "")

# -------------------------
# spdlog
# -------------------------
if (TARGET spdlog::spdlog_header_only)
  set(_VIX_SPDLOG_TARGET spdlog::spdlog_header_only)

elseif (TARGET spdlog::spdlog)
  set(_VIX_SPDLOG_TARGET spdlog::spdlog)

else()
  find_package(spdlog CONFIG QUIET)

  if (TARGET spdlog::spdlog_header_only)
    set(_VIX_SPDLOG_TARGET spdlog::spdlog_header_only)

  elseif (TARGET spdlog::spdlog)
    set(_VIX_SPDLOG_TARGET spdlog::spdlog)

  else()
    pkg_check_modules(PC_SPDLOG QUIET spdlog)

    if (PC_SPDLOG_FOUND)
      add_library(vix_spdlog_external INTERFACE)
      target_include_directories(vix_spdlog_external INTERFACE ${PC_SPDLOG_INCLUDE_DIRS})
      target_link_directories(vix_spdlog_external INTERFACE ${PC_SPDLOG_LIBRARY_DIRS})
      target_link_libraries(vix_spdlog_external INTERFACE ${PC_SPDLOG_LIBRARIES})
      set(_VIX_SPDLOG_TARGET vix_spdlog_external)
    endif()
  endif()
endif()

# -------------------------
# fmt
# -------------------------
if (TARGET fmt::fmt-header-only)
  set(_VIX_FMT_TARGET fmt::fmt-header-only)

elseif (TARGET fmt::fmt)
  set(_VIX_FMT_TARGET fmt::fmt)

else()
  find_package(fmt CONFIG QUIET)

  if (TARGET fmt::fmt-header-only)
    set(_VIX_FMT_TARGET fmt::fmt-header-only)

  elseif (TARGET fmt::fmt)
    set(_VIX_FMT_TARGET fmt::fmt)

  else()
    pkg_check_modules(PC_FMT QUIET fmt)

    if (PC_FMT_FOUND)
      add_library(vix_fmt_external INTERFACE)
      target_include_directories(vix_fmt_external INTERFACE ${PC_FMT_INCLUDE_DIRS})
      target_link_directories(vix_fmt_external INTERFACE ${PC_FMT_LIBRARY_DIRS})
      target_link_libraries(vix_fmt_external INTERFACE ${PC_FMT_LIBRARIES})
      set(_VIX_FMT_TARGET vix_fmt_external)
    endif()
  endif()
endif()

if (NOT _VIX_SPDLOG_TARGET)
  message(FATAL_ERROR
    "[utils] spdlog not found.\n"
    "Expected one of:\n"
    "  - existing target: spdlog::spdlog_header_only or spdlog::spdlog\n"
    "  - installed package resolvable via find_package(spdlog CONFIG)\n"
    "  - pkg-config package: spdlog\n"
    "Make sure spdlog is installed and visible through CMAKE_PREFIX_PATH, spdlog_DIR, or pkg-config."
  )
endif()

if (NOT _VIX_FMT_TARGET)
  message(FATAL_ERROR
    "[utils] fmt not found.\n"
    "Expected one of:\n"
    "  - existing target: fmt::fmt-header-only or fmt::fmt\n"
    "  - installed package resolvable via find_package(fmt CONFIG)\n"
    "  - pkg-config package: fmt\n"
    "Make sure fmt is installed and visible through CMAKE_PREFIX_PATH, fmt_DIR, or pkg-config."
  )
endif()

# --------------------------------------------------------------------
# Library target
# --------------------------------------------------------------------
if (VIX_HEADER_ONLY)
  message(STATUS "[utils] Building HEADER-ONLY library.")

  add_library(vix_utils INTERFACE)
  add_library(vix::utils ALIAS vix_utils)

  target_include_directories(vix_utils INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  target_compile_definitions(vix_utils INTERFACE
    VIX_GIT_HASH="${VIX_GIT_HASH}"
    VIX_BUILD_DATE="${VIX_BUILD_DATE}"
    SPDLOG_FMT_EXTERNAL=1
  )

  target_compile_features(vix_utils INTERFACE cxx_std_20)

  target_link_libraries(vix_utils INTERFACE
    ${_VIX_SPDLOG_TARGET}
    ${_VIX_FMT_TARGET}
  )

  if (WIN32)
    target_compile_definitions(vix_utils INTERFACE SPDLOG_NO_WIN32_API=1)
  endif()

else()
  file(GLOB_RECURSE UTILS_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
  )

  if (NOT UTILS_SOURCES)
    message(FATAL_ERROR
      "No sources found under src/. If you want header-only, set VIX_HEADER_ONLY=ON"
    )
  endif()

  message(STATUS "[utils] Building STATIC library with detected sources.")

  add_library(vix_utils STATIC ${UTILS_SOURCES})
  add_library(vix::utils ALIAS vix_utils)

  target_include_directories(vix_utils
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  target_compile_definitions(vix_utils PUBLIC
    VIX_GIT_HASH="${VIX_GIT_HASH}"
    VIX_BUILD_DATE="${VIX_BUILD_DATE}"
    SPDLOG_FMT_EXTERNAL=1
  )

  target_compile_features(vix_utils PUBLIC cxx_std_20)

  target_link_libraries(vix_utils
    PUBLIC
      ${_VIX_SPDLOG_TARGET}
      ${_VIX_FMT_TARGET}
  )

  if (WIN32)
    target_compile_definitions(vix_utils PRIVATE SPDLOG_NO_WIN32_API=1)
  endif()
endif()

# --------------------------------------------------------------------
# Compiler warnings
# Build-only helpers must not leak into exported interfaces.
# --------------------------------------------------------------------
if (TARGET vix_warnings)
  if (NOT VIX_HEADER_ONLY)
    target_link_libraries(vix_utils PRIVATE
      $<BUILD_INTERFACE:vix_warnings>
    )
  endif()
else()
  if (VIX_HEADER_ONLY)
    if (NOT MSVC)
      target_compile_options(vix_utils INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Wno-array-bounds
        -Wno-stringop-overflow
      )
    endif()

    if (VIX_STRICT AND NOT MSVC)
      target_compile_options(vix_utils INTERFACE -Werror)
    endif()
  else()
    if (NOT MSVC)
      target_compile_options(vix_utils PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wno-array-bounds
        -Wno-stringop-overflow
      )
    endif()

    if (VIX_STRICT AND NOT MSVC)
      target_compile_options(vix_utils PRIVATE -Werror)
    endif()
  endif()
endif()

if (VIX_ENABLE_SANITIZERS AND TARGET vix_sanitizers)
  if (NOT VIX_HEADER_ONLY)
    target_link_libraries(vix_utils PRIVATE
      $<BUILD_INTERFACE:vix_sanitizers>
    )
  endif()
endif()

# --------------------------------------------------------------------
# LTO (Release)
# --------------------------------------------------------------------
if (VIX_ENABLE_LTO AND NOT VIX_HEADER_ONLY AND CMAKE_BUILD_TYPE STREQUAL "Release")
  include(CheckIPOSupported)
  check_ipo_supported(RESULT ipo_ok OUTPUT ipo_msg)

  if (ipo_ok)
    set_property(TARGET vix_utils PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  else()
    message(WARNING "IPO/LTO not supported: ${ipo_msg}")
  endif()
endif()

# --------------------------------------------------------------------
# Install / Export
# --------------------------------------------------------------------
set_target_properties(vix_utils PROPERTIES
  EXPORT_NAME utils
  OUTPUT_NAME vix_utils
  VERSION ${PROJECT_VERSION}
  SOVERSION 0
)

if (NOT VIX_HEADER_ONLY)
  install(TARGETS vix_utils
    EXPORT ${VIX_UTILS_EXPORT_SET}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
else()
  install(TARGETS vix_utils
    EXPORT ${VIX_UTILS_EXPORT_SET}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
endif()

# Standalone package files only outside umbrella
if (NOT (DEFINED VIX_UMBRELLA_BUILD AND VIX_UMBRELLA_BUILD))

  install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
      PATTERN "*.hpp"
      PATTERN "*.h"
  )

  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/vix_utilsConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/vix_utilsConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/vix_utils"
  )

  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/vix_utilsConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
  )

  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/vix_utilsConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/vix_utilsConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/vix_utils"
  )

  install(EXPORT vix_utilsTargets
    FILE vix_utilsTargets.cmake
    NAMESPACE vix::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/vix_utils"
  )

endif()

# --------------------------------------------------------------------
# Examples (opt-in)
# --------------------------------------------------------------------
if (VIX_UTILS_BUILD_EXAMPLES AND NOT VIX_HEADER_ONLY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

  add_executable(utils_log_demo examples/log_demo.cpp)
  target_link_libraries(utils_log_demo PRIVATE vix::utils)

  add_executable(utils_validation_demo examples/validation_demo.cpp)
  target_link_libraries(utils_validation_demo PRIVATE vix::utils)

  add_executable(utils_env_time_uuid examples/env_time_uuid.cpp)
  target_link_libraries(utils_env_time_uuid PRIVATE vix::utils)

  add_custom_target(utils_examples ALL
    DEPENDS
      utils_log_demo
      utils_validation_demo
      utils_env_time_uuid
  )
endif()

# --------------------------------------------------------------------
# Summary
# --------------------------------------------------------------------
if (DEFINED UTILS_SOURCES AND NOT VIX_HEADER_ONLY)
  message(STATUS "Utils library sources: ${UTILS_SOURCES}")
else()
  message(STATUS "Utils header-only mode or no sources listed")
endif()

message(STATUS "Utils export set: ${VIX_UTILS_EXPORT_SET}")
message(STATUS "Utils spdlog target: ${_VIX_SPDLOG_TARGET}")
message(STATUS "Utils fmt target: ${_VIX_FMT_TARGET}")
