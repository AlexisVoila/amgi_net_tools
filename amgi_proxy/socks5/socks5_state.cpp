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

        if (errval == net::error::network_unreachable)
            return socks5::responses::network_unreachable;

        if (errval == net::error::host_unreachable ||
                   errval == net::error::host_not_found ||
                   errval == net::error::no_data)
            return socks5::responses::host_unreachable;

        if (errval == net::error::connection_refused ||
                   errval == net::error::connection_aborted ||
                   errval == net::error::connection_reset)
            return socks5::responses::connection_refused;

        if (errval == net::error::timed_out)
            return socks5::responses::ttl_expired;

        return socks5::responses::general_socks_server_failure;
    }
}

void socks5_state::handle_server_read(socks5_session *session, io_buffer event) {}
void socks5_state::handle_client_read(socks5_session *session, io_buffer event) {}
void socks5_state::handle_client_connect(socks5_session *session, io_buffer event) {}
void socks5_state::handle_server_write(socks5_session *session, io_buffer event) {}
void socks5_state::handle_client_write(socks5_session *session, io_buffer event) {}

void socks5_state::handle_server_error(socks5_session* session, net::error_code ec)
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
            logger::warning((fmt("[%1%] server side session error: %2%") % session->id() % msg).str());
        }
    }

    session->manager()->stop(session->id());
}

void socks5_state::handle_client_error(socks5_session* session, net::error_code ec)
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
            logger::warning((fmt("[%1%] client side session error: %2%") % session->id() % msg).str());
        }
    }

    session->manager()->stop(session->id());
}

void socks5_auth_request::handle_server_read(socks5_session *session, io_buffer event) {
    const auto error = socks5::is_socks5_auth_request(event.data(), event.size());
    const auto auth_mode = error ? proto::auth::kNotSupported : proto::auth::kNoAuth;

    session->set_response(socks5::proto::version, auth_mode);
    if (auth_mode == proto::auth::kNotSupported)
        logger::warning((fmt("[%1%] %2%") % session->id() % error.value_or("")).str());

    io_buffer new_event{session->response()};
    session->manager()->write_server(session->id(), std::move(new_event));
    session->change_state(socks5_connection_request::instance());
}

void socks5_connection_request::handle_server_write(socks5_session *session, io_buffer event) {
    session->manager()->read_server(session->id());
}

void socks5_connection_request::handle_server_read(socks5_session *session, io_buffer event) {
    const auto sid = session->id();

    std::string host, service;
    if (!socks5::is_valid_request_packet(event.data(), event.size())) {
        logger::warning((fmt("[%1%] socks5 protocol: bad request packet") % sid).str());
        session->manager()->stop(sid);
        return;
    }

    if (!socks5::get_remote_address_info(event.data(), event.size(), host, service)) {
        logger::warning((fmt("[%1%] socks5 protocol: bad remote address format") % sid).str());
        session->manager()->stop(sid);
        return;
    }

    session->set_endpoint_info(host, service);

    logger::info((fmt("[%1%] requested [%2%:%3%]") % sid % host % service).str());
    session->set_response(std::move(event));
    session->manager()->connect(sid, std::move(host), std::move(service));
    session->change_state(socks5_connection_established::instance());
}

void socks5_connection_established::handle_client_connect(socks5_session *session, io_buffer event) {
    session->set_response_error_code(socks5::responses::succeeded);
    io_buffer new_event{session->response()};
    session->manager()->write_server(session->id(), std::move(new_event));
    session->change_state(socks5_ready_to_transfer_data::instance());
}

void socks5_connection_established::handle_client_error(socks5_session* session, net::error_code ec)
{
    session->set_response_error_code(get_response_error_code(ec));
    io_buffer new_event{session->response()};
    session->manager()->write_server(session->id(), std::move(new_event));
    logger::warning((fmt("[%1%] client side session error: %2%") % session->id() % ec.message()).str());
}

void socks5_connection_established::handle_server_write(socks5_session* session, io_buffer event)
{
    session->manager()->stop(session->id());
}

void socks5_ready_to_transfer_data::handle_server_write(socks5_session *session, io_buffer event) {
    session->manager()->read_server(session->id());
    session->manager()->read_client(session->id());
    session->change_state(socks5_data_transfer_mode::instance());
}

void socks5_data_transfer_mode::handle_server_write(socks5_session *session, io_buffer event) {
    session->manager()->read_client(session->id());
}

void socks5_data_transfer_mode::handle_server_read(socks5_session *session, io_buffer event) {
    session->update_bytes_sent_to_remote(event.size());
    session->manager()->write_client(session->id(), std::move(event));
}

void socks5_data_transfer_mode::handle_client_write(socks5_session *session, io_buffer event) {
    session->manager()->read_server(session->id());
}

void socks5_data_transfer_mode::handle_client_read(socks5_session *session, io_buffer event) {
    session->update_bytes_sent_to_local(event.size());
    session->manager()->write_server(session->id(), std::move(event));
}
