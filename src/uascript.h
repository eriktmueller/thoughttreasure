/* uascript.c */
Bool IsScript(Obj *obj);
ObjList *ScriptGetAll(void);
ObjListList *ScriptGetAllWithConcepts(void);
ObjList *ScriptSimilarityAssertion(Obj *script, Obj *rel, ObjList *cons);
ObjList *ScriptSimilarityRelation(Obj *script, Obj *rel, ObjList *cons);
ObjList *ScriptSimilarityConcepts(Obj *script);
Float ScriptSimilarity(FILE *stream, Obj *script, ObjList *cons, Obj *con);
void ScriptPrintPolysemous(FILE *stream, int lang);
