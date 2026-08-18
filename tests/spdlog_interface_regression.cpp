// Check the compiler command line before spdlog's own configuration headers
// have a chance to define this macro.  Packaged spdlog installations may set
// SPDLOG_FMT_EXTERNAL in tweakme.h; that is not a vix::utils interface leak.
#ifdef SPDLOG_FMT_EXTERNAL
#error "vix::utils leaked SPDLOG_FMT_EXTERNAL into an independent consumer"
#endif

#include <spdlog/spdlog.h>

int main()
{
  spdlog::info("vix utils interface regression");
  return 0;
}
