/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19940420: begun
 * 19951111: merged quick instance parsing into Token
 *
 * todo: Allow database files to be reloaded.
 */

#include "tt.h"
#include "lexentry.h"
#include "repbasic.h"
#include "repdbf.h"
#include "repdb.h"
#include "repgrid.h"
#include "repobj.h"
#include "repobjl.h"
#include "repprove.h"
#include "repstr.h"
#include "reptime.h"
#include "utildbg.h"

#define TREEMAXLEVELS	20
#define MAXTSR		5

/* todoTHREAD */
int	DbFileLastLevel, DbFileTsRanges, DbFileRetractMode;
TsRange	DbFileTsr[MAXTSR];
Obj	*DbFileAncestors[TREEMAXLEVELS];

void DbFileRead(char *filename, int dbfiletype)
{
  FILE	*stream;
  if (NULL == (stream = StreamOpenEnv(filename, "r"))) return;
  DbFileReadStream(stream, N(filename), dbfiletype);
}

void DbFileReadStream(FILE *stream, Obj *rootclass, int dbfiletype)
{
  DbFileLastLevel = -1;
  while (DbFileReadRecord(stream, rootclass, dbfiletype));
  StreamClose(stream);
  DbgLastObj = NULL;
}

void DbFileAddIsaPolity(Obj *obj, int level)
{
  Bool	france;
  france = (level > 1) && (N("country-FRA") == DbFileAncestors[1]);
  if (level-1 >= 0) {
  /* todo: But this is not true over all time. */
    DbAssert(&TsRangeAlways,
             L(N("cpart-of"), obj, DbFileAncestors[level-1], E));
  }
  switch (level) {
    case 0: ObjAddIsa(obj, N("continent")); break;
    case 1: ObjAddIsa(obj, N("country")); break;
    case 2: if (france) ObjAddIsa(obj, N("region"));
            else ObjAddIsa(obj, N("polity-state")); break;
    case 3: if (france) ObjAddIsa(obj, N("department"));
            else ObjAddIsa(obj, N("county")); break;
    case 4: ObjAddIsa(obj, N("city")); break;
    case 5: if (france) ObjAddIsa(obj, N("arrondissement"));
            else ObjAddIsa(obj, N("borough")); break;
    case 6: ObjAddIsa(obj, N("city-subsubdivision")); break;
    case 7: ObjAddIsa(obj, N("city-subsubsubdivision")); break;
    case 8: ObjAddIsa(obj, N("city-subsubsubsubdivision")); break;
    case 9: ObjAddIsa(obj, N("city-subsubsubsubsubdivision")); break;
    default: {
      Dbg(DBGTREE, DBGBAD, "DbFileAddIsaPolity %d", level);
    }
  }
}

Obj *DbFileReadIsaHeader(FILE *stream, Obj *rootclass)
{
  int	level, flag, c;
  char	le0[PHRASELEN], name[PHRASELEN];
  Obj	*obj;

  /* Read indent levels. */
  level = 0;
  flag = OBJ_CREATE_A;
  while (((c = StreamPeekc(stream)) == TREE_LEVEL) ||
         (c == TREE_LEVEL_CONCRETE) || (c == TREE_LEVEL_CONTRAST)) {
    if (c == TREE_LEVEL_CONCRETE) flag = OBJ_CREATE_C;
    else if (c == TREE_LEVEL_CONTRAST) flag = OBJ_CREATE_AC;
    level++;
    StreamReadc(stream);
  }
  if (level-1 > DbFileLastLevel) {
    Dbg(DBGTREE, DBGBAD, "overindented tree");
    level = DbFileLastLevel+1;
  }
  if (level >= TREEMAXLEVELS) {
    Dbg(DBGTREE, DBGBAD, "increase TREEMAXLEVELS");
    level = TREEMAXLEVELS-1;
  }
  if (StreamPeekc(stream) == LE_SEP) {
    /* Empty Obj name. */
    Dbg(DBGLEX, DBGBAD, "empty Obj name");
    /* These are not good since the generated names can differ from
     * run to run.
     */
    StreamReadc(stream);
    le0[0] = TERM;
    if (level-1 >= 0) {
      StringGenSym(name, PHRASELEN, M(DbFileAncestors[level-1]), NULL);
    } else {
      StringGenSym(name, PHRASELEN, "Obj", NULL);
    }
  } else {
    StreamReadTo(stream, 1, le0, PHRASELEN, LE_SEP, TERM, TERM);
    /* Read Obj name. */
    LexEntryToObjName(le0, name, 1, 1);
    if (name[0] == TERM) StringGenSym(name, PHRASELEN, "Obj", NULL);
  }
  if (streq(name, "breakbreak")) {
    Dbg(DBGTREE, DBGBAD, "break point");
    Stop();
  }
  if ((DbgLastObj = obj = NameToObj(name, OBJ_NO_CREATE))) {
    if (ObjNumParents(obj) > 0) {
      Dbg(DBGTREE, DBGBAD, "name <%s> reused", M(obj));
    }
    if (flag == OBJ_CREATE_C) ObjMakeConcrete(obj);
    else ObjMakeAbstract(obj);
  } else {
    DbgLastObj = obj = NameToObj(name, flag);
  }

#ifdef notdef
  /* For debugging. */
  fprintf(stdout, "<%s>\n", M(DbgLastObj));
  fflush(stdout);
  fprintf(Log, "<%s>\n", M(DbgLastObj));
  fflush(Log);
  if (DbgLastObj == N("aasn-Roddenberry")) Stop();
#endif

  Dbg(DBGTREE, DBGHYPER, "Obj <%s>", M(obj), E);
  if (level-1 >= 0) ObjAddIsa(obj, DbFileAncestors[level-1]);
  else ObjAddIsa(obj, rootclass);
  DbFileAncestors[level] = obj;
  DbFileLastLevel = level;
  if (StringIn(LE_FEATURE_SEP, le0)) LexEntryReadString(le0, obj, 1);
  return(obj);
}

Obj *DbFileReadPolityHeader(FILE *stream, Obj *rootclass)
{
  int	level, c;
  char	le0[PHRASELEN], name[PHRASELEN];
  Obj	*obj, *cap1, *cap2, *cap3;
  int	multi1, multi2, multi3;

  cap1 = cap2 = cap3 = NULL;
  multi1 = multi2 = multi3 = -1;
  /* Read indent levels. */
  level = 0;
  while (((c = StreamPeekc(stream)) == TREE_LEVEL) ||
         (c == TREE_LEVEL_CAPITAL) || (c == TREE_LEVEL_MULTI)) {
    level++;
    if (level > DbFileLastLevel) {
      DbFileAncestors[level] = DbFileAncestors[level-1];
    }
    if (c == TREE_LEVEL_CAPITAL) {
      if (cap1 && cap2 && cap3) {
        Dbg(DBGTREE, DBGBAD, "too many capitals (*)");
      } else if (cap2) {
        cap3 = DbFileAncestors[level];
      } else if (cap1) {
        cap2 = DbFileAncestors[level];
      } else {
        cap1 = DbFileAncestors[level];
      }
    } else if (c == TREE_LEVEL_MULTI) {
      if (multi1 >= 0 && multi2 >= 0 && multi3 >= 0) {
        Dbg(DBGTREE, DBGBAD, "too many multis (-)");
      } else if (multi2 >= 0) {
        multi3 = level-1;
      } else if (multi1 >= 0) {
        multi2 = level-1;
      } else {
        multi1 = level-1;
      }
    }
    StreamReadc(stream);
  }
  if (level >= TREEMAXLEVELS) {
    Dbg(DBGTREE, DBGBAD, "increase TREEMAXLEVELS");
    level = TREEMAXLEVELS-1;
  }
  if (StreamPeekc(stream) == LE_SEP) {
    /* Empty Obj name. */
    StreamReadc(stream);
    le0[0] = TERM;
    StringGenSym(name, PHRASELEN, "polity", NULL);
  } else {
    StreamReadTo(stream, 1, le0, PHRASELEN, LE_SEP, TERM, TERM);
    /* Read Obj name. */
    LexEntryToObjName(le0, name, 1, 1);
    if (name[0] == TERM) StringGenSym(name, PHRASELEN, "Obj", NULL);
  }
  if ((DbgLastObj = obj = NameToObj(name, OBJ_NO_CREATE))) {
    if (ObjNumParents(obj) > 0) {
      Dbg(DBGTREE, DBGBAD, "name <%s> reused", M(obj));
    }
    ObjMakeConcrete(obj);
  } else {
    DbgLastObj = obj = NameToObj(name, OBJ_CREATE_C);
  }
  Dbg(DBGTREE, DBGHYPER, "Obj <%s>", M(obj), E);
  DbFileAddIsaPolity(obj, level);
  if (cap1) DbAssert(&TsRangeAlways, L(N("capital-of"), cap1, obj, E));
  if (cap2) DbAssert(&TsRangeAlways, L(N("capital-of"), cap2, obj, E));
  if (cap3) DbAssert(&TsRangeAlways, L(N("capital-of"), cap3, obj, E));
  /* todo: capital-of interacting properly with multis. */
  if (multi1 >= 0) DbFileAddIsaPolity(obj, multi1);
  if (multi2 >= 0) DbFileAddIsaPolity(obj, multi2);
  if (multi3 >= 0) DbFileAddIsaPolity(obj, multi3);
  DbFileAncestors[level] = obj;
  DbFileLastLevel = level;
  if (StringIn(LE_FEATURE_SEP, le0)) LexEntryReadString(le0, obj, 1);
  return(obj);
}

void DbFileReadIsas(FILE *stream, Obj *obj, Obj *rootclass, int dbfiletype)
{
  int	more;
  char	parent[PHRASELEN];
  Dbg(DBGTREE, DBGHYPER, "read isas", E);
  if (StreamPeekc(stream) == LE_SEP) {
    StreamReadc(stream);
  } else {
    more = 1;
    while (more) {
      more = (TREE_VALUE_SEP == StreamReadTo(stream, 1, parent, PHRASELEN,
                                             LE_SEP, TREE_VALUE_SEP, TERM));
      if (parent[0] == TERM) {
        Dbg(DBGTREE, DBGBAD, "empty parent of <%s>", M(obj));
      } else {
#ifdef maxchecking
        if (StringIn(TREE_LEVEL, parent)) {
          Dbg(DBGTREE, DBGBAD, "= present in <%s> thought to be isa", parent);
        }
        if (StringIn(TREE_SLOT_SEP, parent)) {
          Dbg(DBGTREE, DBGBAD, "| present in <%s> thought to be isa", parent);
        }
#endif
        ObjAddIsa(obj, NameToObj(parent, OBJ_CREATE_A));
      }
    }
  }
}

int DbFileReadRecord(FILE *stream, Obj *rootclass, int dbfiletype)
{
  int	c;
  Obj	*obj;

  Dbg(DBGTREE, DBGHYPER, "read next record", E);
  DbFileTsRanges = 0;
  DbFileRetractMode = 0;
  if (StreamPeekc(stream) == EOF) {
    Dbg(DBGTREE, DBGHYPER, "done reading tree records", E);
    return(0);
  }
  if ((c = StreamReadc(stream)) != TREE_LEVEL) {
    Dbg(DBGTREE, DBGBAD, "expecting TREE_LEVEL, got <%c>", c);
  }
  if (dbfiletype == DBFILETYPE_ISA) {
    obj = DbFileReadIsaHeader(stream, rootclass);
  } else if (dbfiletype == DBFILETYPE_POLITY) {
    obj = DbFileReadPolityHeader(stream, rootclass);
  } else {
    Dbg(DBGTREE, DBGBAD, "unknown dbfiletype %d", dbfiletype);
    dbfiletype = DBFILETYPE_ISA;
    obj = DbFileReadIsaHeader(stream, rootclass);
  }
  DbFileReadIsas(stream, obj, rootclass, dbfiletype);
  if (StreamPeekc(stream) == TREE_LEVEL) {
    Dbg(DBGTREE, DBGHYPER, "no lexical entries or slots", E);
    return(1);
  }
  Dbg(DBGTREE, DBGHYPER, "read lexical entries", E);
  while (LexEntryRead(stream, obj));
  if ((c = StreamPeekc(stream) == TREE_LEVEL) || c == EOF) {
    Dbg(DBGTREE, DBGHYPER, "no slots", E);
    return(1);
  }
/*
  if (obj == N("Lionel-Jospin")) Stop();
*/
  Dbg(DBGTREE, DBGHYPER, "read slots", E);
  while (DbFileReadSlot(stream, obj));
  return(1);
}

void DbFileAssert(Obj *obj, int checkifalreadytrue)
{
  int i;
  if (DbFileTsRanges <= 0) {
    if (checkifalreadytrue && ZRER(&TsRangeAlways, obj)) return;
    DbAssert(&TsRangeAlways, obj);
  } else {
    for (i = 0; i < DbFileTsRanges; i++) {
      if (checkifalreadytrue && ZRER(&DbFileTsr[i], obj)) continue;
      DbAssert(&DbFileTsr[i], obj);
    }
  }
}

Obj *DbFileReadValueString(FILE *stream)
{
  int	c;
  char	valuetext[PARAGRAPHLEN];
  /* ^"this is a test, it is."| */
  StreamReadc(stream);
  /* "^this is a test, it is."| */
  StreamReadTo(stream, 1, valuetext, PARAGRAPHLEN, DQUOTE, TERM, TERM);
  /* "this is a test, it is."^| */
  c = StreamPeekc(stream);
  /* "this is a test, it is."^| */
  if (TERM != c && TREE_VALUE_SEP != c && TREE_SLOT_SEP != c) {
    Dbg(DBGTREE, DBGBAD, "garbage after string", E);
    StreamReadc(stream);
    while ((TERM != (c = StreamReadc(stream))) &&
           TREE_VALUE_SEP != c && TREE_SLOT_SEP != c);
  }
  return(StringToObj(valuetext, NULL, 0));
}

Obj *DbFileReadValue(FILE *stream)
{
  int	c;
  char	valuetext[PHRASELEN];
  Obj   *assertion;
  if (StreamPeekc(stream) == TREE_SLOT_SEP) return(NULL);
  while ((c = StreamPeekc(stream)) == TREE_VALUE_SEP) StreamReadc(stream);
  if (StreamPeekc(stream) == LBRACKET) {
    if (!(assertion = ObjRead(stream))) {
      Dbg(DBGTREE, DBGBAD, "trouble reading assertion");
      return(ObjNA);
    }
    return(assertion);
  }
  if (c == DQUOTE) {
    return(DbFileReadValueString(stream));
  }
  StreamReadTo(stream, 0, valuetext, PHRASELEN, TREE_VALUE_SEP, TREE_SLOT_SEP,
               TERM);
  if (valuetext[0] == TERM) {
    Dbg(DBGTREE, DBGBAD, "empty value", E);
    return(NULL);
  } else if (valuetext[0] == TREE_INSTANTIATE) {
    return(ObjCreateInstance(N(valuetext+1), NULL));
  } else {
    return(TokenToObj(valuetext));
  }
}

int DbFileReadSlot(FILE *stream, Obj *obj)
{
  char	tsrtext[DWORDLEN], slotname[DWORDLEN], slotname1[DWORDLEN];
  int   c, i, len, more;
  Obj	*value, *assertion;
  Grid	*gr;
  if ((c = StreamPeekc(stream)) == TREE_LEVEL) {
    Dbg(DBGTREE, DBGBAD, "trailing | assumed for <%s>", M(obj), E);
    return(0);
  }
  if (c == EOF) {
    Dbg(DBGTREE, DBGHYPER, "trailing | assumed, end of file 1", E);
    return(0);
  }
  if ((c = StreamReadc(stream)) != TREE_SLOT_SEP) {
    Dbg(DBGTREE, DBGBAD, "expecting TREE_SLOT_SEP, got <%c>", c);
  }
  if ((c = StreamPeekc(stream)) == TREE_LEVEL) {
    Dbg(DBGTREE, DBGHYPER, "hit last slot of <%s>", M(obj), E);
    return(0);
  }
  if (c == EOF) {
    Dbg(DBGTREE, DBGHYPER, "end of file 2", E);
    return(0);
  }
  Dbg(DBGTREE, DBGHYPER, "reading slot", E);
  c = StreamPeekc(stream);
  if (c == LBRACKET) {
    if (!(assertion = ObjRead(stream))) {
      Dbg(DBGTREE, DBGBAD, "trouble reading assertion");
      return(1);
    }
    if (!InferenceAdd(assertion)) {
/* Was TE(&DbFileTsr[i].startts, assertion) which doesn't make any sense. */
      DbFileAssert(assertion, 0);
    }
    return(1);
  }
  if (c == TREE_TS_SLOT) {
    StreamReadc(stream);
    DbFileTsRanges = 0;
    more = 1;
    while (more) {
      if (DbFileTsRanges >= MAXTSR) {
        Dbg(DBGTREE, DBGBAD, "increase MAXTSR");
        DbFileTsRanges = MAXTSR-1;
      }
      more = (TREE_VALUE_SEP == StreamReadTo(stream, 0, tsrtext, DWORDLEN,
                                             TREE_VALUE_SEP, TREE_SLOT_SEP,
                                             TERM));
      if (tsrtext[strlen(tsrtext)-1] == TREE_RETRACT) {
        tsrtext[strlen(tsrtext)-1] = TERM;
        DbFileRetractMode = 1;
      } else {
        DbFileRetractMode = 0;
      }
      if (TsRangeParse(&DbFileTsr[DbFileTsRanges], tsrtext)) DbFileTsRanges++;
      if (more) StreamReadc(stream);
    }
    return(1);
  }
  c = StreamReadTo(stream, 0, slotname, DWORDLEN, TREE_SLOTVALUE_SEP1,
                   TREE_SLOTVALUE_SEP2, TREE_SLOT_SEP);
  if (slotname[0] == TERM) {
    Dbg(DBGTREE, DBGBAD, "empty slotname in <%s>", M(obj));
  }
  if (DbFileRetractMode) {
    if (DbFileTsRanges <= 0) {
      Dbg(DBGTREE, DBGBAD,
          "DbFileReadSlot: DbFileRetractMode and no DbFileTsRanges");
    } else {
      for (i = 0; i < DbFileTsRanges; i++) {
        if (c == TREE_SLOT_SEP) {
          TE(&DbFileTsr[i].startts, L(N(slotname), obj, E));
          /* todo: need to go up a few levels from slotname */
        } else if (c == TREE_SLOTVALUE_SEP1) {
          TE(&DbFileTsr[i].startts, L(N(slotname), obj, ObjWild, E));
        } else if (c == TREE_SLOTVALUE_SEP2) {
          TE(&DbFileTsr[i].startts, L(N(slotname), ObjWild, obj, E));
        } else {
          Dbg(DBGTREE, DBGBAD, "DbFileReadSlot 1");
        }
      }
    }
  }
  if (c == TREE_SLOT_SEP) {
    DbFileAssert(L(N(slotname), obj, E), 0);
    return(1);
  }
  StreamReadc(stream);
  if (streq(slotname, "GS")) {
    while ((EOF != (c = getc(stream))) && c != NEWLINE);
    if (c == EOF) return(1);
    if ((gr = GridRead(stream, obj))) {
      obj->u2.grid = gr;
      obj->type = OBJTYPEGRID;
    }
    return(1);
  }
  while ((value = DbFileReadValue(stream))) {
    if (c == TREE_SLOTVALUE_SEP1) {
      DbFileAssert(L(N(slotname), obj, value, E), 0);
      if (ObjIsList(value)) {
      /* Index predicate and arguments of asserted values. */
        for (i = 0, len = ObjLen(value); i < len; i++) {
          if (ObjIsSymbol(I(value, i))) {
            sprintf(slotname1, "%s-%d", slotname, i);
            if (!ISA(N(slotname), N(slotname1))) {
              ObjAddIsa(N(slotname1), N(slotname));
              ObjAddIsa(N(slotname1), N("relation-on-the-fly"));
            }
            DbFileAssert(L(N(slotname1), obj, I(value, i), E), 1);
          }
        }
      }
    } else if (c == TREE_SLOTVALUE_SEP2) {
      DbFileAssert(L(N(slotname), value, obj, E), 0);
    } else {
      Dbg(DBGTREE, DBGBAD, "DbFileReadSlot 2");
    }
  }
  return(1);
}

/* End of file. */
