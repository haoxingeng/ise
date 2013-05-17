add_executable(server_module_msgs
  app_business.cpp
  svr_mod_1.cpp
  svr_mod_2.cpp
  )

target_link_libraries(server_module_msgs ise)
