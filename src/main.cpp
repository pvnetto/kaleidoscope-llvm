#include "Lexer.h"
#include "Parser.h"
#include "IR.h"

#include <llvm/Support/TargetSelect.h>

static std::string s_source = R"(
# declaring externs
extern sub(a, b);

fn sum(a, b) {
	a + b;	# returns last computed value by default
}

fn max(a, b) {
	x = 5;
	w = z = a + b + x;
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

fn testfor(a, b) {
	x = 0;
	for (i = 1; i < a; 1.0) {
		x = x + i;
		if(i > b) {
			return x;
		}
	}

	return b;
}

# top-level expressions are supported
x = 2.0;
y = 3.0;
sum(x, y);
max(2.0 + 5.0 * 3.0, 7);
clamp(20, 50, 100);
testfor(1000.0, 200.0);
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