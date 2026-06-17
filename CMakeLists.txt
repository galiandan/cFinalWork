cmake_minimum_required(VERSION 3.16)

project(StudentCourseManager C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

add_executable(app
    src/main.c
    src/model.c
    src/service.c
    src/storage.c
    src/ui.c
    src/app_paths.c
)

target_include_directories(app PRIVATE
    include
    ${GTK3_INCLUDE_DIRS}
)

target_compile_options(app PRIVATE
    -Wall
    -Wextra
    ${GTK3_CFLAGS_OTHER}
)

target_link_directories(app PRIVATE ${GTK3_LIBRARY_DIRS})
target_link_libraries(app PRIVATE ${GTK3_LIBRARIES})
