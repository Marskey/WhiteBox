set (cjson_src
    "fpconv.c"
    "lua_cjson.c"
    "strbuf.c"
)

add_library(cjson ${cjson_src})
target_include_directories(cjson PUBLIC ${lua_source_dir})
