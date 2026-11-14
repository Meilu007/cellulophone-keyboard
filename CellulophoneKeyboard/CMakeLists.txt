cmake_minimum_required(VERSION 3.19)

project(CellulophoneKeyboard)

BUILD_GMPI_PLUGIN(
    PROJECT_NAME CellulophoneKeyboard
    HAS_DSP
    HAS_GUI
)

if(WIN32)
    target_link_libraries(CellulophoneKeyboard d2d1 dwrite windowscodecs)
elseif(APPLE)
    target_link_libraries(CellulophoneKeyboard "-framework Cocoa" "-framework CoreFoundation" "-framework CoreGraphics" "-framework CoreText")
endif()
