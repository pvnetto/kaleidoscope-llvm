#pragma once

#include "AST.h"

namespace Parser {

	TranslationUnitASTPtr GenerateAST();

	// #### AST parsers
	NumberASTPtr ParseNumberExpr();
	ExprPtr ParseIdentifierExpr();

	ExprPtr ParsePrimary();
	ExprPtr ParseExpr();
	ExprPtr ParseParenthesisExpr();
	ExprPtr ParseBinOpRHS(int minPrecedence, ExprPtr lhs);
	FunctionDeclPtr ParseTopLevelExpr();

	StmtPtr ParseStmt();
	CompoundStmtPtr ParseStmts();
	AssignStmtPtr ParseAssignStmt();
	IfStmtPtr ParseIfStmt();
	ReturnStmtPtr ParseReturnStmt();
	ForStmtPtr ParseForStmt();

	PrototypeASTPtr ParseExtern();
	PrototypeASTPtr ParsePrototype();
	FunctionDeclPtr ParseDefinition();

	// #### Helpers

	template <typename Func, typename R = std::result_of_t<Func && ()>>
	R ExpectSurrounded(char a, Func func, char b) {
		if (s_state.CurrentToken == a) {
			NextToken();

			if (auto result = func()) {
				if (s_state.CurrentToken == b) {
					NextToken();
					return std::move(result);
				}

				return LogErrorPtr<R>("Expected " + b);
			}
		}

		return LogErrorPtr<R>("Expected " + a);
	}

	template <typename Func, typename R = std::result_of_t<Func && ()>>
	R ExpectTrailing(Func func, char s) {
		if (auto result = func()) {
			if (s_state.CurrentToken == s) {
				NextToken();
				return std::move(result);
			}

			return LogErrorPtr<R>("Expected " + s);
		}

		return nullptr;
	}

	template <typename Func, typename R = std::result_of_t<Func && ()>>
	R ExpectSemicolon(Func func) {
		return ExpectTrailing(func, ';');
	}

	ExprPtr LogError(const char *msg);

	template <typename T>
	std::unique_ptr<T> LogErrorT(const char *msg) {
		fprintf(stderr, ">> ERROR: %s\n", msg);
		return nullptr;
	}

	template <typename T>
	T LogErrorPtr(const char *msg) {
		fprintf(stderr, ">> ERROR: %s\n", msg);
		return nullptr;
	}

}