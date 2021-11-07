#ifndef H2SERVER_CONFIG_SCHEMA_H
#define H2SERVER_CONFIG_SCHEMA_H

#include <iostream>

#include "staticjson/document.hpp"
#include "staticjson/staticjson.hpp"
#include "rapidjson/schema.h"
#include "rapidjson/prettywriter.h"

bool debug_mode = false;

class Schema_Header_Match
{
public:
    std::string matchType;
    std::string header;
    std::string input;
    void staticjson_init(staticjson::ObjectHandler* h)
    {
        h->add_property("matchType", &this->matchType);
        h->add_property("header-name", &this->header);
        h->add_property("input", &this->input);
    }
};

class Schema_Payload_Match
{
public:
    std::string matchType;
    std::string jsonPointer;
    std::string input;
    void staticjson_init(staticjson::ObjectHandler* h)
    {
        h->add_property("matchType", &this->matchType);
        h->add_property("JsonPointer", &this->jsonPointer);
        h->add_property("input", &this->input);
    }
};

class Schema_Request_Match
{
public:
    std::vector<Schema_Header_Match> header_match;
    std::vector<Schema_Payload_Match> payload_match;
    void staticjson_init(staticjson::ObjectHandler* h)
    {
        h->add_property("headers", &this->header_match);
        h->add_property("payload", &this->payload_match, staticjson::Flags::Optional);
    }
};

class Schema_Argument
{
public:
    std::string type_of_value;
    std::string value_identifier;
    int64_t substring_start;
    int64_t substring_length;
    explicit Schema_Argument()
    {
        substring_start = 0;
        substring_start = -1;
    }
    void staticjson_init(staticjson::ObjectHandler* h)
    {
        h->add_property("type-of-value", &this->type_of_value);
        h->add_property("value-identifier", &this->value_identifier, staticjson::Flags::Optional);
        h->add_property("sub-string-start", &this->substring_start, staticjson::Flags::Optional);
        h->add_property("sub-string-length", &this->substring_length, staticjson::Flags::Optional);
    }
};

class Schema_Response_Payload
{
public:
    std::string msg_payload;
    std::string placeholder;
    std::vector<Schema_Argument> arguments;
    void staticjson_init(staticjson::ObjectHandler* h)
    {
        h->add_property("msg-payload", &this->msg_payload);
        h->add_property("placeholder", &this->placeholder);
        h->add_property("arguments", &this->arguments);
    }
};

class Schema_Response_Header
{
public:
    std::string header;
    std::string placeholder;
    std::vector<Schema_Argument> arguments;
    void staticjson_init(staticjson::ObjectHandler* h)
    {
        h->add_property("header", &this->header);
        h->add_property("placeholder", &this->placeholder);
        h->add_property("arguments", &this->arguments);
    }
};


class Schema_Response_To_Return
{
public:
    uint32_t status_code;
    Schema_Response_Payload payload;
    std::vector<Schema_Response_Header> additonalHeaders;
    std::string luaScript;
    void staticjson_init(staticjson::ObjectHandler* h)
    {
        h->add_property("status-code", &this->status_code);
        h->add_property("payload", &this->payload, staticjson::Flags::Optional);
        h->add_property("additonalHeaders", &this->additonalHeaders, staticjson::Flags::Optional);
        h->add_property("luaScript", &this->luaScript, staticjson::Flags::Optional);
    }
};

class Schema_Service
{
public:
    Schema_Request_Match request;
    Schema_Response_To_Return response;
    void staticjson_init(staticjson::ObjectHandler* h)
    {
        h->add_property("Request", &this->request);
        h->add_property("Response", &this->response);
    }
};

class H2Server_Config_Schema
{
public:
    bool verbose;
    std::string address;
    uint32_t port;
    uint32_t threads;
    std::string private_key_file;
    std::string cert_file;
    std::string ca_cert_file;
    bool enable_mTLS;
    std::vector<Schema_Service> service;
    explicit H2Server_Config_Schema()
    {
        enable_mTLS = false;
        verbose = false;
    }
    void staticjson_init(staticjson::ObjectHandler* h)
    {
        h->add_property("verbose", &this->verbose, staticjson::Flags::Optional);
        h->add_property("address", &this->address);
        h->add_property("port", &this->port);
        h->add_property("threads", &this->threads);
        h->add_property("private-key-file", &this->private_key_file, staticjson::Flags::Optional);
        h->add_property("cert-file", &this->cert_file, staticjson::Flags::Optional);
        h->add_property("caCert-file", &this->ca_cert_file, staticjson::Flags::Optional);
        h->add_property("mTLS", &this->enable_mTLS, staticjson::Flags::Optional);
        h->add_property("Service", &this->service);
    }
};

#endif
