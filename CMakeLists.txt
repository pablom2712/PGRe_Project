cmake_minimum_required(VERSION 3.15)

PROJECT(PGRE)

option(SDL_SHARED "" OFF)
option(SDL_STATIC "" ON )
add_subdirectory(libs/SDL-release-3.4.18)
add_subdirectory(libs/geGL-master)
add_subdirectory(libs/glm-1.0.3)

add_executable(${PROJECT_NAME} main.cpp bunny.cpp)
target_compile_definitions(${PROJECT_NAME} PRIVATE ASSET_DIR="${CMAKE_SOURCE_DIR}/")
configure_file(
    ${CMAKE_SOURCE_DIR}/earth_daymap.jpg
    ${CMAKE_BINARY_DIR}/earth_daymap.jpg
    COPYONLY
)

target_link_libraries(${PROJECT_NAME} PUBLIC SDL3::SDL3 geGL::geGL glm::glm)