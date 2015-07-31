/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19950530: begun
 * 19951216: mods
 *
 * todo:
 * - Word should point to phrases it occurs in.
 * - Alphabetize hyponyms.
 * - Sometimes the files end up large because there are too many hyponyms.
 * - The words don't include EXPLs in lexical entry display though they
 *   do in the synonyms in the object display.
 */

#include "tt.h"
#include "lexentry.h"
#include "lexobjle.h"
#include "lextheta.h"
#include "repbasic.h"
#include "repdb.h"
#include "repobj.h"
#include "repobjl.h"
#include "repstr.h"
#include "reptext.h"
#include "reptime.h"
#include "semdisc.h"
#include "semgen1.h"
#include "semgen2.h"
#include "uadict.h"
#include "utilbb.h"
#include "utilhtml.h"
#include "utildbg.h"

/******************************************************************************
 * HTML formatting routines
 ******************************************************************************/

void HTML_BeginDocument(FILE *stream, char *title)
{
  fprintf(stream, "<html>\n");
  fprintf(stream, "<head>\n");
  if (title) {
    fprintf(stream, "<title>\n");
    HTML_Print(stream, title);
    fprintf(stream, "\n</title>\n");
  }
  fprintf(stream, "</head>\n");
  fprintf(stream, "<body>\n");
}

void HTML_EndDocument(FILE *stream)
{
  fprintf(stream, "</body>\n");
  fprintf(stream, "</html>\n");
}

void HTML_LineBreak(FILE *stream)
{
  fprintf(stream, "<br>\n");
}

void HTML_BlankPage(FILE *stream)
{
  fprintf(stream, ".<br>.<br>.<br>.<br>.<br>.<br>.<br>.<br>.<br>.<br>.<br>\n");
  fprintf(stream, ".<br>.<br>.<br>.<br>.<br>.<br>.<br>.<br>.<br>.<br>.<br>\n");
  fprintf(stream, ".<br>.<br>.<br>.<br>.<br>.<br>.<br>.<br>.<br>.<br>.<br>\n");
  fprintf(stream, ".<br>.<br>.<br>.<br>\n");
}

void HTML_NewParagraph(FILE *stream)
{
  fprintf(stream, "<p>\n");
}

void HTML_TextNewParagraph(Text *text)
{
  TextPuts("<p>", text);
}


void HTML_HorizontalRule(FILE *stream)
{
  fprintf(stream, "<hr>\n");
}

char *StringNative_To_HTML[] = {
  "Ó", "oe",
  "«", "&laquo;",
  "»", "&raquo;",
  ">", "&gt;",
  "&", "&amp;",
  "\"", "&quot;",
  "á", "&aacute;",
  "á", "&aacute;",
  "â", "&acirc;",
  "à", "&agrave;",
  "ã", "&atilde;",
  "ä", "&auml;",
  "ç", "&ccedil;",
  "é", "&eacute;",
  "ê", "&ecirc;",
  "è", "&egrave;",
  "ë", "&euml;",
  "í", "&iacute;",
  "î", "&icirc;",
  "ì", "&igrave;",
  "ï", "&iuml;",
  "ñ", "&ntilde;",
  "ó", "&oacute;",
  "ô", "&ocirc;",
  "ò", "&ograve;",
  "ø", "&oslash;",
  "õ", "&otilde;",
  "ö", "&ouml;",
  "ú", "&uacute;",
  "û", "&ucirc;",
  "ù", "&ugrave;",
  "ü", "&uuml;",
  "ÿ", "&yuml;",
  "À", "&Agrave;",
  "Ã", "&Atilde;",
  "Ä", "&Auml;",
  "É", "&Eacute;",
  "‹", "I",
  "Ñ", "&Ntilde;",
  "Ø", "&Oslash;",
  "Õ", "&Otilde;",
  "Ö", "&Ouml;",
  "Ü", "&Ucirc;",
  "Ü", "&Uuml;",
  NULL };

void HTML_Print(FILE *stream, char *s)
{
  int	c;
  char	**p;
  while ((c = *((uc *)s))) {
    for (p = StringNative_To_HTML; *p; p += 2) {
      if ((*((uc *)(*p))) == c) {
        fputs(*(p+1), stream);
        goto nextchar;
      }
    }
    fputc(c, stream);
nextchar:
    s++;
  }
}

void HTML_TextPrint(Text *text, char *s)
{
  int	c;
  char	**p;
  while ((c = *s)) {
    for (p = StringNative_To_HTML; *p; p += 2) {
      if ((*p)[0] == c) {
        TextPuts(*(p+1), text);
        goto nextchar;
      }
    }
    TextPutc(c, text);
nextchar:
    s++;
  }
}

void HTML_StringPrint(char *s, /* RESULTS */ char *out)
{
  int	c;
  char	**p;
  while ((c = *((uc *)s))) {
    for (p = StringNative_To_HTML; *p; p += 2) {
      if ((*((uc *)(*p))) == c) {
        strcpy(out, *(p+1));
        out = StringEndOf(out);
        goto nextchar;
      }
    }
    *out++ = c;
nextchar:
    s++;
  }
  *out = TERM;
}

void HTML_TextPrintBoldBeginIfEnglish(Text *text, int lang)
{
  if (lang == F_ENGLISH) {
    TextPuts("<b>", text);
  }
}

void HTML_TextPrintBoldEndIfEnglish(Text *text, int lang)
{
  if (lang == F_ENGLISH) {
    TextPuts("</b>", text);
  }
}

void HTML_PrintBoldBeginIfEnglish(FILE *stream, int lang)
{
  if (lang == F_ENGLISH) {
    fputs("<b>", stream);
  }
}

void HTML_PrintBoldEndIfEnglish(FILE *stream, int lang)
{
  if (lang == F_ENGLISH) {
    fputs("</b>", stream);
  }
}

void HTML_PrintBold(FILE *stream, char *s)
{
  fputs("<b>", stream);
  HTML_Print(stream, s);
  fputs("</b>", stream);
}

void HTML_TextPrintBold(Text *text, char *s)
{
  TextPuts("<b>", text);
  HTML_TextPrint(text, s);
  TextPuts("</b>", text);
}

void HTML_PrintItalic(FILE *stream, char *s)
{
  fputs("<i>", stream);
  HTML_Print(stream, s);
  fputs("</i>", stream);
}

void HTML_PrintTypewriter(FILE *stream, char *s)
{
  fputs("<tt>", stream);
  HTML_Print(stream, s);
  fputs("</tt>", stream);
}

void HTML_TextPrintTypewriter(Text *text, char *s)
{
  TextPuts("<tt>", text);
  HTML_TextPrint(text, s);
  TextPuts("</tt>", text);
}

void HTML_PrintH1(FILE *stream, char *s)
{
  fputs("<h1>", stream);
  HTML_Print(stream, s);
  fputs("</h1>", stream);
}

void HTML_PrintH2(FILE *stream, char *s)
{
  fputs("<h2>", stream);
  HTML_Print(stream, s);
  fputs("</h2>", stream);
}

void HTML_PrintH2Begin(FILE *stream)
{
  fputs("<h2>", stream);
}

void HTML_PrintH2End(FILE *stream)
{
  fputs("</h2>", stream);
}

void HTML_BeginUnnumberedList(FILE *stream)
{
  fprintf(stream, "<ul>\n");
}

void HTML_TextBeginUnnumberedList(Text *text)
{
  TextPuts("<ul>\n", text);
}

void HTML_UnnumberedListElement(FILE *stream)
{
  fprintf(stream, "<li>\n");
}

void HTML_TextUnnumberedListElement(Text *text)
{
  TextPuts("<li>\n", text);
}

void HTML_EndUnnumberedList(FILE *stream)
{
  fprintf(stream, "</ul>\n");
}

void HTML_TextEndUnnumberedList(Text *text)
{
  TextPuts("</ul>\n", text);
}

void HTML_BeginDefinitionList(FILE *stream)
{
  fprintf(stream, "<dl>\n");
}

void HTML_DefinitionListElementTitle(FILE *stream)
{
  fprintf(stream, "<dt>\n");
}

void HTML_DefinitionListElementDef(FILE *stream)
{
  fprintf(stream, "<dd>\n");
}

void HTML_EndDefinitionList(FILE *stream)
{
  fprintf(stream, "</dl>\n");
}

/* Prints to <text>: objname Englishword Frenchword. */
void HTML_TextObjName(Text *text, Obj *obj, int objname, int english,
                      int french)
{
  int	sepneeded, printed;
  char	buf[LINELEN];
  sepneeded = printed = 0;
  if (objname) {
    HTML_TextPrintTypewriter(text, ObjToName(obj));
    sepneeded = printed = 1;
  }
  if (english) {
    if (sepneeded) TextPutc(SPACE, text);
    GenConceptString(obj, N("empty-article"), F_NOUN, F_NULL, F_ENGLISH,
                     F_NULL, F_NULL, F_NULL, LINELEN, 0, 0, StdDiscourse,
                     buf);
    if (buf[0]) {
      HTML_TextPrintBold(text, buf);
      sepneeded = printed = 1;
    }
  }
  if (french) {
    if (sepneeded) TextPutc(SPACE, text);
    GenConceptString(obj, N("empty-article"), F_NOUN, F_NULL, F_FRENCH,
                     F_NULL, F_NULL, F_NULL, LINELEN, 0, 1, StdDiscourse,
                     buf);
    if (buf[0]) {
      HTML_TextPrint(text, buf);
      printed = 1;
    }
  }
  if (!printed) {
  /* Print Obj name as last resort. */
    HTML_TextPrintTypewriter(text, ObjToName(obj));
  }
}

void HTML_ObjName(FILE *stream, Obj *obj, int objname, int english, int french)
{
  Text	*text;
  text = TextCreat(StdDiscourse);
  HTML_TextObjName(text, obj, objname, english, french);
  fputs(TextString(text), stream);
  TextFree(text);
}

void HTML_StringObjName(Obj *obj, int objname, int english, int french,
                   /* RESULTS */ char *out)
{
  Text	*text;
  text = TextCreat(StdDiscourse);
  HTML_TextObjName(text, obj, objname, english, french);
  strcpy(out, TextString(text));
  TextFree(text);
}

/* A source phrase with separators added. */
void HTML_LEName(FILE *stream, LexEntry *le, char *word)
{
  char	buf[LINELEN];
  LexEntryAddSep(le, word, LINELEN, buf);
  HTML_Print(stream, buf);
}

/* Though a separate file for each Obj and LexEntry would be faster,
 * it uses up too much disk space.
 */

#define OBJHASHSIZE	2003
#define LEHASHSIZE	2003

int HTML_Hash(char *name, int hashsize)
{
  int	len;
  unsigned char	s[HASHSIG];
  memset(s, 0, 5);
  len = strlen(name);
  memcpy(s, name, (size_t)IntMin(len, 5));
  s[5] = name[len-1];
  return(((s[1] + s[5] + (s[0]+s[4])*(long)256) +
         (s[3] + s[2]*(long)256)*(long)481) % hashsize);
}

void HTML_StringObjFilename(Obj *obj, /* RESULTS */ char *out)
{
  sprintf(out, "../O/%d.html", HTML_Hash(ObjToName(obj), OBJHASHSIZE));
}

void HTML_TextObjFilename(Text *text, Obj *obj)
{
  TextPrintf(text, "../O/%d.html", HTML_Hash(ObjToName(obj), OBJHASHSIZE));
}

void HTML_LEHrefString(LexEntry *le, /* RESULTS */ char *out)
{
  char	buf[PHRASELEN];
  StringCpy(buf, le->srcphrase, PHRASELEN);
  StringMapLeWhitespaceToDash(buf);
  sprintf(out, "<a href = \"../L/%d.html#%s\">",
          HTML_Hash(le->srcphrase, LEHASHSIZE), buf);
}


void HTML_StringLinkToLE(LexEntry *le, /* RESULTS */ char *out)
{
  char	buf[LINELEN];
  HTML_LEHrefString(le, out);
  LexEntryAddSep(le, le->srcphrase, LINELEN, buf);
  HTML_StringPrint(buf, StringEndOf(out));
  strcpy(StringEndOf(out), "</a>");
}

void HTML_ObjTargetLocation(FILE *stream, Obj *obj, int objname, int english,
                            int french)
{
  fprintf(stream, "<a name = \"%s\">", ObjToName(obj));
  HTML_ObjName(stream, obj, objname, english, french);
  fputs("</a>", stream);
}

void HTML_LETargetLocation(FILE *stream, LexEntry *le)
{
  int	lang;
  char	buf[PHRASELEN];
  lang = FeatureGet(le->features, FT_LANG);
  StringCpy(buf, le->srcphrase, PHRASELEN);
  StringMapLeWhitespaceToDash(buf);
  fprintf(stream, "<a name = \"%s\">", buf);
  LexEntryAddSep(le, le->srcphrase, PHRASELEN, buf);
  HTML_PrintBoldBeginIfEnglish(stream, lang);
  HTML_Print(stream, buf);
  fputc(SPACE, stream);
  HTML_PrintBoldEndIfEnglish(stream, lang);
  fputs("</a>", stream);
}

void HTML_StringLinkToObj(Obj *obj, int objname, int english, int french,
                          /* RESULTS */ char *out)
{
  switch (obj->type) {
    case OBJTYPEASYMBOL:
    case OBJTYPEACSYMBOL:
    case OBJTYPECSYMBOL:
      strcpy(out, "<a href = \"");
      HTML_StringObjFilename(obj, StringEndOf(out));
      sprintf(StringEndOf(out), "#%s\">", ObjToName(obj));
      HTML_StringObjName(obj, objname, english, french, StringEndOf(out));
      strcpy(StringEndOf(out), "</a>");
      break;
    case OBJTYPESTRING:
      HTML_StringPrint(ObjToString(obj), out);
      break;
    case OBJTYPENUMBER:
      sprintf(out, "%g", ObjToNumber(obj));
      break;
    default:
      /* todo */
      strcpy(out, "???");
      break;
  }
}

void HTML_LinkToObj(FILE *stream, Obj *obj, int objname, int english,
                    int french)
{
  char	buf[SENTLEN];
  HTML_StringLinkToObj(obj, objname, english, french, buf);
  fputs(buf, stream);
}

void HTML_TextLinkToObj(Text *text, Obj *obj, int objname, int english,
                    int french)
{
  char	buf[LINELEN];
  HTML_StringLinkToObj(obj, objname, english, french, buf);
  TextPuts(buf, text);
}

void HTML_LinkTo(FILE *stream, char *name)
{
  fprintf(stream, "<a href = \"%s\">", name);
  HTML_Print(stream, name);
  fputs("</a>", stream);
}

/******************************************************************************
 * ThoughtTreasure database to HTML.
 ******************************************************************************/

void TT_HTML(int objname, int english, int french)
{
  int		savelang;
  GenAdvice	save_ga;

  if (!AnaMorphOn) {
    Dbg(DBGDB, DBGBAD, "Better to run this with MorphInit(1)");
  }

  savelang = DC(StdDiscourse).lang;
  save_ga = StdDiscourse->ga;

  DiscourseSetLang(StdDiscourse, F_ENGLISH);
  StdDiscourse->ga.flip_lang_ok = 0;

  TT_HTML_LE(objname, english, french);
  TT_HTML_Obj(objname, english, french);

  DiscourseSetLang(StdDiscourse, savelang);
  StdDiscourse->ga = save_ga;
}

char *TT_Obj_HTML_Dir, *TT_LexEntry_HTML_Dir, *TT_Report_Dir;

void TT_HTML_Init()
{
  TT_Obj_HTML_Dir = "../O";
  TT_LexEntry_HTML_Dir = "../L";
  TT_Report_Dir = ".";
}

/******************************************************************************
 * LE
 ******************************************************************************/

void TT_HTML_LE(int objname, int english, int french)
{
  int		hashi;
  char		fn[PHRASELEN];
  FILE		*index_stream;
  StringArray	*index_sa;

  index_sa = StringArrayCreate();

  for (hashi = 0; hashi < LEHASHSIZE; hashi++) {
    TT_HTML_LE1(hashi, index_sa, objname, english, french);
  }

  StringArrayHTMLSort(index_sa);
  sprintf(fn, "%s/index.html", TT_LexEntry_HTML_Dir);
  if (NULL == (index_stream = StreamOpen(fn, "w+"))) return;
  HTML_BeginDocument(index_stream, "ThoughtTreasure Ontology Index");
  StringArrayPrint(index_stream, index_sa, 1, 1);
  StringArrayFreeCopy(index_sa);
  HTML_EndDocument(index_stream);
  StreamClose(index_stream);
}

void TT_HTML_LE1(int hashi, StringArray *index_sa, int objname, int english,
                 int french)
{
  char			fn[PHRASELEN];
  FILE			*stream;
  LexEntry		*le;
  sprintf(fn, "%s/%d.html", TT_LexEntry_HTML_Dir, hashi);
  if (NULL == (stream = StreamOpen(fn, "w+"))) return;
  HTML_BeginDocument(stream, "ThoughtTreasure Ontology");
  HTML_BlankPage(stream);
  for (le = AllLexEntries; le; le = le->next) {
    if (HTML_Hash(le->srcphrase, LEHASHSIZE) == hashi) {
      if (le->leo) {
        /* To save space for now, only dump objects with attached concepts. */
        TT_HTML_LEDump(stream, index_sa, le, objname, english, french);
      }
    }
  }
  HTML_BlankPage(stream);
  HTML_EndDocument(stream);
  StreamClose(stream);
}

void TT_HTML_LEDump(FILE *stream, StringArray *index_sa, LexEntry *le,
                    int objname, int english, int french)
{
  char		buf[LINELEN];
  LexEntryToObj	*leo;
  Text		*text;

  HTML_HorizontalRule(stream);
  HTML_PrintH2(stream, "ThoughtTreasure lexical entry");
  HTML_LETargetLocation(stream, le);
  GenFeaturesAbbrevString(le->features, 1, 1, FS_INFL_CHECKED FS_REALLY,
                          StdDiscourse, PHRASELEN, buf);
  HTML_Print(stream, buf);

  /* todo: Ideally we should index all inflections, but this would get
   * too big.
   */
  HTML_StringLinkToLE(le, buf);
  strcpy(StringEndOf(buf), "<br>");
  StringArrayAddCopy(index_sa, buf, 0);

  if (le->leo) {
    HTML_BeginUnnumberedList(stream);
    for (leo = le->leo; leo; leo = leo->next) {
      HTML_UnnumberedListElement(stream);
      HTML_LinkToObj(stream, leo->obj, objname, english, french);
      if (leo->theta_roles) {
        fputs(" ", stream);
        text = TextCreat(StdDiscourse);
        ThetaRoleTextPrint(text, 1, le, leo->theta_roles, leo->obj,
                           StdDiscourse);
        fputs(TextString(text), stream);
        TextFree(text);
      }
    }
    HTML_EndUnnumberedList(stream);
  }
  if (StringIn(F_INFL_CHECKED, le->features)) {
    TT_HTML_LEDumpInfl(stream, le);
  }
}

void TT_HTML_LEDumpInfl(FILE *stream, LexEntry *le)
{
  char	buf[PHRASELEN];
  Word	*infl;
  HTML_BeginDefinitionList(stream);
  for (infl = le->infl; infl; infl = infl->next) {
    HTML_DefinitionListElementTitle(stream);
    GenFeaturesAbbrevString(infl->features, 1, 1, FT_POS FT_LANG
                            FS_INFL_CHECKED FS_REALLY, StdDiscourse,
                            PHRASELEN, buf);
    HTML_Print(stream, buf);
    HTML_DefinitionListElementDef(stream);
    /* todo: English inflections in bold. */
    HTML_Print(stream, infl->word);
  }
  HTML_EndDefinitionList(stream);
}

/******************************************************************************
 * Obj
 ******************************************************************************/

void TT_HTML_Obj(int objname, int english, int french)
{
  int		hashi;

  for (hashi = 0; hashi < OBJHASHSIZE; hashi++) {
    TT_HTML_Obj1(hashi, objname, english, french);
  }
}

void TT_HTML_Obj1(int hashi, int objname, int english,
                  int french)
{
  char	fn[PHRASELEN];
  int	cnt;
  FILE	*stream;
  Obj	*obj;
  sprintf(fn, "%s/%d.html", TT_Obj_HTML_Dir, hashi);
  if (NULL == (stream = StreamOpen(fn, "w+"))) return;
  HTML_BeginDocument(stream, "ThoughtTreasure Ontology");
  HTML_BlankPage(stream);
  cnt = 0;
  for (obj = Objs; obj; obj = obj->next) {
    if (ObjIsSymbol(obj) && HTML_Hash(ObjToName(obj), OBJHASHSIZE) == hashi) {
      cnt++;
      TT_HTML_ObjDump(stream, obj, objname, english, french);
    }
  }
  Dbg(DBGDB, DBGBAD, "%d objects", cnt);
  HTML_BlankPage(stream);
  HTML_EndDocument(stream);
  StreamClose(stream);
}

void TT_HTML_ObjAssertions(FILE *stream, Obj *obj, int i, int objname,
                           int english, int french, /* RESULTS */ int *first)
{
  ObjList	*assertions, *p;
  assertions = RE(&TsNA, (i == 1) ? L(ObjWild, obj, ObjWild, E) :
                                    L(ObjWild, ObjWild, obj, E));
  for (p = assertions; p; p = p->next) {
    if (N("part-of") == I(p->obj, 0)) continue;
    if (N("href") == I(p->obj, 0)) continue;
    if (N("comment") == I(p->obj, 0)) continue;
    if (*first) {
      HTML_NewParagraph(stream);
      HTML_Print(stream, "Assertions:");
      HTML_BeginUnnumberedList(stream);
      *first = 0;
    }
#ifdef notdef
    /* This can't generate list values, as in selectional restrictions,
     * nor NUMBER units, etc.
     */
    if ((i == 1 || i == 2) && ObjLen(p->obj) == 3) {
      HTML_UnnumberedListElement(stream);
      HTML_LinkToObj(stream, I(p->obj, 0), objname, english, french);
      if (i == 1) {
        fputs(" = ", stream);
      } else if (i == 2) {
        fputs(" == ", stream);
      }
      HTML_LinkToObj(stream, I(p->obj, FLIP12(i)), objname, english, french);
    } else {
#endif
      HTML_UnnumberedListElement(stream);
      ObjPrettyPrint1(stream, p->obj, NULL, 0, 1, 1, 0, 0, 1);
#ifdef notdef
    }
#endif
  }
  ObjListFree(assertions);
}

void TT_HTML_ObjRelationValues(FILE *stream, char *title, Obj *prop, Obj *obj,
                               int i, int objname, int english, int french)
{
  int		first;
  ObjList	*assertions, *p;
  assertions = RE(&TsNA, (i == 1) ? L(prop, obj, ObjWild, E) :
                  L(prop, ObjWild, obj, E));
  first = 1;
  for (p = assertions; p; p = p->next) {
    if (first) {
      HTML_NewParagraph(stream);
      HTML_Print(stream, title);
      HTML_BeginUnnumberedList(stream);
      first = 0;
    }
    HTML_UnnumberedListElement(stream);
    HTML_LinkToObj(stream, I(p->obj, FLIP12(i)), objname, english, french);
  }
  if (first == 0) HTML_EndUnnumberedList(stream);
  ObjListFree(assertions);
}

void TT_HTML_ObjDump(FILE *stream, Obj *obj, int objname, int english,
                     int french)
{
  int		i, first;
  Text		*text;
  ObjList	*comments, *hrefs, *p;
  ABrainTask	*abt;

  HTML_HorizontalRule(stream);
  HTML_PrintH2(stream, "ThoughtTreasure object");
  HTML_ObjTargetLocation(stream, obj, 1, 0, 0);

  /* Lexical items. */
  text = TextCreat(StdDiscourse);
  abt = BBrainBegin(N("answer"), 10L, INTNA);
  DictRelatedWords(1, text, obj, F_NULL, NULL, abt, StdDiscourse);
  BBrainEnd(abt);
  if (TextLen(text) > 0L) {
    fputs(TextString(text), stream);
    HTML_EndUnnumberedList(stream);
  }
  TextFree(text);

  /* Parents. */
  if (obj->u1.nlst.numparents > 0) {
    HTML_NewParagraph(stream);
    HTML_Print(stream, "Parents:");
    HTML_BeginUnnumberedList(stream);
    for (i = 0; i < obj->u1.nlst.numparents; i++) {
      if (ObjToName(obj->u1.nlst.parents[i])[0] == '.') continue;
      HTML_UnnumberedListElement(stream);
      HTML_LinkToObj(stream, obj->u1.nlst.parents[i], objname, english, french);
    }
    HTML_EndUnnumberedList(stream);
  }

  /* Children. */
  if (obj->u1.nlst.numchildren > 0) {
    HTML_NewParagraph(stream);
    HTML_Print(stream, "Children:");
    HTML_BeginUnnumberedList(stream);
    for (i = 0; i < obj->u1.nlst.numchildren; i++) {
      HTML_UnnumberedListElement(stream);
      HTML_LinkToObj(stream, obj->u1.nlst.children[i], objname, english,
                     french);
    }
    HTML_EndUnnumberedList(stream);
  }

  TT_HTML_ObjRelationValues(stream, "Part of:", N("part-of"), obj, 1,
                            objname, english, french);

  TT_HTML_ObjRelationValues(stream, "Parts:", N("part-of"), obj, 2,
                            objname, english, french);

  /* Textual comments. */
  comments = RE(&TsNA, L(N("comment"), obj, ObjWild, E));
  first = 1;
  for (p = comments; p; p = p->next) {
    if (ObjIsString(I(p->obj, 2))) {
      if (first) {
        HTML_NewParagraph(stream);
        first = 0;
      }
      HTML_Print(stream, ObjToString(I(p->obj, 2)));
    }
  }
  ObjListFree(comments);

  /* Hypertext references. */
  hrefs = RE(&TsNA, L(N("href"), obj, ObjWild, E));
  first = 1;
  for (p = hrefs; p; p = p->next) {
    if (ObjIsString(I(p->obj, 2))) {
      if (first) {
        HTML_NewParagraph(stream);
        HTML_Print(stream, "See ");
        first = 0;
      }
      HTML_LinkTo(stream, ObjToString(I(p->obj, 2)));
    }
  }
  ObjListFree(hrefs);

  first = 1;
  TT_HTML_ObjAssertions(stream, obj, 1, objname, english, french, &first);
  TT_HTML_ObjAssertions(stream, obj, 2, objname, english, french, &first);
  if (first == 0) HTML_EndUnnumberedList(stream);
}

/* End of file. */
