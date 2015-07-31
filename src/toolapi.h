/* toolapi.c */
Discourse *API_DiscourseCreate(void);
void API_DiscourseFree(Discourse *dc);
void API_DiscourseSetInput(Discourse *dc, int lang, int dialect, int style, char *s);
int API_Tag(Discourse *dc);
ObjListList *API_SynSemParse(Discourse *dc);
ObjListList *API_SyntacticParse(Discourse *dc);
ObjListList *API_SemanticParse(Discourse *dc);
char *API_Generate(Discourse *dc, int lang, int dialect, int style, int eos, Obj *obj);
char *API_Chatterbot(Discourse *dc, int lang, int dialect, int style, char *s);
