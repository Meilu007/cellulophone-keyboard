cmake_minimum_required(VERSION 3.19)

project(CellulophoneKeyboard)

se_add_plugin(
    SOURCES
        CellulophoneKeyboard.cpp
        ${se_shared_folder}/Drawing.cpp
    RESOURCES
        CellulophoneKeyboard.xml
)
