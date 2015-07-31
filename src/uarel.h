/* uarel.c */
int UA_Relation_TsDestructive(Ts *existing, Ts *new, Context *cx);
void UA_RelationIdentical012(Obj *existing, Obj *new, Context *cx, int *assertnew);
void UA_RelationSuccession(ObjList *existing, Obj *new, Context *cx);
void UA_RelationSpecification(Obj *old, Obj *new, Context *cx);
void UA_Relation2(Obj *new, Obj *reltype, Context *cx);
void UA_Relation1(Context *cx, Ts *ts, Obj *in);
void UA_Relation(Context *cx, Ts *ts, Obj *in);
