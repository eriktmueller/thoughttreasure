/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19981113T134512: begun
 * 19981114T165112: debugged 
 */

#include "tt.h"
#include "repbasic.h"
#include "repfifo.h"
#include "utildbg.h"

/* Buffer */

Buffer *BufferCreate(int bufsize)
{
  Buffer *b;
  b = CREATE(Buffer);
  b->buf = (char *)MemAlloc((size_t)bufsize, "Buffer* buf");
  memset(b->buf, 0, bufsize); /* REDUNDANT */
  b->next = NULL;
  return b;
}

void BufferFree(Buffer *b)
{
  MemFree(b->buf, "Buffer* buf");
  MemFree(b, "Buffer");
}

/* Fifo */

Fifo *FifoCreate(int bufsize)
{
  Fifo *f;
  f = CREATE(Fifo);
  f->bufsize = bufsize;
  if (f->bufsize > 0) {
    f->first = f->last = BufferCreate(bufsize);
  } else {
    f->first = f->last = NULL;
  }
  f->first_removepos = 0;
  f->last_addpos = 0;
  return f;
}

void FifoFree(Fifo *f)
{
  Buffer *b, *n;
  for (b = f->first; b; b = n) {
    n = b->next;
    BufferFree(b);
  }
  MemFree(f, "Fifo");
}

void FifoAddBegin(Fifo *f, /* RESULTS */ char **buf, int *bufsize)
{
  *buf = f->last->buf + f->last_addpos;
  *bufsize = f->bufsize - f->last_addpos;
}

/* Caller is trusted not to add more than we said they could add. */
void FifoAddEnd(Fifo *f, int added)
{
  Buffer *b;

  f->last_addpos += added;
  if (f->last_addpos >= f->bufsize) {
    b = BufferCreate(f->bufsize);
    f->last->next = b;
    f->last = b;
    f->last_addpos = 0;
  }
}

void FifoRemoveBegin(Fifo *f, /* RESULTS */ char **buf, int *bufsize)
{
  *buf = f->first->buf + f->first_removepos;
  if (f->first == f->last) {
    *bufsize = f->last_addpos - f->first_removepos;
  } else {
    *bufsize = f->bufsize - f->first_removepos;
  }
}

/* Caller is trusted not to remove more than we said they could remove. */
void FifoRemoveEnd(Fifo *f, int removed)
{
  Buffer *b;
  f->first_removepos += removed;
  if (f->first_removepos >= f->bufsize) {
    b = f->first;
    f->first = b->next;
    BufferFree(b);
    f->first_removepos = 0;
  }
}

void FifoLookaheadBegin(Fifo *f, /* OUTPUT */ Buffer **first,
                        int *first_removepos)
{
  *first = f->first;
  *first_removepos = f->first_removepos;
}

void FifoLookaheadNext(Fifo *f,
                       /* INPUT AND OUTPUT */
                       char **buf, int *bufsize,
                       Buffer **first, int *first_removepos)
{
  Buffer *b;

  *buf = (*first)->buf + *first_removepos;
  if (*first == f->last) *bufsize = f->last_addpos - *first_removepos;
  else *bufsize = f->bufsize - *first_removepos;

  *first_removepos += *bufsize;
  if (*first_removepos >= f->bufsize) {
    b = *first;
    *first = b->next;
    *first_removepos = 0;
  }
}

Bool FifoIsEmpty(Fifo *f)
{
  return f->first == f->last &&
         f->first_removepos == f->last_addpos;
}

void FifoWrite(Fifo *f, char *s)
{
  int  len, bufsize, siz;
  char *buf;

  len = strlen(s);
  while (len > 0) {
    FifoAddBegin(f, &buf, &bufsize);
    if (len < bufsize) siz = len;
    else siz = bufsize;
    memcpy(buf, s, siz);
    FifoAddEnd(f, siz);
    s += siz;
    len -= siz;
  }
}

Bool FifoIsLineAvailable(Fifo *f)
{
  int    first_removepos, bufsize, i;
  char   *buf;
  Buffer *first;
  FifoLookaheadBegin(f, &first, &first_removepos);
  while (1) {
    FifoLookaheadNext(f, &buf, &bufsize, &first, &first_removepos);
    if (bufsize == 0) return 0;
    for (i = 0; i < bufsize; i++) {
      if (buf[i] == NEWLINE || buf[i] == CR) return 1;
    }
  }
}

/* 1 on success; 0 on failure */
Bool FifoReadLine(Fifo *f, int linelen, /* RESULTS */ char *line)
{
  int  i, bufsize, len;
  char *buf;
  len = 0;
  if (!FifoIsLineAvailable(f)) return 0;
  while (1) {
    FifoRemoveBegin(f, &buf, &bufsize);
    if (bufsize == 0) {
      Dbg(DBGGEN, DBGBAD, "FifoReadLine: 1");
      return 0;
    }
    for (i = 0; i < bufsize; i++) {
      if (buf[i] == NEWLINE || buf[i] == CR) {
        line[len++] = TERM;
        FifoRemoveEnd(f, i+1);
        return 1;
      }
      if (len >= (linelen-1)) {
        /* Out of space: truncate line. */
      } else {
        line[len++] = buf[i];
      }
    }
    FifoRemoveEnd(f, bufsize);
  }
}

/* End of file. */
