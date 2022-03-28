#include "asio_util.h"
#include "maock_config.h"


bool debug_mode = false;

void close_stream(uint64_t& handler_id, int32_t stream_id)
{
    auto h2_handler = nghttp2::asio_http2::server::http2_handler::find_http2_handler(handler_id);
    if (h2_handler)
    {
        h2_handler->close_stream(stream_id);
    }
}

size_t get_req_name_max_size(const H2Server_Config_Schema& config_schema)
{
    const std::string req_name = "req-name";
    const std::string total = "TOTAL";
    size_t width = req_name.size();
    for (size_t req_index = 0; req_index < config_schema.service.size(); req_index++)
    {
        if (config_schema.service[req_index].request.name.size() > width)
        {
            width = config_schema.service[req_index].request.name.size();
        }
    }
    return width;

}

size_t get_resp_name_max_size(const H2Server_Config_Schema& config_schema)
{
    const std::string resp_name = "resp-name";
    const std::string total = "TOTAL";
    size_t width = resp_name.size();
    for (size_t req_index = 0; req_index < config_schema.service.size(); req_index++)
    {
        for (size_t resp_index = 0; resp_index < config_schema.service[req_index].responses.size(); resp_index++)
            if (config_schema.service[req_index].responses[resp_index].name.size() > width)
            {
                width = config_schema.service[req_index].responses[resp_index].name.size();
            }
    }
    return width;
}

void send_response(uint32_t status_code,
                   const std::map<std::string, std::string>& resp_headers,
                   const std::string& resp_payload,
                   uint64_t handler_id,
                   int32_t stream_id,
                   uint64_t& matchedResponsesSent
                  )
{
    auto h2_handler = nghttp2::asio_http2::server::http2_handler::find_http2_handler(handler_id);
    if (!h2_handler)
    {
        return;
    }
    if (!status_code)
    {
        h2_handler->close_stream(stream_id);;
        return;
    }

    auto orig_stream = h2_handler->find_stream(stream_id);
    if (!orig_stream)
    {
        return;
    }
    nghttp2::asio_http2::header_map headers;
    for (auto& header : resp_headers)
    {
        nghttp2::asio_http2::header_value hdr_val;
        hdr_val.sensitive = false;
        hdr_val.value = header.second;
        headers.insert(std::make_pair(header.first, hdr_val));
        if (debug_mode)
        {
            std::cout << "sending header " << header.first << ": " << header.second << std::endl;
        }
    }
    if (resp_payload.size())
    {
        nghttp2::asio_http2::header_value hdr_val;
        hdr_val.sensitive = false;
        hdr_val.value = std::to_string(resp_payload.size());
        headers.insert(std::make_pair("Content-Length", hdr_val));
    }
    if (debug_mode && resp_payload.size())
    {
        std::cout << "sending header " << "Content-Length: " << resp_payload.size() << std::endl;
        std::cout << "sending msg body " << resp_payload << std::endl;
    }
    if (debug_mode)
    {
        std::cout << "sending status code: " << status_code << std::endl;
    }
    auto& res = orig_stream->response();
    res.write_head(status_code, headers);
    res.end(resp_payload);
    matchedResponsesSent++;
};


void send_response_from_another_thread(boost::asio::io_service* target_io_service,
                                       uint64_t handler_id,
                                       int32_t stream_id,
                                       std::map<std::string, std::string>& resp_headers,
                                       std::string& resp_payload
                                      )
{
    thread_local static uint64_t matchedResponsesSent;
    if (!target_io_service)
    {
        return;
    }
    uint32_t status_code = atoi(resp_headers[status].c_str());
    resp_headers.erase(status);
    auto call_send_response = [handler_id, stream_id, status_code, resp_headers, resp_payload]()
    {
        send_response(status_code, resp_headers, resp_payload, handler_id, stream_id, matchedResponsesSent);
    };
    target_io_service->post(call_send_response);
}

void update_response_with_lua(const H2Server_Response* matched_response,
                              std::multimap<std::string, std::string>& req_headers,
                              std::string& req_payload,
                              std::map<std::string, std::string>& resp_headers,
                              std::string& resp_payload,
                              uint64_t handler_id,
                              int32_t stream_id,
                              uint64_t& matchedResponsesSent)
{
    matched_response->update_response_with_lua(req_headers, req_payload, resp_headers, resp_payload);

    auto io_service = nghttp2::asio_http2::server::http2_handler::find_io_service(handler_id);
    if (!io_service)
    {
        return;
    }

    auto status_code = matched_response->status_code;
    if (resp_headers.count(status))
    {
        status_code = atoi(resp_headers[status].c_str());
    }
    resp_headers.erase(status);
    auto send_response_routine = std::bind(send_response,
                                           status_code,
                                           resp_headers,
                                           resp_payload,
                                           handler_id,
                                           stream_id,
                                           std::ref(matchedResponsesSent)
                                          );
    io_service->post(send_response_routine);
};

std::vector<H2Server>& get_H2Server_match_Instances()
{
    static std::vector<H2Server> H2Server_match_instances;
    return H2Server_match_instances;
}

void init_H2Server_match_Instances(std::size_t number_of_instances, H2Server_Config_Schema& config_schema)
{
    auto init_func = [number_of_instances, &config_schema]()
    {
        for (size_t i = 0; i < number_of_instances; i++)
        {
            get_H2Server_match_Instances().emplace_back(config_schema);
        };
        return true;
    };
    static auto ret_code = init_func();
}

void asio_svr_entry(const std::string& config_in_json)
{
    try
    {
        staticjson::ParseStatus result;
        H2Server_Config_Schema config_schema;
        if (!staticjson::from_json_string(config_in_json.c_str(), &config_schema, &result))
        {
            std::cout << "error reading config file:" << result.description() << std::endl;
            exit(1);
        }

        if (config_schema.verbose)
        {
            std::cerr << "Configuration dump:" << std::endl << staticjson::to_pretty_json_string(config_schema)
                      << std::endl;
        }
        maock_config.nghttp2_max_concurrent_streams = config_schema.max_concurrent_streams;

        H2Server h2server(config_schema); // sanity check to fail early

        if (config_schema.verbose)
        {
            debug_mode = true;
        }

        boost::asio::io_service work_offload_io_service;
        boost::thread_group work_offload_thread_pool;
        boost::asio::io_service::work work(work_offload_io_service);
        for (size_t i = 0; i < config_schema.service.size(); i++)
        {
            work_offload_thread_pool.create_thread(boost::bind(&boost::asio::io_service::run, &work_offload_io_service));
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
        init_H2Server_match_Instances(num_threads, config_schema);

        nghttp2::asio_http2::server::http2 server;

        std::atomic<uint64_t> threadIndex(0);
        std::vector<uint64_t> totalReqsReceived(num_threads, 0);
        std::vector<std::vector<std::vector<ResponseStatistics>>> respStats;

        for (size_t req_idx = 0; req_idx < config_schema.service.size(); req_idx++)
        {
            std::vector<std::vector<ResponseStatistics>> perServiceStats(config_schema.service[req_idx].responses.size(),
                                                                         std::vector<ResponseStatistics>(num_threads));
            respStats.push_back(perServiceStats);
        }

        server.num_threads(num_threads);

        server.handle("/", [&work_offload_io_service, &config_schema,
                            &respStats, &threadIndex,
                            &totalReqsReceived
                           ]
                            (const nghttp2::asio_http2::server::request& req,
                             const nghttp2::asio_http2::server::response& res,
                             uint64_t handler_id, int32_t stream_id
                            )
        {

            static thread_local auto thread_index = threadIndex++;
            static thread_local H2Server& h2server = get_H2Server_match_Instances()[thread_index];
            auto store_io_service_to_H2Server = [handler_id]()
            {
                auto my_io_service = nghttp2::asio_http2::server::http2_handler::find_io_service(handler_id);
                h2server.set_io_service(my_io_service);
                return true;
            };
            static thread_local auto store_io_service_ret_code = store_io_service_to_H2Server();
            static thread_local auto& reqReceived = totalReqsReceived[thread_index];
            auto init_strand = [&work_offload_io_service]()
            {
                std::map<const H2Server_Response*, boost::asio::io_service::strand> strands;
                for (auto& service : h2server.services)
                {
                    for (auto& response : service.second.responses)
                    {
                        strands.insert(std::make_pair(&response, boost::asio::io_service::strand(work_offload_io_service)));
                    }
                }
                return strands;
            };
            static thread_local auto strands = init_strand();

            H2Server_Request_Message msg(req);
            reqReceived++;
            size_t req_index;
            size_t resp_index;
            int64_t matched_request_index = -1;
            auto matched_service = h2server.get_matched_request(msg, matched_request_index);
            if (matched_request_index > -1)
            {
                req_index = matched_request_index;
                if (matched_service->first.get_request_processor())
                {
                    matched_service->first.get_request_processor()(h2server.io_service,
                                                                   handler_id,
                                                                   stream_id,
                                                                   msg.headers,
                                                                   req.unmutable_payload()
                                                                  );
                }
                else
                {
                    auto matched_response = h2server.get_response_to_return(matched_service, resp_index);
                    if (matched_response->is_response_throttled())
                    {
                        close_stream(handler_id, stream_id);
                        respStats[req_index][resp_index][thread_index].response_throttled++;
                        return;
                    }
                    auto response_headers = matched_response->produce_headers(msg);
                    auto response_payload = matched_response->produce_payload(msg);
                    auto status_code = matched_response->status_code;
                    if (matched_response->luaState.get())
                    {
                        if (matched_response->lua_offload)
                        {
                            auto msg_update_routine = std::bind(update_response_with_lua,
                                                                matched_response,
                                                                msg.headers,
                                                                req.unmutable_payload(),
                                                                response_headers,
                                                                response_payload,
                                                                handler_id,
                                                                stream_id,
                                                                std::ref(respStats[req_index][resp_index][thread_index].response_sent));
                            auto it = strands.find(matched_response);
                            it->second.post(msg_update_routine);
                            return;
                        }
                        else
                        {
                            matched_response->update_response_with_lua(msg.headers, req.unmutable_payload(), response_headers, response_payload);
                            if (response_headers.count(status))
                            {
                                status_code = atoi(response_headers[status].c_str());
                            }
                            response_headers.erase(status);
                        }
                    }
                    send_response(status_code, response_headers, response_payload, handler_id, stream_id,
                                  respStats[req_index][resp_index][thread_index].response_sent);
                }
            }
            else
            {
                res.write_head(404, {{"reason", {"no match found"}}});
                res.end("no matched entry found\n");
            }
        });

        std::cout << "addr: " << addr << ", port: " << port << std::endl;
        start_statistic_thread(totalReqsReceived, respStats, config_schema);
        if (config_schema.cert_file.size() && config_schema.private_key_file.size())
        {
            if (config_schema.verbose)
            {
                std::cout << "cert file: " << config_schema.cert_file << std::endl;
                std::cout << "private key file: " << config_schema.private_key_file << std::endl;
            }
            boost::asio::ssl::context tls(boost::asio::ssl::context::sslv23);
            tls.use_private_key_file(config_schema.private_key_file, boost::asio::ssl::context::pem);
            tls.use_certificate_chain_file(config_schema.cert_file);
            if (config_schema.enable_mTLS)
            {
                if (config_schema.verbose)
                {
                    std::cout << "ca cert file: " << config_schema.ca_cert_file << std::endl;
                }
                if (config_schema.ca_cert_file.size())
                {
                    tls.load_verify_file(config_schema.ca_cert_file);
                }
                else
                {
                    std::cerr << "mTLS enabled, but no CA cert file given, mTLS is thus disabled" << std::endl;
                    config_schema.enable_mTLS = false;
                }
            }

            nghttp2::asio_http2::server::configure_tls_context_easy(ec, tls, config_schema.enable_mTLS);

            if (server.listen_and_serve(ec, tls, addr, port))
            {
                std::cerr << "error: " << ec.message() << std::endl;
            }
        }
        else
        {
            if (server.listen_and_serve(ec, addr, port))
            {
                std::cerr << "error: " << ec.message() << std::endl;
            }
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "exception: " << e.what() << "\n";
    }
}

void start_statistic_thread(std::vector<uint64_t>& totalReqsReceived,
                            std::vector<std::vector<std::vector<ResponseStatistics>>>& respStats,
                            H2Server_Config_Schema& config_schema)
{
    auto stats_func = [&totalReqsReceived, &respStats, &config_schema]()
    {
        std::vector<std::vector<uint64_t>> resp_sent_till_now;
        std::vector<std::vector<uint64_t>> resp_throttled_till_now;
        for (size_t i = 0; i < config_schema.service.size(); i++)
        {
            resp_sent_till_now.emplace_back(std::vector<uint64_t>(config_schema.service[i].responses.size(), 0));
            resp_throttled_till_now.emplace_back(std::vector<uint64_t>(config_schema.service[i].responses.size(), 0));
        }
        uint64_t total_req_received_till_now = 0;
        uint64_t total_resp_sent_till_now = 0;
        uint64_t total_resp_throttled_till_now = 0;
        uint64_t counter = 0;

        auto req_name_width = get_req_name_max_size(config_schema);
        auto resp_name_width = get_resp_name_max_size(config_schema);
        size_t request_width = 0;

        auto period_start = std::chrono::steady_clock::now();
        while (true)
        {
            std::stringstream SStream;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (counter % 10 == 0)
            {
                SStream << "req-name,   resp-name,   msg-total,   throttled-total, rps,      throttled-rps" << std::endl;
            }
            counter++;

            auto resp_sent_till_last = resp_sent_till_now;
            auto resp_throttled_till_last = resp_throttled_till_now;

            auto total_req_received_till_last = total_req_received_till_now;
            auto total_resp_sent_till_last = total_resp_sent_till_now;
            auto total_resp_throttled_till_last = total_resp_throttled_till_now;

            total_req_received_till_now = std::accumulate(totalReqsReceived.begin(), totalReqsReceived.end(), 0);
            total_resp_sent_till_now = 0;
            total_resp_throttled_till_now = 0;

            for (size_t req_index = 0; req_index < config_schema.service.size(); req_index++)
            {
                for (size_t resp_index = 0; resp_index < config_schema.service[req_index].responses.size(); resp_index++)
                {
                    resp_sent_till_now[req_index][resp_index] =
                        std::accumulate(respStats[req_index][resp_index].begin(),
                                        respStats[req_index][resp_index].end(),
                                        0,
                                        [](uint64_t sum, const ResponseStatistics & val)
                    {
                        return sum + val.response_sent;
                    }
                                       );
                    resp_throttled_till_now[req_index][resp_index] =
                        std::accumulate(respStats[req_index][resp_index].begin(),
                                        respStats[req_index][resp_index].end(),
                                        0,
                                        [](uint64_t sum, const ResponseStatistics & val)
                    {
                        return sum + val.response_throttled;
                    }
                                       );
                    total_resp_sent_till_now += resp_sent_till_now[req_index][resp_index];
                    total_resp_throttled_till_now += resp_throttled_till_now[req_index][resp_index];
                }
            }

            auto delta_Req_Received = total_req_received_till_now - total_req_received_till_last;
            auto delta_Resp = total_resp_sent_till_now + total_resp_throttled_till_now - total_resp_sent_till_last -
                              total_resp_throttled_till_last;
            if (!delta_Req_Received && !delta_Resp)
            {
                continue;
            }

            auto period_end = std::chrono::steady_clock::now();
            auto period_duration = std::chrono::duration_cast<std::chrono::milliseconds>(period_end - period_start).count();
            period_start = period_end;

            for (size_t req_index = 0; req_index < config_schema.service.size(); req_index++)
            {
                for (size_t resp_index = 0; resp_index < config_schema.service[req_index].responses.size(); resp_index++)
                {
                    SStream <<     std::setw(req_name_width) << config_schema.service[req_index].request.name
                            << "," << std::setw(resp_name_width) << config_schema.service[req_index].responses[resp_index].name
                            << "," << std::setw(req_name_width) << resp_sent_till_now[req_index][resp_index]
                            << "," << std::setw(req_name_width) << resp_throttled_till_now[req_index][resp_index]
                            << "," << std::setw(req_name_width) << ((resp_sent_till_now[req_index][resp_index] -
                                                                     resp_sent_till_last[req_index][resp_index])*std::milli::den) / period_duration
                            << "," << std::setw(req_name_width) << ((resp_throttled_till_now[req_index][resp_index] -
                                                                     resp_throttled_till_last[req_index][resp_index])*std::milli::den) / period_duration
                            << std::endl;
                }
            }
            SStream <<     std::setw(req_name_width) << "SUM"
                    << "," << std::setw(resp_name_width) << "SUM"
                    << "," << std::setw(req_name_width) << total_resp_sent_till_now
                    << "," << std::setw(req_name_width) << total_resp_throttled_till_now
                    << "," << std::setw(req_name_width) << ((total_resp_sent_till_now - total_resp_sent_till_last)*std::milli::den) /
                    period_duration
                    << "," << std::setw(req_name_width) << ((total_resp_throttled_till_now - total_resp_throttled_till_last)
                                                            *std::milli::den) / period_duration
                    << std::endl;
            std::cout << SStream.str();

            SStream <<     std::setw(req_name_width) << "UNMATCHED"
                    << "," << std::setw(resp_name_width) << "---"
                    << "," << std::setw(req_name_width) << total_req_received_till_now - total_resp_sent_till_now -
                    total_resp_throttled_till_now
                    << "," << std::setw(req_name_width) << "---"
                    << "," << std::setw(req_name_width) << ((delta_Req_Received - delta_Resp)*std::milli::den) / period_duration
                    << "," << std::setw(req_name_width) << "---"
                    << std::endl;
            std::cout << SStream.str();

            auto new_request_width = std::to_string(total_resp_sent_till_now).size();
            request_width = request_width > new_request_width ? request_width : new_request_width;
        }
    };
    std::thread stats_thread(stats_func);
    stats_thread.detach();
}
