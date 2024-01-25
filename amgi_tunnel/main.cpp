#include "server.hpp"

#include <cli_parser.h>
#include <iostream>

namespace
{
    struct server_conf 
    {
        std::string listen_port;
        std::string target_port;
        std::string target_host;
        server::tls_options tls_options;
    };

    server_conf parse_command_line_arguments_new(int argc, char* argv[])
    {
        server_conf srv_conf{};

        using namespace cli;

        cli::ParamParser argParser;

        argParser
            .add_parameter(Param("h,help").flag().description("Show help message"))
            .add_parameter(Param("l,listen-port").required().default_value("2080").description("tls forwarder listen port number"))
            .add_parameter(Param("t,target-port").required().default_value("8443").description("tls forwarder target port number"))
            .add_parameter(Param("d,target-host").required().default_value("127.0.0.1").description("tls forwarder target host"))
            .add_parameter(Param("p,private-key").required().description("private key file path (pem format)"))
            .add_parameter(Param("s,client-cert").required().description("client certificate file path (pem format)"))
            .add_parameter(Param("c,ca-cert").required().description("CA certificate file path (pem format)"));

        if (const auto msg = argParser.parse(argc, argv)) {
            std::cout << *msg << std::endl;
            argParser.print_help();
        } else {
            srv_conf.tls_options.private_key = argParser.arg("p").get_value_as_str();
            srv_conf.tls_options.client_cert = argParser.arg("s").get_value_as_str();
            srv_conf.tls_options.ca_cert = argParser.arg("c").get_value_as_str();
            srv_conf.listen_port = argParser.arg("l").get_value_as_str();
            srv_conf.target_host = argParser.arg("d").get_value_as_str();
            srv_conf.target_port = argParser.arg("t").get_value_as_str();
        }

        return srv_conf;
    }
}

int main(int argc, char* argv[])
{
    const auto conf = parse_command_line_arguments_new(argc, argv);

    std::locale::global(std::locale(""));

    try {
        server srv(conf.listen_port, conf.target_host, conf.target_port, conf.tls_options);
        srv.run();
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }

    return 0;
}
