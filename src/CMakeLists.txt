add_executable(Kaleidoscope main.cpp Lexer.cpp Parser.cpp AST.cpp "KailedoscopeJIT.h" "IR.h" "IR.cpp")

target_link_libraries(Kaleidoscope ${llvm-libs})