#include <cassert>
#include <string>
#include <system_error>

#include <vix/utils/NetworkError.hpp>

namespace
{
  void test_case_insensitive_message_classification()
  {
    assert(vix::utils::contains_token_icase("Connection RESET by peer", "reset"));
    assert(vix::utils::contains_token_icase("anything", ""));
    assert(!vix::utils::contains_token_icase("permission denied", "reset"));

    assert(vix::utils::is_normal_network_disconnect_message("Broken pipe"));
    assert(vix::utils::is_normal_network_disconnect_message("broken pipe"));
    assert(vix::utils::is_normal_network_disconnect_message("connection reset"));
    assert(vix::utils::is_normal_network_disconnect_message("Connection reset"));
    assert(vix::utils::is_normal_network_disconnect_message("Connection reset by peer"));
    assert(vix::utils::is_normal_network_disconnect_message("operation canceled"));
    assert(vix::utils::is_normal_network_disconnect_message("operation cancelled"));
    assert(vix::utils::is_normal_network_disconnect_message("Operation canceled"));
    assert(vix::utils::is_normal_network_disconnect_message("Operation cancelled"));
    assert(vix::utils::is_normal_network_disconnect_message("canceled"));
    assert(vix::utils::is_normal_network_disconnect_message("cancelled"));
    assert(vix::utils::is_normal_network_disconnect_message("End of file"));
    assert(vix::utils::is_normal_network_disconnect_message("EOF"));
    assert(vix::utils::is_normal_network_disconnect_message("eof"));
    assert(vix::utils::is_normal_network_disconnect_message("Unexpected eof"));

    assert(vix::utils::is_normal_network_disconnect_message(
        "websocket write failed: Broken pipe"));
    assert(vix::utils::is_normal_network_disconnect_message(
        "read failed: connection reset by peer"));
    assert(vix::utils::is_normal_network_disconnect_message("unexpected EOF"));

    assert(!vix::utils::is_normal_network_disconnect_message("permission denied"));
    assert(!vix::utils::is_normal_network_disconnect_message("invalid response state"));
    assert(!vix::utils::is_normal_network_disconnect_message(
        "websocket handshake must use GET"));
    assert(!vix::utils::is_normal_network_disconnect_message(
        "missing Upgrade: websocket"));
    assert(!vix::utils::is_normal_network_disconnect_message(
        "missing Connection: Upgrade"));
    assert(!vix::utils::is_normal_network_disconnect_message(
        "missing Sec-WebSocket-Key"));
    assert(!vix::utils::is_normal_network_disconnect_message(
        "unsupported Sec-WebSocket-Version"));
    assert(!vix::utils::is_normal_network_disconnect_message(
        "websocket frame write failed"));
    assert(!vix::utils::is_normal_network_disconnect_message(
        "websocket handshake write failed"));
    assert(!vix::utils::is_normal_network_disconnect_message("stream not open"));
    assert(!vix::utils::is_normal_network_disconnect_message(
        "failed to create native Vix TCP listener"));
    assert(!vix::utils::is_normal_network_disconnect_message(""));
    assert(!vix::utils::is_normal_network_disconnect_message(
        "unknown websocket failure"));
    assert(!vix::utils::is_normal_network_disconnect_message(
        "invalid websocket frame"));
  }

  void test_structured_error_classification()
  {
    assert(vix::utils::is_normal_network_disconnect(
        std::system_error(std::make_error_code(std::errc::broken_pipe))));
    assert(vix::utils::is_normal_network_disconnect(
        std::system_error(std::make_error_code(std::errc::connection_reset))));
    assert(vix::utils::is_normal_network_disconnect(
        std::system_error(std::make_error_code(std::errc::operation_canceled))));
    assert(vix::utils::is_normal_network_disconnect(
        std::system_error(std::make_error_code(std::errc::timed_out))));
    assert(!vix::utils::is_normal_network_disconnect(
        std::system_error(std::make_error_code(std::errc::permission_denied))));
  }
} // namespace

int main()
{
  test_case_insensitive_message_classification();
  test_structured_error_classification();
  return 0;
}
