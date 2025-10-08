#include <vix/utils/Logger.hpp>
#include <vix/utils/UUID.hpp>
#include <vix/utils/Env.hpp>

using Vix::Logger;
using namespace Vix::utils;

int main()
{
    auto &log = Logger::getInstance();

    // Pattern par défaut (compatible avec setAsync)
    log.setPattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

    // Niveau & mode async via env
    const bool async_mode = env_bool("VIX_LOG_ASYNC", true);
    log.setAsync(async_mode);
    log.setLevel(env_bool("VIX_LOG_DEBUG", false) ? Logger::Level::DEBUG : Logger::Level::INFO);

    // Contexte (request_id/module/fields)
    Logger::Context cx;
    cx.request_id = uuid4();
    cx.module = "log_demo";
    cx.fields["service"] = "utils";
    cx.fields["env"] = env_or("APP_ENV", "dev");
    log.setContext(cx);

    log.log(Logger::Level::INFO, "Hello from utils/log_demo");
    log.log(Logger::Level::DEBUG, "Debug enabled = {}", env_bool("VIX_LOG_DEBUG", false));

    // Log structuré (clé=valeur)
    log.logf(Logger::Level::INFO, "Boot args", "port", env_int("APP_PORT", 8080), "async", async_mode);

    log.log(Logger::Level::WARN, "This is a warning");
    // log.throwError("Demo error: {}", "something went wrong"); // décommente pour tester
    return 0;
}
