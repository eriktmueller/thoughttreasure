/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19920307: utilities begun
 * 19920905: added string and character mapping
 * 19931224: added strdup1, pe
 * 19940419: basic types begun
 * 19940420: more
 * 19940702: merged Utils into Basic
 * 19940718: switch to big malloc to speed loading
 * 19950223: added many maxlen checks
 * 19970719: redid memory allocation options
 * 19981108T132215: timestamp mods
 * 19981112T075423: changed gets to fgets
 * 19981115T075312: added FileTemp
 */

#include "tt.h"
#include "lexentry.h"
#include "repbasic.h"
#include "repdb.h"
#include "repobj.h"
#include "repobjl.h"
#include "repstr.h"
#include "repsubgl.h"
#include "reptext.h"
#include "reptime.h"
#include "semdisc.h"
#include "tatime.h"
#include "toolsh.h"
#include "utildbg.h"
#include "utilhtml.h"

/* UnsignedInt */

Bool UnsignedIntIsString(char *s)
{
  while (*s) {
    if (!Char_isdigit(*s)) return(0);
    s++;
  }
  return(1);
}

/* Int */

int IntMin(int x, int y)
{
  if (x<y) return(x);
  else return(y);
}

int IntMax(int x, int y)
{
  if (x>y) return(x);
  else return(y);
}

int IntAbs(int x)
{
  if (x >= 0) return(x); else return(-x);
}

Bool IntIsChar(int c)
{
  return(Char_isdigit(c) || c == '-' || c == '+');
}

Bool IntIsString(char *s)
{
  while (*s) {
    if (!(IntIsChar(*s))) return(0);
    s++;
  }
  return(1);
}

Bool IntIsSpecification(int specific, int general)
{
  return(general == INTNA || specific == general);
}

int IntParse(char *s, int len)
{
  char	buf[WORDLEN];
  
  StringCpy((char *)buf, (char *)s, WORDLEN);
  if (len >= 0) buf[len] = TERM;
  if (!IntIsString(buf)) {
    Dbg(DBGGEN, DBGBAD, "IntParse: trouble parsing <%s>", buf, E);
    return(INTNA);
  }
  return(atoi(buf));
}

/* Long */

long LongAbs(long x)
{
  if (x >= 0) return(x); else return(-x);
}

/* Float */

Float FloatMin(Float x, Float y)
{
	if (x<y) return(x);
	else return(y);
}

Float FloatMax(Float x, Float y)
{
	if (x>y) return(x);
	else return(y);
}

Float FloatAbs(Float x)
{
  if (x >= 0.0) return(x); else return(-x);
}

Float FloatSign(Float x)
{
  if (x == 0.0) return(0.0);
  else if (x > 0.0) return(1.0);
  else return(-1.0);
}

Float FloatRound(Float x, Float nearest)
{
  Float	x1; 
  x1 = 0.5+(x/nearest);
  return(nearest*((Float)((long)x1)));
}

Float FloatInt(Float x)
{
  return((Float)((long)x));
}

Float FloatAvgMinMax(Float x, Float y, Float xmin, Float ymin)
{
  if (FLOATNA == x || FLOATNA == y) return(FLOATNA);
  if (x < xmin) x = xmin;
  if (y > ymin) y = ymin;
  return(0.5*(x + y));  
}

void FloatAvgInit(/* RESULTS */ Float *numer, Float *denom)
{
  *numer = *denom = 0.0;
}

void FloatAvgAdd(Float x, /* RESULTS */ Float *numer, Float *denom)
{
  (*numer) += x;
  (*denom) += 1.0;
}

Float FloatAvgResult(Float numer, Float denom, Float def)
{
  if (denom == 0.0) return(def);
  return(numer/denom);
}

Bool FloatIsChar(int c)
{
  /* Used to allow d and D but these are not used and they cause
   * problems with parsing of Days.
   */
  return(Char_isdigit(c) || c == '.' || c == ',' || c == '-' || c == '+' ||
         c == 'e' || c == 'E');
}

Bool FloatIsString(char *s)
{
  while (*s) {
    if (!(FloatIsChar(*s))) return(0);
    s++;
  }
  return(1);
}

Float FloatParse(char *s, int len)
{
  char	buf[WORDLEN];
  
  StringCpy(buf, s, WORDLEN);
  StringElimChar(buf, ',');	/* todo: French decimals. */
  if (len >= 0) buf[len] = TERM;
  if (streq(s, "-Inf")) return(FLOATNEGINF);
  if (streq(s, "+Inf")) return(FLOATPOSINF);
  if (streq(s, "Inf")) return(FLOATPOSINF);
  if (!FloatIsString(buf)) {
    Dbg(DBGGEN, DBGBAD, "FloatParse: trouble parsing <%s>", buf, E);
    return(INTNA);
  }
  return(atof(buf));
}

void FloatRangeParse(char *s, Float *from, Float *to)
{
  char *p;
  for (p = s; *p; p++) {
    if ((unsigned char)*p == TREE_RANGE) {
      *p = TERM;
      *from = FloatParse(s, -1);
      *to = FloatParse(p+1, -1);
      return;
    }
  }
  Dbg(DBGGEN, DBGBAD, "FloatRangeParse: trouble parsing <%s>", s, E);
  *from = FLOATNA;
  *to = FLOATNA;
}

/* Random */

void RandomSeed(long seed)
{
  if (seed == 0) seed = 1235;
/* exhaustive ifdefs */
#ifdef GCC
  srandom(seed);
#endif
#ifdef SOLARIS
  srand48(seed);
#endif
#ifdef SUNOS
  srand48(seed);
#endif
#ifdef MACOS
  srand(seed);
#endif
#ifdef NEXT
  srandom(seed);
#endif
}

/* (0, 1) */
Float RandomFloat0_1()
{
/* exhaustive ifdefs */
#ifdef GCC
  return(random()/2147483647.0);
#endif
#ifdef SOLARIS
  return(drand48());
#endif
#ifdef SUNOS
  return(drand48());
#endif
#ifdef MACOS
  return(rand()/((Float)RAND_MAX));
#endif
#ifdef NEXT
  return(random()/2147483647.0);
#endif
#ifdef MACOS
  return(random()/2147483647.0);
#endif
}

void RandomInit()
{
  RandomSeed(time(0));
}

/* (from, to) */
Float RandomFloatFromTo(Float from, Float to)
{
  return(from + ((to-from)*RandomFloat0_1()));
}

int Roll(int n)
{
  return((int)(RandomFloat0_1()*n));
}

int RandomIntFromTo(int from, int to)
{
  return((int)RandomFloatFromTo((Float)from, 1.0+((Float)to)));
}

Bool WithProbability(Float x)
{
  return(RandomFloat0_1() < x);
}

/* Char */

Bool CharIsOpening(int c)
{
  return(c == LBRACE ||
         c == LGUILLEMETS ||
         c == DQUOTE ||
         c == SQUOTE ||
         c == UNDERLINE);
}

Bool CharClosing(int c)
{
  switch (c) {
    case LBRACE: return(RBRACE);
    case LGUILLEMETS: return(RGUILLEMETS);
    case DQUOTE: return(DQUOTE);
    case SQUOTE: return(SQUOTE);
    case UNDERLINE: return(UNDERLINE);
    default:
      break;
  }
  return(c);
}

Bool CharIsGreekLower(int c)
{
  return(c == ((uc)'ß') || c == ((uc)'µ') || c == ((uc)'þ'));
}

Bool CharIsGreekUpper(int c)
{
  return(c == ((uc)'Ë') || c == ((uc)'Ú') || c == ((uc)'Ô') || c == ((uc)'Ð'));
}

Bool CharIsUpper(int c)
{
  return(Char_isupper(c) || c == ((uc)'À') || c == ((uc)'É') ||
         c == ((uc)'‹') || c == ((uc)'Ò') || c == ((uc)'Ü') ||
         CharIsGreekUpper(c));
}

Bool CharIsLower(int c)
{
  return(Char_islower(c)|| c == ((uc)'à') || c == ((uc)'â') || c == ((uc)'ä') ||
         c == ((uc)'ç') || c == ((uc)'é') || c == ((uc)'è') || c == ((uc)'ê') ||
         c == ((uc)'ë') || c == ((uc)'î') || c == ((uc)'ï') || c == ((uc)'Ó') ||
         c == ((uc)'ò') || c == ((uc)'ô') || c == ((uc)'ö') || c == ((uc)'û') ||
         c == ((uc)'ù') || c == ((uc)'ü') || CharIsGreekLower(c));
}

Bool CharIsAlpha(int c)
{
  return(CharIsLower(c) || CharIsUpper(c));
}

Bool CharIsAlphanumeric(int c)
{
  return(CharIsLower(c) || CharIsUpper(c) || Char_isdigit(c));
}

int CharToUpper(int c)
{
  if (Char_islower(c)) return((c - 'a') + 'A');
  else if (c == ((uc)'à')) return((uc)'À');
  else if (c == ((uc)'â')) return((uc)'A');
  else if (c == ((uc)'ç')) return((uc)'C');
  else if (c == ((uc)'é')) return((uc)'É');
  else if (c == ((uc)'è')) return((uc)'E');
  else if (c == ((uc)'ê')) return((uc)'E');
  else if (c == ((uc)'î')) return((uc)'‹');
  else if (c == ((uc)'Ó')) return((uc)'Ò');
  else if (c == ((uc)'ô')) return((uc)'O');
  else if (c == ((uc)'û')) return((uc)'U');
  else if (c == ((uc)'ù')) return((uc)'U');
  else if (c == ((uc)'ü')) return((uc)'Ü');
  else return(c);
}

int CharToLowerNoAccents(int c)
{
  c = CharToLower(c);
  if (Char_islower(c)) return(c);
  else if (c == ((uc)'à')) return('a');
  else if (c == ((uc)'â')) return('a');
  else if (c == ((uc)'ç')) return('c');
  else if (c == ((uc)'é')) return('e');
  else if (c == ((uc)'è')) return('e');
  else if (c == ((uc)'ê')) return('e');
  else if (c == ((uc)'î')) return('i');
  else if (c == ((uc)'Ó')) return('o');
  else if (c == ((uc)'ô')) return('o');
  else if (c == ((uc)'û')) return('u');
  else if (c == ((uc)'ù')) return('u');
  else if (c == ((uc)'ü')) return('u');
  else return(c);
}

int CharToLower(int c)
{
  if (Char_isupper(c)) return((c - 'A') + 'a');
  else if (c == ((uc)'À')) return((uc)'à');
  else if (c == ((uc)'C')) return((uc)'ç');
  else if (c == ((uc)'É')) return((uc)'é');
  else if (c == ((uc)'E')) return((uc)'è');
  else if (c == ((uc)'E')) return((uc)'ê');
  else if (c == ((uc)'‹')) return((uc)'î');
  else if (c == ((uc)'Ò')) return((uc)'Ó');
  else if (c == ((uc)'O')) return((uc)'ô');
  else if (c == ((uc)'U')) return((uc)'û');
  else if (c == ((uc)'U')) return((uc)'ù');
  else if (c == ((uc)'Ü')) return((uc)'ü');
  else return(c);
}

int CharDevoice(int c)
{
  if (c == 'b') return('p');
  else if (c == 'd') return('t');
  else if (c == 'g') return('k');
  else if (c == 'v') return('f');
  else if (c == 'z') return('s');
  else return(c);
}

int CharReduceVowel(int c)
{
  if (StringIn(c, "aeiou")) return('e');
  else return(c);
}

void CharPutN(FILE *stream, int c, int n)
{
  int i;
  for (i = 0; i < n; i++) fputc(c, stream);
}

/* HashTable */

/*
int HashTableHash(HashTable *ht, char *symbol)
{
  unsigned char s[HASHSIG];

  memset(s, 0, 6);
  memcpy(s, symbol, (size_t)IntMin((int)strlen((char *)symbol), 6));
  return((int)(((s[1] + s[5] + (s[0]+s[4])*(long)256) +
                (s[3] + s[2]*(long)256)*(long)481) % ht->size));
}
*/

int HashTableHash(HashTable *ht, char *symbol)
{
  unsigned char s[HASHSIG];
  size_t len;

  len = (size_t)strlen(symbol);
  if (len > 6) len = 6L;
  memset(s, 0, 6);
  memcpy(s, symbol, (size_t)len);
  return((int)(((s[1] + s[5] + (s[0]+s[4])*(long)256) +
                (s[3] + s[2]*(long)256)*(long)481) % ht->size));
}

HashTable *HashTableCreate(size_t size)
{
  HashTable	*ht;
  int		i;

  ht = (HashTable *)MemAlloc(sizeof(int)+size*sizeof(HashEntry *), "HashTable");
  ht->size = size;
  for (i = 0; i < size; i++) ht->hashentries[i] = NULL;
  return(ht);
}

void *HashTableGet(HashTable *ht, char *symbol)
{
  register HashEntry	*he;

  for (he = ht->hashentries[HashTableHash(ht, symbol)]; he; he = he->next) {
    if (symbol[0] == he->symbol[0] && 0 == strcmp(symbol, he->symbol)) {
      return(he->value);
    }
  }
  return(NULL);
}

char *HashTableIntern(HashTable *ht, char *symbol)
{
  register HashEntry	*he;
  int			i;

  i = HashTableHash(ht, symbol);
  for (he = ht->hashentries[i]; he; he = he->next) {
    if (symbol[0] == he->symbol[0] && 0 == strcmp(symbol, he->symbol)) {
      return(he->symbol);
    }
  }
  he = CREATE(HashEntry);
  he->symbol = StringCopy(symbol, "char HashTableIntern");
  he->value = NULL;
  he->next = ht->hashentries[i];
  ht->hashentries[i] = he;
  return(he->symbol);
}

void HashTableSet(HashTable *ht, char *symbol, void *value)
{
  register HashEntry	*he;
  int			i;

  i = HashTableHash(ht, symbol);
  for (he = ht->hashentries[i]; he; he = he->next) {
    if (symbol[0] == he->symbol[0] && 0 == strcmp(symbol, he->symbol)) {
      he->value = value;
      return;
    }
  }
  he = CREATE(HashEntry);
  he->symbol = symbol;
  he->value = value;
  he->next = ht->hashentries[i];
  ht->hashentries[i] = he;
}

void HashTableSetDup(HashTable *ht, char *symbol, void *value)
{
  register HashEntry	*he;
  int			i;

  i = HashTableHash(ht, symbol);
  for (he = ht->hashentries[i]; he; he = he->next) {
    if (symbol[0] == he->symbol[0] && 0 == strcmp(symbol, he->symbol)) {
      he->value = value;
      return;
    }
  }
  he = CREATE(HashEntry);
  he->symbol = StringCopy(symbol, "char HashTableSetDup");
  he->value = value;
  he->next = ht->hashentries[i];
  ht->hashentries[i] = he;
}

void HashTableForeach(HashTable *ht, void (fn)())
{
  int		i;
  HashEntry	*he;

  for (i = 0; i < ht->size; i++) {
    for (he = ht->hashentries[i]; he; he = he->next) {
      (fn)(he->symbol, he->value);
    }
  }
}

void HashTablePrint(FILE *stream, HashTable *ht)
{
  int	i;
  HashEntry	*he;

  for (i = 0; i < ht->size; i++) {
    he = ht->hashentries[i];
    if (he == NULL) {
      fprintf(stream, "%d: unused\n", i);
    } else {
      for (; he; he = he->next) {
        fprintf(stream, "%d 0x%p: 0x%p %s\n", i, he, he->symbol, he->symbol);
      }
    }
  }
}

void pht(HashTable *ht)
{
  HashTablePrint(stdout, ht);
  fputc(NEWLINE, stdout);
  HashTablePrint(Log, ht);
  fputc(NEWLINE, Log);
}

/* File */

void FileTemp(char *stem, int fn_len, /* OUTPUT */ char *fn)
{
#ifdef _GNU_SOURCE
  snprintf(fn, (size_t)fn_len, "%s.%ld.%ld", stem, (long)getpid(),
           (long)time(0));
#else
  sprintf(fn, "%s.%ld.%ld", stem, (long)getpid(),
          (long)time(0));
#endif
}

void FileNameGen(char *stem, char *suffix, int fn_len, /* OUTPUT */ char *fn)
{
#ifdef _GNU_SOURCE
  snprintf(fn, (size_t)fn_len, "%s.%ld.%ld.%s", stem, (long)getpid(),
           (long)time(0), suffix);
#else
  sprintf(fn, "%s.%ld.%ld.%s", stem, (long)getpid(),
          (long)time(0), suffix);
#endif
}

/* Stream */

FILE *StreamOpen(char *filename, char *mode)
{
  FILE *stream;
  Dbg(DBGGEN, DBGBAD, "%s %s", streq(mode, "r") ? "reading": "opening",
      filename, E);
  if (NULL == (stream = fopen(filename, mode))) {
    Dbg(DBGGEN, DBGBAD, "Trouble opening %s\n", filename);
    return(NULL);
  }
  return(stream);
}

FILE *StreamOpenEnv(char *filename, char *mode)
{
  char fn[FILENAMELEN];
  sprintf(fn, "%s/%s", TTRoot, filename);
  return StreamOpen(fn, mode);
}

void StreamClose(FILE *stream)
{
  if (stream == NULL || stream == stdin ||
      stream == stdout || stream == stderr) {
    return;
  }
  fclose(stream);
}

/* todo: Could add debugging of last n characters read. */
int StreamReadc(FILE *stream)
{
  register int c;
  while (1) {
    if (EOF == (c = getc(stream))) return(EOF);
    if (c == TREE_COMMENT) {
      while (NEWLINE != (c = getc(stream)))
        if (EOF == c) return(EOF);
    } else if (c != NEWLINE) return(c);
  }
}

int StreamPeekc(FILE *stream)
{
  int c;
  while (1) {
    if (EOF == (c = getc(stream))) return(EOF);
    if (c == TREE_COMMENT) {
      while (NEWLINE != (c = getc(stream)))
        if (EOF == c) return(EOF);
    } else if (c != NEWLINE) {
	  ungetc(c, stream);
      return(c);
    }
  }
}

int StreamAsk(FILE *in, FILE *out, char *question, int maxlen,
              /* RESULTS */ char *answer)
{
  while (1) {
    fputs(question, out);
    if (fgets(answer, maxlen, in) == NULL) {
      return(0);
    }
    if (in != stdin) {
      fputs(answer, out);
      Dbg(DBGGEN, DBGDETAIL, "<%s> <%s>", question, answer);
    }
    answer[strlen(answer)-1] = TERM;
    if (streq(answer, "up")) return(0);
    else if (streq(answer, "pop")) return(0);
    else if (streq(answer, "quit")) return(0);
    else if (streq(answer, "stop")) Stop();
    else if (streq(answer, "tt")) Tool_Shell_Std();
    else return(1);
  }
}

int StreamAskStd(char *question, int maxlen, char *answer)
{
  while (1) {
    fputs(question, stdout);
    while (fgets(answer, maxlen, stdin) == NULL) {
      fputs(question, stdout);
    }
    answer[strlen(answer)-1] = TERM;
    if (streq(answer, "up")) return(0);
    else if (streq(answer, "pop")) return(0);
    else if (streq(answer, "quit")) return(0);
    else if (streq(answer, "stop")) Stop();
    else if (streq(answer, "tt")) Tool_Shell_Std();
    else return(1);
  }
}

void StreamPrintf(FILE *stream, char *fmt, ...)
{
  va_list	args;
#ifdef SUNOS
  va_start(args);
#else
  va_start(args, fmt);
#endif
  if (stream == NULL) return;
  vfprintf(stream, fmt, args);
  va_end(args);
}

void StreamPutc(int c, FILE *stream)
{
  if (stream == NULL) return;
  fputc(c, stream);
}

void StreamPutcExpandCntrlChar(int c, FILE *stream)
{
  if (stream == NULL) return;
  if (c == NEWLINE) fputs("\\n", stream);
  else if (c == TAB) fputs("\\t", stream);
  else fputc(c, stream);
}

void StreamPutsExpandCntrlChar(char *s, FILE *stream)
{
  if (stream == NULL) return;
  while (*s) {
    StreamPutcExpandCntrlChar(*s, stream);
    s++;
  }
}

void StreamPutcNoNewline(int c, FILE *stream)
{
  if (stream == NULL) return;
  if (c == NEWLINE) fputc(SPACE, stream);
  else fputc(c, stream);
}

void StreamPuts(char *s, FILE *stream)
{
  if (stream == NULL) return;
  fputs(s, stream);
}

void StreamPutsNoNewline(char *s, FILE *stream)
{
  if (stream == NULL) return;
  while (*s) {
    StreamPutcNoNewline(*s, stream);
    s++;
  }
}

void StreamPutsNoNewlineMax(char *s, FILE *stream, int len)
{
  if (stream == NULL) return;
  while (*s && len > 0) {
    StreamPutcNoNewline(*s, stream);
    s++;
    len--;
  }
}

void StreamPutsExpandCR(char *s, FILE *stream)
{
  if (stream == NULL) return;
  while (*s) {
    fputc(*s, stream);
    if (*s == CR) fputc(NEWLINE, stream);
    s++;
  }
}

int StreamWriteWord(FILE *stream, char *word, int pos, int linelen)
{
  int	len;
  len = strlen(word);
  if (len + pos > linelen) {
    fputc(NEWLINE, stream);
    pos = 0;
  }
  if (pos != 0) fputc(SPACE, stream);
  fputs(word, stream);
  pos += len;
  return(pos);
}

int StreamReadTo(FILE *stream, int including, /* RESULTS */ char *buf,
                 /* INPUT */ int maxlen, int sep1, int sep2, int sep3)
{
  int c;
  char *p;
  p = buf;
  maxlen--;
  while ((c = StreamReadc(stream)) != sep1 && c != sep2 && c != sep3) {
    if (c == TREE_ESCAPE) {
      *p = c;
      p++;
      c = StreamReadc(stream);
    }
    if (c == EOF) {
      buf[maxlen] = TERM;
      Dbg(DBGGEN, DBGBAD, "StreamReadTo: end of file reached", E);
      return(TERM);
    }
    *p = c;
    p++;
    if ((p - buf) >= maxlen) {
      buf[maxlen] = TERM;
      Dbg(DBGGEN, DBGBAD,
          "StreamReadTo: overflow reading up to <%c> <%c> <%c>: <%s>",
          sep1, sep2, sep3, buf);
      return(TERM);
    }
  }
  *p = TERM;
  if (!including) ungetc(c, stream);
  return(c);
}

int StreamSkipTo(FILE *stream, int sep1, int sep2, int sep3)
{
  int c;
  while ((c = StreamPeekc(stream)) != sep1 && c != sep2 && c != sep3) {
    if (c == EOF) {
      Dbg(DBGGEN, DBGBAD, "StreamSkipTo: hit EOF skipping to <%c> <%c> <%c>",
                           sep1, sep2, sep3);
      return(TERM);
    }
    StreamReadc(stream);
  }
  return(c);
}

void StreamPrintSpaces(FILE *stream, int spaces)
{
  int	i;
  for (i = 0; i < spaces; i++) fputc(SPACE, stream);
}

void StreamSep(FILE *stream)
{
  fputs(
"________________________________________________________________________________\n",
        stream);
}

void StreamGen(FILE *stream, char *preface, Obj *obj, int eos, Discourse *dc)
{
  Text *text;
  text = TextCreate(128,
                    DiscourseCurrentChannelLineLen(dc) - strlen(preface),
                    dc);
  TextGen(text, obj, eos, dc);
  if (TextLen(text) > 0) {
    TextPrintWithPreface(stream, preface, text);
    fputc(NEWLINE, stream);
    fflush(stream);
  }
  TextFree(text);
}

void StreamGenParagraph(FILE *stream, char *preface, ObjList *in_objs,
                        int sort_by_ts, int gen_pairs, Discourse *dc)
{
  Text *text;
  text = TextCreate(1024,
                    DiscourseCurrentChannelLineLen(dc) - strlen(preface),
                    dc);
  TextGenParagraph(text, in_objs, sort_by_ts, gen_pairs, dc);
  if (TextLen(text) > 0) {
    TextPrintWithPreface(stream, preface, text);
    fputc(NEWLINE, stream);
    fflush(stream);
  }
  TextFree(text);
}

void StreamGrep(char *fn, char *ptn)
{
  char	line[LINELEN];
  FILE	*stream;
  if (NULL == (stream = StreamOpen(fn, "r"))) return;
  while (fgets(line, LINELEN, stream)) {
    if (strstr(line, ptn)) fputs(line, Log); 
  }
  StreamClose(stream);
}

/* todo: Assumes <tt> and </tt> occur in one line. */
void StreamHTMLMassage(char *infn, char *outfn)
{
  char	line[LINELEN], objname[OBJNAMELEN], *p;
  Obj	*obj;
  FILE	*instream, *outstream;
  if (NULL == (instream = StreamOpen(infn, "r"))) return;
  if (NULL == (outstream = StreamOpen(outfn, "w+"))) return;
  while (fgets(line, LINELEN, instream)) {
    for (p = line; *p; ) {
      if (StringHeadEqualAdvance("<tt>", p, &p)) {
        p = StringReadTo(p, objname, OBJNAMELEN, '<', NEWLINE, TERM);
        if (StringHeadEqualAdvance("/tt>", p, &p)) {
          if ((obj = NameToObj(objname, OBJ_NO_CREATE))) {
            HTML_LinkToObj(outstream, obj, 0, 1, 0);
          } else {
            fprintf(outstream, "<tt>%s</tt>", objname);
          }
        } else {
          Dbg(DBGGEN, DBGBAD, "unbalanced <tt> in line <%s>", line);
        }
      } else {
        fputc(*p, outstream);
        p++;
      }
    }
  }
  StreamClose(instream);
  StreamClose(outstream);
}

void StreamHTMLMassageDirectory(char *indir, char *outdir)
{
  char	outfn[FILENAMELEN];
  Directory *dir, *p;
  if (!(dir = DirectoryRead(indir))) return;
  for (p = dir; p; p = p->next) {
    sprintf(outfn, "%s/%s", outdir, p->basename);
    StreamHTMLMassage(p->fn, outfn);
  }
  DirectoryFree(dir);
}

void StreamSortIn(int treesort)
{
  char		*s, *p, line[TREEITEMLEN];
  StringArray	*sa;
  if (!(s = StringReadFile("In", 0))) return;
  sa = StringArrayCreate();
  p = s;
  while (p[0]) {
    if (treesort) {
      if (!StringReadUntilString(p, "\n===", TREEITEMLEN, line, &p)) break;
      p = p-3;
    } else {
      p = StringReadTo(p, line, TREEITEMLEN, NEWLINE, TERM, TERM);
    }
    StringArrayAddCopy(sa, line, 0);
  }
  StringArraySort(sa);
  StreamSep(Log);
  StringArrayPrint(Log, sa, 1, 0);
  StreamSep(Log);
  StringArrayFreeCopy(sa);
  MemFree(s, "char StringReadUntil");
}

Bool StreamFileModtime(char *fn, /* RESULTS */ Ts *ts)
{
  struct stat	statrec;
  if (0 > stat(fn, &statrec)) return(0);
  TsSetUnixTs(ts, statrec.st_mtime);
  return(1);
}

/* Indent */

int IndentLevel;

void IndentInit()
{
  IndentLevel = -1;
}

void IndentUp()
{
  IndentLevel++;
}

void IndentDown()
{
  IndentLevel--;
}

void IndentPrint(FILE *stream)
{
  int i;
  for (i = 0; i < IndentLevel; i++) {
    fputc('>', stream);
  }
}

void IndentPrintText(Text *text, char *s)
{
  int i;
  for (i = 0; i < IndentLevel; i++) {
    TextPuts(s, text);
  }
}

/* Directory */

Directory *DirectoryCreate(char *fn, char *basename, Ts *ts, Directory *next)
{
  Directory	*dir;
  dir = CREATE(Directory);
  StringCpy(dir->fn, fn, FILENAMELEN);
  StringCpy(dir->basename, basename, FILENAMELEN);
  dir->ts = *ts;
  dir->next = next;
  return(dir);
}

void DirectoryFree(Directory *dir)
{
  /* todo */
}

/* Read directories produced by MS-DOS dir command or Macintosh
 * directory copy/paste.
 */
Directory *DirectoryReadDIRDIR(char *dirfn)
{
  char			dirdirfn[FILENAMELEN], fn[FILENAMELEN], line[LINELEN];
  FILE			*stream;
  Directory		*r;
  Ts			copy_ts, dirdir_mod_ts, stat_mod_ts, ts;
  TsParse(&copy_ts, "19940122T170000");
  sprintf(dirdirfn, "%s/DIR.DIR", dirfn);
  if (NULL == (stream = StreamOpen(dirdirfn, "r"))) return(NULL);
  r = NULL;
  memset(line, 0, LINELEN);
  while (fgets(line, LINELEN, stream)) {
    line[strlen(line)-1] = TERM;
    if (line[0] == TERM || line[0] == SPACE) continue;
  /* Examples:
   * 0000000000111111111122222222223333333333
   * 0123456789012345678901234567890123456789
   * DISSSENT TXT      1052 07-28-87   1:04a
   * TMAC     X        8554 05-18-88   7:10a
   * LongMacFilename
   * LongMacFilename19880518
   */
    if (line[8] == SPACE) {
    /* PC format filename. */
      line[8] = TERM;
      line[12] = TERM;
      StringElimTrailingBlanks(line);
      if (streq(line+9, "   ")) {
        sprintf(fn, "%s/%s", dirfn, line);
      } else {
        StringElimTrailingBlanks(line+9);
        sprintf(fn, "%s/%s.%s", dirfn, line, line+9);
      }
    } else {
    /* Mac format filename. */
      StringElimTrailingBlanks(line);
      sprintf(fn, "%s/%s", dirfn, line);
    }
    if (!StreamFileModtime(fn, &stat_mod_ts)) {
      Dbg(DBGGEN, DBGBAD, "file listed in DIR.DIR <%s> not found", fn);
      continue;
    }
    if (TsGT(&stat_mod_ts, &copy_ts)) {
      ts = stat_mod_ts;
    } else {
      if (TA_TimeDOSDateTime(line+23, &dirdir_mod_ts)) ts = dirdir_mod_ts;
      else ts = stat_mod_ts;
    }
    r = DirectoryCreate(fn, line, &ts, r);
    memset(line, 0, LINELEN);
  }
  StreamClose(stream);
  return(r);
}

#ifdef SOLARIS
Directory *DirectoryRead1(char *dirfn)
{
  char		fn[FILENAMELEN];
  Ts		ts;
  struct dirent	*direntp;
  DIR		*dirp;
  Directory	*r;

  if (NULL == (dirp = opendir(dirfn))) {
    Dbg(DBGGEN, DBGBAD, "DirectoryRead1: directory <%s> not found", dirfn);
    return NULL;
  }
  r = NULL;
  while ((direntp = readdir( dirp )) != NULL) {
    if (streq(direntp->d_name, ".") ||
        streq(direntp->d_name, "..")) {
      continue;
    }
    sprintf(fn, "%s/%s", dirfn, direntp->d_name);
    if (!StreamFileModtime(fn, &ts)) {
      Dbg(DBGGEN, DBGBAD, "DirectoryRead1: file <%s> not found", fn);
      continue;
    }
    r = DirectoryCreate(fn, direntp->d_name, &ts, r);
  }
  closedir(dirp);
  return(r);
}
#endif

#ifdef GCC
#include <dirent.h>
Directory *DirectoryRead1(char *dirfn)
{
  char		fn[FILENAMELEN];
  Ts		ts;
  struct dirent	*direntp;
  DIR		*dirp;
  Directory	*r;

  if (NULL == (dirp = opendir(dirfn))) {
    Dbg(DBGGEN, DBGBAD, "DirectoryRead1: directory <%s> not found", dirfn);
    return NULL;
  }
  r = NULL;
  while ((direntp = readdir( dirp )) != NULL) {
    if (streq(direntp->d_name, ".") ||
        streq(direntp->d_name, "..")) {
      continue;
    }
    sprintf(fn, "%s/%s", dirfn, direntp->d_name);
    if (!StreamFileModtime(fn, &ts)) {
      Dbg(DBGGEN, DBGBAD, "DirectoryRead1: file <%s> not found", fn);
      continue;
    }
    r = DirectoryCreate(fn, direntp->d_name, &ts, r);
  }
  closedir(dirp);
  return(r);
}
#endif

Directory *DirectoryRead(char *dirfn)
{
/* exhaustive ifdefs */
#ifdef GCC
  return(DirectoryRead1(dirfn));
#endif
#ifdef SOLARIS
  return(DirectoryRead1(dirfn));
#endif
#ifdef SUNOS
  return(DirectoryReadDIRDIR(dirfn));
#endif
#ifdef MACOS
  return(DirectoryReadDIRDIR(dirfn));
#endif
#ifdef NEXT
  return(DirectoryReadDIRDIR(dirfn));
#endif
}

Directory *DirectoryReadEnv(char *dirfn)
{
  char fn[FILENAMELEN];
  sprintf(fn, "%s/%s", TTRoot, dirfn);
  return DirectoryRead(fn);
}

/* Sleep */

void Sleep(int secs)
{
  sleep(secs);
}

void SleepMs(int ms)
{
#ifdef GCC
  usleep(1000*(ms % 1000));
#endif
  Sleep(ms / 1000);
}

/* Mem */

/* Three memory allocation alternatives are provided.
 * You may use either:
 * (1) #define QALLOC
 * qalloc works by mallocing a large chunk of memory on startup,
 * so that each object doesn't have to be malloced separately.
 * For some configurations (such as MacOS), this may speed program
 * startup. qalloc is only used during initial load of the database
 * since it does not know how to free. After initial load (as
 * indicated by the Starting global variable), malloc is used.
 *     -OR-
 * (2) #define MEMCHECK
 * memcheck can be used to find memory allocation bugs such as leaks
 * or freeing the wrong pointer. It makes the program very slow.
 *     -OR-
 * (3) #define MALLOC
 * malloc will be used in this case.
 */

/* Enable one of the below. MALLOC is a good starting point. */
/*
#define QALLOC
#define MEMCHECK
*/
#define MALLOC

void *nofail_malloc(size_t size)
{
  void	*r;
  if (NULL == (r = malloc(size))) {
    Panic("Out of memory (malloc).");
  }
  return(r);
}

void *nofail_realloc(void *ptr, size_t size)
{
  void	*r;
  if (NULL == (r = realloc(ptr, size))) {
    Panic("Out of memory (realloc).");
  }
  return(r);
}

#ifdef QALLOC

void MemCheckReset()
{
}

void MemCheckPrint()
{
}

size_t	qallocNext;
size_t  qallocMax;
char	*qallocP;

/* MEMCHUNK should be set to approximately the size of the program
 * after startup.
 */
#define	MEMCHUNK	21000000L

#define MEMQ	12   /* indicates object is from qalloc pool */
#define MEMM	85   /* indicates object is malloced */
#define MEMF	107  /* indicates object is not allocated or freed */

#define QALIGN   8L  /* alignment constraint */

void qallocInit()
{
  qallocMax = MEMCHUNK;
  qallocP = nofail_malloc(qallocMax);
  *qallocP = MEMF;
  qallocNext = 0L;
}

int nofail_qalloc_msg;

void *nofail_qalloc(size_t size)
{
  char   *r;

  /* Align */
  if ((size % QALIGN) != 0) size += QALIGN - (size % QALIGN);

  /* Add space for flag */
  size += QALIGN;

  if (size + qallocNext > qallocMax) {
  /* Ran out of qalloc space; revert to malloc. */
    if (nofail_qalloc_msg == 0) {
      Dbg(DBGGEN, DBGBAD, "qalloc: MEMCHUNK should be increased");
      nofail_qalloc_msg = 1;
    }
    r = nofail_malloc(size);
    *r = MEMM;
  } else {
    r = qallocP+qallocNext;
    qallocNext += size;
    *r = MEMQ;
  }
  return (void *)(r + QALIGN);
}

void *MemAlloc1(size_t size, char *typ, Bool force_malloc)
{
  char	*r;

  if ((!Starting) || force_malloc) {
    r = nofail_malloc(size + QALIGN);
    *r = MEMM;
    return (void *)(r + QALIGN);
  }
  return nofail_qalloc(size);
}

void *MemAlloc(size_t size, char *typ)
{
  return(MemAlloc1(size, typ, 0));
}

void *MemRealloc(void *buffer, size_t size, char *typ)
{
  char	*r;

  if (size > 32768) Dbg(DBGGEN, DBGDETAIL, "reallocating %ld bytes", size);
  r = ((char *)buffer)-QALIGN;
  if (MEMQ == *r) {
    r = nofail_qalloc(size);
    memcpy(r, buffer, size);
    return (void *)r;
  } else if (MEMM == *r) {
    r = nofail_realloc(r, size + QALIGN);
    return (void *)(r + QALIGN);
  } else {
    Dbg(DBGGEN, DBGBAD, "MemRealloc");
    r = nofail_malloc(size + QALIGN);
    *r = MEMM;
    memcpy(r + QALIGN, buffer, size);
    return (void *)(r + QALIGN);
  }
}

void MemFree(void *buffer, char *typ)
{
  char *r;
#ifdef maxchecking
  if (buffer == NULL) {
    Dbg(DBGGEN, DBGBAD, "MemFree NULL");
    return;
  }
#endif
  r = ((char *)buffer)-QALIGN;
  if (MEMM == *r) {
    free(r);
  } else if (MEMQ == *r) {
  } else if (MEMF == *r) {
    Dbg(DBGGEN, DBGBAD, "MemFree: freeing already freed");
    Stop();
  } else {
    Dbg(DBGGEN, DBGBAD, "MemFree: area corrupted (possibly already freed)");
    Stop();
  }
  *r = MEMF;
}

#endif

#ifdef MEMCHECK

void qallocInit()
{
}

#define	MAXMEMSTAT	1024
int	MemCheckWasReset;
int	MemCheckCounter;
int	MemCheckLen;
char	*MemCheckTypes[MAXMEMSTAT];
size_t	MemCheckSizes[MAXMEMSTAT];
long	MemCheckCount[MAXMEMSTAT];

void MemCheckReset()
{
  MemCheckLen = 0;
  MemCheckWasReset = 1;
}

int MemCheckFind(size_t size, char *typ)
{
  int	i;
  for (i = 0; i < MemCheckLen; i++) {
    if (streq(typ, MemCheckTypes[i]) && size == MemCheckSizes[i]) {
      return(i);
    }
  }
  if (MemCheckLen == MAXMEMSTAT) {
    Dbg(DBGGEN, DBGBAD, "increase MAXMEMSTAT");
    return(-1);
  }
  i = MemCheckLen;
  MemCheckTypes[i] = typ;
  MemCheckSizes[i] = size;
  MemCheckCount[i] = 0L;
  MemCheckLen++;
  return(i);
}

void MemCheckAlloc(size_t size, char *typ)
{
  int i;
  if (-1 == (i = MemCheckFind(size, typ))) return;
  MemCheckCount[i]++;
}

void MemCheckFree(size_t size, char *typ)
{
  int i;
  if (-1 == (i = MemCheckFind(size, typ))) return;
  MemCheckCount[i]--;
  if ((!MemCheckWasReset) &&
      (MemCheckCount[i] < 0)) {
    Dbg(DBGGEN, DBGBAD, "MemCheckFree: going negative on <%s>", typ);
    Stop();
  }
}

void MemCheckPrint()
{
  MemCheckPrint1(Log);
}

void MemCheckPrint1(FILE *stream)
{
  int	i;
  long	size, total;
  total = 0L;
  fprintf(stream, "%25s %12s %12s %12s\n", "Type", "Bytes per", "Number",
          "Bytes total");
  for (i = 0; i < MemCheckLen; i++) {
    if (MemCheckCount[i] != 0L) {
      size = (MemCheckSizes[i]+sizeof(MemCheck))*MemCheckCount[i];
      fprintf(stream, "%25s %12ld %12ld %12ld\n", MemCheckTypes[i],
              MemCheckSizes[i]+sizeof(MemCheck), MemCheckCount[i], size);
      total += size;
    }
  }
  fprintf(stream, "%25s %12s %12s %12ld\n", "TOTAL", "", "", total);
}

void *MemAlloc1(size_t size, char *typ, Bool force_malloc)
{
  char	*s;
  MemCheck	*mcheck;
  if (MemCheckCounter++ > 25000) {
    MemCheckPrint();
    MemCheckCounter = 0;
  }
  MemCheckAlloc(size, typ);
  if (size > 8192) {
    Dbg(DBGGEN, DBGDETAIL, "allocating %ld bytes", size);
  }
  s = nofail_malloc(size+sizeof(MemCheck));
  mcheck = (MemCheck *)s;
  mcheck->size = size;
  mcheck->typ = typ;
  return((void *)(s+sizeof(MemCheck)));
}

void *MemAlloc(size_t size, char *typ)
{
  return(MemAlloc1(size, typ, 0));
}

void *MemRealloc(void *buffer, size_t size, char *typ)
{
  char	*s;
  MemCheck	*mcheck;
  s = ((char *)buffer) - sizeof(MemCheck);
  mcheck = (MemCheck *)s;
  if (!streq(mcheck->typ, typ)) {
    Dbg(DBGGEN, DBGBAD, "MemRealloc: type mismatch <%s> <%s>",
        typ, mcheck->typ);
    Stop();
  }
  MemCheckFree(mcheck->size, typ);
  MemCheckAlloc(size, typ);
  if (size > 8192) Dbg(DBGGEN, DBGDETAIL, "reallocating %ld bytes", size);
  s = nofail_realloc(s, size+sizeof(MemCheck));
  mcheck = (MemCheck *)s;
  mcheck->size = size;
  mcheck->typ = typ;
  return(s+sizeof(MemCheck));
}

void MemFree(void *buffer, char *typ)
{
  char			*s;
  MemCheck		*mcheck;
#ifdef maxchecking
  if (buffer == NULL) {
    Dbg(DBGGEN, DBGBAD, "MemFree NULL");
    return;
  }
#endif
  s = ((char *)buffer) - sizeof(MemCheck);
  mcheck = (MemCheck *)s;
  if (mcheck->size >= 1000000) {
    Dbg(DBGGEN, DBGBAD, "MemFree: freeing corrupted area");
    Stop();
    return;
  }
  if (!streq(mcheck->typ, typ)) {
    Dbg(DBGGEN, DBGBAD, "MemFree: type mismatch <%s> <%s>",
        typ, mcheck->typ);
    return;
  }
  MemCheckFree(mcheck->size, typ);
  mcheck->size = 1000000;
  free(s);
}

#endif

#ifdef MALLOC

void qallocInit()
{
}

void MemCheckReset()
{
}

void MemCheckPrint()
{
}

void *MemAlloc1(size_t size, char *typ, Bool force_malloc)
{
  return(nofail_malloc(size));
}

void *MemAlloc(size_t size, char *typ)
{
  return(nofail_malloc(size));
}

void *MemRealloc(void *buffer, size_t size, char *typ)
{
  return(nofail_realloc(buffer, size));
}

void MemFree(void *buffer, char *typ)
{
  free(buffer);
}

#endif

/* Length */

Bool IsLengthStem(char *s)
{
  return(streq(s, "m") ||
         streq(s, "km") ||
         streq(s, "mi") ||
         streq(s, "in") ||
         streq(s, "ft"));
}

Float LengthParse(char *s, char *stem)
{
  if (streq(s, "na")) return(FLOATNA);
  if (streq(stem, "m")) return(FloatParse(s, -1));
  if (streq(stem, "km")) return(1000.0*FloatParse(s, -1));
  else if (streq(stem, "mi")) return(METERSPERSMILE*FloatParse(s, -1));
  else if (streq(stem, "in")) return(METERSPERINCH*FloatParse(s, -1));
  else if (streq(stem, "ft")) return(METERSPERFOOT*FloatParse(s, -1));
  else {
    Dbg(DBGGEN, DBGBAD, "LengthParse: trouble parsing <%s>", s);
    return(FLOATERR);
  }
}

/* Velocity */

Bool IsVelocityStem(char *s)
{
  return(streq(s, "mps") ||
         streq(s, "mph") ||
         streq(s, "kmph"));
}

Float VelocityParse(char *s, char *stem)
{
  if (streq(s, "na")) return(FLOATNA);
  if (streq(stem, "mps")) {
    return(FloatParse(s, -1));
  } else if (streq(stem, "mph")) {
    return(METERSPERSMILE*(FloatParse(s, -1)/3600.0));
  } else if (streq(stem, "kmph")) {
    return(METERSPERSMILE*(FloatParse(s, -1)/3.6));
  } else {
    Dbg(DBGGEN, DBGBAD, "VelocityParse: trouble parsing <%s>", s);
    return(FLOATERR);
  }
}

/* Freq */

Bool IsFreqStem(char *s)
{
  return(streq(s, "hz") ||
         streq(s, "khz") ||
         streq(s, "mhz") ||
         streq(s, "ghz") ||
         streq(s, "ang"));
}

Float FreqParse(char *s, char *stem)
{
  if (streq(s, "na")) return(FLOATNA);
  if (streq(stem, "hz")) return(FloatParse(s, -1));
  else if (streq(stem, "khz")) return(1000.0*FloatParse(s, -1));
  else if (streq(stem, "mhz")) return(1000000.0*FloatParse(s, -1));
  else if (streq(stem, "ghz")) return(1000000000.0*FloatParse(s, -1));
  else if (streq(stem, "ang")) return(SPEEDOFLIGHT/(1e-10*FloatParse(s, -1)));
  else {
    Dbg(DBGGEN, DBGBAD, "FreqParse: trouble parsing <%s>", s);
    return(FLOATERR);
  }
}

/* Mass */

Bool IsMassStem(char *s)
{
  return(streq(s, "g") ||
         streq(s, "lbs"));
}

Float MassParse(char *s, char *stem)
{
  if (streq(s, "na")) return(FLOATNA);
  if (streq(stem, "g")) return(FloatParse(s, -1));
  else if (streq(stem, "lbs")) return(GRAMSPERAPOUND*FloatParse(s, -1));
    /* todo: na proof. */
  else {
    Dbg(DBGGEN, DBGBAD, "MassParse: trouble parsing <%s>", s);
    return(FLOATERR);
  }
}

/* Bytes */

Bool IsBytesStem(char *s)
{
  return(streq(s, "byte") ||
         streq(s, "Kbyte") ||
         streq(s, "Mbyte") ||
         streq(s, "Gbyte") ||
         streq(s, "Tbyte"));
}

Float BytesParse(char *s, char *stem)
{
  if (streq(stem, "byte")) return(FloatParse(s, -1));
  else if (streq(stem, "Kbyte")) return(1024.0*FloatParse(s, -1));
  else if (streq(stem, "Mbyte")) return(1024.0*1024.0*FloatParse(s, -1));
  else if (streq(stem, "Gbyte")) return(1024.0*1024.0*1024.0*FloatParse(s, -1));
  else if (streq(stem, "Tbyte")) {
    return(1024.0*1024.0*1024.0*1024.0*FloatParse(s, -1));
  }
  Dbg(DBGGEN, DBGBAD, "BytesParse: trouble parsing <%s> ,%s>", s, stem);
  return(0.0);
}

/* Currency */

Bool IsCurrencyStem(char *s)
{
  return(streq(s, "$") ||
         streq(s, "F"));
}

Obj *CurrencyParse(char *s, char *stem)
{
  if (streq(stem, "$")) {
    return(NumberToObjClass((Float)FloatParse(s, -1), N("USD")));
  } else if (streq(stem, "F")) {
    return(NumberToObjClass((Float)FloatParse(s, -1), N("FRF")));
  } else {
    Dbg(DBGGEN, DBGBAD, "CurrencyParse: trouble parsing <%s>", s);
    return(NumberToObj((Float)FloatParse(s, -1)));
  }
}

/* Number */

char *NumberStem(char *s)
{
  char *p;
  p = s;
  while (*p && (FloatIsChar(*p) || (TREE_RANGE == (unsigned char)*p))) {
    p++;
  }
  return((p == s) ? NULL : p);
}

/* Keep ObjPrintNumber consistent with this. */
Obj *NumberParse(char *s, char *stem)
{
  if (streq(stem, "u")) {
    return(NumberToObjClass(FloatParse(s, -1), NULL));
  }
  if (streq(stem, "pc")) {
    return(NumberToObjClass(.01*FloatParse(s, -1), NULL));
  }
  if (IsDurStem(stem)) {
    return(NumberToObjClass((Float)DurParse(s, stem),N("second")));
  }
  if (IsLengthStem(stem)) {
    return(NumberToObjClass((Float)LengthParse(s, stem), N("distance-meter")));
  }
  if (IsVelocityStem(stem)) {
    return(NumberToObjClass((Float)VelocityParse(s, stem),
                            N("meter-per-second")));
  }
  if (IsMassStem(stem)) {
    return(NumberToObjClass((Float)MassParse(s, stem), N("gram")));
  }
  if (IsFreqStem(stem)) {
    return(NumberToObjClass((Float)FreqParse(s, stem), N("hertz")));
  }
  if (IsBytesStem(stem)) {
    return(NumberToObjClass(BytesParse(s, stem), N("byte")));
  }
  if (IsCurrencyStem(stem)) {
    return(CurrencyParse(s, stem));
  }
  Dbg(DBGOBJ, DBGBAD, "unknown stem type <%s> <%s>", s, stem);
  return(NumberToObjClass(FloatParse(s, -1), NULL));
}

/* Token */

Bool IsUnprocessedTail(char *s)
{
  return(StringTailEq(s, "usch") ||
         StringTailEq(s, "frch") ||
         StringTailEq(s, "usca") ||
         StringTailEq(s, "frca") ||
         StringTailEq(s, "dbw") ||
         StringTailEq(s, "coor") ||
         StringTailEq(s, "km2") ||
         StringTailEq(s, "mi2"));
}

Bool IsStem(char *s)
{
  return(streq(s, "u") ||
         streq(s, "pc") ||
         IsDurStem(s) ||
         IsLengthStem(s) ||
         IsVelocityStem(s) ||
         IsMassStem(s) ||
         IsFreqStem(s) ||
         IsBytesStem(s) ||
         IsCurrencyStem(s));
}

Obj *RangeParse(char *tok1, char *tok2, char *stem)
{
  Obj	*obj1, *obj2, *units1, *units2;
  obj1 = NumberParse(tok1, stem);
  units1 = ObjToNumberClass(obj1);
  obj2 = NumberParse(tok2, stem);
  units2 = ObjToNumberClass(obj2);
  /* todo: For now, ranges are averaged instead of both numbers being stored. */
  return(NumberToObjClass(.5*(ObjToNumber(obj1)+ObjToNumber(obj2)), units2));
}

Obj *TokenStringParse(char *s0)
{
  int	len;
  char	class[OBJNAMELEN], *s;
  if (streq(s0, "-Inf")) {
    return(NumberToObj(FLOATNEGINF));
  } else if (streq(s0, "+Inf")) {
    return(NumberToObj(FLOATPOSINF));
  } else if (streq(s0, "Inf")) {
    return(NumberToObj(FLOATPOSINF));
  } else if (s0[0] == DQUOTE) {
    len = strlen(s0);
    if (s0[len-1] == DQUOTE) {
      s0[len-1] = TERM;
    } else {
      Dbg(DBGOBJ, DBGBAD, "unterminated string <%s>", s0);
    }
    return(StringToObj(s0+1, N("string"), 0));
  } else if (StringHeadEqual("STRING:", s0)) {
  /* STRING:email-address:"xxx@yyy.zzz" */
    s = StringReadTo(s0+7, class, OBJNAMELEN, TREE_MICRO_SEP, TERM, TERM);
    len = strlen(s);
    if (s[0] == DQUOTE && s[len-1] == DQUOTE) {
      s[len-1] = TERM;
      return(StringToObj(s+1, N(class), 0));
    } else {
      Dbg(DBGOBJ, DBGBAD, "trouble parsing <%s>", s0);
      return(ObjNA);
    }
  } else {
    return(NULL);
  }
}

/* NUMBER:USD:1.50 */
Obj *TokenNumberParse(char *s)
{
  char	class[OBJNAMELEN];
  s = StringReadTo(s+7, class, OBJNAMELEN, TREE_MICRO_SEP, TERM, TERM);
  if (streq(class, "tod")) {
  /* This seems a bit inconsistent. Convenient though. */
    return(NumberToObjClass((Float)TodParse(s), N(class)));
  } else {
    return(NumberToObjClass(FloatParse(s, -1), N(class)));
  }
}

Obj *TokenGetQuickobj(char *s1, char *s2, char *s3, Obj *class, int createok,
                      Bool *wascreated)
{
  char	lexentry[PHRASELEN], objname[PHRASELEN];
  Obj	*obj;
  if (s1 && s1[0] && s2 && s2[0] && s3 && s3[0]) {
    sprintf(lexentry, "%s %s %s", s1, s2, s3);
    LexEntryToObjName(lexentry, objname, 0, 0);
  } else if (s1 && s1[0] && s2 && s2[0]) {
    sprintf(lexentry, "%s %s", s1, s2);
    LexEntryToObjName(lexentry, objname, 0, 0);
  } else if (s1 && s1[0]) {
    LexEntryToObjName(s1, objname, 0, 0);
  } else {
    StringGenSym(objname, PHRASELEN, "TokenGetQuickobj", NULL);
  }
  if ((obj = NameToObj(objname, OBJ_NO_CREATE))) {
    if (ISA(class, obj)) {
      if (wascreated) *wascreated = 0;
      return(obj);
    } else {
      if (ObjNumParents(obj) == 0) {
        ObjAddIsa(obj, class);
        return(obj);
      }
      Dbg(DBGTREE, DBGBAD, "existing <%s> is not <%s>", M(obj), M(class));
      if (!createok) {
        if (wascreated) *wascreated = 0;
        return(NULL);
      }
      obj = ObjCreateInstance(class, NULL);
      if (wascreated) *wascreated = 1;
      return(obj);
    }
  }
  if (!createok) {
    if (wascreated) *wascreated = 0;
    return(NULL);
  }
  obj = NameToObj(objname, OBJ_CREATE_C);
  ObjAddIsa(obj, class);
  if (wascreated) *wascreated = 1;
  return(obj);
}

Obj *TokenGetCountry(char *country)
{
  return(TokenGetQuickobj(country, NULL, NULL, N("country"), 1, NULL));
}

Obj *TokenGetState(char *state, char *country)
{
  Obj	*obj;
  Bool	wascreated;
  if ((obj = TokenGetQuickobj(state, NULL, NULL, N("polity-state"), 0, NULL))) {
    return(obj);
  }
  obj = TokenGetQuickobj(state, country, NULL, N("polity-state"), 1,
                         &wascreated);
  if (wascreated) {
    if (country[0]) {
      DbAssert(&TsRangeAlways,
               L(N("cpart-of"), obj, TokenGetCountry(country), E));
    }
  }
  return(obj);
}

Obj *TokenGetCity(char *city, char *state, char *country)
{
  Obj	*obj;
  Bool	wascreated;
  if ((obj = TokenGetQuickobj(city, NULL, NULL, N("city"), 0, NULL))) {
    return(obj);
  }
  if ((obj = TokenGetQuickobj(city, state, NULL, N("city"), 0, NULL))) {
    return(obj);
  }
  if (city[0] && state[0] && country[0]) {
    obj = TokenGetQuickobj(city, state, country, N("city"), 1, &wascreated);
  } else if (city[0] && country[0]) {
    obj = TokenGetQuickobj(city, country, NULL, N("city"), 1, &wascreated);
  } else {
    Dbg(DBGTREE, DBGBAD, "missing country <%s> <%s> <%s>", city, state,
        country);
    obj = TokenGetQuickobj(city, NULL, NULL, N("city"), 1, &wascreated);
  }
  if (wascreated) {
    if (state[0]) {
      DbAssert(&TsRangeAlways,
               L(N("cpart-of"), obj, TokenGetState(state, country), E));
    } else if (country[0]) {
      DbAssert(&TsRangeAlways,
               L(N("cpart-of"), obj, TokenGetCountry(country), E));
    }
    /* todo: This is incorrectly skipping levels such as counties, departments:
     * but if this is important then the city can be defined in the Geography
     * file.
     */
  }
  return(obj);
}

Obj *TokenGetStreet(char *street, char *city)
{
  Obj	*obj;
  Bool	wascreated;
  if ((obj = TokenGetQuickobj(street, NULL, NULL, N("street"), 0, NULL))) {
    return(obj);
  }
  obj = TokenGetQuickobj(street, city, NULL, N("street"), 1, &wascreated);
/* Something like:
  if (wascreated) {
    DbAssert(&TsRangeAlways, L(N("city-of"), obj, city_obj, E));
    Also deal with streets which run across multiple cities.
  }
 */
  return(obj);
}

Obj *TokenGetHuman(char *type, char *lexentry)
{
  Obj	*obj;
  Bool	wascreated;
  if (lexentry[0] == TERM) {
    Dbg(DBGTREE, DBGBAD, "expected lexentry");
    return(NULL);
  }
  StringElimChar(lexentry, DQUOTE);
  obj = TokenGetQuickobj(lexentry, NULL, NULL, N("human"), 1, &wascreated);
  if (type[0] != 'G') {
    LexEntryReadName(lexentry, obj,
                     (type[0] == 'M' ? "M" : (type[0] == 'F' ? "F" : "")));
    /* todo: Need to handle group names in a different fashion. */
  }
  if (wascreated) {
    switch (type[0]) {
      case 'M':
        DbAssert(&TsRangeAlways, L(N("male"), obj, E));
        break;
      case 'F':
        DbAssert(&TsRangeAlways, L(N("female"), obj, E));
        break;
      case 'H':
        break;
      case 'G':
        ObjAddIsa(obj, N("group"));
          /* todo: Note TokenGetQuickobj already does ObjAddIsa. */
        break;
      default:
        Dbg(DBGTREE, DBGBAD, "TokenGetHuman <%c>", type[0]);
    }
  }
  return(obj);
}

Obj *TokenGetBuilding(char *lexentry, char *streetnumber, char *street,
                     char *postalcode, char *city, char *state, char *country)
{
  Obj	*obj;
  Bool	wascreated;
  if (lexentry[0] &&
      ((obj = TokenGetQuickobj(lexentry, NULL, NULL, N("building"),
                               0, NULL)))) {
    return(obj);
  }
  if ((obj = TokenGetQuickobj(streetnumber, street, NULL, N("building"), 0,
                              NULL))) {
    return(obj);
  }
  if (lexentry[0]) {
    obj = TokenGetQuickobj(lexentry, NULL, NULL, N("building"), 1, &wascreated);
  } else {
    obj = TokenGetQuickobj(streetnumber, street, city, N("building"), 1,
                           &wascreated);
  }
  if (wascreated) {
    DbAssert(&TsRangeAlways, L(N("street-number-of"), obj,
                               StringToObj(streetnumber, N("sn"), 0), E));
    DbAssert(&TsRangeAlways,
             L(N("street-of"), obj, TokenGetStreet(street, city), E));
    DbAssert(&TsRangeAlways, L(N("postal-code-of"), obj,
             StringToObj(postalcode, N("zip"), 0), E));
    DbAssert(&TsRangeAlways,
             L(N("polity-of"), obj, TokenGetCity(city, state, country), E));
  }
  return(obj);
}

Obj *TokenGetFloor(char *lexentry, char *level, char *streetnumber,
                   char *street, char *postalcode, char *city, char *state,
                   char *country)
{
  Obj	*obj, *building;
  Bool	wascreated;
  if (lexentry[0] &&
      (obj = TokenGetQuickobj(lexentry, NULL, NULL, N("floor"), 0, NULL))) {
    return(obj);
  }
  building = TokenGetBuilding("", streetnumber, street, postalcode, city, state,
                             country);
  if (lexentry[0]) {
    obj = TokenGetQuickobj(lexentry, NULL, NULL, N("floor"), 1, &wascreated);
  } else {
    obj = TokenGetQuickobj(level, M(building), NULL, N("floor"), 1,
                           &wascreated);
  }
  if (wascreated) {
    DbAssert(&TsRangeAlways, L(N("level-of"), obj, 
                               NumberToObj((Float)IntParse(level, -1)), E));
    DbAssert(&TsRangeAlways, L(N("cpart-of"), obj, building, E));
  }
  return(obj);
}

Obj *TokenGetApartment(char *lexentry, char *apt, char *level,
                      char *streetnumber, char *street, char *postalcode,
                      char *city, char *state, char *country)
{
  Obj	*obj, *floor_obj;
  Bool	wascreated;
  if (lexentry[0] &&
      (obj = TokenGetQuickobj(lexentry, NULL, NULL, N("apartment"), 0, NULL))) {
    return(obj);
  }
  floor_obj = TokenGetFloor("", level, streetnumber, street, postalcode,
                           city, state, country);
  if (lexentry[0]) {
    obj = TokenGetQuickobj(lexentry, NULL, NULL, N("apartment"), 1,
                           &wascreated);
  } else {
    obj = TokenGetQuickobj(apt, M(floor_obj), NULL, N("apartment"), 1,
                          &wascreated);
  }
  if (wascreated) {
    DbAssert(&TsRangeAlways, L(N("apartment-number-of"), obj,
                               StringToObj(apt, NULL, 0), E));
    DbAssert(&TsRangeAlways, L(N("cpart-of"), obj, floor_obj, E));
  }
  return(obj);
}

Obj *TokenGetRoom(char *lexentry, char *room, char *apt, char *level,
                 char *streetnumber, char *street, char *postalcode,
                 char *city, char *state, char *country)
{
  Obj	*obj, *apt_obj;
  Bool	wascreated;
  if (lexentry[0] &&
      (obj = TokenGetQuickobj(lexentry, NULL, NULL, N("room"), 0, NULL))) {
    return(obj);
  }
  apt_obj = TokenGetApartment("", level, apt, streetnumber, street,
                             postalcode, city, state, country);
  if (lexentry[0]) {
     obj = TokenGetQuickobj(lexentry, NULL, NULL, N("room"), 1, &wascreated);
  } else {
    obj = TokenGetQuickobj(room, M(apt_obj), NULL, N("room"), 1, &wascreated);
  }
  if (wascreated) {
    DbAssert(&TsRangeAlways, L(N("room-number-of"), obj,
                               StringToObj(room, NULL, 0), E));
    DbAssert(&TsRangeAlways, L(N("cpart-of"), obj, apt_obj, E));
  }
  return(obj);
}

Obj *TokenParseQiBuilding(char *type, char *text)
{
  char *s;
  char lexentry[PHRASELEN], room[PHRASELEN], apt[PHRASELEN], level[PHRASELEN];
  char streetnumber[PHRASELEN], street[PHRASELEN], postalcode[PHRASELEN];
  char city[PHRASELEN], state[PHRASELEN], country[PHRASELEN];

  s = text;
  if (s[0] == TERM) {
    Dbg(DBGTREE, DBGBAD, "expected lexentry: <%s>", text);
    return(NULL);
  }
  s = StringReadTo(s, lexentry, PHRASELEN, TREE_MICRO_SEP, TERM, TERM);
  if (type[0] == 'F' || type[0] == 'A' || type[0] == 'R') {
    if (s[0] == TERM) {
      Dbg(DBGTREE, DBGBAD, "expected level: <%s>", text);
      return(NULL);
    }
    s = StringReadTo(s, level, PHRASELEN, TREE_MICRO_SEP, TERM, TERM);
  } else level[0] = TERM; /* REDUNDANT */
  if (type[0] == 'A' || type[0] == 'R') {
    if (s[0] == TERM) {
      Dbg(DBGTREE, DBGBAD, "expected apt: <%s>", text);
      return(NULL);
    }
    s = StringReadTo(s, apt, PHRASELEN, TREE_MICRO_SEP, TERM, TERM);
  } else apt[0] = TERM; /* REDUNDANT */
  if (type[0] == 'R') {
    if (s[0] == TERM) {
      Dbg(DBGTREE, DBGBAD, "expected room-number: <%s>", text);
      return(NULL);
    }
    s = StringReadTo(s, room, PHRASELEN, TREE_MICRO_SEP, TERM, TERM);
  } else room[0] = TERM; /* REDUNDANT */
  if (s[0] == TERM) {
    Dbg(DBGTREE, DBGBAD, "expected street-number: <%s>", text);
    return(NULL);
  }
  s = StringReadTo(s, streetnumber, PHRASELEN, TREE_MICRO_SEP, TERM, TERM);
  if (s[0] == TERM) {
    Dbg(DBGTREE, DBGBAD, "expected street-name: <%s>", text);
    return(NULL);
  }
  s = StringReadTo(s, street, PHRASELEN, TREE_MICRO_SEP, TERM, TERM);
  if (s[0] == TERM) {
    Dbg(DBGTREE, DBGBAD, "expected postal-code: <%s>", text);
    return(NULL);
  }
  s = StringReadTo(s, postalcode, PHRASELEN, TREE_MICRO_SEP, TERM, TERM);
  if (s[0] == TERM) {
    Dbg(DBGTREE, DBGBAD, "expected city: <%s>", text);
    return(NULL);
  }
  s = StringReadTo(s, city, PHRASELEN, TREE_MICRO_SEP, TERM, TERM);
  if (s[0] == TERM) {
    Dbg(DBGTREE, DBGBAD, "expected state: <%s>", text);
    return(NULL);
  }
  s = StringReadTo(s, state, PHRASELEN, TREE_MICRO_SEP, TERM, TERM);
  if (s[0] == TERM) {
    Dbg(DBGTREE, DBGBAD, "expected country: <%s>", text);
    return(NULL);
  }
  s = StringReadTo(s, country, PHRASELEN, TREE_MICRO_SEP, TERM, TERM);
  if (s[0] != TERM) {
    Dbg(DBGTREE, DBGBAD, "ignored <%s> at end of <%s>", s, text);
  }

  switch (type[0]) {
    case 'B':
      return(TokenGetBuilding(lexentry, streetnumber, street, postalcode,
                              city, state, country));
    case 'F': 
      return(TokenGetFloor(lexentry, level, streetnumber, street, postalcode,
                           city, state, country));
    case 'A': 
      return(TokenGetApartment(lexentry, apt, level, streetnumber, street,
                               postalcode, city, state, country));
    case 'R': 
      return(TokenGetRoom(lexentry, room, apt, level, streetnumber, street,
                          postalcode, city, state, country));
    default:
      Dbg(DBGTREE, DBGBAD, "TokenParseQiBuilding <%c>", type[0]);
  }
  return NULL;
}

Obj *TokenParseQiPlay(char *type, char *text)
{
  char *s, lexentry[PHRASELEN], playwright[PHRASELEN], composer[PHRASELEN];
  Obj	*obj;
  Bool	wascreated;

  s = text;
  if (s[0] == TERM) {
    Dbg(DBGTREE, DBGBAD, "expected lexentry: <%s>", text);
    return(NULL);
  }
  s = StringReadTo(s, lexentry, PHRASELEN, TREE_MICRO_SEP, TERM, TERM);
  if (s[0] == TERM) {
    Dbg(DBGTREE, DBGBAD, "expected playwright: <%s>", text);
    return(NULL);
  }
  s = StringReadTo(s, playwright, PHRASELEN, TREE_MICRO_SEP, TERM, TERM);
  s = StringReadTo(s, composer, PHRASELEN, TREE_MICRO_SEP, TERM, TERM);
  if (lexentry[0] &&
      (obj = TokenGetQuickobj(lexentry, NULL, NULL, N("media-object-play"),
                              0, NULL))) {
    return(obj);
  }
  if (lexentry[0] == TERM) {
    StringGenSym(lexentry, PHRASELEN, "media-object-play", NULL);
  }
  obj = TokenGetQuickobj(lexentry, NULL, NULL, N("media-object-play"),
                         1, &wascreated);
  if (wascreated) {
    if (playwright[0]) {
      DbAssert(&TsRangeAlways,
               L(N("writer-of"), obj, TokenGetHuman("HUMAN", playwright), E));
    }
    if (composer[0]) {
      DbAssert(&TsRangeAlways,
               L(N("composer-of"), obj, TokenGetHuman("HUMAN", composer), E));
    }
  }
  return(obj);
}

Obj *TokenParseQiOpera(char *type, char *text)
{
  char *s, lexentry[PHRASELEN], librettist[PHRASELEN], composer[PHRASELEN];
  Obj	*obj;
  Bool	wascreated;

  s = text;
  if (s[0] == TERM) {
    Dbg(DBGTREE, DBGBAD, "expected lexentry: <%s>", text);
    return(NULL);
  }
  s = StringReadTo(s, lexentry, PHRASELEN, TREE_MICRO_SEP, TERM, TERM);
  if (s[0] == TERM) {
    Dbg(DBGTREE, DBGBAD, "expected composer: <%s>", text);
    return(NULL);
  }
  s = StringReadTo(s, composer, PHRASELEN, TREE_MICRO_SEP, TERM, TERM);
  s = StringReadTo(s, librettist, PHRASELEN, TREE_MICRO_SEP, TERM, TERM);
  if (lexentry[0] &&
      (obj = TokenGetQuickobj(lexentry, NULL, NULL, N("opera"), 0, NULL))) {
    return(obj);
  }
  if (lexentry[0] == TERM) StringGenSym(lexentry, PHRASELEN, "opera", NULL);
  obj = TokenGetQuickobj(lexentry, NULL, NULL, N("opera"), 1, &wascreated);
  if (wascreated) {
    if (composer[0]) {
      DbAssert(&TsRangeAlways,
               L(N("composer-of"), obj, TokenGetHuman("HUMAN", composer), E));
    }
    if (librettist[0]) {
      DbAssert(&TsRangeAlways,
               L(N("librettist-of"), obj,
                 TokenGetHuman("HUMAN", librettist), E));
    }
  }
  return(obj);
}

Obj *TokenParseQiDance(char *type, char *text)
{
  char *s, lexentry[PHRASELEN], choreographer[PHRASELEN], composer[PHRASELEN];
  Obj	*obj;
  Bool	wascreated;

  s = text;
  if (s[0] == TERM) {
    Dbg(DBGTREE, DBGBAD, "expected lexentry: <%s>", text);
    return(NULL);
  }
  s = StringReadTo(s, lexentry, PHRASELEN, TREE_MICRO_SEP, TERM, TERM);
  if (s[0] == TERM) {
    Dbg(DBGTREE, DBGBAD, "expected choreographer: <%s>", text);
    return(NULL);
  }
  s = StringReadTo(s, choreographer, PHRASELEN, TREE_MICRO_SEP, TERM, TERM);
  s = StringReadTo(s, composer, PHRASELEN, TREE_MICRO_SEP, TERM, TERM);
  if (lexentry[0] &&
      (obj = TokenGetQuickobj(lexentry, NULL, NULL, N("dance"), 0, NULL))) {
    return(obj);
  }
  if (lexentry[0] == TERM) StringGenSym(lexentry, PHRASELEN, "dance", NULL);
  obj = TokenGetQuickobj(lexentry, NULL, NULL, N("dance"), 1, &wascreated);
  if (wascreated) {
    if (choreographer[0]) {
      DbAssert(&TsRangeAlways,
                L(N("choreographer-of"), obj,
                  TokenGetHuman("HUMAN", choreographer), E));
    }
    if (composer[0]) {
      DbAssert(&TsRangeAlways,
               L(N("composer-of"), obj, TokenGetHuman("HUMAN", composer), E));
    }
  }
  return(obj);
}

Obj *TokenParseQiMusic(char *type, char *text)
{
  char *s, lexentry[PHRASELEN], composer[PHRASELEN];
  Obj	*obj;
  Bool	wascreated;

  s = text;
  if (s[0] == TERM) {
    Dbg(DBGTREE, DBGBAD, "expected lexentry: <%s>", text);
    return(NULL);
  }
  s = StringReadTo(s, lexentry, PHRASELEN, TREE_MICRO_SEP, TERM, TERM);
  s = StringReadTo(s, composer, PHRASELEN, TREE_MICRO_SEP, TERM, TERM);
  /* todo: Try to classify type of musical-media-object better. */
  if (lexentry[0] &&
      (obj = TokenGetQuickobj(lexentry, NULL, NULL, N("music"), 0, NULL))) {
    return(obj);
  }
  if (lexentry[0] == TERM) StringGenSym(lexentry, PHRASELEN, "music", NULL);
  obj = TokenGetQuickobj(lexentry, NULL, NULL, N("music"), 1, &wascreated);
  if (wascreated) {
    if (composer[0]) {
      DbAssert(&TsRangeAlways, L(N("composer-of"), obj,
                                 TokenGetHuman("HUMAN", composer), E));
    }
  }
  return(obj);
}

Obj *TokenParseQuickInstance(char *text)
{
  char *s, type[PHRASELEN];
  s = text;
  if (s[0] == TERM) {
    Dbg(DBGTREE, DBGBAD, "expected quick-instance type: <%s>", text);
    return(NULL);
  }
  s = StringReadTo(s, type, PHRASELEN, TREE_MICRO_SEP, TERM, TERM);
  if (streq(type, "HUMAN") || streq(type, "MALE") || streq(type, "FEMALE") ||
      streq(type, "GROUP")) {
    return(TokenGetHuman(type, s));
  } else if (streq(type, "BUILDING") || streq(type, "FLOOR") ||
             streq(type, "APARTMENT")|| streq(type, "ROOM")) {
    return(TokenParseQiBuilding(type, s));
  } else if (streq(type, "OPERA")) {
    return(TokenParseQiOpera(type, s));
  } else if (streq(type, "PLAY")) {
    return(TokenParseQiPlay(type, s));
  } else if (streq(type, "DANCE")) {
    return(TokenParseQiDance(type, s));
  } else if (streq(type, "MUSIC")) {
    return(TokenParseQiMusic(type, s));
  } else {
    Dbg(DBGTREE, DBGBAD, "unknown quick-instance type <%s> in <%s>", type,
        text);
    return(NULL);
  } 
}

Obj *TokenToObj(char *s)
{
  char    *stem, *p, tok[DWORDLEN];
  Obj     *obj;
  Ts      ts;
  TsRange tsr;

  if ((obj = TokenStringParse(s))) {
    return(obj);
  } else if (StringHeadEqual("NUMBER:", s)) {
    return(TokenNumberParse(s));
  } else if (StringIn(TREE_MICRO_SEP, s) &&
             (obj = TokenParseQuickInstance(s))) {
    return(obj);
  } else if (LooksLikeTs(s)) {
  /* Used for maturity-of, performance-ts-of, ... */
    TsParse(&ts, s);
    TsRangeSetTs(&tsr, &ts);
    return(TsRangeToObj(&tsr));
  } else if ((NULL == (stem = NumberStem(s))) || !IsStem(stem)) {
    if (FloatIsString(s) && (!streq(s, ".E"))) {
      Dbg(DBGOBJ, DBGBAD, "<%s> not interpreted as number", s);
    }
    if (StringIn('=', s)) Dbg(DBGOBJ, DBGBAD, "<%s> contains =", s);
    if (GR_OBJNAME == s[0]) s++;
    return(NameToObj(s, OBJ_CREATE_A));
  }
  StringCpy(tok, s, DWORDLEN);
  tok[stem-s] = TERM;
  for (p = tok; *p; p++) {
    if ((unsigned char)*p == TREE_RANGE) {
      *p = TERM;
       return(RangeParse(tok, p+1, stem));
    }
  }
  return(NumberParse(tok, stem));
}

/* End of file. */
