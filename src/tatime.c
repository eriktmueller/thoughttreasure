/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19950414: parsing value names
 * 19951415: redoing time parsing
 */
 
#include "tt.h"
#include "lexentry.h"
#include "lexobjle.h"
#include "repbasic.h"
#include "repchan.h"
#include "repdb.h"
#include "repmisc.h"
#include "repobj.h"
#include "repobjl.h"
#include "repstr.h"
#include "reptime.h"
#include "semdisc.h"
#include "synpnode.h"
#include "ta.h"
#include "taemail.h"
#include "tale.h"
#include "taname.h"
#include "taprod.h"
#include "tatable.h"
#include "tatagger.h"
#include "tatime.h"
#include "utildbg.h"
#include "utillrn.h"

int TimeFixYear(int year)
{
  if (year < 40) return(year + 2000);
  else if (year < 100) return(year + 1900);
  else return(year);
}

/* Example: 16:34 */
Bool TA_TimeHHMM1(char *s, /* RESULTS */ Tod *tod)
{
  if (Char_isdigit(s[0]) && Char_isdigit(s[1]) && s[2] == ':' &&
      Char_isdigit(s[3]) && Char_isdigit(s[4]) && s[5] == TERM) {
    *tod = IntParse(s, 2)*3600L + IntParse(s+3, 2)*60L;
    return(1);
  }
  return(0);
}

/* Examples:
 * 16:34:23
 * 09:34:23
 * 9:34:23
 * 16:34
 */
Bool TA_TimeHHMMSS1(char *s, /* RESULTS */ Tod *tod)
{
  if (Char_isdigit(s[0]) && Char_isdigit(s[1]) && s[2] == ':' &&
      Char_isdigit(s[3]) && Char_isdigit(s[4]) && s[5] == ':' &&
      Char_isdigit(s[6]) && Char_isdigit(s[7])) {
    *tod = IntParse(s, 2)*3600L + IntParse(s+3, 2)*60L + (long)IntParse(s+6, 2);
    return(1);
  }
  if (Char_isdigit(s[0]) && s[1] == ':' && Char_isdigit(s[2]) &&
      Char_isdigit(s[3]) && s[4] == ':' && Char_isdigit(s[5]) &&
      Char_isdigit(s[6])) {
    *tod = IntParse(s, 1)*3600L + IntParse(s+2, 2)*60L + (long)IntParse(s+5, 2);
    return(1);
  }
  return(TA_TimeHHMM1(s, tod));
}

Bool TA_TimeHHMMSSToday(char *sent, HashTable *ht, /* RESULTS */ TsRange *tsr,
                        char **nextp)
{
  char  word[PHRASELEN];
  Tod   tod;
  Ts    ts;
  if (!StringGetWord(word, sent, PHRASELEN, &sent)) return 0;
  if (!TA_TimeHHMMSS1(word, &tod)) return 0;
  TsTodayTime(tod, &ts);
  TsRangeSetTs(tsr, &ts);
  *nextp = sent;
  return 1;
}

/* Example: 10:09p...
 *          09:09p...
 */
Bool TA_TimeHHMMPX(char *s, /* RESULTS */ Tod *tod)
{
  if ((Char_isdigit(s[0]) || s[0] == SPACE) &&
      Char_isdigit(s[1]) && s[2] == ':' &&
      Char_isdigit(s[3]) && Char_isdigit(s[4]) &&
      (s[5] == 'a' || s[5] == 'p') &&
      s[6] == TERM) {
    if (s[0] == SPACE) {
      *tod = IntParse(s+1, 1)*3600L + IntParse(s+3, 2)*60L;
    } else {
      *tod = IntParse(s, 2)*3600L + IntParse(s+3, 2)*60L;
    }
    if (s[5] == 'p') *tod = *tod + TODNOON;
    return(1);
  }
  return(0);
}

/* Example: 09-29-93... */
Bool TA_TimeMMDDYYX(char *s, /* RESULTS */ Ts *ts)
{
  int	month, day, year;
  char	smonth[3], sday[3], syear[3];
  smonth[0] = s[0];
  smonth[1] = s[1];
  smonth[2] = TERM;
  sday[0] = s[3];
  sday[1] = s[4];
  sday[2] = TERM;
  syear[0] = s[6];
  syear[1] = s[7];
  syear[2] = TERM;
  if (!TA_TimeMonthnum1(smonth, &month)) return(0);
  if (!TA_TimeDay1(sday, &day)) return(0);
  if (!TA_TimeYear1(syear, &year)) return(0);
  year = TimeFixYear(year);
  YMDHMSToTs(year, month, day, 0, 0, 0, ts);
  return(1);
}

/* Examples:
 * 0123456789012345
 * 09-29-93  10:09p
 * 05-27-93  10:53a
 */
Bool TA_TimeDOSDateTime(char *s, /* RESULTS */ Ts *ts)
{
  Dur	dur;
  if (!TA_TimeMMDDYYX(s, ts)) return(0);
  if (!TA_TimeHHMMPX(s+10, &dur)) return(0);
  TsIncrement(ts, dur);
  return(1);
}

Bool TA_TimeHHMMX(char *s, /* RESULTS */ Tod *tod)
{
  if (Char_isdigit(s[0]) && Char_isdigit(s[1]) && s[2] == ':' &&
      Char_isdigit(s[3]) && Char_isdigit(s[4])) {
    *tod = IntParse(s, 2)*3600L + IntParse(s+3, 2)*60L;
    return(1);
  }
  return(0);
}

/* Mar 8, 11:53am */
Bool TA_TimeMini(char *sent, /* RESULTS */ Ts *ts, char **nextp)
{
  int		year, month, day;
  Tod		tod;
  year = 1994; /* todo: <year> needs to be input to this routine. */
  if (!TA_TimeMonth(sent, &month, &sent)) return(0);
  if (!TA_TimeDay(sent, &day, &sent)) return(0);
  sent = StringSkipWhitespaceNonNewline(sent);
  if (!TA_TimeHHMMX(sent, &tod)) return(0);
  sent += 5;
  if (StringHeadEqualAdvance("am", sent, &sent)) {
  } else if (StringHeadEqualAdvance("pm", sent, &sent)) {
    tod += TODNOON;
  } else return(0);
  YMDHMSToTsComm(DAYNA, year, month, day, 0, 0, 0, ts);
  TsIncrement(ts, tod); /* todo: time zone adjustments */
  *nextp = sent;
  return(1);
}

/* Examples:
 *
 * Unix:
 * Fri Jan 21 22:52:33 1994
 * Mon Nov  5 09:39:47 EST 1990
 * Thu Jan 20 16:57:47 MET 1994
 * Tue 01 Feb 1994  0:49 EST
 * Fri, 21 Jan 1994 10:38:10 +0000
 * Fri, 21 Jan 94 11:58:29 +0100
 * Fri, 21 Jan 94 11:07:36 GMT
 * Fri, 21 Jan 1994 16:52:07 -0500 (EST)
 * 6 Jul 89 13:42:43 GMT
 * 16 Feb 95 16:00:40 -0700
 *
 * FirstClass:
 * Jeudi 16 Février 1995 18:28:26
 * Jeudi 16 Février 1995 23:00:40
 * Vendredi 17 Février 1995 9:21:53
 * Lundi 19 Décembre 1994 19:38:17
 * Mercredi 4 Janvier 1995 1:19:00
 *
 * <nextp> is returned to point either to NEWLINE or next nonwhitespace
 * as per StringGetWord.
 */
Bool TA_TimeUnixEtc(char *sent, /* RESULTS */ Ts *ts, char **nextp)
{
  int		year, month, day, day_of_the_week, timezone;
  Tod		tod;
  char		word[PHRASELEN];
  if (!StringGetWord(word, sent, PHRASELEN, &sent)) return(0);
  StringElimChar(word, ',');
  if (!TA_TimeDayOfWeek1(word, &day_of_the_week)) {
    day_of_the_week = DAYNA;
  } else {
    if (!StringGetWord(word, sent, PHRASELEN, &sent)) return(0);
  }
  if (!TA_TimeDay1(word, &day)) {
  /* Fri ^ Jan 21 22:52:33 1994 */
    if (!TA_TimeMonth1(word, &month)) return(0);
    /* Fri Jan ^ 21 22:52:33 1994 */
    if (!StringGetWord(word, sent, PHRASELEN, &sent)) return(0);
    if (!TA_TimeDay1(word, &day)) return(0);
    /* Fri Jan 21 ^ 22:52:33 1994 */
    if (!StringGetWord(word, sent, PHRASELEN, &sent)) return(0);
    if (!TA_TimeHHMMSS1(word, &tod)) return(0);
    /* Fri Jan 21 22:52:33 ^ 1994
     * Mon Nov  5 09:39:47 ^ EST 1990
     */
    if (!StringGetWord(word, sent, PHRASELEN, &sent)) return(0);
    if (!TA_TimeYear1(word, &year)) {
      if (!TA_TimeTimezone1(word, &timezone)) return(0);
      if (!StringGetWord(word, sent, PHRASELEN, &sent)) return(0);
      if (!TA_TimeYear1(word, &year)) return(0);
    } else timezone = -5; /* todo */
  } else {
  /* Tue 01 ^ Feb 1994  0:49 EST */
    if (!TA_TimeMonth(sent, &month, &sent)) return(0);
    if (!TA_TimeYear(sent, &year, &sent)) return(0);
    year = TimeFixYear(year);
    /*
     * Tue 01 Feb 1994  ^ 0:49 EST
     * Fri, 21 Jan 1994 ^ 16:52:07 -0500 (EST)
     */
    if (!StringGetWord(word, sent, PHRASELEN, &sent)) return(0);
    if (!TA_TimeHHMMSS1(word, &tod)) {
      if (!TA_TimeHHMM1(word, &tod)) return(0);
    }
    /*
     * Fri, 21 Jan 94 11:58:29 ^ +0100
     * Fri, 21 Jan 94 11:07:36 ^ GMT
     * Fri, 21 Jan 1994 16:52:07 ^ -0500 (EST)
     * Jeudi 16 Février 1995 18:28:26 ^
     */
    if (sent[0] == NEWLINE || (!StringGetWord(word, sent, PHRASELEN, &sent))) {
      timezone = -5; /* todo */
    } else {
      if (IntIsString(word)) {
        timezone = IntParse(word, -1);
      } else if (!TA_TimeTimezone1(word, &timezone)) return(0);
    }
  }
  YMDHMSToTsComm(day_of_the_week, year, month, day, 0, 0, 0, ts);
  TsIncrement(ts, tod); /* todo: time zone adjustments */
  *nextp = sent;
  return(1);
}

/* 12 */
Bool TA_TimeMonthnum1(char *word, /* RESULTS */ int *month)
{
  if (!IntIsString(word)) return(0);
  *month = atoi(word);
  return((*month >= 1) && (*month <= 12));
}

/* 1925
 * '25
 */
Bool TA_TimeYear1(char *word, /* RESULTS */ int *year)
{
  if (word[0] == SQUOTE) {
    word++;
    if (strlen(word) != 2) return(0);
  }
  if (!IntIsString(word)) return(0);
  *year = atoi(word);
  return((*year >= 1) && (*year <= 3001));
}

/* Parse 2, 3, or 4-digit year ("78", "'78", "1978") in range 1900-2020. */
Bool TA_TimeYear3(char *word, /* RESULTS */ int *year)
{
  int	year1;
  if (TA_TimeYear1(word, &year1)) {
    year1 = TimeFixYear(year1);
    if (year1 >= 1900 && year1 <= 2020) {
      *year = year1;
      return(1);
    }
  }
  return(0);
}

/* Parse 4-digit years in range 1800-2099. */
Bool TA_TimeYear4(char *word, /* RESULTS */ int *year)
{
  if (((word[0] == '1' && word[1] == '8') ||
       (word[0] == '1' && word[1] == '9') ||
       (word[0] == '2' && word[1] == '0')) &&
      Char_isdigit(word[2]) && Char_isdigit(word[3]) &&
      word[4] == TERM) {
    *year = atoi(word);
    return(1);
  }
  return(0);
}

Bool TA_TimeYear(char *sent, /* RESULTS */ int *year, char **nextp)
{
  char			word[PHRASELEN];
  if (!StringGetWord(word, sent, PHRASELEN, &sent)) return(0);
  if (TA_TimeYear1(word, year)) {
    *nextp = sent;
    return(1);
  }
  *year = INTNA;
  return(0);
}

/* 31 */
Bool TA_TimeDay1(char *word, /* RESULTS */ int *day)
{
  if (!IntIsString(word)) return(0);
  *day = atoi(word);
  return((*day >= 1) && (*day <= 31));
}

Bool TA_TimeDay(char *sent, /* RESULTS */ int *day, char **nextp)
{
  char			word[PHRASELEN];
  if (!StringGetWord(word, sent, PHRASELEN, &sent)) return(0);
  if (TA_TimeDay1(word, day)) {
    *nextp = sent;
    return(1);
  }
  *day = INTNA;
  return(0);
}

/* ^ mardi */
Bool TA_TimeDayOfWeek1(char *word, /* RESULTS */ int *day_of_the_week)
{
  return(TA_ValueNameInt(word, N("day-of-the-week"), day_of_the_week));
}

Bool TA_TimeDayOfWeek(char *sent, /* RESULTS */ int *day_of_the_week,
                      char **nextp)
{
  char			word[PHRASELEN];
  if (!StringGetWord(word, sent, PHRASELEN, &sent)) return(0);
  if (TA_TimeDayOfWeek1(word, day_of_the_week)) {
    *nextp = sent;
    return(1);
  }
  *day_of_the_week = DAYNA;
  return(0);
}

Bool TA_TimeMonth1(char *word, /* RESULTS */ int *month)
{
  return(TA_ValueNameInt(word, N("month-of-the-year"), month));
}

Bool TA_TimeMonth(char *sent, /* RESULTS */ int *month, char **nextp)
{
  char			word[PHRASELEN];
  if (!StringGetWord(word, sent, PHRASELEN, &sent)) return(0);
  if (TA_TimeMonth1(word, month)) {
    *nextp = sent;
    return(1);
  }
  *month = INTNA;
  return(0);
}

Bool TA_TimeTimezone1(char *word, /* RESULTS */ int *timezone)
{
  return(TA_ValueNameInt(word, N("time-zone"), timezone));
}

Bool TA_TimeTimezone(char *sent, /* RESULTS */ int *timezone, char **nextp)
{
  char			word[PHRASELEN];
  if (!StringGetWord(word, sent, PHRASELEN, &sent)) return(0);
  if (TA_TimeTimezone1(word, timezone)) {
    *nextp = sent;
    return(1);
  }
  *timezone = INTNA;
  return(0);
}

/* ^ (mardi) (le) (24 avril) 1944 or (mardi) (le) avril (24) 1944
 *
 * Philosophy: Relaxed is better. todoSCORE. Non-"legal" combinations are
 * accepted where feasible.
 */
Bool TA_Time_Date(char *sent, HashTable *ht, /* RESULTS */ Ts *ts, char **nextp)
{
  int			year, month, day, day_of_the_week, freeme;
  char			word[PHRASELEN];
  IndexEntry	*ie;
  /* (mardi) */
  TA_TimeDayOfWeek(sent, &day_of_the_week, &sent);
  /* skip past (le) */
  while (1) {
    if (!StringGetWord(word, sent, PHRASELEN, &sent)) return(0);
    if (!(ie = LexEntryFindPhrase(ht, word, 2, 0, 0, &freeme))) break;
    if (!IEConceptIs(ie, N("definite-article"))) {
      if (freeme) IndexEntryFree(ie);
      break;
    }
    if (freeme) IndexEntryFree(ie);
  }
  if (TA_TimeDay1(word, &day)) {
  /* 24 ^ avril 1944 */
    if (!StringGetWord(word, sent, PHRASELEN, &sent)) return(0);
    if (TA_ValueNameInt(word, N("month-of-the-year"), &month)) {
    /* 24 avril ^ 1944 */
      if (!StringGetWord(word, sent, PHRASELEN, &sent)) return(0);
      if (TA_TimeYear1(word, &year)) {
      /* 24 avril 1944 ^ */
        goto success;
      }
    }
  } else if (TA_ValueNameInt(word, N("month-of-the-year"), &month)) {
  /* avril ^ (24) 1944 */
    if (!StringGetWord(word, sent, PHRASELEN, &sent)) return(0);
    if (TA_TimeDay1(word, &day)) {
    /* avril 24 ^ 1944 */
      if (!StringGetWord(word, sent, PHRASELEN, &sent)) return(0);
      if (TA_TimeYear1(word, &year)) {
      /* avril 24 1944 ^ */
        goto success;
      }
    } else if (TA_TimeYear1(word, &year)) {
    /* avril 1944 ^ */
      day = 1;
      goto success;
    }
  } else if (TA_TimeYear1(word, &year) && year > 1900 && year < 2100) {
  /* 1944 ^ */
    month = day = 1;
    goto success;
  }
  return(0);
success:
  YMDHMSToTsComm(day_of_the_week, year, month, day, 0, 0, 0, ts);
  *nextp = sent;
  return(1);
}

Bool PNodeTypeClassMatch(PNode *pn, int typ, Obj *class,
                         /* RESULTS */ Obj **obj_out)
{
  LexEntryToObj	*leo;
  if (typ != pn->type) return(0);
  if (class == NULL) return(1);
  if (pn->lexitem && pn->lexitem->le) {
    for (leo = pn->lexitem->le->leo; leo; leo = leo->next) {
      if (ISA(class, leo->obj)) {
	/* Was "&& class != leo->obj" but this prevents
         * recognition of TSRANGE at TOD.
         * "I have an appointment with Amy Newton 
         * on September 21, 1997 at eight pm."
	 */
        *obj_out = leo->obj;
        return(1);
      }
    }
  }
  return(0);
}

int PNodeApplyRule(PNode *pns, size_t startpos, int *lhs_types,
                   Obj **lhs_classes, int len, /* RESULTS */ PNode **pns_out,
                   Obj **objs_out, size_t *stoppos)
{
  int	i;
  PNode	*pn;
  i = 0;
  for (pn = pns; pn; pn = pn->next) {
    if (pn->lowerb > startpos) {
      break;
    } else if (startpos == pn->lowerb) {
      if (PNodeTypeClassMatch(pn, lhs_types[i], lhs_classes[i], &objs_out[i])) {
        pns_out[i] = pn;
        i++;
        *stoppos = pn->upperb;
        if (i == len) {
          return(i);
        }
        startpos = pn->upperb+1L;
      }
    }
  }
  if (i > 0) {
  /* Return with partial match. cf "quarter past six (o'clock)" */
    return(i);
  }
  return(0);
}

Bool PNodeInArray(PNode *pn, PNode **pns, int len)
{
  int	i;
  for (i = 0; i < len; i++) {
    if (pn == pns[i]) return(1);
  }
  return(0);
}

void PNodeEliminateLHSNodes(PNodeList *pnf, PNode **elim_pns, int len)
{
  PNode	*p, *prev;
  prev = NULL;
  for (p = pnf->first; p; p = p->next) {
    if (PNodeInArray(p, elim_pns, len)) {
      /* Splice out. */
      if (prev) {
        prev->next = p->next;
        if (prev->next == NULL) pnf->last = prev;
        /* todoFREE */
      } else {
        pnf->first = p->next;
        if (pnf->first == NULL) pnf->last = NULL;
      }
    } else prev = p;
  }
}

#define MAXPTNLEN	10

Bool TA_Time_Abouts(PNode *pns, size_t startpos, Discourse *dc, Channel *ch)
{
  int		len, lhs_types[MAXPTNLEN];
  Obj		*lhs_classes[MAXPTNLEN], *objs_out[MAXPTNLEN];
  PNode		*pns_out[MAXPTNLEN];
  size_t	stoppos;
  TsRange	*rhs_tsr;

  lhs_types[0] = PNTYPE_LEXITEM; lhs_classes[0] = N("approximate");
  lhs_types[1] = PNTYPE_TSRANGE; lhs_classes[1] = NULL;
  len = 2;
  if (len == PNodeApplyRule(pns, startpos, lhs_types, lhs_classes,
                            len, pns_out, objs_out, &stoppos)) {
    rhs_tsr = TsRangeCopy(pns_out[1]->u.tsr);
    TsSetApprox(&rhs_tsr->startts);
    TsSetApprox(&rhs_tsr->stopts);
    goto success;
  }

  return(0);

success:
  PNodeEliminateLHSNodes(ch->pnf, pns_out, len);	/* todoSCORE */
  ChannelAddPNode(ch, PNTYPE_TSRANGE, 1.0, rhs_tsr, NULL,
                  startpos + (char *)ch->buf, 1 + stoppos + (char *)ch->buf);
  return(1);
}

Bool TA_Time_MergeDateTod(PNode *pns, size_t startpos, Discourse *dc,
                          Channel *ch)
{
  int		len, lhs_types[MAXPTNLEN], rhs_type;
  Obj		*lhs_classes[MAXPTNLEN], *objs_out[MAXPTNLEN];
  PNode		*pns_out[MAXPTNLEN];
  size_t	stoppos;
  void		*rhs_any;
  TsRange	rhs_tsr;

  /* <ts> "at/à" <tod> */
  lhs_types[0] = PNTYPE_TSRANGE; lhs_classes[0] = NULL;
  lhs_types[1] = PNTYPE_LEXITEM; lhs_classes[1] =
    N("prep-temporal-position-tod");
  lhs_types[2] = PNTYPE_TSRANGE; lhs_classes[2] = NULL;
  len = 3;
  if (len == PNodeApplyRule(pns, startpos, lhs_types, lhs_classes,
                            len, pns_out, objs_out, &stoppos)) {
    if ((!TsIsTod(&pns_out[0]->u.tsr->startts)) &&
        TsIsTod(&pns_out[2]->u.tsr->startts)) {
      TsRangeSetNa(&rhs_tsr);
      TsMidnight(&pns_out[0]->u.tsr->startts, &rhs_tsr.startts);
      TsIncrement(&rhs_tsr.startts, (Tod)pns_out[2]->u.tsr->startts.unixts);
      rhs_type = PNTYPE_TSRANGE;
      rhs_any = (void *)TsRangeCopy(&rhs_tsr);
      goto success;
    }
  }

  return(0);

success:
  PNodeEliminateLHSNodes(ch->pnf, pns_out, len);	/* todoSCORE */
  ChannelAddPNode(ch, rhs_type, 1.0, rhs_any, NULL,
                  startpos + (char *)ch->buf, 1 + stoppos + (char *)ch->buf);
  return(1);
}

Bool TA_Time_Combinations(PNode *pns, size_t startpos, Discourse *dc,
                          Channel *ch)
{
  int		len, lhs_types[MAXPTNLEN], rhs_type;
  Obj		*lhs_classes[MAXPTNLEN], *objs_out[MAXPTNLEN];
  PNode		*pns_out[MAXPTNLEN];
  size_t	stoppos;
  void		*rhs_any;
  TsRange	*rhs_tsr, rhs_tsr1;

  /* "on" + <ts> */
  lhs_types[0] = PNTYPE_LEXITEM;
  lhs_classes[0] = N("prep-temporal-position-day");
  lhs_types[1] = PNTYPE_TSRANGE; lhs_classes[1] = NULL;
  len = 2;
  if (len == PNodeApplyRule(pns, startpos, lhs_types, lhs_classes,
                            len, pns_out, objs_out, &stoppos)) {
    if ((!TsIsTod(&pns_out[1]->u.tsr->startts)) &&
        (!TsIsTod(&pns_out[1]->u.tsr->stopts))) {
      rhs_type = PNTYPE_TSRANGE;
      rhs_any = (void *)TsRangeCopy(pns_out[1]->u.tsr);
      goto success;
    }
  }

  /* "in" + <ts> */
  lhs_types[0] = PNTYPE_LEXITEM;
  lhs_classes[0] = N("prep-temporal-position-month-year");
  lhs_types[1] = PNTYPE_TSRANGE; lhs_classes[1] = NULL;
  len = 2;
  if (len == PNodeApplyRule(pns, startpos, lhs_types, lhs_classes,
                            len, pns_out, objs_out, &stoppos)) {
    if ((!TsIsTod(&pns_out[1]->u.tsr->startts)) &&
        (!TsIsTod(&pns_out[1]->u.tsr->stopts))) {
      /* todo: perhaps check that day not specified. */
      rhs_type = PNTYPE_TSRANGE;
      rhs_any = (void *)TsRangeCopy(pns_out[1]->u.tsr);
      goto success;
    }
  }

  /* "in/dans" + <dur> */
  lhs_types[0] = PNTYPE_LEXITEM; lhs_classes[0] = N("prep-dur-relative-to-now");
  lhs_types[1] = PNTYPE_TSRANGE; lhs_classes[1] = NULL;
  len = 2;
  if (len == PNodeApplyRule(pns, startpos, lhs_types, lhs_classes,
                            len, pns_out, objs_out, &stoppos)) {
    if (TsIsDur(&pns_out[1]->u.tsr->startts) &&
        TsIsDur(&pns_out[1]->u.tsr->stopts)) {
      rhs_type = PNTYPE_TSRANGE;
      rhs_any = (void *)TsRangeCopy(pns_out[1]->u.tsr);
    }
  }

  if (DC(dc).lang == F_FRENCH) {
    /* "il y a" + <dur> */
    lhs_types[0] = PNTYPE_LEXITEM; lhs_classes[0] = N("adverb-ago");
    lhs_types[1] = PNTYPE_TSRANGE; lhs_classes[1] = NULL;
    len = 2;
    if (len == PNodeApplyRule(pns, startpos, lhs_types, lhs_classes,
                              len, pns_out, objs_out, &stoppos)) {
      if (TsIsDur(&pns_out[1]->u.tsr->startts) &&
          TsIsDur(&pns_out[1]->u.tsr->stopts)) {
        rhs_tsr = TsRangeCopy(pns_out[1]->u.tsr);
        rhs_tsr->startts.unixts = -rhs_tsr->startts.unixts;
        rhs_tsr->stopts.unixts = -rhs_tsr->stopts.unixts;
        rhs_type = PNTYPE_TSRANGE;
        rhs_any = (void *)rhs_tsr;
      }
    }
  }

  if (DC(dc).lang == F_ENGLISH) {
    /* <dur> + "from now" */
    lhs_types[0] = PNTYPE_TSRANGE; lhs_classes[0] = NULL;
    lhs_types[1] = PNTYPE_LEXITEM;
    lhs_classes[1] = N("adverb-dur-end-embedded");
    len = 2;
    if (len == PNodeApplyRule(pns, startpos, lhs_types, lhs_classes,
                              len, pns_out, objs_out, &stoppos)) {
      if (TsIsDur(&pns_out[0]->u.tsr->startts) &&
          TsIsDur(&pns_out[0]->u.tsr->stopts)) {
        rhs_type = PNTYPE_TSRANGE;
        rhs_any = (void *)TsRangeCopy(pns_out[0]->u.tsr);
      }
    }

    /* <dur> + "ago" */
    lhs_types[0] = PNTYPE_TSRANGE; lhs_classes[0] = NULL;
    lhs_types[1] = PNTYPE_LEXITEM;
    lhs_classes[1] = N("adverb-dur-end-embedded");
    len = 2;
    if (len == PNodeApplyRule(pns, startpos, lhs_types, lhs_classes,
                              len, pns_out, objs_out, &stoppos)) {
      if (TsIsDur(&pns_out[0]->u.tsr->startts) &&
          TsIsDur(&pns_out[0]->u.tsr->stopts)) {
        rhs_tsr = TsRangeCopy(pns_out[0]->u.tsr);
        rhs_tsr->startts.unixts = -rhs_tsr->startts.unixts;
        rhs_tsr->stopts.unixts = -rhs_tsr->stopts.unixts;
        rhs_type = PNTYPE_TSRANGE;
        rhs_any = (void *)rhs_tsr;
      }
    }
  }

  /* "at/à" + <tod> */
  lhs_types[0] = PNTYPE_LEXITEM;
  lhs_classes[0] = N("prep-temporal-position-tod");
  lhs_types[1] = PNTYPE_TSRANGE; lhs_classes[1] = NULL;
  len = 2;
  if (len == PNodeApplyRule(pns, startpos, lhs_types, lhs_classes,
                            len, pns_out, objs_out, &stoppos)) {
    if (TsIsTod(&pns_out[1]->u.tsr->startts) &&
        TsIsTod(&pns_out[1]->u.tsr->stopts)) {
      rhs_type = PNTYPE_TSRANGE;
      rhs_any = (void *)TsRangeCopy(pns_out[1]->u.tsr);
      goto success;
    }
  }

  /* "since/depuis" + <ts/tod> */
  lhs_types[0] = PNTYPE_LEXITEM; lhs_classes[0] = N("prep-dur-start-alone");
  lhs_types[1] = PNTYPE_TSRANGE; lhs_classes[1] = NULL;
  len = 2;
  if (len == PNodeApplyRule(pns, startpos, lhs_types, lhs_classes,
                            len, pns_out, objs_out, &stoppos)) {
    rhs_tsr = TsRangeCopy(pns_out[1]->u.tsr);
    TsSetNa(&rhs_tsr->stopts);
    rhs_type = PNTYPE_TSRANGE;
    rhs_any = (void *)rhs_tsr;
    goto success;
  }

  /* "until/jusqu'à" + <ts/tod> */
  lhs_types[0] = PNTYPE_LEXITEM; lhs_classes[0] = N("prep-dur-stop-alone");
  lhs_types[1] = PNTYPE_TSRANGE; lhs_classes[1] = NULL;
  len = 2;
  if (len == PNodeApplyRule(pns, startpos, lhs_types, lhs_classes,
                            len, pns_out, objs_out, &stoppos)) {
    rhs_tsr = TsRangeCopy(pns_out[1]->u.tsr);
    TsSetNa(&rhs_tsr->startts);
    rhs_type = PNTYPE_TSRANGE;
    rhs_any = (void *)rhs_tsr;
    goto success;
  }

  /* "at/à/vers" + <ts/tod> */
  lhs_types[0] = PNTYPE_LEXITEM;
  lhs_classes[0] = N("prep-temporal-position-tod");
  lhs_types[1] = PNTYPE_TSRANGE; lhs_classes[1] = NULL;
  len = 2;
  if (len == PNodeApplyRule(pns, startpos, lhs_types, lhs_classes,
                            len, pns_out, objs_out, &stoppos)) {
    rhs_tsr = TsRangeCopy(pns_out[1]->u.tsr);
    if (ISA(N("prep-temporal-position-tod-approx"), objs_out[0])) {
      TsSetApprox(&rhs_tsr->startts);
      TsSetApprox(&rhs_tsr->stopts);
    }
    rhs_type = PNTYPE_TSRANGE;
    rhs_any = (void *)rhs_tsr;
    goto success;
  }

  /* "between/entre" + <ts/tod/dur> + "and/et" + <ts/tod/dur> */
  lhs_types[0] = PNTYPE_LEXITEM; lhs_classes[0] = N("prep-dur-between");
  lhs_types[1] = PNTYPE_TSRANGE; lhs_classes[1] = NULL;
  lhs_types[2] = PNTYPE_LEXITEM; lhs_classes[2] = N("and");
  lhs_types[3] = PNTYPE_TSRANGE; lhs_classes[3] = NULL;
  len = 4;
  if (len == PNodeApplyRule(pns, startpos, lhs_types, lhs_classes,
                            len, pns_out, objs_out, &stoppos)) {
    goto fromto;
  }

  /* "from" + <ts/tod/dur> + "to" + <ts/tod/dur> */
  lhs_types[0] = PNTYPE_LEXITEM; lhs_classes[0] = N("prep-dur-start");
  lhs_types[1] = PNTYPE_TSRANGE; lhs_classes[1] = NULL;
  lhs_types[2] = PNTYPE_LEXITEM; lhs_classes[2] = N("prep-dur-stop");
  lhs_types[3] = PNTYPE_TSRANGE; lhs_classes[3] = NULL;
  len = 4;
  if (len == PNodeApplyRule(pns, startpos, lhs_types, lhs_classes,
                            len, pns_out, objs_out, &stoppos)) {
fromto:
    /* todo: Perhaps check that
     * pns_out[3]->u.tsr->startts == pns_out[3]->u.tsr->stopts
     * etc.
     */
    TsRangeSetNa(&rhs_tsr1);
    rhs_tsr1.startts = pns_out[1]->u.tsr->startts;
    rhs_tsr1.stopts = pns_out[3]->u.tsr->startts;
    rhs_type = PNTYPE_TSRANGE;
    rhs_any = (void *)TsRangeCopy(&rhs_tsr1);
    goto success;
  }

  return(0);

success:
  PNodeEliminateLHSNodes(ch->pnf, pns_out, len);	/* todoSCORE */
  ChannelAddPNode(ch, rhs_type, 1.0, rhs_any, NULL,
                  startpos + (char *)ch->buf, 1 + stoppos + (char *)ch->buf);
  return(1);
}

Bool TA_Time_Dur(PNode *pns, size_t startpos, Discourse *dc, Channel *ch)
{
  int		len, lhs_types[MAXPTNLEN];
  Obj		*lhs_classes[MAXPTNLEN], *objs_out[MAXPTNLEN];
  PNode		*pns_out[MAXPTNLEN];
  Dur		rhs_dur;
  TsRange	tsr;
  size_t	stoppos;

  /* "six seconds" */
  lhs_types[0] = PNTYPE_LEXITEM; lhs_classes[0] = N("cardinal-number");
  lhs_types[1] = PNTYPE_LEXITEM; lhs_classes[1] = N("absolute-unit-of-time");
  len = 4;
  if (len == PNodeApplyRule(pns, startpos, lhs_types, lhs_classes,
                            len, pns_out, objs_out, &stoppos)) {
    rhs_dur = DurValueOf(objs_out[0]) * DurCanonicalFactorOf(objs_out[1]);
    goto success;
  }

  /* "a moment ago" */
  lhs_types[0] = PNTYPE_LEXITEM; lhs_classes[0] = N("duration-relative-to-now");
  len = 1;
  if (len == PNodeApplyRule(pns, startpos, lhs_types, lhs_classes,
                            len, pns_out, objs_out, &stoppos)) {
    rhs_dur = SECONDSPERHOURL*DurValueOf(objs_out[0]);
    goto success;
  }

  return(0);

success:
  PNodeEliminateLHSNodes(ch->pnf, pns_out, len);	/* todoSCORE */
  TsRangeSetDurRange(rhs_dur, rhs_dur, &tsr);
  ChannelAddPNode(ch, PNTYPE_TSRANGE, 1.0, TsRangeCopy(&tsr), NULL,
                  startpos + (char *)ch->buf, 1 + stoppos + (char *)ch->buf);
  return(1);
}

Float AdverbHourOfDayToOffset(Obj *adv)
{
  if (ISA(N("adverb-o-clock"), adv)) {
  /* todo: This is ambiguous. Perhaps expressions such as "in the evening"
   * can just add 12 to an existing concept instead of treating it as
   * a contradiction ("6 am in the evening").
   */
    return(0.0);
  } else if (ISA(N("adverb-am"), adv)) {
    return(0.0);
  } else if (ISA(N("adverb-pm"), adv)) {
    return(SECONDSPERHOURL*12.0);
  } else {
    return(0.0);
  }
}

Bool TA_Time_Tod(PNode *pns, size_t startpos, Discourse *dc, Channel *ch,
                 /* RESULTS */ size_t *stoppos_r)
{
  int		len, lhs_types[MAXPTNLEN];
  Obj		*lhs_classes[MAXPTNLEN], *objs_out[MAXPTNLEN];
  PNode		*pns_out[MAXPTNLEN];
  Tod		rhs_tod;
  TsRange	tsr;
  size_t	stoppos;

  if (DC(dc).lang == F_FRENCH) {
    /* "six heures et demie" */
    lhs_types[0] = PNTYPE_LEXITEM; lhs_classes[0] = N("cardinal-number");
    lhs_types[1] = PNTYPE_LEXITEM; lhs_classes[1] = N("hour");
    lhs_types[2] = PNTYPE_LEXITEM; lhs_classes[2] = N("and");
    lhs_types[3] = PNTYPE_LEXITEM; lhs_classes[3] = N("fraction");
    len = 4;
    if (len == PNodeApplyRule(pns, startpos, lhs_types, lhs_classes,
                              len, pns_out, objs_out, &stoppos)) {
      rhs_tod = (SECONDSPERHOURL*DurValueOf(objs_out[0])) +
                ((Tod)(SECONDSPERHOURF*FloatValueOf(objs_out[3])));
      goto success;
    }
  
    /* "six heures moins quart" */
    lhs_types[0] = PNTYPE_LEXITEM; lhs_classes[0] = N("cardinal-number");
    lhs_types[1] = PNTYPE_LEXITEM; lhs_classes[1] = N("hour");
    lhs_types[2] = PNTYPE_LEXITEM; lhs_classes[2] =
      N("prep-time-before-hour-french");
    lhs_types[3] = PNTYPE_LEXITEM; lhs_classes[3] = N("fraction");
    len = 4;
    if (len == PNodeApplyRule(pns, startpos, lhs_types, lhs_classes,
                              len, pns_out, objs_out, &stoppos)) {
      rhs_tod = (SECONDSPERHOURL*DurValueOf(objs_out[0])) -
                ((Tod)(SECONDSPERHOURF*FloatValueOf(objs_out[3])));
      goto success;
    }

    /* "six heures" */
    lhs_types[0] = PNTYPE_LEXITEM; lhs_classes[0] = N("cardinal-number");
    lhs_types[1] = PNTYPE_LEXITEM; lhs_classes[1] = N("hour");
    len = 2;
    if (len == PNodeApplyRule(pns, startpos, lhs_types, lhs_classes,
                              len, pns_out, objs_out, &stoppos)) {
      rhs_tod = SECONDSPERHOURL*DurValueOf(objs_out[0]);
      goto success;
    }
  } else {
    /* "quarter to six (o'clock)" */
    lhs_types[0] = PNTYPE_LEXITEM; lhs_classes[0] = N("fraction");
    lhs_types[1] = PNTYPE_LEXITEM; lhs_classes[1] =
      N("prep-time-before-hour-english");
    lhs_types[2] = PNTYPE_LEXITEM; lhs_classes[2] = N("cardinal-number");
    lhs_types[3] = PNTYPE_LEXITEM; lhs_classes[3] = N("adverb-hour-of-day");
    len = 4;
    if (0 < (len = PNodeApplyRule(pns, startpos, lhs_types, lhs_classes,
                                  len, pns_out, objs_out, &stoppos))) {
      if (len == 4 || len == 3) {
        rhs_tod = (SECONDSPERHOURL*DurValueOf(objs_out[2])) -
                  ((Tod)(SECONDSPERHOURF*FloatValueOf(objs_out[0])));
        if (len == 4) rhs_tod += AdverbHourOfDayToOffset(objs_out[3]);
        goto success;
      }
    }

    /* "quarter past six (o'clock)" */
    lhs_types[0] = PNTYPE_LEXITEM; lhs_classes[0] = N("fraction");
    lhs_types[1] = PNTYPE_LEXITEM; lhs_classes[1] = N("prep-time-after-hour");
    lhs_types[2] = PNTYPE_LEXITEM; lhs_classes[2] = N("cardinal-number");
    lhs_types[3] = PNTYPE_LEXITEM; lhs_classes[3] = N("adverb-hour-of-day");
    len = 4;
    if (0 < (len = PNodeApplyRule(pns, startpos, lhs_types, lhs_classes,
                                  len, pns_out, objs_out, &stoppos))) {
      if (len == 4 || len == 3) {
        rhs_tod = (SECONDSPERHOURL*DurValueOf(objs_out[2])) +
                  ((Tod)(SECONDSPERHOURF*FloatValueOf(objs_out[0])));
        if (len == 4) rhs_tod += AdverbHourOfDayToOffset(objs_out[3]);
        goto success;
      }
    }

    /* "six o'clock" */
    lhs_types[0] = PNTYPE_LEXITEM; lhs_classes[0] = N("cardinal-number");
    lhs_types[1] = PNTYPE_LEXITEM; lhs_classes[1] = N("adverb-hour-of-day");
    len = 2;
    if (0 < (len = PNodeApplyRule(pns, startpos, lhs_types, lhs_classes,
                                  len, pns_out, objs_out, &stoppos))) {
      if (len == 2) {
      /* todo: "|| len == 1" but this triggers on the word "number"
       * and also on any number.
       */
        rhs_tod = SECONDSPERHOURL*DurValueOf(objs_out[0]);
        if (len == 2) rhs_tod += AdverbHourOfDayToOffset(objs_out[1]);
        goto success;
      }
    }
  }

  /* "midnight" */
  lhs_types[0] = PNTYPE_LEXITEM; lhs_classes[0] = N("hour-of-the-day");
  len = 1;
  if (len == PNodeApplyRule(pns, startpos, lhs_types, lhs_classes,
                            len, pns_out, objs_out, &stoppos)) {
    rhs_tod = SECONDSPERHOURL*DurValueOf(objs_out[0]);
    goto success;
  }

  *stoppos_r = SIZENA;
  return(0);

success:
  PNodeEliminateLHSNodes(ch->pnf, pns_out, len);	/* todoSCORE */
  TsRangeSetTodRange(rhs_tod, rhs_tod, &tsr);
  ChannelAddPNode(ch, PNTYPE_TSRANGE, 1.0, TsRangeCopy(&tsr), NULL,
                  startpos + (char *)ch->buf, 1 + stoppos + (char *)ch->buf);
  *stoppos_r = 1+stoppos;
  return(1);
}

/*
 * Case 1: N("det-that") + N("part-of-the-day") [french: + N("adverb-there")]
 *         ce matin-là, cette nuit-là, that morning
 *         (relates to story time)
 * Case 2: N("det-this") + N("part-of-the-day")
 *         ce matin, this morning, this evening
 *         (relates to now)
 * todo:
 * Case 3: N("relative-day-and-part-of-the-day-then")
 *         la veille, the night before
 *         (relates to story time)
 * Case 4: N("relative-day-and-part-of-the-day")
 *         tonight, last night
 *         (relates to now)
 */
Bool TA_RelativeDayAndPartOfTheDay(char *sent, HashTable *ht, Discourse *dc,
                                   /* RESULTS */ TsRange *tsr, char **nextp)
{
  int		freeme_det, freeme_adv, tsflag;
  char		word[PHRASELEN], *save_sent;
  IndexEntry	*ie_det, *ie_adv;
  Float		starttod, stoptod;
  freeme_det = freeme_adv = 0;
  /* ^ce matin-là */
  if (!StringGetWord_LeNonwhite(word, sent, PHRASELEN, &sent, NULL)) {
    goto failure;
  }
  /* ce ^matin-là */
  if (!(ie_det = LexEntryFindPhrase(ht, word, 2, 0, 0, &freeme_det))) {
    goto failure;
  }
  if (IEConceptIs(ie_det, N("demonstrative-determiner"))) {
    if (!StringGetWord_LeNonwhite(word, sent, PHRASELEN, &sent, NULL)) {
      goto failure;
    }
    /* ce matin-^là */
    StringElimChar(word, '.');
    if (TA_ValueRangeName1(word, N("part-of-the-day"), ht, &starttod,
                           &stoptod)) {
      if (DC(dc).lang == F_FRENCH) {
        save_sent = sent;
        if (!StringGetWord_LeNonwhite(word, sent, PHRASELEN, &sent, NULL)) {
          goto failure;
        }
        /* ce matin-là ^ */
        if ((ie_adv = LexEntryFindPhrase(ht, word, 2, 0, 0, &freeme_adv)) &&
            IEConceptIs(ie_adv, N("adverb-there"))) {
          tsflag = TSFLAG_STORY_TIME;
        } else {
          sent = save_sent;
          tsflag = TSFLAG_NOW;
        }
      } else {
        if (IEConceptIs(ie_det, N("det-that"))) {
          tsflag = TSFLAG_STORY_TIME;
        } else {
          tsflag = TSFLAG_NOW;
        }
      }
      goto success;
    }
  }
  goto failure;

success:
  TsRangeSetTodRange(starttod, stoptod, tsr);
  tsr->startts.flag |= tsflag;
  tsr->stopts.flag |= tsflag;
  *nextp = sent;
  return(1);

failure:
  if (freeme_det) IndexEntryFree(ie_det);
  if (freeme_adv) IndexEntryFree(ie_adv);
  return(0);
}

/* todo: Handle incomplete specifications: 24 avril, mardi.
 * todo: For efficiency, eliminate reparsing and re-ie'ing?
 * todo: parse shortforms such as 19941231, 12/31/94, 31.12.94, 31-Dec-1994,
 *       31-Dec-94.
 */
Bool TA_TimeTsRange(char *sent, HashTable *ht, /* RESULTS */ TsRange *tsr,
                    char **nextp)
{
  int			freeme;
  char			word[PHRASELEN];
  IndexEntry	*ie;
  Ts			ts, ts1, ts2;
  if (TA_Time_Date(sent, ht, &ts, &sent)) goto point;
  /* REDUNDANT: The below is duplicated by TA_Time_Combinations. */
  if (!StringGetWord(word, sent, PHRASELEN, &sent)) return(0);
  if (!(ie = LexEntryFindPhrase(ht, word, 2, 0, 0, &freeme))) return(0);
  if (IEConceptIs(ie, N("prep-temporal-position-month-year")) ||
      IEConceptIs(ie, N("prep-temporal-position-day"))) {
    if (freeme) IndexEntryFree(ie);
    if (TA_Time_Date(sent, ht, &ts, &sent)) goto point;
  } else if (IEConceptIs(ie, N("prep-dur-between")) ||
             IEConceptIs(ie, N("prep-dur-start"))) {
    if (freeme) IndexEntryFree(ie);
    if (TA_Time_Date(sent, ht, &ts1, &sent)) {
      if (!StringGetWord(word, sent, PHRASELEN, &sent)) return(0);
      if (!(ie = LexEntryFindPhrase(ht, word, 2, 0, 0, &freeme))) return(0);
      if (IEConceptIs(ie, N("and")) || IEConceptIs(ie, N("prep-dur-stop"))) {
        if (freeme) IndexEntryFree(ie);
        if (TA_Time_Date(sent, ht, &ts2, &sent)) {
          if (TsGE(&ts2, &ts1)) goto range;
        }
      }
      if (freeme) IndexEntryFree(ie);
    }
  } else if (IEConceptIs(ie, N("prep-dur-start-alone"))) {
    if (freeme) IndexEntryFree(ie);
    if (TA_Time_Date(sent, ht, &ts1, &sent)) {
      TsSetNa(&ts2);
      goto range;
    }
  } else if (freeme) IndexEntryFree(ie);
  return(0);
range:
  TsRangeSetNever(tsr);
  tsr->startts = ts1;
  tsr->stopts = ts2;
  *nextp = sent;
  return(1);
point:
  TsRangeSetNever(tsr);
  tsr->startts = ts;
  tsr->stopts = ts;
  *nextp = sent;
  return(1);
}

Bool TA_TsRange(char *in, Discourse *dc, /* RESULTS */ Channel *ch,
                char **nextp)
{
  char		*orig_in;
  TsRange	tsr;
  orig_in = in;
  if (TA_TimeHHMMSSToday(in, DC(dc).ht, &tsr, &in) ||
      TA_TimeTsRange(in, DC(dc).ht, &tsr, &in) ||
      TA_RelativeDayAndPartOfTheDay(in, DC(dc).ht, dc, &tsr, &in)) {
    ChannelAddPNode(ch, PNTYPE_TSRANGE, 1.0, TsRangeCopy(&tsr), NULL, orig_in,
                    in);
    *nextp = in;
    return(1);
  }
  return(0);
}

/* End of file. */
