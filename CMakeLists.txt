cmake_minimum_required(VERSION 3.0.0)
project(Hardcoded VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)

include(CTest)
enable_testing()


file(GLOB_RECURSE srcfiles 
    "source/*.cpp"
)

if(WIN32)
    add_executable(Hardcoded WIN32 ${srcfiles})
else()
    add_executable(Hardcoded ${srcfiles})
endif()


target_include_directories(Hardcoded PRIVATE headers)

if(WIN32)
else()
    target_link_libraries(Hardcoded dl pthread)
endif()

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
