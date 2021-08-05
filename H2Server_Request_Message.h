#ifndef H2SERVER_MESSAGE_H
#define H2SERVER_MESSAGE_H

#include <rapidjson/pointer.h>
#include <rapidjson/document.h>
#include <vector>
#include <list>
#include <map>
#include <nghttp2/asio_http2_server.h>

#include "H2Server_Config_Schema.h"


using namespace rapidjson;

class H2Server_Request_Message
{
public:
    std::multimap<std::string, std::string> headers;
    rapidjson::Document  json_payload;
    std::map<size_t, bool> match_result;
    H2Server_Request_Message(const nghttp2::asio_http2::server::request& req)
    {
        json_payload.Parse(req.unmutable_payload().c_str());
        std::string path_header_name = ":path";
        headers.insert(std::make_pair(path_header_name, req.uri().path));
        for (auto& hdr : req.header())
        {
            headers.insert(std::make_pair(hdr.first, hdr.second.value));
        }
        if (debug_mode)
        {
            for (auto& header: headers)
            {
                std::cout<<"header: "<<header.first<<", value: "<<header.second<<std::endl;
            }
        }
    }
};

#endif

