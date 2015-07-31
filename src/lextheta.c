/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * Verb argument structure/subcategorization frame/theta
 * role/buzzword compliant.
 */

#include "tt.h"
#include "lexentry.h"
#include "lextheta.h"
#include "repbasic.h"
#include "repdb.h"
#include "repstr.h"
#include "reptext.h"
#include "repobj.h"
#include "semgen2.h"
#include "utildbg.h"
#include "utilhtml.h"

ThetaRole *ThetaRoleCreate(int isoptional, Obj *cas, LexEntry *le, int subcat,
                           ThetaRole *rest)
{
  ThetaRole *theta_roles;
  theta_roles = CREATE(ThetaRole);
  theta_roles->isoptional = (char)isoptional;
  theta_roles->cas = cas;
  theta_roles->le = le;
  theta_roles->subcat = subcat;
  theta_roles->position = TRPOS_NA;
  theta_roles->next = rest;
  return(theta_roles);
}

/* Expletives occur both at beginning (pre) and at end (post). Actually
 * this is just to make the dictionary entries look nicer. This is still
 * not enough order information to do proper generation.
 */
int ThetaRoleGetCaseIndex(ThetaRole *theta_roles, Obj *cas,
                          /* RESULTS */ int *subcat, Bool *isoptional)
{
  int	slotnum;
  for (slotnum = 1; theta_roles; slotnum++, theta_roles = theta_roles->next) {
    if (theta_roles->cas == N("expl")) { slotnum--; }
    if (cas == theta_roles->cas) {
      if (isoptional) *isoptional = (Bool)theta_roles->isoptional;
      if (subcat) *subcat = theta_roles->subcat;
      return(slotnum);
    }
  }
  if (isoptional) *isoptional = 1;
  if (subcat) *subcat = F_NULL;
  return(-1);
}

Obj *ThetaRoleRestrictionPronoun(Obj *pred, int i)
{
  if (ISAP(N("human"), DbGetRestriction(pred, i))) {
  /* todo: Add the right pronouns for groups. */
    return(N("some-human"));
  }
  return(N("some-nonhuman"));
}

/* Returns: NULL, when <cas> not found in <obj> and <cas> is optional
 *          restriction pronoun, when <cas> not found in <obj> and <cas>
 *            is required
 *          value, when <cas> found in <obj>
 */
Obj *ThetaRoleGetCaseValue(Obj *obj, ThetaRole *theta_roles, Obj *cas,
                           /* RESULTS */ int *i_r, int *subcat_r)
{
  int	i;
  Bool	isoptional;
  i = ThetaRoleGetCaseIndex(theta_roles, cas, subcat_r, &isoptional);
  if (i_r) *i_r = i;
  if (i < 0 || ObjIsNa(I(obj, i))) {
    if (isoptional) return(NULL);
    else return(ThetaRoleRestrictionPronoun(I(obj, 0), i));
  }
  return(I(obj, i));
}

/* Get nth (0-origin) instance of cas. */
void ThetaRoleGetCase(ThetaRole *theta_roles, Obj *cas, int n,
                      /* RESULTS */ LexEntry **le, int *slotnum_r,
                      int *subcat_r, Bool *isoptional)
{
  int	slotnum, cnt;
  for (slotnum = 1, cnt = 0;
       theta_roles;
       slotnum++, theta_roles = theta_roles->next) {
    if (theta_roles->cas == N("expl")) { slotnum--; }
    if (cas == theta_roles->cas) {
      if (cnt == n) {
        *isoptional = (Bool)theta_roles->isoptional;
        *slotnum_r = slotnum;
        *subcat_r = theta_roles->subcat;
        *le = theta_roles->le;
        return;
      }
      cnt++;
    }
  }
  *isoptional = 1;
  *slotnum_r = -1;
  *subcat_r = -1;
  *le = NULL;
}

/* Return values similar to ThetaRoleGetCaseValue. */
Obj *ThetaRoleGetCaseValueIth(Obj *obj, ThetaRole *theta_roles, Obj *cas,
                              int n, /* RESULTS */ LexEntry **le,
                              int *subcat_r, Bool *isoptional, int *done)
{
  int	i;
  ThetaRoleGetCase(theta_roles, cas, n, le, &i, subcat_r, isoptional);
  if (i < 0) {
    *done = 1;
    return(NULL);
  }
  *done = 0;
  if (ObjIsNa(I(obj, i))) {
    if (isoptional) {
      return(NULL);
    } else {
      return(ThetaRoleRestrictionPronoun(I(obj, 0), i));
    }
  }
  return(I(obj, i));
}

/* Value is always last. */
int ThetaRoleGetValueIndex(ThetaRole *theta_roles)
{
  int	slotnum;
  for (slotnum = 1; theta_roles; slotnum++, theta_roles = theta_roles->next) {
    if (theta_roles->cas == N("expl")) { slotnum--; }
  }
  return(slotnum);
}

int ThetaRoleGetIOBJIndex(ThetaRole *theta_roles, LexEntry *le,
                          /* RESULTS */ Bool *isoptional)
{
  int	slotnum;
  for (slotnum = 1; theta_roles; slotnum++, theta_roles = theta_roles->next) {
    if (theta_roles->cas == N("expl")) { slotnum--; continue; }
    if (N("iobj") == theta_roles->cas && le == theta_roles->le) {
      if (isoptional) *isoptional = (Bool)theta_roles->isoptional;
      return(slotnum);
    }
  }
  if (isoptional) *isoptional = 1;
  return(-1);
}

void ThetaRoleGet(ThetaRole *theta_roles, /* RESULTS */ LexEntry **le,
                  Obj **cas, int *subcat, Bool *isoptional)
{
  *isoptional = (Bool)theta_roles->isoptional;
  *cas = theta_roles->cas;
  *subcat = theta_roles->subcat;
  *le = theta_roles->le;
}

int ThetaRoleGetAnySubcat(ThetaRole *theta_roles)
{
  for (; theta_roles; theta_roles = theta_roles->next) {
    if (theta_roles->subcat != F_NULL) return(theta_roles->subcat);
  }
  return(F_NULL);
}

ThetaRole *ThetaRoleReverseDestructive(ThetaRole *theta_roles)
{
  ThetaRole	*r, *p, *next;
  r = NULL;
  for (p = theta_roles; p; ) {
    next = p->next;
    p->next = r;
    r = p;
    p = next;
  }
  return(r);
}

#ifdef notdef
ThetaRole *ThetaRoleLast(ThetaRole *theta_roles)
{
  while (theta_roles->next) theta_roles = theta_roles->next;
  return(theta_roles);
}

ThetaRole *ThetaRoleAppendDestructive(ThetaRole *theta_roles1,
                                      ThetaRole *theta_roles2)
{
  if (theta_roles1) {
    ThetaRoleLast(theta_roles1)->next = theta_roles2;
    return(theta_roles1);
  } else return(theta_roles2);
}
#endif

ThetaRole *ThetaRoleAddExplPreWord(char *word, int pos, HashTable *ht,
                                   ThetaRole *rest, int position)
{
  char		feat[2];
  LexEntry	*le;
  ThetaRole	*theta_roles;
  feat[0] = pos;
  feat[1] = TERM;
  if (!(le = LexEntryFind(word, feat, ht))) {
    Dbg(DBGLEX, DBGBAD, "ThetaRoleAddExplPreWord <%s>.<%s>", word, feat);
    return(rest);
  }
  /* Note that this <theta_roles> order is later reversed. */
  theta_roles = ThetaRoleCreate(0, N("expl"), le, F_NULL, rest);
  theta_roles->position = position;
  return(theta_roles);
}

/* Used in printing word syntax.
 * Long form; contrast with TextPrintWordAndFeatures.
 * (Both long and short forms are used by Dict.)
 */
void ThetaRoleTextPrint1(Text *text, int html, int lang, int position,
                         Obj *cas_restrict, ThetaRole *theta_roles, Obj *con,
                         Discourse *dc)
{
  int		subcat, slotnum, dospace;
  Obj		*cas, *restrct;
  Bool		isoptional;
  char		*p, buf[PHRASELEN], *s;
  LexEntry	*le;
  for (slotnum = 1; theta_roles; theta_roles = theta_roles->next, slotnum++) {
    if (theta_roles->cas == N("expl")) { slotnum--; }
    if (position != theta_roles->position) continue;
    if (cas_restrict && cas_restrict != theta_roles->cas) continue;
    ThetaRoleGet(theta_roles, &le, &cas, &subcat, &isoptional);
    if (cas == ObjNA) {
      buf[0] = LBRACE;
      buf[1] = TERM;
      p = buf+1;
    } else if (cas == N("expl")) {
      buf[0] = TERM;
      p = buf;
    } else if (isoptional) {
      buf[0] = LPAREN;
      buf[1] = TERM;
      p = buf+1;
    } else {
      buf[0] = LBRACKET;
      buf[1] = TERM;
      p = buf+1;
    }
    if (html) {
      TextPuts(buf, text);
    }
    if (cas != ObjNA && cas != N("expl")) {
      if (html) {
        HTML_TextPrint(text, GenConceptAbbrevString(cas, 1, dc));
      } else {
        StringCpy(p, GenConceptAbbrevString(cas, 1, dc), PHRASELEN-1);
      }
      dospace = 1;
    } else {
      dospace = 0;
    }
    if (con && (cas != N("expl"))) restrct = DbGetRestriction(con, slotnum);
    else restrct = NULL;
    if (restrct == N("concept") || restrct == N("object")) restrct = NULL;
    if (le) {
      if (dospace) {
        if (html) {
          TextPutc(SPACE, text);
        } else {
          StringAppendChar(buf, PHRASELEN, SPACE);
        }
        dospace = 0;
      }
      if (html) {
        LexEntryAddSep(le, le->srcphrase, PHRASELEN, buf);
        HTML_TextPrintBoldBeginIfEnglish(text, lang);
        HTML_TextPrint(text, buf);
        TextPutc(SPACE, text);
        HTML_TextPrintBoldEndIfEnglish(text, lang);
      } else {
        LexEntryAddSep(le, le->srcphrase, PHRASELEN, StringEndOf(buf));
      }
      if (cas != N("expl")) {
        if (html) {
          TextPutc('+', text);
        } else {
          StringAppendChar(buf, PHRASELEN, '+');
        }
      }
    }
    if (restrct && !ObjIsList(restrct)) {
      if (dospace) {
        if (html) {
          TextPutc(SPACE, text);
        } else {
          StringAppendChar(buf, PHRASELEN, SPACE);
        }
        dospace = 0;
      }
      if (html) {
        HTML_TextLinkToObj(text, restrct, 1, 0, 0);
      } else {
        GenConceptString(restrct, N("empty-article"), F_NOUN, F_NULL,
                         DC(dc).lang, F_NULL, F_NULL, F_NULL, PHRASELEN,
                         1, 0, dc, p = StringEndOf(buf));
        StringToUpperDestructive(p);
      }
      dospace = 1;
    }
    if (subcat != F_NULL) {
      if (html) {
        if ((s = GenFeatAbbrevString(subcat, 1, dc))) {
          TextPuts(s, text);
        }
      } else {
        if ((s = GenFeatAbbrevString(subcat, 1, dc))) {
          StringCat(buf, s, PHRASELEN);
        }
      }
    }
    if (html) {
      if (cas == ObjNA) TextPutc(RBRACE, text);
      else if (cas == N("expl")) Nop();
      else if (isoptional) TextPutc(RPAREN, text);
      else TextPutc(RBRACKET, text);
      TextPutc(SPACE, text);
    } else {
      if (cas == ObjNA) StringAppendChar(buf, PHRASELEN, RBRACE);
      else if (cas == N("expl")) Nop();
      else if (isoptional) StringAppendChar(buf, PHRASELEN, RPAREN);
      else StringAppendChar(buf, PHRASELEN, RBRACKET);
      TextPutword(buf, TERM, text);
    }
  }
}

void ThetaRoleTextPrint(Text *text, int html, LexEntry *le,
                        ThetaRole *theta_roles, Obj *con, Discourse *dc)
{
  int	lang;
  char	buf[DWORDLEN];
  lang = FeatureGet(le->features, FT_LANG);
  ThetaRoleTextPrint1(text, html, lang, TRPOS_NA, N("subj"), theta_roles, con,
                      dc);
  ThetaRoleTextPrint1(text, html, lang, TRPOS_NA, N("aobj"), theta_roles, con,
                      dc);
  ThetaRoleTextPrint1(text, html, lang, TRPOS_PRE_VERB, N("expl"), theta_roles,
                      con, dc);
  LexEntryAddSep(le, le->srcphrase, DWORDLEN-1, buf);
  if (html) {
    HTML_TextPrintBoldBeginIfEnglish(text, lang);
    HTML_TextPrint(text, buf);
    TextPutc(SPACE, text);
    HTML_TextPrintBoldEndIfEnglish(text, lang);
  } else {
    TextPutword(buf, TERM, text);
  }
  ThetaRoleTextPrint1(text, html, lang, TRPOS_POST_VERB_PRE_OBJ, N("expl"),
                      theta_roles, con, dc);
  ThetaRoleTextPrint1(text, html, lang, TRPOS_NA, N("obj"),  theta_roles, con,
                      dc);
  ThetaRoleTextPrint1(text, html, lang, TRPOS_POST_VERB_POST_OBJ, N("expl"),
                      theta_roles, con, dc);
  ThetaRoleTextPrint1(text, html, lang, TRPOS_NA, N("iobj"), theta_roles, con,
                      dc);
}

void ThetaRoleShortPrint(ThetaRole *theta_roles, int maxlen, int position,
                         Obj *cas_restrict, /* RESULTS */ char *out)
{
  out[0] = TERM;
  for (; theta_roles; theta_roles = theta_roles->next) {
    if (position != (int)theta_roles->position) continue;
    if (cas_restrict && (cas_restrict != theta_roles->cas)) continue;
    if (theta_roles->le) {
      if (out[0] != TERM) {
        StringAppendChar(out, PHRASELEN, SPACE);
      }
      if ((Bool)theta_roles->isoptional) {
        StringAppendChar(out, PHRASELEN, LPAREN);
        StringAppendDots(out, theta_roles->le->srcphrase, PHRASELEN);
        StringAppendChar(out, PHRASELEN, RPAREN);
      } else {
        StringAppendDots(out, theta_roles->le->srcphrase, PHRASELEN);
      }
    }
  }
}

char *TRPOSToString(int position)
{
  switch (position) {
    case TRPOS_NA:
      return("");
    case TRPOS_PRE_VERB:
      return("_V");
    case TRPOS_POST_VERB_PRE_OBJ:
      return("V_O");
    case TRPOS_POST_VERB_POST_OBJ:
      return("VO_");
    default:
      return("???");
  }
}

void ThetaRolePrint1(FILE *stream, ThetaRole *tr, int slotnum, Obj *con)
{
  Obj	*restrct;
  restrct = NULL;
  if (tr->isoptional) fputc(LPAREN, stream);
  else fputc(SPACE, stream);
  if (tr->cas == N("expl")) {
    fputs("  : ", stream);
  } else {
    if (con) {
      restrct = DbGetRestriction(con, slotnum);
      if (restrct == N("concept") || restrct == N("object")) restrct = NULL;
    }
    fprintf(stream, "%2d: ", slotnum);
  }
  fprintf(stream, "%7s ", M(tr->cas));
  if (tr->le) {
    fprintf(stream, "%10s.%4s ", tr->le->srcphrase, tr->le->features);
  } else {
    fprintf(stream, "%10s %4s ", "", "");
  }
  if (tr->subcat != F_NULL) fputc(tr->subcat, stream);
  else fputc(SPACE, stream);
  fprintf(stream, "%4s ", TRPOSToString(tr->position));
  if (restrct) ObjPrint(stream, restrct);
  if (tr->isoptional) fputc(RPAREN, stream);
  else fputc(SPACE, stream);
  fputc(NEWLINE, stream);
}

void ThetaRolePrint(FILE *stream, ThetaRole *theta_roles, Obj *con)
{
  int	slotnum;
  for (slotnum = 1; theta_roles; theta_roles = theta_roles->next, slotnum++) {
    if (theta_roles->cas == N("expl")) { slotnum--; }
    ThetaRolePrint1(stream, theta_roles, slotnum, con);
  }
}

#define MAXCPSIZE	10
  /* cf for-sale */

int ThetaRoleIOBJStart(char *features)
{
  if (StringIn(F_IOBJ3, features)) return(3);
  else if (StringIn(F_IOBJ4, features)) return(4);
  else return(1);
}

Obj *ThetaRoleSubjCase(int pos)
{
  if (pos == F_ADJECTIVE) return(N("aobj"));
  else return(N("subj"));
}

Bool ThetaRoleGetSubcat(int c, char *s, /* RESULTS */ int *isoptional,
                        int *subcat)
{
  *isoptional = 0;
  while (*s) {
    if (c == *((uc *)s)) {
      s++;
      if (*s == F_OPTIONAL) {
        *isoptional = 1;
        s++;
      }
      if (StringIn(*((uc *)s), FT_SUBCAT)) {
        *subcat = *((uc *)s);
        s++;
      } else {
        *subcat = F_NULL;
      }
      return(1);
    }
    s++;
  }
  return(0);
}

void ThetaRoleCheckSubcat(char *features)
{
  char *p, *save;
  for (p = features; *p; p++) {
    if (StringIn(*p, FT_SUBCAT)) {
      save = p;
      p--;
      if (*p == F_OPTIONAL) {
        p--;
      }
      if (p <= features || !StringIn(*p, FT_OBJLOC)) {
        Dbg(DBGLEX, DBGBAD,
            "<%s> FT_SUBCAT doesn't follow FT_OBJLOC [F_OPTIONAL]", features);
      }
      p = save;
    }
  }
  for (p = features; *p; p++) {
    if (*p == F_OPTIONAL) {
      if (p <= features || !StringIn(*(p-1), FT_OBJLOC)) {
        Dbg(DBGLEX, DBGBAD,
            "<%s> F_OPTIONAL doesn't follow FT_OBJLOC", features);
      }
    }
  }
}

ThetaRole *ThetaRoleBuild(char *features, LexEntry **les, int lelen,
                          int *delims, int *subcats, Obj *con,
                          ThetaRole *theta_roles_expl)
{
  int		i, j, pos, subjfound, subcat, is_optional;
  LexEntry	*le;
  ThetaRole	*trs[MAXCPSIZE], *r;
  for (j = 0; j < MAXCPSIZE; j++) {
    trs[j] = NULL;
  }
  pos = FeatureGetRequired("ThetaRoleBuild", features, FT_POS);
  subjfound = 0;
  if (pos == F_ADJECTIVE || pos == F_NOUN || pos == F_VERB) {
#ifdef maxchecking
    ThetaRoleCheckSubcat(features);
#endif
    /* subj */
    if (StringIn(F_SUBJ3, features)) {
      trs[3] = ThetaRoleCreate(0, ThetaRoleSubjCase(pos), NULL, F_NULL, NULL);
      subjfound = 1;
    }
    if (StringIn(F_SUBJ2, features)) {
      if (subjfound) {
        Dbg(DBGLEX, DBGBAD, "ThetaRoleBuild: multiple subjects");
      }
      trs[2] = ThetaRoleCreate(0, ThetaRoleSubjCase(pos), NULL, F_NULL, NULL);
      subjfound = 1;
    }
    if (pos == F_NOUN) {
      if (subjfound) {
        Dbg(DBGLEX, DBGBAD, "ThetaRoleBuild: subj defined for noun");
      }
      trs[1] = ThetaRoleCreate(0, ThetaRoleSubjCase(pos), NULL, F_NULL, NULL);
    } else {
      if (!subjfound) {
        trs[1] = ThetaRoleCreate(0, ThetaRoleSubjCase(pos), NULL, F_NULL, NULL);
        subjfound = 1;
      }
    }
    /* obj */
    if (ThetaRoleGetSubcat(F_OBJ3, features, &is_optional, &subcat)) {
      if (trs[3]) {
        Dbg(DBGLEX, DBGBAD, "ThetaRoleBuild: object index same as subject");
      }
      trs[3] = ThetaRoleCreate(is_optional, N("obj"), NULL, subcat, NULL);
    }
    if (ThetaRoleGetSubcat(F_OBJ2, features, &is_optional, &subcat)) {
      if (trs[2]) {
        Dbg(DBGLEX, DBGBAD, "ThetaRoleBuild: object index same as subject");
      }
      trs[2] = ThetaRoleCreate(is_optional, N("obj"), NULL, subcat, NULL);
    }
    if (ThetaRoleGetSubcat(F_OBJ1, features, &is_optional, &subcat)) {
      if (trs[1]) {
        Dbg(DBGLEX, DBGBAD, "ThetaRoleBuild: object index same as subject");
      }
      trs[1] = ThetaRoleCreate(is_optional, N("obj"), NULL, subcat, NULL);
    }
  } else {
    subcat = FeatureGet(features, FT_SUBCAT);
    if (pos == F_CONJUNCTION || pos == F_PREPOSITION ||
        pos == F_PRONOUN) {
      if (subcat != F_NULL) {
      /* todo: Inelegant. In order to hold the subcategorization restriction
       * for later use by ObjToLexEntryGet2 in generating conjunctions.
       */
        trs[1] = ThetaRoleCreate(0, N("kobj1"), NULL, subcat, NULL);
      }
    } else {
#ifdef maxchecking
      if (subcat != F_NULL) {
        Dbg(DBGLEX, DBGBAD, "ThetaRoleBuild: <%s> unexpected FT_SUBCAT",
            features);
      }
#endif
    }
#ifdef maxchecking
    if (F_NULL != FeatureGet(features, FT_SUBJLOC) ||
        F_NULL != FeatureGet(features, FT_OBJLOC)) {
      Dbg(DBGLEX, DBGBAD, "inapplicable subj/obj location specs <%s>",
          features);
    }
#endif
  }
  if (les) {
    for (i = 0; i < lelen; i++) {
      if (delims[i] == LE_PHRASE_PREP || delims[i] == LE_PHRASE_OPT_PREP) {
        if (!(le = les[i])) {
          Dbg(DBGLEX, DBGBAD, "ThetaRoleBuild: empty le");
        } else {
          for (j = ThetaRoleIOBJStart(features); j < MAXCPSIZE; j++) {
            if (trs[j] == NULL) {
              trs[j] = ThetaRoleCreate(delims[i] == LE_PHRASE_OPT_PREP,
                                       N("iobj"), le, subcats[i], NULL);
              break;
            }
          }
        }
      }
    }
  }
  for (j = MAXCPSIZE-1; j >= 1; j--) {
    if (trs[j]) {
      for (j = j-1; j >= 1; j--) {
        if (trs[j] == NULL) {
          trs[j] = ThetaRoleCreate(1, ObjNA, NULL, F_NULL, NULL);
        }
      }
      r = theta_roles_expl;
      for (j = MAXCPSIZE-1; j >= 1; j--) {
        if (trs[j]) {
          trs[j]->next = r;
          r = trs[j];
        }
      }
      return(r);
    }
  }
  return(NULL);
}

/* End of file. */
