cmake_minimum_required(VERSION 3.5.1)

project(processing_unit)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "Building processing_unit")
add_library(processing_unit
    include/processing_unit_server.hpp
    include/vms_agent.hpp
    include/osprey_ws_protocol.hpp
    include/concurrent_queue.hpp
    include/job_connection.hpp
    include/job.hpp
    include/job_connection_manager.hpp
    include/observable.hpp
    include/observables_resolver.hpp
    src/processing_unit_server.cpp
    src/vms_agent.cpp
    src/osprey_ws_protocol.cpp
    src/concurrent_queue.cpp
    src/job_connection.cpp
    src/job.cpp
    src/job_connection_manager.cpp
    src/observable.cpp
    )

    

set(3RDPARTY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# websockets backend
if (NOT TARGET simple-websocket-server)
    add_subdirectory(${3RDPARTY_PATH}/Simple-WebSocket-Server)
endif() 

add_subdirectory(${3RDPARTY_PATH}/spdlog)


source_group("source" REGULAR_EXPRESSION .*\\.cpp)
source_group("headers" REGULAR_EXPRESSION .*\\.h)

# Headers
set(HEADERS
    ${HEADERS}
    ${CMAKE_CURRENT_SOURCE_DIR}/include/processing_unit_server.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vms_agent.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/osprey_ws_protocol.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/concurrent_queue.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/job_connection.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/job.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/job_connection_manager.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/json/jsonconfig.hpp

    )
source_group("headers" FILES ${HEADERS})

# Source
set(SOURCE
    ${SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/processing_unit_server.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vms_agent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/osprey_ws_protocol.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/concurrent_queue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/job_connection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/job.cpp
    )

source_group("source" FILES ${SOURCE})

# ----------------------------------------------------------------------------
# Setup defines and compiler flags
# ----------------------------------------------------------------------------
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)


# Add definitions
if (WIN32)
  target_compile_definitions(simple-websocket-server INTERFACE BOOST_ALL_NO_LIB)
endif()

#USE_STANDALONE_ASIO
# Use standalone ASIO and turn off some Boost dependencies
target_compile_definitions(processing_unit PRIVATE ASIO_STANDALONE _WEBSOCKETPP_CPP11_TYPE_TRAITS_ ASIO_HAS_STD_ADDRESSOF ASIO_HAS_STD_SHARED_PTR ASIO_HAS_STD_ARRAY ASIO_HAS_CSTDINT ASIO_HAS_STD_TYPE_TRAITS)

# Enable C++11
set_property(TARGET processing_unit PROPERTY CXX_STANDARD 11)
set_property(TARGET processing_unit PROPERTY CXX_STANDARD_REQUIRED ON)

# Disable compiler specific extensions (e.g. -std=c++11 instead of -std=gnu++11)
set_property(TARGET processing_unit PROPERTY CXX_EXTENSIONS OFF)

find_package(Boost REQUIRED)

 # ----------------------------------------------------------------------------
 # GNU and Clang compiler specifics
 # ----------------------------------------------------------------------------
 if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
     target_compile_options(processing_unit PRIVATE -g -Wall -Wno-long-long -Wno-unknown-pragmas -Wno-unused-variable -ftemplate-backtrace-limit=0)
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(processing_unit PRIVATE -fvisibility=hidden)
endif()


target_link_libraries(processing_unit
    PRIVATE 
    simple-websocket-server
    spdlog
    )


target_include_directories(processing_unit
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/msgpack-c/include
)