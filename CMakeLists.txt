# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.16)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("amgi_net_tools")

# Suppressing relish warnings when using fresh versions of C++ Boost
set(Boost_NO_WARN_NEW_VERSIONS 1)

if(WIN32)
    set(Boost_USE_STATIC_LIBS       ON)
    set(Boost_USE_MULTITHREADED     ON)
    set(Boost_USE_STATIC_RUNTIME    ON)

    set(BOOST_ROOT                  "c:/dev/lib/boost/boost_1_79_0")
    set(Boost_INCLUDE_DIR           "c:/dev/lib/boost/boost_1_79_0")
    set(Boost_LIBRARIES             "c:/dev/lib/boost/boost_1_79_0/lib64-msvc-14.3")


    set(OPENSSL_USE_STATIC_LIBS     TRUE)
    set(OPENSSL_MSVC_STATIC_RT      TRUE)
    set(OPENSSL_ROOT_DIR            "c:/dev/lib/OpenSSL-Win64/")
    set(OPENSSL_INCLUDE_DIR         "c:/dev/lib/OpenSSL-Win64/include")
    set(OPENSSL_LIBRARIES           "c:/dev/lib/OpenSSL-Win64/lib")
endif()

find_package(Boost COMPONENTS system program_options REQUIRED)

if(BOOST_FOUND)
    message(STATUS "Boost path:             ${BOOST_ROOT}")
    message(STATUS "Boost include path:     ${Boost_INCLUDE_DIR}")
    message(STATUS "Boost libraries path:   ${Boost_LIBRARIES}")
else()
    message(STATUS "Boost Not Found")
endif()

find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    message(STATUS "OpenSSL path:           ${OPENSSL_ROOT_DIR}")
    message(STATUS "OpenSSL include path:   ${OPENSSL_INCLUDE_DIR}")
    message(STATUS "OpenSSL libraries path: ${OPENSSL_LIBRARIES}")
else()
    message(FATAL "OpenSSL Not Found")
endif()

add_library(asio INTERFACE)
target_include_directories(asio INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/asio/asio/include")
if (UNIX)
    find_package(Threads)
    target_link_libraries(asio INTERFACE Threads::Threads)
endif()


# Include sub-projects.
add_subdirectory("thirdparty/cli_tools")
add_subdirectory("amgi_tunnel")
add_subdirectory("amgi_proxy")
