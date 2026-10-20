cmake_minimum_required(VERSION 3.21)

project(AerokosmikGroundStation
    VERSION 1.0.0
    DESCRIPTION "CubeSat Ground Station - Mission: Kripto"
    LANGUAGES CXX
)

# ── C++ Standard ─────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Qt6 auto-tools ───────────────────────────────────────────────────────────
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ── Qt6 packages ─────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    SerialPort
    Charts
    Network
    Concurrent
    Multimedia
    MultimediaWidgets
)

# ── Source files ─────────────────────────────────────────────────────────────
set(PROJECT_SOURCES
    src/main.cpp

    src/TelemetryPacket.h

    src/TelemetryParser.h
    src/TelemetryParser.cpp

    src/SerialManager.h
    src/SerialManager.cpp

    src/CsvLogger.h
    src/CsvLogger.cpp

    src/SimulationManager.h
    src/SimulationManager.cpp

    src/ChartManager.h
    src/ChartManager.cpp

    src/MapView.h
    src/MapView.cpp

    src/CameraPanel.h
    src/CameraPanel.cpp

    src/MainWindow.h
    src/MainWindow.cpp

    resources/resources.qrc
)

# ── Executable ───────────────────────────────────────────────────────────────
if(WIN32)
    # WIN32 suppresses the console window in release builds
    add_executable(${PROJECT_NAME} WIN32 ${PROJECT_SOURCES})
else()
    add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
endif()

# ── Link Qt6 modules ─────────────────────────────────────────────────────────
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::SerialPort
    Qt6::Charts
    Qt6::Network
    Qt6::Concurrent
    Qt6::Multimedia
    Qt6::MultimediaWidgets
)

# ── Include directories ───────────────────────────────────────────────────────
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ── Compiler-specific flags ──────────────────────────────────────────────────
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /utf-8)
    # Suppress some MSVC warnings that are noisy with Qt
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
    )
else()
    # MinGW / GCC / Clang
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Wpedantic
    )
endif()

# ── Windows deployment helper ─────────────────────────────────────────────────
# Run: cmake --install . --prefix ./install  (after building)
# Or use windeployqt manually:
#   windeployqt --qmldir . AerokosmikGroundStation.exe
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo
            "Build complete. Run windeployqt on the executable for deployment."
    )
endif()

# ── Print config summary ──────────────────────────────────────────────────────
message(STATUS "")
message(STATUS "========================================")
message(STATUS " Mission: Kripto Ground Station v${PROJECT_VERSION}")
message(STATUS " Qt6 version : ${Qt6_VERSION}")
message(STATUS " Compiler    : ${CMAKE_CXX_COMPILER_ID}")
message(STATUS " Build type  : ${CMAKE_BUILD_TYPE}")
message(STATUS "========================================")
message(STATUS "")
