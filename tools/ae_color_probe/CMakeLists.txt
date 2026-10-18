cmake_minimum_required(VERSION 3.20)
project(FSTRColorProbe LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
if(NOT APPLE OR NOT FSTR_AE_SDK_ROOT)
 message(FATAL_ERROR "Owned macOS/AE SDK required")
endif()
set(SDK "${FSTR_AE_SDK_ROOT}/Examples")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/PkgInfo" "eFKTFXTC")
add_library(FSTRColorProbe MODULE probe.cpp)
target_include_directories(FSTRColorProbe PRIVATE "${SDK}/Headers" "${SDK}/Headers/SP" "${SDK}/Util")
set_target_properties(FSTRColorProbe PROPERTIES BUNDLE TRUE BUNDLE_EXTENSION plugin PREFIX "")
add_custom_command(TARGET FSTRColorProbe POST_BUILD
 COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:FSTRColorProbe>/Contents/Resources"
 COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist" "$<TARGET_BUNDLE_DIR:FSTRColorProbe>/Contents/Info.plist"
 COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/PkgInfo" "$<TARGET_BUNDLE_DIR:FSTRColorProbe>/Contents/PkgInfo"
 COMMAND /usr/bin/xcrun Rez -useDF -d __arm64__ -I "${SDK}/Headers" -I "${SDK}/Resources" "${CMAKE_CURRENT_SOURCE_DIR}/probe.r" -o "$<TARGET_BUNDLE_DIR:FSTRColorProbe>/Contents/Resources/FSTRColorProbe.rsrc" VERBATIM)
