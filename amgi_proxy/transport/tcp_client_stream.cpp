#include "tcp_client_stream.h"
#include "stream_manager.h"
#include "logger/logger.h"

#include <boost/format.hpp>

using fmt = boost::format;
using logger = logging::logger;

namespace
{
    enum : std::int32_t { eRemote, eLocal };
    std::string ep_to_str(const tcp::socket& sock, std::int32_t dir)
    {
        if (!sock.is_open())
            return "socket not opened";

        net::error_code ec;
        const auto& rep = (dir == eRemote) ? sock.remote_endpoint(ec) : sock.local_endpoint(ec);
        if (ec)
        {
            std::stringstream ss;
            ss << ((dir == eRemote) ? "remote_endpoint failed: " : "local_endpoint failed: ");
            ss << ec.message();
            return ss.str();
        }

        net::error_code ignored_ec;
        return { rep.address().to_string(ignored_ec) + ":" + std::to_string(rep.port()) };
    }
}

tcp_client_stream::tcp_client_stream(const stream_manager_ptr& ptr, int id, net::io_context& ctx)
    : client_stream{ptr, id}
    , socket_{ctx}
    , resolver_{ctx}, read_buffer_{}, write_buffer_{}
{}

tcp_client_stream::~tcp_client_stream() 
{
    auto str = (fmt("[%1%] tcp client stream closed (%2%:%3%)") % id() % host_ % port_).str();
    logger::trace(str);
}

void tcp_client_stream::do_start() 
{
    resolver_.async_resolve(
        host_, port_,
        [this, self{shared_from_this()}] (const net::error_code& ec, tcp::resolver::results_type results) {
            if (!ec) {
                do_connect(std::move(results));
            } else {
                handle_error(ec);
            }
        });
}

void tcp_client_stream::do_stop() 
{
    net::error_code ignored_ec;
    socket_.shutdown(tcp::socket::shutdown_both, ignored_ec);
}

void tcp_client_stream::do_connect(tcp::resolver::results_type&& results) 
{
    net::async_connect(
        socket_, results,
        [this, self{shared_from_this()}](const net::error_code& ec, const tcp::endpoint& ep) {
            if (!ec) {
                logger::info((fmt("[%1%] connected to [%2%] --> [%3%]") % id() % host_ % ep_to_str(socket_, eRemote)));
                logger::debug((fmt("[%1%] local address [%2%]") % id() % ep_to_str(socket_, eLocal)));
                io_buffer event{};
                manager()->on_connect(std::move(event), shared_from_this());
            } else {
                handle_error(ec);
            }
        });
}

void tcp_client_stream::do_write(io_buffer event) 
{
    std::copy(event.begin(), event.end(), write_buffer_.begin());
    net::async_write(
        socket_, net::buffer(write_buffer_, event.size()),
        [this, self{shared_from_this()}] (const net::error_code& ec, std::size_t) {
            if (!ec) {
                manager()->on_write(std::move(io_buffer{}), shared_from_this());
            } else {
                handle_error(ec);
            }
        });
}

void tcp_client_stream::do_read() 
{
    socket_.async_read_some(
        net::buffer(read_buffer_),
        [this, self{shared_from_this()}] (const net::error_code& ec, const std::size_t length) {
            if (!ec && length) {
                io_buffer event{read_buffer_.data(), read_buffer_.data() + length};
                manager()->on_read(std::move(event), shared_from_this());
            } else {
                handle_error(ec);
            }
        });
}

void tcp_client_stream::handle_error(const net::error_code& ec) 
{
    manager()->on_error(ec, shared_from_this());
}

void tcp_client_stream::do_set_host(std::string host) { host_.swap(host); }

void tcp_client_stream::do_set_service(std::string service) { port_.swap(service); }
