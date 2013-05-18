
aux_source_directory(. ise_utils_cipher_SRCS)
add_library(ise_utils_cipher ${ise_utils_cipher_SRCS})

install(TARGETS ise_utils_cipher DESTINATION lib)

file(GLOB ise_utils_cipher_HEADERS ${ISE_ROOT_PATH}/ise/ext/utils/cipher/*.h)
install(FILES ${ise_utils_cipher_HEADERS} DESTINATION include/ise/ext/utils/cipher)
