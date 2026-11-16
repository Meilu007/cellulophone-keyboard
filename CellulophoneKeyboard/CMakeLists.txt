cmake_minimum_required(VERSION 3.19)
project(CellulophoneKeyboard)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_GUI
    SOURCE_FILES
        CellulophoneKeyboardGui.cpp
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} d2d1 dwrite windowscodecs)
endif()
