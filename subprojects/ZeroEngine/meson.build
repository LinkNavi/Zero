project('ZeroEngine', 'cpp',
  version: '0.1.0',
  default_options: ['cpp_std=c++17', 'warning_level=2']
)

vulkan_dep = dependency('vulkan')
glfw_dep = dependency('glfw3')
assimp_dep = dependency('assimp')

inc = include_directories(
  'include',
  'external',
  'external/imgui'
)

imgui_sources = files(
  'external/imgui/imgui.cpp',
  'external/imgui/imgui_draw.cpp',
  'external/imgui/imgui_tables.cpp',
  'external/imgui/imgui_widgets.cpp',
  'external/imgui/imgui_impl_glfw.cpp',
  'external/imgui/imgui_impl_vulkan.cpp',
)

engine_sources = files(
  'src/Physics.cpp',
  'src/SceneManager.cpp',
  'src/impls.cpp',
  'src/Renderer.cpp',
)

zeroengine_lib = static_library('ZeroEngine',
  engine_sources + imgui_sources,
  include_directories: inc,
  dependencies: [vulkan_dep, glfw_dep, assimp_dep]
)

zeroengine_dep = declare_dependency(
  include_directories: inc,
  link_with: zeroengine_lib,
  dependencies: [vulkan_dep, glfw_dep, assimp_dep]
)

glslc = find_program('glslc')

shader_list = [
  ['shaders/unified.vert', 'unified_vert.spv'],
  ['shaders/unified.frag', 'unified_frag.spv'], 
  ['shaders/shadow.vert', 'shadow_vert.spv'],
  ['shaders/skybox.vert', 'skybox_vert.spv'],
  ['shaders/skybox.frag', 'skybox_frag.spv'],
  ['shaders/fullscreen.vert', 'fullscreen_vert.spv'],
  ['shaders/bloom.frag', 'bloom_frag.spv'],
  ['shaders/composite.frag', 'composite_frag.spv'],
]

foreach shader : shader_list
  custom_target('shader_' + shader[1],
    input: shader[0],
    output: shader[1],
    command: [glslc, '@INPUT@', '-o', '@OUTPUT@'],
    build_by_default: true
  )
endforeach
