/* semcase.c */
CaseFrame *CaseFrameAdd(CaseFrame *next, Obj *cas, Obj *concept, Float score, PNode *pn, Anaphor *anaphors, LexEntryToObj *leo);
CaseFrame *CaseFrameAddSP(CaseFrame *next, Obj *cas, Obj *concept, SP *sp);
CaseFrame *CaseFrameRemove(CaseFrame *cf);
void CaseFrameThetaMarkClear(CaseFrame *cf);
Obj *CaseFrameGet(CaseFrame *cf, Obj *cas, SP *sp);
int CaseFrameIsClause(PNode *z);
int CaseFrameSubcatOK(CaseFrame *cf, Obj *cas, int subcat);
Obj *CaseFrameGetLe(CaseFrame *cf, Obj *cas, LexEntry *le, int subcat, SP *sp);
Obj *CaseFrameGetClass(CaseFrame *cf, Obj *cas, Obj *class, SP *sp);
Obj *CaseFrameGetNonThetaMarked(CaseFrame *cf, Obj *cas, SP *sp);
Obj *CaseFrameGetLe_DoNotMark(CaseFrame *cf, Obj *cas, LexEntry *le, SP *sp);
Bool CaseFrameThetaMarkExpletive(CaseFrame *cf, LexEntry *le);
void CaseFramePrint1(FILE *stream, CaseFrame *cf);
void CaseFramePrint(FILE *stream, CaseFrame *cf);
Bool CaseFrameDisobeysThetaCriterion(CaseFrame *cf, Obj *obj);
