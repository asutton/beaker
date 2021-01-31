
add_executable(beaker-fuzz
  fuzzer.cpp
  main.cpp)
target_link_libraries(beaker-fuzz beaker-frontend)

