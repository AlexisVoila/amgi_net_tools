#ifndef HTTP_STREAM_MANAGER_H
#define HTTP_STREAM_MANAGER_H

#include "transport/stream_manager.h"
#include "http_session.h"


class http_stream_manager final
    : public stream_manager
    , public std::enable_shared_from_this<http_stream_manager> 
{
public:
    http_stream_manager() = default;
    ~http_stream_manager() = default;

    http_stream_manager(const http_stream_manager& other) = delete;
    http_stream_manager& operator=(const http_stream_manager& other) = delete;

    void stop(stream_ptr stream) override;
    void stop(int id) override;
    void on_close(stream_ptr stream) override;
    void on_error(net::error_code ec, server_stream_ptr stream) override;
    void on_error(net::error_code ec, client_stream_ptr stream) override;

    void on_accept(server_stream_ptr stream) override;
    void on_read(io_event event, server_stream_ptr stream) override;
    void on_write(io_event event, server_stream_ptr stream) override;
    void read_server(int id) override;
    void write_server(int id, io_event event) override;

    void on_connect(io_event event, client_stream_ptr stream) override;
    void on_read(io_event event, client_stream_ptr stream) override;
    void on_write(io_event event, client_stream_ptr stream) override;
    void read_client(int id) override;
    void write_client(int id, io_event event) override;
    void connect(int id, std::string host, std::string service) override;

private:
    struct http_pair {
        int id;
        server_stream_ptr server;
        client_stream_ptr client;
    };

    std::unordered_map<int, http_pair> sessions_;
    std::unordered_map<int, http_session> states_;
};

using http_stream_manager_ptr = std::shared_ptr<http_stream_manager>;


#endif // HTTP_STREAM_MANAGER_H
