#ifndef SOCKS5_PROXY_STREAM_MANAGER_H
#define SOCKS5_PROXY_STREAM_MANAGER_H

#include "server_stream.h"
#include "client_stream.h"

namespace sys = boost::system;

class stream_manager 
{
public:
    // Common interface
    virtual void stop(stream_ptr ptr) = 0;
    virtual void stop(int id) = 0;
    virtual void on_close(stream_ptr stream) = 0;
    virtual void on_error(sys::error_code ec, server_stream_ptr stream) = 0;
    virtual void on_error(sys::error_code ec, client_stream_ptr stream) = 0;

    // Passive session interface
    virtual void on_accept(server_stream_ptr ptr) = 0;
    virtual void on_read(io_event event, server_stream_ptr stream) = 0;
    virtual void on_write(io_event event, server_stream_ptr stream) = 0;
    virtual void read_server(int id) = 0;
    virtual void write_server(int id, io_event event) = 0;

    // Active session interface
    virtual void on_connect(io_event event, client_stream_ptr stream) = 0;
    virtual void on_read(io_event event, client_stream_ptr stream) = 0;
    virtual void on_write(io_event event, client_stream_ptr stream) = 0;
    virtual void read_client(int id) = 0;
    virtual void write_client(int id, io_event event) = 0;
    virtual void connect(int id, std::string host, std::string service) = 0;
};

using stream_manager_ptr = std::shared_ptr<stream_manager>;


#endif //SOCKS5_PROXY_STREAM_MANAGER_H
