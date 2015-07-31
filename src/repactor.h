/* repactor.c */
void AntecedentInit(Antecedent *an);
void AntecedentPrint(FILE *stream, int i, Antecedent *an);
void AntecedentDecay(Antecedent *an);
void AntecedentRefresh(Antecedent *an, Obj *obj, PNode *pn, PNode *pn_top, int gender, int number, int person);
Actor *ActorCreate(Obj *actor, Context *cx, Actor *next);
Actor *ActorCopy(Actor *ac_parent, Context *cx_parent, Context *cx_child, Actor *next);
Actor *ActorCopyAll(Actor *ac_parent, Context *cx_parent, Context *cx_child);
ObjList *ActorFindSubgoalsHead(Actor *ac, Obj *head_class);
void ActorPrint(FILE *stream, Actor *ac);
void ActorPrintAll(FILE *stream, Actor *actors);
