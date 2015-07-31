/* uaasker.c */
Question *QuestionCreate(Obj *qclass, Float importance, Obj *asking_ua, Obj *answer_class, ObjList *preface, Obj *elided_question, Obj *full_question, Obj *full_question_var, Intension *itn, Question *next);
void QuestionPrint(FILE *stream, Question *q);
void QuestionPrintAll(FILE *stream, Question *questions);
Question *QuestionMostImportant(Question *questions);
void UA_AskerFindBestCut(Intension *itn, ObjList *selected, ObjList **isas_r, ObjList **attrs_r);
Bool UA_AskerNarrowDown(Context *cx, Float importance, Obj *asking_ua, Intension *itn, ObjList *selected, Obj *full_question_var, int elided_ok);
ObjList *UA_AskerSelect(Context *cx, Obj *obj, Intension **itn_r);
void UA_AskerQWQ(Context *cx, Float importance, Obj *asking_ua, Obj *answer_class, Obj *elided_question, Obj *full_question, Obj *full_question_var);
void UA_Asker(Context *cx, Discourse *dc);
Obj *UA_AskerInterpretElided(Context *parent_cx, Obj *elided);
void UA_AskerAskQuestions(Discourse *dc);
