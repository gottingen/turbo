
add_executable(future_benchmark future_benchmark.cc)
target_link_libraries(future_benchmark ${BENCHMARK_LIB}  ${BENCHMARK_MAIN_LIB} ${TURBO_LIB} ${DYNAMIC_LIB})