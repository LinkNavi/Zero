project('ZeroEditor', 'cpp',
  default_options: ['cpp_std=c++17']
)

zeroengine = subproject('ZeroEngine')
zeroengine_dep = zeroengine.get_variable('zeroengine_dep')
inc = include_directories(
  'include',
 
)
executable('ZeroEditor',
  'src/main.cpp',
  'src/Editor.cpp',
  'src/EditorUI.cpp',
include_directories: inc,
  dependencies: zeroengine_dep
)
