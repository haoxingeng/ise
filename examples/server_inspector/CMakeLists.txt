add_executable(server_inspector
  app_business.cpp
  svr_mod_inspector.cpp
  )

target_link_libraries(server_inspector ise)
