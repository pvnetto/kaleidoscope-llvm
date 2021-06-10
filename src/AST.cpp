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

	void PrototypeAST::Dump(int depth) const {
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

	void FunctionAST::Dump(int depth) const {
		PrintSpacing(depth);
		printf("- FunctionAST: \n");

		m_prototype->Dump(depth + 1);
		m_body->Dump(depth + 1);
	}

	void TranslationUnitAST::Dump() const {
		printf("TranslationUnitAST: '%s'\n", m_name.c_str());
		for (const auto &proto : m_prototypes)
			proto->Dump(1);

		for (const auto &func : m_functions)
			func->Dump(1);
	}

	NumberExprAST::NumberExprAST(double value) : m_value(value) {}

	VariableExprAST::VariableExprAST(const std::string &name) : m_name(name) {}

	BinaryExprAST::BinaryExprAST(char op, ExprASTPtr lhs, ExprASTPtr rhs) : m_op(op), m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

	CallExprAST::CallExprAST(const std::string &name, std::vector<ExprASTPtr> args) : m_calleeName(name), m_args(std::move(args)) {}

	PrototypeAST::PrototypeAST(std::string name, std::vector<std::string> params) : m_name(name), m_params(params) {}

	FunctionAST::FunctionAST(PrototypeASTPtr prototype, ExprASTPtr body) : m_prototype(std::move(prototype)), m_body(std::move(body)) {}

	TranslationUnitAST::TranslationUnitAST(const std::string &name, std::vector<PrototypeASTPtr> protos, std::vector<FunctionASTPtr> funcs) :
		m_prototypes(std::move(protos)), m_functions(std::move(funcs)) {}
}