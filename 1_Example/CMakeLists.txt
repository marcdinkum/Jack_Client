add_executable(1_Example main.cpp)
target_link_libraries(1_Example PRIVATE -L"/usr/local/Cellar/jack/1.9.21/lib/libjack.0.dylib")
target_compile_features(1_Example PRIVATE cxx_std_20)
target_include_directories(1_Example PRIVATE "/usr/local/Cellar/jack/1.9.21/include/")