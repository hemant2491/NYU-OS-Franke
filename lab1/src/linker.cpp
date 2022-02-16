#include <iostream>
#include <stdlib.h> 
#include <string>
#include <string.h>
#include <fstream>
#include <map>
#include <vector>
#include <sstream>
#include <iomanip>
// #include <getopt.h>
// #include <fmt/format.h>
// #include <unistd.h>

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


// string INPUT_FILE = "";
ifstream fin;
string line;
char* lineCharArray = NULL;
char* tokenString = NULL;
int lineNumber = 0;
int offset = -1;
int moduleNumber = 0;
char DELIMS[3] = {' ', '\t', '\n'};
vector<Token> tokens;
vector<Token>::iterator tokenItr;
vector<pair<string, int>> symbols;
vector<int> moduleLengths;
vector<string> instructions;


// Method to print char array
void PrintCharArray(char* char_arr)
{
    cout << "char array: ";
    for (int i = 0; i< strlen(char_arr); i++)
    {
    cout << char_arr[i];
    }
    cout << endl;
}

// Read input file and create Tokens
void CreateTokens(PASS_TYPE passNum, char* inputFile)
{
  tokens.clear();
  try
  {
    // cout << "DEBUG LOG: opening file for reading: " << inputFile << endl;
    fin.open(inputFile);
    // cout << "DEBUG LOG: input file opened for reading." << endl;
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
      exit(1);
    }
  }

  // cout << "DEBUG LOG: Creating tokens" << endl;
  string line;
  while (fin) {
    lineNumber++;
    offset = -1;
    // Read a line from input file
    getline(fin, line);

    char line_cstr[line.length() + 1];
    strcpy(line_cstr, line.c_str());
    // PrintCharArray(line_cstr);
    char* tokenString = strtok(line_cstr, DELIMS);
    
    // // cout << "DEBUG LOG: line: " << line << endl;
    while (tokenString != NULL)
    {
      // printf ("'%s'\n",tokenString);
      offset = line.find(tokenString, offset+1);
      Token t1 = Token(tokenString, lineNumber, offset+1);
      tokens.push_back(t1);
      // PrintCharArray(line_cstr);
      tokenString = strtok (NULL, DELIMS);      
      // // cout << "DEBUG LOG: " << t1 << endl;
    }
  }
  // Close input file
  fin.close();
  tokenItr = tokens.begin();
}

// Get next token
Token GetToken()
{
  if (tokenItr != tokens.end())
  {
    Token t1 = *tokenItr;
    tokenItr++;
    // tokens.erase(tokens.begin());
    return t1;
  }
  else
  {
    return Token();
  }
}

int ReadInt()
{
  Token intToken = GetToken();
  // cout << "DEBUG LOG: " << intToken << endl;
  return stoi(intToken.value);
}

string ReadSymbol()
{
  Token symbolToken = GetToken();
  // cout << "DEBUG LOG: " << symbolToken << endl;
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
    // cout << "DEBUG LOG: " << IAREToken << endl;
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

void ReadInstructions(PASS_TYPE passNum, int moduleBase, int instcount, int usecount, vector<string> moduleUseList)
{
  /* cout << "DEBUG LOG: pass " << passNum << " module base " << moduleBase
       << " inst count " << instcount << " use count " << usecount << endl;
  cout << " DEBUG LOG: Use List - {";
  for (string sym : moduleUseList)
  {
    cout << " " << sym; 
  }
  cout << "}" << endl; */
  for (int i=0; i<instcount; i++)
  {
    char addressmode = ReadIARE();
    int  instr = ReadInt();
    if (passNum == PASS_TWO)
    {
      string errorString;
      int opcode = instr/1000;
      int operand = instr%1000;
      // cout << "DEBUG LOG: Instruction addr type " << addressmode << " opcode " << opcode << " operand " << operand << endl;
                // various checks
                //  - “ -
      switch(addressmode)
      {
        case 'I':
          // cout << "DEBUG LOG: case I" << endl;
          if(instr>9999)
          {
            errorString = " Error: Illegal opcode; treated as 9999";
            operand = 999;
            opcode = 9;
          }
          break;
        case 'A':
          // cout << "DEBUG LOG: case B" << endl;
          if (operand>511)
          {
            errorString = " Error: Absolute address exceeds machine size; zero used";
            operand = 0;
          }
          break;
        case 'E':
          // cout << "DEBUG LOG: case E" << endl;
          if (operand>usecount-1)
          {
            errorString = " Error: External address exceeds length of uselist; treated as immediate";
          }
          else
          {
            string usedSym = moduleUseList[operand];
            for (pair<string, int> p : symbols)
            {
              if (p.first  == usedSym)
              {
                operand = p.second;
                break;
              }
            }
          }
          break;
        case 'R':
          // cout << "DEBUG LOG: case R" << endl;
          if(instr>9999)
          {
            errorString = " Error: Illegal opcode; treated as 9999";
            operand = 999;
            opcode = 9;
          }
          else if(operand>instcount)
          {
            errorString = " Error: Relative address exceeds module size; zero used";
            operand=moduleBase;
          }
          else
          {
            operand += moduleBase;
          }
          break;
        default:
          errorString = "Unknown instruction type detected: " + addressmode;
      }
      int index = moduleBase+i;
      // char indexCharArray[7];
      // snprintf (indexCharArray, 7, "%05d", index);
      std::stringstream ss;
      ss << std::setw(3) << std::setfill('0') << index;
      ss << setfill(' ');
      std::string indexString = ss.str();

      int tmpInstr = opcode*1000 + operand;
      string instructionString;
      if (errorString.empty())
      {
        instructionString = indexString + ": " + to_string(tmpInstr);
      }
      else
      {
       instructionString = indexString + ": " + to_string(tmpInstr) + errorString; 
      }
      // string instructionString = string(indexCharArray) + ": " + to_string(tmpInstr) + errorString;
      // string instructionString = indexString + ": " + to_string(tmpInstr) + errorMessage;
      instructions.push_back(instructionString);
    }
    if (passNum == PASS_ONE)
    {
      //check symbol lengths; use instcount
      //Perform pass1 checks
    }
  }
}

void ParseInput(PASS_TYPE passNum, char* inputFile)
{
    // cout << "DEBUG LOG: Parse Input" << endl;
    // cout << "DEBUG LOG: Beginning pass " << passNum << endl;
    CreateTokens(passNum, inputFile);

    // Token t1 = GetToken();
    vector<int>::iterator moduleItr = moduleLengths.begin();
    vector<pair<string,int>>::iterator symbolsItr = symbols.begin();
    int moduleBase = 0;
    vector<string> moduleUseList;
    while(tokenItr != tokens.end())
    {
      // createModule();
      moduleUseList.clear();
      vector<pair<string,int>>::iterator localSymbolsItr = symbolsItr;
      // cout << "DEBUG LOG: Reading defintion count" << endl;
      int defcount = ReadInt();
      // cout << "DEBUG LOG: Reading definition list for " << defcount << " symbols." << endl;
      for (int i=0;i<defcount;i++)
      {
        // Symbol sym = ReadSymbol();
        string sym = ReadSymbol();
        int val = ReadInt();
        // cout << "DEBUG LOG: Read symbol " << sym << " and value " << val << endl;
        // createSymbol(sym,val);
        symbols.push_back(make_pair(sym, val+moduleBase));
      }

      int usecount = ReadInt();
      // cout << "DEBUG LOG: Reading uselist for " << usecount << " symbols."  << endl;
      for (int i=0;i<usecount;i++)
      {
        // Symbol sym = ReadSymbol();
        string sym = ReadSymbol();
        moduleUseList.push_back(sym);
        // cout << "DEBUG LOG: Read symbol " << sym << endl;
        //we don’t do anything here <- this would change in pass 2 }
      }

      int instcount = ReadInt();

      if (passNum == PASS_ONE)
      {
        moduleLengths.push_back(instcount);
      }
      // cout << "DEBUG LOG: Reading " << instcount << " instructions." << endl;
      
      ReadInstructions(passNum, moduleBase, instcount, usecount, moduleUseList);
      moduleBase += instcount;
    }
    
    if (passNum == PASS_ONE)
    {
      // To-do: print warnings
      cout << "Symbol Table" << endl;
      for(pair<string, int> p : symbols)
      {
        cout << p.first << "=" << p.second << endl;
      }
      cout << endl;
    }
    else
    {
      cout << "Memory Map" << endl;
      for(string instr : instructions)
      {
        cout << instr << endl;
      }
    }
}

int main (int argc, char** argv)
{
//     int num = 5;
//     printf("The answer is %05d.\n", num);

    // Read input file name from the command line arguments
    // if (argc < 2 || argv[1] == "")
    // {
    //   cout << "Fail: Provide input file name as program argument." << endl;
    //   exit(1);
    // }

    // INPUT_FILE = string(argv[1]);

    // printf("Input file name: %s\n", INPUT_FILE.c_str());
    // cout << "Input file name: " << argv[1] << endl;

    // fileArrayPtr = ReadFileToArray();

    // Parse for Symbol Map i.e. first pass
    ParseInput(PASS_ONE, argv[1]);

    // Parse second time to update addresses and resolve external references in Memory map
    ParseInput(PASS_TWO, argv[1]);

}
