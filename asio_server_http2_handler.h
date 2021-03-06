/*
 * nghttp2 - HTTP/2 C Library
 *
 * Copyright (c) 2014 Tatsuhiro Tsujikawa
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef ASIO_SERVER_HTTP2_HANDLER_H
#define ASIO_SERVER_HTTP2_HANDLER_H

#include "nghttp2_config.h"

#include <map>
#include <functional>
#include <string>
#include <mutex>

#include <boost/array.hpp>

#include <nghttp2/asio_httpx_server.h>
#include "asio_server_base_handler.h"

namespace nghttp2 {
namespace asio_http2 {
namespace server {

class asio_server_stream;
class serve_mux;
class asio_server_response;
class asio_server_request;

class http2_handler : public std::enable_shared_from_this<http2_handler>, public base_handler {
public:
  http2_handler(boost::asio::io_service &io_service,
                boost::asio::ip::tcp::endpoint ep, connection_write writefun,
                serve_mux &mux,
                const H2Server_Config_Schema& conf);

  ~http2_handler();

  void call_on_request(asio_server_stream &s);

  virtual int start();

  virtual bool should_stop() const;

  virtual int start_response(asio_server_stream &s);

  virtual int submit_trailer(asio_server_stream &s, header_map h);

  virtual void stream_error(int32_t stream_id, uint32_t error_code);

  virtual void resume(asio_server_stream &s);

  virtual asio_server_response* push_promise(boost::system::error_code &ec, asio_server_stream &s,
                         std::string method, std::string raw_path_query,
                         header_map h);

  virtual void initiate_write();

  virtual void signal_write();

  virtual int on_read(const std::vector<uint8_t>& buffer, std::size_t len) {
    callback_guard cg(*this);

    int rv;

    rv = nghttp2_session_mem_recv(session_, buffer.data(), len);

    if (rv < 0) {
      return -1;
    }

    return 0;
  }

  virtual int on_write(std::vector<uint8_t>& buffer, std::size_t &len) {
    callback_guard cg(*this);

    len = 0;

    if (buf_) {
      std::copy_n(buf_, buflen_, std::begin(buffer));

      len += buflen_;

      buf_ = nullptr;
      buflen_ = 0;
    }

    for (;;) {
      const uint8_t *data;
      auto nread = nghttp2_session_mem_send(session_, &data);
      if (nread < 0) {
        return -1;
      }

      if (nread == 0) {
        break;
      }

      if (len + nread > buffer.size()) {
        buf_ = data;
        buflen_ = nread;

        break;
      }

      std::copy_n(data, nread, std::begin(buffer) + len);

      len += nread;
    }

    return 0;
  }

private:
  nghttp2_session *session_;
  const uint8_t *buf_;
  std::size_t buflen_;
};

} // namespace server
} // namespace asio_http2
} // namespace nghttp2

#endif // ASIO_SERVER_HTTP2_HANDLER_H
