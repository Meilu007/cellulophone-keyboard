cmake_minimum_required(VERSION 3.19)

project(CellulophoneKeyboard)

BUILD_GMPI_PLUGIN(
    PROJECT_NAME CellulophoneKeyboard
    HAS_DSP
    HAS_GUI
)

target_include_directories(CellulophoneKeyboard PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
    target_compile_definitions(CellulophoneKeyboard PRIVATE SE_TARGET_SEM)
    target_link_libraries(CellulophoneKeyboard d2d1 dwrite windowscodecs)
    target_link_options(CellulophoneKeyboard PRIVATE "/EXPORT:MP_GetFactory")
elseif(APPLE)
    target_link_libraries(CellulophoneKeyboard "-framework Cocoa" "-framework CoreFoundation" "-framework CoreGraphics" "-framework CoreText")
endif()
