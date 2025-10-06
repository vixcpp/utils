# ======================================================
# vix.cpp utils module CMake configuration 
# ======================================================

cmake_minimum_required(VERSION 3.20)
project(vix_utils VERSION 0.1 LANGUAGES CXX)

# ======================================================
# C++ standard and options
# ======================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)  # required for static/shared libs

# ======================================================
# Build types and warnings
# ======================================================
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(GLOBAL_CXX_FLAGS "-Wall -Wextra -Wshadow -Weffc++")
set(CMAKE_CXX_FLAGS_RELEASE "${GLOBAL_CXX_FLAGS} -O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${GLOBAL_CXX_FLAGS} -g -fsanitize=address -fsanitize=undefined -D_GLIBCXX_DEBUG")

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "CXX flags: ${CMAKE_CXX_FLAGS}")

# ======================================================
# Sources
# ======================================================
file(GLOB_RECURSE UTILS_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# ======================================================
# Create static library 'utils'
# ======================================================
add_library(utils STATIC ${UTILS_SOURCES})

target_include_directories(utils
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ======================================================
# Dependencies
# ======================================================
find_package(spdlog REQUIRED)
target_link_libraries(utils PRIVATE spdlog::spdlog)

# ======================================================
# Export target for other projects
# ======================================================
set_target_properties(utils PROPERTIES
    EXPORT_NAME utils
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
)

# ======================================================
# Optional: installation
# ======================================================
install(TARGETS utils
    EXPORT vix_utilsTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/ DESTINATION include)

# ======================================================
# Export CMake config for downstream projects
# ======================================================
export(EXPORT vix_utilsTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/vix_utilsTargets.cmake"
    NAMESPACE Vix::
)

message(STATUS "Utils library configured with ${UTILS_SOURCES}")
