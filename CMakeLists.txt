cmake_minimum_required(VERSION 3.20)

project(EdaLab1
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(wxWidgets_USE_STATIC ON)
if(WIN32 AND NOT wxWidgets_ROOT_DIR)
    if(DEFINED ENV{WXWIN} AND EXISTS "$ENV{WXWIN}/include/wx/wx.h")
        set(wxWidgets_ROOT_DIR "$ENV{WXWIN}" CACHE PATH "wxWidgets root directory")
    elseif(EXISTS "D:/wxWeigets/wxWidgets-3.3.3/include/wx/wx.h")
        set(wxWidgets_ROOT_DIR "D:/wxWeigets/wxWidgets-3.3.3"
            CACHE PATH "wxWidgets root directory")
    endif()
endif()

if(WIN32 AND wxWidgets_ROOT_DIR AND NOT wxWidgets_LIB_DIR)
    if(MSVC)
        if(EXISTS "${wxWidgets_ROOT_DIR}/lib/vc_x64_lib")
            set(wxWidgets_LIB_DIR "${wxWidgets_ROOT_DIR}/lib/vc_x64_lib"
                CACHE PATH "wxWidgets library directory")
        elseif(EXISTS "${wxWidgets_ROOT_DIR}/lib/vc_x64_dll")
            set(wxWidgets_LIB_DIR "${wxWidgets_ROOT_DIR}/lib/vc_x64_dll"
                CACHE PATH "wxWidgets library directory")
        endif()
    endif()
endif()

if(WIN32 AND wxWidgets_LIB_DIR AND NOT wxWidgets_CONFIGURATION)
    if(EXISTS "${wxWidgets_LIB_DIR}/mswu")
        set(wxWidgets_CONFIGURATION mswu CACHE STRING "wxWidgets configuration")
    endif()
endif()

set(wxWidgets_USE_STATIC ON)
set(wxWidgets_USE_UNICODE ON)

if(NOT wxWidgets_ROOT_DIR)
    message(FATAL_ERROR "wxWidgets was not found. Set wxWidgets_ROOT_DIR or WXWIN.")
endif()

find_package(wxWidgets REQUIRED COMPONENTS core base)

add_executable(EdaLab1 WIN32
    src/App.cpp
    src/FootprintModel.cpp
    src/FootprintLibrary.cpp
    src/FootprintCanvas.cpp
    src/MainFrame.cpp
)

if(TARGET wxWidgets::wxWidgets)
    target_link_libraries(EdaLab1 PRIVATE wxWidgets::wxWidgets)
else()
    include(${wxWidgets_USE_FILE})
    target_link_libraries(EdaLab1 PRIVATE ${wxWidgets_LIBRARIES})
endif()

if(MSVC)
    target_compile_options(EdaLab1 PRIVATE /utf-8 /W4)
else()
    target_compile_options(EdaLab1 PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(EdaLab1 PROPERTIES
    OUTPUT_NAME "EdaLab1"
)

install(TARGETS EdaLab1 RUNTIME DESTINATION .)

include(CTest)
if(BUILD_TESTING)
    add_executable(EdaLab1ModelTest
        tests/model_roundtrip.cpp
        src/FootprintModel.cpp
    )
    target_include_directories(EdaLab1ModelTest PRIVATE src)
    add_test(NAME EdaLab1ModelTest COMMAND EdaLab1ModelTest)
endif()

