cmake_minimum_required(VERSION 3.20)
project(iot-core CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon CONFIG REQUIRED)
find_package(Threads REQUIRED)
find_package(PostgreSQL REQUIRED)  # libpq для Drogon ORM

set(SOURCES main.cc)
include_directories(controllers models)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
    Drogon::Drogon
    Threads::Threads
    PostgreSQL::PostgreSQL
)