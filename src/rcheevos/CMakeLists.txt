add_library(rcheevos STATIC
    src/api/rc_api_common.c
    src/api/rc_api_info.c
    src/api/rc_api_runtime.c
    src/api/rc_api_user.c
    src/client/rc_client.c
    src/client/rc_client_async.c
    src/client/rc_client_do_frame.c
    src/client/rc_client_external.c
    src/client/rc_client_game.c
    src/client/rc_client_info.c
    src/client/rc_client_load.c
    src/client/rc_client_server.c
    src/client/rc_client_user.c
    src/hash/rc_cdreader.c
    src/hash/rc_hash.c
    src/hash/rc_hash_disc.c
    src/hash/rc_hash_encrypted.c
    src/hash/rc_hash_rom.c
    src/hash/rc_hash_zip.c
    src/runtime/rc_alloc.c
    src/runtime/rc_condition.c
    src/runtime/rc_condset.c
    src/runtime/rc_consoleinfo.c
    src/runtime/rc_format.c
    src/runtime/rc_lboard.c
    src/runtime/rc_memref.c
    src/runtime/rc_operand.c
    src/runtime/rc_richpresence.c
    src/runtime/rc_runtime.c
    src/runtime/rc_runtime_progress.c
    src/runtime/rc_trigger.c
    src/runtime/rc_typed_value.c
    src/runtime/rc_validate.c
    src/runtime/rc_value.c
    src/util/md5.c
    src/util/rc_compat.c
    src/util/rc_util.c
    src/util/rc_version.c
)

target_include_directories(rcheevos PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_definitions(rcheevos PUBLIC RC_CLIENT_SUPPORTS_HASH=1)
target_compile_definitions(rcheevos PRIVATE RC_DISABLE_LUA=1)
