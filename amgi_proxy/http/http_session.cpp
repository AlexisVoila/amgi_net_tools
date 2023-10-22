#include "http_session.h"

#include <utility>

http_session::http_session(int id, stream_manager_ptr mgr)
    : context_{id}, manager_{std::move(mgr)}
{
    state_ = http_wait_request::instance();
}

void http_session::change_state(std::unique_ptr<http_state> state)
{
    state_ = std::move(state);
}

void http_session::handle_server_read(io_buffer &event)
{
    state_->handle_server_read(this, event);
}

void http_session::handle_client_read(io_buffer &event)
{
    state_->handle_client_read(this, event);
}

void http_session::handle_server_write(io_buffer &event)
{
    state_->handle_server_write(this, event);
}

void http_session::handle_client_write(io_buffer &event)
{
    state_->handle_client_write(this, event);
}

void http_session::handle_client_connect(io_buffer &event)
{
    state_->handle_client_connect(this, event);
}

void http_session::handle_server_error(net::error_code ec)
{
    state_->handle_server_error(this, ec);
}

void http_session::handle_client_error(net::error_code ec)
{
    state_->handle_client_error(this, ec);
}

stream_manager_ptr http_session::manager() 
{
    return manager_;
}


