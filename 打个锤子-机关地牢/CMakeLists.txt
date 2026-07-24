cmake_minimum_required(VERSION 3.20)

project(HammerMechanismDungeon VERSION 2.3.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(dungeon_core
    core/Game.cpp
    core/GameMap.cpp
    core/LevelData.cpp
    core/Monster.cpp
    core/Player.cpp
)
target_include_directories(dungeon_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/core")

if(MSVC)
    target_compile_options(dungeon_core PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(dungeon_core PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow
    )
endif()

option(DUNGEON_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
if(DUNGEON_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(dungeon_core PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(dungeon_core PUBLIC -fsanitize=address,undefined)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(dungeon_core_tests tests/core_tests.cpp)
    target_link_libraries(dungeon_core_tests PRIVATE dungeon_core)
    if(MSVC)
        target_compile_options(dungeon_core_tests PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(dungeon_core_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow
        )
    endif()
    add_test(NAME dungeon_core_tests COMMAND dungeon_core_tests)
endif()

option(DUNGEON_BUILD_EASYX_GAME "Build the Windows EasyX graphical executable" ON)
set(EASYX_INCLUDE_DIR "" CACHE PATH "Directory containing EasyX graphics.h (optional when EasyX is integrated into Visual Studio)")
set(EASYX_LIBRARY "" CACHE FILEPATH "Path to the EasyX library file (optional when graphics.h links it automatically)")

# The project can compile graphics.h because EasyX is installed in this
# Visual Studio instance, but the linker does not necessarily search the
# corresponding Auxiliary/VS/lib directory.  Prefer the detected local x64
# library when the user has not supplied an explicit EASYX_LIBRARY value.
if(MSVC AND NOT EASYX_LIBRARY)
    set(_EASYX_DEFAULT_LIBRARY "D:/Visual Studio/VC/Auxiliary/VS/lib/x64/EasyXw.lib")
    if(EXISTS "${_EASYX_DEFAULT_LIBRARY}")
        set(EASYX_LIBRARY "${_EASYX_DEFAULT_LIBRARY}")
    endif()
endif()

if(WIN32 AND DUNGEON_BUILD_EASYX_GAME)
    add_executable(hammer_dungeon
        main.cpp
        ui/AnimationManager.cpp
        ui/GameUI.cpp
    )
    target_link_libraries(hammer_dungeon PRIVATE dungeon_core)
    target_compile_definitions(hammer_dungeon PRIVATE UNICODE _UNICODE NOMINMAX)
    if(EASYX_INCLUDE_DIR)
        target_include_directories(hammer_dungeon PRIVATE "${EASYX_INCLUDE_DIR}")
    endif()
    if(EASYX_LIBRARY)
        target_link_libraries(hammer_dungeon PRIVATE "${EASYX_LIBRARY}")
    elseif(MSVC)
        message(FATAL_ERROR
            "找不到 EasyXw.lib。请在 CMake 配置中设置 EASYX_LIBRARY，"
            "例如 D:/Visual Studio/VC/Auxiliary/VS/lib/x64/EasyXw.lib。")
    endif()
    if(MSVC)
        # EasyX 的 ExMessage 在官方头文件中使用匿名联合，C4201 来自第三方头文件。
        target_compile_options(hammer_dungeon PRIVATE /W4 /permissive- /utf-8 /wd4201)
    endif()
elseif(NOT WIN32)
    message(STATUS "EasyX is Windows-only; building the portable core and tests on this platform.")
endif()
