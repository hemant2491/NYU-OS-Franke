#include <iostream>
#include <stdlib.h> 
#include <string>
#include <string.h>
#include <fstream>

using namespace std;

ifstream fin;
char DELIMS[3] = {' ', '\t', '\n'};
int lineNumber = 0;
int offset = -1;

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
    empty = false;
  }
  Token()
  {
    value = "";
    lineNumber = 0;
    offset = 0;
    empty = true;
  }

  bool IsEmpty(){ return empty;}

  private:
    bool empty = true;
};

ostream& operator<<(ostream& os, const Token& token) {
    return os << "Token: " 
              << token.lineNumber << ":" << token.offset
              << " : " << token.value;
}

void PrintCharArray(char* char_arr)
{
    cout << "char array: ";
    for (int i = 0; i< strlen(char_arr); i++)
    {
    cout << char_arr[i];
    }
    cout << endl;

}

char* GetToken1(char* char_arr, char* delims)
{
    char* temp_arr = strtok(char_arr, delims);
    return temp_arr;
}

char* GetToken2(char* delims)
{
    char* temp_arr = strtok(NULL, delims);
    return temp_arr;
}

void DoStuff()
{
    string line;
    while (fin) {
      lineNumber++;
      offset=-1;
      // Read a line from input file
      getline(fin, line);

      char line_cstr[line.length() + 1];
      strcpy(line_cstr, line.c_str());
      // PrintCharArray(line_cstr);
      // char* tokenString = strtok(line_cstr, DELIMS);
      char* tokenString = GetToken1(line_cstr, DELIMS);
      
      cout << "line: " << line << endl;
      while (tokenString != NULL)
      {
        // printf ("'%s'\n",tokenString);
        offset = line.find(tokenString, offset+1);
        Token t1 = Token(tokenString, lineNumber, offset+1);
        // PrintCharArray(line_cstr);
        // tokenString = strtok (NULL, DELIMS);
        tokenString = GetToken2(DELIMS);
        cout << t1 << endl;
      }
    }
}

int main (int argc, char** argv)
{
    fin.open(argv[1]);
    DoStuff();
}

