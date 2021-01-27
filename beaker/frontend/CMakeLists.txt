
add_library(beaker-frontend STATIC
  location.cpp
  token.cpp
  syntax.cpp
  lexer.cpp
  parser.cpp)
target_link_libraries(beaker-frontend PUBLIC beaker-language)
