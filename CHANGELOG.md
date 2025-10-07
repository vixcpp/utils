Changelog – vixcpp/utils

All notable changes to this module will be documented in this file.
The format is based on Keep a Changelog : https://keepachangelog.com/en/1.0.0/

and this project adheres to Semantic Versioning: https://semver.org/spec/v2.0.0.html
.

[0.1.0] – 2025-10-07
Added

Initial release of the utils module.

String utilities: trimming, splitting, case conversion, and prefix/suffix checks.

File utilities: read/write entire files, check for existence, create directories.

Path helpers: normalize paths, extract filename or extension.

Time helpers: timestamp formatting, current time in ISO 8601, duration helpers.

Cross-platform support (Linux, macOS, Windows).

Fully header-only, minimal dependencies.

Changed

Refactored internal string helpers from core into standalone utils module.

Fixed

Fixed inconsistent newline handling on Windows in file utilities.

Corrected path normalization to handle trailing slashes properly.

[0.0.1] – Draft

Created project structure for utils.

Set up CMake target vix_utils with interface include directory.

Populated initial utility headers: string.hpp, file.hpp, path.hpp, time.hpp.

Added example usages in examples/ for each utility group.
