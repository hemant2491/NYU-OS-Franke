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
#include <tuple>
#include <math.h>


using namespace std;

enum PASS_TYPE {PASS_ONE, PASS_TWO};
enum ADDR_MODE {RELATIVE, EXTERNAL, IMMEDIATE, ABSOLUTE, ERROR};

// Struct with line number and offset info of a string
struct StringLineOffset
{
    string value;
    int lineNumber;
    int offset;

    StringLineOffset(string _value, int _lineNum, int _offset)
    {
        value = _value;
        lineNumber = _lineNum;
        offset = _offset;
        empty = false;
    }
    StringLineOffset()
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

ostream& operator<<(ostream& os, const StringLineOffset& token) {
        return os << "Token: " 
                            << token.lineNumber << ":" << token.offset
                            << " : " << token.value;
}

ifstream fin;
string line;
char* lineCharArray = NULL;
char* tokenString = NULL;
int lineNumber = 0;
int offset = -1;
int moduleNumber = 0;
char DELIMS[3] = {' ', '\t', '\n'};
int lastLineNumber = 0;
int lastOffset = 0;
vector<StringLineOffset> tokens;
vector<StringLineOffset>::iterator tokenItr;
/* vector containing Symbol map details */
vector<pair<string, int>> symbolsAddresses;
/* vector containing instructions */
vector<int> instructions;
/* List of used symbols from all Modules */
vector<string> allUsedSymbols;
/* Each symbol definition to Module mapping */
map<string,int> symbolDefinedToModule;
/* Symbols with Rule 2 errors */
vector<string> HasError2;
/* Symbols with Rule 3 errors */
map<int, string> HasError3;

/* Instruction addresses to Instructions with errors for rules 6, 8, 9, 10 and 11*/
map<int,int> HasInstructionError;
map<int, string> InstructionErrorStrings;
/* For Rule 5 */
map<int,vector<tuple<string,int,int>>> moduleToExceedingSymbolAddressMax;
/* For Rule 7 */
map<int,vector<string>> moduleToUnusedSymbols;


// Error strings for Rule 2
const string ERROR_STRING_2 = "Error: This variable is multiple times defined; first value used";

// Error strings for Rules 6, 8, 9, 10 and 11
void InitializeErrorStrings()
{
    InstructionErrorStrings[6] = "Error: External address exceeds length of uselist; treated as immediate";
    InstructionErrorStrings[8] = "Error: Absolute address exceeds machine size; zero used";
    InstructionErrorStrings[9] = "Error: Relative address exceeds module size; zero used";
    InstructionErrorStrings[10] = "Error: Illegal immediate value; treated as 9999";
    InstructionErrorStrings[11] = "Error: Illegal opcode; treated as 9999";
}

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

// Read input file and create StringLineOffset structures
void DetermineOffsets(PASS_TYPE passNum, char* inputFile)
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
    lineNumber=0;
    lastLineNumber = 0;
    lastOffset = 0;
    int thisLineNumber = 0;
    int thisLineOffset = 0;
    while (fin) {
        lastLineNumber = thisLineNumber;
        lastOffset = thisLineOffset;

        lineNumber++;
        offset = -1;
        // Read a line from input file
        getline(fin, line);

        thisLineNumber = lineNumber;
        thisLineOffset = line.length()+1;

        char line_cstr[line.length() + 1];
        strcpy(line_cstr, line.c_str());
        // PrintCharArray(line_cstr);
        char* tokenString = strtok(line_cstr, DELIMS);
        
        // // cout << "DEBUG LOG: line: " << line << endl;
        while (tokenString != NULL)
        {
            // printf ("'%s'\n",tokenString);
            offset = line.find(tokenString, offset+1);
            StringLineOffset t1 = StringLineOffset(tokenString, lineNumber, offset+1);
            tokens.push_back(t1);
            // PrintCharArray(line_cstr);
            tokenString = strtok (NULL, DELIMS);            
            // cout << "DEBUG LOG: " << t1 << endl;
        }
    }

    // Close input file
    fin.close();
    tokenItr = tokens.begin();
}

// Print following parsing errors:
//         "NUM_EXPECTED", "SYM_EXPECTED", "ADDR_EXPECTED", "SYM_TOO_LONG",
//         "TOO_MANY_DEF_IN_MODULE", "TOO_MANY_USE_IN_MODULE", "TOO_MANY_INSTR"
void PrintParseError(int linenum, int lineoffset, string error)
{
    printf("Parse Error line %d offset %d: %s\n", linenum, lineoffset, error.c_str());
}

// Get next token
StringLineOffset GetToken()
{
    if (tokenItr != tokens.end())
    {
        StringLineOffset t1 = *tokenItr;
        tokenItr++;
        // tokens.erase(tokens.begin());
        return t1;
    }
    else
    {
        return StringLineOffset();
    }
}

int ReadInt(bool checkDefCount, bool checkUseCount, bool checkInstrCount, int moduleBase = -1)
{
    StringLineOffset intToken = GetToken();
    if (intToken.IsEmpty())
    {
        PrintParseError(lastLineNumber, lastOffset, "NUM_EXPECTED");
        exit(1);
    }
    int digitLength = 0;
    int tempNum;
    // cout << "DEBUG LOG: " << intToken << endl;
    const char* tempCharArr = intToken.value.c_str();
    while(isdigit(tempCharArr[digitLength]))
    {
        digitLength++;
    }
    bool hasNumError = digitLength < intToken.value.length();

    if (!hasNumError)
    {
      tempNum = stoi(intToken.value);
      hasNumError = (tempNum >= (int) pow(2, 30));
    }
    if (hasNumError)
    {
        PrintParseError(intToken.lineNumber, intToken.offset, "NUM_EXPECTED");
        exit(1);
    }
    if (checkDefCount && tempNum > 16)
    {
        PrintParseError(intToken.lineNumber, intToken.offset, "TOO_MANY_DEF_IN_MODULE");
        exit(1);
    }
    if (checkUseCount && tempNum > 16)
    {
        PrintParseError(intToken.lineNumber, intToken.offset, "TOO_MANY_USE_IN_MODULE");
        exit(1);
    }
    int totalInstrCount = tempNum + moduleBase;
    if (checkInstrCount && totalInstrCount > 512)
    {
        PrintParseError(intToken.lineNumber, intToken.offset, "TOO_MANY_INSTR");
        exit(1);
    }
    return tempNum;
}

string ReadSymbol()
{
    StringLineOffset symbolToken = GetToken();
    // cout << "DEBUG LOG: " << symbolToken << endl;
    if (symbolToken.IsEmpty())
    {
        PrintParseError(lastLineNumber, lastOffset, "SYM_EXPECTED");
        exit(1);
    }
    int symLength = 1;
    int tempSym;
    const char* tempCharArr = symbolToken.value.c_str();
    bool hasSymbolError = !isalpha(tempCharArr[0]);
    while(!hasSymbolError && isalnum(tempCharArr[symLength]))
    {
        symLength++;
    }

    hasSymbolError = symLength < symbolToken.value.length();
    if (hasSymbolError)
    {
        PrintParseError(symbolToken.lineNumber, symbolToken.offset, "SYM_EXPECTED");
        exit(1);
    }
    if (symbolToken.value.length() > 16)
    {
        PrintParseError(symbolToken.lineNumber, symbolToken.offset, "SYM_TOO_LONG");
        exit(1);
    }
    return symbolToken.value;
}

char ReadIARE()
{
        //read I, A, E or R
        // cout << "DEBUG LOG: " << IAREToken << endl;
        string iare = "IARE";
        StringLineOffset IAREToken = GetToken();
        if (IAREToken.IsEmpty())
        {
            PrintParseError(lastLineNumber, lastOffset, "ADDR_EXPECTED");
            exit(1);
        }

        if (IAREToken.value.length() > 1 || !isalpha(IAREToken.value.c_str()[0]) || iare.find(IAREToken.value.c_str()) == string::npos)
        {
            PrintParseError(IAREToken.lineNumber, IAREToken.offset, "ADDR_EXPECTED");
            exit(1);
        }
        
        return IAREToken.value.front();
}

void ReadInstructions(PASS_TYPE passNum, int moduleNumber, int moduleBase, int instcount, int usecount, vector<string> moduleUseList)
{
    /* cout << "DEBUG LOG: pass " << passNum << " module base " << moduleBase
             << " inst count " << instcount << " use count " << usecount << endl; */
    /* cout << " DEBUG LOG: Use List - {";
    for (string sym : moduleUseList){ cout << " " << sym; } cout << "}" << endl; */
    vector<string> symbolsUsedInModule;
    for (int i=0; i<instcount; i++)
    {
        char addressmode = ReadIARE();
        int    instr = ReadInt(false, false, false);
        if (passNum == PASS_TWO)
        {
            string errorString;
            int opcode = instr/1000;
            int operand = instr%1000;
            // cout << "DEBUG LOG: Instruction addr type " << addressmode
            // << " opcode " << opcode << " operand " << operand << endl;

            switch(addressmode)
            {
                case 'I':
                    // cout << "DEBUG LOG: case I" << endl;
                    // Rule 10: If an illegal immediate value (I) is encountered (i.e. >= 10000), print an error and convert the value to 9999.
                    if(instr>9999)
                    {
                        HasInstructionError[i+moduleBase] = 10;
                        operand = 999;
                        opcode = 9;
                    }
                    break;
                case 'A':
                    // cout << "DEBUG LOG: case B" << endl;
                    // Rule 8: If an absolute address exceeds the size of the machine, print an error message and use the absolute value zero.
                    if (operand>511)
                    {
                        HasInstructionError[i+moduleBase] = 8;
                        operand = 0;
                    }
                    break;
                case 'E':
                    // cout << "DEBUG LOG: case E" << endl;
                    // Rule 6: If an external address is too large to reference an entry in the use list,
                    // print an error message and treat the address as immediate.
                    if (operand>usecount-1)
                    {
                        HasInstructionError[i+moduleBase] = 6;
                    }
                    else
                    {
                        string usedSym = moduleUseList[operand];
                        symbolsUsedInModule.push_back(usedSym);
                        // Check Rule 3: If a symbol is used in an E-instruction but not defined anywhere, print an error message and use the value absolute zero.
                        if (symbolDefinedToModule.find(usedSym) == symbolDefinedToModule.end())
                        {
                            HasError3[i+moduleBase] = usedSym;
                            operand = 0;
                            break;
                        }
                        for (pair<string, int> p : symbolsAddresses)
                        {
                            if (p.first    == usedSym)
                            {
                                operand = p.second;
                                break;
                            }
                        }
                    }
                    break;
                case 'R':
                    // cout << "DEBUG LOG: case R" << endl;
                    // Rule 11: If an illegal opcode is encountered (i.e. op >= 10), print an error and convert the <opcode,operand> to 9999.
                    if(instr>9999)
                    {
                        HasInstructionError[i+moduleBase] = 11;
                        operand = 999;
                        opcode = 9;
                    }
                    // Rule 9: If a relative address exceeds the size of the module, print an error message and use the module relative value zero
                    else if(operand>instcount)
                    {
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
            int tmpInstr = opcode*1000 + operand;

            instructions.push_back(tmpInstr);
        }
    }

    if (passNum == PASS_TWO)
    {
        for (string sym: moduleUseList)
        {
            if(find(symbolsUsedInModule.begin(), symbolsUsedInModule.end(), sym) == symbolsUsedInModule.end())
            {
                if (find(moduleToUnusedSymbols[moduleNumber].begin(), moduleToUnusedSymbols[moduleNumber].end(), sym) == moduleToUnusedSymbols[moduleNumber].end())
                {
                    moduleToUnusedSymbols[moduleNumber].push_back(sym);
                }
            }
        }
    }
}

void ParseInput(PASS_TYPE passNum, char* inputFile)
{
        // cout << "DEBUG LOG: Parse Input" << endl;
        // cout << "DEBUG LOG: Beginning pass " << passNum << endl;
        DetermineOffsets(passNum, inputFile);

        // StringLineOffset t1 = GetToken();
        vector<pair<string,int>>::iterator symbolsItr = symbolsAddresses.begin();
        int moduleBase = 0;
        int moduleCount = 0;
        vector<string> moduleUseList;
        while(tokenItr != tokens.end())
        {
            moduleUseList.clear();
            moduleCount++;
            vector<pair<string,int>>::iterator localSymbolsItr = symbolsItr;
            map<string,int> localSymbolsAdresses;
            // cout << "DEBUG LOG: Reading defintion count" << endl;
            int defcount = ReadInt(true, false, false);
            // cout << "DEBUG LOG: Reading definition list for " << defcount << " symbols." << endl;
            for (int i=0;i<defcount;i++)
            {
                // Symbol sym = ReadSymbol();
                string sym = ReadSymbol();
                int val = ReadInt(false,false,false);
                // cout << "DEBUG LOG: Read symbol " << sym << " and value " << val << endl;
                if (passNum == PASS_ONE)
                {
                    // Check Rule 2: If a symbol is defined multiple times, print an error message and use the value given in the first definition.
                    if(symbolDefinedToModule.find(sym) != symbolDefinedToModule.end())
                    {
                        HasError2.push_back(sym);
                    }
                    else
                    {
                        localSymbolsAdresses[sym] = val;
                        symbolsAddresses.push_back(make_pair(sym, val+moduleBase));
                        symbolDefinedToModule[sym] = moduleCount;
                    }
                }
            }

            int usecount = ReadInt(false,true,false);
            // cout << "DEBUG LOG: Reading uselist for " << usecount << " symbols."    << endl;
            for (int i=0;i<usecount;i++)
            {
                string sym = ReadSymbol();
                moduleUseList.push_back(sym);
                allUsedSymbols.push_back(sym);
                // cout << "DEBUG LOG: Read symbol " << sym << endl;
            }

            int instcount = ReadInt(false,false,true,moduleBase);
            // cout << "DEBUG LOG: Reading " << instcount << " instructions." << endl;
            ReadInstructions(passNum, moduleCount, moduleBase, instcount, usecount, moduleUseList);

            // Find violators for Rule 5: If an address appearing in a definition exceeds the size of the module,
            // print a warning message and treat the address given as 0 (relative to the module).
            for (auto it = localSymbolsAdresses.begin(); it != localSymbolsAdresses.end(); it++)
            {
                if (it->second >= instcount)
                {
                    moduleToExceedingSymbolAddressMax[moduleCount].push_back(make_tuple(it->first, it->second, instcount-1));
                    for(auto itt = symbolsAddresses.begin(); itt != symbolsAddresses.end(); itt++)
                    {
                        if (itt->first == it->first)
                        {
                            itt->second = moduleBase;
                            break;
                        }
                    }
                }
            }
            moduleBase += instcount;
        }
        
        if (passNum == PASS_TWO)
        {
            // Print error for Rule 5
            if (!moduleToExceedingSymbolAddressMax.empty())
            {
                for (int i = 0; i <= moduleCount; i++)
                {
                    if(moduleToExceedingSymbolAddressMax.find(i) != moduleToExceedingSymbolAddressMax.end())
                    {
                        for (tuple<string,int,int> t : moduleToExceedingSymbolAddressMax[i])
                        {
                            printf("Warning: Module %d: %s too big %d (max=%d) assume zero relative\n", i, get<0>(t).c_str(), get<1>(t), get<2>(t));
                        }
                    }
                }
            }

            cout << "Symbol Table" << endl;
            for(pair<string, int> p : symbolsAddresses)
            {
                // Print errors for Rule 2
                if (find(HasError2.begin(), HasError2.end(), p.first) != HasError2.end())
                {
                    cout << p.first << "=" << p.second << " " << ERROR_STRING_2    << endl;
                }
                else
                {
                    cout << p.first << "=" << p.second << endl;
                }
            }
            cout << endl;

            cout << "Memory Map" << endl;
            for(int i=0; i < instructions.size(); i++)
            {
                // Print errors for Rule 3
                if (HasError3.find(i) != HasError3.end())
                {
                    printf("%03d: %04d Error: %s is not defined; zero used\n", i, instructions[i], HasError3[i].c_str());
                }
                // Print errors for Rules 6, 8, 9, 10 and 11
                else if(HasInstructionError.find(i) != HasInstructionError.end())
                {
                    printf("%03d: %04d %s\n", i, instructions[i], InstructionErrorStrings[HasInstructionError[i]].c_str());
                }
                else
                {
                    printf("%03d: %04d\n", i, instructions[i]);
                }
            }

            // Check Rule 7: If a symbol appears in a use list but is not actually used in the module (i.e., not referred to in an E-type address),
            // print a warning message and continue.
            if (!moduleToUnusedSymbols.empty())
            {
                for (int i = 1; i <= moduleCount; i++)
                {
                    if(moduleToUnusedSymbols.find(i) != moduleToUnusedSymbols.end())
                    {
                        for (string sym : moduleToUnusedSymbols[i])
                        {
                            printf("Warning: Module %d: %s appeared in the uselist but was not actually used\n", i, sym.c_str());
                        }
                    }
                }
            }
            
            // Check Rule 4: If a symbol is defined but not used
            std::stringstream errorString4;
            bool errorFound4;
            for(pair<string, int> p : symbolsAddresses)
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
    InitializeErrorStrings();

    // Parse for Symbol Map i.e. first pass
    ParseInput(PASS_ONE, argv[1]);

    // Parse second time to update addresses and resolve external references in Memory map
    ParseInput(PASS_TWO, argv[1]);
}
