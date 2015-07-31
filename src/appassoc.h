/* appassoc.c */
Obj *AssocObjectPick1(ObjList *objs, int i, ObjList *visited, int concrete_only);
Obj *AssocObjectPick(ObjList *objs, int i, ObjList *visited);
Obj *AssocObjectNextObject(Obj *from, ObjList *visited);
Obj *AssocObjectRandomObject(void);
ObjList *AssocObject(Discourse *dc, Obj *con, int maxlen);
int AssocAssertion2(Obj *obj1, Obj *obj2, Obj *class, ObjList *r, int depth, int maxdepth, ObjList **out);
int AssocAssertion1(Obj *obj1, Obj *obj2, Obj *class, ObjList *r, int depth, int maxdepth, ObjList **out);
ObjList *AssocAssertion(Obj *obj1, Obj *obj2, Obj *class);
