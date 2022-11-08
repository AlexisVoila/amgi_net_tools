#ifndef SERVER_H
#define SERVER_H

#include "session.hpp"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <charconv>

namespace net = boost::asio;
namespace sys = boost::system;
using tcp = boost::asio::ip::tcp;

class server 
{
public:
    struct tls_options {
        std::string private_key;
        std::string client_cert;
        std::string ca_cert;
    };

    server(std::string_view listen_port, std::string_view target_host, std::string_view target_service, tls_options settings)
        : signals_(ioc_)
        , acceptor_{ioc_}
        , remote_host_(target_host)
        , remote_service_(target_service)
        , ssl_ctx_{net::ssl::context::tlsv13_client}
    {
        configure_signals();
        start_wait_signals();
        configure_tls(settings);

        uint16_t port{ 0 };
        std::from_chars(listen_port.data(), listen_port.data() + listen_port.size(), port);

        tcp::endpoint ep{tcp::endpoint(tcp::v4(), port)};
        acceptor_.open(ep.protocol());
        acceptor_.set_option(tcp::acceptor::reuse_address(true));
        acceptor_.bind(ep);
        acceptor_.listen();


        start_accept();
    }

    void start_accept() 
    {
        auto new_session = session::create(ioc_, ssl_ctx_, manager_, remote_host_, remote_service_);

        acceptor_.async_accept(
            new_session->socket(),
            [this, new_session](const sys::error_code& ec) {
                if (!ec) {
                    new_session->start();
                } else {
                    std::cout << ec.message() << std::endl;
                }

                start_accept();
            });
    }

    void run()
    {
        ioc_.run();
    }

private:
    void configure_signals() 
    {
        signals_.add(SIGINT);
        signals_.add(SIGTERM);
    }

    void configure_tls(tls_options settings)
    {
        ssl_ctx_.set_options(
            net::ssl::context::default_workarounds | 
            net::ssl::context::no_tlsv1_1 | 
            net::ssl::context::no_tlsv1_2);

        ssl_ctx_.load_verify_file(settings.ca_cert);
        ssl_ctx_.use_private_key_file(settings.private_key, net::ssl::context::pem);
        ssl_ctx_.use_certificate_file(settings.client_cert, net::ssl::context::pem);
    }

    void start_wait_signals() 
    {
        signals_.async_wait(
            [this](const sys::error_code& ec, int /*signo*/) {
                if (ec)
                    std::cout << ec.message() << std::endl;

                acceptor_.close();
                ioc_.stop();
            });
    }

    net::io_context ioc_;
    net::signal_set signals_;
    tcp::acceptor acceptor_;
    session_manager manager_;
    std::string remote_host_;
    std::string remote_service_;
    net::ssl::context ssl_ctx_;
};


#endif //SERVER_H