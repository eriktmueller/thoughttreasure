/* semstyle.c */
ObjList *StyleTemporalTreeCreate(void);
ObjList *StyleFindContainingNode(ObjList *objs, TsRange *tsr_con);
void StyleTemporalTreeAdd(ObjList *tree, Obj *con);
void StyleTemporalTreePrint2(FILE *stream, Obj *obj);
void StyleTemporalTreePrint1(FILE *stream, ObjList *children, Obj *parent);
void StyleTemporalTreePrint(FILE *stream, ObjList *tree);
ObjList *StyleMergeAnds(ObjList *in);
void StyleListToTemporalTree(ObjList *in, ObjList **r_tree, ObjList **r_generality, ObjList **r_unterminated);
void StyleTemporalTreeResort(ObjList *tree);
ObjList *StyleReorganize1(Obj *parent, ObjList *children, ObjList *r, Discourse *dc);
ObjList *StyleReorganize(ObjList *in, Discourse *dc);
