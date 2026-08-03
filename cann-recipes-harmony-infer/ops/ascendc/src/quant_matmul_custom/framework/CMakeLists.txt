if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/mindspore")
    if(EXISTS  "${CMAKE_CURRENT_SOURCE_DIR}/caffe_plugin")
        add_subdirectory(caffe_plugin)
    endif()
    if(EXISTS  "${CMAKE_CURRENT_SOURCE_DIR}/tf_plugin")
        add_subdirectory(tf_plugin)
    endif()
    if(EXISTS  "${CMAKE_CURRENT_SOURCE_DIR}/onnx_plugin")
        add_subdirectory(onnx_plugin)
    endif()
endif()

set(CMAKE_TLS_VERIFY OFF)
set(JSON_VERSION "3.11.3")
set(JSON_URL "https://github.com/nlohmann/json/releases/download/v${JSON_VERSION}/json.hpp")

set(JSON_DEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/onnx_plugin")
set(JSON_DEST_FILE "${JSON_DEST_DIR}/json.hpp")

file(MAKE_DIRECTORY ${JSON_DEST_DIR})

if(NOT EXISTS "${JSON_DEST_FILE}")
    message(STATUS "正在下载 nlohmann/json v${JSON_VERSION} ...")
    message(STATUS "源地址：${JSON_URL}")

    file(DOWNLOAD
        ${JSON_URL}
        ${JSON_DEST_FILE}
        EXPECTED_HASH SHA256=9bea4c8066ef4a1c206b2be5a36302f8926f7fdc6087af5d20b417d0cf103ea6
        SHOW_PROGRESS
        STATUS DOWNLOAD_STATUS
    )

    list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
    list(GET DOWNLOAD_STATUS 1 STATUS_MSG)
    if (NOT STATUS_CODE EQUAL 0)
        message(FATAL_ERROR "下载失败：${STATUS_MSG}")
    else()
        message(STATUS "下载成功：${JSON_DEST_FILE}")
    endif()
endif()
