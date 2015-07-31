/* utiltype.c */
Bool TypingLocateChar(char **table, int in, int *row, int *col);
int TypingGetChar(char **table, int row, int col);
Bool TypingCorruptChar(char **table, int in, char *out);
Bool TypingCorruptString(char **table, char *s, char *wrong, char *right, char **s_out);
void TypingAddSpillover(char *s, char *wrong, char *right, char **s_out);
void TypingPutChar(FILE *stream, int c);
void TypingPutString(FILE *stream, char *s);
void TypingPutError(FILE *stream, char *wrong, char *right);
void TypingPrint(FILE *stream, char *s, Discourse *dc);
