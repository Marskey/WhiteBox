set (cjson_src
    fpconv.c
    lua_cjson.c
    strbuf.c
)

add_library(cjson ${cjson_src})
