#include "http.h"
#include "http_state.h"
#include "http_session.h"
#include "logger/logger.h"
#include "transport/stream_manager.h"

#include <boost/format.hpp>

using fmt = boost::format;
using logger = logging::logger;

namespace 
{
    const std::string kHttpError500 =
        "HTTP/1.1 500 Internal Server Error\r\n"
        "Connection : Closed\r\n"
        "\r\n";

    const std::string kHttpDone = 
        "HTTP/1.1 200 OK\r\n"
        "\r\n";

    void log_error(http_session* session, net::error_code ec, std::string_view participant)
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
                logger::warning((fmt("[%1%] %2% side session error: %3%")
                                 % session->id()
                                 % participant
                                 % msg).str());
            }
        }
    }
}

void http_state::handle_server_read(http_session* session, io_buffer buffer) {}
void http_state::handle_client_read(http_session* session, io_buffer buffer) {}
void http_state::handle_client_connect(http_session* session, io_buffer buffer) {}
void http_state::handle_server_write(http_session* session, io_buffer buffer) {}
void http_state::handle_client_write(http_session* session, io_buffer buffer) {}

void http_state::handle_server_error(http_session* session, net::error_code ec) 
{
    log_error(session, ec, "server");
    session->stop();
}

void http_state::handle_client_error(http_session* session, net::error_code ec)
{
    log_error(session, ec, "client");
    session->stop();
}

void http_wait_request::handle_server_read(http_session* session, io_buffer buffer)
{
    const auto sid = session->id();
    const auto* req_str = reinterpret_cast<const char*>(buffer.data());

    auto http_req = http::get_headers(std::string_view{req_str, buffer.size()});
    const auto host = http_req.get_host();
    const auto service = http_req.get_service();

    if (host.empty()) {
        logger::warning((fmt("[%1%] http protocol: bad request packet") % sid).str());
        session->write_to_server(io_buffer{kHttpError500.begin(), kHttpError500.end()});
        session->stop();
        return;
    }

    if (service.empty()) {
        logger::warning((fmt("[%1%] http protocol: bad remote address format") % sid).str());
        session->write_to_server(io_buffer{kHttpError500.begin(), kHttpError500.end()});
        session->stop();
        return;
    }

    if (http_req.method == http::kConnect)
        session->set_response(io_buffer{kHttpDone.begin(), kHttpDone.end()});
    else
        session->set_response(std::move(buffer));

    session->set_endpoint_info(host, service);

    logger::info((fmt("[%1%] requested [%2%:%3%]") % sid % host % service).str());
    session->connect();
    session->change_state(http_connection_established::instance());
}

void http_connection_established::handle_client_connect(http_session* session, io_buffer buffer)
{
    // TODO
    const std::string resp{session->get_response().begin(), session->get_response().end()};
	if (resp == kHttpDone)
		session->write_to_server(session->get_response());
	else
		session->write_to_client(session->get_response());
	session->change_state(http_ready_to_transfer_data::instance());
}

void http_ready_to_transfer_data::handle_client_write(http_session* session, io_buffer buffer)
{
    session->read_from_server();
    session->read_from_server();
    session->change_state(http_data_transfer_mode::instance());
}

void http_ready_to_transfer_data::handle_server_write(http_session* session, io_buffer buffer)
{
    session->read_from_server();
    session->read_from_client();
    session->change_state(http_data_transfer_mode::instance());
}


void http_data_transfer_mode::handle_server_write(http_session* session, io_buffer buffer)
{
    session->read_from_client();
}

void http_data_transfer_mode::handle_server_read(http_session* session, io_buffer buffer)
{
    session->update_bytes_sent_to_remote(buffer.size());
    session->write_to_client(std::move(buffer));
}

void http_data_transfer_mode::handle_client_write(http_session* session, io_buffer buffer)
{
    session->read_from_server();
}

void http_data_transfer_mode::handle_client_read(http_session* session, io_buffer buffer)
{
    session->update_bytes_sent_to_local(buffer.size());
    session->write_to_server(std::move(buffer));
}
