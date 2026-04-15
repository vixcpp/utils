cmake_minimum_required(VERSION 3.20)

#  @file CMakeLists.cpp
#  @author Gaspard Kirira
#
#  Copyright 2025, Gaspard Kirira.  All rights reserved.
#  https://github.com/vixcpp/vix
#  Use of this source code is governed by a MIT license
#  that can be found in the License file.
#
#  Vix.cpp
#
# UTILS MODULES
# Purpose:
#   Cross-cutting utilities used by other Vix modules and end-user apps:
#   logging, validation helpers, environment/time/UUID utilities, etc.
#
# Targets:
#   - vix_utils   : Real library target (STATIC by default, or header-only INTERFACE)
#   - vix::utils  : Namespaced alias for consumers
#
# Integration & Install:
#   - Public headers live under modules/utils/include/...
#   - This module depends on spdlog for logging.
#   - The target joins the umbrella export-set `VixTargets` so that
#     a system-wide installation of Vix exposes vix::utils via:
#       find_package(Vix CONFIG REQUIRED)
#       target_link_libraries(app PRIVATE vix::utils)
#
# Notes:
#   - Sanitizers are inherited from the umbrella if VIX_ENABLE_SANITIZERS=ON.
#   - Header-only mode is supported via VIX_HEADER_ONLY=ON.
# ====================================================================

project(vix_utils VERSION 1.5.2 LANGUAGES CXX)

include(GNUInstallDirs)
include(CheckCXXCompilerFlag)
include(CMakePackageConfigHelpers)

# Options
option(VIX_STRICT               "Treat warnings as errors"                  OFF)
option(VIX_ENABLE_LTO           "Enable link-time optimization (Release)"   OFF)
option(VIX_HEADER_ONLY          "Build vix_utils as header-only INTERFACE"  OFF)
option(VIX_UTILS_BUILD_EXAMPLES "Build utils examples"                      OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Git / Build metadata
execute_process(
  COMMAND git rev-parse --short HEAD
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE VIX_GIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)
if(NOT VIX_GIT_HASH)
  set(VIX_GIT_HASH "unknown")
endif()

string(TIMESTAMP VIX_BUILD_DATE "%Y-%m-%d %H:%M:%S UTC" UTC)

# Sources
if(VIX_HEADER_ONLY)
  add_library(vix_utils INTERFACE)
  add_library(vix::utils ALIAS vix_utils)

  target_include_directories(vix_utils INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )
  target_compile_definitions(vix_utils INTERFACE
    VIX_GIT_HASH="${VIX_GIT_HASH}"
    VIX_BUILD_DATE="${VIX_BUILD_DATE}"
  )
else()
  file(GLOB_RECURSE UTILS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
  if(NOT UTILS_SOURCES)
    message(FATAL_ERROR "No sources found under src/. If you want header-only, set VIX_HEADER_ONLY=ON")
  endif()

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
  )
endif()

# -------------------------
# Dependencies: spdlog + fmt
# Prefer header-only targets to avoid runtime shared-library ABI issues
# in released Linux binaries.
# -------------------------
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

set(_VIX_SPDLOG_TARGET "")
set(_VIX_FMT_TARGET "")

# Prefer header-only spdlog to avoid libspdlog.so runtime dependency
if (TARGET spdlog::spdlog_header_only)
  set(_VIX_SPDLOG_TARGET spdlog::spdlog_header_only)
elseif (TARGET spdlog::spdlog)
  set(_VIX_SPDLOG_TARGET spdlog::spdlog)
else()
  message(FATAL_ERROR "[utils] spdlog target not found.")
endif()

# Prefer header-only fmt to avoid libfmt.so runtime dependency
if (TARGET fmt::fmt-header-only)
  set(_VIX_FMT_TARGET fmt::fmt-header-only)
elseif (TARGET fmt::fmt)
  set(_VIX_FMT_TARGET fmt::fmt)
else()
  message(FATAL_ERROR "[utils] fmt target not found.")
endif()

target_link_libraries(vix_utils
  PUBLIC
    ${_VIX_SPDLOG_TARGET}
    ${_VIX_FMT_TARGET}
)

if (VIX_HEADER_ONLY)
  target_compile_definitions(vix_utils INTERFACE SPDLOG_FMT_EXTERNAL=1)
else()
  target_compile_definitions(vix_utils PUBLIC SPDLOG_FMT_EXTERNAL=1)
endif()
# -------------------------
# Windows: prevent Win32 API usage by spdlog
# -------------------------
if (WIN32)
  if (VIX_HEADER_ONLY)
    target_compile_definitions(vix_utils INTERFACE SPDLOG_NO_WIN32_API=1)
  else()
    target_compile_definitions(vix_utils PRIVATE SPDLOG_NO_WIN32_API=1)
  endif()
endif()

# Compiler warnings
if (TARGET vix_warnings)
  if (VIX_HEADER_ONLY)
    target_link_libraries(vix_utils INTERFACE vix_warnings)
  else()
    target_link_libraries(vix_utils PRIVATE vix_warnings)
  endif()
else()
  if (VIX_HEADER_ONLY)
    if (NOT MSVC)
      target_compile_options(vix_utils INTERFACE
        -Wall -Wextra -Wpedantic
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
        -Wall -Wextra -Wpedantic
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
  if (VIX_HEADER_ONLY)
    target_link_libraries(vix_utils INTERFACE vix_sanitizers)
  else()
    target_link_libraries(vix_utils PRIVATE vix_sanitizers)
  endif()
endif()

# LTO (Release)
if(VIX_ENABLE_LTO AND NOT VIX_HEADER_ONLY AND CMAKE_BUILD_TYPE STREQUAL "Release")
  include(CheckIPOSupported)
  check_ipo_supported(RESULT ipo_ok OUTPUT ipo_msg)
  if(ipo_ok)
    set_property(TARGET vix_utils PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  else()
    message(WARNING "IPO/LTO not supported: ${ipo_msg}")
  endif()
endif()

# Install / Export (umbrella)
set_target_properties(vix_utils PROPERTIES
  EXPORT_NAME utils
  OUTPUT_NAME vix_utils
  VERSION ${PROJECT_VERSION}
  SOVERSION 0
)

if(NOT VIX_HEADER_ONLY)
  install(TARGETS vix_utils
    EXPORT  VixTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
else()
  install(TARGETS vix_utils
    EXPORT  VixTargets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
endif()

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")

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

install(EXPORT VixTargets
  FILE vix_utilsTargets.cmake
  NAMESPACE vix::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/vix_utils"
)

# Examples (opt-in)
if (VIX_UTILS_BUILD_EXAMPLES AND NOT VIX_HEADER_ONLY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  add_executable(utils_log_demo         examples/log_demo.cpp)
  target_link_libraries(utils_log_demo PRIVATE vix::utils)

  add_executable(utils_validation_demo  examples/validation_demo.cpp)
  target_link_libraries(utils_validation_demo PRIVATE vix::utils)

  add_executable(utils_env_time_uuid    examples/env_time_uuid.cpp)
  target_link_libraries(utils_env_time_uuid PRIVATE vix::utils)

  add_custom_target(utils_examples ALL
    DEPENDS utils_log_demo utils_validation_demo utils_env_time_uuid
  )
endif()

# Status
if(DEFINED UTILS_SOURCES AND NOT VIX_HEADER_ONLY)
  message(STATUS "Utils library sources: ${UTILS_SOURCES}")
else()
  message(STATUS "Utils header-only mode or no sources listed")
endif()
