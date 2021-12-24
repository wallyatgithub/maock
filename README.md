[![license](https://img.shields.io/github/license/wallyatgithub/maock.svg?style=flat-square)](https://github.com/wallyatgithub/maock)

*Read this in other languages: [简体中文](README.zh-cn.md).*

# Maock is an http2 mock server
  
  Maock was forked from asio-sv of nghttp2, with configuration interface added, to enable it to mock many http2 services with simple configuration, while for most cases, it does not require any programing skills from the end user.
 
  Maock supports programing interface as well, to further customized the response message with Lua.
  
  Maock support mTLS.
  
# How Maock works 

  Maock supports Json file to customize mock services.
 
  First, open a GUI Json editor, for example, https://pmk65.github.io/jedemov2/dist/demo.html
 
  Then paste Maock Json schema file https://raw.githubusercontent.com/wallyatgithub/maock/main/H2Server_Config_Schema.json
  
  into the text box under "Schema" tab, and then click "Generate Form" button, which would produce a form named h2server_configuration
 
  Edit the form; check the text under each field for the description of each field.
  
  Configuration overview：
 
  In Maock configuration, each mock service is represented by a "Service" entry.
  
  Each Service has a "Request", and a "Response".
  
  Each "Request" defines a set of rule to match the incoming request message.
  
  A match rule could be:
  
  Header value match, i.e., a certain header value starts with / ends with / contains / equals to some certain keyword.
   
  Payload match. With Json pointer, Maock can locate a value in the incoming Json payload, so a payload match rule could be:
  
  Some value (identified by a certain Json pointer) in the Json payload starts with / ends with / contains / equals to some certain keyword.
  
  When the incoming request message passes all the matching rules that a "Request" defines, the "Response" associated with the "Request", may be chosen to generate the response message.
  
  It is possible to define and load multiple "Service" entries, and Maock will chose the one that best fits to find the corresponding "Response" to generate the response message.
  
  Best fit, means, when there are more than one "Requst" can match the incoming request message, then the "Request" with more matching rules will be chosen as matched, and thus the corresponding "Response" is finally picked up to generate the outgoing response messsage.
  
  "Response" has a list of configuration fields available to customize the actual response:

  1. status-code, such as 201, 200, 204, etc.
 
  2. The outgoing message payload; it could be a piece of Json data, or some data in other format. 
  
     As told by the description of the msg-payload field, msg-payload can have placeholders for variables.
     
     Each placeholder is associated with an argument that follows in "Arguments" field.
    
     Each argument produces a string, to replace the placeholder in the payload, i.e., string generated by first argument would replace the first placeholder occurance, and that of the second argument would replace the second occurance, and so on.
     
     There are different sources from which an argument produce the string:
     
     From a certain header value of the incoming message, such as the :path header value
     
     From a certain value (identified by Json pointer) in the Json payload of the incoming message
     
     A sub string action can be done to the target value above, to get only a portion of the string. For example, to get the user Id from the :path header.
     
     Other sources to produce a string might be added when needed. Please refer to the scheme to get a complete list.
     
  3. Headers of the outgoing response message.
  
     A header is defined as a string, with colon (:) as the delimeter, with header name before the colon and header value following the colon.
     
     Like payload, a header value can also have placeholders for variable replacement by argument. It works exactly the same with that of payload.
     
  4. A Lua script to further customize the response headers and response payload generated above.

     If this is desired, a piece of Lua script, or a file containing the script, should be provided to "luaScript" field of "Response". 

     The script should have a function named "customize_response", which takes 4 arguments: request_headers (table), request_payload, response_headers (table), response_payload
     
     The script can define other lua functions, and function "customize_response" can call that. It can also "require" another lua module, the respective lua file required can be placed in the same directory of maock executable, or any other directory that is within LUA_PATH

     The customize_response function can make whatever update to response_headers and response_payload, and then return them two.
     
     Maock will pick up the updated response_headers and response_payload, and send back in the response message.
     
     Different "Response" can have different customize_response scripts for best flexibility.
    
     An example of the customize_response function:
     
    function customize_response(request_header, request_payload, response_headers_to_send, response_payload_to_send)
        response_headers_to_send["test"] = "test_value"
        response_payload_to_send = "hello lua"
        return response_headers_to_send, response_payload_to_send
    end

  After finish editing the form, copy the data of the left edit box under the "Output" tab, and save into a file, such as maock.json
  
  Then start Maock with maock.json as the input: 
 
    maock maock.json
  
  Other online Json editors available:

  https://rjsf-team.github.io/react-jsonschema-form/

  http://brutusin.org/json-forms/ 
  

# How to build Maock from the source：

  Maock has dependency on openssl, rapidJson, boost, nghttp2, libluajit-5.1-dev.
  
  Install the dependencies with package managers like yum or apt, and then use cmake to build Maock.
  
  Maock also provides 2 docker files to build docker images with Maock.
  
  Dockerfile_CentOS7: CentOS 7 based docker image
  
  Dockerfile_Ubuntu: Ubuntu latest based docker image

  
# What is the throughput of Maock:

  For different configurations, less or more "Service", simple or complex matching rules or arguments, Maock behaves differently, so the throughput may vary.
  
  Here the performance data of such a typical configuration is given:
  
  Configuration：
  
    The "Request" has a match rule to :path header, a match rule to user-agent header, and a match rule to the Json message payload.
  
    The "Response" has three headers with four arguments in total, in which three arguments are based on incoming message, and the rest is a random hex.
  
  With such configuration, Maock can reach 20K TPS with only 1 vCPU.
  
  While Maock supports multi-threading, so its throughput can linearly grow with multicore.
 
