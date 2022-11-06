#include "http.h"
#include "http_state.h"
#include "http_session.h"
#include "logger/logger.h"
#include "transport/stream_manager.h"

#include <boost/format.hpp>

const std::string g_httpError500 =
    "HTTP/1.1 500 Internal Server Error\r\n"
    "Connection : Closed\r\n"
    "\r\n";

const std::string g_httpDone = 
    "HTTP/1.1 200 OK\r\n"
    "\r\n";

using fmt = boost::format;
using logger = logging::logger;

void http_state::handle_server_read(http_session* session, io_event& event) {}
void http_state::handle_client_read(http_session* session, io_event& event) {}
void http_state::handle_client_connect(http_session* session, io_event& event) {}
void http_state::handle_server_write(http_session* session, io_event& event) {}
void http_state::handle_client_write(http_session* session, io_event& event) {}

void http_state::handle_server_error(http_session* session, sys::error_code ec) 
{
    const auto error = ec.value();
    const auto msg = ec.message();

    if (ec) {
        if (!(error == net::error::eof ||
              error == net::error::connection_aborted ||
              error == net::error::connection_refused ||
              error == net::error::connection_reset ||
              error == net::error::timed_out ||
              error == net::error::operation_aborted ||
              error == net::error::bad_descriptor)) {
            logger::warning((fmt("[%1%] server side session error: %2%") % session->context().id % msg).str());
        }
    }

    session->manager()->stop(session->context().id);
}

void http_state::handle_client_error(http_session* session, sys::error_code ec) 
{
    const auto error = ec.value();
    const auto msg = ec.message();

    if (ec) {
        if (!(error == net::error::eof ||
            error == net::error::connection_aborted ||
            error == net::error::connection_refused ||
            error == net::error::connection_reset ||
            error == net::error::timed_out ||
            error == net::error::operation_aborted ||
            error == net::error::bad_descriptor)) {
            logger::warning((fmt("[%1%] client side session error: %2%") % session->context().id % msg).str());
        }
    } 

    session->manager()->stop(session->context().id);
}

void http_wait_request::handle_server_read(http_session* session, io_event& event)
{
    auto& ctx = session->context();

    const auto* req_str = reinterpret_cast<const char*>(event.data());

    const std::string sv{req_str, event.size()};

    auto http_req = http::get_headers(std::string_view{req_str, event.size()});
    auto host = http_req.get_host();
    auto service = http_req.get_service();

    if (host.empty()) {
        logger::warning((fmt("[%1%] http protocol: bad request packet") % ctx.id).str());
        io_event new_event{g_httpError500.begin(), g_httpError500.end()};
        session->manager()->write_server(ctx.id, new_event);
        session->manager()->stop(ctx.id);
        return;
    }

    if (service.empty()) {
        logger::warning((fmt("[%1%] http protocol: bad remote address format") % ctx.id).str());
        io_event new_event{g_httpError500.begin(), g_httpError500.end()};
        session->manager()->write_server(ctx.id, new_event);
        session->manager()->stop(ctx.id);
        return;
    }

    if (http_req.method == http::kConnect) {
        ctx.response.resize(g_httpDone.size());
        std::copy(std::cbegin(g_httpDone), std::cend(g_httpDone), std::begin(ctx.response));
    } else {
        ctx.response.resize(event.size());
        std::copy(std::cbegin(event), std::cend(event), std::begin(ctx.response));
    }

    ctx.host = host;
    ctx.service = service;

    logger::info((fmt("[%1%] requested [%2%:%3%]") % ctx.id % host % service).str());
    session->manager()->connect(ctx.id, std::move(host), std::move(service));
    session->change_state(http_connection_established::instance());
}

void http_connection_established::handle_client_connect(http_session* session, io_event& event)
{
    auto& ctx = session->context();
    io_event new_event{ctx.response};
    if (ctx.response.size() == g_httpDone.size()) {
        session->manager()->write_server(ctx.id, std::move(new_event));
    } else {
        session->manager()->write_client(ctx.id, std::move(new_event));
    }
    session->change_state(http_ready_to_transfer_data::instance());
}

void http_ready_to_transfer_data::handle_client_write(http_session* session, io_event& event)
{
    auto& ctx{session->context()};
    session->manager()->read_server(ctx.id);
    session->manager()->read_client(ctx.id);
    session->change_state(http_data_transfer_mode::instance());
}

void http_ready_to_transfer_data::handle_server_write(http_session* session, io_event& event)
{
    auto& ctx{session->context()};
    session->manager()->read_server(ctx.id);
    session->manager()->read_client(ctx.id);
    session->change_state(http_data_transfer_mode::instance());
}


void http_data_transfer_mode::handle_server_write(http_session* session, io_event& event)
{
    auto& ctx{session->context()};
    session->manager()->read_client(ctx.id);
}

void http_data_transfer_mode::handle_server_read(http_session* session, io_event& event)
{
    auto& ctx{session->context()};
    ctx.transferred_bytes_to_remote += event.size();
    session->manager()->write_client(ctx.id, std::move(event));
}

void http_data_transfer_mode::handle_client_write(http_session* session, io_event& event)
{
    auto& ctx{session->context()};
    session->manager()->read_server(ctx.id);
}

void http_data_transfer_mode::handle_client_read(http_session* session, io_event& event)
{
    auto& ctx{session->context()};
    ctx.transferred_bytes_to_local += event.size();
    session->manager()->write_server(ctx.id, std::move(event));
}
