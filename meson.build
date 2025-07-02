project('redlog', 'cpp',
  version : '1.0.0',
  license : 'MIT',
  default_options : [
    'cpp_std=c++17',
    'warning_level=3',
    'werror=false'
  ]
)

# header-only library
redlog_inc = include_directories('include')

# platform-specific compile definitions
cpp_args = []
if host_machine.system() == 'windows'
  cpp_args += [
    '-DNOMINMAX',
    '-DWIN32_LEAN_AND_MEAN',
    '-D_CRT_SECURE_NO_WARNINGS'
  ]
endif

redlog_dep = declare_dependency(
  include_directories : redlog_inc,
  compile_args : cpp_args
)

# make available as subproject
meson.override_dependency('redlog', redlog_dep)

# install header directly so it can be included as #include <redlog.hpp>
install_headers('include/redlog.hpp')

# generate pkg-config file
pkg = import('pkgconfig')
pkg.generate(
  name : 'redlog',
  description : 'modern C++ header-only logging library',
  version : meson.project_version()
)