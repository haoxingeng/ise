add_executable(4in1_server
  app_business.cpp
  svr_mod_chargen.cpp
  svr_mod_daytime.cpp
  svr_mod_discard.cpp
  svr_mod_echo.cpp
  )

target_link_libraries(4in1_server ise)
