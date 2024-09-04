project('websocket-c', 'c',
  version: '1.0.0',
  license: 'BSD-2-Clause',
  license_files: 'LICENSE',
  default_options: [
    'c_std=c11',
    'warning_level=3',
    'buildtype=release',
    'strip=true',
  ])

c = meson.get_compiler('c')

add_project_arguments(c.get_supported_arguments([
  '-Werror=return-type',
  '-Werror=switch',
  '-Werror',
  '-Wformat=2',
  '-Wimplicit-fallthrough',
  '-Wmissing-declarations',
  '-Wmissing-prototypes',
  '-pedantic',
  '-Wno-expansion-to-defined',
  '-Wno-unknown-warning-option',
  '-Wno-unused-command-line-argument',
  '-Wno-user-defined-literals',
  '-fstrict-flex-arrays=2',
]), language: 'c')

subdir('websocket-c')
pkg = import('pkgconfig')
pkg.generate(
  libraries: websocket_lib,
  version: meson.project_version(),
  name: meson.project_name(),
  description: 'Small websocket library'
)

if get_option('examples').enabled()
  subdir('examples')
endif
