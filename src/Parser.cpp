#include "Parser.h"

#include "Lexer.h"

#include <unordered_map>

// #### Forward declarations
namespace Parser {
	int GetTokenPrecedence(int token);
}

namespace Parser {

#define EXPECT_TOKEN(c, t)                                               \
	if (s_state.CurrentToken != c) return LogErrorT<t>("Expected " + c); \
	NextToken();

#define EXPECT_TOKEN_ID(t)                                         \
	if (s_state.CurrentToken != Lexer::Token_Identifier) return LogErrorT<t>("Expected identifier"); \
	NextToken();

// Checks current token but doesn't move to next
#define CHECK_TOKEN(c, t) \
	if (s_state.CurrentToken != c) return LogErrorT<t>("Expected " + c); \

#define CHECK_TOKEN_ID(t) \
	if (s_state.CurrentToken != Lexer::Token_Identifier) return LogErrorT<t>("Expected identifier"); \

	struct State {
		int CurrentToken;
	};

	static State s_state;

	int NextToken() {
		s_state.CurrentToken = Lexer::GetToken();
		return s_state.CurrentToken;
	}

	TranslationUnitASTPtr GenerateAST() {
		fprintf(stderr, ">> INFO: Generating AST:\n");

		NextToken();

		std::vector<PrototypeASTPtr> m_prototypes;
		std::vector<FunctionASTPtr> m_functions;
		while (true) {
			switch (s_state.CurrentToken) {
				case ';': // skips
					NextToken();
					break;
				case Lexer::Token_EndOfFile: {
					std::unique_ptr<TranslationUnitDecl> unit =
					    std::make_unique<TranslationUnitDecl>("main", std::move(m_prototypes), std::move(m_functions));
					unit->Dump();
					return std::move(unit);
				}
				case Lexer::Token_Extern:
					if (auto externExpr = ParseExtern()) {
						m_prototypes.push_back(std::move(externExpr));
						break;
					}
					return nullptr;
				case Lexer::Token_Definition:
					if (auto definitionExpr = ParseDefinition()) {
						m_functions.push_back(std::move(definitionExpr));
						break;
					}
					return nullptr;
				default:
					if (auto topLevelExpr = ParseTopLevelExpr()) {
						m_functions.push_back(std::move(topLevelExpr));
						break;
					}
					return nullptr;
			}
		}

		return nullptr;
	}

	// Assumes it's only called when current token is a number
	NumberASTPtr ParseNumberExpr() {
		auto result = std::make_unique<NumberExpr>(Lexer::GetNumberValue());
		NextToken();
		return std::move(result);
	}

	// variable ::= <identifier>
	// function call ::= <identifier>()
	ExprPtr ParseIdentifierExpr() {
		std::string identifier = Lexer::GetIdentifier();
		NextToken();

		// function call
		if (s_state.CurrentToken == '(') {
			NextToken();

			std::vector<ExprPtr> args;
			while (s_state.CurrentToken != ')' && s_state.CurrentToken != ',') {
				if (auto arg = ParseExpr()) {
					args.push_back(std::move(arg));
				} else {
					return LogError("Expected function arguments");
				}

				if (s_state.CurrentToken == ')')
					break;
				if (s_state.CurrentToken != ',')
					return LogError("Expected ')' or ','");

				NextToken();
			}

			if (s_state.CurrentToken == ')') {
				NextToken();
				return std::make_unique<CallExpr>(identifier, std::move(args));
			}

			return LogError("Expected ')'");
		}
		// variable
		else {
			return std::make_unique<VariableExpr>(identifier);
		}
	}

	ExprPtr ParsePrimary() {
		switch (s_state.CurrentToken) {
			case Lexer::Token_Number:
				return ParseNumberExpr();
			case Lexer::Token_Identifier:
				return ParseIdentifierExpr();
			case '(':
				return ParseParenthesisExpr();
			// Just ignore some symbols instead of issuing an error msg
			case '}':
			case ')':
			case ';':
			case Lexer::Token_EndOfFile:
				return nullptr;
			default:
				return LogError("Unknown expression format");
		}
	}

	ExprPtr ParseExpr() {
		if (auto lhs = ParsePrimary())
			return ParseBinOpRHS(0, std::move(lhs));
		return nullptr;
	}

	// Based on pseudocode from: https://en.wikipedia.org/wiki/Operator-precedence_parser
	ExprPtr ParseBinOpRHS(int minPrecedence, ExprPtr lhs) {
		int lookahead = s_state.CurrentToken;
		while (GetTokenPrecedence(lookahead) >= minPrecedence) {
			int op = lookahead;
			NextToken();

			auto rhs = ParsePrimary();

			lookahead = s_state.CurrentToken;
			int opPrecedence = GetTokenPrecedence(op);
			while (GetTokenPrecedence(lookahead) > opPrecedence) {
				rhs = ParseBinOpRHS(opPrecedence + 1, std::move(rhs));
				lookahead = Lexer::PeekToken();
			}

			lhs = std::make_unique<BinaryExpr>(op, std::move(lhs), std::move(rhs));
		}

		return std::move(lhs);
	}

	// Builds expressions between parenthesis. Note that the parenthesis are
	// never added to the AST, they simply provide grouping.
	ExprPtr ParseParenthesisExpr() {
		NextToken(); // consumes (
		if (auto v = ParseExpr()) {
			if (s_state.CurrentToken == ')') {
				NextToken(); // consumes )
				return v;
			}
			return LogError("Expected ')'");
		}

		return nullptr;
	}

	// Top-level expressions are represented as anonymous functions
	FunctionASTPtr ParseTopLevelExpr() {
		if (auto compoundStmt = ParseStmts()) {
			auto anonProto = std::make_unique<PrototypeDecl>(ANON_EXPR_NAME, std::vector<std::string>());
			return std::make_unique<FunctionDecl>(std::move(anonProto), std::move(compoundStmt));
		}

		return nullptr;
	}

	StmtPtr ParseStmt() {
		switch (s_state.CurrentToken) {
			case Lexer::Token_Return:
				return ParseReturnStmt();
			case Lexer::Token_If:
				return ParseIfStmt();
			case Lexer::Token_For:
				return ParseForStmt();
			default:
				return ExpectSemicolon(ParseExpr);
		}
	}

	CompoundStmtPtr ParseStmts() {
		std::vector<StmtPtr> statements;
		while (auto expr = ParseStmt()) {
			statements.push_back(std::move(expr));
		}

		return std::make_unique<CompoundStmt>(std::move(statements));
	}

	IfStmtPtr ParseIfStmtSingle() {
		NextToken(); // consumes if

		if (auto cond = ParseExpr()) {
			if (auto compoundStmt = ExpectSurrounded('{', ParseStmts, '}')) {
				return std::make_unique<IfStmt>(std::move(cond), std::move(compoundStmt));
			}
		}

		return nullptr;
	}

	// Parses if/if else/else statements. 'Else if' statements are a special
	// case of 'else' where the entire block is surrounded by an if statement.
	IfStmtPtr ParseIfStmt() {
		if (IfStmtPtr parentIf = ParseIfStmtSingle()) {

			IfStmt *lastIf = parentIf.get();
			while (s_state.CurrentToken == Lexer::Token_Else) {
				NextToken();

				// appends else statement to the last parsed if
				if (s_state.CurrentToken == Lexer::Token_If) {
					if (auto elseifStmt = ParseIfStmtSingle()) {
						IfStmt *elseIfPtr = elseifStmt.get();
						lastIf->SetElse(std::move(elseifStmt));
						lastIf = elseIfPtr;
						continue;	// checks for more else if/else statements
					}
				}
				if (auto elseStmt = ExpectSurrounded('{', ParseStmts, '}')) {
					lastIf->SetElse(std::move(elseStmt));
					break;			// there should be no more else if/else
				}

				return nullptr;
			}

			return std::move(parentIf);
		}

		return nullptr;
	}

	ReturnStmtPtr ParseReturnStmt() {
		NextToken();

		if (auto returnExpr = ExpectSemicolon(ParseExpr)) {
			return std::make_unique<ReturnStmt>(std::move(returnExpr));
		}

		return nullptr;
	}

	ForStmtPtr ParseForStmt() {
		NextToken();

		EXPECT_TOKEN('(', ForStmt);
		CHECK_TOKEN_ID(ForStmt);
		std::string loopVarId = Lexer::GetIdentifier();
		NextToken();
		EXPECT_TOKEN('=', ForStmt);

		ExprPtr valueExpr, condExpr, stepExpr;
		if ((valueExpr = ExpectSemicolon(ParseExpr)),
			(condExpr = ExpectSemicolon(ParseExpr)),
		    (stepExpr = ParseExpr())) {

			EXPECT_TOKEN(')', ForStmt);
			
			if (auto forBody = ExpectSurrounded('{', ParseStmts, '}')) {
				return std::make_unique<ForStmt>(loopVarId, std::move(valueExpr), std::move(condExpr), std::move(stepExpr), std::move(forBody));
			}
		}

		return nullptr;
	}

	PrototypeASTPtr ParseExtern() {
		NextToken();

		if (auto proto = ExpectSemicolon(ParsePrototype)) {
			return proto;
		}
		return nullptr;
	}

	// prototype ::= <identifier>(<args>)
	// args ::= <id>, ...
	PrototypeASTPtr ParsePrototype() {
		if (s_state.CurrentToken != Lexer::Token_Identifier)
			return LogErrorT<PrototypeDecl>("Expected function identifier");

		// Parses prototype identifier
		std::string funcIdentifier = Lexer::GetIdentifier();
		NextToken();

		// Parses prototype parameter list
		std::vector<std::string> params;
		while (NextToken() != ')') {
			CHECK_TOKEN_ID(PrototypeDecl);

			params.push_back(Lexer::GetIdentifier());
			NextToken();

			if (s_state.CurrentToken == ')') {
				break;
			}

			CHECK_TOKEN(',', PrototypeDecl);
		}
		EXPECT_TOKEN(')', PrototypeDecl);

		return std::make_unique<PrototypeDecl>(funcIdentifier, std::move(params));
	}

	FunctionASTPtr ParseDefinition() {
		NextToken();
		if (auto prototype = ParsePrototype()) {
			if (s_state.CurrentToken != '{')
				return LogErrorT<FunctionDecl>("Expected {");
			NextToken();

			if (auto compoundStmt = ParseStmts()) {
				if (s_state.CurrentToken != '}')
					return LogErrorT<FunctionDecl>("Expected {");
				NextToken();

				return std::make_unique<FunctionDecl>(std::move(prototype), std::move(compoundStmt));
			}
		}
		return nullptr;
	}

	ExprPtr LogError(const char *msg) {
		fprintf(stderr, ">> ERROR: %s\n", msg);
		return nullptr;
	}

}

// #### Operator-precedence parsing helpers
namespace Parser {
	static std::unordered_map<int, int> s_precedenceTable{
	    {'<', 10}, { '>', 10 }, {'+', 20}, {'-', 30}, {'*', 40}, {'/', 50}};

	int GetTokenPrecedence(int token) {
		if (!__isascii(token) || s_precedenceTable[token] <= 0)
			return -1;

		return s_precedenceTable[token];
	}
}