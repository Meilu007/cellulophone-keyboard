cmake_minimum_required(VERSION 3.19)

project(CellulophoneKeyboard)

set_plugin_standard_srcs(${PROJECT_NAME} TRUE TRUE)

if(WIN32)
    set(platform_srcs
        CellulophoneKeyboard.rc
    )
elseif(APPLE)
    set(platform_srcs
        CellulophoneKeyboard.xml
    )
    set_source_files_properties(CellulophoneKeyboard.xml PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
    )
endif()

add_library(${PROJECT_NAME} MODULE
    ${sdk_srcs}
    ${srcs}
    ${platform_srcs}
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Aktiviert den offiziellen Factory-Export (MP_GetFactory) im SynthEdit SDK:
target_compile_definitions(${PROJECT_NAME} PRIVATE
    SE_TARGET_SEM
    GMPI_IS_PLATFORM_SDK
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
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa" "-framework CoreFoundation" "-framework CoreGraphics" "-framework CoreText")
elseif(WIN32)
    target_link_libraries(${PROJECT_NAME} d2d1 dwrite windowscodecs)
endif()
