cmake_minimum_required(VERSION 3.20)

project(stalker_mod LANGUAGES CXX)

# ============================================================================
#  S.T.A.L.K.E.R.: Тень Чернобыля — тотальная конверсия для GTA San Andreas
#  Сборка: Win32 DLL/ASI для GTA SA 1.0 US (Visual Studio 2022).
#
#  Основной режим (Windows):
#    cmake -B build -A Win32
#    cmake --build build --config Release
#    => build/Release/stalker_mod.asi
#
#  Режим проверки синтаксиса (Linux, без Windows SDK):
#    cmake -B build-check -DSTALKER_SYNTAX_CHECK=ON -DCMAKE_BUILD_TYPE=Debug
#    cmake --build build-check
# ============================================================================

option(STALKER_SYNTAX_CHECK "Syntax-only build (no Windows required)" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(STALKER_SOURCES
    src/Main.cpp
    src/Hooking/Int3Hooks.cpp
    src/Gta/ModelLoader.cpp
    src/Core/Log.cpp
    src/Core/Config.cpp
    src/Core/KeyState.cpp
    src/Core/ModState.cpp
    src/Core/SaveSystem.cpp
    src/Systems/MissionBlocker.cpp
    src/Systems/MutantCatalog.cpp
    src/Systems/MutantDirector.cpp
    src/Systems/PopulationControl.cpp
    src/Systems/RadiationSystem.cpp
    src/Systems/AnomalySystem.cpp
    src/Systems/InventorySystem.cpp
    src/Systems/BlowoutSystem.cpp
    src/Systems/CheatSystem.cpp
    src/Systems/HudSystem.cpp
    src/Systems/TradeSystem.cpp
    src/Systems/AudioSystem.cpp
)

if (STALKER_SYNTAX_CHECK)
    # Проверка компилируемости на любой платформе (обёртки игры — заглушки)
    add_library(stalker_mod_check STATIC ${STALKER_SOURCES})
    target_compile_definitions(stalker_mod_check PRIVATE STALKER_SYNTAX_CHECK)
    target_include_directories(stalker_mod_check PRIVATE src)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(stalker_mod_check PRIVATE -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare)
    endif()
    return()
endif()

# ---- Windows-сборка (основная) ----
if (NOT WIN32)
    message(FATAL_ERROR
        "stalker_mod — Win32 ASI: собирайте на Windows через "
        "Visual Studio 2022 (cmake -B build -A Win32). "
        "На Linux доступна только проверка синтаксиса: "
        "cmake -B build-check -DSTALKER_SYNTAX_CHECK=ON")
endif()

if (NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "Нужна 32-битная (Win32) сборка: используйте -A Win32")
endif()

add_library(stalker_mod MODULE ${STALKER_SOURCES})

set_target_properties(stalker_mod PROPERTIES
    PREFIX ""
    SUFFIX ".asi"
    OUTPUT_NAME "stalker_mod"
)

target_include_directories(stalker_mod PRIVATE src)

if (MSVC)
    target_compile_options(stalker_mod PRIVATE /W3 /utf-8 /permissive-)
    set_property(TARGET stalker_mod PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

target_compile_definitions(stalker_mod PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
)

target_link_libraries(stalker_mod PRIVATE user32 kernel32)
