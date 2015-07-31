/* synbase.c */
int POSGI(char c);
char IPOSG(int i);
void BaseRuleInit(void);
void BaseRuleEnter(char a, char b, char z);
void BaseRulePrint(FILE *stream);
void BaseRuleCompile(void);
void BaseRuleCompileA(FILE *stream);
char BaseRuleGetFeature(Obj *obj);
void BaseRuleProcessList(Obj *obj);
