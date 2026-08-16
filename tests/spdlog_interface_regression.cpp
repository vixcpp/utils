#include <spdlog/spdlog.h>

#ifdef SPDLOG_FMT_EXTERNAL
#error "vix::utils leaked SPDLOG_FMT_EXTERNAL into an independent consumer"
#endif

int main()
{
  spdlog::info("vix utils interface regression");
  return 0;
}
