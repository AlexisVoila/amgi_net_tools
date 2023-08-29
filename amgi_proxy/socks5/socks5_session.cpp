#include "socks5_session.h"

#include <utility>

socks5_session::socks5_session(int id, stream_manager_ptr mgr)
    : context_{id}, manager_{std::move(mgr)} 
{
    state_ = socks5_auth_request::instance();
}

void socks5_session::change_state(std::unique_ptr<socks5_state> state) 
{
    state_ = move(state);
}

void socks5_session::handle_server_read(io_event &event) 
{
    state_->handle_server_read(this, event);
}

void socks5_session::handle_client_read(io_event &event) 
{
    state_->handle_client_read(this, event);
}

void socks5_session::handle_server_write(io_event &event) 
{
    state_->handle_server_write(this, event);
}

void socks5_session::handle_client_write(io_event &event) 
{
    state_->handle_client_write(this, event);
}

void socks5_session::handle_client_connect(io_event &event) 
{
    state_->handle_client_connect(this, event);
}

void socks5_session::handle_server_error(net::error_code ec)
{
    state_->handle_server_error(this, ec);
}

void socks5_session::handle_client_error(net::error_code ec)
{
    state_->handle_client_error(this, ec);
}

stream_manager_ptr socks5_session::manager() 
{
    return manager_;
}


