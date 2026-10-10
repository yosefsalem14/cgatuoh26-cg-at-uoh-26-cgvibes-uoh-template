cmake_minimum_required(VERSION 3.14)
project(minigui)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# --- minifb ---
set(MINIFB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(MINIFB_BUILD_VERSION_INFO OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    minifb
    GIT_REPOSITORY https://github.com/emoon/minifb.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(minifb)

# --- microui ---
FetchContent_Declare(
    microui
    GIT_REPOSITORY https://github.com/rxi/microui.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(microui)

add_library(microui_lib STATIC ${microui_SOURCE_DIR}/src/microui.c)
target_include_directories(microui_lib PUBLIC ${microui_SOURCE_DIR}/src)

# --- Our application (BEFORE GLM includes) ---
add_executable(minigui
    src/main.cpp
    src/ui_renderer.cpp
    src/ui_renderer.h
    src/ui_bridge.h
    src/microui.c
    src/microui.h
)

# Link libraries
target_link_libraries(minigui PRIVATE microui_lib minifb)

# Include directories (GLM uses manual download)
target_include_directories(minigui PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${microui_SOURCE_DIR}/src
    ${minifb_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/glm-master     # ← Your manual GLM folder
)