/* taprod.c */
void ModelOfThisOrAnotherBrand(Obj *brand, Obj *model, int *this_brand, Obj **another_brand);
ObjList *TA_ProductFindExisting(Obj *brand, ObjListList *isas, ObjListList *attributes, Ts *ts_created);
ObjList *TA_ProductLearnNew(Obj *brand, ObjListList *isas, ObjListList *attributes, ObjList *new_isas, int year, Ts *ts_created, char *input_text, Discourse *dc);
Bool TA_ProductElement1(Obj *brand, char *in, char *in_base, Obj *class, int numwords, int is_fwd, Discourse *dc, ObjList **isas, ObjList **attributes, ObjList **new_isas, int *year, char *punc, char **nextp);
Bool TA_ProductElement(Obj *brand, char *in, char *in_base, Obj *class, int is_fwd, Discourse *dc, ObjList **isas, ObjList **attributes, ObjList **new_isas, int *year, char *punc, char **nextp);
Bool TA_Product(char *in, char *in_base, Discourse *dc, Channel *ch, char **nextp);
void TA_ProductTrade(PNode *pns);
Bool TA_Price(char *in, char *in_base, Discourse *dc, Channel *ch, char **nextp);
Bool TA_Telno(char *in, Discourse *dc, Channel *ch, char **nextp);
Bool TA_MediaObject(char *in, char *in_base, Discourse *dc, Channel *ch, char **nextp);
