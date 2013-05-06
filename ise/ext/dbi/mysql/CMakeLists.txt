
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}) 

find_package(libmysqlclient REQUIRED)

include_directories(${MYSQL_INCLUDE_DIR})

aux_source_directory(. ISE_DBI_MYSQL_SRCS)
add_library(ise_dbi_mysql ${ISE_DBI_MYSQL_SRCS})

install(TARGETS ise_dbi_mysql DESTINATION lib)

file(GLOB ISE_DBI_MYSQL_HEADERS ${ISE_ROOT_PATH}/ise/ext/dbi/mysql/*.h)
install(FILES ${ISE_DBI_MYSQL_HEADERS} DESTINATION include/ise/ext/dbi/mysql)
