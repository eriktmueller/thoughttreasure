/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19940101: begun
 * 19940109: redid for phrasal
 * 19940702: refamiliarization
 * 19941006: adding time parsing
 */

#include "tt.h"
#include "lexentry.h"
#include "lexitem.h"
#include "repbasic.h"
#include "toolsvr.h"

/* Lexitem */

Lexitem *LexitemCreate(char *word, LexEntry *le, char *features)
{
  Lexitem *lb;
  lb = CREATE(Lexitem);
  lb->word = word;
  lb->features = features;
  lb->le = le;
  return(lb);
}

Bool LexitemIsTrace(Lexitem *lexitem)
{
  return(lexitem->word[0] == TERM &&
         lexitem->features[0] == TERM &&
         lexitem->le == NULL);
}

void LexitemMakeTrace(Lexitem *lexitem)
{
  lexitem->word = "";
  lexitem->features = "";
  lexitem->le = NULL;
}

void LexitemPrint(FILE *stream, Lexitem *lexitem)
{
  if (lexitem->word == NULL ||
      lexitem->le == NULL ||
      lexitem->le->srcphrase == NULL ||
      streq(lexitem->word, lexitem->le->srcphrase)) {
    fprintf(stream, "<%s.%s>",
            (lexitem->word && lexitem->word[0]) ? lexitem->word : "",
            lexitem->features ? lexitem->features : "");
  } else {
    fprintf(stream, "<%s.%s:%s>",
            (lexitem->word && lexitem->word[0]) ? lexitem->word : "",
            lexitem->features ? lexitem->features : "",
            lexitem->le ? lexitem->le->srcphrase : "");
  }
}

void LexitemSocketPrint(Socket *skt, Lexitem *li)
{
  char buf[SENTLEN];
#ifdef _GNU_SOURCE
  snprintf(buf, SENTLEN, "\"%s\":\"%s\":\"%s\":\"%s\"",
           li->le ? li->le->srcphrase : "",
           li->le ? li->le->features : "",
           li->word ? li->word : "",
           li->features ? li->features : "");
#else
  sprintf(buf, "\"%s\":\"%s\":\"%s\":\"%s\"",
          li->le ? li->le->srcphrase : "",
          li->le ? li->le->features : "",
          li->word ? li->word : "",
          li->features ? li->features : "");
#endif
  SocketWrite(skt, buf);
}

int LexitemInitialSound(Lexitem *lexitem, Discourse *dc)
{
  int	feature;
  if (lexitem == NULL) return(F_ASPIRE);	/* todo */
  if (lexitem->le && lexitem->le->srcphrase[0] == lexitem->word[0]) {
    if (F_NULL !=
         (feature = FeatureGet(lexitem->le->features, FT_INITIAL_SOUND))) {
      return(feature);
    }
    if (DC(dc).lang == F_FRENCH) {
      return(LexEntryFrenchInitialSound(lexitem->le->srcphrase[0]));
    } else {
      return(LexEntryEnglishInitialSound(lexitem->le->srcphrase[0]));
    }
  } else {
  /* todo */
    if (DC(dc).lang == F_FRENCH) {
      return(LexEntryFrenchInitialSound(lexitem->word[0]));
    } else {
      return(LexEntryEnglishInitialSound(lexitem->word[0]));
    }
  }
}

/* LexitemList */

LexitemList *LexitemListCreate(Lexitem *lexitem, LexitemList *next)
{
  LexitemList *l;
  l = CREATE(LexitemList);
  l->lexitem = lexitem;
  l->next = next;
  return(l);
}

/* End of file. */
