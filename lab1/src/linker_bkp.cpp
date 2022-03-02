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
ifstream fin;
string line;
char* fileArrayPtr = NULL;

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


char* ReadFileToArray()
{
  fin.open(INPUT_FILE.c_str());
  // while (fin) 
  // {
  //     // Read a line from input file
  //     getline(fin, line);
  //     cout << line << endl;

  // }
  // t.open("file.txt");      // open input file
  fin.seekg(0, std::ios::end);    // go to the end
  int length = fin.tellg();           // report location (this is the length)
  fin.seekg(0, std::ios::beg);    // go back to the beginning
  char fileArray [length];    // allocate memory for a buffer of appropriate dimension
  fin.read(fileArray, length);       // read the whole file into the buffer
  fin.close();

  // char* tokenString = strtok(fileArray, " \t\n");
      
  // while (tokenString != NULL)
  // {
  //   printf ("'%s'\n",tokenString);
  //   tokenString = strtok (NULL, " \t\n");
  // }

  return fileArray;

}

Token getToken(bool isFirst = false)
{
    
    // if (fin) {
    //   // Read a line from input file
    //   getline(fin, line);

    //   char line_cstr[line.length() + 1];
    //   strcpy(line_cstr, line.c_str());
    //   char* tokenString = strtok(line_cstr, " \t\n");
      
    //   cout << line << endl;
    //   while (tokenString != NULL)
    //   {
    //     printf ("'%s'\n",tokenString);
    //     tokenString = strtok (NULL, " \t\n");
    //   }
    // }

    if(!isFirst)
    {
      
    }


    
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
    

    while(fin)
    {
      Token t1 = getToken();
    }

    // Close input file
    fin.close();
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

    fileArrayPtr = ReadFileToArray();

    // Parse for Symbol Map i.e. first pass
    ParseInput(PASS_ONE);

    // Parse second time to update addresses and resolve external references in Memory map
    ParseInput(PASS_TWO);

}
