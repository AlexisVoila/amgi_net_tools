#ifndef SOCKS5_PROXY_SERVER_STREAM_H
#define SOCKS5_PROXY_SERVER_STREAM_H


#include "stream.h"
#include <boost/asio/executor.hpp>

namespace net = boost::asio;

class server_stream : public stream, public std::enable_shared_from_this<server_stream> {
public:
    server_stream(const stream_manager_ptr& smp, int id) : stream(smp, id) {}
    virtual net::io_context& context() = 0;
};

using server_stream_ptr = std::shared_ptr<server_stream>;


#endif //SOCKS5_PROXY_SERVER_STREAM_H
