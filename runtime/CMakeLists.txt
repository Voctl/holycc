add_library(holyc_runtime STATIC holyc_runtime.c)
target_include_directories(holyc_runtime PUBLIC ${CMAKE_SOURCE_DIR}/include)
