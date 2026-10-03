include(CTest)
enable_testing()

get_target_property(_vix_utils_interface_definitions vix_utils INTERFACE_COMPILE_DEFINITIONS)
foreach(_vix_definition IN LISTS _vix_utils_interface_definitions)
  if (_vix_definition MATCHES "^SPDLOG_FMT_EXTERNAL(=|$)")
    message(FATAL_ERROR
      "vix_utils exports SPDLOG_FMT_EXTERNAL to consumers: ${_vix_definition}")
  endif()
endforeach()

# Verify the Vix2 compatibility surface against the canonical Vix logging
# contract. This test must not require direct knowledge of the spdlog provider.
add_executable(vix_utils_spdlog_interface_regression
  spdlog_interface_regression.cpp
)
target_compile_features(vix_utils_spdlog_interface_regression PRIVATE cxx_std_20)
target_link_libraries(vix_utils_spdlog_interface_regression PRIVATE vix::utils)

add_test(
  NAME vix_utils_spdlog_interface_regression
  COMMAND vix_utils_spdlog_interface_regression
)

# Verify that the historical environment header delegates to vix::env's
# compatibility semantics rather than retaining a second parser or lookup path.
add_executable(vix_utils_env_compatibility_regression
  env_compatibility_regression.cpp
)
target_compile_features(vix_utils_env_compatibility_regression PRIVATE cxx_std_20)
target_link_libraries(vix_utils_env_compatibility_regression PRIVATE vix::utils)

add_test(
  NAME vix_utils_env_compatibility_regression
  COMMAND vix_utils_env_compatibility_regression
)

# Preserve the historical generic Result contract independently from the
# canonical vix::error::Result contract. Validation still relies on this
# Vix2 compatibility surface.
add_executable(vix_utils_result_compatibility_regression
  result_compatibility_regression.cpp
)
target_compile_features(vix_utils_result_compatibility_regression PRIVATE cxx_std_20)
target_link_libraries(vix_utils_result_compatibility_regression PRIVATE vix::utils)

add_test(
  NAME vix_utils_result_compatibility_regression
  COMMAND vix_utils_result_compatibility_regression
)

# Preserve the independent Vix2 string-map validation contract. It must not
# depend on or redefine the canonical vix::validation capability.
add_executable(vix_utils_validation_compatibility_regression
  validation_compatibility_regression.cpp
)
target_compile_features(vix_utils_validation_compatibility_regression PRIVATE cxx_std_20)
target_link_libraries(vix_utils_validation_compatibility_regression PRIVATE vix::utils)

add_test(
  NAME vix_utils_validation_compatibility_regression
  COMMAND vix_utils_validation_compatibility_regression
)

# Preserve the independent Vix2 time helper contract. It intentionally does
# not depend on or replace the canonical vix::time API.
add_executable(vix_utils_time_compatibility_regression
  time_compatibility_regression.cpp
)
target_compile_features(vix_utils_time_compatibility_regression PRIVATE cxx_std_20)
target_link_libraries(vix_utils_time_compatibility_regression PRIVATE vix::utils)

add_test(
  NAME vix_utils_time_compatibility_regression
  COMMAND vix_utils_time_compatibility_regression
)

# Preserve the Vix2 String contract while focused consumers take ownership of
# their canonical parsing responsibilities independently.
add_executable(vix_utils_string_compatibility_regression
  string_compatibility_regression.cpp
)
target_compile_features(vix_utils_string_compatibility_regression PRIVATE cxx_std_20)
target_link_libraries(vix_utils_string_compatibility_regression PRIVATE vix::utils)

add_test(
  NAME vix_utils_string_compatibility_regression
  COMMAND vix_utils_string_compatibility_regression
)

# Preserve the Vix2 server-ready banner contract independently from the
# canonical HTTP/server startup presentation surface.
add_executable(vix_utils_server_pretty_logs_compatibility_regression
  server_pretty_logs_compatibility_regression.cpp
)
target_compile_features(vix_utils_server_pretty_logs_compatibility_regression PRIVATE cxx_std_20)
target_link_libraries(vix_utils_server_pretty_logs_compatibility_regression PRIVATE vix::utils)

add_test(
  NAME vix_utils_server_pretty_logs_compatibility_regression
  COMMAND vix_utils_server_pretty_logs_compatibility_regression
)

# Preserve the Vix2 NetworkError heuristic as compatibility-only behavior.
add_executable(vix_utils_network_error_compatibility_regression
  network_error_compatibility_regression.cpp
)
target_compile_features(vix_utils_network_error_compatibility_regression PRIVATE cxx_std_20)
target_link_libraries(vix_utils_network_error_compatibility_regression PRIVATE vix::utils)

add_test(
  NAME vix_utils_network_error_compatibility_regression
  COMMAND vix_utils_network_error_compatibility_regression
)
