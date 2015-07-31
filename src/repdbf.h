/* repdbf.c */
void DbFileRead(char *filename, int dbfiletype);
void DbFileReadStream(FILE *stream, Obj *rootclass, int dbfiletype);
void DbFileAddIsaPolity(Obj *obj, int level);
Obj *DbFileReadIsaHeader(FILE *stream, Obj *rootclass);
Obj *DbFileReadPolityHeader(FILE *stream, Obj *rootclass);
void DbFileReadIsas(FILE *stream, Obj *obj, Obj *rootclass, int dbfiletype);
int DbFileReadRecord(FILE *stream, Obj *rootclass, int dbfiletype);
void DbFileAssert(Obj *obj, int checkifalreadytrue);
Obj *DbFileReadValueString(FILE *stream);
Obj *DbFileReadValue(FILE *stream);
int DbFileReadSlot(FILE *stream, Obj *obj);
