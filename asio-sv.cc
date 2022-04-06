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
#include <algorithm>
#include <numeric>
#include <iostream>
#include <string>
#include <fstream>
#include <thread>
#include <future>
#include <memory>
#include <tuple>
#ifdef _WINDOWS
#include <sdkddkver.h>
#endif
#include <boost/asio/io_service.hpp>
#include <boost/thread/thread.hpp>

#include <nghttp2/asio_http2_server.h>
#include "asio_server_http2_handler.h"
#include "asio_server_stream.h"

#include "H2Server_Config_Schema.h"
#include "H2Server_Request.h"
#include "H2Server.h"
#include "asio_util.h"

using namespace nghttp2::asio_http2;
using namespace nghttp2::asio_http2::server;

H2Server_Config_Schema config_schema;

void start_server(const std::string& config_file_name, bool start_stats_thread)
{
    std::ifstream buffer(config_file_name);
    std::string jsonStr((std::istreambuf_iterator<char>(buffer)), std::istreambuf_iterator<char>());

    staticjson::ParseStatus result;
    if (!staticjson::from_json_string(jsonStr.c_str(), &config_schema, &result))
    {
        std::cout << "error reading config file:" << result.description() << std::endl;
        exit(1);
    }

    if (config_schema.verbose)
    {
        std::cerr << "Configuration dump:" << std::endl << staticjson::to_pretty_json_string(config_schema)
                  << std::endl;
        debug_mode = true;
    }

    H2Server h2server(config_schema); // sanity check to fail early

    std::size_t num_threads = config_schema.threads;
    if (!num_threads)
    {
        num_threads = std::thread::hardware_concurrency();
    }
    if (!num_threads)
    {
        num_threads = 1;
    }
    config_schema.threads = num_threads;

    static std::vector<uint64_t> totalReqsReceived(num_threads, 0);
    static std::vector<uint64_t> totalUnMatchedResponses(num_threads, 0);
    static std::vector<std::vector<std::vector<ResponseStatistics>>> respStats;
    for (size_t req_idx = 0; req_idx < config_schema.service.size(); req_idx++)
    {
        std::vector<std::vector<ResponseStatistics>> perServiceStats(config_schema.service[req_idx].responses.size(),
                                                                     std::vector<ResponseStatistics>(num_threads));
        respStats.push_back(perServiceStats);
    }
    if (start_stats_thread)
    {
        start_statistic_thread(totalReqsReceived, respStats, totalUnMatchedResponses, config_schema);
    }

    asio_svr_entry(config_schema, totalReqsReceived, totalUnMatchedResponses, respStats);
}

int main(int argc, char *argv[]) {
  if (argc < 2)
  {
      std::cerr<< "Usage: "<<argv[0]<<" config.json"<<std::endl;
      return 1;
  }
  std::string config_file_name = argv[1];

  start_server(config_file_name, true);
  return 0;
}
