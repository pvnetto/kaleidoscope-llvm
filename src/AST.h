#pragma once

#include <memory>
#include <string>
#include <vector>

#include <llvm/IR/Value.h>

#define ANON_EXPR_NAME "__anon_expr"

namespace Parser {

	class Stmt {
	public:
		virtual ~Stmt() = default;

		virtual llvm::Value *GenerateCode() = 0;
		virtual void Dump(int depth) const = 0;
	};
	using StmtPtr = std::unique_ptr<Stmt>;


	// Generic AST node for expressions. All expressions are also statements.
	class Expr : public Stmt {
	public:
		virtual ~Expr() {}
	};
	using ExprPtr = std::unique_ptr<Expr>;


	// Numeric literals
	class NumberExprAST : public Expr {
	public:
		NumberExprAST(double value);
		virtual llvm::Value *GenerateCode() override;
		virtual void Dump(int depth) const override;

	private:
		double m_value;
	};
	using NumberASTPtr = std::unique_ptr<NumberExprAST>;

	// variable
	//		::= <id>
	class VariableExprAST : public Expr {
	public:
		VariableExprAST(const std::string &name);
		virtual llvm::Value *GenerateCode() override;
		virtual void Dump(int depth) const override;

	private:
		std::string m_name;
	};

	// binary expressions (i.e 1 + 2)
	//		::= <lhs_expr> <operator> <rhs_expr>
	class BinaryExprAST : public Expr {
	public:
		BinaryExprAST(char op, ExprPtr lhs, ExprPtr rhs);
		virtual llvm::Value *GenerateCode() override;
		virtual void Dump(int depth) const override;

	private:
		char m_op;               // +, -, *, /
		ExprPtr m_lhs, m_rhs; // right/left-hand sides of expression
	};

	// function call
	//		::= <identifier>(<args>)
	class CallExprAST : public Expr {
	public:
		CallExprAST(const std::string &name, std::vector<ExprPtr> args);
		virtual llvm::Value *GenerateCode() override;
		virtual void Dump(int depth) const override;

	private:
		std::string m_calleeName;
		std::vector<ExprPtr> m_args;
	};

	//// compound statement
	////		::= <stmt>, <stmt>, ...
	//class CompoundStmt : public Stmt {
	//public:
	//	CompoundStmt() = default;
	//	CompoundStmt(std::vector<StmtPtr> stmts);
	//	virtual llvm::Value *GenerateCode() override;
	//	virtual void Dump(int depth) const override;

	//private:
	//	std::vector<StmtPtr> m_statements;
	//};
	//using CompoundStmtPtr = std::unique_ptr<CompoundStmt>;


	////  if
	////		::= if (<cond>) <expr>
	////		::= if (<cond>) <expr> else <expr>
	//class IfStmt : public Stmt {
	//public:
	//	IfStmt(ExprPtr cond, CompoundStmtPtr elseStmt);

	//	inline bool HasElseStmt() { return m_else != nullptr; }

	//private:
	//	ExprPtr m_condition;
	//	CompoundStmtPtr m_else;		// 'else if' is just a special case of 'else'
	//};
	

	// prototypes
	//		::= fn <id>(<args>)
	class PrototypeDeclAST {
	public:
		PrototypeDeclAST(std::string name, std::vector<std::string> params);

		inline std::string GetName() const { return m_name; }

		llvm::Function *GenerateCode();
		void Dump(int depth) const;

	private:
		std::string m_name;
		std::vector<std::string> m_params;
	};
	using PrototypeASTPtr = std::unique_ptr<PrototypeDeclAST>;


	// declarations
	//		::= <prototype> { <stmt_list> }
	class FunctionDeclAST {
	public:
		FunctionDeclAST(PrototypeASTPtr prototype, ExprPtr body);

		llvm::Function * GenerateCode();
		void Dump(int depth) const;

	private:
		PrototypeASTPtr m_prototype;
		ExprPtr m_body;
	};
	using FunctionASTPtr = std::unique_ptr<FunctionDeclAST>;

	class TranslationUnitDeclAST {
	public:
		TranslationUnitDeclAST(const std::string &name, std::vector<PrototypeASTPtr> protos, std::vector<FunctionASTPtr> funcs);
		void GenerateCode();
		void Dump() const;

	private:
		std::string m_name;
		std::vector<FunctionASTPtr> m_functions;
		std::vector<PrototypeASTPtr> m_prototypes;

	};
	using TranslationUnitASTPtr = std::unique_ptr<TranslationUnitDeclAST>;
}