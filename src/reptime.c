/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19981108T125318: converted to:
 *                  YYYYMMDD"T"HHMMSS"Z" (in GMT) or
 *                  YYYYMMDD"T"HHMMSS (in some unspecified local time) or
 *                  YYYYMMDD"T"HHMMSS"-"HHMM or
 *                  YYYYMMDD"T"HHMMSS"+"HHMM
 *                  YYYYMMDD or
 *                  YYYYMM or
 *                  YYYY or
 *                  na or
 *                  -Inf or
 *                  +Inf or
 *                  Inf
 * 19981123T084304: fixed TsRangeParse bug
 *
 * todo: Implement underspecified timestamps properly. Currently, December 1
 * can be misinterpreted as December with the day unknown.
 */

#include "tt.h"
#include "repbasic.h"
#include "repcxt.h"
#include "repdb.h"
#include "repmisc.h"
#include "repobj.h"
#include "repobjl.h"
#include "repspace.h"
#include "repstr.h"
#include "reptext.h"
#include "reptime.h"
#include "semgen1.h"
#include "semgen2.h"
#include "toolsvr.h"
#include "uaquest.h"
#include "utildbg.h"

/******************************************************************************
 * Day
 ******************************************************************************/

char *DayName(Day day)
{
  switch(day) {
    case 0: return("sun");
    case 1: return("mon");
    case 2: return("tue");
    case 3: return("wed");
    case 4: return("thu");
    case 5: return("fri");
    case 6: return("sat");
    default: return("???");
  }
}

Day DayParse(char *s)
{
  if (strncmp((char *)s, "sun", 3) == 0) return(0);
  else if (strncmp((char *)s, "mon", 3) == 0) return(1);
  else if (strncmp((char *)s, "tue", 3) == 0) return(2);
  else if (strncmp((char *)s, "wed", 3) == 0) return(3);
  else if (strncmp((char *)s, "thu", 3) == 0) return(4);
  else if (strncmp((char *)s, "fri", 3) == 0) return(5);
  else if (strncmp((char *)s, "sat", 3) == 0) return(6);
  else if (strncmp((char *)s, "dim", 3) == 0) return(0);
  else if (strncmp((char *)s, "lun", 3) == 0) return(1);
  else if (strncmp((char *)s, "mar", 3) == 0) return(2);
  else if (strncmp((char *)s, "mer", 3) == 0) return(3);
  else if (strncmp((char *)s, "jeu", 3) == 0) return(4);
  else if (strncmp((char *)s, "ven", 3) == 0) return(5);
  else if (strncmp((char *)s, "sam", 3) == 0) return(6);
  else {
    fprintf(Log, "DayParse: trouble parsing <%.3s>\n", s);
    return(0);
  }
}

/******************************************************************************
 * Days
 ******************************************************************************/

Bool DaysIn(Day day, Days days)
{
  return((days == DAYSNA) || ((1 << day) & days));
}

Days DaysParse(char *s)
{
  int i, len;
  Day startday, stopday;
  Days days;
  len = strlen((char *)s);
  if (len == 3) {
    return(1 << DayParse(s));
  } else if (len == 7 && ((unsigned char)s[3]) == TREE_RANGE) {
    startday = DayParse(s);
    stopday = DayParse(s+4);
    for (days = 0, i = startday; ; i++) {
      days |= (1 << (i % 7));
      if ((i % 7) == stopday) break;
    }
    return(days);
  } else {
    days = (1 << DayParse(s));
    while (1) {
      if (s[3] != TREE_NANO_SEP) {
        if (s[3] != TERM)
          Dbg(DBGGEN, DBGBAD, "DaysParse: trouble parsing <%s>", s);
        return(days);
      }
      s += 4;
      days |= (1 << DayParse(s));
    }
  }
}

void DaysPrint(FILE *stream, Days days)
{
  int i;
  if (days == DAYSNA) {
    fputs("na", stream);
    return;
  }
  for (i = 0; i < 7; i++) {
    if (days & (1 << i)) {
      fputs(DayName(i), stream);
      fputc(TREE_NANO_SEP, stream);
    }
  }
}

/******************************************************************************
 * Tod
 ******************************************************************************/

Tod TodParse(char *s)
{
  int len;
  if (streq(s, "na")) return(TODNA);
  len = strlen(s);
  switch (len) {
    case 2: return(atoi(s)*3600L);
    case 4: return(IntParse(s, 2)*3600L + IntParse(s+2, 2)*60L);
    case 6: return(IntParse(s, 2)*3600L + IntParse(s+2, 2)*60L +
                   (long)IntParse(s+4, 2));
    default:
      Dbg(DBGGEN, DBGBAD, "TodParse: trouble parsing <%s>", s);
      return(TODERR);
  }
}

void TodPrint(FILE *stream, Tod tod)
{
  long	hour, min, sec;
  if (tod == TODNA) fputs("na", stream);
  else {
    hour = tod / 3600;
    min = (tod - 3600*hour)/60;
    sec = tod % 60;
    fprintf(stream, "%.2ld%.2ld%.2ld", hour, min, sec);
  }
}

void TodSocketPrint(Socket *skt, Tod tod)
{
  long	hour, min, sec;
  char  buf[7];
  if (tod == TODNA) SocketWrite(skt, "na");
  else {
    hour = tod / 3600;
    min = (tod - 3600*hour)/60;
    sec = tod % 60;
#ifdef _GNU_SOURCE
    snprintf(buf, 7, "%.2ld%.2ld%.2ld", hour, min, sec);
#else
    sprintf(buf, "%.2ld%.2ld%.2ld", hour, min, sec);
#endif
    SocketWrite(skt, buf);
  }
}

void TodTextPrint(Text *text, Tod tod)
{
  long	hour, min, sec;
  if (tod == TODNA) TextPrintf(text, "na");
  else {
    hour = tod / 3600;
    min = (tod - 3600*hour)/60;
    sec = tod % 60;
    TextPrintf(text, "%.2ld%.2ld%.2ld", hour, min, sec);
  }
}

void TodToString(Tod tod, int maxlen, /* RESULTS */ char *r)
{
  long	hour, min, sec;
  if (tod == TODNA) {
    StringCpy(r, "na", maxlen);
  } else {
    hour = tod / 3600;
    min = (tod - 3600*hour)/60;
    sec = tod % 60;
    sprintf(r, "%.2ld%.2ld%.2ld", hour, min, sec);
  }
}

void TodToHMS(Tod tod, /* RESULTS */ int *hour, int *min, int *sec)
{
  if (tod == TODNA) {
    *hour = INTNAF;
    *min = INTNAF;
    *sec = INTNAF;
    return;
  }
  *hour = tod / 3600;
  *min = (tod - 3600*(*hour))/60;
  *sec = tod % 60;
}

/******************************************************************************
 * Dur
 ******************************************************************************/

Bool IsDurStem(char *s)
{
  return(streq(s, "ns") ||
         streq(s, "ms") ||
         streq(s, "sec") ||
         streq(s, "min") ||
         streq(s, "hrs") ||
         streq(s, "day") ||
         streq(s, "yrs") ||
         streq(s, "hhmm") ||
         streq(s, "hhmmss"));
}

Dur DurParse(char *s, char *stem)
{
  if (streq(stem, "sec")) {
    return((Dur)FloatParse(s, -1));
  } else if (streq(stem, "min")) {
    return((Dur)60.0*FloatParse(s, -1));
  } else if (streq(stem, "hrs")) {
    return((Dur)3600.0*FloatParse(s, -1));
  } else if (streq(stem, "day")) {
    return((Dur)24.0*3600.0*FloatParse(s, -1));
  } else if (streq(stem, "yrs")) {
    return((Dur)DAYSPERYEAR*24.0*3600.0*FloatParse(s, -1));
  } else if (streq(stem, "hhmm")) {
    return(IntParse(s, 2)*3600L + IntParse(s+2, 2)*60L);
  } else if (streq(stem, "hhmmss")) {
    return(IntParse(s, 2)*3600L + IntParse(s+2, 2)*60L +
           (long)IntParse(s+4, 2));
  } else if (streq(stem, "ns")) {
    return((Dur)0);	/* todo */
  } else if (streq(stem, "ms")) {
    return((Dur)0);	/* todo */
  } else {
    Dbg(DBGGEN, DBGBAD, "DurParse: trouble parsing <%s> <%s>", s, stem);
    return(DURERR);
  }
}

Dur DurParseA(char *s)
{
  char	tok[DWORDLEN], *stem;
  if (streq(s, "na")) return(DURNA);
  if (NULL == (stem = NumberStem(s))) {
    Dbg(DBGGEN, DBGBAD, "DurParseA: trouble parsing <%s>", s);
    return(DURERR);
  }
  StringCpy(tok, s, DWORDLEN);
  tok[stem-s] = TERM;
  return(DurParse(tok, stem));
}

void DurPrint(FILE *stream, Dur dur)
{
  if (dur == DURNA) fputs("na", stream);
  else fprintf(stream, "%ldsec", dur);
}

void DurSocketPrint(Socket *skt, Dur dur)
{
  char buf[PHRASELEN];
  if (dur == DURNA) SocketWrite(skt, "na");
  else {
#ifdef _GNU_SOURCE
    snprintf(buf, PHRASELEN, "%ldsec", dur);
#else
    sprintf(buf, "%ldsec", dur);
#endif
    SocketWrite(skt, buf);
  }
}

void DurTextPrint(Text *text, Dur dur)
{
  if (dur == DURNA) TextPrintf(text, "na");
  else TextPrintf(text, "%ldsec", dur);
}

void DurToString(Dur dur, int maxlen, /* RESULTS */ char *r)
{
  if (dur == TODNA) {
    StringCpy(r, "na", maxlen);
  } else {
    sprintf(r, "%ldsec", dur);
  }
}

/******************************************************************************
 * Ts
 ******************************************************************************/

Ts TsNA;

void TsInit()
{
  TsSetNa(&TsNA);
}

/* Converters. */

Tod TsToTod(Ts *ts)
{
  struct tm *tms;
  if (NULL == (tms = localtime(&ts->unixts))) return(TODNA);
  return(tms->tm_hour*3600L + tms->tm_min*60L + (long)tms->tm_sec);
}

Day TsToDay(Ts *ts)
{
  struct tm *tms;
  if (NULL == (tms = localtime(&ts->unixts))) return(DAYNA);
  return(tms->tm_wday);
}

Float TsToFloat(Ts *ts)
{
  return((Float)ts->unixts); /* todo */
}

Ts *UnixTsToTs(time_t unixts)
{
  Ts *ts;
  ts = CREATE(Ts);
  ts->unixts = unixts;
  ts->flag = 0;
  ts->cx = ContextRoot;
  return(ts);
}

time_t YMDHMSToUnixTs(int year, int month, int day,
                      int hour, int min, int sec)
{
  struct tm tms;
  time_t    r;
#ifdef MACOS
  if (year < 1905) {
#else
  if (year < 1970) {
#endif
    goto failure;
  }
  memset(&tms, 0, (size_t)sizeof(struct tm));
  tms.tm_sec = sec;
  tms.tm_min = min;
  tms.tm_hour = hour;
  tms.tm_mday = day;
  tms.tm_mon = month-1;
  tms.tm_year = year-1900;
#ifdef SOLARIS
  tms.tm_isdst = -1;
#endif
#ifdef GCC
  tms.tm_isdst = -1;
#endif
  /* todo: deal with time zones (tms.__tm_zone, tms.__tm_gmtoff?) */
  if (((time_t)-1) == (r = mktime(&tms))) goto failure;
  return r;

failure:
  /* todo */
  Dbg(DBGGEN, DBGDETAIL, "cannot represent <%.4d%.2d%.2d%.2d%.2d%.2d>",
      year, month, day, hour, min, sec);
  return UNIXTSNEGINF;
}

void YMDHMSToTs(int year, int month, int day, int hour, int min, int sec,
                /* RESULTS */ Ts *ts)
{
  ts->unixts = YMDHMSToUnixTs(year, month, day, hour, min, sec);
  ts->flag = 0;
  ts->cx = ContextRoot;
}

void YearToTs(int year, /* RESULTS */ Ts *ts)
{
  YMDHMSToTs(year, 1, 1, 0, 0, 0, ts);
}

void YMDHMSToTsComm(int day_of_the_week, int year, int month, int day, int hour,
                    int min, int sec, /* RESULTS */ Ts *ts)
{
  int	actual_day_of_the_week;
  Obj			*day_obj, *obj;
  TsRange		tsr;
  ts->unixts = YMDHMSToUnixTs(year, month, day, hour, min, sec);
  ts->flag = 0;
  ts->cx = ContextRoot;
  if (day_of_the_week != DAYNA &&
      day_of_the_week != (actual_day_of_the_week = TsToDay(ts))) {
    if ((day_obj = GenValueObj((Float)actual_day_of_the_week,
                               N("day-of-the-week")))) {
      /* todo: To do this properly, full blown timestamp objects are needed,
       * which include a description of which components to generate.
       */
      obj = L(N("standard-copula"), day_obj, E);
      TsRangeSetNever(&tsr);
      tsr.startts = *ts;
      tsr.stopts = *ts;
      ObjSetTsRange(obj, &tsr);
      CommentaryAdd(NULL, ObjListCreate(obj, NULL),
                    N("current-date-without-day-request"));
    }
  }
}

void TsToStringDate(Ts *ts, int show_nonspecific, int maxlen,
                    /* RESULTS */ char *out)
{
  struct tm *tms;
  *out = TERM;
  if (ts->flag & TSFLAG_TOD) {
    TodToString((Tod)ts->unixts, maxlen, out);
  } else if (ts->flag & TSFLAG_DUR) {
    DurToString((Tod)ts->unixts, maxlen, out);
  } else if (ts->unixts == UNIXTSNA) {
    if (show_nonspecific) StringCpy(out, "na", maxlen);
  } else if (ts->unixts == UNIXTSNEGINF) {
    if (show_nonspecific) StringCpy(out, "-Inf", maxlen);
  } else if (ts->unixts == UNIXTSPOSINF) {
    if (show_nonspecific) StringCpy(out, "Inf", maxlen);
  } else {
    if (NULL == (tms = localtime(&ts->unixts))) {
      StringCpy(out, "error", maxlen);
    } else {
      sprintf(out, "%.4d%.2d%.2d", 1900+tms->tm_year, tms->tm_mon+1,
              tms->tm_mday);
    }
  }
  if (ts->cx != ContextRoot) {
    ContextNameToString(ts->cx, StringEndOf(out));
  }
}

void TsToYMDHMS(Ts *ts, int *year, int *month, int *day, int *hour, int *min,
                int *sec)
{
  struct tm *tms;
  if (ts->flag & TSFLAG_TOD) {
    *year = *month = *day = INTNA;
    TodToHMS((Tod)ts->unixts, hour, min, sec);
  } else if (ts->flag & TSFLAG_DUR) {
    *year = *month = *day = *hour = *min = *sec = INTNA;
  } else if (ts->unixts == UNIXTSNA) {
    *year = *month = *day = *hour = *min = *sec = INTNA;
  } else if (ts->unixts == UNIXTSNEGINF) {
    *year = *month = *day = *hour = *min = *sec = INTNA;
  } else if (ts->unixts == UNIXTSPOSINF) {
    *year = *month = *day = *hour = *min = *sec = INTNA;
  } else {
    if (NULL == (tms = localtime(&ts->unixts))) {
      *year = *month = *day = *hour = *min = *sec = INTNA;
      return;
    }
    *year = 1900+tms->tm_year;
    *month = tms->tm_mon+1,
    *day = tms->tm_mday;
    *hour = tms->tm_hour;
    *min = tms->tm_min;
    *sec = tms->tm_sec;
    /* todo: NA components of timestamps need to be represented. */
    if (*month == 1 && *day == 1) {
      *month = *day = INTNA;
    } else if (*day == 1) {
      *day = INTNA;
    }
    if (*hour == 0 && *min == 0 && *sec == 0) {
      *hour = *min = *sec = INTNA;
    }
  }
}

/* Relations. */

Bool TsGT(Ts *ts1, Ts *ts2)
{
  return(ts1->unixts > ts2->unixts);
}

Bool TsGT1(Ts *ts1, Ts *ts2)
{
  if ((!TsIsSpecific(ts1)) || (!TsIsSpecific(ts2))) return(0);
  return(ts1->unixts > ts2->unixts);
}

Bool TsGT2(Ts *ts1, Ts *ts2)
{
  if (TsIsNa(ts1) || TsIsNa(ts2)) return(0);
  return(ts1->unixts > ts2->unixts);
}

Bool TsGE(Ts *ts1, Ts *ts2)
{
  return(ts1->unixts >= ts2->unixts);
}

Bool TsGE1(Ts *ts1, Ts *ts2)
{
  if ((!TsIsSpecific(ts1)) || (!TsIsSpecific(ts2))) return(0);
  return(ts1->unixts >= ts2->unixts);
}

Bool TsLE(Ts *ts1, Ts *ts2)
{
  return(ts1->unixts <= ts2->unixts);
}

Bool TsLE1(Ts *ts1, Ts *ts2)
{
  if ((!TsIsSpecific(ts1)) || (!TsIsSpecific(ts2))) return(0);
  return(ts1->unixts <= ts2->unixts);
}

Bool TsLT(Ts *ts1, Ts *ts2)
{
  return(ts1->unixts < ts2->unixts);
}

Bool TsLT1(Ts *ts1, Ts *ts2)
{
  if ((!TsIsSpecific(ts1)) || (!TsIsSpecific(ts2))) return(0);
  return(ts1->unixts < ts2->unixts);
}

Bool TsEQ(Ts *ts1, Ts *ts2)
{
  return(ts1->unixts == ts2->unixts);
}

Bool TsEQ1(Ts *ts1, Ts *ts2)
{
  if ((!TsIsSpecific(ts1)) || (!TsIsSpecific(ts2))) return(0);
  return(ts1->unixts == ts2->unixts);
}

Bool TsNE(Ts *ts1, Ts *ts2)
{
  return(ts1->unixts != ts2->unixts);
}

Bool TsNE1(Ts *ts1, Ts *ts2)
{
  if ((!TsIsSpecific(ts1)) || (!TsIsSpecific(ts2))) return(0);
  return(ts1->unixts != ts2->unixts);
}

/* Modifiers. */

void TsSetUnixTs(Ts *ts, time_t unixts)
{
  ts->unixts = unixts;
  ts->flag = 0;
  ts->cx = ContextRoot;
}

void TsSetNow(Ts *ts)
{
  ts->unixts = time(NULL);
  ts->flag = 0;
  ts->cx = ContextRoot;
}

void TsSetNa(Ts *ts)
{
  ts->unixts = UNIXTSNA;
  ts->flag = 0;
  ts->cx = ContextRoot;
}

void TsSetNaCx(/* RESULTS */ Ts *ts, /* INPUT */ Context *cx)
{
  ts->unixts = UNIXTSNA;
  ts->flag = 0;
  ts->cx = cx;
}

void TsSetNegInf(Ts *ts)
{
  ts->unixts = UNIXTSNEGINF;
  ts->flag = 0;
  ts->cx = ContextRoot;
}

void TsSetPosInf(Ts *ts)
{
  ts->unixts = UNIXTSPOSINF;
  ts->flag = 0;
  ts->cx = ContextRoot;
}

void TsSetTod(Tod tod, /* RESULTS */ Ts *ts)
{
  ts->flag |= TSFLAG_TOD;
  ts->unixts = (time_t)tod;
  ts->cx = ContextRoot;
}

void TsSetDur(Dur dur, /* RESULTS */ Ts *ts)
{
  ts->flag |= TSFLAG_DUR;
  ts->unixts = (time_t)dur;
  ts->cx = ContextRoot;
}

void TsSetApprox(/* RESULT */ Ts *ts)
{
  ts->flag |= TSFLAG_APPROX;
}

void TsIncrement(Ts *ts, Dur dur)
{
  ts->unixts += dur;
}

void TsDecrement(Ts *ts, Dur dur)
{
  ts->unixts -= dur;
}

void TsPlus(Ts *ts_in, Dur dur, /* RESULTS */ Ts *ts_out)
{
  *ts_out = *ts_in;
  if (!TsIsSpecific(ts_in)) return;
  ts_out->unixts += dur;
}

void TsTodayTime(Tod tod, /* RESULT */ Ts *outts)
{
  Ts  now;
  TsSetNow(&now);
  TsMidnight(&now, outts);
  outts->unixts += tod;
}

void TsMidnight(Ts *ints, /* RESULT */ Ts *outts)
{
  struct tm *tms;
  outts->flag = 0;
  outts->cx = ints->cx;
  if (!TsIsSpecific(ints)) outts->unixts = ints->unixts;
  else {
    if (NULL == (tms = localtime(&ints->unixts))) {
      outts->unixts = ints->unixts;
      return;
    }
    tms->tm_sec = 0;
    tms->tm_min = 0;
    tms->tm_hour = 0;
    outts->unixts = mktime(tms);
  }
}

/* Accessors. */

Bool TsIsNa(Ts *ts)
{
  return(ts->unixts == UNIXTSNA);
}

Bool TsIsTod(Ts *ts)
{
  return(0 != (ts->flag & TSFLAG_TOD));
}

Bool TsIsDur(Ts *ts)
{
  return(0 != (ts->flag & TSFLAG_DUR));
}

Bool TsNonNa(Ts *ts)
{
  return(ts->unixts != UNIXTSNA);
}

Bool TsIsSpecific(Ts *ts)
{
  if (ts->flag & TSFLAG_TOD) {
    return(((Tod)(ts->unixts)) != TODNA);
  } else if (ts->flag & TSFLAG_DUR) {
    return(((Dur)(ts->unixts)) != DURNA);
  } else {
    return(ts->unixts != UNIXTSNA &&
           ts->unixts != UNIXTSNEGINF &&
           ts->unixts != UNIXTSPOSINF);
  }
}

Dur TsGetDur(Ts *ts)
{
  return((Dur)ts->unixts);
}

Tod TsGetTod(Ts *ts)
{
  return((Tod)ts->unixts);
}

Dur TsMinus(Ts *ts1, Ts *ts2)
{
  if ((!TsIsSpecific(ts1)) || (!TsIsSpecific(ts2))) return(0L);
  return(ts1->unixts - ts2->unixts);
}

Dur TsSpread(Ts *ts1, Ts *ts2, Ts *ts3, Ts *ts4, Ts *ts5)
{
  time_t	tmin, tmax;
  tmin = UNIXTSPOSINF;
  tmax = UNIXTSNEGINF;
  if (ts1 && TsIsSpecific(ts1)) {
    if (ts1->unixts < tmin) tmin = ts1->unixts;
    else if (ts1->unixts > tmax) tmax = ts1->unixts;
  }
  if (ts2 && TsIsSpecific(ts2)) {
    if (ts2->unixts < tmin) tmin = ts2->unixts;
    else if (ts2->unixts > tmax) tmax = ts2->unixts;
  }
  if (ts3 && TsIsSpecific(ts3)) {
    if (ts3->unixts < tmin) tmin = ts3->unixts;
    else if (ts3->unixts > tmax) tmax = ts3->unixts;
  }
  if (ts4 && TsIsSpecific(ts4)) {
    if (ts4->unixts < tmin) tmin = ts4->unixts;
    else if (ts4->unixts > tmax) tmax = ts4->unixts;
  }
  if (ts5 && TsIsSpecific(ts5)) {
    if (ts5->unixts < tmin) tmin = ts5->unixts;
    else if (ts5->unixts > tmax) tmax = ts5->unixts;
  }
  if (tmin != UNIXTSPOSINF && tmax != UNIXTSNEGINF)
    return(tmax - tmin);
  else return(0L);
}

/* This might fix problems with sign bit. */
Float TsFloatMinus(Ts *ts1, Ts *ts2)
{
  if ((!TsIsSpecific(ts1)) || (!TsIsSpecific(ts2))) return(0L);
  return(((Float)ts1->unixts) - ((Float)ts2->unixts));
}

/* This might fix problems with sign bit. */
Float TsFloatSpread(Ts *ts1, Ts *ts2, Ts *ts3, Ts *ts4, Ts *ts5)
{
  time_t	tmin, tmax;
  tmin = UNIXTSPOSINF;
  tmax = UNIXTSNEGINF;
  if (ts1 && TsIsSpecific(ts1)) {
    if (ts1->unixts < tmin) tmin = ts1->unixts;
    else if (ts1->unixts > tmax) tmax = ts1->unixts;
  }
  if (ts2 && TsIsSpecific(ts2)) {
    if (ts2->unixts < tmin) tmin = ts2->unixts;
    else if (ts2->unixts > tmax) tmax = ts2->unixts;
  }
  if (ts3 && TsIsSpecific(ts3)) {
    if (ts3->unixts < tmin) tmin = ts3->unixts;
    else if (ts3->unixts > tmax) tmax = ts3->unixts;
  }
  if (ts4 && TsIsSpecific(ts4)) {
    if (ts4->unixts < tmin) tmin = ts4->unixts;
    else if (ts4->unixts > tmax) tmax = ts4->unixts;
  }
  if (ts5 && TsIsSpecific(ts5)) {
    if (ts5->unixts < tmin) tmin = ts5->unixts;
    else if (ts5->unixts > tmax) tmax = ts5->unixts;
  }
  if (tmin != UNIXTSPOSINF && tmax != UNIXTSNEGINF)
    return(((Float)tmax) - ((Float)tmin));
  else return(0.0);
}

Bool TsIsSpecification(Ts *specific, Ts *general)
{
  int	syear, smonth, sday, shour, smin, ssec;
  int	gyear, gmonth, gday, ghour, gmin, gsec;
  TsToYMDHMS(specific, &syear, &smonth, &sday, &shour, &smin, &ssec);
  TsToYMDHMS(general, &gyear, &gmonth, &gday, &ghour, &gmin, &gsec);
  return(IntIsSpecification(syear, gyear) &&
         IntIsSpecification(smonth, gmonth) &&
         IntIsSpecification(sday, gday) &&
         IntIsSpecification(shour, ghour) &&
         IntIsSpecification(smin, gmin) &&
         IntIsSpecification(ssec, gsec));
}

Bool TsSameDay(Ts *ts1, Ts *ts2)
{
  Ts	tsm1, tsm2;
  TsMidnight(ts1, &tsm1);
  TsMidnight(ts2, &tsm2);
  return(TsEQ(&tsm1, &tsm2));
}

/* todo: This is buggy around leap seconds. */
Dur TsDaysBetween(Ts *ts1, Ts *ts2)
{
  Ts	tsa, tsb;
  Dur	a, b;
  TsMidnight(ts1, &tsa);
  TsMidnight(ts2, &tsb);
  a = (Dur)(tsa.unixts / ((time_t)SECONDSPERDAYL));
  b = (Dur)(tsb.unixts / ((time_t)SECONDSPERDAYL));
  return(a - b);
}

/* Parsing. */

Bool LooksLikeTs(char *s)
{
  int len;
  len = strlen(s);
  switch (len) {
    case 4: /* YYYY */
      return Char_isdigit(s[0]) && Char_isdigit(s[1]) &&
             Char_isdigit(s[2]) && Char_isdigit(s[3]);
    case 6: /* YYYYMM */
      return Char_isdigit(s[0]) && Char_isdigit(s[1]) &&
             Char_isdigit(s[2]) && Char_isdigit(s[3]) &&
             Char_isdigit(s[4]) && Char_isdigit(s[5]);
    case 8: /* YYYYMMDD */
      return Char_isdigit(s[0]) && Char_isdigit(s[1]) &&
             Char_isdigit(s[2]) && Char_isdigit(s[3]) &&
             Char_isdigit(s[4]) && Char_isdigit(s[5]) &&
             Char_isdigit(s[6]) && Char_isdigit(s[7]);
    case 15: /* YYYYMMDDTHHMMSS */
      return Char_isdigit(s[0]) && Char_isdigit(s[1]) &&
             Char_isdigit(s[2]) && Char_isdigit(s[3]) &&
             Char_isdigit(s[4]) && Char_isdigit(s[5]) &&
             Char_isdigit(s[6]) && Char_isdigit(s[7]) &&
             s[8] == 'T' &&
             Char_isdigit(s[9]) && Char_isdigit(s[10]) &&
             Char_isdigit(s[11]) && Char_isdigit(s[12]) &&
             Char_isdigit(s[13]) && Char_isdigit(s[14]);
    case 16: /* YYYYMMDDTHHMMSSZ */
      return Char_isdigit(s[0]) && Char_isdigit(s[1]) &&
             Char_isdigit(s[2]) && Char_isdigit(s[3]) &&
             Char_isdigit(s[4]) && Char_isdigit(s[5]) &&
             Char_isdigit(s[6]) && Char_isdigit(s[7]) &&
             s[8] == 'T' &&
             Char_isdigit(s[9]) && Char_isdigit(s[10]) &&
             Char_isdigit(s[11]) && Char_isdigit(s[12]) &&
             Char_isdigit(s[13]) && Char_isdigit(s[14]) &&
             s[15] == 'T';
    case 20: /* YYYYMMDDTHHMMSS-HHMM */
      return Char_isdigit(s[0]) && Char_isdigit(s[1]) &&
             Char_isdigit(s[2]) && Char_isdigit(s[3]) &&
             Char_isdigit(s[4]) && Char_isdigit(s[5]) &&
             Char_isdigit(s[6]) && Char_isdigit(s[7]) &&
             s[8] == 'T' &&
             Char_isdigit(s[9]) && Char_isdigit(s[10]) &&
             Char_isdigit(s[11]) && Char_isdigit(s[12]) &&
             Char_isdigit(s[13]) && Char_isdigit(s[14]) &&
             (s[15] == '-' || s[15] == '+') &&
             Char_isdigit(s[16]) && Char_isdigit(s[17]) &&
             Char_isdigit(s[18]) && Char_isdigit(s[19]);
    default:
             break;
             /* Do nothing. */
  }
  return 0;
}

Bool TsParse(Ts *ts, char *s)
{
  int len;
  ts->flag = 0;
  ts->cx = ContextRoot;
  ts->unixts = UNIXTSNA; /* REDUNDANT */
  if (streq(s, "na") || streq(s, "?")) {
    ts->unixts = UNIXTSNA; return(1);
  } else if (streq(s, "Inf") || streq(s, "+Inf")) {
    ts->unixts = UNIXTSPOSINF; return(1);
  } else if (streq(s, "-Inf")) {
    ts->unixts = UNIXTSNEGINF; return(1);
  } 
  len = strlen(s);
  switch (len) {
    case 4: /* YYYY */
      ts->unixts = YMDHMSToUnixTs(atoi(s), 1, 1, 0, 0, 0);
      break;
    case 6: /* YYYYMM */
      ts->unixts = YMDHMSToUnixTs(IntParse(s, 4), IntParse(s+4, 2), 1, 0, 0, 0);
      break;
    case 8: /* YYYYMMDD */
      ts->unixts = YMDHMSToUnixTs(IntParse(s, 4), IntParse(s+4, 2),
                                  IntParse(s+6, 2), 0, 0, 0);
      break;
    case 15: /* YYYYMMDDTHHMMSS */
      /* todo: time zones */
      ts->unixts = YMDHMSToUnixTs(IntParse(s, 4), IntParse(s+4, 2),
                                  IntParse(s+6, 2), IntParse(s+9, 2),
                                  IntParse(s+11, 2), IntParse(s+13, 2));
      break;
    case 16: /* YYYYMMDDTHHMMSSZ */
      /* todo: time zones */
      ts->unixts = YMDHMSToUnixTs(IntParse(s, 4), IntParse(s+4, 2),
                                  IntParse(s+6, 2), IntParse(s+9, 2),
                                  IntParse(s+11, 2), IntParse(s+13, 2));
      break;
    case 20: /* YYYYMMDDTHHMMSS-HHMM */
      /* todo: time zones */
      ts->unixts = YMDHMSToUnixTs(IntParse(s, 4), IntParse(s+4, 2),
                                  IntParse(s+6, 2), IntParse(s+9, 2),
                                  IntParse(s+11, 2), IntParse(s+13, 2));
      break;
    default:
      Dbg(DBGGEN, DBGBAD, "TsParse: trouble parsing <%s>", s);
      ts->unixts = UNIXTSNA;
      return(0);
  }
  return(1);
}

/* Printing. */

void TsPrint1(FILE *stream, Ts *ts, Context *cx)
{
  struct tm *tms;
  if (ts == NULL) {
    fputs("NULL Ts", stream);
    return;
  }
  if (ts->flag & TSFLAG_APPROX) fputc('~', stream);
  if (ts->flag & TSFLAG_TOD) {
    TodPrint(stream, (Tod)ts->unixts);
    fputs("tod", stream);
  } else if (ts->flag & TSFLAG_DUR) {
    DurPrint(stream, (Dur)ts->unixts);
  } else if (ts->unixts == UNIXTSNA) {
    fputs("na", stream);
  } else if (ts->unixts == UNIXTSNEGINF) {
    fputs("-Inf", stream);
  } else if (ts->unixts == UNIXTSPOSINF) {
    fputs("Inf", stream);
  } else {
    if (NULL == (tms = localtime(&ts->unixts))) {
      fputs("error", stream);
    } else {
/*
      if ((1900+tms->tm_year) > 2100) Stop();
 */
      fprintf(stream, "%.4d%.2d%.2dT%.2d%.2d%.2d", 1900+tms->tm_year,
              tms->tm_mon+1, tms->tm_mday, tms->tm_hour, tms->tm_min,
              tms->tm_sec);
    }
  }
  if (ts->flag & TSFLAG_STORY_TIME) {
    fputs("(story time)", stream);
  }
  if (ts->flag & TSFLAG_NOW) {
    fputs("(now)", stream);
  }
  if (ts->cx != NULL && ts->cx != ContextRoot && ts->cx != cx) {
    ContextPrintName(stream, ts->cx);
  }
}

void TsSocketPrint(Socket *skt, Ts *ts)
{
  char      buf[16];
  struct tm *tms;
  if (ts == NULL) { SocketWrite(skt, "NULL"); return; }
  if (ts->flag & TSFLAG_APPROX) SocketWrite(skt, "~");
  if (ts->flag & TSFLAG_TOD) {
    TodSocketPrint(skt, (Tod)ts->unixts);
    SocketWrite(skt, "tod");
  } else if (ts->flag & TSFLAG_DUR) {
    DurSocketPrint(skt, (Dur)ts->unixts);
  } else if (ts->unixts == UNIXTSNA) {
    SocketWrite(skt, "na");
  } else if (ts->unixts == UNIXTSNEGINF) {
    SocketWrite(skt, "-Inf");
  } else if (ts->unixts == UNIXTSPOSINF) {
    SocketWrite(skt, "Inf");
  } else {
    if (NULL == (tms = localtime(&ts->unixts))) {
      SocketWrite(skt, "ERROR");
    } else {
#ifdef _GNU_SOURCE
      snprintf(buf, 16, "%.4d%.2d%.2dT%.2d%.2d%.2d", 1900+tms->tm_year,
               tms->tm_mon+1, tms->tm_mday, tms->tm_hour, tms->tm_min,
               tms->tm_sec);
#else
      sprintf(buf, "%.4d%.2d%.2dT%.2d%.2d%.2d", 1900+tms->tm_year,
              tms->tm_mon+1, tms->tm_mday, tms->tm_hour, tms->tm_min,
              tms->tm_sec);
#endif
      SocketWrite(skt, buf);
    }
  }
}

void TsTextPrint(Text *text, Ts *ts)
{
  struct tm *tms;
  if (ts == NULL) { TextPrintf(text, "NULL"); return; }
  if (ts->flag & TSFLAG_APPROX) TextPrintf(text, "~");
  if (ts->flag & TSFLAG_TOD) {
    TodTextPrint(text, (Tod)ts->unixts);
    TextPrintf(text, "tod");
  } else if (ts->flag & TSFLAG_DUR) {
    DurTextPrint(text, (Dur)ts->unixts);
  } else if (ts->unixts == UNIXTSNA) {
    TextPrintf(text, "na");
  } else if (ts->unixts == UNIXTSNEGINF) {
    TextPrintf(text, "-Inf");
  } else if (ts->unixts == UNIXTSPOSINF) {
    TextPrintf(text, "Inf");
  } else {
    if (NULL == (tms = localtime(&ts->unixts))) {
      TextPrintf(text, "ERROR");
    } else {
      TextPrintf(text, "%.4d%.2d%.2dT%.2d%.2d%.2d", 1900+tms->tm_year,
                 tms->tm_mon+1, tms->tm_mday, tms->tm_hour, tms->tm_min,
                 tms->tm_sec);
    }
  }
}

void TsPrint(FILE *stream, Ts *ts)
{
  TsPrint1(stream, ts, NULL);
}

void TsToString(Ts *ts, int show_nonspecific, int maxlen,
                /* RESULTS */ char *out)
{
  struct tm *tms;
  *out = TERM;
  if (ts->flag & TSFLAG_TOD) {
    TodToString((Tod)ts->unixts, maxlen, out);
    StringCat(out, "tod", maxlen);
  } else if (ts->flag & TSFLAG_DUR) {
    DurToString((Tod)ts->unixts, maxlen, out);
  } else if (ts->unixts == UNIXTSNA) {
    if (show_nonspecific) StringCpy(out, "na", maxlen);
  } else if (ts->unixts == UNIXTSNEGINF) {
    if (show_nonspecific) StringCpy(out, "-Inf", maxlen);
  } else if (ts->unixts == UNIXTSPOSINF) {
    if (show_nonspecific) StringCpy(out, "Inf", maxlen);
  } else {
    if (NULL == (tms = localtime(&ts->unixts))) {
      StringCpy(out, "error", maxlen);
    } else {
      sprintf(out, "%.4d%.2d%.2dT%.2d%.2d%.2d", 1900+tms->tm_year, tms->tm_mon+1,
              tms->tm_mday, tms->tm_hour, tms->tm_min, tms->tm_sec);
    }
  }
  if (ts->cx != ContextRoot) {
    ContextNameToString(ts->cx, StringEndOf(out));
  }
}

/* Debugging. */

void pts(Ts *ts)
{
  TsPrint(stdout, ts);
  fputc(NEWLINE, stdout);
  TsPrint(Log, ts);
  fputc(NEWLINE, Log);
}

/******************************************************************************
 * TsRange
 ******************************************************************************/

TsRange TsRangeAlways;

void TsRangeInit()
{
  TsRangeSetAlways(&TsRangeAlways);
}

TsRange *TsRangeCreate(Ts *startts, Ts *stopts, Days days, Tod tod, Dur dur)
{
  TsRange *tsr;
  tsr = CREATE(TsRange);
  tsr->startts = *startts;
  tsr->stopts = *stopts;
  tsr->days = days;
  tsr->tod = tod;
  tsr->dur = dur;
  tsr->cx = startts->cx;
  return(tsr);
}

TsRange *TsRangeCopy(TsRange *tsr)
{
  TsRange	*new;
  new = CREATE(TsRange);
  *new = *tsr;
  return(new);
}

/* Accessors. */

Bool TsRangeIsAlways(TsRange *tsr)
{
  return(tsr->startts.unixts == UNIXTSNEGINF &&
         tsr->stopts.unixts == UNIXTSPOSINF &&
         tsr->days == DAYSNA &&
         tsr->tod == TODNA &&
         tsr->dur == DURNA &&
         tsr->cx == ContextRoot);
}

Bool TsRangeIsAlwaysIgnoreCx(TsRange *tsr)
{
  return(tsr->startts.unixts == UNIXTSNEGINF &&
         tsr->stopts.unixts == UNIXTSPOSINF &&
         tsr->days == DAYSNA &&
         tsr->tod == TODNA &&
         tsr->dur == DURNA);
}

Bool TsRangeIsNa(TsRange *tsr)
{
  return(tsr->startts.unixts == UNIXTSNA &&
         tsr->stopts.unixts == UNIXTSNA &&
         tsr->days == DAYSNA &&
         tsr->tod == TODNA &&
         tsr->dur == DURNA &&
         tsr->cx == ContextRoot);
}

Bool TsRangeIsNaIgnoreCx(TsRange *tsr)
{
  return(tsr->startts.unixts == UNIXTSNA &&
         tsr->stopts.unixts == UNIXTSNA &&
         tsr->days == DAYSNA &&
         tsr->tod == TODNA &&
         tsr->dur == DURNA);
}

Bool TsRangeIsOK(TsRange *tsr)
{
  return(1); /* todo */
}

Bool TsRangeIsNonNa(TsRange *tsr)
{
  return(TsNonNa(&tsr->startts) && TsNonNa(&tsr->stopts));
}

Bool TsRangeIsSpecific(TsRange *tsr)
{
  return(TsIsSpecific(&tsr->startts) && TsIsSpecific(&tsr->stopts));
}

Bool TsRangeValid(TsRange *tsr)
{
  return(!TsGT2(&tsr->startts, &tsr->stopts));
}

Dur TsRangeDur(TsRange *tsr)
{
  if (TsIsSpecific(&tsr->startts) && TsIsSpecific(&tsr->stopts)) {
    return(tsr->stopts.unixts - tsr->startts.unixts);
  } else return(tsr->dur);
}

/* Modifiers. */

void TsRangeSetAlways(TsRange *tsr)
{
  tsr->startts.unixts = UNIXTSNEGINF;
  tsr->startts.flag = 0;
  tsr->startts.cx = ContextRoot;
  tsr->stopts.unixts = UNIXTSPOSINF;
  tsr->stopts.flag = 0;
  tsr->stopts.cx = ContextRoot;
  tsr->days = DAYSNA;
  tsr->tod = TODNA;
  tsr->dur = DURNA;
  tsr->cx = ContextRoot;
}

void TsRangeSetNow(TsRange *tsr)
{
  tsr->startts.unixts = time(NULL);
  tsr->startts.flag = 0;
  tsr->startts.cx = ContextRoot;
  tsr->stopts.unixts = tsr->startts.unixts;
  tsr->stopts.flag = 0;
  tsr->stopts.cx = ContextRoot;
  tsr->days = DAYSNA;
  tsr->tod = TODNA;
  tsr->dur = DURNA;
  tsr->cx = ContextRoot;
}

void TsRangeSetContext(/* RESULTS */ TsRange *tsr, /* INPUT */ Context *cx)
{
  tsr->startts.cx = cx;
  tsr->stopts.cx = cx;
  tsr->cx = cx;
}

void TsRangeSetNever(TsRange *tsr)
{
  tsr->startts.unixts = UNIXTSPOSINF;
  tsr->startts.flag = 0;
  tsr->startts.cx = ContextRoot;
  tsr->stopts.unixts = UNIXTSNEGINF;
  tsr->stopts.flag = 0;
  tsr->stopts.cx = ContextRoot;
  tsr->days = DAYSNA;
  tsr->tod = TODNA;
  tsr->dur = DURNA;
  tsr->cx = ContextRoot;
}

void TsRangeSetNa(TsRange *tsr)
{
  tsr->startts.unixts = UNIXTSNA;
  tsr->startts.flag = 0;
  tsr->startts.cx = ContextRoot;
  tsr->stopts.unixts = UNIXTSNA;
  tsr->stopts.flag = 0;
  tsr->stopts.cx = ContextRoot;
  tsr->days = DAYSNA;
  tsr->tod = TODNA;
  tsr->dur = DURNA;
  tsr->cx = ContextRoot;
}

void TsRangeSetTs(/* RESULTS */ TsRange *tsr, /* INPUT */ Ts *ts)
{
  tsr->startts = *ts;
  tsr->stopts = *ts;
  tsr->days = DAYSNA;
  tsr->tod = TODNA;
  tsr->dur = DURNA;
  tsr->cx = ContextRoot;
}

void TsRangeSetTodRange(Tod starttod, Tod stoptod, /* RESULTS */ TsRange *tsr)
{
  TsRangeSetNa(tsr);
  TsSetTod(starttod, &tsr->startts);
  TsSetTod(stoptod, &tsr->stopts);
}

void TsRangeSetDurRange(Dur startdur, Dur stopdur, /* RESULTS */ TsRange *tsr)
{
  TsRangeSetNa(tsr);
  TsSetDur(startdur, &tsr->startts);
  TsSetDur(stopdur, &tsr->stopts);
}

void TsRangeDefault(TsRange *in, TsRange *def, /* RESULTS */ TsRange *out)
{
  if (!TsIsSpecific(&in->startts)) out->startts = def->startts;
  else out->startts = in->startts;
  if (!TsIsSpecific(&in->stopts)) out->stopts = def->stopts;
  else out->stopts = in->stopts;
}

void TsMakeRange(TsRange *r, Ts *startts, Ts *stopts)
{
  r->startts = *startts;
  r->stopts = *stopts;
  r->days = DAYSNA;
  r->tod = TODNA;
  r->dur = DURNA;
  r->cx = startts->cx;
}

void TsMakeRangeInf(TsRange *r, Ts *ts)
{
  r->startts = *ts;
  r->stopts.unixts = UNIXTSPOSINF;
  r->stopts.flag = 0;
  r->stopts.cx = ts->cx;
  r->days = DAYSNA;
  r->tod = TODNA;
  r->dur = DURNA;
  r->cx = ts->cx;
}

void TsMakeRangeAlways(/* RESULTS */ TsRange *r, /* INPUT */ Ts *ts)
{
  r->startts.unixts = UNIXTSNEGINF;
  r->stopts.flag = 0;
  r->stopts.cx = ts->cx;
  r->stopts.unixts = UNIXTSPOSINF;
  r->stopts.flag = 0;
  r->stopts.cx = ts->cx;
  r->days = DAYSNA;
  r->tod = TODNA;
  r->dur = DURNA;
  r->cx = ts->cx;
}

void TsMakeRangeAction(TsRange *r, Ts *ts, Dur dur)
{
  r->startts = *ts;
  r->stopts = *ts;
  r->stopts.unixts += dur;
  r->days = DAYSNA;
  r->tod = TODNA;
  r->dur = DURNA;
  r->cx = ts->cx;
}

void TsMakeRangeState(TsRange *r, Ts *ts, Dur dur)
{
  r->startts = *ts;
  TsIncrement(&r->startts, dur);
  r->stopts.unixts = UNIXTSPOSINF;
  r->stopts.flag = 0;
  r->stopts.cx = ts->cx;
  r->days = DAYSNA;
  r->tod = TODNA;
  r->dur = DURNA;
  r->cx = ts->cx;
}

void TsRangeIncrement(TsRange *tsr, Dur dur)
{
  tsr->startts.unixts += dur;
  tsr->stopts.unixts += dur;
}

void TsRangeQueryStartTs(TsRange *tsr, Ts *queryts, /* RESULT */ Ts *ts)
{
  if ((!TsIsSpecific(queryts)) || tsr->tod == TODNA) {
    TsSetNa(ts);
    return;
  }
  TsMidnight(queryts, ts);
  while ((queryts->unixts - ts->unixts) < tsr->tod) ts->unixts -= SECONDSPERDAY;
  ts->unixts += tsr->tod;
}

void TsRangeQueryStopTs(TsRange *tsr, Ts *queryts, /* RESULT */ Ts *ts)
{
  TsRangeQueryStartTs(tsr, queryts, ts);
  ts->unixts += tsr->dur;
}

void TsRangeLast24Hours(Ts *ts, /* RESULTS */ TsRange *tsr)
{
  TsRangeSetNa(tsr);
  tsr->startts = *ts;
  tsr->stopts = *ts;
  TsDecrement(&tsr->startts, SECONDSPERDAYL);
}

/* Relations. */

Bool TsRangeEqual(TsRange *tsr1, TsRange *tsr2)
{
  return(tsr1->startts.unixts == tsr2->startts.unixts &&
         tsr1->stopts.unixts == tsr2->stopts.unixts &&
         tsr1->days == tsr2->days &&
         tsr1->tod == tsr2->tod &&
         tsr1->dur == tsr2->dur);
}

/* Whether ts is in the interval [ts0, ts1). Frequently called.
 * Note that ts0 won't match [ts0, ts0]. Solution is that all assertions
 * should have a duration of at least a second. cf TsRangeParse.
 */
Bool TsRangeMatch(Ts *ts, TsRange *tsrange)
{
  Tod tstod;
  if (ts->unixts == UNIXTSNA) return(1);
  return((tsrange->startts.unixts == UNIXTSNA ||
          ts->unixts >= tsrange->startts.unixts) &&
         (tsrange->stopts.unixts == UNIXTSNA ||
          ts->unixts < tsrange->stopts.unixts) &&
         DaysIn((TsToDay(ts) % 7), tsrange->days) &&
         ((tsrange->tod == TODNA) ||
          (((tstod = TsToTod(ts)) >= tsrange->tod) &&
           (tstod < tsrange->tod+tsrange->dur))));
}

Bool TsRangeOverlaps(TsRange *tsr1, TsRange *tsr2)
{
  TsRange	tsra, tsrb;

  if (TsRangeEqual(tsr1, tsr2)) {
  /* The case where both are points. cf UA_Appointment revision. */
    return(1);
  }
  tsra = *tsr1;
  tsrb = *tsr2;
  if (tsra.startts.unixts == UNIXTSNA) tsra.startts.unixts = UNIXTSNEGINF;
  if (tsra.stopts.unixts == UNIXTSNA) tsra.stopts.unixts = UNIXTSPOSINF;
  if (tsrb.startts.unixts == UNIXTSNA) tsrb.startts.unixts = UNIXTSNEGINF;
  if (tsrb.stopts.unixts == UNIXTSNA) tsrb.stopts.unixts = UNIXTSPOSINF;
  return((tsra.startts.unixts >= tsrb.startts.unixts &&
          tsra.startts.unixts < tsrb.stopts.unixts) ||
         (tsra.stopts.unixts > tsrb.startts.unixts &&
          tsra.stopts.unixts <= tsrb.stopts.unixts) ||
         (tsrb.startts.unixts >= tsra.startts.unixts &&
          tsrb.startts.unixts < tsra.stopts.unixts) ||
         (tsrb.stopts.unixts > tsra.startts.unixts &&
          tsrb.stopts.unixts <= tsra.stopts.unixts));
}

/* tsr2 (improperly) contained in tsr1
 * todo: Don't compare apples (ts) and oranges (tod, dur).
 */
Bool TsRangeIsContained(TsRange *tsr1, TsRange *tsr2)
{
  if ((!TsIsSpecific(&tsr2->startts)) || (!TsIsSpecific(&tsr2->stopts))) {
    return(0);  
  }
  return(((!TsIsSpecific(&tsr1->startts)) ||
           TsGE(&tsr2->startts, &tsr1->startts)) &&
         ((!TsIsSpecific(&tsr1->stopts)) ||
          TsLE(&tsr2->stopts, &tsr1->stopts)));
}

/* Examples:
 *     specific                general               result
 *     ----------------------  --------------------  ------
 * (1) -inf:inf / na:na        @199504:199505        1
 * (2) @~073000tod:~073000tod  @060000tod:120000tod  1
 *     @19950404:19950506      @19950403:19950506    1
 * (3) @19950403:19950506      @199504:199505        1
 */
Bool TsRangeIsSpecification(TsRange *specific, TsRange *general)
{
  if (TsRangeEqual(specific, general)) return(0);

  /* Case (1) */
  if ((!TsRangeIsSpecific(general)) && TsRangeIsSpecific(specific)) return(1);

  /* Case (2) */
  if (TsRangeIsContained(general, specific)) return(1);

  /* Case (3) */
  if (TsIsSpecification(&specific->startts, &general->startts) &&
      TsIsSpecification(&specific->stopts, &general->stopts)) {
    return(1);
  }

  return(0);
}

/* <ts> contained in <tsr>. */
Bool TsRangeTsIsContained(TsRange *tsr, Ts *ts)
{
  if ((!TsIsSpecific(ts)) ||
      (!TsIsSpecific(&tsr->startts)) ||
      (!TsIsSpecific(&tsr->stopts))) {
    return(0);
  }
  return(TsGE(ts, &tsr->startts) && TsLE(ts, &tsr->stopts));
}

/* <tsr2> properly contained in <tsr1>. */
Bool TsRangeIsProperContained(TsRange *tsr1, TsRange *tsr2)
{
  if ((!TsIsSpecific(&tsr2->startts)) || (!TsIsSpecific(&tsr2->stopts))) {
    return(0);  
  }
  return(((!TsIsSpecific(&tsr1->startts)) ||
          TsGT(&tsr2->startts, &tsr1->startts)) &&
         ((!TsIsSpecific(&tsr1->stopts)) ||
          TsLT(&tsr2->stopts, &tsr1->stopts)));
}

/* <ts> properly contained in <tsr>. */
Bool TsRangeTsIsProperContained(TsRange *tsr, Ts *ts)
{
  if ((!TsIsSpecific(ts)) ||
      (!TsIsSpecific(&tsr->startts)) ||
      (!TsIsSpecific(&tsr->stopts))) {
    return(0);
  }
  return(TsGT(ts, &tsr->startts) && TsLT(ts, &tsr->stopts));
}

Bool TsRangeGT(TsRange *tsr1, TsRange *tsr2)
{
  return(TsIsSpecific(&tsr1->startts) &&
         TsIsSpecific(&tsr2->stopts) &&
         TsGT(&tsr1->startts, &tsr2->stopts));
}

Bool TsRangeLT(TsRange *tsr1, TsRange *tsr2)
{
  return(TsIsSpecific(&tsr1->stopts) &&
         TsIsSpecific(&tsr2->startts) &&
         TsLT(&tsr1->stopts, &tsr2->startts));
}

/* Parsing. */

int TsRangeParse(TsRange *tsr, char *s)
{
  char	buf[WORDLEN];

  tsr->cx = ContextRoot;
  s = StringReadTo(s, buf, WORDLEN, TREE_MICRO_SEP, TERM, TERM);
  TsParse(&tsr->startts, buf);
  if (s[0] == TERM) {
    if (TsIsSpecific(&tsr->startts)) {
      tsr->stopts.unixts = tsr->startts.unixts+1L; /* cf TsRangeMatch */
    } else {
      tsr->stopts.unixts = tsr->startts.unixts;
    }
    tsr->stopts.flag = 0;
    tsr->stopts.cx = ContextRoot;
    tsr->days = DAYSNA;
    tsr->tod = TODNA;
    tsr->dur = DURNA;
    return(1);
  }
  s = StringReadTo(s, buf, WORDLEN, TREE_MICRO_SEP, TERM, TERM);
  TsParse(&tsr->stopts, buf);
  if (s[0] == TERM) {
    tsr->days = DAYSNA;
    tsr->tod = TODNA;
    tsr->dur = DURNA;
    return(1);
  }
  s = StringReadTo(s, buf, WORDLEN, TREE_MICRO_SEP, TERM, TERM);
  tsr->days = DaysParse(buf);
  if (s[0] == TERM) {
    tsr->tod = TODNA;
    tsr->dur = DURNA;
    return(1);
  }
  s = StringReadTo(s, buf, WORDLEN, TREE_MICRO_SEP, TERM, TERM);
  tsr->tod = TodParse(buf);
  if (s[0] == TERM) {
    tsr->dur = DURNA;
    return(1);
  }
  s = StringReadTo(s, buf, WORDLEN, TREE_MICRO_SEP, TERM, TERM);
  tsr->dur = DurParseA(buf);
  if (s[0]) {
    Dbg(DBGGEN, DBGBAD, "TsParse: ignored garbage at end of <%s>", s);
  }
  return(1);
}

/* Printing. */

void TsRangePrint(FILE *stream, TsRange *tsr)
{
  if (tsr == NULL) {
    fputs("NULL TsRange", stream);
    return;
  }
  fputc(TREE_TS_SLOT, stream);
  TsPrint1(stream, &tsr->startts, tsr->cx);
  fputc(':', stream);
  TsPrint1(stream, &tsr->stopts, tsr->cx);
  if (tsr->days != DAYSNA || tsr->tod != TODNA || tsr->dur != DURNA) {
    fputc(':', stream);
    DaysPrint(stream, tsr->days);
    fputc(':', stream);
    TodPrint(stream, tsr->tod);
    fputc(':', stream);
    DurPrint(stream, tsr->dur);
  }
  if (tsr->cx != NULL && tsr->cx != ContextRoot) {
    ContextPrintName(stream, tsr->cx);
  }
}

void TsRangeSocketPrint(Socket *skt, TsRange *tsr)
{
  if (tsr == NULL) { SocketWrite(skt, "NULL"); return; }
  SocketWrite(skt, "@");
  TsSocketPrint(skt, &tsr->startts);
  SocketWrite(skt, ":");
  TsSocketPrint(skt, &tsr->stopts);
  if (tsr->days != DAYSNA || tsr->tod != TODNA || tsr->dur != DURNA) {
  /* todo */
  }
}

void TsRangeTextPrint(Text *text, TsRange *tsr)
{
  if (tsr == NULL) { TextPrintf(text, "NULL"); return; }
  TextPrintf(text, "@");
  TsTextPrint(text, &tsr->startts);
  TextPrintf(text, ":");
  TsTextPrint(text, &tsr->stopts);
  if (tsr->days != DAYSNA || tsr->tod != TODNA || tsr->dur != DURNA) {
  /* todo */
  }
}

void TsRangeToString(TsRange *tsr, int maxlen, /* RESULTS */ char *out)
{
  char	*orig_out;
  orig_out = out;
  out[0] = TREE_TS_SLOT;
  TsToString(&tsr->startts, 1, maxlen-1, out+1);
  StringAppendChar(out, maxlen, ':');
  out = StringEndOf(out);
  TsToString(&tsr->stopts, 1, maxlen-(out-orig_out), out);
  /* todo: days, tod, dur */
  if (tsr->cx != NULL && tsr->cx != ContextRoot) {
    ContextNameToString(tsr->cx, StringEndOf(out));
  }
}

/* Filling helping functions. */

void TsRangeFillSameStart(TsRange *tsr1, TsRange *tsr2)
{
  if (TsIsSpecific(&tsr1->startts) && (!TsIsSpecific(&tsr2->startts))) {
    tsr2->startts = tsr1->startts;
  } else if (TsIsSpecific(&tsr2->startts) && (!TsIsSpecific(&tsr1->startts))) {
    tsr1->startts = tsr2->startts;
  }
}

void TsRangeFillSameStop(TsRange *tsr1, TsRange *tsr2)
{
  if (TsIsSpecific(&tsr1->stopts) && (!TsIsSpecific(&tsr2->stopts))) {
    tsr2->stopts = tsr1->stopts;
  } else if (TsIsSpecific(&tsr2->stopts) && (!TsIsSpecific(&tsr1->stopts))) {
    tsr1->stopts = tsr2->stopts;
  }
}

void TsRangeFillStop1SameAsStart2(TsRange *tsr1, TsRange *tsr2)
{
  if (TsIsSpecific(&tsr2->startts) && (!TsIsSpecific(&tsr1->stopts))) {
    tsr1->stopts = tsr2->startts;
  } else if (TsIsSpecific(&tsr1->stopts) && (!TsIsSpecific(&tsr2->startts))) {
    tsr2->startts = tsr1->stopts;
  }
}

/* Temporal relation filling. */

void TsRangeFillAdjacentPosteriority(TsRange *before, TsRange *after)
{
  TsRangeFillStop1SameAsStart2(before, after);
}

void TsRangeFillEquallyLongSimultaneity(TsRange *tsr1, TsRange *tsr2)
{
  TsRangeFillSameStart(tsr1, tsr2);
  TsRangeFillSameStop(tsr1, tsr2);
}

void TsRangeFillPersistentPosteriority(TsRange *tsr1, TsRange *tsr2)
{
  TsRangeFillSameStart(tsr1, tsr2);
}

/* Debugging. */

void ptsr(TsRange *tsr)
{
  TsRangePrint(stdout, tsr);
  fputc(NEWLINE, stdout);
  TsRangePrint(Log, tsr);
  fputc(NEWLINE, Log);
}

/* End of file. */
