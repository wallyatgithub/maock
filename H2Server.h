#ifndef H2SERVER_H
#define H2SERVER_H

#include <map>
#include "H2Server_Request.h"
#include "H2Server_Request_Message.h"

class H2Server_Response_Group
{
  public:
      std::vector<H2Server_Response> responses;

      void init_distribution_map_and_total_weight(const std::vector<Schema_Response_To_Return>& responses_schema)
      {
          for (auto i = 0; i < responses_schema.size(); i++)
          {
              responses.emplace_back(H2Server_Response(responses_schema[i]));
          }

          uint64_t totalWeight = 0;
          for (size_t index = 0; index < responses.size(); index++)
          {
              auto& resp = responses[index];
              totalWeight += responses[index].weight;
              distribution_map[totalWeight] = index;
          }
          total_weight = distribution_map.rbegin()->first;
      }
      H2Server_Response_Group(const std::vector<Schema_Response_To_Return>& responses_schema)
      :generator((std::random_device())())
      {
          init_distribution_map_and_total_weight(responses_schema);
          distr.param(std::uniform_int_distribution<>::param_type(0, total_weight - 1));
      }
      size_t select_response()
      {
          size_t response_index = 0;
          if (responses.size() > 1)
          {
              uint64_t randomNumber = distr(generator);
              auto iter = distribution_map.upper_bound(randomNumber);
              if (iter != distribution_map.end())
              {
                  response_index = iter->second;
              }
              if (debug_mode)
              {
                  std::cout<<"randomNumber: " << randomNumber << ", response index: "<<response_index<<std::endl;
              }
          }
          return response_index;
      }
private:
    std::map<uint64_t, size_t> distribution_map;
    uint64_t total_weight;
    std::mt19937_64 generator;
    std::uniform_int_distribution<int>  distr;
};

class H2Server_Service
{
public:
    H2Server_Request request;
    H2Server_Response_Group response_group;
    H2Server_Service(const Schema_Service& service):
        request(service.request),
        response_group(service.responses)
    {
    }
};

std::ostream& operator<<(std::ostream& o, const H2Server_Request& request)
{
    o << "H2Server_Request:" << std::endl;
    for (auto& match : request.match_rules)
    {
        o << "match_rule: " << match.match_type
          << ", " << match.header_name
          << ", " << match.json_pointer
          << ", " << match.object
          << ", " << match.unique_id
          << std::endl;
    }
    return o;
}

std::ostream& operator<<(std::ostream& o, const H2Server_Response& response)
{
    o << "H2Server_Response:" << std::endl;

    o << "status_code:" << response.status_code << std::endl;
    for (auto& header : response.additonalHeaders)
    {
        std::cout<<"header:"<<std::endl;
        for (auto& token : header.tokenizedHeader)
        {
            o << token << " ";
        }
        o << std::endl;

        o << "header_arguments: ";
        for (auto& arg : header.header_arguments)
        {
            o << "json_pointer: " << arg.json_pointer << std::endl;
            o << "header name: " << arg.header_name << std::endl;
            o << "substring_start: " << arg.substring_start << std::endl;
            o << "substring_length: " << arg.substring_length << std::endl;
        }
    }
    o << "payload: ";
    for (auto& token : response.tokenizedPayload)
    {
        o << token << " ";
    }
    o << std::endl;

    o << "payload_arguments: ";
    for (auto& arg : response.payload_arguments)
    {
        o << "json_pointer: " << arg.json_pointer << std::endl;
        o << "header name: " << arg.header_name << std::endl;
        o << "substring_start: " << arg.substring_start << std::endl;
        o << "substring_length: " << arg.substring_length << std::endl;
    }

    return o;
}


class H2Server
{
public:
    std::map<H2Server_Request, H2Server_Response_Group> services;

    void build_match_rule_unique_id(std::map<H2Server_Request, H2Server_Response_Group>& services)
    {
        std::set<Match_Rule> all_match_rules;
        for (auto& each_service : services)
        {
            for (auto& match_rule : each_service.first.match_rules)
            {
                all_match_rules.insert(match_rule);
            }
        }

        std::map<H2Server_Request, H2Server_Response_Group> new_services;
        for (auto& each_service : services)
        {
            for (auto& match_rule : each_service.first.match_rules)
            {
                match_rule.unique_id = std::distance(all_match_rules.begin(), all_match_rules.find(match_rule));
            }
            new_services.insert(std::make_pair(each_service.first, each_service.second));
        }
        services.swap(new_services);
    }

    H2Server(const H2Server_Config_Schema& config_schema)
    {
        for (auto& service_in_config_schema : config_schema.service)
        {
            H2Server_Service service(service_in_config_schema);
            services.insert(std::make_pair(std::move(service.request), std::move(service.response_group)));
        }
        build_match_rule_unique_id(services);
    }

    H2Server_Response* get_response_to_return(H2Server_Request_Message& msg)
    {
        for (auto iter = services.rbegin(); iter != services.rend(); iter++)
        {
            if (debug_mode)
            {
                std::cout<<"checking request: "<<iter->first<<std::endl;
            }
            if (iter->first.match(msg))
            {
                size_t index = iter->second.select_response();
                if (debug_mode)
                {
                    std::cout<<"find matched response: "<<iter->second.responses[index]<<std::endl;
                }
                return &iter->second.responses[index];
            }
        }
        return nullptr;
    }

};


#endif

