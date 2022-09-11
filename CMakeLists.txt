cmake_minimum_required(VERSION 3.0)
project(maock VERSION 0.1)

set(CMAKE_VERBOSE_MAKEFILE ON)

if (DEFINED ALPINE_STATIC)
     add_link_options(-static)
     set(MIMALLOC_LIB mimalloc)
endif()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

set(Boost_USE_STATIC_LIBS ON)

find_package(OpenSSL 1.0.1)

find_package(Boost 1.53.0 REQUIRED system thread)

find_package(LuaJIT)

find_path(NGHTTP2_INCLUDE_DIR nghttp2/nghttp2.h)

find_library(NGHTTP2_LIBRARY NAMES nghttp2)

include(GNUInstallDirs)
# Checks for header files.
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

include(CheckTypeSize)
# Checks for typedefs, structures, and compiler characteristics.
# AC_TYPE_SIZE_T
check_type_size("ssize_t" SIZEOF_SSIZE_T)
if(SIZEOF_SSIZE_T STREQUAL "")
  # ssize_t is a signed type in POSIX storing at least -1.
  # Set it to "int" to match the behavior of AC_TYPE_SSIZE_T (autotools).
  set(ssize_t int)
endif()
# AC_TYPE_UINT8_T
# AC_TYPE_UINT16_T
# AC_TYPE_UINT32_T
# AC_TYPE_UINT64_T
# AC_TYPE_INT8_T
# AC_TYPE_INT16_T
# AC_TYPE_INT32_T
# AC_TYPE_INT64_T
# AC_TYPE_OFF_T
# AC_TYPE_PID_T
# AC_TYPE_UID_T
# XXX To support inline for crappy compilers, see https://cmake.org/Wiki/CMakeTestInline
# AC_C_INLINE
# XXX is AC_SYS_LARGEFILE still needed for modern systems?
# add_definitions(-D_FILE_OFFSET_BITS=64)

include(CheckStructHasMember)
check_struct_has_member("struct tm" tm_gmtoff time.h HAVE_STRUCT_TM_TM_GMTOFF)

# Check size of pointer to decide we need 8 bytes alignment adjustment.
check_type_size("int *"   SIZEOF_INT_P)
check_type_size("time_t"  SIZEOF_TIME_T)

# Checks for library functions.
include(CheckFunctionExists)
check_function_exists(_Exit     HAVE__EXIT)
check_function_exists(accept4   HAVE_ACCEPT4)
check_function_exists(mkostemp  HAVE_MKOSTEMP)

include(CheckSymbolExists)
# XXX does this correctly detect initgroups (un)availability on cygwin?
check_symbol_exists(initgroups grp.h HAVE_DECL_INITGROUPS)
if(NOT HAVE_DECL_INITGROUPS AND HAVE_UNISTD_H)
  # FreeBSD declares initgroups() in unistd.h
  check_symbol_exists(initgroups unistd.h HAVE_DECL_INITGROUPS2)
  if(HAVE_DECL_INITGROUPS2)
    set(HAVE_DECL_INITGROUPS 1)
  endif()
endif()

if(DEFINED ADDRESS_SANITIZE)
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif()

IF(WIN32)
    add_definitions(-DNOMINMAX=1)
    #add_definitions(-D_WIN32_WINNT=${ver})
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    #add_definitions(-D_ITERATOR_DEBUG_LEVEL=0)
    find_library(GETOPT_LIBRARY NAMES getopt)
endif()

if(VCPKG_TARGET_TRIPLET MATCHES "${__ARCH}-windows-static")
    if(MSVC)
        add_compile_options(
            $<$<CONFIG:>:/MT>
            $<$<CONFIG:Debug>:/MTd>
            $<$<CONFIG:Release>:/MT>
        )
    endif()
endif()

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

set(ASIO_SV_SOURCES
  asio_common.cc
  asio_io_service_pool.cc
  asio_httpx_server.cc
  asio_httpx_server_impl.cc
  asio_server.cc
  asio_server_http2_handler.cc
  asio_server_request.cc
  asio_server_response.cc
  asio_server_stream.cc
  asio_server_serve_mux.cc
  asio_server_request_handler.cc
  asio_server_tls_context.cc
  asio_server_base_handler.cc
  asio_server_http1_handler.cc
  asio_util.cc
  common_lua.cc
)

set(THIRD_PARTY_LUA_MOD_SOURCE
    third-party/lua-rapidJson/Document.cpp
    third-party/lua-rapidJson/Schema.cpp
    third-party/lua-rapidJson/Userdata.hpp
    third-party/lua-rapidJson/file.hpp
    third-party/lua-rapidJson/luax.hpp
    third-party/lua-rapidJson/rapidjson.cpp
    third-party/lua-rapidJson/values.cpp
    third-party/lua-rapidJson/values.hpp
    pb.c
)

set(NGHTTP2_ASIO_SV_SOURCES
  util.cc
  http2.cc
  tls.cc
  timegm.c
  ${ASIO_SV_SOURCES}
  ${THIRD_PARTY_LUA_MOD_SOURCE}
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
  ${NGHTTP2_LIBRARY}
  ${OPENSSL_LIBRARIES}
  ${Boost_LIBRARIES}
)
set_target_properties(nghttp2_asio_sv PROPERTIES
  VERSION 1.0.0 SOVERSION 1)

link_libraries(
${NGHTTP2_LIBRARY}
${OPENSSL_LIBRARIES}
${APP_LIBRARIES}
${LUA_LIBRARIES}
${MIMALLOC_LIB}
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
${NGHTTP2_LIBRARY}
${OPENSSL_LIBRARIES}
${Boost_LIBRARIES}
${APP_LIBRARIES}
${LUA_LIBRARIES}
${MIMALLOC_LIB}
)
