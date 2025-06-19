// $Id: sm.c,v 1.8 1993/06/09 16:25:03 rmartin Exp $
// ----------------------------------------------------------
// Name
//  sm.c
// 
// Description
//  This is the set of C functions called by the smc yacc parser.
//  it writes the C++ program which controls the finite state
//  machine.
// 
// Bugs
//  This code is very ugly.  It has evloved over time.  It
//  needs to be rewritten as a C++ program with typesafe objects
//  and all.   (any takers?)
// 

#include <stdio.h>
//#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

typedef enum {NO=0, YES=1} Bool;

extern char GfsmName[], GcontextName[], 
			Gversion[];

static char LinitialStateName[255];
static Bool LinitialStateSet = NO;


extern int lineNumber;


typedef enum TheType {
	state, subState, 
	superState, 
	superSubState, 
	transition, 
	action
} TheType;

typedef struct Node
{
	struct Node *link;
	char stateName[255];
	TheType type;
	char transitionName[255];
	char actionName[255];
	char superStateName[255];
	struct Node* subStates;
	struct Node* transitions;
	struct Node* actions;
} Node;
#define NONODE ((Node*)0)

typedef struct nameNode
{
	struct nameNode *link;
	Node *itsNode;
	char name[255];
} nameNode;

Node *stack = NULL;
Node *actionList = NULL;
Node *currentState = NULL;
Node *currentTransition = 0;
Node *currentAction = 0;

struct nameNode *tNameList = NULL;
struct nameNode *sNameList = NULL;
struct nameNode *hNameList = NULL;
FILE *cFile;
FILE *hFile;


Node* FindState(char *theStateName)
{
    struct nameNode* p;
    Node* retval = 0;
    for (p=sNameList; !retval && p; p=p->link)
    {
        if (strcmp(theStateName, p->name) == 0)
        {
            retval = p->itsNode;
        }
    }
    return retval;
}

int yyparse(void);


void yyerror(const char * s)
{
	printf("Line %d:%s\n",lineNumber,s);
}	


int yywrap()
{
	return 1;
}	

void CheckNotSuperState(char * theStateName)
{
    Node* found = FindState(theStateName);
	if (found && 
            (found->type == superState || found->type == superSubState))
	{
	    printf("Line: %d. '%s' can't be a super state\n",
	            lineNumber, theStateName);
	    exit(1);
	}
}

void AddName(nameNode ** nameList, char * tName,Node * node)
//struct nameNode **nameList;
//char* tName;
//Node* node;
{
	struct nameNode *p;
	struct nameNode* found = 0;

	for (p=*nameList; p && !found; p=p->link)
	{
		if (strcmp(p->name, tName) == 0)
		{
			found = p;
		}
	}

	if (!found) /* didn't find it */
	{
		p = (struct nameNode *)malloc(sizeof(struct nameNode));
		p->link = *nameList;
		*nameList = p;
		strcpy(p->name, tName);
		p->itsNode = node;
	}
	else if (node != NONODE)
	{
	    if (found->itsNode == 0)
	    {
	        found->itsNode = node;
	    }
	    else if (found->itsNode != node)
	    {
	        printf("Line %d: '%s' redefined.\n", lineNumber, tName);
	        exit(1);
	    }
	}
}

void PushState(char * theStateName)
{
	Node *myNode = FindState(theStateName);
    CheckNotSuperState(theStateName);
    if (myNode == 0)
    {
		myNode = (Node*) malloc(sizeof(Node));
		myNode->type = state;
		myNode->subStates = 0;
		myNode->transitions = 0;
		strcpy(myNode->stateName, theStateName);
		myNode->link = stack;
		stack = myNode;
		AddName(&sNameList, myNode->stateName, myNode);
	}
	currentState = myNode;
	currentTransition = 0;
}

void CheckNotLeafState(char * theStateName)
{
    Node* found = FindState(theStateName);
	if (found &&  
	   (found->type == state || found->type == subState))
	{
	    printf("Line: %d. '%s' can't be a target of a transition.\n",
	            lineNumber, theStateName);
	    exit(1);
	}
}

void PushSubState(char * theStateName,char *  superStateName)
{
	Node *myNode = FindState(theStateName);
	CheckNotSuperState(theStateName);
	CheckNotLeafState(superStateName);

	if (myNode == 0)
	{
		Node *superNode;
		myNode = (Node*) malloc(sizeof(Node));
		myNode->subStates = 0;
		myNode->transitions = 0;
		superNode = FindState(superStateName);
		if (!superNode) 
		{
			printf("Line %d: '%s' not defined\n", 
				   lineNumber, superStateName);
			exit(1);
		}
		myNode->type = subState;
		strcpy(myNode->stateName, theStateName);
		strcpy(myNode->superStateName, superStateName);
		myNode->link = superNode->subStates;
		superNode->subStates = myNode;
		AddName(&sNameList, myNode->stateName, myNode);
	}
	currentState = myNode;
	currentTransition = 0;
}

void PushSuperState(char *theStateName)
{
	Node *myNode = FindState(theStateName);
    CheckNotLeafState(theStateName);
	if (myNode == 0)
	{
		myNode = (Node*) malloc(sizeof(Node));
		myNode->subStates = 0;
		myNode->transitions = 0;
		myNode->type = superState;
		strcpy(myNode->stateName, theStateName);
		myNode->link = stack;
		stack = myNode;
		AddName(&sNameList, myNode->stateName, myNode);
	}
	currentState = myNode;
	currentTransition = 0;
}

void PushSuperSubState(char * theStateName, char * superStateName)
{
	Node *myNode = FindState(theStateName);
	CheckNotLeafState(theStateName);
	CheckNotLeafState(superStateName);
	if (myNode == 0)
	{
		Node *superNode;
		myNode = (Node*) malloc(sizeof(Node));
		myNode->subStates = 0;
		myNode->transitions = 0;
		myNode->type = superSubState;
		superNode = FindState(superStateName);
		if (!superNode) 
		{
			printf("Line %d: '%s' not defined\n", 
				   lineNumber, superStateName);
			exit(1);
		}
		strcpy(myNode->stateName, theStateName);
		strcpy(myNode->superStateName, superStateName);
		myNode->link = superNode->subStates;
		superNode->subStates = myNode;

		AddName(&sNameList, myNode->stateName, myNode);
	}
	currentState = myNode;
	currentTransition = 0;
}

void SetInitialState(char* initialStateName)
{
    strcpy(LinitialStateName, initialStateName);
    LinitialStateSet = YES;
    AddName(&sNameList, initialStateName, NONODE);
}

void PushTransitionLine(char * trans, char * theStateName)
{
	Node *myNode = (Node*) malloc(sizeof(Node));
    if (currentState == 0)
    {
    	printf("Line %d: No current state for '%s'\n", lineNumber, trans);
    }
	myNode->actions = 0;
	myNode->type = transition;
	strcpy(myNode->transitionName, trans);
	strcpy(myNode->stateName, theStateName);
	myNode->link = currentState->transitions;
	currentState->transitions = myNode;

	AddName(&tNameList, trans, NONODE);
	CheckNotSuperState(myNode->stateName);
	AddName(&sNameList, myNode->stateName, NONODE);
	currentTransition = myNode;
	currentAction = 0;
}

void PushHeader(char * header)
{
	AddName(&hNameList, header, NONODE);
}

void PushAction(char * actionName)
{
	Node *myNode = (Node*) malloc(sizeof(Node));
	myNode->type = action;
	myNode->link = 0;
	strcpy(myNode->actionName, actionName);
	if (currentAction == 0)
	{
		currentTransition->actions = myNode;
	}
	else
	{
		currentAction->link = myNode;
	}
	currentAction = myNode;
}


void DumpStateHeaders(Node * theNode)
{
    Node* t;
	fprintf(hFile, "\n");
	if (theNode->type == state || theNode->type == superState)
	{
		fprintf(hFile, "class %s%sState : public %sState {\n",
			GfsmName, theNode->stateName,  GfsmName);
	}
	else if (theNode->type == subState || theNode->type == superSubState)
	{
		fprintf(hFile, "class %s%sState : public %s%sState {\n",
			GfsmName, theNode->stateName,  
			GfsmName, theNode->superStateName);
	}
	fprintf(hFile, "public:\n");
	if (theNode->type == state || theNode->type == subState)
	{
		fprintf(hFile, 
			"  virtual const char* StateName() const\n");
		fprintf(hFile, 
			"  {return(\"%s\");};\n", theNode->stateName);
	}

	for (t = theNode->transitions; t; t=t->link)
	{
		fprintf(hFile, "  virtual void %s(%s&);\n", 
			t->transitionName,GfsmName); 
	}
	fprintf(hFile, "};\n");

	if (theNode->subStates) DumpStateHeaders(theNode->subStates);
	if (theNode->link) DumpStateHeaders(theNode->link);
}

void DumpStateImplementations(Node * theNode)
{
    Node* t = 0;
    for (t=theNode->transitions; t; t=t->link)
    {
        Node* a = 0;
		fprintf(cFile, "void %s%sState::%s(%s& s) {\n",
			GfsmName, theNode->stateName, t->transitionName, GfsmName);
		fprintf(cFile, "  s.SetState(%s::%sState);\n", 
						GfsmName, t->stateName);
		for (a=t->actions; a; a=a->link)
		{
			fprintf(cFile, "  s.%s();\n", a->actionName);
		}
		fprintf(cFile, "}\n");
    }
	if (theNode->subStates) DumpStateImplementations(theNode->subStates);
	if (theNode->link) DumpStateImplementations(theNode->link);
}


// void GenStateMap()
// {
// 	char cName[255];
// 	char hName[255];
// 	Node *myNode;
// 	struct nameNode *tName;
// 	struct nameNode *sName;
// 	struct nameNode *hdrName;
// 	Node* reverseList = NULL;
// 	Bool inState = NO;
// 	Bool inTransition = NO;
// 	char currentState[255];
// 	char contextHeader[256];
// 
// 	sprintf(cName, "%s.cc", GfsmName);
// 	sprintf(hName, "%s.h", GfsmName);
// 	/*cName[0] = tolower(cName[0]);*/
// 	/*hName[0] = tolower(hName[0]);*/
// 
// 	cFile = fopen(cName, "w");
// 	if (cFile == NULL)
// 	{
// 		perror(cName);
// 		exit(1);
// 	}
// 
// 	hFile = fopen(hName, "w");
// 	if (hFile == NULL)
// 	{
// 		perror(hName);
// 		exit(1);
// 	}
// 
// 	fprintf(hFile, "#ifndef _H_%s\n#define _H_%s\n",
// 		GfsmName, GfsmName);
// 
// 	fprintf(hFile, "#include <stddef.h>\n");
// 
// 	for (hdrName=hNameList; hdrName; hdrName=hdrName->link)
// 	{
// 		fprintf(hFile, "#include \"%s\"\n", hdrName->name);
// 	}
// 
//     fprintf(hFile, "class %s;\n", GfsmName);
// 	fprintf(hFile, "\n");
// 	fprintf(hFile, "class %sState {\n", 
// 								GfsmName);
// 	fprintf(hFile, "public:\n");
// 
// 	fprintf(hFile, "\n");
// 	fprintf(hFile, "  virtual const char* StateName() const = 0;\n");
// 
// 	for (tName=tNameList; tName; tName=tName->link)
// 	{
// 		fprintf(hFile, "  virtual void %s(%s& s);\n", tName->name,
// 				GfsmName);
// 	}
// 		
// 	fprintf(hFile, "};\n");
// 
//     DumpStateHeaders(stack);
// 
//     fprintf(hFile, "class %s : public %s {\n", GfsmName, GcontextName);
//     fprintf(hFile, "  public:\n");
// 
// 	for (sName=sNameList; sName; sName=sName->link)
// 	{
// 	    if (sName->itsNode->type == state ||
// 	        sName->itsNode->type == subState)
// 	    {
// 			fprintf(hFile, "  static %s%sState %sState;\n",
// 				GfsmName, sName->name, sName->name);
// 		}
// 	}
//     if (LinitialStateSet)
//     {
// 		fprintf(hFile, "  %s();// default constructor\n", GfsmName);
// 	}
// 
// 	for (tName=tNameList; tName; tName=tName->link)
// 	{
// 		fprintf(hFile, "  void %s() {itsState->%s(*this);}\n",
// 		               tName->name, tName->name);
// 	}
// 
//     fprintf(hFile, "  void SetState(%sState& theState) {itsState=&theState;}\n",
//                    GfsmName);
//     fprintf(hFile, "  %sState& GetState() const {return *itsState;};\n",
//                     GfsmName);
// 	fprintf(hFile, "  private:\n");
// 	fprintf(hFile, "    %sState* itsState;\n", GfsmName);
// 	fprintf(hFile, "};\n");
// 
// 	fprintf(hFile, "#endif\n");
// 	fclose(hFile);
// 	inState = NO;
// 
// 	fprintf(cFile, "#include \"%s\"\n", hName);
// 
// 	fprintf(cFile, "static char _versID[] = \"%s\";\n", Gversion);
// 
// 	for (sName=sNameList; sName; sName=sName->link)
// 	{
// 	    if (sName->itsNode->type == state ||
// 	        sName->itsNode->type == subState)
// 	    {
// 		    fprintf(cFile, "%s%sState %s::%sState;\n", 
// 			    GfsmName, sName->name, GfsmName, sName->name);
// 		}
// 	}
// 
// 	for (tName=tNameList; tName; tName=tName->link)
// 	{
// 		fprintf(cFile, "void %sState::%s(%s& s)\n", 
// 		        GfsmName, tName->name, GfsmName);
// 		fprintf(cFile, 
// 			"  {s.FSMError(\"%s\", s.GetState().StateName());}\n",
// 			tName->name);
// 	}
// 
// 	DumpStateImplementations(stack);
// 
// 	if (LinitialStateSet)
// 	{
// 	    fprintf(cFile, "%s::%s() : itsState(&%sState) {}\n", 
// 	            GfsmName, GfsmName, LinitialStateName);
// 	}
// }

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>


std::istream* inputStream = nullptr;
std::ifstream fileStream;
std::string outputDirectory = ".";



void GenStateMap()
{
	char cName[255];
	char hName[255];

	// Build full paths using std::filesystem::path
	std::filesystem::path cPath = std::filesystem::path(outputDirectory) / (std::string(GfsmName) + ".cc");
	std::filesystem::path hPath = std::filesystem::path(outputDirectory) / (std::string(GfsmName) + ".h");

	// Copy paths into old-style char arrays
	strncpy(cName, cPath.c_str(), sizeof(cName));
	strncpy(hName, hPath.c_str(), sizeof(hName));
	cName[sizeof(cName) - 1] = '\0';
	hName[sizeof(hName) - 1] = '\0';

	cFile = fopen(cName, "w");
	if (cFile == NULL) {
		perror(cName);
		exit(1);
	}

	hFile = fopen(hName, "w");
	if (hFile == NULL) {
		perror(hName);
		exit(1);
	}

	fprintf(hFile, "#ifndef _H_%s\n#define _H_%s\n", GfsmName, GfsmName);
	fprintf(hFile, "#include <stddef.h>\n");

	for (nameNode* hdrName = hNameList; hdrName; hdrName = hdrName->link) {
		fprintf(hFile, "#include \"%s\"\n", hdrName->name);
	}

	fprintf(hFile, "class %s;\n\n", GfsmName);
	fprintf(hFile, "class %sState {\npublic:\n", GfsmName);
	fprintf(hFile, "  virtual const char* StateName() const = 0;\n");

	for (nameNode* tName = tNameList; tName; tName = tName->link) {
		fprintf(hFile, "  virtual void %s(%s& s);\n", tName->name, GfsmName);
	}

	fprintf(hFile, "};\n");

	DumpStateHeaders(stack);

	fprintf(hFile, "class %s : public %s {\npublic:\n", GfsmName, GcontextName);

	for (nameNode* sName = sNameList; sName; sName = sName->link) {
		if (sName->itsNode->type == state || sName->itsNode->type == subState) {
			fprintf(hFile, "  static %s%sState %sState;\n", GfsmName, sName->name, sName->name);
		}
	}

	if (LinitialStateSet) {
		fprintf(hFile, "  %s(); // default constructor\n", GfsmName);
	}

	for (nameNode* tName = tNameList; tName; tName = tName->link) {
		fprintf(hFile, "  void %s() { itsState->%s(*this); }\n", tName->name, tName->name);
	}

	fprintf(hFile, "  void SetState(%sState& theState) { itsState = &theState; }\n", GfsmName);
	fprintf(hFile, "  %sState& GetState() const { return *itsState; }\n", GfsmName);
	fprintf(hFile, "private:\n");
	fprintf(hFile, "  %sState* itsState;\n", GfsmName);
	fprintf(hFile, "};\n");
	fprintf(hFile, "#endif\n");
	fclose(hFile);

	fprintf(cFile, "#include \"%s\"\n", hName);
	fprintf(cFile, "static char _versID[] = \"%s\";\n", Gversion);

	for (nameNode* sName = sNameList; sName; sName = sName->link) {
		if (sName->itsNode->type == state || sName->itsNode->type == subState) {
			fprintf(cFile, "%s%sState %s::%sState;\n", GfsmName, sName->name, GfsmName, sName->name);
		}
	}

	for (nameNode* tName = tNameList; tName; tName = tName->link) {
		fprintf(cFile, "void %sState::%s(%s& s)\n", GfsmName, tName->name, GfsmName);
		fprintf(cFile, "  { s.FSMError(\"%s\", s.GetState().StateName()); }\n", tName->name);
	}

	DumpStateImplementations(stack);

	if (LinitialStateSet) {
		fprintf(cFile, "%s::%s() : itsState(&%sState) {}\n", GfsmName, GfsmName, LinitialStateName);
	}

	fclose(cFile);
}


void printUsage(const char* programName) {
    std::cerr << "Usage: " << programName << " [input_file] [-o output_directory]\n";
}

int parseArguments(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-o") {
            if (i + 1 >= argc) {
                std::cerr << "Missing argument for -o\n";
                return 1;
            }
            outputDirectory = argv[++i];
            if (!std::filesystem::is_directory(outputDirectory)) {
                std::cerr << "Output path is not a directory: " << outputDirectory << "\n";
                return 1;
            }
        } else if (!inputStream) {
            fileStream.open(arg);
            if (!fileStream) {
                std::cerr << "Could not open input file: " << arg << "\n";
                return 1;
            }
            inputStream = &fileStream;
        } else {
            std::cerr << "Unknown or duplicate argument: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        }
    }

    if (!inputStream) {
        inputStream = &std::cin;
    }

    return 0;
}


int main(int argc, char** argv)
{
    lineNumber = 1;

    if (parseArguments(argc, argv) != 0) {
        return 1;
    }

    // Redirect `yyin` to use the input stream if it's not stdin
    extern FILE* yyin;
    if (inputStream != &std::cin) {
        // Use the raw FILE* from the ifstream
        // fails compile // yyin = fdopen(fileStream.rdbuf()->fd(), "r");
        //if (!yyin) {
        //    std::cerr << "Failed to convert ifstream to FILE*\n";
        //    return 1;
        //}
        yyin = fopen(argv[1], "r");
		if (!yyin) {
    		std::cerr << "Failed to open input file: " << argv[1] << "\n";
   			return 1;
		}
    }

    return yyparse();
}



