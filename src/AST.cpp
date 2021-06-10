#include "AST.h"

#include <unordered_map>

namespace Parser {

	void PrintSpacing(int depth) {
		printf("%s", std::string(depth, '\t').c_str());
	}

	void NumberExprAST::Dump(int depth) const {
		PrintSpacing(depth);
		printf("- NumberExprAST: %f\n", m_value);
	}

	void VariableExprAST::Dump(int depth) const {
		PrintSpacing(depth);
		printf("- VariableExprAST: '%s'\n", m_name.c_str());
	}

	void BinaryExprAST::Dump(int depth) const {
		PrintSpacing(depth);
		printf("- BinaryExprAST: op = '%c'\n", m_op);
		m_rhs->Dump(depth + 1);
		m_lhs->Dump(depth + 1);
	}

	void CallExprAST::Dump(int depth) const {
		PrintSpacing(depth);
		printf("- CallExprAST: %s\n", m_calleeName.c_str());
		for (const auto &arg : m_args)
			arg->Dump(depth + 1);
	}

	void ReturnStmtAST::Dump(int depth) const {
		PrintSpacing(depth);
		printf("- ReturnStmtAST:\n");
		m_returnExpr->Dump(depth + 1);
	}

	void CompoundStmtAST::Dump(int depth) const {
		PrintSpacing(depth);
		printf("- CompoundStmtAST:\n");
		for (const auto &stmt : m_statements)
			stmt->Dump(depth + 1);
	}

	void PrototypeDeclAST::Dump(int depth) const {
		PrintSpacing(depth);
		std::string displayName = m_name.empty() ? "__anonymous__" : m_name;
		printf("- PrototypeAST: %s", displayName.c_str());

		printf("(");
		for (int i = 0; i < m_params.size(); i++) {
			printf("%s", m_params[i].c_str());
			if (i < m_params.size() - 1)
				printf(", ");
		}
		printf(")");

		printf("\n");
	}

	void FunctionDeclAST::Dump(int depth) const {
		PrintSpacing(depth);
		printf("- FunctionAST: \n");

		m_prototype->Dump(depth + 1);
		m_body->Dump(depth + 1);
	}

	void TranslationUnitDeclAST::Dump() const {
		printf("TranslationUnitAST: '%s'\n", m_name.c_str());
		for (const auto &proto : m_prototypes)
			proto->Dump(1);

		for (const auto &func : m_functions)
			func->Dump(1);
	}

	NumberExprAST::NumberExprAST(double value) : m_value(value) {}

	VariableExprAST::VariableExprAST(const std::string &name) : m_name(name) {}

	BinaryExprAST::BinaryExprAST(char op, ExprPtr lhs, ExprPtr rhs) : m_op(op), m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

	CallExprAST::CallExprAST(const std::string &name, std::vector<ExprPtr> args) : m_calleeName(name), m_args(std::move(args)) {}

	ReturnStmtAST::ReturnStmtAST(ExprPtr returnExpr) : m_returnExpr(std::move(returnExpr)) {}

	CompoundStmtAST::CompoundStmtAST(std::vector<StmtPtr> stmts) : m_statements(std::move(stmts)) {}

	PrototypeDeclAST::PrototypeDeclAST(std::string name, std::vector<std::string> params) : m_name(name), m_params(params) {}

	FunctionDeclAST::FunctionDeclAST(PrototypeASTPtr prototype, CompoundStmtPtr body) : m_prototype(std::move(prototype)), m_body(std::move(body)) {}

	TranslationUnitDeclAST::TranslationUnitDeclAST(const std::string &name, std::vector<PrototypeASTPtr> protos, std::vector<FunctionASTPtr> funcs) : m_prototypes(std::move(protos)), m_functions(std::move(funcs)) {}
}