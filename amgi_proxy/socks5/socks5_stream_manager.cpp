#include "socks5_stream_manager.h"
#include "transport/tcp_client_stream.h"
#include "logger/logger.h"

#include <boost/format.hpp>

using fmt = boost::format;
using logger = logging::logger;

void socks5_stream_manager::stop(stream_ptr stream)
{
    stop(stream->id());
}

void socks5_stream_manager::stop(int id)
{
    if (auto it = sessions_.find(id); it != sessions_.end()) {
        it->second.client->stop();
        it->second.server->stop();

        if (const auto itt = states_.find(id); itt != states_.end()) {
            logger::info((fmt("[%1%] session closed: [%2%:%3%] rx_bytes: %4%, tx_bytes: %5%, live sessions %6% ")
                          % id
                          % itt->second.context().host
                          % itt->second.context().service
                          % itt->second.context().transferred_bytes_to_local
                          % itt->second.context().transferred_bytes_to_remote
                          % states_.size()).str());

            states_.erase(itt);
        }

        sessions_.erase(it);
    }
}

void socks5_stream_manager::on_close(stream_ptr stream)
{
    stop(stream);
}

void socks5_stream_manager::on_error(net::error_code ec, server_stream_ptr stream)
{
    if (auto it = states_.find(stream->id()); it != states_.end())
        it->second.handle_server_error(ec);
}

void socks5_stream_manager::on_error(net::error_code ec, client_stream_ptr stream)
{
    if (auto it = states_.find(stream->id()); it != states_.end())
        it->second.handle_client_error(ec);
}

void socks5_stream_manager::on_accept(server_stream_ptr upstream)
{
    upstream->start();
    const auto id{upstream->id()};
    logger::trace((fmt("[%1%] session created") % id).str());

    auto downstream = std::make_shared<tcp_client_stream>(shared_from_this(), id, upstream->context());

    socks_pair pair{id, std::move(upstream), std::move(downstream)};
    sessions_.insert({id, std::move(pair)});

    socks5_session session{id, shared_from_this()};
    states_.insert({id, std::move(session)});
}

void socks5_stream_manager::on_read(io_event event, server_stream_ptr stream)
{
    if (auto it = states_.find(stream->id()); it != states_.end())
        it->second.handle_server_read(event);
}

void socks5_stream_manager::on_write(io_event event, server_stream_ptr stream)
{
    if (auto it = states_.find(stream->id()); it != states_.end())
        it->second.handle_server_write(event);
}

void socks5_stream_manager::read_server(int id)
{
    if (auto it = sessions_.find(id); it != sessions_.end())
        it->second.server->read();
}

void socks5_stream_manager::write_server(int id, io_event event)
{
    if (auto it = sessions_.find(id); it != sessions_.end())
        it->second.server->write(std::move(event));
}

void socks5_stream_manager::on_read(io_event event, client_stream_ptr stream)
{
    if (auto it = states_.find(stream->id()); it != states_.end())
        it->second.handle_client_read(event);
}

void socks5_stream_manager::on_write(io_event event, client_stream_ptr stream)
{
    if (auto it = states_.find(stream->id()); it != states_.end())
        it->second.handle_client_write(event);
}

void socks5_stream_manager::on_connect(io_event event, client_stream_ptr stream)
{
    if (auto it = states_.find(stream->id()); it != states_.end())
        it->second.handle_client_connect(event);
}

void socks5_stream_manager::read_client(int id)
{
    if (auto it = sessions_.find(id); it != sessions_.end())
        it->second.client->read();
}

void socks5_stream_manager::write_client(int id, io_event event)
{
    if (auto it = sessions_.find(id); it != sessions_.end())
        it->second.client->write(std::move(event));
}

void socks5_stream_manager::connect(int id, std::string host, std::string service)
{
    if (auto it = sessions_.find(id); it != sessions_.end()) {
        it->second.client->set_host(std::move(host));
        it->second.client->set_service(std::move(service));
        it->second.client->start();
    }
}

