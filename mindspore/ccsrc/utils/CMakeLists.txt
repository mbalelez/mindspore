file(GLOB_RECURSE _UTILS_SRC_LIST ./*.cc)

if (NOT ENABLE_GE)
    file(GLOB_RECURSE _UTILS_GE_SRC_FILES ./callbacks_ge.cc)
    list(REMOVE_ITEM _UTILS_SRC_LIST ${_UTILS_GE_SRC_FILES})
endif ()

file(GLOB_RECURSE _UTILS_LITE_SRC_FILES
        ./load_onnx/anf_converter.cc
        ./load_onnx/anf_model_parser.cc
        )
list(REMOVE_ITEM _UTILS_SRC_LIST ${_UTILS_LITE_SRC_FILES})

set_property(SOURCE ${_UTILS_SRC_LIST} PROPERTY COMPILE_DEFINITIONS SUBMODULE_ID=mindspore::SubModuleId::SM_UTILS)
add_library(_mindspore_utils_obj OBJECT ${_UTILS_SRC_LIST})
