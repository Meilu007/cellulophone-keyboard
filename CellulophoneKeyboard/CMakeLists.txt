cmake_minimum_required(VERSION 3.19)

project(CellulophoneKeyboard)

set_plugin_standard_srcs(${PROJECT_NAME} TRUE TRUE)

add_library(${PROJECT_NAME} MODULE
    ${sdk_srcs}
    ${srcs}
    ${rc_file}
    CellulophoneKeyboard.xml
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    SUFFIX ".sem"
)

if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        BUNDLE TRUE
        BUNDLE_EXTENSION "sem"
    )
    set_source_files_properties(CellulophoneKeyboard.xml PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
    )
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa" "-framework CoreFoundation" "-framework CoreGraphics" "-framework CoreText")
elseif(WIN32)
    target_link_libraries(${PROJECT_NAME} d2d1 dwrite windowscodecs)
endif()
