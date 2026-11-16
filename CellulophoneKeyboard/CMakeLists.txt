cmake_minimum_required(VERSION 3.19)
project(CellulophoneKeyboard)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_GUI
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
    target_sources(${PROJECT_NAME} PRIVATE CellulophoneKeyboard.rc)
    target_compile_definitions(${PROJECT_NAME} PRIVATE SE_TARGET_SEM _USRDLL _WINDLL)
    target_link_libraries(${PROJECT_NAME} d2d1 dwrite windowscodecs)
    target_link_options(${PROJECT_NAME} PRIVATE "/EXPORT:MP_GetFactory")
elseif(APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa" "-framework CoreFoundation" "-framework CoreGraphics" "-framework CoreText")
endif()
