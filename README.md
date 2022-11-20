# amgi_net_tools
___
Network utilities developed for research purposes


## Description
A set of utilities developed by me for use in everyday life.
At the moment, the set consists of two programs:
* amgi_proxy
* amgi_tunnel

When working through a TLS tunnel, mutual authentication is always performed between 
the client (amgi_tunnel) and the proxy server (amgi_proxy). The root certificate of 
the CA, as well as the signed keys for the proxy server and client, can be generated 
using openssl. Accordingly, only those clients who have a signed certificate have 
access to the proxy server.  

The expected format for certificates and private keys is PEM.

### amgi_proxy
amgi_proxy is a proxy server that accepts tcp client connections as well as connections
authenticated by X509 certificates using the TLS 1.3 protocol. Currently, two proxy 
modes are supported: socks5 (tcp only) and http/s. To work in the mode of accepting 
connections via the TLS protocol you need to provide it with:
- CA certificate
- server certificate
- server private key


### amgi_tunnel
amgi_tunnel establishes a TLS 1.3 tunnel with the amgi_proxy program, 
and forwards all incoming TCP connections to the proxy server.
For the program to work, you need to provide it with:
- CA certificate
- client certificate 
- private client key

## Requirements
___
* CMake (>= 3.16)
* C++ 17 (GCC >= 10.3.0, Clang >= 12.0.0, Visual C++ >= 12.0)
* C++ Boost (>= 1.71)
* OpenSSL (>= 1.1.1)
* Ninja (>= 1.10.0)
* Windows, Linux (tested on Windows 10 and Ubuntu 20.04/22.04)

## Build instructions
### Windows
    $ git clone https://github.com/alexisvoila/amgi_net_tools.git
    $ cd amgi_net_tools
    $ mkdir build
    $ cd build
    $ cmake .. -G "Visual Studio 17 2022" -A x64
    $ cmake --build . --config Release

### Linux
    $ git clone https://github.com/alexisvoila/amgi_net_tools.git
    $ cd amgi_net_tools
    $ mkdir build
    $ cd build
    $ cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release ..
    $ cmake --build .