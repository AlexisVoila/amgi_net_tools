#include "server.hpp"

#include <boost/program_options.hpp>

#include <iostream>
#include <charconv>

namespace po = boost::program_options;

namespace
{
    struct server_conf 
    {
        std::string listen_port;
        std::string target_port;
        std::string target_host;
        server::tls_options tls_options;
    };

    server_conf parse_command_line_arguments(int argc, char* argv[])
    {
        po::options_description all("Allowed options");

        server_conf conf;
        po::options_description general("General options");
        general.add_options()
            ("listen-port,l", po::value<std::string>(&conf.listen_port)->default_value("2080")->required(), "tls forwarder listen port number")
            ("target-port,t", po::value<std::string>(&conf.target_port)->default_value("8443")->required(), "tls forwarder target port number")
            ("target-host,d", po::value<std::string>(&conf.target_host)->default_value("127.0.0.1")->required(), "tls forwarder target host")
            ("help,h", "show help message");

        po::options_description tls("Tls tunnel options");
        tls.add_options()
            ("private-key,p", po::value<std::string>(&conf.tls_options.private_key)->default_value(""), "private key file path (pem format)")
            ("client-cert,s", po::value<std::string>(&conf.tls_options.client_cert)->default_value(""), "client certificate file path (pem format)")
            ("ca-cert,c", po::value<std::string>(&conf.tls_options.ca_cert)->default_value(""), "CA certificate file path (pem format)");

        all.add(general).add(tls);

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, all), vm);
        if (vm.count("help")) {
            std::cout << all << "\n";
            exit(EXIT_SUCCESS);
        }

        try {
            po::notify(vm);
        }
        catch (const po::required_option& e) {
            std::cout << "Error: " << e.what() << "\n";
            exit(EXIT_FAILURE);
        }

        if (conf.tls_options.private_key.empty()) {
            std::cout << "The <private-key> parameter must be specified\n";
            exit(EXIT_FAILURE);
        }

        if (conf.tls_options.client_cert.empty()) {
            std::cout << "The <server-cert> parameter must be specified\n";
            exit(EXIT_FAILURE);
        }

        if (conf.tls_options.ca_cert.empty()) {
            std::cout << "The <ca-cert> parameter must be specified\n";
            exit(EXIT_FAILURE);
        }

        return conf;
    }
}

int main(int argc, char* argv[])
{
    const auto conf = parse_command_line_arguments(argc, argv);

    std::locale::global(std::locale(""));

    try {
        server srv(conf.listen_port, conf.target_host, conf.target_port, conf.tls_options);
        srv.run();
    }
    catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }

    return 0;
}
