cmake_minimum_required(VERSION 3.21)

set(CMAKE_CONFIGURATION_TYPES Debug Release)

project(examshell)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#
# CEF configuration.
#

set(CEF_VERSION "137.0.10+g7e14fe1+chromium-137.0.7151.69")

# Determine the platform.
if(NOT DEFINED CEF_PLATFORM)
  if("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
    if("${PROJECT_ARCH}" STREQUAL "arm64")
      set(CEF_PLATFORM "macosarm64")
    elseif("${PROJECT_ARCH}" STREQUAL "x86_64")
      set(CEF_PLATFORM "macosx64")
    elseif("${CMAKE_HOST_SYSTEM_PROCESSOR}" STREQUAL "arm64")
      set(PROJECT_ARCH "arm64")
      set(CEF_PLATFORM "macosarm64")
    else()
      set(PROJECT_ARCH "x86_64")
      set(CEF_PLATFORM "macosx64")
    endif()
  elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    if(CMAKE_SIZEOF_VOID_P MATCHES 8)
      set(CEF_PLATFORM "linux64")
    else()
      message(FATAL_ERROR "Linux 32-bit builds are not supported.")
    endif()
  elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    if(CMAKE_SIZEOF_VOID_P MATCHES 8)
      set(CEF_PLATFORM "windows64")
    else()
      set(CEF_PLATFORM "windows32")
    endif()
  endif()
endif()

message(STATUS "CEF Version: ${CEF_VERSION}")
message(STATUS "CEF Platform: ${CEF_PLATFORM}")

# Add this project's cmake/ directory to the module path.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Download and extract the CEF binary distribution.
include(DownloadCEF)
DownloadCEF("${CEF_PLATFORM}" "${CEF_VERSION}" "${CMAKE_SOURCE_DIR}/third_party/cef")

# Add the CEF binary distribution's cmake/ directory to the module path.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CEF_ROOT}/cmake")

# Disable sandbox to simplify the build.
set(USE_SANDBOX OFF CACHE BOOL "Disable CEF sandbox" FORCE)

# Load the CEF configuration.
find_package(CEF REQUIRED)

#
# libcef_dll_wrapper target.
#

add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper)

# Allow includes relative to the src directory.
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

#
# Determine the output directory.
#

if("${CMAKE_GENERATOR}" STREQUAL "Ninja" OR
   "${CMAKE_GENERATOR}" STREQUAL "Unix Makefiles")
  set(TARGET_OUT_DIR "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}")
else()
  set(TARGET_OUT_DIR "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
endif()

#
# Application sources.
#

set(APP_SRCS
  src/app.h
  src/app.cc
  src/handler.h
  src/handler.cc
)

#
# Linux configuration.
#

if(OS_LINUX)
  # Executable target.
  list(APPEND APP_SRCS src/main_linux.cc)
  add_executable(${PROJECT_NAME} ${APP_SRCS})

  set_executable_target_properties(${PROJECT_NAME})

  add_dependencies(${PROJECT_NAME} libcef_dll_wrapper)

  # Logical target used to link the libcef library on Linux.
  add_logical_target("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")

  target_link_libraries(${PROJECT_NAME} libcef_lib libcef_dll_wrapper ${CEF_STANDARD_LIBS})

  # Set rpath so that libraries can be placed next to the executable.
  set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN")
  set_target_properties(${PROJECT_NAME} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)

  set_target_properties(${PROJECT_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${TARGET_OUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${TARGET_OUT_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${TARGET_OUT_DIR}"
  )

  # Copy binary and resource files to the target output directory.
  copy_files("${PROJECT_NAME}" "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${TARGET_OUT_DIR}")
  copy_files("${PROJECT_NAME}" "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${TARGET_OUT_DIR}")
endif()

#
# macOS configuration.
#

if(OS_MAC)
  set(APP_TARGET "${PROJECT_NAME}")
  set(APP_HELPER_TARGET "${PROJECT_NAME}_Helper")
  set(APP_HELPER_OUTPUT_NAME "${PROJECT_NAME} Helper")

  # Main app bundle.
  list(APPEND APP_SRCS src/main_mac.mm)
  add_executable(${APP_TARGET} MACOSX_BUNDLE ${APP_SRCS})
  set_executable_target_properties(${APP_TARGET})
  add_dependencies(${APP_TARGET} libcef_dll_wrapper)
  target_link_libraries(${APP_TARGET} libcef_dll_wrapper ${CEF_STANDARD_LIBS})

  set_target_properties(${APP_TARGET} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${TARGET_OUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${TARGET_OUT_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${TARGET_OUT_DIR}"
    MACOSX_BUNDLE_BUNDLE_NAME "${APP_TARGET}"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.examshell.app"
    MACOSX_BUNDLE_INFO_STRING "${APP_TARGET}"
    MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
  )

  # Output path for the main app bundle.
  set(APP_PATH "${TARGET_OUT_DIR}/${APP_TARGET}.app")

  # Copy the CEF framework into the Frameworks directory.
  add_custom_command(
    TARGET ${APP_TARGET}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CEF_BINARY_DIR}/Chromium Embedded Framework.framework"
      "${APP_PATH}/Contents/Frameworks/Chromium Embedded Framework.framework"
    VERBATIM
  )

  # Create the multiple helper app bundle targets.
  foreach(_suffix_list ${CEF_HELPER_APP_SUFFIXES})
    # Convert to a list and extract the suffix values.
    string(REPLACE ":" ";" _suffix_list ${_suffix_list})
    list(GET _suffix_list 0 _name_suffix)
    list(GET _suffix_list 1 _target_suffix)
    list(GET _suffix_list 2 _plist_suffix)

    set(_helper_target "${APP_HELPER_TARGET}${_target_suffix}")
    set(_helper_output_name "${APP_HELPER_OUTPUT_NAME}${_name_suffix}")

    # Create helper-specific Info.plist.
    set(_helper_plist "${CMAKE_CURRENT_BINARY_DIR}/helper-Info${_target_suffix}.plist")
    file(WRITE ${_helper_plist}
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
      "<plist version=\"1.0\">\n"
      "<dict>\n"
      "  <key>CFBundleDisplayName</key>\n"
      "  <string>${_helper_output_name}</string>\n"
      "  <key>CFBundleExecutable</key>\n"
      "  <string>${_helper_output_name}</string>\n"
      "  <key>CFBundleIdentifier</key>\n"
      "  <string>com.examshell.app.helper${_plist_suffix}</string>\n"
      "  <key>CFBundleInfoDictionaryVersion</key>\n"
      "  <string>6.0</string>\n"
      "  <key>CFBundleName</key>\n"
      "  <string>${_helper_output_name}</string>\n"
      "  <key>CFBundlePackageType</key>\n"
      "  <string>APPL</string>\n"
      "  <key>CFBundleVersion</key>\n"
      "  <string>1.0.0</string>\n"
      "  <key>LSFileQuaringEnabled</key>\n"
      "  <true/>\n"
      "  <key>NSSupportsAutomaticGraphicsSwitching</key>\n"
      "  <true/>\n"
      "</dict>\n"
      "</plist>\n"
    )

    # Helper executable target.
    add_executable(${_helper_target} MACOSX_BUNDLE src/mac_helper.cc)
    set_executable_target_properties(${_helper_target})
    add_dependencies(${_helper_target} libcef_dll_wrapper)
    target_link_libraries(${_helper_target} libcef_dll_wrapper ${CEF_STANDARD_LIBS})
    set_target_properties(${_helper_target} PROPERTIES
      MACOSX_BUNDLE_INFO_PLIST ${_helper_plist}
      OUTPUT_NAME ${_helper_output_name}
      ARCHIVE_OUTPUT_DIRECTORY "${TARGET_OUT_DIR}"
      RUNTIME_OUTPUT_DIRECTORY "${TARGET_OUT_DIR}"
      LIBRARY_OUTPUT_DIRECTORY "${TARGET_OUT_DIR}"
    )

    # Add the helper as a dependency of the main executable target.
    add_dependencies(${APP_TARGET} "${_helper_target}")

    # Copy the helper app bundle into the Frameworks directory.
    add_custom_command(
      TARGET ${APP_TARGET}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${TARGET_OUT_DIR}/${_helper_output_name}.app"
        "${APP_PATH}/Contents/Frameworks/${_helper_output_name}.app"
      VERBATIM
    )
  endforeach()
endif()

#
# Windows configuration.
#

if(OS_WINDOWS)
  list(APPEND APP_SRCS src/main_win.cc)
  add_executable(${PROJECT_NAME} WIN32 ${APP_SRCS})

  set_executable_target_properties(${PROJECT_NAME})

  add_dependencies(${PROJECT_NAME} libcef_dll_wrapper)

  # Logical target used to link the libcef library on Windows.
  add_logical_target("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")

  target_link_libraries(${PROJECT_NAME} libcef_lib libcef_dll_wrapper ${CEF_STANDARD_LIBS})

  set_target_properties(${PROJECT_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${TARGET_OUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${TARGET_OUT_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${TARGET_OUT_DIR}"
  )

  # Copy binary and resource files to the target output directory.
  copy_files("${PROJECT_NAME}" "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${TARGET_OUT_DIR}")
  copy_files("${PROJECT_NAME}" "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${TARGET_OUT_DIR}")
endif()

# Display configuration settings.
PRINT_CEF_CONFIG()
