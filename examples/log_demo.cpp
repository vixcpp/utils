#include <vix/utils/Logger.hpp>
#include <vix/utils/UUID.hpp>
#include <vix/utils/Env.hpp>

using Vix::Logger;
using namespace Vix::utils;

int main()
{
    auto &log = Logger::getInstance();
    log.setLevel(Logger::Level::INFO);
    log.setPattern("[%Y-%m-%d %T.%e] [%^%l%$] %v");

    Logger::Context cx;
    cx.request_id = uuid4();
    cx.module = "log_demo";
    cx.fields["service"] = "utils";
    log.setContext(cx);

    log.log(Logger::Level::INFO, "Hello from utils/log_demo");
    log.logf(Logger::Level::INFO, "Boot args", "port", 8080, "env", env_or("APP_ENV", "dev"));
    log.log(Logger::Level::WARN, "This is a warning");
    // log.throwError("Demo error"); // décommente pour tester
    return 0;
}
