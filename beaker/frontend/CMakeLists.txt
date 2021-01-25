
add_library(beaker-frontend STATIC
  location.cpp
  token.cpp
  syntax.cpp
  lexer.cpp
  parser.cpp
  first/first_parser.cpp
  second/second_parser.cpp
  fourth/fourth_parser.cpp)
target_link_libraries(beaker-frontend PUBLIC beaker-language)
