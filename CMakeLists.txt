cmake_minimum_required(VERSION 3.16)
project(grpc_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# 方案B: 完全显式定义目标。
# 不再自动扫描 src/。每个目标(工程)包含哪些源文件,由下方手动列出。
# 新增 .cpp 文件后,若想让它加入某个目标,手动把路径加到对应 add_executable。
# 若想让它成为独立 exe,手动 add_executable 一个新目标。
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# 链接项目自带的 gRPC 依赖 (depend/grpc, 目录结构与安装版 grpc-install 一致)
# 依赖的头文件/库文件都放在 depend/grpc 下面, 不依赖机器上已安装的 gRPC。
# 若想改回使用 D:/grpc-install, 把 GRPC_ROOT 改成 "D:/grpc-install" 即可。
# ---------------------------------------------------------------------------
set(GRPC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/depend/grpc")
# 依次放入 CMAKE_PREFIX_PATH: grpc、空目录, 再补上 protobuf/externals
# 这样 find_package 会先命中 depend/grpc 下的 gRPC 和 protobuf。
set(CMAKE_PREFIX_PATH
    "${GRPC_ROOT}"
    "${CMAKE_CURRENT_SOURCE_DIR}/depend"
    "${GRPC_ROOT}/lib/cmake/protobuf/externals"
    ${CMAKE_PREFIX_PATH})

# 先找到 protobuf (grpc 的 gRPCConfig 内部会依赖 protobuf::libprotobuf)
find_package(protobuf CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

# 找到 gRPC 的 protoc 插件 (grpc_cpp_plugin), 用于生成桩代码
get_target_property(GRPC_CPP_PLUGIN_EXECUTABLE gRPC::grpc_cpp_plugin
                    IMPORTED_LOCATION_RELEASE)
if(NOT GRPC_CPP_PLUGIN_EXECUTABLE)
    get_target_property(GRPC_CPP_PLUGIN_EXECUTABLE gRPC::grpc_cpp_plugin
                        IMPORTED_LOCATION)
endif()

# 收集所有 .proto
file(GLOB PROTO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto")

set(PROTO_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${PROTO_GEN_DIR}")

# 为每个 .proto 生成 c++ 源文件和 grpc 桩文件
set(PROTO_GEN_SRCS "")
foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_BASE "${PROTO_FILE}" NAME_WE)
    list(APPEND PROTO_GEN_SRCS
         "${PROTO_GEN_DIR}/${PROTO_BASE}.pb.cc"
         "${PROTO_GEN_DIR}/${PROTO_BASE}.pb.h"
         "${PROTO_GEN_DIR}/${PROTO_BASE}.grpc.pb.cc"
         "${PROTO_GEN_DIR}/${PROTO_BASE}.grpc.pb.h")
endforeach()

add_custom_command(
    OUTPUT ${PROTO_GEN_SRCS}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROTO_GEN_DIR}"
    COMMAND protobuf::protoc
            --cpp_out=${PROTO_GEN_DIR}
            --grpc_out=${PROTO_GEN_DIR}
            --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN_EXECUTABLE}
            -I "${CMAKE_CURRENT_SOURCE_DIR}/proto"
            ${PROTO_FILES}
    DEPENDS ${PROTO_FILES}
    COMMENT "生成 proto 的 C++ 和 gRPC 代码"
)

if(PROTO_FILES)
    add_custom_target(gen_proto ALL DEPENDS ${PROTO_GEN_SRCS})
endif()

# ===========================================================================
# 方案B: 显式定义一个可执行目标(工程)。
# 用法: add_grpc_exe(名字  源文件1  源文件2 ...)
#   只在调用函数时,把你要加入这个工程的 .cpp 传进去。
#   新增文件想进哪个工程,就加到对应的 add_grpc_exe 调用里。
# ===========================================================================
function(add_grpc_exe TARGET_NAME)
    add_executable(${TARGET_NAME} ${ARGN} ${PROTO_GEN_SRCS})
    add_dependencies(${TARGET_NAME} gen_proto)

    # Visual Studio / IDE 分组: 以 src 目录为树根, 每个源文件在工程树里的
    # 文件夹 = 它相对 src 的目录。例如:
    #   src/server/server.cpp      → server 工程下的 "server" 文件夹
    #   src/server/test/Test.cpp   → server 工程下的 "server/test" 文件夹
    #   src/client/client.cpp      → client 工程下的 "client" 文件夹
    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Source Files"
                 FILES ${ARGN})

    # proto 生成文件单独放在 generated 文件夹下 (它们不在 src/ 里)
    source_group("generated" FILES ${PROTO_GEN_SRCS})

    # MSVC + gRPC: 关闭 windows 宏 (min/max, SendMessage 等), 避免头文件被破坏
    target_compile_definitions(${TARGET_NAME} PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN)

    # 源/生成文件含中文, 用 UTF-8 编译, 否则 MSVC 按 GBK 解析会报语法错
    if(MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE /utf-8)
    endif()

    target_include_directories(${TARGET_NAME} PRIVATE
        src
        ${PROTO_GEN_DIR}
    )

    target_link_libraries(${TARGET_NAME} PRIVATE
        gRPC::grpc++
        gRPC::grpc++_reflection
        protobuf::libprotobuf)
endfunction()

# ---------------------------------------------------------------------------
# 目标列表: 每个工程 = 一次 add_grpc_exe 调用, 它的参数是"这个工程包含的文件"。
# 新增文件编进某个工程 → 加到对应调用的参数里。
# 新增一个独立 exe   → 新加一行 add_grpc_exe(名字 文件)。
#
# 当前 src/ 为二级目录结构:
#   src/client/client.cpp      → client 工程
#   src/server/server.cpp      → server 工程
#   src/server/test/Test.cpp   → 编进 server 工程
# ---------------------------------------------------------------------------
add_grpc_exe(server
    src/server/server.cpp
    src/server/test/Test.cpp)
add_grpc_exe(client
    src/client/client.cpp)