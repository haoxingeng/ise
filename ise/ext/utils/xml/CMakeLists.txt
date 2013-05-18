
aux_source_directory(. ise_utils_xml_SRCS)
add_library(ise_utils_xml ${ise_utils_xml_SRCS})

install(TARGETS ise_utils_xml DESTINATION lib)

file(GLOB ise_utils_xml_HEADERS ${ISE_ROOT_PATH}/ise/ext/utils/xml/*.h)
install(FILES ${ise_utils_xml_HEADERS} DESTINATION include/ise/ext/utils/xml)
