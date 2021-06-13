#include "Lexer.h"
#include "Parser.h"
#include "IR.h"

#include <llvm/Support/TargetSelect.h>

static std::string s_source = R"(
# declaring externs
extern sub(a, b);

# sums both numbers together
fn sum(a, b) {
	return a + b;	# returns last computed value by default
}

fn max(a, b) {
	if(a > b) {
		return a;
	}

	return b;
}

fn min(a, b) {
	if(a < b) {
		return a;
	}

	return b;
}

fn clamp(value, min, max) {
	if(value < min) {
		return min;
	}
	else if(value > max) {
		return max;
	}

	return value;
}

fn someformula(a, b) {
    return (1 + 2) + (1 + 2) * (a + b);
}

# top-level expressions are supported
sum(2.0, 3.0);
someformula(2.0 + 5.0 * 3.0, 7);
clamp(20, 50, 100);
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