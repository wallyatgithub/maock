#ifndef H2SERVER_REQUEST_MATCH_H
#define H2SERVER_REQUEST_MATCH_H

#include <rapidjson/pointer.h>
#include <rapidjson/document.h>
#include <vector>
#include <list>
#include <map>
#include <set>
#include "H2Server_Config_Schema.h"
#include "H2Server_Response.h"
#include "H2Server_Request_Message.h"

using namespace rapidjson;

class Match_Rule
{
public:
    enum Match_Type
    {
        EQUALS_TO = 0,
        START_WITH,
        END_WITH,
        CONTAINS
    };

    Match_Type match_type;
    std::string header_name;
    std::string json_pointer;
    std::string object;
    mutable uint64_t unique_id;
    Match_Rule(const Schema_Header_Match& header_match)
    {
        object = header_match.input;
        header_name = header_match.header;
        json_pointer = "";
        match_type = string_to_match_type[header_match.matchType];
    }
    Match_Rule(const Schema_Payload_Match& payload_match)
    {
        object = payload_match.input;
        header_name = "";
        json_pointer = payload_match.jsonPointer;
        match_type = string_to_match_type[payload_match.matchType];
    }

    std::map<std::string, Match_Type> string_to_match_type {{"EqualsTo", EQUALS_TO}, {"StartsWith", START_WITH}, {"EndsWith", END_WITH}, {"Contains", CONTAINS}};

    bool match(const std::string& subject, Match_Type verb, const std::string& object) const
    {
        if (debug_mode)
        {
            std::cout<<"subject: "<<subject<<", match type: "<<verb<<", object: "<<object<<std::endl;
        }
        switch (verb)
        {
            case EQUALS_TO:
            {
                return (subject == object);
            }
            case START_WITH:
            {
                return (subject.find(object) == 0);
            }
            case END_WITH:
            {
                return (subject.size() >= object.size() && 0 == subject.compare(subject.size() - object.size(), object.size(), object));
            }
            case CONTAINS:
            {
                return (subject.find(object) != std::string::npos);
            }
        }
        return false;
    }

    bool match(const std::string& subject) const
    {
        return match(subject, match_type, object);
    }

    bool match(const rapidjson::Document& d) const
    {
        return match(getJsonPointerValue(d, json_pointer), match_type, object);
    }

    bool match(H2Server_Request_Message& request) const
    {
        if (request.match_result.count(unique_id))
        {
            return request.match_result[unique_id];
        }
        else
        {
            bool matched = false;
            if (header_name.size())
            {
                auto header_val = request.headers.find(header_name);
                if (header_val != request.headers.end())
                {
                    matched = match(header_val->second);
                }
            }
            else
            {
                matched = match(request.json_payload);
            }
            request.match_result[unique_id] = matched;
            return matched;
        }
    }

    bool operator<(const Match_Rule& rhs) const
    {
        if (!header_name.empty() && rhs.header_name.empty())
        {
            return true;
        }
        else if (header_name.empty() && !rhs.header_name.empty())
        {
            return false;
        }
        else
        {
            std::string mine = std::to_string(match_type);
            mine.append(header_name);
            mine.append(json_pointer);
            mine.append(object);

            std::string other = std::to_string(rhs.match_type);
            other.append(rhs.header_name);
            other.append(rhs.json_pointer);
            other.append(rhs.object);
            return (mine < other);
        }
    }

};

class H2Server_Request
{
public:
    std::set<Match_Rule> match_rules;
    H2Server_Request(const Schema_Request_Match& request_match)
    {
        for (auto& schema_header_match : request_match.header_match)
        {
            match_rules.emplace(Match_Rule(schema_header_match));
        }

        for (auto& schema_payload_match : request_match.payload_match)
        {
            match_rules.emplace(Match_Rule(schema_payload_match));
        }
    }
    bool match(H2Server_Request_Message& request) const
    {
        for (auto& match_rule : match_rules)
        {
            if (!match_rule.match(request))
            {
                return false;
            }
        }
        return true;
    }
    bool operator<(const H2Server_Request& rhs) const
    {
        if (match_rules.size() < rhs.match_rules.size())
        {
            return true;
        }
        else if (match_rules.size() > rhs.match_rules.size())
        {
            return false;
        }
        else
        {
            auto match = match_rules.begin();
            auto other_match = rhs.match_rules.begin();
            while (match != match_rules.end() && other_match != rhs.match_rules.end())
            {
                if (*match < *other_match)
                {
                    return true;
                }
                else if (*other_match < *match)
                {
                    return false;
                }
                else
                {
                    match++;
                    other_match++;
                }
            }
        }
        return false;
    }
};



#endif
