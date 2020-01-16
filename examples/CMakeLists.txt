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

add_executable (split_packet_client
  split_packet_client.cc
)
target_link_libraries(split_packet_client
  websocket
)

add_executable (deep_receive_server
  deep_receive_server.cc
)
target_link_libraries(deep_receive_server
  websocket
)