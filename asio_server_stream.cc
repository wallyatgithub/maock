/*
 * nghttp2 - HTTP/2 C Library
 *
 * Copyright (c) 2015 Tatsuhiro Tsujikawa
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
#include "asio_server_stream.h"

#include "asio_server_base_handler.h"
#include "asio_server_request.h"
#include "asio_server_response.h"

namespace nghttp2 {
namespace asio_http2 {
namespace server {

asio_server_stream::asio_server_stream(base_handler *h, int32_t stream_id)
    : handler_(h), stream_id_(stream_id) {
  request_.stream(this);
  response_.stream(this);
}

int32_t asio_server_stream::get_stream_id() const { return stream_id_; }

class asio_server_request& asio_server_stream::request() {
  return request_;
}

class asio_server_response& asio_server_stream::response() {
  return response_;
}

base_handler* asio_server_stream::handler() const { return handler_; }

} // namespace server
} // namespace asio_http2
} // namespace nghttp2
