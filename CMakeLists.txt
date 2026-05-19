cmake_minimum_required(VERSION 3.16)
project(StudentManagementSystem LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})
add_definitions(${GTK3_CFLAGS_OTHER})

set(SOURCES
    src/main.c
    src/data_manager.c
    src/ui.c
)

set(HEADERS
    src/models.h
    src/data_manager.h
    src/ui.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${PROJECT_NAME} ${GTK3_LIBRARIES})

target_include_directories(${PROJECT_NAME} PRIVATE src)
