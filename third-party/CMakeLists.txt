set(LIBLLHTTP_SOURCES
  llhttp/src/api.c
  llhttp/src/http.c
  llhttp/src/llhttp.c
)

IF(WIN32)
    add_definitions(-DNOMINMAX=1)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    #add_definitions(-D_ITERATOR_DEBUG_LEVEL=0)
endif()

if(VCPKG_TARGET_TRIPLET MATCHES "${__ARCH}-windows-static")
    if(MSVC)
        add_compile_options(
            $<$<CONFIG:>:/MT>
            $<$<CONFIG:Debug>:/MTd>
            $<$<CONFIG:Release>:/MT>
        )
    endif()
endif()

find_path(RAPIDJSON_INCLUDE_DIR rapidjson/rapidjson.h)

add_library(llhttp OBJECT ${LIBLLHTTP_SOURCES})
target_include_directories(llhttp PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/llhttp/include"
)
set_target_properties(llhttp PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)

set(LIBURL_PARSER_SOURCES
  url-parser/url_parser.c
)
add_library(url-parser OBJECT ${LIBURL_PARSER_SOURCES})
set_target_properties(url-parser PROPERTIES
  POSITION_INDEPENDENT_CODE ON)

if(HAVE_NEVERBLEED)
  set(NEVERBLEED_SOURCES
    neverbleed/neverbleed.c
  )
  add_library(neverbleed ${NEVERBLEED_SOURCES})
  target_include_directories(neverbleed PRIVATE ${OPENSSL_INCLUDE_DIRS})
  target_include_directories(neverbleed INTERFACE
    "${CMAKE_SOURCE_DIR}/third-party/neverbleed"
  )
  target_link_libraries(neverbleed ${OPENSSL_LIBRARIES})
endif()

if(HAVE_MRUBY)
  # EXTRA_DIST = build_config.rb mruby/*

  set(MRUBY_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/mruby/build")
  set(MRUBY_LIBRARY
    "${CMAKE_STATIC_LIBRARY_PREFIX}mruby${CMAKE_STATIC_LIBRARY_SUFFIX}"
  )

  # The mruby build needs some env vars. Alternatively, look at cmake -P
  if(CMAKE_VERSION VERSION_LESS "3.1")
    # XXX works only for Unixes?
    set(ENV_COMMAND env)
  else()
    set(ENV_COMMAND ${CMAKE_COMMAND} -E env)
  endif()
  # Required for the Ninja generator. For older CMake, you first have to
  # invoke 'ninja mruby' before building dependents.
  if(CMAKE_VERSION VERSION_LESS "3.2")
    set(_byproducts)
  else()
    set(_byproducts BYPRODUCTS "mruby/build/lib/${MRUBY_LIBRARY}")
  endif()
  add_custom_target(mruby
    COMMAND ${ENV_COMMAND}
      "MRUBY_CONFIG=${CMAKE_CURRENT_SOURCE_DIR}/build_config.rb"
      "BUILD_DIR=${MRUBY_BUILD_DIR}"
      "INSTALL_DIR=${MRUBY_BUILD_DIR}/install/bin"
      "CC=${CMAKE_C_COMPILER}" "CXX=${CMAKE_CXX_COMPILER}"
      "${CMAKE_CURRENT_SOURCE_DIR}/mruby/minirake"
      -f "${CMAKE_CURRENT_SOURCE_DIR}/mruby/Rakefile"
    ${_byproducts}
    VERBATIM
  )

  # Make the mruby library available to others in this project without them
  # having to worry about include dirs and the mruby location.
  add_library(mruby-lib STATIC IMPORTED GLOBAL)
  set_target_properties(mruby-lib PROPERTIES
    IMPORTED_LOCATION "${MRUBY_BUILD_DIR}/lib/${MRUBY_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/mruby/include"
  )
  add_dependencies(mruby-lib mruby)

  set_directory_properties(PROPERTIES
    ADDITIONAL_MAKE_CLEAN_FILES mruby/build
  )
endif()

include_directories(
"${CMAKE_CURRENT_SOURCE_DIR}/"
"${CMAKE_CURRENT_SOURCE_DIR}/StaticJSON/include"
"${RAPIDJSON_INCLUDE_DIR}"
)

set(LIBSTATICJSON_SOURCE
StaticJSON/src/staticjson.cpp
)
add_library(staticjson OBJECT ${LIBSTATICJSON_SOURCE})
set_target_properties(staticjson PROPERTIES
POSITION_INDEPENDENT_CODE ON)

