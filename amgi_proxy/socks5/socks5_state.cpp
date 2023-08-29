#include "socks5.h"
#include "socks5_state.h"
#include "socks5_session.h"
#include "logger/logger.h"
#include "transport/stream_manager.h"

#include <boost/format.hpp>

using fmt = boost::format;
using logger = logging::logger;

namespace
{
    std::uint8_t get_response_error_code(net::error_code ec)
    {
        const auto errval = ec.value();

        if (errval == net::error::network_unreachable) {
            return socks5::responses::network_unreachable;
        } else if (errval == net::error::host_unreachable ||
                   errval == net::error::host_not_found ||
                   errval == net::error::no_data) {
            return socks5::responses::host_unreachable;
        } else if (errval == net::error::connection_refused ||
                   errval == net::error::connection_aborted ||
                   errval == net::error::connection_reset) {
            return socks5::responses::connection_refused;
        } else if (errval == net::error::timed_out) {
            return socks5::responses::ttl_expired;
        } else {
            return socks5::responses::general_socks_server_failure;
        }
    }
}

void socks5_state::handle_server_read(socks5_session *session, io_event& event) {}
void socks5_state::handle_client_read(socks5_session *session, io_event& event) {}
void socks5_state::handle_client_connect(socks5_session *session, io_event& event) {}
void socks5_state::handle_server_write(socks5_session *session, io_event& event) {}
void socks5_state::handle_client_write(socks5_session *session, io_event& event) {}

void socks5_state::handle_server_error(socks5_session* session, net::error_code ec)
{
    const auto& ctx = session->context();

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
            logger::warning((fmt("[%1%] server side session error: %2%") % ctx.id % msg).str());
        }
    }

    session->manager()->stop(ctx.id);
}

void socks5_state::handle_client_error(socks5_session* session, net::error_code ec)
{
    const auto& ctx = session->context();

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
            logger::warning((fmt("[%1%] client side session error: %2%") % ctx.id % msg).str());
        }
    }

    session->manager()->stop(ctx.id);
}

void socks5_auth_request::handle_server_read(socks5_session *session, io_event &event) {
    auto& ctx = session->context();
    const auto error = socks5::is_socks5_auth_request(event.data(), event.size());

    ctx.response.resize(2);
    ctx.response[0] = socks5::proto::version;
    ctx.response[1] = error ? proto::auth::kNotSupported : proto::auth::kNoAuth;
    if (ctx.response[1] == proto::auth::kNotSupported)
        logger::warning((fmt("[%1%] %2%") % ctx.id % *error).str());

    io_event new_event{ctx.response};
    session->manager()->write_server(ctx.id, std::move(new_event));
    session->change_state(socks5_connection_request::instance());
}

void socks5_connection_request::handle_server_write(socks5_session *session, io_event &event) {
    session->manager()->read_server(session->context().id);
}

void socks5_connection_request::handle_server_read(socks5_session *session, io_event &event) {
    auto &ctx = session->context();

    std::string host, service;
    if (!socks5::is_valid_request_packet(event.data(), event.size())) {
        logger::warning((fmt("[%1%] socks5 protocol: bad request packet") % ctx.id).str());
        session->manager()->stop(ctx.id);
        return;
    }

    if (!socks5::get_remote_address_info(event.data(), event.size(), host, service)) {
        logger::warning((fmt("[%1%] socks5 protocol: bad remote address format") % ctx.id).str());
        session->manager()->stop(ctx.id);
        return;
    }

    ctx.host = host;
    ctx.service = service;

    logger::info((fmt("[%1%] requested [%2%:%3%]") % ctx.id % host % service).str());
    ctx.response.resize(event.size());
    std::copy(std::cbegin(event), std::cend(event), std::begin(ctx.response));
    session->manager()->connect(ctx.id, std::move(host), std::move(service));
    session->change_state(socks5_connection_established::instance());
}

void socks5_connection_established::handle_client_connect(socks5_session *session, io_event &event) {
    auto& ctx = session->context();
    (reinterpret_cast<socks5::request_header*>(ctx.response.data()))->command = socks5::responses::succeeded;
    io_event new_event{ctx.response};
    session->manager()->write_server(ctx.id, std::move(new_event));
    session->change_state(socks5_ready_to_transfer_data::instance());
}

void socks5_connection_established::handle_client_error(socks5_session* session, net::error_code ec)
{
    auto& ctx = session->context();
    ctx.response[1] = get_response_error_code(ec);
    (reinterpret_cast<socks5::request_header*>(ctx.response.data()))->command = 0x00;
    io_event new_event{ctx.response};
    session->manager()->write_server(ctx.id, std::move(new_event));
    logger::warning((fmt("[%1%] client side session error: %2%") % ctx.id % ec.message()).str());
}

void socks5_connection_established::handle_server_write(socks5_session* session, io_event& event)
{
    session->manager()->stop(session->context().id);
}

void socks5_ready_to_transfer_data::handle_server_write(socks5_session *session, io_event &event) {
    auto& ctx{session->context()};
    session->manager()->read_server(ctx.id);
    session->manager()->read_client(ctx.id);
    session->change_state(socks5_data_transfer_mode::instance());
}

void socks5_data_transfer_mode::handle_server_write(socks5_session *session, io_event &event) {
    auto& ctx{session->context()};
    session->manager()->read_client(ctx.id);
}

void socks5_data_transfer_mode::handle_server_read(socks5_session *session, io_event &event) {
    auto& ctx{session->context()};
    ctx.transferred_bytes_to_remote += event.size();
    session->manager()->write_client(ctx.id, std::move(event));
}

void socks5_data_transfer_mode::handle_client_write(socks5_session *session, io_event &event) {
    auto& ctx{session->context()};
    session->manager()->read_server(ctx.id);
}

void socks5_data_transfer_mode::handle_client_read(socks5_session *session, io_event &event) {
    auto& ctx{session->context()};
    ctx.transferred_bytes_to_local += event.size();
    session->manager()->write_server(ctx.id, std::move(event));
}
