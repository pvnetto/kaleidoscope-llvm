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

	class Expr : public Stmt {
	public:
		virtual ~Expr() {}
	};
	using ExprPtr = std::unique_ptr<Expr>;

	//	<literal>
	// 		::= <digit> | <digit> <literal>
	class NumberExpr : public Expr {
	public:
		NumberExpr(double value);
		virtual llvm::Value *GenerateCode() override;
		virtual void Dump(int depth) const override;

	private:
		double m_value;
	};
	using NumberASTPtr = std::unique_ptr<NumberExpr>;

	//  <variable>
	//		::= <id>
	class VariableExpr : public Expr {
	public:
		VariableExpr(const std::string &name);

		std::string GetName() const { return m_name; }

		virtual llvm::Value *GenerateCode() override;
		virtual void Dump(int depth) const override;

	private:
		std::string m_name;
	};
	using VariableExprPtr = std::unique_ptr<VariableExpr>;

	// 	<binary_expr>
	//		::= <number> [<operator> <binary_expr>]
	class BinaryExpr : public Expr {
	public:
		BinaryExpr(char op, ExprPtr lhs, ExprPtr rhs);
		virtual llvm::Value *GenerateCode() override;
		virtual void Dump(int depth) const override;

	private:
		char m_op;
		ExprPtr m_lhs, m_rhs;
	};

	//  <function_call>
	//		::= <identifier>(<args>)
	class CallExpr : public Expr {
	public:
		CallExpr(const std::string &name, std::vector<ExprPtr> args);
		virtual llvm::Value *GenerateCode() override;
		virtual void Dump(int depth) const override;

	private:
		std::string m_calleeName;
		std::vector<ExprPtr> m_args;
	};

	// <stmts>
	//		::= <stmt> [<stmts>]
	class CompoundStmt : public Stmt {
	public:
		CompoundStmt(std::vector<StmtPtr> stmts);

		void AddStmt(StmtPtr stmt) { m_statements.push_back(std::move(stmt)); }

		// Overrides begin/end operators so statements can be iterated by a for-range loop
		std::vector<StmtPtr>::iterator begin() { return m_statements.begin(); }
		std::vector<StmtPtr>::iterator end() { return m_statements.end(); }

		virtual llvm::Value *GenerateCode() override;
		virtual void Dump(int depth) const override;

	private:
		std::vector<StmtPtr> m_statements;
	};
	using CompoundStmtPtr = std::unique_ptr<CompoundStmt>;


	// <assign_stmt>
	//		::= <variable> = [{ <variable> = }] <expr>
	class AssignStmt : public Stmt {
	public:
		AssignStmt(std::vector<VariableExprPtr> lhs, ExprPtr rhs);

		virtual llvm::Value *GenerateCode() override;
		virtual void Dump(int depth) const override;

	private:
		std::vector<VariableExprPtr> m_lhs;
		ExprPtr m_rhs;
	};
	using AssignStmtPtr = std::unique_ptr<AssignStmt>;

	// <return>
	//		::= 'return' <expr>
	class ReturnStmt : public Stmt {
	public:
		ReturnStmt(ExprPtr returnExpr);

		virtual llvm::Value *GenerateCode() override;
		virtual void Dump(int depth) const override;

	private:
		ExprPtr m_returnExpr;
	};
	using ReturnStmtPtr = std::unique_ptr<ReturnStmt>;

	//  <if>
	//		::= 'if' (<cond>) <expr> ['else if' <expr>] ['else' <expr>]
	class IfStmt : public Stmt {
	public:
		IfStmt(ExprPtr cond, CompoundStmtPtr body, CompoundStmtPtr elseStmt=nullptr);

		virtual llvm::Value *GenerateCode() override;
		virtual void Dump(int depth) const override;

		// Generates code for sequential else if/else statements
		llvm::Value *GenerateCodeSequence(llvm::BasicBlock* exit);


		inline void SetElse(StmtPtr elseStmt) { m_else = std::move(elseStmt); } 

	private:
		ExprPtr m_condition;
		CompoundStmtPtr m_body;
		StmtPtr m_else; // 'else if' is just a special case of 'else'
	};
	using IfStmtPtr = std::unique_ptr<IfStmt>;

	//  <for>
	//		::= for(<expr>;<cond>;<number>) <stmts>
	class ForStmt : public Stmt {
	public:
		ForStmt(const std::string& loopVarName, ExprPtr value, ExprPtr cond, ExprPtr step, CompoundStmtPtr body);

		virtual llvm::Value *GenerateCode() override;
		virtual void Dump(int depth) const override;

	private:
		std::string m_loopVarName;
		ExprPtr m_value, m_condition, m_step;
		CompoundStmtPtr m_body;
	};
	using ForStmtPtr = std::unique_ptr<ForStmt>;


	// prototypes
	//		::= fn <id>(<args>)
	class PrototypeDecl {
	public:
		PrototypeDecl(std::string name, std::vector<std::string> params);

		inline std::string GetName() const { return m_name; }

		llvm::Function *GenerateCode();
		void Dump(int depth) const;

	private:
		std::string m_name;
		std::vector<std::string> m_params;
	};
	using PrototypeASTPtr = std::unique_ptr<PrototypeDecl>;

	// declarations
	//		::= <prototype> <stmts>
	class FunctionDecl {
	public:
		FunctionDecl(PrototypeASTPtr prototype, CompoundStmtPtr body);

		llvm::Function *GenerateCode();
		void Dump(int depth) const;

	private:
		PrototypeASTPtr m_prototype;
		CompoundStmtPtr m_body;
	};
	using FunctionDeclPtr = std::unique_ptr<FunctionDecl>;

	class TranslationUnitDecl {
	public:
		TranslationUnitDecl(const std::string &name, std::vector<PrototypeASTPtr> protos, std::vector<FunctionDeclPtr> funcs);
		void GenerateCode();
		void Dump() const;

	private:
		std::string m_name;
		std::vector<FunctionDeclPtr> m_functions;
		std::vector<PrototypeASTPtr> m_prototypes;
	};
	using TranslationUnitASTPtr = std::unique_ptr<TranslationUnitDecl>;
}