cmake_minimum_required(VERSION 3.20)
project(iot-core CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Drogon CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_library(TGBOT_LIB TgBot REQUIRED)
file(GLOB NOTIFY_SRC notifications/*.cc)
add_executable(iot-core
    main.cc
    controllers/ApiController.cpp
    services/ResourceService.cpp
    services/TelemetryService.cpp
    services/NotificationService.cpp
    repositories/UserRepository.cpp
    repositories/DeviceRepository.cpp
    repositories/TelemetryRepository.cpp
    repositories/HttpRepository.cpp
    notifications/TelegramNotifier.cpp
    notifications/EmailNotifier.cpp
)
target_include_directories(iot-core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(iot-core PRIVATE Drogon::Drogon
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
    ${TGBOT_LIB}
)
