# Embed the bundled, trusted font so installed games need no system fonts or
# additional asset search paths. The source font remains unmodified.
file(READ "${INPUT}" data HEX)
string(REGEX REPLACE "(..)" "0x\\1," data "${data}")
# Keep generated lines short for compilers with physical line-length limits.
string(REGEX REPLACE "(0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,)" "\\1\n" data "${data}")
if(NOT DEFINED SYMBOL)
  set(SYMBOL FontData)
endif()
file(WRITE "${OUTPUT}" "namespace popup_font {\nextern const unsigned char ${SYMBOL}[] = {${data}};\n}\n")
