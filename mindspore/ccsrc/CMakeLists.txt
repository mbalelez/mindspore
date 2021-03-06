## common setting
include_directories(${CMAKE_SOURCE_DIR}/mindspore/core)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR})
link_directories(${CMAKE_SOURCE_DIR}/build/mindspore/graphengine)

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-attributes -DHAVE_SNPRINTF")
    add_compile_definitions(BUILDING_DLL)
endif()

if (ENABLE_MPI)
    add_compile_definitions(ENABLE_MPI)
endif ()

if(ENABLE_GPU)
    find_package(CUDA REQUIRED)
    find_package(Threads)
    if(${CUDA_VERSION} VERSION_LESS ${MS_REQUIRE_CUDA_VERSION})
      message(FATAL_ERROR "The minimum CUDA version ${MS_REQUIRE_CUDA_VERSION} is required, but only CUDA ${CUDA_VERSION} found.")
    endif()
    enable_language(CUDA)
    if(NOT CUDA_PATH OR CUDA_PATH STREQUAL "")
        if(DEFINED ENV{CUDA_HOME} AND NOT $ENV{CUDA_HOME} STREQUAL "")
            set(CUDA_PATH $ENV{CUDA_HOME})
        else()
            set(CUDA_PATH ${CUDA_TOOLKIT_ROOT_DIR})
        endif()
    endif()

    if (DEFINED ENV{CUDNN_HOME} AND NOT $ENV{CUDNN_HOME} STREQUAL "")
        set(CUDNN_INCLUDE_DIR $ENV{CUDNN_HOME}/include)
        set(CUDNN_LIBRARY_DIR $ENV{CUDNN_HOME}/lib64)
        find_path(CUDNN_INCLUDE_PATH cudnn.h HINTS ${CUDNN_INCLUDE_DIR} NO_DEFAULT_PATH)
        find_library(CUDNN_LIBRARY_PATH "cudnn" HINTS ${CUDNN_LIBRARY_DIR} NO_DEFAULT_PATH)
        if (CUDNN_INCLUDE_PATH STREQUAL CUDNN_INCLUDE_PATH-NOTFOUND)
            message(FATAL_ERROR "Failed to find cudnn header file, please set environment variable CUDNN_HOME to cudnn installation position.")
        endif()
        if (CUDNN_LIBRARY_PATH STREQUAL CUDNN_LIBRARY_PATH-NOTFOUND)
            message(FATAL_ERROR "Failed to find cudnn library file, please set environment variable CUDNN_HOME to cudnn installation position.")
        endif()
    else()
        list(APPEND CMAKE_PREFIX_PATH  ${CUDA_TOOLKIT_ROOT_DIR})
        find_path(CUDNN_INCLUDE_PATH cudnn.h PATH_SUFFIXES cuda/inclulde include cuda)
        find_library(CUDNN_LIBRARY_PATH "cudnn" PATH_SUFFIXES cuda/lib64 lib64 lib cuda/lib lib/x86_64-linux-gnu)
        if (CUDNN_INCLUDE_PATH STREQUAL CUDNN_INCLUDE_PATH-NOTFOUND)
            message(FATAL_ERROR "Failed to find cudnn header file, if cudnn library is not installed, please put cudnn header file in cuda include path \
                    or user include path(eg. /usr/local/cuda/include; /usr/local/include; /usr/include), if cudnn library is installed in other position,\
                    please set environment variable CUDNN_HOME to cudnn installation position, there should be cudnn.h in {CUDNN_HOME}/include.")
        endif()
        if (CUDNN_LIBRARY_PATH STREQUAL CUDNN_LIBRARY_PATH-NOTFOUND)
            message(FATAL_ERROR "Failed to find cudnn library file, if cudnn library is not installed, please put cudnn library file in \
                    cuda library path or user library path(eg. /usr/local/cuda/lib64; /usr/local/lib64; /usr/lib64; /usr/local/lib; /usr/lib),\
                    if cudnn library is installed in other position, please set environment variable CUDNN_HOME to cudnn installation position, \
                    there should be cudnn library file in {CUDNN_HOME}/lib64.")
        endif()
    endif()

    if(NOT CUPTI_INCLUDE_DIRS OR CUPTI_INCLUDE_DIRS STREQUAL "")
        set(CUPTI_INCLUDE_DIRS  ${CUDA_PATH}/extras/CUPTI/include)
    endif()
    message("CUDA_PATH: ${CUDA_PATH}")
    message("CUDA_INCLUDE_DIRS: ${CUDA_INCLUDE_DIRS}")
    message("CUDNN_INCLUDE_PATH: ${CUDNN_INCLUDE_PATH}")
    message("CUDNN_LIBRARY_PATH: ${CUDNN_LIBRARY_PATH}")
    message("CUPTI_INCLUDE_DIRS: ${CUPTI_INCLUDE_DIRS}")
    include_directories(${CUDNN_INCLUDE_PATH} ${CUDA_PATH} ${CUDA_INCLUDE_DIRS} ${CUPTI_INCLUDE_DIRS})

    file(GLOB_RECURSE GPU_SRC_LIST RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
            "runtime/device/gpu/*.cc"
            "runtime/device/gpu/*.cu"
            "backend/kernel_compiler/gpu/*.cu"
            "backend/kernel_compiler/akg/gpu/*.cc"
            "backend/kernel_compiler/akg/akg_kernel_json_generator.cc"
            "backend/kernel_compiler/akg/akg_kernel_json_decoder.cc"
            "backend/kernel_compiler/akg/akg_kernel_attrs_process.cc"
            )

    list(APPEND CUDA_NVCC_FLAGS -arch=sm_53 --expt-relaxed-constexpr)
    list(REMOVE_ITEM GPU_SRC_LIST "runtime/device/gpu/blocking_queue.cc" "runtime/device/gpu/gpu_buffer_mgr.cc")
    list(REMOVE_ITEM GPU_SRC_LIST "runtime/device/gpu/mpi/mpi_initializer.cc"
                                  "runtime/device/gpu/distribution/collective_wrapper.cc"
                                  "runtime/device/gpu/distribution/mpi_wrapper.cc"
                                  "runtime/device/gpu/distribution/nccl_wrapper.cc"
                                  )

    set(NVCC_TMP_CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    string(REPLACE "-std=c++17" "-std=c++11" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set_property(SOURCE ${GPU_SRC_LIST} PROPERTY COMPILE_DEFINITIONS SUBMODULE_ID=mindspore::SubModuleId::SM_DEVICE)
    cuda_add_library(gpu_cuda_lib STATIC ${GPU_SRC_LIST})
    set(CMAKE_CXX_FLAGS ${NVCC_TMP_CMAKE_CXX_FLAGS})
    add_compile_definitions(ENABLE_GPU)
endif ()


## make protobuf files
file(COPY "${ms_onnx_INC}/onnx/onnx.proto" DESTINATION ${CMAKE_BINARY_DIR}/proto)
file(GLOB ONNX_PROTO "" ${CMAKE_BINARY_DIR}/proto/onnx.proto)
message("onnx proto path is :" ${ONNX_PROTO})
ms_protobuf_generate(ONNX_PROTO_SRCS ONNX_PROTO_HDRS ${ONNX_PROTO})
list(APPEND MINDSPORE_PROTO_LIST ${ONNX_PROTO_SRCS})

if (ENABLE_DEBUGGER)
    # debugger: compile proto files
    include_directories("${CMAKE_BINARY_DIR}/debug/debugger")
    file(GLOB_RECURSE DEBUGGER_PROTO_LIST RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "debug/debugger/debug_graph.proto")
    ms_protobuf_generate(DEBUGGER_PROTO_SRCS DEBUGGER_PROTO_HDRS ${DEBUGGER_PROTO_LIST})
    file(GLOB_RECURSE DEBUGGER_GRPC_LIST RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "debug/debugger/debug_grpc.proto")
    ms_grpc_generate(DEBUGGER_GRPC_SRCS DEBUGGER_GRPC_HDRS ${DEBUGGER_GRPC_LIST})
    list(APPEND MINDSPORE_PROTO_LIST ${DEBUGGER_PROTO_SRCS})
    list(APPEND MINDSPORE_PROTO_LIST ${DEBUGGER_GRPC_SRCS})
endif ()

if (ENABLE_DUMP_PROTO)
    include_directories(${CMAKE_BINARY_DIR})

    file(GLOB_RECURSE PROTO_PY RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        "utils/anf_ir.proto"
        "utils/summary.proto"
        "utils/lineage.proto"
        "utils/checkpoint.proto"
        "utils/print.proto"
	"utils/node_strategy.proto"
    )
    ms_protobuf_generate_py(PY_SRCS PY_HDRS PY_PYS ${PROTO_PY})

    list(APPEND MINDSPORE_PROTO_LIST ${PROTO_SRCS})
    list(APPEND MINDSPORE_PROTO_LIST ${PY_SRCS})
endif ()

if (ENABLE_D)
    include_directories("${CMAKE_BINARY_DIR}/backend/kernel_compiler/aicpu")
    file(GLOB_RECURSE PROTO_IN RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "backend/kernel_compiler/aicpu/proto/*.proto")
    ms_protobuf_generate(PROTOSRCS PROTOHDRS ${PROTO_IN})

    file(GLOB_RECURSE PROTO_DUMP RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "runtime/device/ascend/dump/proto/*.proto")
    ms_protobuf_generate(DUMP_PROTOSRCS PROTOHDRS ${PROTO_DUMP})

    list(APPEND MINDSPORE_PROTO_LIST ${PROTOSRCS})
    list(APPEND MINDSPORE_PROTO_LIST ${PREDICT_PROTOSRCS})
    list(APPEND MINDSPORE_PROTO_LIST ${DUMP_PROTOSRCS})

    add_compile_definitions(ENABLE_D)
endif ()

if (MINDSPORE_PROTO_LIST)
    add_library(proto_input STATIC ${MINDSPORE_PROTO_LIST})
    set_target_properties(proto_input PROPERTIES COMPILE_FLAGS "-Wno-unused-variable")
endif()

## make sub objects
set(SUB_COMP
        transform/graph_ir
        transform/onnx
        backend/optimizer
        backend/kernel_compiler
        backend/session
        runtime/device
        frontend/optimizer
        frontend/parallel
        frontend/operator
        pipeline/jit
        pipeline/pynative
        common debug pybind_api utils vm profiler ps
)

foreach (_comp ${SUB_COMP})
    add_subdirectory(${_comp})
    string(REPLACE "/" "_" sub ${_comp})
    if (TARGET _mindspore_${sub}_obj)
        list(APPEND SUB_OBJECTS_SRC $<TARGET_OBJECTS:_mindspore_${sub}_obj>)
        add_dependencies(_mindspore_${sub}_obj proto_input )
    endif ()
endforeach ()

set_property(SOURCE ${SUB_OBJECTS_SRC} PROPERTY COMPILE_DEFINITIONS SUBMODULE_ID=mindspore::SubModuleId::SM_ME)
add_library(mindspore STATIC ${SUB_OBJECTS_SRC})

target_link_libraries(mindspore mindspore_core)

if (ENABLE_DEBUGGER)
    # debugger: link grpc
    target_link_libraries(proto_input mindspore::grpc++)
endif()

target_link_libraries(mindspore securec mindspore::flatbuffers)

if (NOT WIN32)
  target_link_libraries(mindspore dl)
endif()

if (ENABLE_GE)
    if(ENABLE_TRAIN)
        target_link_libraries(mindspore ge_runner hccl)
    else ()
        target_link_libraries(mindspore ge_client)
    endif ()
    target_link_libraries(mindspore graph tsdclient datatransfer)
endif()

if (ENABLE_D)
    if (DEFINED ENV{D_LINK_PATH})
        if (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "aarch64")
            MESSAGE("system processor matches aarch64")
            set(D_LIB_PATH $ENV{D_LINK_PATH}/aarch64)
        elseif (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64")
            MESSAGE("system processor matches x86_64")
            set(D_LIB_PATH $ENV{D_LINK_PATH}/x86_64)
        else ()
            MESSAGE("system ${CMAKE_HOST_SYSTEM_PROCESSOR} not support")
        endif()
    else ()
        MESSAGE("use system default lib")
        if (DEFINED ENV{ASCEND_CUSTOM_PATH})
            set(ASCEND_PATH $ENV{ASCEND_CUSTOM_PATH})
        else ()
            set(ASCEND_PATH /usr/local/Ascend)
        endif ()
        set(ASCEND_DRIVER_PATH ${ASCEND_PATH}/driver/lib64/common)
        set(ASCEND_DRIVER_BACK_PATH ${ASCEND_PATH}/driver/lib64/driver)
        set(ASCEND_RUNTIME_PATH ${ASCEND_PATH}/fwkacllib/lib64)
        set(ASCEND_OPP_PATH ${ASCEND_PATH}/opp/op_impl/built-in/ai_core/tbe/op_tiling)
    endif()

    MESSAGE("USE DAV LIB PATH: ${ASCEND_PATH}")
    find_library(HCCL hccl ${ASCEND_RUNTIME_PATH})
    find_library(CCE_LIB cce ${ASCEND_RUNTIME_PATH})
    find_library(RUNTIME_LIB runtime ${ASCEND_RUNTIME_PATH})
    find_library(TSDCLIENT tsdclient HINTS ${ASCEND_RUNTIME_PATH} ${ASCEND_DRIVER_BACK_PATH})
    find_library(DATATRANSFER datatransfer HINTS ${ASCEND_RUNTIME_PATH} ${ASCEND_DRIVER_BACK_PATH})
    find_library(PROFILING msprofiler ${ASCEND_RUNTIME_PATH})
    find_library(PROFILING_SHARED msprof ${ASCEND_DRIVER_PATH})
    find_library(REGISTER register ${ASCEND_RUNTIME_PATH})
    find_library(OPTILING optiling ${ASCEND_OPP_PATH})
    add_library(ms_profile SHARED ${PROFILING})
    set_target_properties(ms_profile PROPERTIES LINKER_LANGUAGE CXX)
    target_link_libraries(ms_profile -Wl,--start-group ${PROFILING_SHARED} ${PROFILING} mindspore::protobuf -Wl,--end-group)
    target_link_libraries(mindspore ms_profile ge_runtime ${CCE_LIB} ${RUNTIME_LIB} ${TSDCLIENT} ${HCCL} ${DATATRANSFER} ${REGISTER} -Wl,--no-as-needed ${OPTILING} -Wl,--as-needed)
    target_link_libraries(mindspore -Wl,--start-group proto_input mindspore::protobuf -Wl,--end-group)
elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")
    target_link_libraries(mindspore -Wl,--start-group proto_input mindspore::protobuf mindspore::sentencepiece -Wl,--end-group)
else ()
    target_link_libraries(mindspore -Wl,--start-group proto_input mindspore::protobuf -Wl,--end-group)
endif()

# set c_expression building
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set_property(SOURCE "pipeline/jit/init.cc" PROPERTY COMPILE_DEFINITIONS SUBMODULE_ID=mindspore::SubModuleId::SM_PIPELINE)
pybind11_add_module(_c_expression "pipeline/jit/init.cc")

MESSAGE(STATUS "operation system is ${CMAKE_SYSTEM}")
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_link_options(_c_expression PRIVATE -Wl,-init,mindspore_log_init)
    set(ORIGIN_PATH $ORIGIN)
elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set_target_properties(_c_expression PROPERTIES MACOSX_RPATH ON)
    set(ORIGIN_PATH @loader_path)
elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(ORIGIN_PATH $ORIGIN)
else ()
    MESSAGE(FATAL_ERROR "other platform: ${CMAKE_SYSTEM_NAME}")
endif ()
set(MINDSPORE_RPATH ${ORIGIN_PATH}/lib)
if (ENABLE_D)
    set(MINDSPORE_RPATH ${MINDSPORE_RPATH}:/usr/local/Ascend/nnae/latest/fwkacllib/lib64)
    set(MINDSPORE_RPATH ${MINDSPORE_RPATH}:/usr/local/Ascend/ascend-toolkit/latest/fwkacllib/lib64)
    set(MINDSPORE_RPATH ${MINDSPORE_RPATH}:/usr/local/Ascend/fwkacllib/lib64)
    set(MINDSPORE_RPATH ${MINDSPORE_RPATH}:/usr/local/Ascend/add-ons)
    set(MINDSPORE_RPATH ${MINDSPORE_RPATH}:/usr/local/Ascend/opp/op_impl/built-in/ai_core/tbe/op_tiling)
    set(MINDSPORE_RPATH ${MINDSPORE_RPATH}:/usr/local/Ascend/nnae/latest/opp/op_impl/built-in/ai_core/tbe/op_tiling)
    set(MINDSPORE_RPATH ${MINDSPORE_RPATH}:/usr/local/Ascend/ascend-toolkit/latest/opp/op_impl/built-in/ai_core/tbe/op_tiling)
elseif (ENABLE_GPU)
    set(MINDSPORE_RPATH ${MINDSPORE_RPATH}:/usr/local/cuda/lib64)
endif ()

set_target_properties(_c_expression PROPERTIES INSTALL_RPATH ${MINDSPORE_RPATH})

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    target_link_libraries(mindspore mindspore::pybind11_module)
    target_link_libraries(mindspore mindspore_gvar)
    target_link_libraries(_c_expression PRIVATE -Wl,--whole-archive mindspore -Wl,--no-whole-archive)
else ()
    if (ENABLE_CPU AND (ENABLE_D OR ENABLE_GPU))
        target_link_libraries(mindspore mindspore::pslite mindspore::protobuf mindspore::event mindspore::event_pthreads ${zeromq_DIRPATH}/zmq_install/lib/libzmq.a)
        if (${ENABLE_IBVERBS} STREQUAL "ON")
            target_link_libraries(mindspore ibverbs rdmacm)
        endif()
    endif()
    target_link_libraries(_c_expression PRIVATE -Wl,--whole-archive mindspore proto_input -Wl,--no-whole-archive)
    target_link_libraries(_c_expression PRIVATE mindspore::pybind11_module)
    target_link_libraries(_c_expression PRIVATE mindspore_gvar)
endif ()

if (USE_GLOG)
    target_link_libraries(_c_expression PRIVATE mindspore::glog)
endif ()

if (ENABLE_GPU)
    message("add gpu lib to c_expression")
    target_link_libraries(_c_expression PRIVATE gpu_cuda_lib gpu_queue cublas
                          ${CUDA_PATH}/lib64/libcurand.so
                          ${CUDNN_LIBRARY_PATH}
                          ${CUDA_PATH}/lib64/libcudart.so
                          ${CUDA_PATH}/lib64/stubs/libcuda.so
                          ${CUDA_PATH}/lib64/libcusolver.so)
    if (ENABLE_MPI)
        set_target_properties(_ms_mpi PROPERTIES INSTALL_RPATH ${MINDSPORE_RPATH})
    endif()
endif ()

if (ENABLE_CPU)
    target_link_libraries(_c_expression PRIVATE mindspore::dnnl mindspore::mkldnn)
endif ()

if (ENABLE_MINDDATA)
    add_subdirectory(minddata/mindrecord)
    add_subdirectory(minddata/dataset)
endif ()

# build inference
set(LOAD_ONNX_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/utils/load_onnx/anf_converter.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/utils/load_onnx/anf_model_parser.cc
        )
add_library(inference SHARED
        ${CMAKE_CURRENT_SOURCE_DIR}/backend/session/infer_session.cc
        ${LOAD_ONNX_SRC}
        )

set_target_properties(inference PROPERTIES INSTALL_RPATH ${MINDSPORE_RPATH})
target_link_libraries(inference PRIVATE ${PYTHON_LIBRARIES} ${SECUREC_LIBRARY}
        -Wl,--whole-archive mindspore proto_input -Wl,--no-whole-archive mindspore_gvar)

if (ENABLE_D)
    target_link_libraries(_c_expression PRIVATE ${ASCEND_PATH}/fwkacllib/lib64/libadump_server.a)
    target_link_libraries(inference PRIVATE ${ASCEND_PATH}/fwkacllib/lib64/libadump_server.a)
endif()

if (ENABLE_CPU)
    target_link_libraries(inference PRIVATE mindspore::dnnl mindspore::mkldnn)
endif ()

if (USE_GLOG)
    target_link_libraries(inference PRIVATE mindspore::glog)
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_link_options(inference PRIVATE -Wl,-init,common_log_init)
elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set_target_properties(inference PROPERTIES MACOSX_RPATH ON)
endif ()
