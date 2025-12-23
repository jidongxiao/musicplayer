cmake_minimum_required(VERSION 3.16)
project(player LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Enable Auto-Resource handling
set(CMAKE_AUTORCC ON)

# Option for CLI-only build
option(CLI_ONLY "Build CLI only version" OFF)

# Always include core and CLI sources
set(player_sources
    main.cpp
    cli/cli_app.cpp
    core/PlaylistImpl.cpp
    core/Mp3Reader.cpp
)

# Include GUI sources only if not CLI_ONLY
if(NOT CLI_ONLY)
    find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia Svg)
    qt_standard_project_setup()
    qt_add_resources(player_sources "resources.qrc")

    list(APPEND player_sources
        gui/gui_app.cpp
        gui/MainWindow.cpp
        gui/TrackIndicator.cpp
    )
endif()

# Create the executable
add_executable(player ${player_sources})

# Include core headers
target_include_directories(player PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/core)

if(NOT CLI_ONLY)
    target_link_libraries(player PRIVATE Qt6::Widgets Qt6::Multimedia Qt6::Svg)
else()
    target_compile_definitions(player PRIVATE CLI_ONLY)

    # Manually specify the full paths
    set(LIBVLC_LIBRARY "/usr/lib/x86_64-linux-gnu/libvlc.so")
    set(LIBVLC_CORE_LIBRARY "/usr/lib/x86_64-linux-gnu/libvlccore.so")

    target_include_directories(player PRIVATE /usr/include/vlc)
    target_link_libraries(player PRIVATE ${LIBVLC_LIBRARY} ${LIBVLC_CORE_LIBRARY})

    # ncurses
    find_package(Curses REQUIRED)
    target_include_directories(player PRIVATE ${CURSES_INCLUDE_DIR})
    target_link_libraries(player PRIVATE ${CURSES_LIBRARIES})
endif()
