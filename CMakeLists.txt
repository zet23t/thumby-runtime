# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)
set(CMAKE_BUILD_TYPE Release)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define LIB_PICO_STDIO_USB to enable USB serial output
add_definitions(-DLIB_PICO_STDIO_USB=1)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NEVER EDIT THE NEXT LINES for Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.0.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
include(${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

include_directories(${PICO_SDK_PATH}/src/rp2_common/hardware_pwm/include)
include_directories(${PICO_SDK_PATH}/src/rp2_common/hardware_spi/include)
include_directories(${PICO_SDK_PATH}/src/rp2_common/hardware_dma/include)
include_directories(${CMAKE_SOURCE_DIR}/_src_gen) 

project(blink C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(blink
    blink.c
    _src_gen/atlas.c
    _src_gen/fnt_medium.c
    _src_gen/fnt_myfont.c
    _src_gen/fnt_tiny.c
    _src_gen/fnt_mymono.c
    game/game_assets.c
    game/game.c
    game/engine_main.c
    game/game_character.c
    game/game_enemies.c
    game/game_environment.c
    game/game_menu.c
    game/game_particlesystem.c
    game/game_player.c
    game/game_projectile.c
    game/game_scene_0_testing.c
    game/game_scene_3.c
    game/game_scenes.c
    game/game_renderobjects.c
    game/game_battle_actions.c
    game/game_battle.c
    game/TE_Image.c
    game/TE_Font.c
    game/TE_math.c
    game/TE_rand.c
    game/TE_sdfmap.c
    )

# pull in common dependencies
target_link_libraries(blink pico_stdlib hardware_dma hardware_spi pico_stdio_usb)

if (PICO_CYW43_SUPPORTED)
    target_link_libraries(blink pico_cyw43_arch_none)
endif()

# create map/bin/hex file etc.
pico_add_extra_outputs(blink)

# add url via pico_set_program_url
