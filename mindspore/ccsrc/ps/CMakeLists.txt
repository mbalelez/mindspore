file(GLOB_RECURSE _PS_SRC_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cc")

if (NOT (ENABLE_CPU AND (ENABLE_D OR ENABLE_GPU)))
    list(REMOVE_ITEM _PS_SRC_FILES "optimizer_info_builder.cc")
    list(REMOVE_ITEM _PS_SRC_FILES "optimizer_info.cc")
    list(REMOVE_ITEM _PS_SRC_FILES "scheduler.cc")
    list(REMOVE_ITEM _PS_SRC_FILES "util.cc")
    list(REMOVE_ITEM _PS_SRC_FILES "comm/http_message_handler.cc")
    list(REMOVE_ITEM _PS_SRC_FILES "comm/http_server.cc")
    list(REMOVE_ITEM _PS_SRC_FILES "comm/comm_util.cc")
    list(REMOVE_ITEM _PS_SRC_FILES "comm/tcp_client.cc")
    list(REMOVE_ITEM _PS_SRC_FILES "comm/tcp_message_handler.cc")
    list(REMOVE_ITEM _PS_SRC_FILES "comm/tcp_server.cc")
endif()

set_property(SOURCE ${_PS_SRC_FILES} PROPERTY COMPILE_DEFINITIONS SUBMODULE_ID=mindspore::SubModuleId::SM_PS)
add_library(_mindspore_ps_obj OBJECT ${_PS_SRC_FILES})
