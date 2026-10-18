cmake_minimum_required(VERSION 3.20)
project(FSTRColorOwnedRunner LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
if(NOT APPLE OR NOT FSTR_AE_SDK_ROOT OR NOT FSTR_RUNNER_SCRIPT OR NOT FSTR_RUNNER_NONCE)
  message(FATAL_ERROR "Owned macOS SDK/script/nonce inputs required")
endif()
option(FSTR_RUNNER_TRACE_IDLE "Trace main-thread idle and render queue monitor in owned host" OFF)
set(SDK "${FSTR_AE_SDK_ROOT}/Examples")
file(READ "${FSTR_RUNNER_SCRIPT}" FSTR_SCRIPT)
configure_file(owned.hpp.in generated/owned.hpp @ONLY)
# Give nonce-owned helpers distinct bundle IDs for provenance and isolation.
string(SHA256 FSTR_RUNNER_ID "${FSTR_RUNNER_NONCE}")
string(SUBSTRING "${FSTR_RUNNER_ID}" 0 20 FSTR_RUNNER_ID)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist" FSTR_RUNNER_PLIST)
string(REPLACE "com.fstr.lut.owned.test.runner" "com.fstr.lut.owned.test.${FSTR_RUNNER_ID}" FSTR_RUNNER_PLIST "${FSTR_RUNNER_PLIST}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/generated/Info.plist" "${FSTR_RUNNER_PLIST}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/generated/PkgInfo" "AEgxFXTC")
add_library(FSTRColorOwnedRunner MODULE runner.cpp)
if(FSTR_RUNNER_TRACE_IDLE)
 target_compile_definitions(FSTRColorOwnedRunner PRIVATE FSTR_RUNNER_TRACE_IDLE=1)
endif()
target_include_directories(FSTRColorOwnedRunner PRIVATE "${SDK}/Headers" "${SDK}/Headers/SP" "${SDK}/Util" "${CMAKE_CURRENT_BINARY_DIR}/generated")
set_target_properties(FSTRColorOwnedRunner PROPERTIES BUNDLE TRUE BUNDLE_EXTENSION plugin PREFIX "" MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/generated/Info.plist")
add_custom_command(TARGET FSTRColorOwnedRunner POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:FSTRColorOwnedRunner>/Contents/Resources"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/generated/Info.plist" "$<TARGET_BUNDLE_DIR:FSTRColorOwnedRunner>/Contents/Info.plist"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/generated/PkgInfo" "$<TARGET_BUNDLE_DIR:FSTRColorOwnedRunner>/Contents/PkgInfo"
  COMMAND /usr/bin/xcrun Rez -useDF -d __arm64__ -I "${SDK}/Headers" -I "${SDK}/Resources" "${CMAKE_CURRENT_SOURCE_DIR}/runner.r" -o "$<TARGET_BUNDLE_DIR:FSTRColorOwnedRunner>/Contents/Resources/FSTRColorOwnedRunner.rsrc"
  VERBATIM)
