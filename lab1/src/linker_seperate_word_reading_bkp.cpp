#include <iostream>
#include <stdlib.h> 
#include <string>
#include <string.h>
#include <fstream>
#include <map>
#include <vector>
// #include <getopt.h>
// #include <fmt/format.h>
#include <unistd.h>

using namespace std;

enum PASS_TYPE {PASS_ONE, PASS_TWO};
enum ADDR_MODE {RELATIVE, EXTERNAL, IMMEDIATE, ABSOLUTE, ERROR};

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

map<string, int> symbolMap;
// string INPUT_FILE = "";
ifstream fin;
string line;
char* lineCharArray = NULL;
char* tokenString = NULL;
int lineNumber = 0;
int offset = 0;
int moduleNumber = 0;
char DELIMS[3] = {' ', '\t', '\n'};
vector<Token> tokens;
vector<Token>::iterator token_itr;
// char* fileArrayPtr = NULL;



// Read next line from the input file into a char array: lineCharArray
void ReadNextLineToCharArray()
{
  if (fin.peek() != EOF)
  {
    // line = "";
    cout<<"YESSS!!!"<<endl;
    getline(fin, line);
    //To-do: check empty line, use trim, maybe handled in getToken
    lineNumber++;
    offset = 0;
    char tempArray[line.length() + 1];
    lineCharArray = tempArray;
    strcpy(lineCharArray, line.c_str());
    cout<<"Str read :"<<lineCharArray<<endl;
  } else {
    exit(1);
  }
}

void CreateTokens()
{
    string line;
    while (fin) {
      lineNumber++;
      // Read a line from input file
      getline(fin, line);

      char line_cstr[line.length() + 1];
      strcpy(line_cstr, line.c_str());
      // PrintCharArray(line_cstr);
      char* tokenString = strtok(line_cstr, DELIMS);
      // char* tokenString = GetToken1(line_cstr, DELIMS);
      
      // cout << "LOG: line: " << line << endl;
      while (tokenString != NULL)
      {
        // printf ("'%s'\n",tokenString);
        offset = line.find(tokenString);
        Token t1 = Token(tokenString, lineNumber, offset+1);
        tokens.push_back(t1);
        // PrintCharArray(line_cstr);
        tokenString = strtok (NULL, DELIMS);
        // tokenString = GetToken2(DELIMS);
        
        // cout << t1 << endl;
      }
    }
    token_itr = tokens.begin();
}

// Get next token from last read line
Token GetToken()
{
  if (token_itr != tokens.end())
  {
    Token t1 = *token_itr;
    token_itr++;
    tokens.erase(tokens.begin());
    return t1;
  }
  else
  {
    return Token();
  }
  /* cout<<"TEST"<<endl;
  if(fin.peek() != EOF)
  {
    if(lineNumber == 0)
    {
      cout<<"Test1"<<endl;
      ReadNextLineToCharArray();
      cout<<"lineC :"<<lineCharArray<<endl;
      //string t = "1 x c";
      char *t = lineCharArray;//1 x c";
      tokenString = strtok(t, " ");
      cout<<"Test2 :["<<tokenString<<"]"<<endl;
    }
    else
    {
      tokenString = strtok(NULL, DELIMS);
    }

    if (tokenString == NULL)
    {
      while(tokenString == NULL)
      {
        ReadNextLineToCharArray();
        //cout<<"Test3"<<endl;
        tokenString = strtok(lineCharArray, DELIMS);
        
      }
    }

    offset = line.find(tokenString);

    return Token(tokenString, lineNumber, offset+1);
    // return Token(tokenString, lineNumber, 0);
  }
  else
  {
    return Token();
  } */
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
    CreateTokens();
    Token t1 = GetToken();
    while(!t1.IsEmpty())
    {
      cout << "TOKEN READ " << t1 << endl;
      t1 = GetToken();
    }
    /* while (fin)
    {
      Token t1 = GetToken();
      cout << "TOKEN READ " << t1 << endl;
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
    } */


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
