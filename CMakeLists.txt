cmake_minimum_required(VERSION 3.20)

# ====================================================================
# Vix.cpp — Utils Module
# ====================================================================
# Purpose:
#   Cross-cutting utilities used by other Vix modules and end-user apps:
#   logging, validation helpers, environment/time/UUID utilities, etc.
#
# Targets:
#   - vix_utils   : Real library target (STATIC by default, or header-only INTERFACE)
#   - Vix::utils  : Namespaced alias for consumers
#
# Integration & Install:
#   - Public headers live under modules/utils/include/...
#   - This module depends on spdlog for logging.
#   - The target joins the umbrella export-set `VixTargets` so that
#     a system-wide installation of Vix exposes Vix::utils via:
#       find_package(Vix CONFIG REQUIRED)
#       target_link_libraries(app PRIVATE Vix::utils)
#
# Notes:
#   - Sanitizers are inherited from the umbrella if VIX_ENABLE_SANITIZERS=ON.
#   - Header-only mode is supported via VIX_HEADER_ONLY=ON.
# ====================================================================

project(vix_utils VERSION 1.4.1 LANGUAGES CXX)

include(GNUInstallDirs)
include(CheckCXXCompilerFlag)

# ================== Options ==================
option(VIX_STRICT               "Treat warnings as errors"                  OFF)
option(VIX_ENABLE_LTO           "Enable link-time optimization (Release)"   OFF)
option(VIX_HEADER_ONLY          "Build vix_utils as header-only INTERFACE"  OFF)
option(VIX_UTILS_BUILD_EXAMPLES "Build utils examples"                      OFF)
# Sanitizers are inherited from the umbrella via VIX_ENABLE_SANITIZERS

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ================== Git / Build metadata ==================
# Use CURRENT_SOURCE_DIR to allow inclusion via add_subdirectory.
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

# ================== Sources ==================
if(VIX_HEADER_ONLY)
  add_library(vix_utils INTERFACE)
  add_library(Vix::utils ALIAS vix_utils)

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
  if(UTILS_SOURCES STREQUAL "")
    message(FATAL_ERROR "No sources found under src/. If you want header-only, set VIX_HEADER_ONLY=ON")
  endif()

  add_library(vix_utils STATIC ${UTILS_SOURCES})
  add_library(Vix::utils ALIAS vix_utils)

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

# ================== Dependencies ==================
find_package(spdlog REQUIRED)
# Prefer header-only if available; otherwise use the compiled target.
if(TARGET spdlog::spdlog_header_only)
  if(VIX_HEADER_ONLY)
    target_link_libraries(vix_utils INTERFACE spdlog::spdlog_header_only)
  else()
    target_link_libraries(vix_utils PUBLIC spdlog::spdlog_header_only)
  endif()
else()
  if(VIX_HEADER_ONLY)
    target_link_libraries(vix_utils INTERFACE spdlog::spdlog)
  else()
    target_link_libraries(vix_utils PUBLIC spdlog::spdlog)
  endif()
endif()

# ================== Compiler warnings ==================
function(vix_add_flags_if_supported tgt)
  foreach(flag IN LISTS ARGN)
    string(REGEX REPLACE "[^A-Za-z0-9]" "_" flag_var "${flag}")
    set(test_var "HAVE_CXXFLAG_${flag_var}")
    check_cxx_compiler_flag("${flag}" ${test_var})
    if(${test_var})
      target_compile_options(${tgt} PRIVATE "${flag}")
    endif()
  endforeach()
endfunction()

function(vix_apply_warnings tgt)
  if(MSVC)
    vix_add_flags_if_supported(${tgt} /W4 /permissive-)
    if(VIX_STRICT)
      vix_add_flags_if_supported(${tgt} /WX)
    endif()
  else()
    vix_add_flags_if_supported(${tgt}
      -Wall -Wextra -Wshadow
      -Wconversion -Wsign-conversion
      -Wformat=2 -Wundef
      -Wnon-virtual-dtor -Woverloaded-virtual
      -Wnull-dereference -Wdouble-promotion
      -Wold-style-cast -Wredundant-decls -Wcast-qual -Wcast-align
      -Weffc++
    )
    vix_add_flags_if_supported(${tgt}
      -Wduplicated-cond -Wduplicated-branches
      -Wlogical-op -Wuseless-cast -Wno-psabi
    )
    vix_add_flags_if_supported(${tgt} -Wno-unknown-warning-option)

    if(VIX_STRICT)
      vix_add_flags_if_supported(${tgt} -Werror)
    endif()
  endif()
endfunction()

if(NOT VIX_HEADER_ONLY)
  vix_apply_warnings(vix_utils)
endif()

# ================== Sanitizers (umbrella-inherited) ==================
function(vix_utils_apply_sanitizers tgt)
  if (VIX_ENABLE_SANITIZERS AND NOT VIX_HEADER_ONLY AND TARGET ${tgt})
    message(STATUS "[utils] enabling sanitizers on ${tgt}")
    target_compile_options(${tgt} PRIVATE -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined)
    target_link_options(   ${tgt} PRIVATE                       -fsanitize=address,undefined)
  endif()
endfunction()
vix_utils_apply_sanitizers(vix_utils)

# ================== LTO (Release) ==================
if(VIX_ENABLE_LTO AND NOT VIX_HEADER_ONLY AND CMAKE_BUILD_TYPE STREQUAL "Release")
  include(CheckIPOSupported)
  check_ipo_supported(RESULT ipo_ok OUTPUT ipo_msg)
  if(ipo_ok)
    set_property(TARGET vix_utils PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  else()
    message(WARNING "IPO/LTO not supported: ${ipo_msg}")
  endif()
endif()

# ================== Install / Export (umbrella) ==================
set_target_properties(vix_utils PROPERTIES
  EXPORT_NAME utils
  OUTPUT_NAME vix_utils
  VERSION ${PROJECT_VERSION}
  SOVERSION 0
)

if(NOT VIX_HEADER_ONLY)
  install(TARGETS vix_utils
    EXPORT  VixTargets                 # <- join umbrella export-set
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
else()
  install(TARGETS vix_utils
    EXPORT  VixTargets                 # <- even for INTERFACE, export target
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
endif()

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")

# ================== Examples (opt-in) ==================
if (VIX_UTILS_BUILD_EXAMPLES AND NOT VIX_HEADER_ONLY)
  add_executable(utils_log_demo         examples/log_demo.cpp)
  target_link_libraries(utils_log_demo PRIVATE Vix::utils)

  add_executable(utils_validation_demo  examples/validation_demo.cpp)
  target_link_libraries(utils_validation_demo PRIVATE Vix::utils)

  add_executable(utils_env_time_uuid    examples/env_time_uuid.cpp)
  target_link_libraries(utils_env_time_uuid PRIVATE Vix::utils)
endif()

# ================== Status ==================
if(DEFINED UTILS_SOURCES AND NOT VIX_HEADER_ONLY)
  message(STATUS "Utils library sources: ${UTILS_SOURCES}")
else()
  message(STATUS "Utils header-only mode or no sources listed")
endif()
