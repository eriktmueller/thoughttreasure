/* toolstat.c */
Float Percent(int numer, int denom);
void StatInit(Stat *s);
void StatNext(Stat *s, long a);
Float StatAvg(Stat *s);
long StatMin(Stat *s);
long StatMax(Stat *s);
void StatLexEntry2(FILE *stream, char *features, Bool is_phrase);
void StatLexEntry1(FILE *stream, char *features);
void StatLexEntry(FILE *stream);
void StatObj1(FILE *stream, Obj *class);
void StatObj(FILE *stream);
void ReportStatistics(FILE *stream);
void StatClassify(FILE *stream);
