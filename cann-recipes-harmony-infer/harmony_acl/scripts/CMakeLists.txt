cmake_minimum_required(VERSION 3.16)

project(acl_opensrc)

set(SRC_DIR ../src/)
set(ACL_SRC ${SRC_DIR}/acl_data.cpp
            ${SRC_DIR}/acl_manager.cpp
            ${SRC_DIR}/acl_mdl.cpp
            ${SRC_DIR}/acl_aipp.cpp
            ${SRC_DIR}/ascend_string.cpp
            ${SRC_DIR}/ndk/dynamic_load_helper.cpp
            ${SRC_DIR}/ndk/hiai_ndk_aipp.cpp
            ${SRC_DIR}/ndk/hiai_ndk_executor.cpp
            ${SRC_DIR}/ndk/hiai_ndk_nncore.cpp
            ${SRC_DIR}/ndk/hiai_ndk_options.cpp
            ${SRC_DIR}/ndk/hiai_ndk_tensor.cpp
            ${SRC_DIR}/ndk/ndk_proxy.cpp)

# 设置编译参数
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g3")

# 生成libhiai_ascendcl.so动态库
add_library(hiai_ascendcl SHARED ${ACL_SRC})

if (OHOS_PLATFORM STREQUAL "OHOS")
    # arm64-ohso目标下，编译libhiai_ascendcl库以及BBIT
    target_include_directories(hiai_ascendcl PUBLIC ../include
                                                    ../src
                                                    ../depends
                                                    ${OHOS_INC_DIR}
                                                    ${HMS_INC_DIR})

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__OHOS__")
    target_link_libraries(hiai_ascendcl PUBLIC libhilog_ndk.z.so)
else()
    set(UT_TEST_NDK_DIR ../tests/ut/ndk)
    # x86-linux目标下, 编译libhiai_ascendcl库以及UT
    target_include_directories(hiai_ascendcl PUBLIC ../include
                                                    ../src
                                                    ../depends
                                                    ${UT_TEST_NDK_DIR}/include)
                                                
    # 编译libhiai_foundation桩
    set(UT_TEST_FOUNDATION_SRC ${UT_TEST_NDK_DIR}/hiai_aipp_param.cpp
                               ${UT_TEST_NDK_DIR}/hiai_executor.cpp
                               ${UT_TEST_NDK_DIR}/hiai_helper.cpp
                               ${UT_TEST_NDK_DIR}/hiai_options.cpp
                               ${UT_TEST_NDK_DIR}/hiai_tensor.cpp)
    add_library(hiai_foundation SHARED ${UT_TEST_FOUNDATION_SRC})
    target_include_directories(hiai_foundation PUBLIC  ../src/
                                                       ${UT_TEST_NDK_DIR}/include)

    # 编译libneural_network_core桩
    set(UT_TEST_NNCORE_SRC ${UT_TEST_NDK_DIR}/neural_network_core.cpp)
    add_library(neural_network_core SHARED ${UT_TEST_NNCORE_SRC})
    target_include_directories(neural_network_core PUBLIC ../src/
                                                          ${UT_TEST_NDK_DIR}/include)

    # 编译ut、请确保系统已经安装gtest，可使用如下命令安装sudo apt-get install libgtest-dev
    find_package(GTest REQUIRED)
    set(UT_TEST_DIR ../tests/ut/)
    set(UT_TEST_SRC ${UT_TEST_DIR}/aippParam_ut.cpp
                    ${UT_TEST_DIR}/compiler_ut.cpp
                    ${UT_TEST_DIR}/main.cpp)

    add_executable(ascendcl_ut ${UT_TEST_SRC})
    target_include_directories(ascendcl_ut PUBLIC ../include
                                                  ../src
                                                  ../depends
                                                  ${GTEST_INCLUDE_DIRS})
                    
    target_link_libraries(ascendcl_ut PUBLIC hiai_ascendcl neural_network_core hiai_foundation ${GTEST_LIBRARIES})
endif()







