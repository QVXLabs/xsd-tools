# Quickstart interop example. Built only when the top-level build is configured
# with -DQUICKSTART=ON. It generates Python/C++/Java bindings from message.xsd
# with the in-tree xsdb, compiles the C++ and Java endpoints, and adds a
# `run-quickstart` target that runs the five-endpoint relay.
#
# Unix-oriented (the relay uses pipes; the C++ targets need Expat). The example
# is built for JDK 17 (--release 17 when the JDK supports it); the generated
# bindings themselves remain JDK 8 compatible.

find_package(EXPAT REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
# find_program (not find_package(Java)) — we only need javac + java, and
# FindJava insists on javah, which modern JDKs no longer ship.
find_program(QS_JAVAC javac REQUIRED)
find_program(QS_JAVA  java  REQUIRED)

# Detect the javac major version to gate --release 17 (JDK 17 is the example's
# target; fall back gracefully so the demo still builds on an older local JDK).
execute_process(COMMAND "${QS_JAVAC}" -version
                OUTPUT_VARIABLE _javac_v ERROR_VARIABLE _javac_v
                OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REGEX MATCH "javac (1\\.)?([0-9]+)" _ "${_javac_v}")
set(JAVA_MAJOR "${CMAKE_MATCH_2}")
if(JAVA_MAJOR AND JAVA_MAJOR GREATER_EQUAL 17)
  set(JAVAC_RELEASE "--release" "17")
else()
  set(JAVAC_RELEASE "")
  message(STATUS "quickstart: javac ${JAVA_MAJOR} (<17); building example Java "
                 "without --release 17 (endpoint source is JDK 8 compatible)")
endif()

set(QS_SRC    "${CMAKE_CURRENT_SOURCE_DIR}")
set(QS_GEN    "${CMAKE_CURRENT_BINARY_DIR}/gen")
set(SCHEMA    "${QS_SRC}/message.xsd")
set(TEMPLATES "${CMAKE_SOURCE_DIR}/templates")
file(MAKE_DIRECTORY "${QS_GEN}" "${QS_GEN}/cpp" "${QS_GEN}/java" "${QS_GEN}/javac")

# Run xsdb with XSDTOOLS_DATA pointed at the in-tree templates.
set(XSDB "${CMAKE_COMMAND}" -E env "XSDTOOLS_DATA=${TEMPLATES}" "$<TARGET_FILE:xsdb>")

# --- generate the three language bindings -------------------------------------
add_custom_command(
  OUTPUT "${QS_GEN}/message.py"
  COMMAND ${CMAKE_COMMAND} -E env "XSDTOOLS_DATA=${TEMPLATES}"
          sh -c "'$<TARGET_FILE:xsdb>' python-sax '${SCHEMA}' > '${QS_GEN}/message.py'"
  DEPENDS xsdb "${SCHEMA}" VERBATIM)

add_custom_command(
  OUTPUT "${QS_GEN}/cpp/xml_message.cpp" "${QS_GEN}/cpp/xml_message.hpp"
         "${QS_GEN}/cpp/xml_common.h"
  COMMAND ${XSDB} --out-dir "${QS_GEN}/cpp" cpp-xml-expat "${SCHEMA}"
  DEPENDS xsdb "${SCHEMA}" VERBATIM)

add_custom_command(
  OUTPUT "${QS_GEN}/java/message.java"
  COMMAND ${XSDB} --out-dir "${QS_GEN}/java" java-xml-stax "${SCHEMA}" -package interop
  DEPENDS xsdb "${SCHEMA}" VERBATIM)

# --- C++ endpoints (cpp-xml-expat) --------------------------------------------
foreach(ep ep2_relay ep5_sink)
  string(REGEX REPLACE "_.*" "" out "${ep}")   # ep2_relay -> ep2
  add_executable(qs_${out} "${QS_SRC}/${ep}.cpp" "${QS_GEN}/cpp/xml_message.cpp")
  target_include_directories(qs_${out} PRIVATE "${QS_GEN}/cpp" "${QS_SRC}")
  target_link_libraries(qs_${out} PRIVATE EXPAT::EXPAT)
  set_target_properties(qs_${out} PROPERTIES
    CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY "${QS_GEN}" OUTPUT_NAME "${out}")
endforeach()

# --- Java endpoint (java-xml-stax) --------------------------------------------
add_custom_command(
  OUTPUT "${QS_GEN}/javac/interop/Ep3Relay.class"
  COMMAND sh -c "'${QS_JAVAC}' ${JAVAC_RELEASE} -d '${QS_GEN}/javac' '${QS_GEN}'/java/*.java '${QS_SRC}/Ep3Relay.java'"
  DEPENDS "${QS_GEN}/java/message.java" "${QS_SRC}/Ep3Relay.java" VERBATIM)

# --- run target: drive the relay ----------------------------------------------
add_custom_target(run-quickstart
  COMMAND ${CMAKE_COMMAND} -E env
          "PYTHON3=${Python3_EXECUTABLE}" "JAVA=${QS_JAVA}"
          sh "${QS_SRC}/run.sh" "${QS_GEN}"
  DEPENDS qs_ep2 qs_ep5
          "${QS_GEN}/message.py" "${QS_GEN}/javac/interop/Ep3Relay.class"
  WORKING_DIRECTORY "${QS_SRC}"
  USES_TERMINAL VERBATIM
  COMMENT "Running the 5-endpoint quickstart relay")
