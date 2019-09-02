cmake_minimum_required(VERSION 3.0.0)
project(Hardcoded VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)

include(CTest)
enable_testing()


file(GLOB_RECURSE srcfiles 
    "source/*.cpp"
)

add_executable(Hardcoded ${srcfiles})

target_include_directories(Hardcoded PRIVATE headers)

if(WIN32)
else()
    target_link_libraries(Hardcoded dl pthread)
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
