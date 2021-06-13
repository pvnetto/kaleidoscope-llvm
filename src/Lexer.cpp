#include "Lexer.h"

#include <cstdio>
#include <ctype.h>

namespace Lexer {

	struct State {
		int LastChar = ' ';
        std::string Identifier = "";
        double NumberValue = 0;

		std::string::iterator Iter;
	};

	static std::string s_source;

    int ReadNext(State& state) {
        if(state.Iter == s_source.end())
            return '\0';
        return *(state.Iter++);
    }

	int FindIdentifier(State& state) {
		state.Identifier = state.LastChar;

		while (isalnum((state.LastChar = ReadNext(state)))) {
			state.Identifier += state.LastChar;
		}

		if (state.Identifier == "fn")
			return Token_Definition;
		if (state.Identifier == "extern")
			return Token_Extern;
		if (state.Identifier == "return")
			return Token_Return;
		if (state.Identifier == "if")
			return Token_If;
		if (state.Identifier == "else")
			return Token_Else;
		return Token_Identifier;
	}

	int FindNumber(State &state) {
		std::string numberStr = "";
		bool foundDot = false;
		do {
			numberStr += state.LastChar;

			if (state.LastChar == '.' && foundDot) {
				return Token_Unknown; // invalid number formatting. ex: 3.2.333
			}
			foundDot = state.LastChar == '.';
			state.LastChar = ReadNext(state);
		} while (isdigit(state.LastChar) || state.LastChar == '.');

		state.NumberValue = strtod(numberStr.c_str(), 0);

		return Token_Number;
	}

	int GetToken(State& state) {
        state.Identifier = "";

		while (isspace(state.LastChar) || iscr(state.LastChar) || isnewline(state.LastChar)) {
            state.LastChar = ReadNext(state);
		}

        // skips comments
		if (state.LastChar == '#') {
			do {
				state.LastChar = ReadNext(state);
			} while (!isnewline(state.LastChar) && !iscr(state.LastChar));

			return GetToken(state);
		}

        // parses identifiers and keywords
        if(isalpha(state.LastChar))
            return FindIdentifier(state);

        // parses numbers
        if(isdigit(state.LastChar))
            return FindNumber(state);
        
        if(state.LastChar == '\0')
            return Token_EndOfFile;
        
        // returns the value itself
        int currentChar = state.LastChar;
        state.LastChar = ReadNext(state);
		return currentChar;
	}

	static State s_internal;

	void Init(const std::string &source) {
		s_source = source;
		s_internal.Iter = s_source.begin();
	}

	int GetToken() {
		return GetToken(s_internal);
	}

	int PeekToken() {
		State peekState = s_internal;
		return GetToken(peekState);
	}

	const std::string& GetIdentifier() {
		return s_internal.Identifier;
	}

	double GetNumberValue() {
		return s_internal.NumberValue;
	}
}