project('ZeroRuntime', 'cpp',
  default_options: ['cpp_std=c++17']
)

zeroengine = subproject('ZeroEngine')
zeroengine_dep = zeroengine.get_variable('zeroengine_dep')

executable('ZeroRuntime',
  'src/main.cpp',
  dependencies: zeroengine_dep
)

executable('SceneCreator',
  'src/scene_creator.cpp',
  dependencies: zeroengine_dep
)
