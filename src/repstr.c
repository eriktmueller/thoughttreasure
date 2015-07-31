/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19981022T083324: added StringGetLeNonwhite
 */

#include "tt.h"
#include "repbasic.h"
#include "repstr.h"
#include "repdb.h"
#include "repobj.h"
#include "repobjl.h"
#include "repspace.h"
#include "reptext.h"
#include "reptime.h"
#include "utilchar.h"
#include "utildbg.h"

int StringGenNext;

/* String indexing with bounds checking. */
char *StringIth(char *s, int i)
{
  return(s + IntMin(strlen(s), i));
}

void StringInit()
{
  StringGenNext = 0;
}

char *StringCopy(char *s, char *typ)
{
  char	*r;

  r = (char *)MemAlloc(1+strlen(s), typ);
  strcpy(r, s);
  return(r);
}

/* strncpy is not the same. It pads, which is too risky. */
void StringCpy(/* RESULTS */ char *dest, /* INPUT */ char *src, int maxlen)
{
  maxlen--;
  while (*src && maxlen-- > 0) {
    *dest++ = *src++;
  }
  *dest = TERM;
#ifdef maxchecking
  if (*src) {
    Dbg(DBGGEN, DBGDETAIL, "StringCpy <%.20s>", src);
  }
#endif
}

/* strncat is not the same. The size argument is the size of <src>,
 * not <dest>.
 */
void StringCat(/* RESULTS */ char *dest, /* INPUT */ char *src, int maxlen)
{
  maxlen--;
  while (*dest) {
    maxlen--;
    dest++;
  }
  while (*src && maxlen-- > 0) {
    *dest++ = *src++;
  }
  *dest = TERM;
#ifdef maxchecking
  if (*src) {
    Dbg(DBGGEN, DBGDETAIL, "StringCat <%.20s> <%.20s>", dest, src);
  }
#endif
}

void StringReverse(char *in, /* RESULTS */ char *out)
{
  int	i;
  for (i = strlen(in)-1; i >= 0; i--) {
    *out++ = in[i];
  }
  *out = TERM;
}

char *StringCopyLen(char *s, int len, char *typ)
{
  char	*r;

  r = (char *)MemAlloc(len+1, typ);
  memcpy(r, s, (size_t)len);
  r[len] = TERM;
  return(r);
}

char *StringCopyLenLong(char *s, size_t len, char *typ)
{
  char	*r;

  r = (char *)MemAlloc(len+1, typ);
  memcpy(r, s, len);
  r[len] = TERM;
  return(r);
}

/* <s1> is the head of <s2> */
Bool StringHeadEqual(char *s1, char *s2)
{
  int	len;
  if (s1[0] != s2[0]) return(0);
  len = strlen(s1);
  if (strlen(s2) >= len) {
    return(0 == strncmp(s1, s2, len));
  } else return(0);
}

Bool StringHeadEqualAdvance(char *s1, char *s2, /* RESULTS */ char **nextp)
{
  int	len;
  if (s1[0] != s2[0]) return(0);
  len = strlen(s1);
  if (strlen(s2) >= len) {
    if (0 == strncmp(s1, s2, len)) {
      *nextp = s2 + len;
      return(1);
    }
  }
  return(0);
}

/* If successful, <nextp> points to after <stopstring>.
 * <out> does not include <stopstring>.
 */
Bool StringReadUntilString(char *in, char *stopstring, int maxlen,
                           /* RESULTS */ char *out, char **nextp)
{
  int	i;
  i = 0;
  while (1) {
    if (StringHeadEqual(stopstring, in)) {
      out[i] = TERM;
      *nextp = in + strlen(stopstring);
      return(1);
    }
    if (i >= (maxlen-1)) return(0);
    out[i] = *in;
    i++;
    in++;
  }
}

Bool StringLocate(char *in, char *stopstring,
                  /* RESULTS */ char **start, char **after)
{
  while (in[0]) {
    if (StringHeadEqual(stopstring, in)) {
      *start = in;
      *after = in + strlen(stopstring);
      return 1;
    }
    in++;
  }
  return 0;
}

Bool StringLocateFollowedByDigit(char *in, char *stopstring,
                                 /* RESULTS */ char **start, char **after)
{
  char *p;
  while (in[0]) {
    if (StringHeadEqual(stopstring, in)) {
      p = in + strlen(stopstring);
      if (Char_isdigit(*p)) {
        *start = in;
        *after = p;
        return 1;
      }
    }
    in++;
  }
  return 0;
}

Bool StringIn(register char c, register char *s)
{
  while (*s) if (c == *s++) return(1);
  return(0);
}

int StringCountOccurrences(char *s, int c)
{
  int	cnt;
  cnt = 0;
  while (*s) {
    if (c == *((uc *)s)) cnt++;
    s++;
  }
  return(cnt);
}

Bool StringInWithin(register char c, register int within, register char *s)
{
  int	i;
  i = 0;
  while (*s) {
    if (i >= within) return(0);
    if (c == *s++) return(1);
    i++;
  }
  return(0);
}

Bool StringLineIsAll(char *s, char *set, /* RESULTS */ char **next_line)
{
  while (*s) {
    if (*s == NEWLINE) {
      *next_line = s+1;
      return(1);
    }
    if (!StringIn(*s, set)) return(0);
    s++;
  }
  /* Hit EOF; we'll accept. */
  *next_line = s;
  return(1);
}

Bool StringAnyIn(char *s1, char *s2)
{
  if (s1[0] == TERM) return(1);
  while (*s1) {
    if (StringIn(*s1, s2)) return(1);
    s1++;
  }
  return(0);
}

Bool StringAllIn(char *s1, char *s2)
{
  while (*s1) {
    if (!StringIn(*s1, s2)) return(0);
    s1++;
  }
  return(1);
}

Bool StringAllEqual(char *s, char c)
{
  while (*s) {
    if (*s != c) return(0);
    s++;
  }
  return(1);
}

char *StringNthTail(char *s, int n)
{
  int	len;
  len = strlen(s);
  if (n >= len) return(s);
  else return(s+len-n);
}

Bool StringTailEq(char *s, char *tail)
{
  int lens, lentail;
  lens = strlen(s);
  lentail = strlen(tail);
  return((lens >= lentail) && streq(s+lens-lentail, tail));
}

Bool StringTailEqual(char *s, char *tail, /* RESULTS */ char *stem)
{
  int lens, lentail;
  lens = strlen(s);
  lentail = strlen(tail);
  if ((lens >= lentail) && streq(s+lens-lentail, tail)) {
    memcpy(stem, s, (size_t)(lens - lentail));
    stem[lens - lentail] = TERM;
    return(1);
  } else {
    return(0);
  }
}

/* Returned location is after separator (unless end of string reached).
 * Result doesn't include separator.
 */
char *StringReadTo(char *in, /* RESULTS */ char *out, /* INPUT */ int maxlen,
                   int sep1, int sep2, int sep3)
{
  char *p;
  p = out;
  maxlen--;
  while (*in && ((uc)*in) != ((uc)sep1) &&
         ((uc)*in) != ((uc)sep2) && ((uc)*in) != ((uc)sep3)) {
    if (*in == TREE_ESCAPE) {
      *p = *in;
      p++; in++;
      if (in[0] == TERM) {
        Dbg(DBGGEN, DBGBAD, "StreamReadTo: empty escape");
        break;
      }
    }
    if ((p - out) >= maxlen) {
      out[maxlen] = TERM;
      Dbg(DBGGEN, DBGBAD,
          "StreamReadTo: overflow reading up to <%c> <%c> <%c>: <%s>",
          sep1, sep2, sep3, out);
      return(in);
    }
    *p = *in;
    p++; in++;
  }
  *p = TERM;
  if (*in != TERM) in++;
  return(in);
}

/* Returned location is after separator (unless end of string reached).
 * Result doesn't include separator.
 */
Bool StringReadToRequireTerm(char *in, int sep1, int sep2, int maxlen,
                            /* RESULTS */ char *out, char **nextp)
{
  char *p;
  p = out;
  maxlen--;
  while (*in && ((uc)*in) != ((uc)sep1) && ((uc)*in) != ((uc)sep2)) {
    if (*in == TREE_ESCAPE) {
      *p = *in;
      p++; in++;
      if (in[0] == TERM) {
        Dbg(DBGGEN, DBGBAD, "StringReadToRequireTerm: empty escape");
        break;
      }
    }
    if ((p - out) >= maxlen) {
      return(0);
    }
    *p = *in;
    p++; in++;
  }
  *p = TERM;
  if (*in == TERM) return(0);
  in++;
  *nextp = in;
  return(1);
}

/* Returns pointing after <sep>. */
char *StringReadTo1(char *in, char *out, int maxlen, int sep,
                    /* RESULTS */ int *sep_found)
{
  char *p;
  p = out;
  maxlen--;
  while (*in && *in != sep) {
    if (*in == TREE_ESCAPE) {
      *p = *in;
      p++; in++;
      if (in[0] == TERM) {
        Dbg(DBGGEN, DBGBAD, "StreamReadTo1: empty escape");
        break;
      }
    }
    if ((p - out) >= maxlen) {
      out[maxlen] = TERM;
      Dbg(DBGGEN, DBGBAD, "StreamReadTo1: overflow reading up to <%c>: <%s>",
          sep, out);
      return(in);
    }
    *p = *in;
    p++; in++;
  }
  if (sep_found) *sep_found = (*in == sep);
  *p = TERM;
  if (*in != TERM) in++;
  return(in);
}

char *StringReadTos(char *in, int including, char *out, int maxlen, char *seps)
{
  char *p;
  p = out;
  maxlen--;
  while (*in && !StringIn(*in, seps)) {
    if (*in == TREE_ESCAPE) {
      *p = *in;
      p++; in++;
      if (in[0] == TERM) {
        Dbg(DBGGEN, DBGBAD, "StringReadTos: empty escape");
        break;
      }
    }
    if ((p - out) >= maxlen) {
      out[maxlen] = TERM;
      Dbg(DBGGEN, DBGBAD, "StringReadTos: overflow reading up to <%s>: <%s>",
          seps, out);
      return(in);
    }
    *p = *in;
    p++; in++;
  }
  *p = TERM;
  if (including && (*in != TERM)) in++;
  return(in);
}

char *StringReadWord(char *in, int word_len, /* RESULT */ char *word)
{
  in = StringSkipWhitespace(in);
  in = StringReadTos(in, 1, word, word_len, WHITESPACE);
  in = StringSkipWhitespace(in);
  return(in);
}

char *StringReadToNons(char *in, int including, char *out, int maxlen,
                       char *seps)
{
  char *p;
  p = out;
  maxlen--;
  while (*in && StringIn(*in, seps)) {
    if (*in == TREE_ESCAPE) {
      *p = *in;
      p++; in++;
      if (in[0] == TERM) {
        Dbg(DBGGEN, DBGBAD, "StringReadToNons: empty escape");
        break;
      }
    }
    if ((p - out) >= maxlen) {
      out[maxlen] = TERM;
      Dbg(DBGGEN, DBGBAD, "StringReadToNons: overflow reading up to <%s>: <%s>",
          seps, out);
      return(in);
    }
    *p = *in;
    p++; in++;
  }
  *p = TERM;
  if (including && (*in != TERM)) in++;
  return(in);
}

/* Newlines may not be inside the parenthetical expression.
 * Returns pointer to after the closing paren.
 */
char *StringSkipParenthetical(char *s)
{
  char	*p;
  int	indent;
  p = s;
  if (p[0] == LPAREN) {
    indent = 1;
    p++;
    while (*p && (*p != NEWLINE)) {
      if (p[0] == LPAREN) {
        indent++;
      } else if (p[0] == RPAREN) {
        indent--;
        if (indent <= 0) return(p+1);
      }
      p++;
    }
  }
  return(s);
}

Bool StringSkipChar(char *s, int c, /* RESULTS */ char **nextp)
{
  if (*s != c) return(0);
  *nextp = s+1;
  return(1);
}

/* Returns pointer to after separator. */
char *StringSkipTo(char *in, int sep1, int sep2, int sep3)
{
  while (*in && *in != sep1 && *in != sep2 && *in != sep3) {
    if (*in == TREE_ESCAPE) {
      in++;
      if (*in == TERM) {
        Dbg(DBGGEN, DBGBAD, "StringSkipTo");
        return(in);
      }
    }
    if (*in == TERM) {
      Dbg(DBGGEN, DBGBAD, "StringSkipTo: hit end skipping to <%c> <%c> <%c>",
          sep1, sep2, sep3);
      return(in);
    }
    in++;
  }
  if (*in != TERM) in++;
  return(in);
}

/* Result points to after character. */
Bool StringSkipToAcrossNewlines(char *in, int c, int maxnewlines,
                                /* RESULTS */ char **nextp)
{
  int	newlines;
  newlines = 0;
  while (1) {
    if (*in == c) {
      *nextp = in+1;
      return(1);
    }
    if (*in == TERM) return(0);
    if (*in == NEWLINE) {
      newlines++;
      if (newlines > maxnewlines) return(0);
    }
    in++;
  }
}

/* Returns pointer to after separator. */
char *StringSkipTos(char *in, char *seps)
{
  while (*in && !StringIn(*in, seps)) {
    if (*in == TREE_ESCAPE) {
      in++;
      if (*in == TERM) {
        Dbg(DBGGEN, DBGBAD, "StringSkipTos");
        return(in);
      }
    }
    if (*in == TERM) {
      Dbg(DBGGEN, DBGBAD, "StringSkipTos: hit end skipping to <%s>", seps);
      return(in);
    }
    in++;
  }
  if (*in != TERM) in++;
  return(in);
}

/* Returns pointer to first non separator character. */
char *StringSkipToNons(char *in, char *seps)
{
  while (*in && StringIn(*in, seps)) {
    if (*in == TREE_ESCAPE) {
      in++;
      if (*in == TERM) {
        Dbg(DBGGEN, DBGBAD, "StringSkipNons");
        return(in);
      }
    }
    if (*in == TERM) {
      Dbg(DBGGEN, DBGBAD, "StringSkipNons: hit end skipping to <%s>", seps);
      return(in);
    }
    in++;
  }
  return(in);
}

char *StringSkipWhitespaceNonNewline(char *in)
{
  while (*in && IsWhitespace(*in) && *in != NEWLINE) in++;
  return(in);
}

char *StringSkipWhitespace(char *in)
{
  while (*in && IsWhitespace(*in)) in++;
  return(in);
}

char *StringSkipspace(char *in)
{
  while (*in && (*in == SPACE)) in++;
  return(in);
}

Bool StringIsCharInLine(char *s, int c)
{
  while (*s) {
    if (*s == NEWLINE) return(0);
    if (*s == c) return(1);
    s++;
  }
  return(0);
}

void StringAppendDots(/* RESULTS */ char *s1, /* INPUT */ char *s2, int maxlen)
{
  int	len1, len2;
  len1 = strlen(s1);
  len2 = strlen(s1);
  if ((len1 + len2 + 1) > maxlen) {
    if ((len1 + 4) <= maxlen) {
      strcat(s1, "...");
    }
  } else {
    strcat(s1, s2);
  }
}

void StringAppendLen(/* RESULTS */ char *s1, /* INPUT */ char *s2, int len)
{
  s1 = StringEndOf(s1);
  while (len > 0) {
    *s1 = *s2;
    s1++;
    s2++;
    len--;
  }
  *s1 = TERM;
}

void StringAppendChar(/* RESULTS */ char *r, /* INPUT */ int maxlen, int c)
{
  int	len;
  len = strlen(r);
  if (len >= (maxlen-1)) {
    Dbg(DBGGEN, DBGBAD, "StringAppendChar: maxlen");
    return;
  }
  r[len] = c;
  r[len+1] = TERM;
}

void StringPrependChar(/* RESULTS */ char *r, /* INPUT */ int maxlen, int c)
{
  int	len;
  len = strlen(r);
  if (len >= (maxlen-1)) {
    Dbg(DBGGEN, DBGBAD, "StringPrependChar: maxlen");
    return;
  }
#ifdef SUNOS
  bcopy(r, r+1, len);
#else
  memmove(r+1, r, len);
#endif
  r[0] = c;
  r[len+1] = TERM;
}

int StringLastChar(char *s)
{
  if (*s) return(s[strlen(s)-1]);
  else return(TERM);
}

char *StringEndOf(char *s)
{
  while (*s) s++;
  return(s);
}

void StringCopyExcept(char *in, int except, int maxlen, /* RESULTS */ char *out)
{
  char	*orig_out;
  orig_out = out;
  maxlen--;
  while (*in) {
    if ((unsigned char)*in != except) {
      if ((out - orig_out) >= maxlen) {
        orig_out[maxlen] = TERM;
        return;
      }
      *out = *in;
      out++;
    }
    in++;
  }
  *out = TERM;
}

void StringCopyExcept2(char *in, int except1, int except2, int maxlen,
                      /* RESULTS */ char *out)
{
  char	*orig_out;
  orig_out = out;
  maxlen--;
  while (*in) {
    if ((unsigned char)*in != except1 && (unsigned char)*in != except2) {
      if ((out - orig_out) >= maxlen) {
        orig_out[maxlen] = TERM;
        return;
      }
      *out = *in;
      out++;
    }
    in++;
  }
  *out = TERM;
}

void StringElimChar(char *to, int c)
{
  char *from;
  from = to;
  while (*from) {
    if (*((uc *)from) != c) {
      *to = *from;
      to++;
      from++;
    } else {
      from++;
    }
  }
  *to = TERM;
}

void StringElims(char *to, char *set, /* RESULTS */ int *mods)
{
  char *from;
  from = to;
  if (mods) *mods = 0;
  while (*from) {
    if (!StringIn(*from, set)) {
      *to = *from;
      to++;
      from++;
    } else {
      if (mods) *mods = 1;
      from++;
    }
  }
  *to = TERM;
}

void StringElimNonAlphanumeric(char *to)
{
  char *from;
  from = to;
  while (*from) {
    if (CharIsAlphanumeric(*from)) {
      *to = *from;
      to++;
      from++;
    } else {
      from++;
    }
  }
  *to = TERM;
}

Float StringPercentAlpha(char *s)
{
  int	numer, denom;
  numer = denom = 0;
  while (*s) {
    if (CharIsAlpha(*((uc *)s))) numer++;
    denom++;
    s++;
  }
  if (denom == 0) return(0.0);
  else return(100.0*(((Float)numer)/((Float)denom)));
}

void StringMapChar(char *to, int from_c, int to_c)
{
  while (*to) {
    if (*to == from_c) *to = to_c;
    to++;
  }
}

void StringMap(char *to, char *from_map, char *to_map)
{
  int	c;
  while (*to) {
    if ((c = to_map[StringFirstLoc(*to, from_map)])) {
      *to = c;
    }
    to++;
  }
}

void StringMapLeWhitespaceToSpace(char *to)
{
  while (*to) {
    if (StringIn(*to, LE_WHITESPACE)) *to = SPACE;
    to++;
  }
}

void StringMapLeWhitespaceToDash(char *to)
{
  while (*to) {
    if (StringIn(*to, LE_WHITESPACE)) *to = '-';
    to++;
  }
}

void StringElimTrailingBlanks(/* RESULTS */ char *s)
{
  int	i, len;
  len = strlen(s);
  for (i = len-1; i > 0; i--) {
    if (s[i] != SPACE) break;
    s[i] = TERM;
  }
}

void StringElimLeadingBlanks(/* RESULTS */ char *s)
{
  char	*p;
  p = s;
  while (*p == SPACE) p++;
  while (*p) {
    *s = *p;
    s++;
    p++;
  }
  *s = TERM;
}

void StringAppendIfNotAlreadyIns(char *in, int maxlen, /* RESULTS */ char *out)
{
  char	*p;
  maxlen--;
  p = StringEndOf(out);
  while (*in) {
    if (!StringIn(*in, out)) {
      if ((p - out) >= maxlen) {
        out[maxlen] = TERM;
        return;
      }
      *p = *in;
      p++;
      *p = TERM;
    }
    in++;
  }
  *p = TERM;
}

void StringAppendIfNotAlreadyIn(int in, int maxlen, /* RESULTS */ char *out)
{
  char	*p;
  maxlen--;
  p = StringEndOf(out);
  if (!StringIn(in, out)) {
    if ((p - out) >= maxlen) { out[maxlen] = TERM; return; }
    p[0] = in;
    p[1] = TERM;
  }
}

#ifdef notdef
void StringUnion(char *in1, char *in2, int maxlen, /* RESULTS */ char *out)
{
  int	i;
  char	*orig_out, 
  orig_out = out;
  maxlen--;
  for (i = 1; i < 256; i++) {
    if (StringIn(i, in1) || StringIn(i, in2)) {
      if ((out - orig_out) >= maxlen) {
        orig_out[maxlen] = TERM;
        return;
      }
      *out = i;
      out++;
    }
  }
  *out = TERM;
}
#endif

void StringIntersect(char *in1, char *in2, /* RESULTS */ char *out)
{
  while (*in1) {
    if (StringIn(*in1, in2)) {
      *out = *in1;
      out++;
    }
    in1++;
  }
  *out = TERM;
}

char *StringReadFile(char *fn, int useroot)
{
  char        *s;
  size_t      cnt;
  FILE        *stream;
  struct stat statrec;
  char fn1[FILENAMELEN];

  if (useroot) {
    sprintf(fn1, "%s/%s", TTRoot, fn);
    fn = fn1;
  }

  if (0 > stat(fn, &statrec)) {
    Dbg(DBGGEN, DBGBAD, "Trouble opening %s\n", fn);
    return(NULL);
  }
  if (NULL == (stream = StreamOpen(fn, "r"))) return(NULL);
  s = (char *)MemAlloc(statrec.st_size+1L, "char StringReadFile");
  cnt = fread(s, 1L, statrec.st_size, stream);
#ifdef notdef
  /* Some kind of newline problem on PCs? */
  if (cnt < statrec.st_size) {
    Dbg(DBGGEN, DBGBAD, "StringReadFile: %ld < %ld", cnt, statrec.st_size);
    MemFree(s, "char StringReadFile");
    fclose(stream);
    return(NULL);
  }
#endif
  s[statrec.st_size] = TERM;
  fclose(stream);
#ifdef MACOS
  if (StringTailEq(fn, ".iso")) {
    /* todo: Need to add this to ChannelReadBatch too. */
    StringISO_8859_1ToMacDestructive(s);
  }
#endif
  return(s);
}

char *StringReadUntil(FILE *stream, char *term)
{
  char *r, line[LINELEN];
  int len, maxlen, linelen, normalterm;
  maxlen = 256;
  r = (char *)MemAlloc(maxlen, "char StringReadUntil");
  len = 0;
  normalterm = 0;
  while (fgets(line, LINELEN, stream)) {
    if (term && streq(line, term)) {
      normalterm = 1;
      break;
    }
    linelen = strlen(line);
    if ((len + linelen + 1) > maxlen) {
      maxlen = maxlen*2;
      r = (char *)MemRealloc(r, maxlen, "char StringReadUntil");
    }
    memcpy(r + len, line, (size_t)linelen);
    len += linelen;
  }
  if (term && !normalterm) {
    Dbg(DBGGEN, DBGBAD, "StringReadUntil");
    /* todo: recovery? */
  }
  r = (char *)MemRealloc(r, len+1, "char StringReadUntil");
  r[len] = TERM;
  return(r);
}

void StringSearchReplace(char *s, char *from, char *to)
{
  int len, fromlen, tolen, i;
  char *found;
  fromlen = strlen(from);
  tolen = strlen(to);
  while (NULL != (found = strstr(s, from))) {
    i = found-s;
    len = strlen(s);
#ifdef SUNOS
    bcopy(s+i+fromlen, s+i+tolen, 1+len-(i+fromlen));
    bcopy(to, s+i, tolen);
#else
    memmove(s+i+tolen, s+i+fromlen, 1+len-(i+fromlen));
    memmove(s+i, to, tolen);
#endif
  }
}

void StringToLowerNoAccentsDest(char *s)
{
  while (*s) {
    *s = CharToLowerNoAccents((uc)(*s));
    s++;
  }
}

void StringISO_8859_1ToMacDestructive(char *s)
{
  while (*s) {
    *s = CharsetISO_8859_1ToMac((uc)(*s));
    s++;
  }
}

void StringToLowerNoAccents(char *in, int maxlen, /* RESULTS */ char *out)
{
  char	*orig_out;
  orig_out = out;
  maxlen--;
  while (*in) {
    if ((out - orig_out) >= maxlen) {
      orig_out[maxlen] = TERM;
      return;
    }
    *out = CharToLowerNoAccents((uc)(*in));
    in++;
    out++;
  }
  *out = TERM;
}

void StringDevoice(char *s)
{
  while (*s) {
    *s = CharDevoice((uc)(*s));
    s++;
  }
}

/* Levels of inflection match relaxation:
 * (0) exact match
 * (1) insensitive to case (StringToLowerDestructive) +
 *     insensitive to embedded punctuation characters, especially "."
 *     (StringElimNonAlphanumeric) 
 * (2) above + insensitive to accents (StringToLowerNoAccentsDest)
 * (3) above + eliminate embedded punctuation (StringElimPunctDest)
 * (4) above + eliminate duplicate letters (StringElimDupDest)
 * (5) above + reduce voiced consonants to unvoiced,
 *     "ph" => "f", "tch" => "ch", ... (StringReduceConsonDest)
 * (6) above + reduce all vowels (including "y") to "e" (StringReduceVowelDest)
 *     eliminate "h"
 * (7-12) the above in the other language
 * todo: Eliminate trailing vowels for French.
 */
void StringReduceTotal(char *out)
{
  /* todo: This could be reimplemented in a single pass, though all the
   * subroutines need to be kept, for retrieval.
   */
  StringElimNonAlphanumeric(out);
  StringToLowerNoAccentsDest(out);
  StringElimPunctDest(out);
  StringElimDupDest(out);
  StringReduceConsonDest(out);
  StringReduceVowelDest(out);
  StringElimDupDest(out);
}

void StringReduce6(char *out)
{
  StringReduceTotal(out);
}

void StringReduce5(char *out)
{
  StringToLowerNoAccentsDest(out);
  StringElimPunctDest(out);
  StringElimDupDest(out);
  StringReduceConsonDest(out);
  StringElimDupDest(out);
}

void StringReduce4(char *out)
{
  StringToLowerNoAccentsDest(out);
  StringElimPunctDest(out);
  StringElimDupDest(out);
}

void StringReduce3(char *out)
{
  StringToLowerNoAccentsDest(out);
  StringElimPunctDest(out);
}

void StringReduce2(char *out)
{
  StringToLowerNoAccentsDest(out);
}

void StringReduce1(char *out)
{
  StringElimNonAlphanumeric(out);
    /* todo: Why is this here, when it is also in StringReduce3 as
     * StringElimPunctDest?
     */
  StringToLowerDestructive(out);
}

void StringElimPunctDest(char *to)
{
  StringElims(to, ".&", NULL);
}

void StringElimDupDest(char *to)
{
  int	prev;
  char	*from;
  prev = TERM;
  from = to;
  while (*from) {
    if (*from != prev) {
      prev = *to = *from;
      to++;
      from++;
    } else {
      from++;
    }
  }
  *to = TERM;
}

void StringReduceConsonDest(char *to)
{
  StringDevoice(to);
  StringSearchReplace(to, "tch", "ch");
  StringSearchReplace(to, "ph", "f");
  /* todo: Add more transformations here. */
}

void StringReduceVowelDest(char *s)
{
  while (*s) {
    *s = CharReduceVowel((uc)(*s));
    s++;
  }
  StringElimChar(s, 'h');
}

void StringToLowerDestructive(char *s)
{
  while (*s) {
    *s = CharToLower((uc)(*s));
    s++;
  }
}

void StringToLower(char *in, int maxlen, /* RESULTS */ char *out)
{
  char	*orig_out;
  orig_out = out;
  maxlen--;
  while (*in) {
    if ((out - orig_out) >= maxlen) {
      orig_out[maxlen] = TERM;
      return;
    }
    *out = CharToLower((uc)(*in));
    in++;
    out++;
  }
  *out = TERM;
}

void StringToUpper(char *in, int maxlen, /* RESULTS */ char *out)
{
  char	*orig_out;
  orig_out = out;
  maxlen--;
  while (*in) {
    if ((out - orig_out) >= maxlen) {
      orig_out[maxlen] = TERM;
      return;
    }
    *out = CharToUpper((uc)(*in));
    in++;
    out++;
  }
  *out = TERM;
}

void StringToUpperDestructive(char *out)
{
  while (*out) {
    *out = CharToUpper((uc)(*out));
    out++;
  }
}

Bool StringIsRomanNumeral(char *s)
{
  return(StringAllIn(s, "IVXLCDM"));
}

Bool StringIsAllUpper(char *s)
{
  while (*s) {
    if (!CharIsUpper(*((uc *)s))) return(0);
    s++;
  }
  return(1);
}

/* Example:
 * if s = "GARNIER", it is converted to "Garnier"
 * if s = "GaRNIER" or "garnier", it is left untouched.
 */
void StringAllUpperToUL(char *s)
{
  if (StringIsAllUpper(s)) {
    StringToLowerDestructive(s+1);
  }
}

/* Example:
 * if s = "GARNIER", it is converted to "garnier"
 * if s = "GaRNIER" or "Garnier", it is left untouched.
 */
void StringAllUpperToLower(char *s)
{
  if (StringIsAllUpper(s)) {
    StringToLowerDestructive(s);
  }
}

Bool CharPhoneMatch(int ptn, int c)
{
  switch (ptn) {
    case 'X': return(StringIn(c, "0123456789"));
    case 'N': return(StringIn(c, "23456789"));
    case '=': return(StringIn(c, PHONE_WHITESPACE));
    default:
      return(ptn == c);
  }
}

Bool StringPhoneMatch(char *ptn, char *s, /* RESULTS */ char *digits,
                      char **nextp)
{
  while (*ptn) {
    if (!CharPhoneMatch(*ptn, *s)) return(0);
    if (StringIn(*s, "0123456789")) *digits++ = *s;
    ptn++;
    s++;
  }
  *digits = TERM;
  *nextp = s;
  return(1);
}

/* Multiple spaces in <s> may match a single space in <ptn>.
 * In addition, <s> may have leading and trailing spaces.
 * <ptn> must not have leading or trailing spaces.
 * If successful, <nextp> points to next line.
 */
Bool StringMatch_ModuloSpaces(char *ptn, char *s, int entireline,
                              /* RESULTS */ char **nextp)
{
  s = StringSkipspace(s);
  while (*ptn && *s == *ptn) {
    s++;
    ptn++;
    if (*ptn == SPACE) {
      if (*s != SPACE) return(0);
      s = StringSkipspace(s);
      ptn++;
    }
  }
  if (*ptn == TERM) {
    s = StringSkipspace(s);
    if (entireline) {
      if (*s == NEWLINE) {
        *nextp = s+1;
        return(1);
      }
    } else {
      *nextp = s;
      return(1);
    }
  }
  return(0);
}

void StringPrintLen(FILE *stream, char *s, int len)
{
  for (; len > 0; len--) {
    fputc(*s, stream);
    s++;
  }
}

void StringGenSym(char *result, int maxlen, char *prefix1, char *prefix2)
{
  if (prefix2) {
    if ((strlen(prefix1)+strlen(prefix2)+8)>maxlen)
      sprintf(result, "%s%d", prefix1, StringGenNext);
    else sprintf(result, "%s-%s%d", prefix1, prefix2, StringGenNext);
  } else {
    sprintf(result, "%s%d", prefix1, StringGenNext);
  }
  StringGenNext++;
}

Bool StringIsDigits(char *s)
{
  while (*s) {
    if (!Char_isdigit(*s)) return(0);
    s++;
  }
  return(1);
}

Bool StringIsAlphaOr(char *s, char *set)
{
  while (*s) {
    if ((!CharIsAlpha(*s)) && (!StringIn(*s, set))) return(0);
    s++;
  }
  return(1);
}

Bool StringIsDigitOr(char *s, char *set)
{
  while (*s) {
    if ((!Char_isdigit(*s)) && (!StringIn(*s, set))) return(0);
    s++;
  }
  return(1);
}

Bool StringIsDigitOrUpper(char *s)
{
  while (*s) {
    if ((!Char_isdigit(*s)) && (!CharIsUpper(*s))) return(0);
    s++;
  }
  return(1);
}

Bool StringIsNumber(char *s)
{
  if (s[0] == 'e') return(0);
  while (*s) {
    if ((!Char_isdigit(*s)) && *s != ',' && *s != '.'&& *s != '-' &&
        *s != '+' && *s != 'e') {
      return(0);
    }
    s++;
  }
  return(1);
}

char *StringSkipWhitespace_LeNonwhite(char *in)
{
  while (*in && (!LexEntryNonwhite(*((uc *)in)))) in++;
  return(in);
}

char *StringReadWhitespace_LeNonwhite(/* RESULTS */ char *out,
                                      /* INPUT */ char *in, int maxlen)
{
  maxlen--;
  while (*in && (!LexEntryNonwhite(*((uc *)in)))) {
    if (maxlen <= 0) {
      *out = TERM;
      while (*in && (!LexEntryNonwhite(*((uc *)in)))) {
        in++;
      }
      return(in);
    }
      /* No Dbg here because this is frequent when the punct is _____ etc. */
    *out++ = *in++;
    maxlen--;
  }
  *out = TERM;
  return(in);
}

void StringElimDupWhitespaceDest(char *to)
{
  int	prev;
  char	*from;
  prev = TERM;
  from = to;
  while (*from) {
    if (IsWhitespace(*from) && IsWhitespace(prev)) {
      from++;
    } else {
      prev = *to = *from;
      to++;
      from++;
    }
  }
  *to = TERM;
}

/* <nextp> is returned to point either to NEWLINE or next nonwhitespace.
 * todo: This is a bit inconsistent? We stop at commas, yet comma isn't
 * considered white space for forward scanning. But this may very well
 * be the right behavior...
 */
Bool StringGetWord(/* RESULTS */ char *out, /* INPUT */ char *in, int maxlen,
                   /* RESULTS */ char **nextp)
{
  char	*orig_out;
  orig_out = out;
  maxlen--;
  while (*in) {
    if (*in == SPACE || *in == NEWLINE || *in == ',') {
       /* Was *in == '-' but this messes up TA_TimeUnixEtc. */
      if (*in == NEWLINE) {
        *nextp = in;
      } else {
        *nextp = StringSkipWhitespaceNonNewline(in + 1);
      }
      *out = TERM;
      return(1);
    }
    if ((out - orig_out) >= maxlen) {
      return(0);
    }
    *out = *in;
    out++;
    in++;
  }
  return(0);
}

/* Returns pointer to next nonwhite. */
Bool StringGetWord_LeNonwhite(/* RESULTS */ char *out, /* INPUT */ char *in,
                              int maxlen, /* RESULTS */ char **nextp,
                              char *trail_punc)
{
  char	*orig_out;
  orig_out = out;
  maxlen--;
  while (*in) {
    if (!LexEntryNonwhite(*((uc *)in))) {
      if (trail_punc) {
        *nextp = StringReadWhitespace_LeNonwhite(trail_punc, in, PUNCLEN);
      } else {
        *nextp = StringSkipWhitespace_LeNonwhite(in + 1);
      }
      *out = TERM;
      return(1);
    }
    if ((out - orig_out) >= maxlen) {
      return(0);
    }
    *out = *in;
    out++;
    in++;
  }
  *nextp = in;
  *out = TERM;
  return(1);
}

Bool StringGetNWords_LeNonwhite(/* RESULTS */ char *out, char *trail_punc,
                                /* INPUT */ char *in, int maxlen, int n,
                                /* RESULTS */ char **nextp)
{
  char	*orig_out;
  int	inword;
  orig_out = out;
  inword = 0;
  maxlen--;
  while (*in) {
    if (LexEntryNonwhite(*((uc *)in))) {
      if (!inword) {
        inword = 1;
        n--;
        if (orig_out != out) {
          if ((out - orig_out) >= maxlen) {
            return(0);
          }
          *out = SPACE;
          out++;
        }
      }
      if ((out - orig_out) >= maxlen) {
        return(0);
      }
      *out = *in;
      out++;
      in++;
    } else {
      if (n <= 0) {
        *out = TERM;
        *nextp = StringReadWhitespace_LeNonwhite(trail_punc, in, PUNCLEN);
        return(1);
      }
      inword = 0;
      in++;
    }
  }
  return(0);
}

/* Convert string so it can be looked up by LexEntryFindPhrase.
 * "day-dream   day" => "day dream day"
 * (similar to LexEntryDbFileToPhrase?)
 */
Bool StringGetLeNonwhite(/* RESULTS */ char *out,
                         /* INPUT */ char *in, int maxlen)
{
  int	inword;
  char	*orig_out;
  inword = 0;
  orig_out = out;
  maxlen--;
  while (*in) {
    if (LexEntryNonwhite(*((uc *)in))) {
      if (!inword) {
        inword = 1;
        if (orig_out != out) {
          if ((out - orig_out) >= maxlen) {
            return(0);
          }
          *out = SPACE;
          out++;
        }
      }
      if ((out - orig_out) >= maxlen) {
        return(0);
      }
      *out = *in;
      out++;
      in++;
    } else {
      inword = 0;
      in++;
    }
  }
  if ((out - orig_out) >= maxlen) {
    return(0);
  }
  *out = TERM;
  return(1);
}

Bool StringGetNWords_LeN_Back(/* RESULTS */ char *output, /* INPUT */ char *in,
                              char *in_base, int maxlen, int n,
                              /* RESULTS */ char **nextp)
{
  char	*orig_out, buf[PHRASELEN], *out;
  int	inword;
  out = buf;
  orig_out = out;
  inword = 0;
  maxlen--;
  while (*in && in >= in_base) {
    if (LexEntryNonwhite(*((uc *)in))) {
      if (!inword) {
        inword = 1;
        n--;
        if (orig_out != out) {
          if ((out - orig_out) >= maxlen) {
            return(0);
          }
          *out = SPACE;
          out++;
        }
      }
      if ((out - orig_out) >= maxlen) {
        return(0);
      }
      *out = *in;
      out++;
      in--;
    } else {
      if (n == 0) {
        *nextp = in-1;	/* todo? */
        *out = TERM;
        StringReverse(orig_out, output);
        return(1);
      }
      inword = 0;
      in--;
    }
  }
  return(0);
}

/* Returns pointer to after digits. */
Bool StringGetDigits(/* RESULTS */ char *out, /* INPUT */ char *in,
                     int maxlen, /* RESULTS */ char **nextp)
{
  char	*orig_out;
  orig_out = out;
  maxlen--;
  while (*in) {
    if (!Char_isdigit(*in)) {
      *nextp = in;
      *out = TERM;
      return(1);
    }
    if ((out - orig_out) >= maxlen) {
      return(0);
    }
    *out = *in;
    out++;
    in++;
  }
  return(0);
}

/* Returns pointer to after digits. */
Bool StringGetDigitsOrSpace(/* RESULTS */ char *out, /* INPUT */ char *in,
                            int maxlen, /* RESULTS */ char **nextp)
{
  char	*orig_out;
  orig_out = out;
  maxlen--;
  while (*in) {
    if ((!Char_isdigit(*in)) && *in != SPACE) {
      *nextp = in;
      *out = TERM;
      return(1);
    }
    if (*in != SPACE) {
      if ((out - orig_out) >= maxlen) {
        return(0);
      }
      *out = *in;
      out++;
    }
    in++;
  }
  return(0);
}

int StringFirstLoc(char c, char *s)
{
  int	i;
  for (i = 0; *s; i++, s++) {
    if (c == *s) return(i);
  }
  return(i);
}

int StringTab(int pos, int tabsize)
{
  return(tabsize*((pos + tabsize)/tabsize));
}

int french_strcmp(const char *s, const char *t)
{
  for (; CharToLowerNoAccents(*((uc *)s)) == CharToLowerNoAccents(*((uc *)t));
       s++, t++) {
    if (*s == TERM) return(0);
  }
  return(CharToLowerNoAccents(*((uc *)s)) - CharToLowerNoAccents(*((uc *)t)));
}

int negative_strcmp(const char *s, const char *t)
{
  return(-french_strcmp(s, t));
}

int inverse_strcmp(const char *s, const char *t)
{
  const char	*orig_s, *orig_t;
  orig_s = s;
  orig_t = t;
  while (*s) s++;
  while (*t) t++;
  s--;
  t--;
  for (; CharToLowerNoAccents(*((uc *)s)) == CharToLowerNoAccents(*((uc *)t)) &&
         s >= orig_s && t >= orig_t; s--, t--);
  return(CharToLowerNoAccents(s >= orig_s ? *((uc *)s) : TERM) -
         CharToLowerNoAccents(t >= orig_t ? *((uc *)t) : TERM));
}

Bool StringIsPalindrome(char *s)
{
  char	*p;
  if (s[0] == TERM) return(0);
  if (s[1] == TERM) return(0);
  if (s[2] == TERM) return(0);
  p = s;
  while (*p) p++;
  p--;
  while (*p == *s) {
    p--;
    s++;
    if (p <= s) return(1);
  }
  return(0);
}

void StringAnagramUniquify(char *in, int maxlen, /* RESULTS */ char *out)
{
  int	i;
  char	*p, *orig_out;
  orig_out = out;
  maxlen--;
  for (i = 1; i < 128; i++) {
    for (p = in; *p; p++) {
      if (i == CharToLowerNoAccents(*((uc *)p))) {
        if ((out - orig_out) >= maxlen) {
          orig_out[maxlen] = TERM;
          return;
        }
        *out = i;
        out++;
      }
    }
  }
  *out = TERM;
}

void StringPrintJustify(FILE *stream, char *s, int len, int just)
{
  int	i, slen, total;
  slen = strlen(s);
  if (slen > len) {
    for (i = 0; i < len; i++) StreamPutcNoNewline(s[i], stream);
    return;
  }
  if (just == RPT_JUST_RIGHT) {
    total = len - slen;
    StreamPrintSpaces(stream, total);
  } else if (just == RPT_JUST_CENTER) {
    total = (len - slen)/2;
    StreamPrintSpaces(stream, total);
  } else total = 0;
  StreamPutsNoNewline(s, stream);
  total += slen;
  StreamPrintSpaces(stream, len-total);
}

#ifdef notdef
void StringPrintCorpusLine(FILE *stream, char *text, long offset,
                           int halflinelen, int wordlen)
{
  char	buf[LINELEN];
  int	startpos, len;

  /* What precedes word. */
  startpos = IntMax(0, offset - halflinelen);
  len = offset - startpos;
  memcpy(buf, text + startpos, (size_t)len);
  buf[len] = TERM;
  StringPrintJustify(stream, buf, halflinelen, RPT_JUST_RIGHT);
  /* Word and what follows word. */
  StringPrintJustify(stream, text + offset, halflinelen + wordlen,
                     RPT_JUST_LEFT);
}
#endif

void StringMakeCorpusLine(char *text, long offset, int halflinelen,
                          int wordlen, /* RESULTS */ char *buf)
{
  long	startpos, len;
  startpos = offset - halflinelen;
  len = halflinelen + halflinelen + wordlen;
  if (startpos >= 0) {
    memcpy(buf, text + startpos, (size_t)len);
    buf[len] = TERM;
  } else {
    memset(buf, SPACE, (long)(-startpos));
    len += startpos;
    memcpy(buf-startpos, text, (size_t)len);
    buf[len-startpos] = TERM;
  }
  StringMapChar(buf, NEWLINE, SPACE);
  StringMapChar(buf, TAB, SPACE);
}

/* StringArray */

StringArray *StringArrayCreate()
{
  StringArray	*sa;
  sa = CREATE(StringArray);
  sa->maxlen = 16L;
  sa->len = 0L;
  sa->strings = (char **)MemAlloc(sa->maxlen*sizeof(char *),
                                  "StringArray* strings");
  return(sa);
}

void StringArrayFree(StringArray *sa)
{
  MemFree(sa->strings, "StringArray* strings");
  MemFree(sa, "StringArray");
}

void StringArrayFreeCopy(StringArray *sa)
{
  long	i;
  for (i = 0; i < sa->len; i++) {
    MemFree(sa->strings[i], "StringArrayAddCopy");
  }
  MemFree(sa->strings, "StringArray* strings");
  MemFree(sa, "StringArray");
}

void StringArrayAdd(StringArray *sa, char *s, int elimdup)
{
  int	i;
  if (elimdup) {
    for (i = 0; i < sa->len; i++) {
      if (streq(sa->strings[i], s)) return;
    }
  }
  if (sa->len >= sa->maxlen) {
    sa->maxlen = sa->maxlen*2;
    sa->strings = (char **)MemRealloc(sa->strings,
                                      sa->maxlen*sizeof(char *),
                                      "StringArray* strings");
  }
  sa->strings[sa->len] = s;
  sa->len++;
}

void StringArrayAddCopy(StringArray *sa, char *s, int elimdup)
{
  StringArrayAdd(sa, StringCopy(s, "StringArrayAddCopy"), elimdup);
}

int StringArraySortColumn;

int StringArrayComparStrcmp(const void *s1, const void *s2)
{
  return(french_strcmp(StringIth(*((char **)s1), StringArraySortColumn),
                       StringIth(*((char **)s2), StringArraySortColumn)));
}

char *StringSkipHTML(char *s)
{
  while (*s == '<') {
    s++;
    while (*s && *s != '>') s++;
    if (*s == TERM) return(s);
    s++;
  }
  return(s);
}

int StringArrayHTMLComparStrcmp(const void *s1, const void *s2)
{
  return(french_strcmp(StringSkipHTML(*((char **)s1)),
                       StringSkipHTML(*((char **)s2))));
}

int StringArrayComparNegativeStrcmp(const void *s1, const void *s2)
{
  return(negative_strcmp(StringIth(*((char **)s1), StringArraySortColumn),
                         StringIth(*((char **)s2), StringArraySortColumn)));
}

int StringArrayComparInverseStrcmp(const void *s1, const void *s2)
{
  return(inverse_strcmp(StringIth(*((char **)s1), StringArraySortColumn),
                        StringIth(*((char **)s2), StringArraySortColumn)));
}

void StringArraySort1(StringArray *sa,
                      int (*compar)(const void *, const void *))
{
  qsort(sa->strings, (size_t)sa->len, (size_t)sizeof(char *), compar);
}

void StringArraySort(StringArray *sa)
{
  StringArraySortColumn = 0;
  StringArraySort1(sa, StringArrayComparStrcmp);
}

void StringArrayHTMLSort(StringArray *sa)
{
  StringArraySort1(sa, StringArrayHTMLComparStrcmp);
}

void StringArraySortCol(StringArray *sa, int col)
{
  StringArraySortColumn = col;
  StringArraySort1(sa, StringArrayComparStrcmp);
}

void StringArrayNegativeSort(StringArray *sa)
{
  StringArraySortColumn = 0;
  StringArraySort1(sa, StringArrayComparNegativeStrcmp);
}

void StringArrayInverseSort(StringArray *sa)
{
  StringArraySortColumn = 0;
  StringArraySort1(sa, StringArrayComparInverseStrcmp);
}

void StringArrayPrint(FILE *stream, StringArray *sa, int elimdup, int showcount)
{
  char	*prev;
  long	i, cnt;
  prev = "";
  for (i = 0, cnt = 0; i < sa->len; i++) {
    if ((!elimdup) || (0 != french_strcmp(prev, sa->strings[i]))) {
      fputs(prev = sa->strings[i], stream);
      fputc(NEWLINE, stream);
      cnt++;
    }
  }
  if (showcount) fprintf(stream, "%ld items\n", cnt);
}

void StringArrayTextPrintList(Text *text, StringArray *sa,
                              int use_serial_comma, int post, char *and_string)
{
  long	i;
  for (i = 0; i < sa->len; i++) {
    if (i == sa->len-2) {
      TextPrintPhrase(text, (use_serial_comma && (sa->len > 2)) ? ',' : TERM,
                      sa->strings[i]);
      TextPrintPhrase(text, TERM, and_string);
    } else if (i == sa->len-1) {
      TextPrintPhrase(text, post, sa->strings[i]);
    } else {
      TextPrintPhrase(text, ',', sa->strings[i]);
    }
  }
}

StringArray *StringArrayStripOutColumn(char *in, char *stop_before,
                                       int startpos, int stoppos, int tabsize)
{
  int		pos, i;
  char		value[PHRASELEN];
  StringArray	*sa;
  sa = StringArrayCreate();
  pos = 0;
  while (stop_before == NULL || in < stop_before) {
    if (*in == TERM) break;
    if (*in == TAB) {
      pos = StringTab(pos, tabsize);
      in++;
    } else if (*in == NEWLINE) {
      pos = 0;
      in++;
    } else if (pos == startpos) {
      for (i = 0; pos <= stoppos; i++, in++) {
        if (*in == NEWLINE) break;
        if (*in == TAB) {
          value[i] = SPACE;
          pos = StringTab(pos, tabsize);
        } else {
          value[i] = *in;
          pos++;
        }
      }
      value[i] = TERM;
      StringElimTrailingBlanks(value);
      StringElimLeadingBlanks(value);
      StringArrayAddCopy(sa, value, 0);
      pos++;
    } else {
      pos++;
      in++;
    }
  }
  return(sa);
}

/* End of file. */
