#pragma once

#include "AST.h"

namespace Parser {

    TranslationUnitASTPtr GenerateAST();

    // #### AST parsers
    NumberASTPtr ParseNumberExpr();
    ExprASTPtr ParseIdentifierExpr();

    ExprASTPtr ParsePrimary();
    ExprASTPtr ParseExpr();
	ExprASTPtr ParseExprSemicolon();
    ExprASTPtr ParseParenthesisExpr();
    ExprASTPtr ParseBinOpRHS(int minPrecedence, ExprASTPtr lhs);
	FunctionASTPtr ParseTopLevelExpr();

    PrototypeASTPtr ParseExtern();
    PrototypeASTPtr ParsePrototype();
    FunctionASTPtr ParseDefinition();


    // #### Helpers
    ExprASTPtr LogError(const char* msg);

    template <typename T>
    std::unique_ptr<T> LogErrorT(const char* msg) {
        fprintf(stderr, ">> ERROR: %s\n", msg);
        return nullptr;
    }

}