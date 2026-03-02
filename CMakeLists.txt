cmake_minimum_required(VERSION 3.10)

project(raylib-osu)

find_package(raylib REQUIRED)

# ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- ----------
# Include and link the aubio library
# ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- ----------

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
include_directories("${CMAKE_CURRENT_LIST_DIR}/include/")
link_directories("${CMAKE_CURRENT_LIST_DIR}/lib/")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
include_directories("/usr/local/include/aubio")
link_directories("/usr/local/lib")
endif()

add_executable(wav_to_beats "${CMAKE_CURRENT_LIST_DIR}/src/wav_to_beats.cpp")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
target_link_libraries(wav_to_beats aubio -static)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
target_link_libraries(wav_to_beats aubio)
endif()

# ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- ----------
# Build send_key utility for sending keyboard input (Windows only)
# ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- ----------

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
add_executable(send_key "${CMAKE_CURRENT_LIST_DIR}/src/send_key.cpp")
endif()

# ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- ----------
# Include and link the raylib and sqlite library
# ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- ----------

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
include_directories("${CMAKE_CURRENT_LIST_DIR}/include/sqlite3")
link_directories("${CMAKE_CURRENT_LIST_DIR}/lib/sqlite3")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
include_directories("/usr/include/sqlite3")
link_directories("/usr/lib")
endif()

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/main.cpp")

set(PROJECT_INCLUDE "${CMAKE_CURRENT_LIST_DIR}/src/")

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_INCLUDE})

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
target_link_libraries(${PROJECT_NAME} PRIVATE raylib sqlite3.dll)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
target_link_libraries(${PROJECT_NAME} PRIVATE raylib sqlite3)
endif()

# ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- ----------
