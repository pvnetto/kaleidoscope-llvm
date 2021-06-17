#include "AST.h"

#include <unordered_map>

namespace Parser {

	void PrintSpacing(int depth) {
		printf("%s", std::string(depth, '\t').c_str());
	}

	void NumberExpr::Dump(int depth) const {
		PrintSpacing(depth);
		printf("- NumberExpr: %f\n", m_value);
	}

	void VariableExpr::Dump(int depth) const {
		PrintSpacing(depth);
		printf("- VariableExpr: '%s'\n", m_name.c_str());
	}

	void BinaryExpr::Dump(int depth) const {
		PrintSpacing(depth);
		printf("- BinaryExpr: op = '%c'\n", m_op);
		m_rhs->Dump(depth + 1);
		m_lhs->Dump(depth + 1);
	}

	void CallExpr::Dump(int depth) const {
		PrintSpacing(depth);
		printf("- CallExpr: %s\n", m_calleeName.c_str());
		for (const auto &arg : m_args)
			arg->Dump(depth + 1);
	}

	void CompoundStmt::Dump(int depth) const {
		PrintSpacing(depth);
		printf("- CompoundStmt:\n");
		for (const auto &stmt : m_statements)
			stmt->Dump(depth + 1);
	}

	void AssignStmt::Dump(int depth) const {
		PrintSpacing(depth);
		printf("- AssignStmt:\n");
		for (const auto &lhs : m_lhs) {
			lhs->Dump(depth + 1);
		}
		m_rhs->Dump(depth + 1);
	}

	void ReturnStmt::Dump(int depth) const {
		PrintSpacing(depth);
		printf("- ReturnStmt:\n");
		m_returnExpr->Dump(depth + 1);
	}

	void IfStmt::Dump(int depth) const {
		PrintSpacing(depth);
		printf("- IfStmt: %s\n", m_else ? "has_else" : "");
		m_condition->Dump(depth + 1);
		m_body->Dump(depth + 1);

		if (m_else)
			m_else->Dump(depth + 1);
	}

	void ForStmt::Dump(int depth) const {
		PrintSpacing(depth);
		printf("- ForStmt: \n");
		m_value->Dump(depth + 1);
		m_condition->Dump(depth + 1);
		m_step->Dump(depth + 1);
		m_body->Dump(depth + 1);
	}

	void PrototypeDecl::Dump(int depth) const {
		PrintSpacing(depth);
		std::string displayName = m_name.empty() ? "__anonymous__" : m_name;
		printf("- PrototypeDecl: %s", displayName.c_str());

		printf("(");
		for (int i = 0; i < m_params.size(); i++) {
			printf("%s", m_params[i].c_str());
			if (i < m_params.size() - 1)
				printf(", ");
		}
		printf(")");

		printf("\n");
	}

	void FunctionDecl::Dump(int depth) const {
		PrintSpacing(depth);
		printf("- FunctionDecl: \n");

		m_prototype->Dump(depth + 1);
		m_body->Dump(depth + 1);
	}

	void TranslationUnitDecl::Dump() const {
		printf("TranslationUnitDecl: '%s'\n", m_name.c_str());
		for (const auto &proto : m_prototypes)
			proto->Dump(1);

		for (const auto &func : m_functions)
			func->Dump(1);
	}

	NumberExpr::NumberExpr(double value) : m_value(value) {}

	VariableExpr::VariableExpr(const std::string &name) : m_name(name) {}

	BinaryExpr::BinaryExpr(char op, ExprPtr lhs, ExprPtr rhs) : m_op(op), m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

	CallExpr::CallExpr(const std::string &name, std::vector<ExprPtr> args) : m_calleeName(name), m_args(std::move(args)) {}

	ReturnStmt::ReturnStmt(ExprPtr returnExpr) : m_returnExpr(std::move(returnExpr)) {}

	IfStmt::IfStmt(ExprPtr cond, CompoundStmtPtr body, CompoundStmtPtr elseStmt) : m_condition(std::move(cond)), m_body(std::move(body)), m_else(std::move(elseStmt)) {}

	ForStmt::ForStmt(const std::string &varName, ExprPtr value, ExprPtr cond, ExprPtr step, CompoundStmtPtr body) : m_loopVarName(varName), m_value(std::move(value)), m_condition(std::move(cond)), m_step(std::move(step)), m_body(std::move(body)) {}

	CompoundStmt::CompoundStmt(std::vector<StmtPtr> stmts) : m_statements(std::move(stmts)) {}

	AssignStmt::AssignStmt(std::vector<VariableExprPtr> lhs, ExprPtr rhs) : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

	PrototypeDecl::PrototypeDecl(std::string name, std::vector<std::string> params) : m_name(name), m_params(params) {}

	FunctionDecl::FunctionDecl(PrototypeASTPtr prototype, CompoundStmtPtr body) : m_prototype(std::move(prototype)), m_body(std::move(body)) {}

	TranslationUnitDecl::TranslationUnitDecl(const std::string &name, std::vector<PrototypeASTPtr> protos, std::vector<FunctionDeclPtr> funcs) : m_prototypes(std::move(protos)), m_functions(std::move(funcs)) {}
}