#ifndef SESSION_H
#define SESSION_H

#include "manager.hpp"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <iostream>
#include <array>

namespace net = boost::asio;
namespace sys = boost::system;
using tcp = boost::asio::ip::tcp;

using std::placeholders::_1;
using std::placeholders::_2;

static size_t g_scount = 0;

class session 
    : public session_base
    , public std::enable_shared_from_this<session>
{
    enum { max_buff_size = 0x4000 };
    using buffer_type = std::array<std::uint8_t, max_buff_size>;

    tcp::resolver resolver_;
    tcp::socket local_sock_;
    net::ssl::stream<tcp::socket> remote_sock_;
    session_manager& manager_;

    std::string remote_host_;
    std::string remote_service_;
    std::string remote_ep_;
    std::string remote_resolved_ep_;

    std::string client_ep_;

    buffer_type local_buffer_{};
    buffer_type remote_buffer_{};

    session(net::io_context& ios, 
            net::ssl::context& ctx, 
            session_manager& mgr, 
            std::string_view remote_host, 
            std::string_view remote_service)
        : resolver_{ios}
        , local_sock_{ios}
        , remote_sock_{ios, ctx}
        , manager_{mgr}
        , remote_host_{remote_host}
        , remote_service_{remote_service} 
    {
        remote_ep_ = remote_host_ + ':' + remote_service_;
        //remote_sock_.set_verify_mode(net::ssl::verify_none);
        remote_sock_.set_verify_mode(net::ssl::verify_peer);
        remote_sock_.set_verify_callback(std::bind(&session::verify_certificate, this, _1, _2));
    }

public:
    ~session() 
    {
        std::cout 
            << "connection from " 
            << client_ep_ << " to " << remote_ep_ << remote_resolved_ep_ 
            << " closed, sescnt: " << manager_.ses_count() << ", scnt: " << --g_scount << std::endl;
    }

    using pointer = std::shared_ptr<session>;

    tcp::socket& socket() { return local_sock_; }

    static pointer create(net::io_context& io_context, 
                          net::ssl::context& ctx,
                          session_manager& mgr,
                          std::string_view remote_host, 
                          std::string_view remote_port) 
    {
        return pointer(new session(io_context, ctx, mgr, remote_host, remote_port));
    }

    void start() 
    {
        manager_.join(shared_from_this());
        tcp::endpoint from_ep{local_sock_.remote_endpoint()};
        client_ep_ = from_ep.address().to_string() + ':' + std::to_string(from_ep.port());
        std::cout 
            << "accepted connection from " << client_ep_ 
            << " , sescnt: " << manager_.ses_count() << ", scnt: " << ++g_scount << std::endl;
        do_resolve();
    }

    void stop() override 
    {
        close();
    }

private:
    void close() 
    {
        sys::error_code ignored_ec;
        if (local_sock_.is_open()) {
            local_sock_.shutdown(net::socket_base::shutdown_both, ignored_ec);
            local_sock_.close();
        }

        if (remote_sock_.lowest_layer().is_open()) {
            sys::error_code ec;
            remote_sock_.lowest_layer().cancel(ec);
            remote_sock_.async_shutdown(
                [this, self{shared_from_this()}](const sys::error_code& ec) {
                    if (ec && ec.category() == net::error::get_ssl_category()) {
                        if (ec != net::error::operation_aborted/* && ec != net::error::bad_descriptor*/) {
                            std::string msg = ec.message();
                            std::cout << msg << " value: " << ec.value() << std::endl;
                        }
                    }
                    remote_sock_.lowest_layer().close();
                });
            net::async_write(
                remote_sock_, net::null_buffers{},
                [this, self{shared_from_this()}](const sys::error_code& ec, std::size_t trans_bytes) { 
                    if (ec && ec.category() == net::error::get_ssl_category()) {
                        if (ec != net::error::operation_aborted/* && ec != net::error::bad_descriptor*/)
                        {
                            std::string msg = ec.message();
                            std::cout << msg << " value: " << ec.value() << std::endl;
                        }
                    }
                    remote_sock_.lowest_layer().close(); 
                });
        }
     }

    void close_ssl() 
    {
        if (remote_sock_.lowest_layer().is_open()) {
            sys::error_code ec;
            remote_sock_.lowest_layer().cancel(ec);
            remote_sock_.async_shutdown(
                [this, self{shared_from_this()}](const sys::error_code& ec) {
                    if (ec) {
                        if (ec != net::error::operation_aborted && ec != net::error::bad_descriptor) {
                            std::string msg = ec.message();
                            std::cout << msg << " value: " << ec.value() << std::endl;
                        }
                    }
                    remote_sock_.lowest_layer().close();
                    if (local_sock_.is_open())
                        local_sock_.cancel();
                });
        }
    }

    bool verify_certificate(bool preverified, net::ssl::verify_context& ctx)
    {
        return preverified;
    }

    void handshake()
    {
        //std::cout << "start handshake with: " << remote_resolved_ep_ << std::endl;
        remote_sock_.async_handshake(
            net::ssl::stream_base::client,
            [this, self{shared_from_this()}](const sys::error_code& error) {
                if (!error) {
                    //std::cout << "handshake ok: " << remote_resolved_ep_ << std::endl;
                    do_read_from_local();
                    do_read_from_remote();
                } else {
                    std::cout << "Handshake failed: " << error.message() << "\n";
                    manager_.leave(shared_from_this());
                }
            });
    }

    void do_resolve() 
    {
        resolver_.async_resolve(
            remote_host_, remote_service_,
            [this, self{shared_from_this()}](const sys::error_code& ec, const tcp::resolver::results_type& eps) {
                if (!ec) {
                    do_connect(eps);
                } else {
                    std::cout << '[' << remote_ep_ << "] " << ec.message() << std::endl;
                    manager_.leave(shared_from_this());
                }
            });
    }

    void do_connect(const tcp::resolver::results_type& eps) {
        net::async_connect(
            remote_sock_.lowest_layer(), eps,
            [this, self{shared_from_this()}](const sys::error_code& ec, const tcp::endpoint& /*ep*/) {
                tcp::endpoint to_ep{remote_sock_.lowest_layer().remote_endpoint()};
                remote_resolved_ep_ = '(' + to_ep.address().to_string() + ':' + std::to_string(to_ep.port()) + ')';
                std::cout << "connection from " << client_ep_ << " to "
                    << remote_ep_ << remote_resolved_ep_ << " established\n";
                if (!ec) {
                    handshake();
                } else {
                    std::cout << ec.message() << std::endl;
                    manager_.leave(shared_from_this());
                }
            });
    }

    void do_read_from_local() {
        local_sock_.async_read_some(
            net::buffer(local_buffer_),
            [this, self{shared_from_this()}](const sys::error_code& ec, std::size_t bytes_transferred) {
                if (!ec && bytes_transferred > 0) {
                    do_write_to_remote(bytes_transferred);
                } else {
                    manager_.leave(shared_from_this());
                }
            });
    }

    void do_read_from_remote() {
        remote_sock_.async_read_some(
            net::buffer(remote_buffer_),
            [this, self{shared_from_this()}](const sys::error_code& ec, std::size_t bytes_transferred) {
                if (!ec && bytes_transferred > 0) {
                    do_write_to_local(bytes_transferred);
                } else {
                    manager_.leave(shared_from_this());
                }
            });
    }

    void do_write_to_remote(std::size_t bytes_transferred) {
        net::async_write(
            remote_sock_, net::buffer(local_buffer_.data(), bytes_transferred),
            [this, self{shared_from_this()}](const sys::error_code& ec, std::size_t bytes_transferred) {
                if (!ec && bytes_transferred > 0) {
                    do_read_from_local();
                } else {
                    manager_.leave(shared_from_this());
                }
            });
    }

    void do_write_to_local(std::size_t bytes_transferred) {
        net::async_write(
            local_sock_, net::buffer(remote_buffer_.data(), bytes_transferred),
            [this, self{shared_from_this()}](const sys::error_code& ec, std::size_t bytes_transferred) {
                if (!ec && bytes_transferred > 0) {
                    do_read_from_remote();
                } else {
                    manager_.leave(shared_from_this());
                }
            });
    }
};

#endif //SESSION_H