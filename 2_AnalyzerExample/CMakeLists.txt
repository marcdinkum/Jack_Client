
add_executable(2_Example main.cpp)
target_link_libraries(2_Example PRIVATE -L"/usr/local/Cellar/jack/1.9.21/lib/libjack.0.dylib")
target_compile_features(2_Example PRIVATE cxx_std_20)
target_include_directories(2_Example PRIVATE "/usr/local/Cellar/jack/1.9.21/include/")