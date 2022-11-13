#ifndef TCP_SERVER_STREAM_H
#define TCP_SERVER_STREAM_H

#include "server_stream.h"

#include <boost/asio.hpp>

namespace net = boost::asio;
namespace sys = boost::system;
using tcp = boost::asio::ip::tcp;

class tcp_server_stream final : public server_stream 
{
public:
    tcp_server_stream(const stream_manager_ptr& ptr, int id, net::io_context& ctx);
    ~tcp_server_stream() override;

    net::io_context& context() override;
    tcp::socket& socket();
private:
    void do_start() final;
    void do_stop() final;
    void do_read() final;
    void do_write(io_event event) final;

    void handle_error(const sys::error_code& ec);

    net::io_context& ctx_;
    tcp::socket socket_;

    std::array<std::uint8_t, max_buffer_size> read_buffer_;
    std::array<std::uint8_t, max_buffer_size> write_buffer_;
};

#endif //TCP_SERVER_STREAM_H
