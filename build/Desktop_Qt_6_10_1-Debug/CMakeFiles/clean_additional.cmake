# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/client_exec_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/client_exec_autogen.dir/ParseCache.txt"
  "CMakeFiles/server_exec_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/server_exec_autogen.dir/ParseCache.txt"
  "client_exec_autogen"
  "server_exec_autogen"
  )
endif()
