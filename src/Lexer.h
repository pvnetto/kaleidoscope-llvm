#pragma once

#include <string>

namespace Lexer {

	enum TokenType {
		Token_EndOfFile = -100,

		Token_Definition,
		Token_Extern,
        Token_Return,
		Token_If,
		Token_Else,

		Token_Identifier,
		Token_Number,

        Token_Unknown,
	};


    // ##### Lexer
    void Init(const std::string& source);
	int GetToken();
	int PeekToken();

	const std::string& GetIdentifier();
	double GetNumberValue();


    // ##### Helpers
	inline bool iscr(int c) { return c == '\r'; }
	inline bool isnewline(int c) { return c == '\n'; }
}