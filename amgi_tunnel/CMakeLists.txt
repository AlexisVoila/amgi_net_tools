cmake_minimum_required(VERSION 3.16)

project("amgi_tunnel" CXX)

add_executable(${PROJECT_NAME} "")

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

target_sources(${PROJECT_NAME} PRIVATE
    manager.hpp
    session.hpp
    server.hpp
    main.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} 
    PUBLIC ${OPENSSL_INCLUDE_DIR}
    PRIVATE ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE OpenSSL::SSL 
    PRIVATE OpenSSL::Crypto
)

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
target_link_libraries(${PROJECT_NAME} PRIVATE asio)
target_link_libraries(${PROJECT_NAME} PRIVATE cli_tools::parser)

if(MSVC)
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

endif()

 if (UNIX)
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
endif()
