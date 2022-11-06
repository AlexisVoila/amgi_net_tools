#ifndef SOCKS5_PROXY_SERVER_H
#define SOCKS5_PROXY_SERVER_H

#include "stream_manager.h"

#include <boost/asio.hpp>

using tcp = boost::asio::ip::tcp;
namespace net = boost::asio;
namespace sys = boost::system;

class server {
public:
    explicit server(const std::string& port, stream_manager_ptr proxy_backend);
    virtual ~server();

    server(const server& other) = delete;
    server& operator=(const server& other) = delete;

    void run();
private:
    net::io_context ctx_;
    net::signal_set signals_;
    tcp::acceptor acceptor_;
    stream_manager_ptr stream_manager_;
    int stream_id_;

    void configure_signals();
    void async_wait_signals();

    void start_accept();
};


#endif //SOCKS5_PROXY_SERVER_H
