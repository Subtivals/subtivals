cmake_minimum_required(VERSION 3.21)
project(RemoteScreensUiDemo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(WEB_RESOURCES resources.qrc)

find_package(Qt6 6.8 REQUIRED COMPONENTS Widgets Network WebSockets HttpServer)

add_executable(${PROJECT_NAME}
    main.cpp
    RemoteScreensDialog.h
    RemoteScreensDialog.cpp
    RemoteScreensDialog.ui
    RemoteScreensService.h
    RemoteScreensService.cpp
    RemoteScreensTypes.h
    # (MIT) https://www.nayuki.io/page/qr-code-generator-library
    qrcodegen.hpp
    qrcodegen.cpp
    ${WEB_RESOURCES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::WebSockets
    Qt6::HttpServer
)
