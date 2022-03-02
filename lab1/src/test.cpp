#include <iostream>

using namespace std;

struct Token
{
    string value;
    int lineNumber;
    int offset;
    
    Token(string _value, int _lineNum, int _offset)
    {
        value = _value;
        lineNumber = _lineNum;
        offset = _offset;
    }
    
    ostream& operator<<(ostream& os, const Token& token) {
        return os << "Token: " << token.lineNumber << ":" << token.offset
              << " : " << token.value << endl;
    }
};

int main()
{
    cout<<"Hello World";
    
    /* Token t1("xy", 1, 3);
    cout << "Token: " << t1.lineNumber << ":" << t1.offset
              << " : " << t1.value << endl; */
    
    if (__cplusplus == 201703L) std::cout << "C++17\n";
    else if (__cplusplus == 201402L) std::cout << "C++14\n";
    else if (__cplusplus == 201103L) std::cout << "C++11\n";
    else if (__cplusplus == 199711L) std::cout << "C++98\n";
    else std::cout << "pre-standard C++\n";
    
    return 0;
}

