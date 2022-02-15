#include <iostream>
#include <stdlib.h> 
#include <string>
#include <string.h>
#include <fstream>
#include <map>
// #include <getopt.h>
// #include <fmt/format.h>
#include <unistd.h>

using namespace std;

enum PASS_TYPE {PASS_ONE, PASS_TWO};
enum ADDR_MODE {RELATIVE, EXTERNAL, IMMEDIATE, ABSOLUTE, ERROR};
map<string, int> symbolMap;
// string INPUT_FILE = "";
ifstream fin;
string line = "";
char* lineCharArray = NULL;
char* tokenString = NULL;
int lineNumber = 0;
int offset = 0;
int moduleNumber = 0;
// char* fileArrayPtr = NULL;

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

// Read next line from the input file into a char array: lineCharArray
void ReadNextLineToCharArray()
{
  if (fin.peek() != EOF)
  {
    line = "";
    getline(fin, line);
    //To-do: check empty line, use trim, maybe handled in getToken
    lineNumber++;
    offset = 0;
    char tempArray[line.length() + 1];
    lineCharArray = tempArray;
    strcpy(lineCharArray, line.c_str());
  }
}

// Get next token from last read line
Token GetToken()
{
  if(fin.peek() != EOF)
  {
    if(lineNumber == 0)
    {
      ReadNextLineToCharArray();
      tokenString = strtok(lineCharArray, " \t\n");
    }
    else
    {
      tokenString = strtok(NULL, " \t\n");
    }

    if (tokenString == NULL)
    {
      while(tokenString == NULL)
      {
        ReadNextLineToCharArray();
        tokenString = strtok (lineCharArray, " \t\n");
      }
    }

    offset = line.find(tokenString);

    return Token(tokenString, lineNumber, offset);
  }
  else
  {
    return Token();
  }
}

int ReadInt()
{
  Token intToken = GetToken();
  cout << intToken << endl;
  return stoi(intToken.value);
}

string ReadSymbol()
{
  Token symbolToken = GetToken();
  return symbolToken.value;
}

// pair<ADDR_MODE, string> ReadIARE()
char ReadIARE()
{
    //read I, A, E or R
    char iare = 'A';
    string errorString = "";
    // To-do: check length of token value
    Token IAREToken = GetToken();
    iare = IAREToken.value.front();
    return iare;
    /* switch(iare)
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
          errorString = "Unknown type detected: " + iare;
          return make_pair(ERROR, errorString);
    } */
}

void ParseInput(PASS_TYPE passNum, char* inputFile)
{
    
    cout << "LOG: Parse Input" << endl;
    if (passNum == PASS_ONE)
    {
      printf("Symbol Table\n");
    }
    else
    {
      printf("Memory Map\n");
    }

    /* try
    {
      cout << "LOG: opening file for reading: " << inputFile << endl;
      fin.open(inputFile);
      cout << "LOG: input file opened for reading." << endl;
    }
    catch(std::exception const& e)
    {
      if(passNum == PASS_ONE)
      {
        cout << "There was an error in opening input file " << inputFile << ": " << e.what() << endl;
        return;
      }
      else
      {
        exit(EXIT_FAILURE);
      }
    } */

    cout << "LOG: Beginning pass " << passNum << endl;
    while (fin.peek() != EOF)
    {
      // createModule();
      cout << "LOG: Reading defintion count";
      // sleep(10);
      int defcount = ReadInt();
      cout << "LOG: Reading definition list for " << defcount << " symbols." << endl;
      for (int i=0;i<defcount;i++)
      {
        // Symbol sym = ReadSymbol();
        string sym = ReadSymbol();
        int val = ReadInt();
        cout << "LOG: Read symbol " << sym << " and value " << val << endl;
        // createSymbol(sym,val);
      }

      int usecount = ReadInt();
      cout << "LOG: Reading uselist for " << usecount << " symbols."  << endl;
      for (int i=0;i<usecount;i++)
      {
        // Symbol sym = ReadSymbol();
        string sym = ReadSymbol();
        cout << "LOG: Read symbol " << sym << endl;
        //we don’t do anything here <- this would change in pass 2 }
      }

      int instcount = ReadInt();
      cout << "LOG: Reading " << instcount << " instructions." << endl;
      for (int i=0;i<instcount;i++) {
        char addressmode = ReadIARE();
        int  operand = ReadInt();
        cout << "LOG: Instruction addr type " << addressmode << " operand " << operand << endl;
                  // various checks
                  //  - “ -
      }
    }


    // Close input file
    fin.close();
    if (passNum == PASS_ONE)
    {
      printf("\n");
    }
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

    // INPUT_FILE = string(argv[1]);

    // printf("Input file name: %s\n", INPUT_FILE.c_str());
    printf("Input file name: %s\n", argv[1]);

    // fileArrayPtr = ReadFileToArray();

    fin.open(argv[1]);
    // Parse for Symbol Map i.e. first pass
    ParseInput(PASS_ONE, argv[1]);

    // Parse second time to update addresses and resolve external references in Memory map
    // ParseInput(PASS_TWO, argv[1]);

}
