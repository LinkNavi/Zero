
cmake_minimum_required(VERSION 3.10)
project(Zero CXX)









file(GLOB_RECURSE SOURCES 

    "${PROJECT_SOURCE_DIR}/../../src/*.c"
    "${PROJECT_SOURCE_DIR}/../../src/*.cpp"

    "${PROJECT_SOURCE_DIR}/../../external/*.c"
    "${PROJECT_SOURCE_DIR}/../../external/*.cpp"

    "${PROJECT_SOURCE_DIR}/../../external/imgui/*.c"
    "${PROJECT_SOURCE_DIR}/../../external/imgui/*.cpp"

)


add_library(Zero STATIC ${SOURCES})



target_include_directories(Zero PRIVATE "${PROJECT_SOURCE_DIR}/../../include")

target_include_directories(Zero PRIVATE "${PROJECT_SOURCE_DIR}/../../external")

target_include_directories(Zero PRIVATE "${PROJECT_SOURCE_DIR}/../../external/Vulkan-Headers/include")

target_include_directories(Zero PRIVATE "${PROJECT_SOURCE_DIR}/../../external/imgui")





target_compile_options(Zero PRIVATE 

    "-Wall"

    "-Wextra"

    "-Wpedantic"

)







target_link_libraries(Zero PRIVATE 

    glfw

    vulkan

    assimp

)





