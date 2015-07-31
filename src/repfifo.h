/* repfifo.c */
Buffer *BufferCreate(int bufsize);
void BufferFree(Buffer *b);
Fifo *FifoCreate(int bufsize);
void FifoFree(Fifo *f);
void FifoAddBegin(Fifo *f, char **buf, int *bufsize);
void FifoAddEnd(Fifo *f, int added);
void FifoRemoveBegin(Fifo *f, char **buf, int *bufsize);
void FifoRemoveEnd(Fifo *f, int removed);
void FifoLookaheadBegin(Fifo *f, Buffer **first, int *first_removepos);
void FifoLookaheadNext(Fifo *f, char **buf, int *bufsize, Buffer **first, int *first_removepos);
Bool FifoIsEmpty(Fifo *f);
void FifoWrite(Fifo *f, char *s);
Bool FifoIsLineAvailable(Fifo *f);
Bool FifoReadLine(Fifo *f, int linelen, char *line);
