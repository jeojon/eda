cmake_minimum_required(VERSION 3.20)

project(EdaLab1Skeleton LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32 AND NOT wxWidgets_ROOT_DIR)
    if(DEFINED ENV{WXWIN} AND EXISTS "$ENV{WXWIN}/include/wx/wx.h")
        set(wxWidgets_ROOT_DIR "$ENV{WXWIN}" CACHE PATH "wxWidgets root directory")
    elseif(EXISTS "D:/wxWeigets/wxWidgets-3.3.3/include/wx/wx.h")
        set(wxWidgets_ROOT_DIR "D:/wxWeigets/wxWidgets-3.3.3"
            CACHE PATH "wxWidgets root directory")
    endif()
endif()

if(WIN32 AND wxWidgets_ROOT_DIR AND NOT wxWidgets_LIB_DIR)
    if(MSVC AND EXISTS "${wxWidgets_ROOT_DIR}/lib/vc_x64_lib")
        set(wxWidgets_LIB_DIR "${wxWidgets_ROOT_DIR}/lib/vc_x64_lib"
            CACHE PATH "wxWidgets library directory")
    endif()
endif()

if(WIN32 AND wxWidgets_LIB_DIR AND EXISTS "${wxWidgets_LIB_DIR}/mswu")
    set(wxWidgets_CONFIGURATION mswu CACHE STRING "wxWidgets configuration")
endif()

set(wxWidgets_USE_STATIC ON)
set(wxWidgets_USE_UNICODE ON)
find_package(wxWidgets REQUIRED COMPONENTS core base)

add_executable(EdaLab1Skeleton WIN32
    src/gui/App.cpp
    src/gui/MainFrame.cpp
    src/gui/FootprintCanvas.cpp
    src/todo/FootprintEditorService.cpp
    src/todo/GeometryService.cpp
    src/todo/FootprintCodec.cpp
    src/todo/FilesystemFootprintLibrary.cpp
)

target_include_directories(EdaLab1Skeleton PRIVATE include)

if(TARGET wxWidgets::wxWidgets)
    target_link_libraries(EdaLab1Skeleton PRIVATE wxWidgets::wxWidgets)
else()
    include(${wxWidgets_USE_FILE})
    target_link_libraries(EdaLab1Skeleton PRIVATE ${wxWidgets_LIBRARIES})
endif()

if(MSVC)
    target_compile_options(EdaLab1Skeleton PRIVATE /utf-8 /W4 /permissive-)
else()
    target_compile_options(EdaLab1Skeleton PRIVATE -Wall -Wextra -Wpedantic)
endif()

