cmake_minimum_required( VERSION 3.0 )

project( CatchTests CXX )

message( STATUS "Catch tests" )

# Add catch as an interface library
set(CATCH_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/catch2)
add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE ${CATCH_INCLUDE_DIR})

# Add test executable
add_executable (tests testmain.cpp test.cpp)
target_link_libraries(tests Catch processing_unit)

# Enable C++11
set_property(TARGET tests PROPERTY CXX_STANDARD 11)
set_property(TARGET tests PROPERTY CXX_STANDARD_REQUIRED ON)
# Disable compiler specific extensions (e.g. -std=c++11 instead of -std=gnu++11)
set_property(TARGET tests PROPERTY CXX_EXTENSIONS OFF)