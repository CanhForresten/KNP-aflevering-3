cmake_minimum_required(VERSION 3.10)
project(restinio_project LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

# Saml alle cpp filer i src/
file(GLOB_RECURSE SRC_FILES src/*.cpp)

# llhttp sources
set(LLHTTP_SRC
    ${CMAKE_SOURCE_DIR}/external/llhttp/api.c
    ${CMAKE_SOURCE_DIR}/external/llhttp/http.c
    ${CMAKE_SOURCE_DIR}/external/llhttp/llhttp.c
)

# Windows API version
if (WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)
endif()

# Header-only fmt
add_definitions(-DFMT_HEADER_ONLY)

# Byg executable
add_executable(${PROJECT_NAME} ${SRC_FILES} ${LLHTTP_SRC})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/external         # base folder for external deps
    ${CMAKE_SOURCE_DIR}/external/restinio
    ${CMAKE_SOURCE_DIR}/external/asio
    ${CMAKE_SOURCE_DIR}/external/fmt      # <-- fmt ligger her: fmt/format.h
    ${CMAKE_SOURCE_DIR}/external/json_dto
    ${CMAKE_SOURCE_DIR}/external/llhttp
    ${CMAKE_SOURCE_DIR}/external/expected-lite
    ${CMAKE_SOURCE_DIR}/external/rapidjson
)

# Link system libraries
if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ws2_32
        mswsock
        Threads::Threads
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Threads::Threads
    )
endif()
