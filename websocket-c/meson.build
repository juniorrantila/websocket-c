websocket_lib = library('websocket-c', [
  './websocket.c',
  './client.c',
  './server.c',
], install: true)

websocket_dep = declare_dependency(
  link_with: websocket_lib,
  include_directories: '..'
)

install_headers([
  './websocket.h',
  './client.h',
  './server.h',
], subdir: 'websocket-c')
