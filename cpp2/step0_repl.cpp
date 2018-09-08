#include <iostream>
#include <string>

using namespace std;

string READ(const string& input);
string EVAL(const string& ast);
string PRINT(const string& ast);
string rep(const string& input);
bool readline(const string& prompt, string& line);

int main(int argc, char* argv[]) {
	string input;
	while (readline("user> ", input)) {
		cout << rep(input) << "\n";
	}
	return 0;
}

string rep(const string& input) {
	return PRINT(EVAL(READ(input)));
}

string READ(const string& input) {
	return input;
}

string EVAL(const string& ast) {
	return ast;
}

string PRINT(const string& ast) {
	return ast;
}

bool readline(const string& prompt, string& line) {
	cout << prompt;
	getline(cin, line);
	return (line.size() > 0);
}
