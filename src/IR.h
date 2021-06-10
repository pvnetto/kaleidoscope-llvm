#pragma once

#include "AST.h"

namespace IR {
	void GenerateCode(Parser::TranslationUnitASTPtr unit);
	void JITCompile();
}