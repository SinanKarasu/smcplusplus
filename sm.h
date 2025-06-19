#ifndef sm_h
#define sm_h

void GenStateMap();
void SetInitialState(char* initialStateName);
void PushHeader(char * header);
void PushSuperSubState(char * theStateName, char * superStateName);
void PushSuperState(char *theStateName);
void PushSubState(char * theStateName,char *  superStateName);
void PushState(char * theStateName);
void PushTransitionLine(char * trans, char * theStateName);
void PushAction(char * actionName);
#endif // sm_h
