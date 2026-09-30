cmake_minimum_required(VERSION 3.20)
project(vix_utils VERSION 1.5.2 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (DEFINED VIX_UMBRELLA_BUILD AND VIX_UMBRELLA_BUILD)
  set(VIX_UTILS_EXPORT_SET VixTargets)
else()
  set(VIX_UTILS_EXPORT_SET vix_utilsTargets)
endif()

option(VIX_STRICT "Treat warnings as errors" OFF)
option(VIX_ENABLE_LTO "Enable link-time optimization (Release)" OFF)
option(VIX_HEADER_ONLY "Build vix_utils as header-only INTERFACE" OFF)
option(VIX_UTILS_BUILD_EXAMPLES "Build utils examples" OFF)
option(VIX_UTILS_BUILD_TESTS "Build utils CMake regression tests" OFF)

execute_process(
  COMMAND git rev-parse --short HEAD
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE VIX_GIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET)
if (NOT VIX_GIT_HASH)
  set(VIX_GIT_HASH unknown)
endif()
string(TIMESTAMP VIX_BUILD_DATE "%Y-%m-%d %H:%M:%S UTC" UTC)

if (TARGET vix::log)
  set(VIX_LOG_TARGET vix::log)
elseif (TARGET vix_log)
  set(VIX_LOG_TARGET vix_log)
else()
  find_package(vix_log CONFIG QUIET)
  if (TARGET vix::log)
    set(VIX_LOG_TARGET vix::log)
  elseif (TARGET vix_log)
    set(VIX_LOG_TARGET vix_log)
  endif()
endif()
if (NOT VIX_LOG_TARGET)
  message(FATAL_ERROR "[utils] vix::log is required for deprecated logging compatibility headers.")
endif()

if (TARGET vix::env)
  set(VIX_ENV_TARGET vix::env)
elseif (TARGET vix_env)
  set(VIX_ENV_TARGET vix_env)
else()
  find_package(vix_env CONFIG QUIET)
  if (TARGET vix::env)
    set(VIX_ENV_TARGET vix::env)
  elseif (TARGET vix_env)
    set(VIX_ENV_TARGET vix_env)
  endif()
endif()
if (NOT VIX_ENV_TARGET)
  message(FATAL_ERROR "[utils] vix::env is required for deprecated environment compatibility headers.")
endif()

if (VIX_HEADER_ONLY)
  add_library(vix_utils INTERFACE)
  add_library(vix::utils ALIAS vix_utils)
  target_compile_features(vix_utils INTERFACE cxx_std_20)
  target_include_directories(vix_utils INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
  target_compile_definitions(vix_utils INTERFACE
    VIX_GIT_HASH="${VIX_GIT_HASH}"
    VIX_BUILD_DATE="${VIX_BUILD_DATE}")
  target_link_libraries(vix_utils INTERFACE ${VIX_LOG_TARGET} ${VIX_ENV_TARGET})
else()
  file(GLOB_RECURSE UTILS_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
  add_library(vix_utils STATIC ${UTILS_SOURCES})
  add_library(vix::utils ALIAS vix_utils)
  target_compile_features(vix_utils PUBLIC cxx_std_20)
  target_include_directories(vix_utils PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
  target_compile_definitions(vix_utils PUBLIC
    VIX_GIT_HASH="${VIX_GIT_HASH}"
    VIX_BUILD_DATE="${VIX_BUILD_DATE}")
  target_link_libraries(vix_utils PUBLIC ${VIX_LOG_TARGET} ${VIX_ENV_TARGET})
  if (NOT MSVC)
    target_compile_options(vix_utils PRIVATE -Wall -Wextra -Wpedantic)
    if (VIX_STRICT)
      target_compile_options(vix_utils PRIVATE -Werror)
    endif()
  endif()
endif()

set_target_properties(vix_utils PROPERTIES
  EXPORT_NAME utils
  OUTPUT_NAME vix_utils
  VERSION ${PROJECT_VERSION}
  SOVERSION 0)
install(TARGETS vix_utils EXPORT ${VIX_UTILS_EXPORT_SET}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if (NOT (DEFINED VIX_UMBRELLA_BUILD AND VIX_UMBRELLA_BUILD))
  install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
  configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/vix_utilsConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/vix_utilsConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/vix_utils")
  write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/vix_utilsConfigVersion.cmake"
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/vix_utilsConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/vix_utilsConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/vix_utils")
  install(EXPORT vix_utilsTargets FILE vix_utilsTargets.cmake NAMESPACE vix::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/vix_utils")
endif()

if (VIX_UTILS_BUILD_EXAMPLES AND NOT VIX_HEADER_ONLY)
  add_executable(utils_log_demo examples/log_demo.cpp)
  target_link_libraries(utils_log_demo PRIVATE vix::utils)
  add_executable(utils_validation_demo examples/validation_demo.cpp)
  target_link_libraries(utils_validation_demo PRIVATE vix::utils)
  add_executable(utils_env_time_uuid examples/env_time_uuid.cpp)
  target_link_libraries(utils_env_time_uuid PRIVATE vix::utils)
endif()
if (VIX_UTILS_BUILD_TESTS)
  include(CTest)
  add_subdirectory(tests)
endif()
