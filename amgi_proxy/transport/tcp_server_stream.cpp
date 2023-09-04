#include "tcp_server_stream.h"
#include "stream_manager.h"
#include "logger/logger.h"

#include <boost/format.hpp>

using fmt = boost::format;
using logger = logging::logger;

namespace
{
    std::string ep_to_str(const tcp::socket& sock)
    {
        if (!sock.is_open())
            return "socket not opened";

        net::error_code ec;
        const auto& rep = sock.remote_endpoint(ec);

        if (ec)
            return std::string{"remote_endpoint failed: " + ec.message()};

        net::error_code ignored_ec;
        return { rep.address().to_string(ignored_ec) + ":" + std::to_string(rep.port()) };
    }
}

tcp_server_stream::tcp_server_stream(const stream_manager_ptr& ptr, int id, net::io_context& ctx)
    : server_stream{ptr, id}, ctx_{ctx}, socket_{ctx_}, read_buffer_{}, write_buffer_{} 
{
}

tcp_server_stream::~tcp_server_stream() 
{
    auto str = (fmt("[%1%] tcp server stream closed") % id()).str();
    logger::trace(str);
}

net::io_context& tcp_server_stream::context() { return ctx_; }

tcp::socket& tcp_server_stream::socket() { return socket_; }

void tcp_server_stream::do_start() 
{
    const auto str{(fmt("[%1%] incoming connection from socks5-client: [%2%]")
                   % id() % ep_to_str(socket_)).str()};
    logger::debug(str);
    do_read();
}

void tcp_server_stream::do_stop() 
{
    net::error_code ignored_ec;
    socket_.shutdown(tcp::socket::shutdown_both, ignored_ec);
}

void tcp_server_stream::do_write(io_event event) 
{
    copy(event.begin(), event.end(), write_buffer_.begin());
    net::async_write(
            socket_, net::buffer(write_buffer_, event.size()),
            [this, self{shared_from_this()}](const net::error_code &ec, size_t) {
                if (!ec) {
                    manager()->on_write(std::move(io_event{}), shared_from_this());
                } else {
                    handle_error(ec);
                }
            });
}

void tcp_server_stream::do_read() 
{
    socket_.async_read_some(
            net::buffer(read_buffer_),
            [this, self{shared_from_this()}](const net::error_code &ec, const size_t length) {
                if (!ec) {
                    io_event event(read_buffer_.data(), read_buffer_.data() + length);
                    manager()->on_read(std::move(event), shared_from_this());
                } else {
                    handle_error(ec);
                }
            });
}

void tcp_server_stream::handle_error(const net::error_code& ec) 
{
    manager()->on_error(ec, shared_from_this());
}
