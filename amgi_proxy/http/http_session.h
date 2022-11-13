#ifndef HTTP_SESSION_H
#define HTTP_SESSION_H

#include "http_state.h"

#include <string>

class stream_manager;
using stream_manager_ptr = std::shared_ptr<stream_manager>;

class http_session 
{
    struct socks_ctx {
        int id;
        std::vector<uint8_t> response;
        std::string host;
        std::string service;
        std::size_t transferred_bytes_to_remote;
        std::size_t transferred_bytes_to_local;
    };

public:
    http_session(int id, stream_manager_ptr manager);
    void change_state(std::unique_ptr<http_state> state);
    void handle_server_read(io_event& event);
    void handle_client_read(io_event& event);
    void handle_server_write(io_event& event);
    void handle_client_write(io_event& event);
    void handle_client_connect(io_event& event);
    void handle_server_error(sys::error_code ec);
    void handle_client_error(sys::error_code ec);

    auto& context() { return context_; }

   stream_manager_ptr manager();

private:
    socks_ctx context_;
    std::unique_ptr<http_state> state_;
    stream_manager_ptr manager_;
};

#endif //HTTP_SESSION_H
