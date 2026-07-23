set(SOURCES
    token.c
    diag.c
    utils.c
    lexer/lexer.c
    ast/ast.c
    types/types.c
    symbol/symbol.c
    parser/parser.c
    semantic/semantic.c
    codegen/codegen.c
    driver/driver.c
    main.c
)

add_library(holyc_core STATIC ${SOURCES})

set(EXECUTABLE_NAME holycc)
add_executable(${EXECUTABLE_NAME} ${SOURCES})

target_include_directories(holyc_core PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_include_directories(${EXECUTABLE_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)
