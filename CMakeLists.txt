# ====================================================================
# Vix.cpp — Utils Module
# ====================================================================
# Purpose:
#   - Cross-cutting utilities (logging, validation helpers, env/time/UUID, etc.)
#   - Usually a small static library; can be header-only if desired.
#
# Targets:
#   - vix_utils   (real target)
#   - Vix::utils  (alias, preferred by consumers)
#
# Notes:
#   - Public headers are under modules/utils/include/...
#   - This module depends on spdlog for logging.
# ====================================================================

cmake_minimum_required(VERSION 3.20)
project(vix_utils VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Default build type (friendly for local development)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Reasonable warnings; projects can add more via their own toolchains.
set(GLOBAL_CXX_FLAGS "-Wall -Wextra -Wshadow -Weffc++")
set(CMAKE_CXX_FLAGS_RELEASE "${GLOBAL_CXX_FLAGS} -O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG   "${GLOBAL_CXX_FLAGS} -g -fsanitize=address -fsanitize=undefined -D_GLIBCXX_DEBUG")

# Source discovery — keep it simple and explicit if you prefer.
file(GLOB_RECURSE UTILS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(vix_utils STATIC ${UTILS_SOURCES})
add_library(Vix::utils ALIAS vix_utils)

target_compile_features(vix_utils PUBLIC cxx_std_20)

target_include_directories(vix_utils
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# External dependencies
find_package(spdlog REQUIRED)
target_link_libraries(vix_utils PUBLIC spdlog::spdlog)

# Export metadata (optional install/export support)
set_target_properties(vix_utils PROPERTIES
  EXPORT_NAME utils
  OUTPUT_NAME vix_utils
  VERSION ${PROJECT_VERSION}
  SOVERSION 0
)

install(TARGETS vix_utils
  EXPORT vix_utilsTargets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

install(DIRECTORY include/ DESTINATION include)

export(EXPORT vix_utilsTargets
  FILE "${CMAKE_CURRENT_BINARY_DIR}/vix_utilsTargets.cmake"
  NAMESPACE Vix::
)

# Examples (opt-in)
option(VIX_UTILS_BUILD_EXAMPLES "Build utils examples" OFF)
if (VIX_UTILS_BUILD_EXAMPLES)
  add_executable(utils_log_demo         examples/log_demo.cpp)
  target_link_libraries(utils_log_demo PRIVATE Vix::utils)

  add_executable(utils_validation_demo  examples/validation_demo.cpp)
  target_link_libraries(utils_validation_demo PRIVATE Vix::utils)

  add_executable(utils_env_time_uuid    examples/env_time_uuid.cpp)
  target_link_libraries(utils_env_time_uuid PRIVATE Vix::utils)
endif()

message(STATUS "Utils library configured with ${UTILS_SOURCES}")
