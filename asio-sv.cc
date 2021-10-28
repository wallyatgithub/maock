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
// We wrote this code based on the original code which has the
// following license:
//
// main.cpp
// ~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <string>
#include <fstream>
#include <thread>
#include <future>
#include <memory>

#include <nghttp2/asio_http2_server.h>
#include "H2Server_Config_Schema.h"
#include "H2Server_Request.h"
#include "H2Server.h"

using namespace nghttp2::asio_http2;
using namespace nghttp2::asio_http2::server;

int main(int argc, char *argv[]) {
  try {
    // Check command line arguments.
    if (argc < 2) {
      std::cerr<< "Usage: "<<argv[0]<<" config.json"<<std::endl;
      return 1;
    }

    std::string config_file_name = argv[1];
    std::ifstream buffer(config_file_name);
    std::string jsonStr((std::istreambuf_iterator<char>(buffer)),
                        std::istreambuf_iterator<char>());
    staticjson::ParseStatus result;
    H2Server_Config_Schema config_schema;
    if (!staticjson::from_json_string(jsonStr.c_str(), &config_schema, &result))
    {
        std::cout << "error reading config file:" << result.description() << std::endl;
        exit(1);
    }
    if (config_schema.verbose)
    {
        debug_mode = true;
    }

    boost::system::error_code ec;

    std::string addr = config_schema.address;
    std::string port = std::to_string(config_schema.port);
    std::size_t num_threads = config_schema.threads;
    if (!num_threads)
    {
        num_threads = std::thread::hardware_concurrency();
    }
    if (!num_threads)
    {
        num_threads = 1;
    }
    http2 server;

    auto totalReqReceived = std::make_shared<std::atomic<uint64_t>>(0);
    auto totalMatchedResponseSent = std::make_shared<std::atomic<uint64_t>>(0);

    server.num_threads(num_threads);

    server.handle("/", [&, totalReqReceived, totalMatchedResponseSent](const request &req, const response &res) {

      static thread_local H2Server h2server(config_schema);

      H2Server_Request_Message msg(req);
      (*totalReqReceived)++;
      auto matched_response = h2server.get_response_to_return(msg);
      if (matched_response)
      {
          header_map headers;
          for (auto header: matched_response->produce_headers(msg))
          {
              nghttp2::asio_http2::header_value hdr_val;
              hdr_val.sensitive = false;
              hdr_val.value = header.second;
              headers.insert(std::make_pair(header.first, hdr_val));
          }
          std::string payload = matched_response->produce_payload(msg);
          if (payload.size())
          {
              nghttp2::asio_http2::header_value hdr_val;
              hdr_val.sensitive = false;
              hdr_val.value = std::to_string(payload.size());
              headers.insert(std::make_pair("Content-Length", hdr_val));
          }

          res.write_head(matched_response->status_code, headers);
          res.end(payload);
          (*totalMatchedResponseSent)++;
      }
      else
      {
          res.write_head(404, {{"reason", {"no match found"}}});
          res.end("no matched entry found\n");
      }
      
    });

    std::cout<<"addr: "<<addr<<", port: "<<port<<std::endl;

    std::future<void> fu_tps =
        std::async(std::launch::async, [totalReqReceived, totalMatchedResponseSent]()
    {
        uint64_t totalReqReceived_till_lastInterval = 0;
        uint64_t totalMatchedResponseSent_till_lastInterval = 0;
        while (true)
        {
            auto total_received = totalReqReceived->load();
            auto total_matched_sent = totalMatchedResponseSent->load();
            auto delta_received = total_received - totalReqReceived_till_lastInterval;
            auto delta_matched_sent = total_matched_sent - totalMatchedResponseSent_till_lastInterval;
            auto now = std::chrono::system_clock::now();
            auto now_c = std::chrono::system_clock::to_time_t(now);
            std::cout<<std::endl;
            std::cout << std::put_time(std::localtime(&now_c), "%c")<<std::endl
                      << "total req recv: "<<total_received
                      << ", total response sent: " << total_matched_sent
                      << ", incoming rps: " << delta_received
                      << ", outgoing rps: " << delta_matched_sent
                      << std::endl;
            totalReqReceived_till_lastInterval = total_received;
            totalMatchedResponseSent_till_lastInterval = total_matched_sent;
            
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
    
    if (config_schema.cert_file.size() && config_schema.private_key_file.size()) {
      boost::asio::ssl::context tls(boost::asio::ssl::context::sslv23);
      tls.use_private_key_file(config_schema.private_key_file, boost::asio::ssl::context::pem);
      tls.use_certificate_chain_file(config_schema.cert_file);

      configure_tls_context_easy(ec, tls);

      if (server.listen_and_serve(ec, tls, addr, port)) {
        std::cerr << "error: " << ec.message() << std::endl;
      }
    } else {
      if (server.listen_and_serve(ec, addr, port)) {
        std::cerr << "error: " << ec.message() << std::endl;
      }
    }
  } catch (std::exception &e) {
    std::cerr << "exception: " << e.what() << "\n";
  }

  return 0;
}
