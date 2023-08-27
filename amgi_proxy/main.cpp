#include "transport/server.h"
#include "transport/tls/tls_server.h"
#include "socks5/socks5_stream_manager.h"
#include "http/http_stream_manager.h"
#include "logger/logger.h"

#include <boost/program_options.hpp>

namespace po = boost::program_options;

namespace 
{
    using logger = logging::logger;
    struct server_conf
    {
        std::string listen_port;
        std::string proxy_backend;
        std::string log_file_path;
        logger::level log_level;
        tls_server::tls_options tls_options;
    };

    logger::level parse_log_level(const std::string level_str) 
    {
        logger::level level;

        if (level_str == "debug")
            level = logger::level::debug;
        else if (level_str == "trace")
            level = logger::level::trace;
        else if (level_str == "info")
            level = logger::level::info;
        else if (level_str == "warning")
            level = logger::level::warning;
        else if (level_str == "error")
            level = logger::level::error;
        else if (level_str == "fatal")
            level = logger::level::fatal;
        else {
            std::cout << "Error: log_level value must be one of [debug|trace|info|warning|error|fatal]\n";
            exit(EXIT_FAILURE);
        }

        return level;
    }

    server_conf parse_command_line_arguments(int argc, char* argv[]) 
    {
        po::options_description all("Allowed options");

        server_conf conf;
        po::options_description general("General options");
        general.add_options()
            ("port,p", po::value<std::string>(&conf.listen_port)->default_value("8443")->required(), "socks5 server listen port number")
            ("mode,m", po::value<std::string>(&conf.proxy_backend)->default_value("http"), "proxy mode [http|socks5]")
            ("log_level,v", po::value<std::string>()->default_value("info"), "verbosity level of log messages [debug|trace|info|warning|error|fatal]")
            ("log_file,l", po::value<std::string>(&conf.log_file_path), "log file path")
            ("help,h", "show help message");

        po::options_description tls("Tls tunnel options");
        tls.add_options()
            ("tls,t", po::value<std::string>()->implicit_value("0"), "use tls tunnel mode")
            ("private-key,k", po::value<std::string>(&conf.tls_options.private_key)->default_value(""), "private key file path")
            ("server-cert,s", po::value<std::string>(&conf.tls_options.server_cert)->default_value(""), "server certificate file path")
            ("ca-cert,c", po::value<std::string>(&conf.tls_options.ca_cert)->default_value(""), "CA certificate file path");

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

        if (vm.count("log_level"))
            conf.log_level = parse_log_level(vm["log_level"].as<std::string>());

        if (vm.count("tls")) {
            if (conf.tls_options.private_key.empty()) {
                std::cout << "In tls mode, the <private-key> parameter must be specified\n";
                exit(EXIT_FAILURE);
            }
            if (conf.tls_options.server_cert.empty()) {
                std::cout << "In tls mode, the <server-cert> parameter must be specified\n";
                exit(EXIT_FAILURE);
            }
            if (conf.tls_options.ca_cert.empty()) {
                std::cout << "In tls mode, the <ca-cert> parameter must be specified\n";
                exit(EXIT_FAILURE);
            }
        }

        return conf;
    }
}

int main(int argc, char* argv[]) 
{
    const auto conf = parse_command_line_arguments(argc, argv);

    using logger = logging::logger;
    logger::output log_output = conf.log_file_path.empty()
        ? logger::output::console
        : logger::output::file;

    logger::initialize(conf.log_file_path, log_output, conf.log_level);

    try {
        stream_manager_ptr proxy_backend;
        if (conf.proxy_backend == "http") {
            std::cout << "Proxy-mode: http/s\n";
            proxy_backend = std::make_shared<http_stream_manager>();
        } else {
            std::cout << "Proxy-mode: socks5\n";
            proxy_backend = std::make_shared<socks5_stream_manager>();
        }

        if (!conf.tls_options.private_key.empty()) {
            std::cout << "Start listening port: " << conf.listen_port << ", tls tunnel mode enabled\n";
            tls_server srv(conf.listen_port, conf.tls_options, std::move(proxy_backend));
            srv.run();
        } else {
            std::cout << "Start listening port: " << conf.listen_port << ", tls tunnel mode disabled\n";
            server srv(conf.listen_port, std::move(proxy_backend));
            srv.run();
        }
    } catch (std::exception& ex) {
        std::cout << ex.what() << std::endl;
        logging::logger::fatal(std::string{"fatal error: "} + ex.what());
    }

    return 0;
}
