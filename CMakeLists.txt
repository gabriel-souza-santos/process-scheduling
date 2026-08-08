cmake_minimum_required(VERSION 3.20)

project(process_scheduling
    VERSION 1.0.0
    LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
    src/*.c
)

add_executable(${PROJECT_NAME}
    ${SOURCE_FILES}
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        include
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()
