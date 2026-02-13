project('Zero', 'cpp')

subproject('ZeroEngine')
subproject('ZeroRuntime')


run_command('mkdir', '-p', 'shaders', check: false)
run_command('sh', '-c', 'cp build/subprojects/ZeroEngine/shaders/*.spv shaders/ 2>/dev/null || true', check: true)
