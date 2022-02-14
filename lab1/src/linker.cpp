#include <iostream>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <fstream>
#include <map>
#include <getopt.h>
// #include <fmt/format.h>

using namespace std;

enum PASS_TYPE {PASS_ONE, PASS_TWO};
enum ADDR_MODE {RELATIVE, EXTERNAL, IMMEDIATE, ABSOLUTE, ERROR};
map<string, int> symbolMap;
string INPUT_FILE = "";

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
};

ostream& operator<<(ostream& os, const Token& token) {
    return os << "Token: " 
              << token.lineNumber << ":" << token.offset
              << " : " << token.value;
}


string getToken()
{
    return "token";
}

int readInt()
{
    return 1;
}
string readSymbol()
{
    return "symbol";
}

pair<ADDR_MODE, string> readIAER()
{
    //read I, A, E or R
    char iaer = 'A';
    string errorString = "";
    switch(iaer)
    {
        case 'I':
          return make_pair(IMMEDIATE, errorString);
        case 'A':
          return make_pair(ABSOLUTE, errorString);
        case 'E':
          return make_pair(EXTERNAL, errorString);
        case 'R':
          return make_pair(RELATIVE, errorString);
        default:
          return make_pair(ERROR, errorString);
    }
}

void ParseInput(PASS_TYPE passNum)
{
    // getToken
}

int main (int argc, char** argv)
{
    int num = 5;
    printf("The answer is %05d.\n", num);

    // Read input file name from the command line arguments
    if (argc < 2 || argv[1] == "")
    {
      printf("Fail: Provide input file name as program argument.");
      exit(EXIT_FAILURE);
    }

    INPUT_FILE = string(argv[1]);

    printf("Input file name: %s\n", INPUT_FILE.c_str());

    ifstream fin;
    fin.open(INPUT_FILE.c_str());
    string line;
    while (fin) {
      // Read a line from input file
      getline(fin, line);

      char line_cstr[line.length() + 1];
      strcpy(line_cstr, line.c_str());
      char* sToken = strtok(line_cstr, " \t\n");
      
      cout << line << endl;
      while (sToken != NULL)
      {
        printf ("'%s'\n",sToken);
        sToken = strtok (NULL, " \t\n");
      }

    }
    // Close input file
    fin.close();

    // Parse for Symbol Map i.e. first pass
    ParseInput(PASS_ONE);

    // Parse second time to update addresses and resolve external references in Memory map
    ParseInput(PASS_TWO);

}
