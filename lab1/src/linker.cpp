#include <iostream>
#include <string>
#include <fstream>
#include <map>
// #include <fmt/format.h>

using namespace std;

/* For file based output
bool outputToFile = true;
bool isOutputFileOpen = false;
string OUTFILE = "output.txt";
ofstream outFileStream;
*/
enum PASS_TYPE {PASS_ONE, PASS_TWO};
enum ADDR_MODE {RELATIVE, EXTERNAL, IMMEDIATE, ABSOLUTE, ERROR};
map<string, int> symbolMap;

/* void PrintOutput(string str)
{
    if (outputToFile)
    {
        outFileStream << str;
    }
    else
    {
        cout << str;
    }
} */

/* void PerformInitializationTasks()
{
    if (outputToFile)
    {
        outFileStream.close();
        outFileStream.open(OUTFILE);
        isOutputFileOpen = true;
    }
} */

/*
* void PrintSymbolMap()
* {
* 
* }
*/

/*
* void PrintMemoryMap()
* {
* 
* }
*/

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

/* void PerformFinalTasks()
{
    if (isOutputFileOpen)
    {
        outFileStream.close();
    }
} */

void ParseInput(PASS_TYPE passNum)
{
    // getToken
}

int main (int argc, char** argv)
{
    cout << "Hello, world!" << endl;
    // PerformInitializationTasks();
    int num = 5;
    // PrintOutput(fmt::format("The answer is {:05}.", num));
    printf("The answer is %05d.\n", num);

    // Parse for Symbol Map i.e. first pass
    ParseInput(PASS_ONE);
    // PrintSymbolMap();

    // Parse second time to update addresses and resolve external references in Memory map
    ParseInput(PASS_TWO);
    // PrintMemoryMap();

    // PerformFinalTasks();
}
