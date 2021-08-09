#ifndef H2SERVER_RESPONSE_H
#define H2SERVER_RESPONSE_H

#include <rapidjson/pointer.h>
#include <rapidjson/document.h>
#include <vector>
#include <list>
#include <random>
#include <map>
#include "H2Server_Config_Schema.h"
#include "H2Server_Request_Message.h"
#include <rapidjson/writer.h>

using namespace rapidjson;

const std::string extended_json_pointer_indicator = "/~#";

std::vector<std::string> tokenize_string(const std::string& source, const std::string& delimeter)
{
    std::vector<std::string> retVec;
    size_t start = 0;
    size_t delimeter_len = delimeter.length();
    if (!delimeter.empty())
    {
        size_t pos = source.find(delimeter, start);
        while (pos != std::string::npos)
        {
            retVec.emplace_back(source.substr(start, (pos - start)));
            start = pos + delimeter_len;
            pos = source.find(delimeter, start);
        }
        retVec.emplace_back(source.substr(start, std::string::npos));
    }
    else
    {
        retVec.emplace_back(source);
    }
    return retVec;
}

template<typename RapidJsonType>
const rapidjson::Value* getNthValue(const RapidJsonType& d, int64_t n)
{
    if (d.MemberCount() && n > 0)
    {
        int64_t index = 1;
        for (rapidjson::Value::ConstMemberIterator itr = d.MemberBegin();
            itr != d.MemberEnd();)
        {
            if (index == n)
            {
                return &(itr->value);
            }
            ++index;
            ++itr;
        }
    }
    else if (d.MemberCount() && n < 0)
    {
        if ((0 - n) == d.MemberCount())
        {
            return &(d.MemberBegin()->value);
        }
        else
        {
            int64_t index = -1;
            rapidjson::Value::ConstMemberIterator itr = d.MemberEnd();
            itr--;
            while (itr != d.MemberBegin())
            {
                if (index == n)
                {
                    return &(itr->value);
                }
                itr--;
                index--;
            }
        }

    }
    return nullptr;
}

std::vector<std::string> splitComplexJsonPointer(const std::string& json_pointer)
{
    std::vector<std::string> JsonPointers;
    std::string extended_json_pointer_indicator = "/~#";
    size_t start_pos = 0;
    while (start_pos != std::string::npos)
    {
        size_t end_pos = json_pointer.find(extended_json_pointer_indicator, start_pos);
        if (end_pos - start_pos)
        {
            JsonPointers.emplace_back(json_pointer.substr(start_pos, (end_pos - start_pos)));
        }
        if (end_pos != std::string::npos)
        {
            size_t nextJsonPtrPos = json_pointer.find("/", end_pos+1);
            JsonPointers.emplace_back(json_pointer.substr(end_pos, (nextJsonPtrPos - end_pos)));
            start_pos = nextJsonPtrPos;
        }
        else
        {
            break;
        }
    }
    if (debug_mode)
    {
        std::cout<<"ComplexJsonPointer:"<<json_pointer<<std::endl;
        for (auto& ptr: JsonPointers)
        {
            std::cout<<"Simple JsonPointer:"<<ptr<<std::endl;
        }
    }
    return JsonPointers;
}

template<typename RapidJsonType>
std::string convertRapidVJsonValueToStr(RapidJsonType* value)
{
    if (value)
    {
        if (value->IsString())
        {
            return value->GetString();
        }
        else if (value->IsBool())
        {
            return value->GetBool() ? "true" : "false";
        }
        else if (value->IsUint64())
        {
            return std::to_string(value->GetUint64());
        }
        else if (value->IsDouble())
        {
            return std::to_string(value->GetDouble());
        }
        else if (value->IsObject())
        {
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            value->Accept(writer);
            return std::string(buffer.GetString());
        }
    }
    return "";
}


template<typename RapidJsonType>
std::string getValueFromJsonPtr(const RapidJsonType& d, const std::string& json_pointer)
{
    std::vector<std::string> pointers = splitComplexJsonPointer(json_pointer);
    const rapidjson::Value* value = &d;
    for (size_t vectorIndex = 0; vectorIndex < pointers.size(); vectorIndex++)
    {
        if (pointers[vectorIndex].find(extended_json_pointer_indicator) != std::string::npos)
        {
            try
            {
                int64_t index = std::stoi(pointers[vectorIndex].substr(extended_json_pointer_indicator.size()));
                value = getNthValue(*value, index);
            }
            catch (std::invalid_argument& e)
            {
                std::cerr<<"invalid_argument: "<<pointers[vectorIndex]<<std::endl;
                exit(1);
            }
            catch (std::out_of_range& e)
            {
                std::cerr<<"out_of_range: "<<pointers[vectorIndex]<<std::endl;
                exit(1);
            }
        }
        else
        {
            rapidjson::Pointer ptr(pointers[vectorIndex].c_str());
            value = ptr.Get(*value);
        }
        if (debug_mode)
        {
            std::cout<<"json pointer:"<<pointers[vectorIndex]<<", value:"<<convertRapidVJsonValueToStr(value)<<std::endl;
        }

        if (!value)
        {
            break;
        }
    }
    return convertRapidVJsonValueToStr(value);
}

template<typename RapidJsonType>
std::string getJsonPointerValue(const RapidJsonType& d, const std::string& json_pointer)
{
    return convertRapidVJsonValueToStr(getValueFromJsonPtr(d, json_pointer));
}

class Argument
{
public:
    std::string json_pointer;
    int64_t substring_start;
    int64_t substring_length;
    std::string header_name;
    bool random_hex;
    Argument(const Schema_Argument& payload_argument)
    {
        if (payload_argument.type_of_value == "JsonPointer")
        {
            json_pointer = payload_argument.value_identifier;
            header_name = "";
            random_hex = false;
        }
        else if (payload_argument.type_of_value == "Header")
        {
            json_pointer = "";
            random_hex = false;
            header_name = payload_argument.value_identifier;
        }
        else if (payload_argument.type_of_value == "RandomHex")
        {
            header_name = "";
            json_pointer = "";
            random_hex = true;
        }
        substring_start = payload_argument.substring_start;
        substring_length = payload_argument.substring_length;
    }
    std::string getValue(const H2Server_Request_Message& msg) const
    {
        std::string str;
        if (json_pointer.size())
        {
            str = getValueFromJsonPtr(msg.json_payload, json_pointer);
        }
        else if (header_name.size()&&msg.headers.count(header_name))
        {
            str = msg.headers.find(header_name)->second;
        }
        else if (random_hex)
        {
            static std::random_device              rd;
            static std::mt19937                    gen(rd());
            static std::uniform_int_distribution<> dis(0, 15);
            std::stringstream stream;
            stream << std::hex << dis(gen);
            str = stream.str();
        }

        if (debug_mode)
        {
            std::cout<<"json_pointer: "<<json_pointer<<std::endl;
            std::cout<<"header_name: "<<header_name<<std::endl;
            std::cout<<"random_hex: "<<random_hex<<std::endl;
            std::cout<<"target string: "<<str<<std::endl;
            std::cout<<"substring_start: "<<substring_start<<std::endl;
            std::cout<<"substring_length: "<<substring_length<<std::endl;
        }


        if ((substring_start > 0) || (substring_length != -1))
        {
            return str.substr(substring_start, substring_length);
        }
        else
        {
            return str;
        }
    }
};

class H2Server_Response_Header
{
public:
    std::vector<std::string> tokenizedHeader;
    std::vector<Argument> header_arguments;
    H2Server_Response_Header(const Schema_Response_Header& schema_header)
    {
        size_t t = schema_header.header.find(":", 1);
        if ((t == std::string::npos) ||
            (schema_header.header[0] == ':' && 1 == t))
        {
            std::cerr << "invalid header, no name: " << schema_header.header << std::endl;
            abort();
        }
        std::string header_name = schema_header.header.substr(0, t);
        std::string header_value = schema_header.header.substr(t + 1);

        if (header_value.empty())
        {
            std::cerr << "invalid header - no value: " << schema_header.header
                      << std::endl;
            exit(1);
        }

        tokenizedHeader = tokenize_string(schema_header.header, schema_header.placeholder);
        for (auto& arg : schema_header.arguments)
        {
            header_arguments.emplace_back(Argument(arg));
        }
        if (tokenizedHeader.size() - header_arguments.size() != 1)
        {
            std::cerr << "number of placeholders does not match number of arguments:" << staticjson::to_pretty_json_string(
                          schema_header) << std::endl;
            exit(1);
        }
    }
};
class H2Server_Response
{
public:
    uint32_t status_code;
    std::vector<H2Server_Response_Header> additonalHeaders;
    std::vector<std::string> tokenizedPayload;
    std::vector<Argument> payload_arguments;

    H2Server_Response(const Schema_Response_To_Return& resp)
    {
        status_code = resp.status_code;
        tokenizedPayload = tokenize_string(resp.payload.msg_payload, resp.payload.placeholder);
        for (auto& arg : resp.payload.arguments)
        {
            payload_arguments.emplace_back(Argument(arg));
        }
        if (tokenizedPayload.size() - payload_arguments.size() != 1)
        {
            std::cerr << "number of placeholders does not match number of arguments:" << staticjson::to_pretty_json_string(
                          resp) << std::endl;
            exit(1);
        }
        for (auto& header: resp.additonalHeaders)
        {
            additonalHeaders.emplace_back(H2Server_Response_Header(header));
        }
    }

    std::string produce_payload(const H2Server_Request_Message& msg) const
    {
        std::string payload;
        for (size_t index = 0; index < tokenizedPayload.size(); index++)
        {
            payload.append(tokenizedPayload[index]);
            if (index < payload_arguments.size())
            {
                payload.append(payload_arguments[index].getValue(msg));
            }
        }
        return payload;
    }

    std::pair<std::string, std::string> produce_header(const H2Server_Response_Header& header_with_var, const H2Server_Request_Message& msg) const
    {
        std::string header_with_value;
        for (size_t index = 0; index < header_with_var.tokenizedHeader.size(); index++)
        {
            header_with_value.append(header_with_var.tokenizedHeader[index]);
            if (index < header_with_var.header_arguments.size())
            {
                header_with_value.append(header_with_var.header_arguments[index].getValue(msg));
            }
        }

        size_t t = header_with_value.find(":", 1);
        std::string header_name = header_with_value.substr(0, t);
        std::string header_value = header_with_value.substr(t + 1);
        /*
        header_value.erase(header_value.begin(), std::find_if(header_value.begin(), header_value.end(),
                                                              [](unsigned char ch)
        {
            return !std::isspace(ch);
        }));
        */
        return std::make_pair<std::string, std::string>(std::move(header_name), std::move(header_value));

    }

    std::map<std::string, std::string> produce_headers(const H2Server_Request_Message& msg) const
    {
        std::map<std::string, std::string> headers;

        for (auto& header_with_var : additonalHeaders)
        {
            auto header_with_value = produce_header(header_with_var, msg);

            headers.insert(header_with_value);
        }
        return headers;
    }
};


#endif

