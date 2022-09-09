[![license](https://img.shields.io/github/license/wallyatgithub/maock.svg?style=flat-square)](https://github.com/wallyatgithub/maock)

*Read this in other languages: [English](README.md).*

# Maock 是一个http1.x / http2 mock server
  
  Maock从nghttp2的asio-sv fork而来，并加入了可配置的接口，以及http1.x的支持
  
  通过不同的配置文件，Maock能够模拟多种http2的服务，而不需要使用者具备编程技能。
  
  当然，Maock也提供了可编程接口，用户可以通过Lua脚本定制响应消息的header和消息体。
  
  Maock支持mTLS。
  
  Maock原生支持Linux和Windows

# 基本用法与原理

  Maock通过Json配置文件来定制需要模拟的服务。
  
  定制的基本过程：
  
  首先，打开一个图形界面的Json编辑器，比如https://pmk65.github.io/jedemov2/dist/demo.html
  
  然后把Maock的Json schema文件https://raw.githubusercontent.com/wallyatgithub/maock/main/H2Server_Config_Schema.json

  粘贴进Json编辑器的“Schema”标签页的编辑框内，并点击“Generate Form”按钮，得到一个名为h2server_configuration的表单，
  
  按照表单的每一项的提示，编辑表单。
  
  基本原理：
  
  Maock以Service来组织需要被模拟的服务。每一个Service包含一个Request，和一组Response
  
  Request定义了一系列匹配规则， 
  
  匹配规则可以是对进入的请求消息的header的匹配，比如，某一名称的header的值，包含或者等于某一字符串，或者以某字符串开头/结尾，
  
  匹配规则也可以是对进入的请求消息的消息体的匹配，当前支持的消息体的格式是Json格式。用Json Pointer（rfc6901），我们可以很直接的定位到Json消息中某一个特定的值，
  
  所以消息体的匹配规则就是，某一Json pointer指向的Json消息体的值，包含或者等于某一个字符串，或者以某字符串开头/结尾。
  
  当进入的请求消息满足某一个Request定义的所有匹配规则的时候，则该Request所在的Service中定义的Response，会被用来生成对该请求消息的应答。
  
  很自然的，定义若干组Service让Maock加载是允许的，Maock在运行的时候，对于一个进入的请求消息，会优先选择最优的匹配，来确定匹配到的Request，并用对应的Response来生成应答。
  
  所谓最优匹配，指的是，当有两个或者以上的Request可以匹配进入的请求消息的时候，那么，包含更多匹配规则的Request，会被选中，所以，最优匹配本质上就是一种最精确匹配。
  
  比如，一个Request只包含一个对:path header的匹配，并可以与进入的请求消息匹配成功，
  
  而另一个Request既包含对:path的匹配，又包含对Json消息体的匹配，并且:path和Json消息皆可以与进入的请求消息成功匹配，相比上面那个只有一条匹配规则的Request，这个Request就是更优匹配，如果没有比它更优的，它就是最优匹配。
  
  所以,这个最优匹配的Request对应的Response列表中的某一个Response，就会被用来生成最终的应答消息。
  
  而具体选择Response列表中哪一个Response，则是根据Response列表中各自的权重来决定，当然，对一个Request，可以只定义一个Response，这样就会选择这个唯一的Response
     
  Response定义了几项基本内容：
  
  1. 应答的status-code，这个顾名思义，如201 Created， 200 OK，等等
  
  2. 应答的消息体payload，其可以是一段Json数据，也可以是一段其它文本数据。如msg-payload处的注解所言，此处填的payload可以带有变量占位符，每一个占位符对应后续一个argument
  
     每一个argument负责生成一个字符串，用来替换msg-payload中对应序号的占位符，即，第一个argument生成的字符串替换第一个占位符，第二个argument替换第二个占位符，以此类推
     
     argument生成字符串的来源可以有几种：
     
     从进入消息的header的值来获得，比如:path header的值
     
     从进入消息的Json消息体获得，用Json pointer来指定Json消息体中某一个特定的值
     
     上述二者取得目的值之后，可以有一个附加的可选的动作，来截取该值的一个子串，比如可以截取:path header的一部分
     
     第三种生成字符串的途径，是产生一个16进制的随机数。通过串联一细列的16进制随机数，这个功能可以用来生成一个类UUID的字符串，比如生成一个应答消息中的subscription Id
     
     更多生成字符串的途径，可能会根据需求陆续添加，就不在这里逐一描述了，详情请参见h2server_configuration的表单中对应条目的帮助信息
     
  3. 应答消息的header
  
     每一个header的格式是一个用冒号分隔的字符串，冒号之前是header的名字，之后值header的值。
     
     如payload一样，每header的值也可以带有占位符和变量，原理和上述payload中描述的完全一致
     
  4. 如有需要，Maock还可以加载一个Lua脚本，来对上述生成的header和payload做更进一步的定制。     
    
     不同的Service的"Response"可以有不同的customize_response函数，以实现最佳灵活的需求。

     如果需要应用这个功能，则需要提供一段带有名为customize_response函数的Lua脚本给"Response"的"luaScript"配置部分，该函数读取四个输入参数：request_headers (table), request_payload, response_headers (table), response_payload
     
     然后该函数可以根据特定的需要，对response_headers和response_payload做特定的修改，然后返回它们两个给Maock。
     
     Maock会以更新过的内容作为应答响应返回。
     
     下面是该函数的一个例子:
     
    function customize_response(request_header, request_payload, response_headers_to_send, response_payload_to_send)
        response_headers_to_send["test"] = "test_value"
        response_payload_to_send = "hello lua"
        return response_headers_to_send, response_payload_to_send
    end
    
  
  编辑表单结束之后，从output标签页的编辑框中复制出生成的Json文本，并存入一个文件，比如maock.json
  
  用这个Json文件作为输入，启动Maock:
  
    maock maock.json
 
  除了前述的Json编辑器之外，下面的几个Json编辑器也是不错的选择，请随意挑选：
 
  https://rjsf-team.github.io/react-jsonschema-form/

  http://brutusin.org/json-forms/ 
  
# 如何快速尝试一下

  从 https://github.com/wallyatgithub/maock/releases 下载预编译好的Linux或者Windows 10的Maock可执行文件的压缩包
  
  解压，然后运行:

  ./maock ./maock.json # for linux
  
  or
  
  maock.exe maock.json # for windows 10
  
  Maock现在在8081端口监听，并且已经加载了预定义好的规则
 
  从 https://github.com/wallyatgithub/h2loadrunner/releases 下载预编译好的Linux或者Windows 10的H2loadrunner可执行文件的压缩包
  
  解压，然后运行:
  
  ./h2loadrunner --config-file=./h2load.json # for linux
  
  or
  
  h2loadrunner.exe --config-file=h2load.json # for windows 10
  
  现在，H2loadrunner已经启动，并向127.0.0.1:8081的Maock建立了两条连接，每条连接的测试流量是每秒100个请求

  请查看Maock和Hloadrunner的各自屏幕输出，来获得当前的流量统计信息
  
  
# 如何从源代码构建Maock：

  Maock提供了两个dockerfile，
  
  一个用来生成CentOS7为基础的docker镜像：Dockerfile_CentOS7
  
  一个用来生成以Ubuntu为基础的docker镜像：Dockerfile_Ubuntu

# 如何在Windows平台上从源代码生成Maock的可执行文件：

  首先，需要安装cmake 3.20或者更高的版本，Visual Studio 2022 MSVC x86/x64 build tool，以及Windows 10 SDK

  然后，安装vcpkg，请按照https://vcpkg.io/en/getting-started.html 进行vcpkg的安装
  
  用vcpkg安装下面的依赖包:
  
    boost:x64-windows
    getopt:x64-windows
    openssl:x64-windows
    luajit:x64-windows
    nghttp2:x64-windows
    rapidjson:x64-windows
  
  下载Maock的源码，用网页下载maock源码的zip包然后解压，或者git clone，都可以，比如，下载位置是c:\tmp
  
  接下来, 
  
    从开始菜单打开 "x64 Native Tools Command Prompt"
    
    C:\tmp>cd maock
    
    C:\tmp\maock>mkdir build
    
    C:\tmp\maock>cd build
    
    C:\tmp\maock\build>cmake ../ -DCMAKE_TOOLCHAIN_FILE=_REPLACE_THIS_WITH_YOUR_VCPKG_PATH_\scripts\buildsystems\vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
    
    注意，请用vcpkg的实际安装路径替换 _REPLACE_THIS_WITH_YOUR_VCPKG_PATH_
    
    可选步骤：
    如果想要链接到静态库，请在上述cmake命令之后附加下面的参数
    -DVCPKG_TARGET_TRIPLET=x64-windows-static 
    注意，这意味着需要安装上述的依赖包的x64-windows-static triplet
    
    C:\tmp\maock\build>cmake --build ./ --config=Release
  
  等待一会，上述命令会生成maock.exe
  
  
# Maock的吞吐量有多大：

  对应不同的复杂程度的配置，Maock执行的逻辑并不一致，所以吞吐量也会随之变化，此处给出一个典型的例子供参考：
  
  对于下面一个例子：
  
    对于进入的请求消息，包含三个匹配规则：:path header，user-agent header，以及Json消息体中的某一个值
  
    对于生成的响应，包含三个可变header，三个header总共含有四个变量，其中三个是从进入的请求消息取值，另一个是一个随机数变量
  
  上述例子，处理20K TPS，只需要占用一个vCPU
  
  而Maock可以启动多个线程， 所以在多核系统上，吞吐量可以线性扩展
  
