
cmake_minimum_required(VERSION 3.10)
project(Zero CXX)







file(GLOB_RECURSE SOURCES 

    "${PROJECT_SOURCE_DIR}/../../src/*.c"
    "${PROJECT_SOURCE_DIR}/../../src/*.cpp"

)



add_executable(Zero ${SOURCES})



target_include_directories(Zero PRIVATE "${PROJECT_SOURCE_DIR}/../../include")





# Custom build flags
target_compile_options(Zero PRIVATE 

    "-Wall"

    "-Wextra"

    "-Wpedantic"

)







