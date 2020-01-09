cmake_minimum_required (VERSION 2.8)


add_executable (websocket_client
  websocket_client.cc
)
target_link_libraries(websocket_client
  websocket
)

add_executable (websocket_server
  websocket_server.cc
)
target_link_libraries(websocket_server
  websocket
)
