#include "tls_server.h"
#include "tls_server_stream.h"
#include "logger/logger.h"

#include <charconv>
#include <memory>

tls_server::tls_server(const std::string& port, tls_options settings, stream_manager_ptr proxy_backend)
    : ssl_ctx_{net::ssl::context::tlsv13_server}
    , signals_(ctx_), acceptor_(ctx_), stream_manager_{std::move(proxy_backend)}, stream_id_(0)
{
    configure_signals();
    async_wait_signals();

    ssl_ctx_.set_options(net::ssl::context::default_workarounds | 
                         net::ssl::context::no_tlsv1_1 |
                         net::ssl::context::no_tlsv1_2);

    //ssl_ctx_.set_password_callback(std::bind(&tls_server::get_password, this));
    //ssl_ctx_.use_certificate_chain_file(cert_path.data()); //"server.pem");
    //ssl_ctx_.use_certificate_file(settings.server_cert, net::ssl::context::pem);

    const auto rc = SSL_CTX_set_min_proto_version(ssl_ctx_.native_handle(), TLS1_3_VERSION);

    ssl_ctx_.use_certificate_chain_file(settings.server_cert);
    SSL_CTX_set_client_CA_list(ssl_ctx_.native_handle(), SSL_load_client_CA_file(settings.ca_cert.c_str()));
    ssl_ctx_.use_private_key_file(settings.private_key, net::ssl::context::pem);
    ssl_ctx_.load_verify_file(settings.ca_cert);
    ssl_ctx_.set_verify_mode(net::ssl::verify_peer | net::ssl::verify_fail_if_no_peer_cert);

    //ssl_ctx_.set_options(boost::asio::ssl::context::single_dh_use);
    //SSL_CTX_set_ecdh_auto(ssl_ctx_.native_handle(), 1);
    //SSL_CTX_set_options(ssl_ctx_.native_handle(), SSL_OP_NO_TICKET);
    //SSL_CTX_set_session_cache_mode(ssl_ctx_.native_handle(), SSL_SESS_CACHE_OFF);
    //SSL_CTX_set_mode(ssl_ctx_.native_handle(), SSL_MODE_AUTO_RETRY);
    //ssl_ctx_.use_tmp_dh_file("dh4096.pem");

    uint16_t listen_port{0};
    std::from_chars(port.data(), port.data() + port.size(), listen_port);

    tcp::endpoint ep{tcp::endpoint(tcp::v4(), listen_port)};
    acceptor_.open(ep.protocol());
    acceptor_.set_option(tcp::acceptor::reuse_address(true));
    acceptor_.bind(ep);
    acceptor_.listen();

    logging::logger::info("socks5-proxy tls_server starts on port: " + port);
    start_accept();
}

void tls_server::run() 
{
    ctx_.run();
}

void tls_server::configure_signals() 
{
    signals_.add(SIGINT);
    signals_.add(SIGTERM);
}

void tls_server::async_wait_signals() 
{
    signals_.async_wait(
        [this](net::error_code /*ec*/, int /*signno*/) {
            logging::logger::info("socks5-proxy tls_server stopping");
            acceptor_.close();
            ctx_.stop();
            logging::logger::info("socks5-proxy tls_server stopped");
        });
}

void tls_server::start_accept() 
{
    auto new_stream = std::make_shared<tls_server_stream>(stream_manager_, ++stream_id_, ctx_, ssl_ctx_);
    acceptor_.async_accept(
        new_stream->socket(),
        [this, new_stream](const net::error_code& ec) {
            if (!acceptor_.is_open()) {
                logging::logger::trace("tls proxy server acceptor is closed");
                if (ec) {
                    std::string serr{"tls proxy server error: " + ec.message()};
                    logging::logger::trace(serr);
                }

                return;
            }

            if (!ec)
                stream_manager_->on_accept(new_stream);

            start_accept();
        });
}

tls_server::~tls_server() 
{
    logging::logger::trace("tls proxy server stopped");
}

