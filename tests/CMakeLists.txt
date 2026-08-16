include(CTest)
enable_testing()

get_target_property(_vix_utils_interface_definitions vix_utils INTERFACE_COMPILE_DEFINITIONS)
foreach(_vix_definition IN LISTS _vix_utils_interface_definitions)
  if (_vix_definition MATCHES "^SPDLOG_FMT_EXTERNAL(=|$)")
    message(FATAL_ERROR
      "vix_utils exports SPDLOG_FMT_EXTERNAL to consumers: ${_vix_definition}")
  endif()
endforeach()

# Model a consumer which uses an independent bundled-fmt spdlog.  It gets the
# spdlog include path but deliberately not spdlog's target usage requirements;
# vix::utils must therefore not inject SPDLOG_FMT_EXTERNAL into this compile.
get_target_property(_vix_spdlog_includes ${_VIX_SPDLOG_TARGET} INTERFACE_INCLUDE_DIRECTORIES)
add_executable(vix_utils_spdlog_interface_regression
  spdlog_interface_regression.cpp
)
target_compile_features(vix_utils_spdlog_interface_regression PRIVATE cxx_std_20)
target_include_directories(vix_utils_spdlog_interface_regression PRIVATE ${_vix_spdlog_includes})
target_link_libraries(vix_utils_spdlog_interface_regression PRIVATE vix::utils)

add_test(
  NAME vix_utils_spdlog_interface_regression
  COMMAND vix_utils_spdlog_interface_regression
)
