
cmake_minimum_required(VERSION 3.10)
project(ZeroEngine CXX)







file(GLOB_RECURSE SOURCES 

    "${PROJECT_SOURCE_DIR}/../../src/*.c"
    "${PROJECT_SOURCE_DIR}/../../src/*.cpp"

)



add_executable(ZeroEngine ${SOURCES})



target_include_directories(ZeroEngine PRIVATE "${PROJECT_SOURCE_DIR}/../../include")





# Custom build flags
target_compile_options(ZeroEngine PRIVATE 

    "-Wall"

    "-Wextra"

    "-Wpedantic"

    "-std=c++17"

)





# Additional libraries
target_link_libraries(ZeroEngine PRIVATE 

    GL

    glfw

    dl

    pthread

)



