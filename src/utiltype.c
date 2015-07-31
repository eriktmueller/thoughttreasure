/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19941215: begun
 */

#include "tt.h"
#include "repbasic.h"
#include "repstr.h"

/* Conditional rates. */
#define ERROR_RATE	0.005
#define TRANSPOSE_RATE	0.20
#define COLSHIFT_RATE	0.90
#define COMMISSION_RATE	0.50
#define UNFIXED_RATE	0.25

#define MAX_SPILLOVER	3

/* Times in milliseconds. */
#define NORMAL_PAUSE	20
#define SPACE_PAUSE	90
#define ERROR_PAUSE	250

char *TypingAzerty[] = {
  "&é\"'(§è!çà)-",
  "azertyuiop^$",
  "qsdfghjklmù`",
  "wxcvbn,;:=",
  "          ",
  "1234567890°_",
  "AZERTYUIOP¨*",
  "QSDFGHJKLM%£",
  "WXCVBN?./+"
};

char *TypingQwerty[] = {
  "1234567890-=",
  "qwertyuiop[]",
  "asdfghjkl;'\\",
  "zxcvbnm,./",
  "          ",
  "!@#$%^&*()_+",
  "QWERTYUIOP{}",
  "ASDFGHJKL:\"|",
  "ZXCVBNM<>?"
};

Bool TypingLocateChar(char **table, int in, /* RESULTS */ int *row, int *col)
{
  int	i, j;
  for (i = 0; table[i]; i++) {
    for (j = 0; table[i][j]; j++) {
      if (in == table[i][j]) {
        *row = i;
        *col = j;
        return(1);
      }
    }
  }
  return(0);
}

int TypingGetChar(char **table, int row, int col)
{
  if (table[row]) {
    if (col < strlen(table[row])) {
      return(table[row][col]);
    }
  }
  return(TERM);
}

/* Returns whether character was corrupted. */
Bool TypingCorruptChar(char **table, int in, /* RESULTS */ char *out)
{
  int	row, col, inx;
  if (!TypingLocateChar(table, in, &row, &col)) return(0);
  if (WithProbability(COLSHIFT_RATE)) {
    if (WithProbability(0.5)) col++;
    else col--;
  } else {
    if (WithProbability(0.5)) row++;
    else row--;
  }
  if (TERM == (inx = TypingGetChar(table, row, col))) return(0);
  if (inx == SPACE) return(0);
  if (WithProbability(COMMISSION_RATE)) {
    out[0] = inx;
    out[1] = in;
    out[2] = TERM;
  } else {
    out[0] = inx;
    out[1] = TERM;
  }
  return(1);
}

/* Returns whether string was corrupted. */
Bool TypingCorruptString(char **table, char *s, /* RESULTS */ char *wrong,
                         char *right, char **s_out)
{
  if ((strlen(s) >= 2) && WithProbability(TRANSPOSE_RATE)) {
    right[0] = s[0];
    right[1] = s[1];
    right[2] = TERM;
    wrong[0] = s[1];
    wrong[1] = s[0];
    wrong[2] = TERM;
    s += 2;
    *s_out = s;
    return(1);
  } else if (TypingCorruptChar(table, *s, wrong)) {
    right[0] = *s;
    right[1] = TERM;
    s++;
    *s_out = s;
    return(1);
  } 
  return(0);
}

void TypingAddSpillover(char *s, /* RESULTS */ char *wrong, char *right,
                        char **s_out)
{
  int	spill;
  if ((0 < (spill = RandomIntFromTo(0, MAX_SPILLOVER))) &&
      (spill <= strlen(s))) {
    StringAppendLen(wrong, s, spill);
    StringAppendLen(right, s, spill);
    s += spill;
  }           
  *s_out = s;
}

int TypingLastChar; /* todoTHREAD */

void TypingPutChar(FILE *stream, int c)
{
  if (c == NEWLINE && TypingLastChar == NEWLINE) return;
  fputc(c, stream);
  fflush(stream);
  if (c == TypingLastChar) SleepMs(NORMAL_PAUSE+Roll(20));
  else if (c == SPACE) SleepMs(SPACE_PAUSE+Roll(SPACE_PAUSE));
  else SleepMs(NORMAL_PAUSE+Roll(4*NORMAL_PAUSE));
  TypingLastChar = c;
}

void TypingPutString(FILE *stream, char *s)
{
  for (; *s; s++) TypingPutChar(stream, *s);
}

void TypingPutError(FILE *stream, char *wrong, char *right)
{
  int	i, len;
  TypingPutString(stream, wrong);
  if (WithProbability(UNFIXED_RATE)) return;
  SleepMs(ERROR_PAUSE+Roll(ERROR_PAUSE*2));
  for (i = 0, len = strlen(wrong); i < len; i++) {
    fputc(BS, stream);
    fputc(SPACE, stream);
    TypingPutChar(stream, BS);
  }
  TypingPutString(stream, right);
}

void TypingPrint(FILE *stream, char *s, Discourse *dc)
{
  char	**table;
  char	wrong[WORDLEN], right[WORDLEN];
  if (DC(dc).lang == F_FRENCH && DC(dc).dialect != F_CANADIAN) {
    table = TypingAzerty;
  } else {
    table = TypingQwerty;
  }
  while (*s) {
    if (WithProbability(ERROR_RATE) &&
        TypingCorruptString(table, s, wrong, right, &s)) {
      TypingAddSpillover(s, wrong, right, &s);
      TypingPutError(stream, wrong, right);            
    } else {
      TypingPutChar(stream, *s);
      s++;
    }
  }
}

/* End of file. */
