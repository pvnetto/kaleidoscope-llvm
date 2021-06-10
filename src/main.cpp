#include "Lexer.h"
#include "Parser.h"
#include "IR.h"

#include <llvm/Support/TargetSelect.h>

static std::string s_source = R"(
# declaring externs
extern sub(a, b);
extern print();

# sums both numbers together
fn sum(a, b) {
	a + b;
}

fn someformula(a, b) {
    (1 + 2) + (1 + 2) * (a + b);
}

# calls sum
sum(2.0 + 5.0 * 3.0, 7);
)";


int main() {
	// Initializes LLVM target architecture
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();
	llvm::InitializeNativeTargetAsmParser();

	// Compiles source code
    Lexer::Init(s_source);
	if (auto unit = Parser::GenerateAST()) {
		IR::GenerateCode(std::move(unit));
		IR::JITCompile();
	}

    return 0;
}