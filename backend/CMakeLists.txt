cmake_minimum_required(VERSION 3.15)
project(BookTrackerBackend)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(../external/SQLiteCpp external/SQLiteCpp)
add_subdirectory(../external/json external/json)

include_directories(
    include
    ../external/json/include
    ../external/SQLiteCpp/include
    ../external/cpp-httplib
)

add_executable(BookTrackerBackend
    src/main.cpp
    src/database.cpp
    src/api.cpp
)

target_link_libraries(BookTrackerBackend SQLiteCpp sqlite3 pthread dl curl)
