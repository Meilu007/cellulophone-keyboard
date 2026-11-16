cmake_minimum_required(VERSION 3.19)
project(CellulophoneKeyboard)

set(gui_sdk_srcs
    ${se_sdk_folder}/mp_sdk_common.cpp
    ${se_sdk_folder}/mp_sdk_gui.cpp
)
if(EXISTS "${se_sdk_folder}/mp_sdk_gui2.cpp")
    list(APPEND gui_sdk_srcs ${se_sdk_folder}/mp_sdk_gui2.cpp)
endif()

add_library(CellulophoneKeyboard MODULE
    CellulophoneKeyboardGui.cpp
    ${gui_sdk_srcs}
)

target_include_directories(CellulophoneKeyboard PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${se_sdk_folder}
    ${se_shared_folder}
)

target_compile_definitions(CellulophoneKeyboard PRIVATE
    _UNICODE
    UNICODE
    SE_TARGET_SEM
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

set_target_properties(CellulophoneKeyboard PROPERTIES
    PREFIX ""
    SUFFIX ".sem"
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)

if(WIN32)
    target_sources(CellulophoneKeyboard PRIVATE CellulophoneKeyboard.rc)
    target_link_libraries(CellulophoneKeyboard d2d1 dwrite windowscodecs)
    target_link_options(CellulophoneKeyboard PRIVATE "/EXPORT:MP_GetFactory")
elseif(APPLE)
    set_target_properties(CellulophoneKeyboard PROPERTIES
        BUNDLE TRUE
        BUNDLE_EXTENSION "sem"
    )
    target_sources(CellulophoneKeyboard PUBLIC CellulophoneKeyboard.xml)
    set_source_files_properties(CellulophoneKeyboard.xml PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_link_libraries(CellulophoneKeyboard "-framework Cocoa" "-framework CoreFoundation" "-framework CoreGraphics" "-framework CoreText")
endif()
