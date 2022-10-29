add_executable(JackTest main.cpp)
target_link_libraries(JackTest PRIVATE -L"/usr/local/Cellar/jack/1.9.21/lib/libjack.0.dylib")
target_compile_features(JackTest PRIVATE cxx_std_20)
target_include_directories(JackTest PRIVATE "/usr/local/Cellar/jack/1.9.21/include/")