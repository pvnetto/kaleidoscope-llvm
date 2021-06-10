#pragma once

#include "AST.h"

namespace Parser {

	TranslationUnitASTPtr GenerateAST();

	// #### AST parsers
	NumberASTPtr ParseNumberExpr();
	ExprPtr ParseIdentifierExpr();

	ExprPtr ParsePrimary();
	ExprPtr ParseExpr();
	ExprPtr ParseExprSemicolon();
	ExprPtr ParseParenthesisExpr();
	ExprPtr ParseBinOpRHS(int minPrecedence, ExprPtr lhs);
	FunctionASTPtr ParseTopLevelExpr();

	StmtPtr ParseStmt();
	ReturnStmtPtr ParseReturnStmt();

	PrototypeASTPtr ParseExtern();
	PrototypeASTPtr ParsePrototype();
	FunctionASTPtr ParseDefinition();

	// #### Helpers
	ExprPtr LogError(const char *msg);

	template <typename T>
	std::unique_ptr<T> LogErrorT(const char *msg) {
		fprintf(stderr, ">> ERROR: %s\n", msg);
		return nullptr;
	}

}