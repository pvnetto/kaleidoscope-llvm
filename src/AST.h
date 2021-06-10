#pragma once

#include <memory>
#include <string>
#include <vector>

#include <llvm/IR/Value.h>

#define ANON_EXPR_NAME "__anon_expr"

namespace Parser {

	// Generic AST node for expressions
	class ExprAST {
	public:
		virtual ~ExprAST() {}

		virtual llvm::Value *GenerateCode() = 0;
		virtual void Dump(int depth) const = 0;
	};

	using ExprASTPtr = std::unique_ptr<ExprAST>;

	// Numeric literals
	class NumberExprAST : public ExprAST {
	public:
		NumberExprAST(double value);
		virtual llvm::Value *GenerateCode() override;
		virtual void Dump(int depth) const override;

	private:
		double m_value;
	};
	using NumberASTPtr = std::unique_ptr<NumberExprAST>;

	// Variables
	class VariableExprAST : public ExprAST {
	public:
		VariableExprAST(const std::string &name);
		virtual llvm::Value *GenerateCode() override;
		virtual void Dump(int depth) const override;

	private:
		std::string m_name;
	};

	// binary expressions (i.e 1 + 2)
	//		::= <lhs_expr> <operator> <rhs_expr>
	class BinaryExprAST : public ExprAST {
	public:
		BinaryExprAST(char op, ExprASTPtr lhs, ExprASTPtr rhs);
		virtual llvm::Value *GenerateCode() override;
		virtual void Dump(int depth) const override;

	private:
		char m_op;               // +, -, *, /
		ExprASTPtr m_lhs, m_rhs; // right/left-hand sides of expression
	};

	// function call
	//		::= <identifier>(<args>)
	class CallExprAST : public ExprAST {
	public:
		CallExprAST(const std::string &name, std::vector<ExprASTPtr> args);
		virtual llvm::Value *GenerateCode() override;
		virtual void Dump(int depth) const override;

	private:
		std::string m_calleeName;
		std::vector<ExprASTPtr> m_args;
	};

	//  if
	//		::= if (<cond>) <expr>
	//		::= if (<cond>) <expr> else <expr>
	class IfExprAST : public ExprAST {
	public:
		IfExprAST(ExprASTPtr cond, ExprASTPtr el);
		virtual llvm::Value *GenerateCode() override;
		virtual void Dump(int depth) const override;

		inline bool HasElse() { return m_else != nullptr; }

	private:
		ExprASTPtr m_condition, m_else;
	};
	

	// Function prototypes
	class PrototypeAST {
	public:
		PrototypeAST(std::string name, std::vector<std::string> params);

		inline std::string GetName() const { return m_name; }

		llvm::Function *GenerateCode();
		void Dump(int depth) const;

	private:
		std::string m_name;
		std::vector<std::string> m_params;
	};
	using PrototypeASTPtr = std::unique_ptr<PrototypeAST>;

	// Function declarations
	class FunctionAST {
	public:
		FunctionAST(PrototypeASTPtr prototype, ExprASTPtr body);

		llvm::Function * GenerateCode();
		void Dump(int depth) const;

	private:
		PrototypeASTPtr m_prototype;
		ExprASTPtr m_body;
	};
	using FunctionASTPtr = std::unique_ptr<FunctionAST>;

	class TranslationUnitAST {
	public:
		TranslationUnitAST(const std::string &name, std::vector<PrototypeASTPtr> protos, std::vector<FunctionASTPtr> funcs);
		void GenerateCode();
		void Dump() const;

	private:
		std::string m_name;
		std::vector<FunctionASTPtr> m_functions;
		std::vector<PrototypeASTPtr> m_prototypes;

	};
	using TranslationUnitASTPtr = std::unique_ptr<TranslationUnitAST>;
}