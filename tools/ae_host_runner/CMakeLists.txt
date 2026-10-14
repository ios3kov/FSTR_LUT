cmake_minimum_required(VERSION 3.20)
project(FSTRColorOwnedRunner LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
if(NOT APPLE OR NOT FSTR_AE_SDK_ROOT OR NOT FSTR_RUNNER_SCRIPT OR NOT FSTR_RUNNER_NONCE)
  message(FATAL_ERROR "Owned macOS SDK/script/nonce inputs required")
endif()
set(SDK "${FSTR_AE_SDK_ROOT}/Examples")
file(READ "${FSTR_RUNNER_SCRIPT}" FSTR_SCRIPT)
configure_file(owned.hpp.in generated/owned.hpp @ONLY)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/generated/PkgInfo" "AEgxFXTC")
add_library(FSTRColorOwnedRunner MODULE runner.cpp)
target_include_directories(FSTRColorOwnedRunner PRIVATE "${SDK}/Headers" "${SDK}/Headers/SP" "${SDK}/Util" "${CMAKE_CURRENT_BINARY_DIR}/generated")
set_target_properties(FSTRColorOwnedRunner PROPERTIES BUNDLE TRUE BUNDLE_EXTENSION plugin PREFIX "" MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist")
add_custom_command(TARGET FSTRColorOwnedRunner POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:FSTRColorOwnedRunner>/Contents/Resources"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist" "$<TARGET_BUNDLE_DIR:FSTRColorOwnedRunner>/Contents/Info.plist"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/generated/PkgInfo" "$<TARGET_BUNDLE_DIR:FSTRColorOwnedRunner>/Contents/PkgInfo"
  COMMAND /usr/bin/xcrun Rez -useDF -d __arm64__ -I "${SDK}/Headers" -I "${SDK}/Resources" "${CMAKE_CURRENT_SOURCE_DIR}/runner.r" -o "$<TARGET_BUNDLE_DIR:FSTRColorOwnedRunner>/Contents/Resources/FSTRColorOwnedRunner.rsrc"
  VERBATIM)
