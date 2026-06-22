cmake_minimum_required(VERSION 3.31)
project(NTOTest)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

add_executable(NTOTest src/main.cpp src/qml/qml.qrc
        src/FileProcessor.cpp
        src/FileProcessor.h
        src/ProcessManager.cpp
        src/ProcessManager.h
        src/AppController.cpp
        src/AppController.h)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick)
