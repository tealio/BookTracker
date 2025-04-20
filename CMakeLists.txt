cmake_minimum_required(VERSION 3.10)
project(BookTrackerBackend)

set(CMAKE_CXX_STANDARD 17)

# External libraries
add_subdirectory(external/SQLiteCpp)
include_directories(external/SQLiteCpp/include)
include_directories(external/json/include)
include_directories(external/cpp-httplib)

# Your header files
include_directories(backend/include)

# Source files
add_executable(BookTrackerBackend
    backend/src/main.cpp
    backend/src/database.cpp
    backend/src/api.cpp
)

# Link libraries
target_link_libraries(BookTrackerBackend SQLiteCpp sqlite3 pthread curl)

