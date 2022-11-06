#include "tls_server_stream.h"
#include "transport/stream_manager.h"
#include "logger/logger.h"

#include <boost/format.hpp>

using fmt = boost::format;
using logger = logging::logger;

tls_server_stream::tls_server_stream(const stream_manager_ptr& ptr, int id, net::io_context& ctx, net::ssl::context& ssl_ctx)
    : server_stream{ptr, id}
    , ctx_{ctx}
    , ssl_ctx_{ssl_ctx}
    , socket_{ctx_, ssl_ctx_}
    , read_buffer_{}
    , write_buffer_{} 
{
}

tls_server_stream::~tls_server_stream() 
{
    auto str = (fmt("[%1%] tcp server stream closed") % id()).str();
    logger::trace(str);
}

net::io_context& tls_server_stream::context() { return ctx_; }

tcp::socket& tls_server_stream::socket() 
{ 
    return socket_.next_layer();
}

void tls_server_stream::do_start() 
{
    const auto str{(fmt("[%1%] incoming connection from socks5-client: [%2%]")
                   % id() % socket_.next_layer().remote_endpoint()).str()};
    logger::debug(str);
    do_handshake();
}

void tls_server_stream::do_stop() 
{
    if (!socket_.lowest_layer().is_open())
        return;

    sys::error_code ignored_ec;
    socket_.lowest_layer().cancel(ignored_ec);

    socket_.async_shutdown(
        [this, self{shared_from_this()}](const sys::error_code& ec) {
            if (ec)
                handle_error(ec);
            socket_.lowest_layer().close();
        });

    net::async_write(
        socket_, net::null_buffers{},
        [this, self{shared_from_this()}](const sys::error_code& ec, std::size_t trans_bytes) {
            if (ec)
                handle_error(ec);
            socket_.lowest_layer().close();
        });
}

void tls_server_stream::do_handshake()
{
    socket_.async_handshake(
        net::ssl::stream_base::server,
        [this, self{shared_from_this()}](const sys::error_code& ec) {
        if (!ec) {
            do_read();
        } else {
            handle_error(ec);
        }
    });
}

void tls_server_stream::do_write(io_event event) 
{
    copy(event.begin(), event.end(), write_buffer_.begin());
    net::async_write(
            socket_, net::buffer(write_buffer_, event.size()),
            [this, self{shared_from_this()}](const sys::error_code &ec, size_t) {
                if (!ec) {
                    manager()->on_write(std::move(io_event{}), shared_from_this());
                } else {
                    handle_error(ec);
                }
            });
}

void tls_server_stream::do_read() 
{
    socket_.async_read_some(
            net::buffer(read_buffer_),
            [this, self{shared_from_this()}](const sys::error_code &ec, const size_t length) {
                if (!ec) {
                    io_event event(read_buffer_.data(), read_buffer_.data() + length);
                    manager()->on_read(std::move(event), shared_from_this());
                } else {
                    handle_error(ec);
                }
            });
}

void tls_server_stream::handle_error(const sys::error_code& ec) 
{
    manager()->on_error(ec, shared_from_this());
}






