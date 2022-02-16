#include <iostream>
#include <stdlib.h> 
#include <string>
#include <string.h>
#include <fstream>
#include <map>
#include <algorithm>
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

const string ERROR_STRING_2 = "Error: This variable is multiple times defined; first value used";

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
vector<int> instructions;
vector<string> allUsedSymbols;
map<string,int> symbolDefinedToModule;
map<string, string> symbolToError2;
vector<string> HasError2;
map<int, string> HasError3;
// vector<int> HasError6;
// vector<int> HasError8;
// vector<int> HasError9;
// vector<int> HasError10;
// vector<int> HasError11;
map<int,int> HasInstructionError;
map<int, string> InstructionErrorStrings;




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

void InitializeErrorStrings()
{
  InstructionErrorStrings[6] = "Error: External address exceeds length of uselist; treated as immediate";
  InstructionErrorStrings[8] = "Error: Absolute address exceeds machine size; zero used";
  InstructionErrorStrings[9] = "Error: Relative address exceeds module size; zero used";
  InstructionErrorStrings[10] = "Error: Illegal immediate value; treated as 9999";
  InstructionErrorStrings[11] = "Error: Illegal opcode; treated as 9999";
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

      // bool error6 = false;
      // bool error8 = false;
      // bool error9 = false;
      // bool error10 = false;
      // bool error11 = false;
      switch(addressmode)
      {
        case 'I':
          // cout << "DEBUG LOG: case I" << endl;
          // Rule 10
          if(instr>9999)
          {
            // errorString = " Error: Illegal immediate value; treated as 9999";
            // error10 =  true;
            // HasError10.push_back(i+moduleBase);
            HasInstructionError[i+moduleBase] = 10;
            operand = 999;
            opcode = 9;
          }
          break;
        case 'A':
          // cout << "DEBUG LOG: case B" << endl;
          if (operand>511)
          {
            // errorString = " Error: Absolute address exceeds machine size; zero used";
            // error8 = true;
            // HasError8.push_back(i+moduleBase);
            HasInstructionError[i+moduleBase] = 8;
            operand = 0;
          }
          break;
        case 'E':
          // cout << "DEBUG LOG: case E" << endl;
          if (operand>usecount-1)
          {
            // errorString = " Error: External address exceeds length of uselist; treated as immediate";
            // error6 = true;
            // HasError6.push_back(i+moduleBase);
            HasInstructionError[i+moduleBase] = 6;
          }
          else
          {
            string usedSym = moduleUseList[operand];
            // Rule 3 check
            if (symbolDefinedToModule.find(usedSym) == symbolDefinedToModule.end())
            {
              HasError3[i+moduleBase] = usedSym;
              operand = 0;
              break;
            }
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
          // Rule 11
          if(instr>9999)
          {
            // errorString = " Error: Illegal opcode; treated as 9999";
            // error11 =  true;
            // HasError11.push_back(i+moduleBase);
            HasInstructionError[i+moduleBase] = 11;
            operand = 999;
            opcode = 9;
          }
          else if(operand>instcount)
          {
            // errorString = " Error: Relative address exceeds module size; zero used";
            // error9 = true;
            // HasError9.push_back(i+moduleBase);
            HasInstructionError[i+moduleBase] = 9;
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
      /* std::stringstream ss;
      ss << std::setw(3) << std::setfill('0') << index;
      ss << setfill(' ');
      std::string indexString = ss.str(); */

      int tmpInstr = opcode*1000 + operand;

      /* string instructionString;
      if (errorString.empty())
      {
        instructionString = indexString + ": " + to_string(tmpInstr);
      }
      else
      {
       instructionString = indexString + ": " + to_string(tmpInstr) + errorString; 
      } */
      // string instructionString = string(indexCharArray) + ": " + to_string(tmpInstr) + errorString;
      // string instructionString = indexString + ": " + to_string(tmpInstr) + errorMessage;
      // instructions.push_back(instructionString);
      instructions.push_back(tmpInstr);
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
    int moduleCount = 0;
    vector<string> moduleUseList;
    while(tokenItr != tokens.end())
    {
      // createModule();
      moduleUseList.clear();
      moduleCount++;
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
        if (passNum == PASS_ONE)
        {
          if(symbolDefinedToModule.find(sym) != symbolDefinedToModule.end())
          {
            // std::stringstream  errorString2;
            // errorString2 << "Error: This variable is multiple times defined; first value used";
            // symbolToError2[sym] = errorString2.str();
            HasError2.push_back(sym);
          }
          else
          {
            symbols.push_back(make_pair(sym, val+moduleBase));
            symbolDefinedToModule[sym] = moduleCount;
          }
        }
      }

      int usecount = ReadInt();
      // cout << "DEBUG LOG: Reading uselist for " << usecount << " symbols."  << endl;
      for (int i=0;i<usecount;i++)
      {
        // Symbol sym = ReadSymbol();
        string sym = ReadSymbol();
        moduleUseList.push_back(sym);
        allUsedSymbols.push_back(sym);
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
        //Rule 2 check
        if (find(HasError2.begin(), HasError2.end(), p.first) != HasError2.end())
        {
          cout << p.first << "=" << p.second << " " << ERROR_STRING_2  << endl;
        }
        else
        {
          cout << p.first << "=" << p.second << endl;
        }
      }
      cout << endl;
    }
    else
    {
      cout << "Memory Map" << endl;
      // for(string instr : instructions)
      // {
      //   cout << instr << endl;
      // }
      for(int i=0; i < instructions.size(); i++)
      {
        if (HasError3.find(i) != HasError3.end())
        {
          printf("%03d: %d Error: %s is not defined; zero used\n", i, instructions[i], HasError3[i].c_str());
        }
        else if(HasInstructionError.find(i) != HasInstructionError.end())
        {
          printf("%03d: %d %s\n", i, instructions[i], InstructionErrorStrings[HasInstructionError[i]].c_str());
        }
        else
        {
          printf("%03d: %d\n", i, instructions[i]);
        }
      }
      // Rule 4 check
      std::stringstream  errorString4;
      bool errorFound4;
      for(pair<string, int> p : symbols)
      {
        if (find(allUsedSymbols.begin(), allUsedSymbols.end(), p.first) == allUsedSymbols.end())
        {
          errorString4 << "Warning: Module " << symbolDefinedToModule[p.first] << ": " << p.first << " was defined but never used" << "\n";
          errorFound4 = true;
        }
      }
      if(errorFound4)
      {
        cout << "\n" << errorString4.str();
      }
    }
}

int main (int argc, char** argv)
{
//     int num = 5;
//     printf("The answer is %05d.\n", num);

    InitializeErrorStrings();

    // Parse for Symbol Map i.e. first pass
    ParseInput(PASS_ONE, argv[1]);

    // Parse second time to update addresses and resolve external references in Memory map
    ParseInput(PASS_TWO, argv[1]);

}
