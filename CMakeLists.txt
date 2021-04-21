cmake_minimum_required(VERSION 3.16)
project(avir)

set(CMAKE_CXX_STANDARD 14)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

set(OPENSSL_USE_STATIC_LIBS TRUE)
set(Boost_USE_STATIC_LIBS   ON)

find_package(OpenSSL REQUIRED)
find_package(Boost COMPONENTS iostreams filesystem REQUIRED)

include_directories(${Boost_INCLUDE_DIR})

link_directories(${Boost_LIBRARY_DIR})

add_executable(${PROJECT_NAME} src/main.cpp src/scan.cpp src/scan.h)

target_link_libraries(${PROJECT_NAME} OpenSSL::SSL)
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})

install (TARGETS avir DESTINATION bin)