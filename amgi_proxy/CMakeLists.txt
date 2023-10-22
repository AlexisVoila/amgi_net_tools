cmake_minimum_required(VERSION 3.16)

project("amgi_proxy" CXX)

add_executable(${PROJECT_NAME} "")

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

# Asking Boost for information about automatic link requests
target_compile_definitions(${PROJECT_NAME} PRIVATE ${Boost_LIB_DIAGNOSTIC_DEFINITIONS})

target_sources(${PROJECT_NAME} PRIVATE
        "http/http.h"
        "http/http.cpp"
        "http/http_state.h"
        "http/http_state.cpp"
        "http/http_session.h"
        "http/http_session.cpp"
        "http/http_stream_manager.h"
        "http/http_stream_manager.cpp"

        "socks5/socks5.h"
        "socks5/socks5.cpp"
        "socks5/socks5_state.h"
        "socks5/socks5_state.cpp"
        "socks5/socks5_session.h"
        "socks5/socks5_session.cpp"
        "socks5/socks5_stream_manager.h"
        "socks5/socks5_stream_manager.cpp"

        "transport/io_buffer.h"
        "transport/stream.h"
        "transport/stream_manager.h"

        "transport/client_stream.h"
        "transport/server_stream.h"
        "transport/tcp_server_stream.h"
        "transport/tcp_server_stream.cpp"
        "transport/tcp_client_stream.h"
        "transport/tcp_client_stream.cpp"

        "transport/server.h"
        "transport/server.cpp"
        "transport/tls/tls_server.h"
        "transport/tls/tls_server.cpp"
        "transport/tls/tls_server_stream.h"
        "transport/tls/tls_server_stream.cpp"
        "logger/logger.h"
        "logger/logger.cpp"
        "main.cpp"
)

target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})

# If the Asio target has not been created before, then create it
if (NOT (TARGET asio))
    add_library(asio INTERFACE)
    target_include_directories(asio 
        INTERFACE 
            "${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/asio/asio/include"
    )
    if (UNIX)
        find_package(Threads)
        target_link_libraries(asio INTERFACE Threads::Threads)
    endif()
endif()

# Add external required library dependencies
add_dependencies(${PROJECT_NAME} asio)
target_link_libraries(${PROJECT_NAME} PUBLIC asio)

if (MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE 
        "_WIN32_WINNT=0x0A00"
        "$<$<CONFIG:Release>:/NDEBUG>"
    )

    set_property(TARGET ${PROJECT_NAME} PROPERTY 
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )

    target_link_options(${PROJECT_NAME} PRIVATE 
        "$<$<CONFIG:Release>:/INCREMENTAL:NO>"
		"$<$<CONFIG:Release>:/OPT:REF>"
		"$<$<CONFIG:Release>:/OPT:ICF>"
	)

    target_compile_options(${PROJECT_NAME} PRIVATE 
        "/EHsc"
    )
    set(CMAKE_VS_JUST_MY_CODE_DEBUGGING "$<$<CONFIG:Debug>:ON>")
endif()

target_link_libraries(${PROJECT_NAME} 
    PRIVATE ${Boost_LIBRARIES} 
    PRIVATE OpenSSL::SSL 
    PRIVATE OpenSSL::Crypto
)

if (UNIX)
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
endif()

target_include_directories(${PROJECT_NAME} 
    PUBLIC ${OPENSSL_INCLUDE_DIR}
    PRIVATE ${PROJECT_SOURCE_DIR}/include
)





