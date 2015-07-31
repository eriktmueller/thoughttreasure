/* syntrans.c */
LexitemList *LexitemListAddPronoun(Lexitem *lexitem, LexitemList *ll);
void TransformGenAddPronoun(Obj *type, int gender, int number, int person, Discourse *dc, LexitemList **pronouns);
Lexitem *TransformPronoun(Lexitem *in, Obj *to_pronoun_class, Discourse *dc);
Bool TransformGenFrenchPPElim(PNode *pn, Discourse *dc, LexitemList **pronouns);
Bool TransformGenFrenchNPElim(PNode *pn, Discourse *dc, LexitemList **pronouns);
PNode *TransformGenPossessive(PNode *pn, Discourse *dc, int *change);
PNode *TransformGenFixEnglishGenitive(PNode *pn, Discourse *dc, int *change);
PNode *TransformGenPrevowel(PNode *pn, Discourse *dc, int *change);
PNode *TransformGenFrenchElision(PNode *pn, Discourse *dc, int *change);
PNode *TransformGenFrenchContractions(PNode *pn, Discourse *dc, int *change);
PNode *TransformGenEnglishAtThere(PNode *pn, Discourse *dc, int *change);
PNode *TransformGenEnglishThatDeletion(PNode *pn, Discourse *dc, int *change);
PNode *TransformGen1(PNode *pn, Discourse *dc, int collect_pronouns, int np_elim, LexitemList **pronouns, int *change);
PNode *TransformGen(PNode *pn, Discourse *dc);
Bool PNodeElimSubject(PNode *pn);
Bool TransformSubordinateSubjectDeletion(int subcat, PNode *subj_np, PNode *obj_np, PNode *subord, Discourse *dc);
