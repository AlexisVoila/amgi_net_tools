#ifndef HTTP_STATE_H
#define HTTP_STATE_H

#include "transport/io_event.h"

#include <boost/asio.hpp>

#include <memory>

namespace sys = boost::system;

class http_session;

class http_state 
{
public:
    virtual void handle_server_read(http_session *session, io_event& event);
    virtual void handle_client_read(http_session *session, io_event& event);
    virtual void handle_client_connect(http_session *session, io_event& event);
    virtual void handle_server_write(http_session *session, io_event& event);
    virtual void handle_client_write(http_session *session, io_event& event);
    virtual void handle_server_error(http_session* session, sys::error_code ec);
    virtual void handle_client_error(http_session* session, sys::error_code ec);
};

class http_wait_request final : public http_state 
{
public:
    static auto instance() { return std::make_unique<http_wait_request>(); }
    void handle_server_read(http_session *session, io_event& event) override;
};

class http_connection_established final : public http_state 
{
public:
    static auto instance() { return std::make_unique<http_connection_established>(); }
    void handle_client_connect(http_session *session, io_event &event) override;
};

class http_ready_to_transfer_data final : public http_state 
{
public:
    static auto instance() { return std::make_unique<http_ready_to_transfer_data>(); }
    void handle_client_write(http_session* session, io_event& event) override;
    void handle_server_write(http_session* session, io_event& event) override;
};

class http_data_transfer_mode final : public http_state 
{
public:
    static auto instance() { return std::make_unique<http_data_transfer_mode>(); }
    void handle_server_read(http_session *session, io_event& event) override;
    void handle_server_write(http_session *session, io_event& event) override;
    void handle_client_read(http_session *session, io_event& event) override;
    void handle_client_write(http_session *session, io_event& event) override;
};

#endif //HTTP_STATE_H
