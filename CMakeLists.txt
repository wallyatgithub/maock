cmake_minimum_required(VERSION 3.0)
project(maock VERSION 0.1)

set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

set(Boost_USE_STATIC_LIBS ON)

find_package(OpenSSL 1.0.1)

find_package(Boost 1.53.0 REQUIRED system thread)

find_package(LuaJIT)

include(GNUInstallDirs)
# Checks for header files.
include(CheckIncludeFile)
check_include_file("arpa/inet.h"    HAVE_ARPA_INET_H)
check_include_file("fcntl.h"        HAVE_FCNTL_H)
check_include_file("inttypes.h"     HAVE_INTTYPES_H)
check_include_file("limits.h"       HAVE_LIMITS_H)
check_include_file("netdb.h"        HAVE_NETDB_H)
check_include_file("netinet/in.h"   HAVE_NETINET_IN_H)
check_include_file("pwd.h"          HAVE_PWD_H)
check_include_file("sys/socket.h"   HAVE_SYS_SOCKET_H)
check_include_file("sys/time.h"     HAVE_SYS_TIME_H)
check_include_file("syslog.h"       HAVE_SYSLOG_H)
check_include_file("time.h"         HAVE_TIME_H)
check_include_file("unistd.h"       HAVE_UNISTD_H)

set (CMAKE_INCLUDE_CURRENT_DIR ON)

add_definitions(-DHAVE_CONFIG_H)
configure_file(cmakeconfig.h.in config.h)

set(APP_LIBRARIES)
find_package(Threads)
list(APPEND APP_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})

add_subdirectory(third-party)

add_subdirectory(includes)

file(GLOB c_sources *.c)
set_source_files_properties(${c_sources} PROPERTIES
  COMPILE_FLAGS "${WARNCFLAGS}")
file(GLOB cxx_sources *.cc)
set_source_files_properties(${cxx_sources} PROPERTIES
  COMPILE_FLAGS "${WARNCXXFLAGS} ${CXX1XCXXFLAGS}")

include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}/includes"
  "${CMAKE_CURRENT_SOURCE_DIR}/third-party"
  "${CMAKE_CURRENT_SOURCE_DIR}/third-party/llhttp/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/third-party/StaticJSON/include"
  "${LUA_INCLUDE_DIR}"
  "${OPENSSL_INCLUDE_DIR}"
)

set(NGHTTP2_ASIO_SV_SOURCES
  util.cc http2.cc
  tls.cc
  timegm.c
  asio_common.cc
  asio_io_service_pool.cc
  asio_server_http2.cc
  asio_server_http2_impl.cc
  asio_server.cc
  asio_server_http2_handler.cc
  asio_server_request.cc
  asio_server_request_impl.cc
  asio_server_response.cc
  asio_server_response_impl.cc
  asio_server_stream.cc
  asio_server_serve_mux.cc
  asio_server_request_handler.cc
  asio_server_tls_context.cc
)

add_library(nghttp2_asio_sv OBJECT
   ${NGHTTP2_ASIO_SV_SOURCES}
   $<TARGET_OBJECTS:llhttp>
   $<TARGET_OBJECTS:url-parser>
)
target_include_directories(nghttp2_asio_sv PRIVATE
  ${OPENSSL_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
)
target_include_directories(nghttp2_asio_sv INTERFACE
  "${CMAKE_CURRENT_BINARY_DIR}/../lib/includes"
  "${CMAKE_CURRENT_SOURCE_DIR}/../lib/includes"
  "${CMAKE_CURRENT_SOURCE_DIR}/includes"
)
target_link_libraries(nghttp2_asio_sv
  nghttp2
  ${OPENSSL_LIBRARIES}
  ${Boost_LIBRARIES}
)
set_target_properties(nghttp2_asio_sv PROPERTIES
  VERSION 1.0.0 SOVERSION 1)

link_libraries(
nghttp2
${OPENSSL_LIBRARIES}
${APP_LIBRARIES}
${LUA_LIBRARIES}
)

add_executable(maock asio-sv.cc $<TARGET_OBJECTS:llhttp>
$<TARGET_OBJECTS:nghttp2_asio_sv>
$<TARGET_OBJECTS:url-parser> $<TARGET_OBJECTS:staticjson>
)
target_include_directories(maock PRIVATE
${OPENSSL_INCLUDE_DIRS}
${Boost_INCLUDE_DIRS}
)
target_link_libraries(maock
nghttp2
${OPENSSL_LIBRARIES}
${Boost_LIBRARIES}
${APP_LIBRARIES}
)


