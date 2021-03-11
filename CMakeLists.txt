cmake_minimum_required(VERSION 3.17)
project(Avir)

set(CMAKE_CXX_STANDARD 14)

#set(Boost_INCLUDE_DIR "C:\\Program Files\\boost_1_75_0")
#set(Boost_LIBRARY_DIR "C:\\Program Files\\boost_1_75_0\\stage\\lib")

#set(OPENSSL_USE_STATIC_LIBS TRUE)

find_package(OpenSSL REQUIRED)
find_package(Boost COMPONENTS iostreams filesystem REQUIRED)

include_directories(${Boost_INCLUDE_DIR})

link_directories(${Boost_LIBRARY_DIR})

add_executable(${PROJECT_NAME} main.cpp hash.cpp hash.h)

target_link_libraries(${PROJECT_NAME} OpenSSL::SSL)
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})