cmake_minimum_required(VERSION 3.16)
project(player LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)
qt_standard_project_setup()

add_executable(player
    main.cpp
    cli/cli_app.cpp
    gui/gui_app.cpp
    core/PlaylistImpl.cpp
    gui/MainWindow.h gui/MainWindow.cpp
    core/Mp3Reader.h core/Mp3Reader.cpp
    gui/TrackIndicator.h gui/TrackIndicator.cpp
)

# 🔹 Tell compiler where headers live
target_include_directories(player
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/core
)

# 🔹 Link Qt
target_link_libraries(player
    PRIVATE
        Qt6::Widgets
        Qt6::Multimedia
)
