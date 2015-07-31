/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19981021T123337: begun
 * 19981022T083019: more work
 * 19981024T112030: database retrieval
 * 19981028T075919: added command set documentation
 * 19981107T093557: added Parents, Children, IsPartOf
 * 19981108T123909: mods to IsPartOf
 * 19981109T095032: mods to returned strings
 * 19981113T114123: select work
 * 19981114T150421: more work
 * 19981115T151512: chatterbot
 * 19981116T141045: syntactic parse, generate
 * 19981120T184203: some case insensitivity
 */

/* Implementation of ThoughtTreasure Server Protocol (TTSP)
 * as defined in <URL:../htm/ttsp.htm>.
 */

#include "tt.h"

#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>

#include "lexentry.h"
#include "lexobjle.h"
#include "repbasic.h"
#include "repchan.h"
#include "repdb.h"
#include "repfifo.h"
#include "repobj.h"
#include "repobjl.h"
#include "repstr.h"
#include "reptext.h"
#include "reptime.h"
#include "semdisc.h"
#include "synpnode.h"
#include "ta.h"
#include "tale.h"
#include "toolapi.h"
#include "toolsvr.h"
#include "utildbg.h"
#include "utillrn.h"

#define BUFSIZE 4096

Socket *sockets[FD_SETSIZE];

/* Socket */

void SocketInit()
{
  int i;
  for (i = 0; i < FD_SETSIZE; i++) sockets[i] = NULL;
}

Socket *SocketCreate(int fd, char *host, int bufsize)
{
  Socket *skt;
  skt = CREATE(Socket);
  skt->fd = fd;
  skt->host = StringCopy(host, "Socket host");
  skt->fifo_read = FifoCreate(bufsize);
  skt->fifo_write = FifoCreate(bufsize);
  skt->dc = NULL;
  sockets[fd] = skt;
  return skt;
}

void SocketFree(Socket *skt)
{
  Dbg(DBGGEN, DBGOK, "%s [%d]: closed", skt->host, skt->fd);
  close(skt->fd);
  sockets[skt->fd] = NULL;
  MemFree(skt->host, "Socket host");
  FifoFree(skt->fifo_read);
  FifoFree(skt->fifo_write);
  if (skt->dc != NULL) API_DiscourseFree(skt->dc);
  MemFree(skt, "Socket");
}

Bool SocketReadLine(Socket *skt, int linelen, /* RESULTS */ char *line)
{
  return FifoReadLine(skt->fifo_read, linelen, line);
}

void SocketWrite(Socket *skt, char *s)
{
  FifoWrite(skt->fifo_write, s);
}

/* Tool_Server */

/* Returns 1 on success,
 *         0 if caller should close connection,
 *         -1 if it is time to exit.
 */
Bool Tool_Server_ProcessLine(Socket *skt, char *line)
{
  char *p, cmd[PHRASELEN];

  p = line;
  p = StringReadWord(p, PHRASELEN, cmd);
  Dbg(DBGGEN, DBGOK, "%s [%d]: <%s>", skt->host, skt->fd, line);
  StringToLowerDestructive(cmd);
  if (cmd[0] == TERM) {
    SocketWrite(skt, "error: empty command\n");
  } else if (streq(cmd, "status")) {
    Tool_Server_Status(skt, p);
  } else if (streq(cmd, "isa")) {
    Tool_Server_ISA(skt, p);
  } else if (streq(cmd, "ispartof")) {
    Tool_Server_IsPartOf(skt, p);
  } else if (streq(cmd, "parents") ||
             streq(cmd, "children") ||
             streq(cmd, "ancestors") ||
             streq(cmd, "descendants")) {
    Tool_Server_AncDesc(skt, cmd, p);
  } else if (streq(cmd, "retrieve")) {
    Tool_Server_Retrieve(skt, p);
  } else if (streq(cmd, "assert")) {
    Tool_Server_Assert(skt, p);
  } else if (streq(cmd, "phrasetoconcepts")) {
    Tool_Server_PhraseToConcepts(skt, p);
  } else if (streq(cmd, "concepttolexentries")) {
    Tool_Server_ConceptToLexEntries(skt, p);
  } else if (streq(cmd, "tag")) {
    Tool_Server_Tag(skt, p);
  } else if (streq(cmd, "syntacticparse")) {
    Tool_Server_SemanticParse(skt, p, 0);
  } else if (streq(cmd, "semanticparse")) {
    Tool_Server_SemanticParse(skt, p, 1);
  } else if (streq(cmd, "generate")) {
    Tool_Server_Generate(skt, p);
  } else if (streq(cmd, "chatterbot")) {
    Tool_Server_Chatterbot(skt, p);
  } else if (streq(cmd, "clearcontext")) {
    Tool_Server_ClearContext(skt);
  } else if (streq(cmd, "quit")) {
    return 0;
  } else if (streq(cmd, "bringdown")) {
    return -1;
  } else {
    SocketWrite(skt, "error: unknown command\n");
  }
  return 1;
}

void Tool_Server_Status(Socket *skt, char *p)
{
  SocketWrite(skt, "up\n");
}

void Tool_Server_ISA(Socket *skt, char *p)
{
  char class[OBJNAMELEN], obj[OBJNAMELEN], *r;
  r = "error: Usage: ISA <class> <obj>\n";
  p = StringReadWord(p, OBJNAMELEN, class);
  if (class[0] != TERM) {
    p = StringReadWord(p, OBJNAMELEN, obj);
    if (obj[0] != TERM) {
      if (ISAP(N(class), N(obj))) r = "1\n";
      else r = "0\n";
    }
  }
  SocketWrite(skt, r);
}

void Tool_Server_IsPartOf(Socket *skt, char *p)
{
  char part[OBJNAMELEN], whole[OBJNAMELEN], *r;
  r = "error: Usage: IsPartOf <part> <whole>\n";
  p = StringReadWord(p, OBJNAMELEN, part);
  if (part[0] != TERM) {
    p = StringReadWord(p, OBJNAMELEN, whole);
    if (whole[0] != TERM) {
      if (DbIsPartOf_Simple(N(part), N(whole), 0)) {
        r = "1\n";
      } else {
        r = "0\n";
      }
    }
  }
  SocketWrite(skt, r);
}

void Tool_Server_AncDesc(Socket *skt, char *cmd, char *p)
{
  int     first;
  char    obj[OBJNAMELEN], *r;
  ObjList *objs, *p1;
  p = StringReadWord(p, OBJNAMELEN, obj);
  if (obj[0] == TERM) goto usage;
  if (streq(cmd, "parents")) {
    objs = ObjParents(N(obj), OBJTYPEANY);
  } else if (streq(cmd, "children")) {
    objs = ObjChildren(N(obj), OBJTYPEANY);
  } else if (streq(cmd, "ancestors")) {
    objs = ObjAncestors(N(obj), OBJTYPEANY);
  } else if (streq(cmd, "descendants")) {
    objs = ObjDescendants(N(obj), OBJTYPEANY);
  } else {
    Dbg(DBGGEN, DBGBAD, "%s [%d]: unknown mode", skt->host, skt->fd);
    objs = NULL;
  }
  first = 1;
  for (p1 = objs; p1; p1 = p1->next) {
    if (!first) SocketWrite(skt, ":");
    if (p1->obj == NULL) continue;
    first = 0;
    SocketWrite(skt, M(p1->obj));
  }
  ObjListFree(objs);
  SocketWrite(skt, "\n");
  return;

usage:
  r = "error: Usage: Ancestors/Descendants <obj>\n";
  SocketWrite(skt, r);
}

/* todo: add ts and tsr arguments */
void Tool_Server_Retrieve(Socket *skt, char *p)
{
  int     len, picki, anci, desci;
  char    pickis[PHRASELEN], ancis[PHRASELEN], descis[PHRASELEN];
  char    mode[PHRASELEN];
  char    buf[OBJNAMELEN], *r;
  Obj     *elems[MAXLISTLEN], *ptn, *obj;
  ObjList *objs, *p1;
  /* <ptn> is list of tokens (a flat concept)
   * <picki>=-1 means return entire assertion
   * <anci>=-1 for exact
   * "Retrieve 2 1 anc something-of somecon ?"
   * "Retrieve 2 -1 exact something-of somecon ?"
   */
  p = StringReadWord(p, PHRASELEN, pickis);
  if (pickis[0] == TERM) goto usage;
  picki = atoi(pickis);
  p = StringReadWord(p, PHRASELEN, ancis);
  if (ancis[0] == TERM) goto usage;
  anci = atoi(ancis);
  p = StringReadWord(p, PHRASELEN, descis);
  if (descis[0] == TERM) goto usage;
  desci = atoi(descis);
  p = StringReadWord(p, PHRASELEN, mode);
  if (mode[0] == TERM) goto usage;
  StringToLowerDestructive(mode);
  len = 0;
  while (1) {
    if (len >= MAXLISTLEN) break;
    p = StringReadWord(p, OBJNAMELEN, buf);
    if (buf[0] == TERM) break;
    elems[len] = TokenToObj(buf);
    len++;
  }
  if (len == 0) goto usage;
  ptn = ObjCreateList1(elems, len);
  objs = NULL; /* REDUNDANT */
  if (streq(mode, "exact")) {
    if (anci != -1) goto usage;
    if (desci != -1) goto usage;
    objs = DbRetrieveExact(&TsNA, NULL, ptn, 1);
  } else if (streq(mode, "anc")) {
    if (anci < 0) goto usage;
    if (desci != -1) goto usage;
    objs = DbRetrieveAnc(&TsNA, NULL, ptn, anci, 1);
  } else if (streq(mode, "desc")) {
    if (desci < 0) goto usage;
    if (anci != -1) goto usage;
    objs = DbRetrieveDesc(&TsNA, NULL, ptn, desci, 1, 1);
  } else if (streq(mode, "ancdesc")) {
    if (anci < 0) goto usage;
    if (desci < 0) goto usage;
    objs = DbRetrieveAncDesc(&TsNA, NULL, ptn, anci, desci, 1);
  } else goto usage;
  for (p1 = objs; p1; p1 = p1->next) {
    if (p1->obj == NULL) continue; /* REDUNDANT */
    if (picki == -1) obj = p1->obj;
    else obj = I(p1->obj, picki);
    if (obj == NULL) continue;
    ObjSocketPrint(skt, obj);
    SocketWrite(skt, "\n");
  }
  SocketWrite(skt, ".\n");
  return;

usage:
  r =
"error: Usage: Retrieve <picki> <anci> <desci> exact|anc|desc|ancdesc <ptn>\n";
  SocketWrite(skt, r);
}

void Tool_Server_Assert(Socket *skt, char *p)
{
  char *r;
  Obj  *obj;
  if (NULL != (obj = ObjReadStringWithTSR(p))) {
    if (ZRER(ObjToTsRange(obj), obj)) {
      Dbg(DBGGEN, DBGOK, "assertion already true");
    } else {
      LearnAssert(obj, skt->dc);
    }
    r = "1\n";
    SocketWrite(skt, r);
  } else {
    SocketWrite(skt, "error: Usage: Assert <obj>\n");
  }
}

void Tool_Server_PhraseToConcepts(Socket *skt, char *p)
{
  int           freeme, lang, len;
  char          feat[FEATLEN], phrase[PHRASELEN], buf[SENTLEN], *r;
  IndexEntry    *ie;
  LexEntryToObj *leo;
  p = StringReadWord(p, FEATLEN, feat);
  if (feat[0] == TERM) goto usage;
  lang = FeatureGetDefault(feat, FT_LANG, F_ENGLISH);
  if (!StringGetLeNonwhite(phrase, p, PHRASELEN)) goto usage;

  ie = LexEntryFindPhrase(LexEntryLangHt1(lang), phrase, INTPOSINF, 0,
                          0, &freeme);
  len = strlen(phrase);
  for (; ie; ie = ie->next) {
    if (ie->lexentry == NULL) continue;
/*
    if (LexEntryFilterOut(ie->lexentry)) continue;
    if (LexEntryHasAnyExpl(ie->lexentry)) continue;
 */
    for (leo = ie->lexentry->leo; leo; leo = leo->next) {
#ifdef _GNU_SOURCE
      snprintf(buf, SENTLEN, "%s:%s:%s:%s:%s:%s\n",
               ie->lexentry->srcphrase,
               ie->lexentry->features,
               phrase,
               ie->features,
               ObjToName(leo->obj),
               leo->features);
#else
      sprintf(buf, "%s:%s:%s:%s:%s:%s\n",
              ie->lexentry->srcphrase,
              ie->lexentry->features,
              phrase,
              ie->features,
              ObjToName(leo->obj),
              leo->features);
#endif
      SocketWrite(skt, buf);
    }
  }
  SocketWrite(skt, ".\n");
  if (freeme) IndexEntryFree(ie);
  return;

usage:
  r = "error: Usage: PhraseToConcepts <features> <phrase>\n";
  SocketWrite(skt, r);
}

void Tool_Server_ConceptToLexEntries(Socket *skt, char *p)
{
  char               con[OBJNAMELEN], buf[SENTLEN], *r;
  Obj                *obj;
  ObjToLexEntry      *ole;
  LexEntry           *le;
  p = StringReadWord(p, OBJNAMELEN, con);
  if (con[0] == TERM) goto usage;
  if ((obj = N(con))) {
    for (ole = obj->ole; ole; ole = ole->next) {
      le = ole->le;
      if (le == NULL) continue;
/*
      if (LexEntryFilterOut(le)) continue;
      if (LexEntryHasAnyExpl(le)) continue;
 */
#ifdef _GNU_SOURCE
      snprintf(buf, SENTLEN, "%s:%s:%s:%s:%s:%s\n",
               le->srcphrase,
               le->features,
               le->srcphrase,
               le->features,
               con,
               ole->features);
#else
      sprintf(buf, "%s:%s:%s:%s:%s:%s\n",
              le->srcphrase,
              le->features,
              le->srcphrase,
              le->features,
              con,
              ole->features);
#endif
      SocketWrite(skt, buf);
    }
  }
  SocketWrite(skt, ".\n");
  return;

usage:
  r = "error: Usage: ConceptToLexEntries <obj>\n";
  SocketWrite(skt, r);
}

void Tool_Server_FmtWMA_SS(char *lephrase, char *lefeat, char *inflphrase,
                           char *inflfeat, char *objname, char *leofeat,
                           size_t lowerb, size_t upperb, int buflen,
                           /* OUTPUT */ char *buf)
{
#ifdef _GNU_SOURCE
  snprintf(buf, buflen, "%s:%s:%s:%s:%s:%s:%lu:%lu\n",
           lephrase, lefeat, inflphrase, inflfeat, objname,
           leofeat, lowerb, upperb);
#else
  sprintf(buf, "%s:%s:%s:%s:%s:%s:%lu:%lu\n",
          lephrase, lefeat, inflphrase, inflfeat, objname,
          leofeat, lowerb, upperb);
#endif
}

void Tool_Server_PNodeTelnoWrite(Socket *skt, PNode *pn)
{
  char buf[SENTLEN], *s;
  Obj *telno, *sc;
  telno = pn->u.telno;
  if (telno == NULL) return;
  if (NULL == (s = ObjToString(telno))) return;
  if (NULL == (sc = ObjToStringClass(telno))) return;
  Tool_Server_FmtWMA_SS(s, FS_NOUN, s, FS_NOUN,
                        ObjToString(sc), "", (long)pn->lowerb,
                        (long)pn->upperb, SENTLEN, buf);
  SocketWrite(skt, buf);
}

void Tool_Server_PNodeMediaObjWrite(Socket *skt, PNode *pn)
{
  char buf[SENTLEN], *s;
  ObjList *objs;
  Obj *obj;
  objs = pn->u.media_obj;
  if (objs == NULL) return;
  obj = objs->obj;
  if (obj == NULL) return;
  if (NULL == (s = ObjToString(obj))) return;
  Tool_Server_FmtWMA_SS(s, FS_NOUN, s, FS_NOUN,
                        "media-object", "", (long)pn->lowerb,
                        (long)pn->upperb, SENTLEN, buf);
  SocketWrite(skt, buf);
}

void Tool_Server_PNodeProductWrite(Socket *skt, PNode *pn)
{
  char buf[SENTLEN], *s;
  ObjList *objs;
  Obj *obj;
  objs = pn->u.product;
  if (objs == NULL) return;
  obj = objs->obj;
  if (obj == NULL) return;
  if (NULL == (s = ObjToString(obj))) return;
  Tool_Server_FmtWMA_SS(s, FS_NOUN, s, FS_NOUN,
                        "product-of", "", (long)pn->lowerb,
                        (long)pn->upperb, SENTLEN, buf);
  SocketWrite(skt, buf);
}

void Tool_Server_PNodeNumberWrite(Socket *skt, PNode *pn)
{
  Float x;
  char nbuf[WORDLEN], buf[SENTLEN];
  Obj *num, *nc;
  num = pn->u.number;
  if (num == NULL) return;
  x = ObjToNumber(num);
  if (NULL == (nc = ObjToNumberClass(num))) return;
  sprintf(nbuf, "%g", x);
  Tool_Server_FmtWMA_SS(nbuf, FS_NOUN, nbuf, FS_NOUN,
                        ObjToString(nc), "", (long)pn->lowerb,
                        (long)pn->upperb, SENTLEN, buf);
  SocketWrite(skt, buf);
}

void Tool_Server_PNodeStringWrite(Socket *skt, PNode *pn)
{
  char buf[SENTLEN], *s;
  s = pn->u.s;
  if (s == NULL) return;
  Tool_Server_FmtWMA_SS(s, FS_NOUN, s, FS_NOUN,
                        "string", "", (long)pn->lowerb,
                        (long)pn->upperb, SENTLEN, buf);
  SocketWrite(skt, buf);
}

void Tool_Server_PNodeCommuniconWrite(Socket *skt, PNode *pn)
{
  char buf[SENTLEN], *s;
  ObjList *objs;
  Obj *obj;
  objs = pn->u.communicons;
  if (objs == NULL) return;
  obj = objs->obj;
  if (obj == NULL) return;
  if (NULL == (s = ObjToString(obj))) return;
  Tool_Server_FmtWMA_SS(s, FS_NOUN, s, FS_NOUN,
                        "communicon", "", (long)pn->lowerb,
                        (long)pn->upperb, SENTLEN, buf);
  SocketWrite(skt, buf);
}

void Tool_Server_PNodeNameWrite(Socket *skt, PNode *pn)
{
  char *name, buf[SENTLEN];
  name = pn->u.name->fullname;
  Tool_Server_FmtWMA_SS(name, FS_NOUN, name, FS_NOUN,
                        "human", "", (long)pn->lowerb,
                        (long)pn->upperb, SENTLEN, buf);
  SocketWrite(skt, buf);
}

void Tool_Server_PNodeLexitemWrite(Socket *skt, PNode *pn)
{
  char          buf[SENTLEN];
  Lexitem       *li;
  LexEntryToObj *leo;
  if (NULL == (li = pn->lexitem)) return;
  if (li->le == NULL || li->le->leo == NULL) {
    Tool_Server_FmtWMA_SS(li->word, li->features, li->word, li->features,
                          "concept", "", (long)pn->lowerb,
                          (long)pn->upperb, SENTLEN, buf);
    SocketWrite(skt, buf);
  } else {
/*
    if (LexEntryFilterOut(li->le)) return;
    if (LexEntryHasAnyExpl(li->le)) return;
 */
    for (leo = li->le->leo; leo; leo = leo->next) {
      Tool_Server_FmtWMA_SS(li->le->srcphrase, li->le->features,
                            li->word, li->features, ObjToName(leo->obj),
                            leo->features, pn->lowerb,
                            pn->upperb, SENTLEN, buf);
      SocketWrite(skt, buf);
    }
  }
}

void Tool_Server_PNodeWrite(Socket *skt, PNode *pn)
{
  switch (pn->type) {
    case PNTYPE_NA:
      break;
    case PNTYPE_LEXITEM:
      Tool_Server_PNodeLexitemWrite(skt, pn);
      break;
    case PNTYPE_NAME:
      Tool_Server_PNodeNameWrite(skt, pn);
      break;
    case PNTYPE_TELNO:
      Tool_Server_PNodeTelnoWrite(skt, pn);
      break;
    case PNTYPE_MEDIA_OBJ:
      Tool_Server_PNodeMediaObjWrite(skt, pn);
      break;
    case PNTYPE_PRODUCT:
      Tool_Server_PNodeProductWrite(skt, pn);
      break;
    case PNTYPE_NUMBER:
      Tool_Server_PNodeNumberWrite(skt, pn);
      break;
    case PNTYPE_STRING:
      Tool_Server_PNodeStringWrite(skt, pn);
      break;
    case PNTYPE_COMMUNICON:
      Tool_Server_PNodeCommuniconWrite(skt, pn);
      break;
    case PNTYPE_TSRANGE:
    case PNTYPE_POLITY:
    case PNTYPE_ORG_NAME:
    case PNTYPE_END_OF_SENT:
    case PNTYPE_CONSTITUENT:
    case PNTYPE_ATTRIBUTION:
    case PNTYPE_EMAILHEADER:
    case PNTYPE_TABLE:
    case PNTYPE_ARTICLE:
    default:
      break;
  }
}

void Tool_Server_Tag(Socket *skt, char *p)
{
  int           lang, dialect, style;
  char          feat[FEATLEN];
  Channel       *ch;
  PNode         *pn;
  p = StringReadWord(p, FEATLEN, feat);
  lang = FeatureGetDefault(feat, FT_LANG, F_ENGLISH);
  dialect = FeatureGetDefault(feat, FT_DIALECT, F_NULL);
  style = FeatureGetDefault(feat, FT_STYLE, F_NULL);
  API_DiscourseSetInput(skt->dc, lang, dialect, style, p);
  API_Tag(skt->dc);
  DiscourseSetCurrentChannel(skt->dc, DCIN);
  ch = &DC(skt->dc);
  for (pn = ch->pnf->first; pn; pn = pn->next) Tool_Server_PNodeWrite(skt, pn);
  SocketWrite(skt, ".\n");
}

void Tool_Server_SemanticParse(Socket *skt, char *p, int isSemParse)
{
  int         lang, dialect, style;
  char        feat[FEATLEN];
  ObjListList *sentences, *sent;
  ObjList     *parse;
  p = StringReadWord(p, FEATLEN, feat);
  lang = FeatureGetDefault(feat, FT_LANG, F_ENGLISH);
  dialect = FeatureGetDefault(feat, FT_DIALECT, F_NULL);
  style = FeatureGetDefault(feat, FT_STYLE, F_NULL);
  API_DiscourseSetInput(skt->dc, lang, dialect, style, p);
  if (API_Tag(skt->dc)) {
    if (isSemParse) {
      sentences = API_SemanticParse(skt->dc);
    } else {
      sentences = API_SyntacticParse(skt->dc);
    }
  } else {
    sentences = NULL;
  }
  for (sent = sentences; sent; sent = sent->next) {
    /* Next sentence. */
    SocketWrite(skt, "SENTENCE\n");
    for (parse = sent->u.objs; parse; parse = parse->next) {
      /* Next parse for this sentence. */
      if (parse->obj == NULL) continue;
      if (isSemParse) {
        ObjSocketPrint(skt, parse->obj);
        SocketWrite(skt, "\n");
      }
      PNodeSocketPrint(skt, parse->u.sp.pn);
      SocketWrite(skt, "\n");
    }
  }
  SocketWrite(skt, ".\n");
}

void Tool_Server_Generate(Socket *skt, char *p)
{
  int  lang, dialect, style;
  char feat[FEATLEN], *r;
  Obj  *obj;
  p = StringReadWord(p, FEATLEN, feat);
  lang = FeatureGetDefault(feat, FT_LANG, F_ENGLISH);
  dialect = FeatureGetDefault(feat, FT_DIALECT, F_NULL);
  style = FeatureGetDefault(feat, FT_STYLE, F_NULL);
  if (NULL != (obj = ObjReadStringWithTSR(p))) {
    r = API_Generate(skt->dc, lang, dialect, style, '.', obj);
    StringMapChar(r, NEWLINE, SPACE);
    StringMapChar(r, CR, SPACE);
    SocketWrite(skt, r);
    SocketWrite(skt, "\n");
    MemFree(r, "char StringReadFile");
  } else {
    SocketWrite(skt, "error: Usage: Generate <features> <obj>\n");
  }
}

void Tool_Server_Chatterbot(Socket *skt, char *p)
{
  int         lang, dialect, style;
  char        feat[FEATLEN], *answer;
  p = StringReadWord(p, FEATLEN, feat);
  lang = FeatureGetDefault(feat, FT_LANG, F_ENGLISH);
  dialect = FeatureGetDefault(feat, FT_DIALECT, F_NULL);
  style = FeatureGetDefault(feat, FT_STYLE, F_NULL);
  answer = API_Chatterbot(skt->dc, lang, dialect, style, p);
  StringMapChar(answer, NEWLINE, SPACE);
  StringMapChar(answer, CR, SPACE);
  SocketWrite(skt, answer);
  SocketWrite(skt, "\n");
  MemFree(answer, "char StringReadFile");
}

void Tool_Server_ClearContext(Socket *skt)
{
  Ts	ts;
  TsSetNow(&ts);
  DiscourseContextInit(skt->dc, &ts);
  SocketWrite(skt, "1\n");
}

void Tool_Server_NewConnection(int s)
{
  socklen_t peer_len;
  long l;
  char *nhost, *host;
  struct hostent *he;
  struct sockaddr_in peer;
  Socket *skt;

  peer_len = sizeof(struct sockaddr_in);
  getpeername(s, (struct sockaddr *)&peer, &peer_len);

  nhost = inet_ntoa(peer.sin_addr);

  if (NULL == (he = gethostbyaddr((char *)&peer.sin_addr.s_addr,
                                  sizeof(peer.sin_addr.s_addr),
                                  AF_INET))) {
    host = nhost;
    Dbg(DBGGEN, DBGOK, "connection from %s", host);
  } else {
    host = he->h_name;
    Dbg(DBGGEN, DBGOK, "connection from %s [%s]",
        host, nhost);
  }

  l = 1L;
  if (-1 == setsockopt(s, SOL_SOCKET, SO_LINGER, (char *)&l, sizeof(long))) {
    Dbg(DBGGEN, DBGBAD, "%s [%d]: setsockopt trouble: %s", host, s,
        strerror(errno));
  }

  if (-1 == fcntl(s, F_SETFL, O_NONBLOCK)) {
    Dbg(DBGGEN, DBGBAD, "%s [%d]: fcntl trouble: %s", host, s,
        strerror(errno));
    close(s);
    return;
  }

  skt = SocketCreate(s, host, BUFSIZE);
  skt->dc = API_DiscourseCreate();
}

void Tool_Server_SelectLoop(int ls)
{
  int retcode;
  while (1) {
    retcode = Tool_Server_ReadAndWrite(ls);
    if (retcode == -1) {
      Dbg(DBGGEN, DBGBAD, "select trouble: %s", strerror(errno));
      return;
    }
    if (retcode == 0) continue;
    if (!Tool_Server_Process(ls)) return;
  }
}

int Tool_Server_ReadAndWrite(int ls)
{ 
  int            i, maxfd, retcode;
  fd_set         readfds, writefds, exceptfds;
  struct timeval tv;
  Socket         *skt;

  FD_ZERO(&readfds);
  FD_ZERO(&writefds);
  FD_ZERO(&exceptfds);
  maxfd = 0;
  for (i = 0; i < FD_SETSIZE; i++) {
    if (sockets[i] != NULL) {
      FD_SET(i, &readfds);
      if (!FifoIsEmpty(sockets[i]->fifo_write)) FD_SET(i, &writefds);
      FD_SET(i, &exceptfds);
      maxfd = i;
    }
  }
  tv.tv_usec = 0;
  tv.tv_sec = 1;
  retcode = select(maxfd+1, &readfds, &writefds, &exceptfds, &tv);
  if (retcode <= 0) return retcode;
  for (i = 0; i < FD_SETSIZE; i++) {
    skt = sockets[i];
    if (FD_ISSET(i, &exceptfds)) {
      Dbg(DBGGEN, DBGOK, "%s [%d]: exception", skt->host, skt->fd);
      SocketFree(skt);
    } else {
      if (FD_ISSET(i, &writefds)) {
        Tool_Server_SelectWrite(skt);
      }
      if (FD_ISSET(i, &readfds)) {
        Tool_Server_SelectRead(skt, ls);
      }
    }
  }
  return retcode;
}

/* returns 0 if it is time to exit. */
Bool Tool_Server_Process(int ls)
{
  int  i, retcode;
  char line[PARAGRAPHLEN];
  Socket *skt;
  for (i = 0; i < FD_SETSIZE; i++) {
    if (sockets[i] != NULL && sockets[i]->fd != ls) {
      skt = sockets[i];
      while (1) {
        if (!SocketReadLine(skt, PARAGRAPHLEN, line)) break;
        retcode = Tool_Server_ProcessLine(skt, line);
        if (retcode == -1) {
          return 0;
        } else if (retcode == 0) {
          SocketFree(skt);
          break;
        }
      }
    }
  }
  return 1;
}

void Tool_Server_SelectWrite(Socket *skt)
{
  char *buf;
  int to_write, actually_written;
  while (1) {
    FifoRemoveBegin(skt->fifo_write, &buf, &to_write);
    if (to_write == 0) return;
    actually_written = send(skt->fd, buf, to_write, 0);
    if (-1 == actually_written) {
      if (errno == EWOULDBLOCK) return;
      Dbg(DBGGEN, DBGBAD, "%s [%d]: socket send trouble: %s",
          skt->host, skt->fd,
          strerror(errno));
      SocketFree(skt);
      FifoRemoveEnd(skt->fifo_write, 0);
      return;
    }
    FifoRemoveEnd(skt->fifo_write, actually_written);
  }
}

void Tool_Server_SelectRead(Socket *skt, int ls)
{
  if (skt->fd == ls) {
    Tool_Server_SelectReadListen(skt, ls);
  } else {
    Tool_Server_SelectReadConnection(skt);
  }
}

void Tool_Server_SelectReadListen(Socket *skt, int ls)
{
  int s;
  socklen_t len;
  struct sockaddr_in s_sai;
  len = sizeof(struct sockaddr_in);
  memset((char *)&s_sai, 0, len);
  if (-1 == (s = accept(ls, (struct sockaddr *)&s_sai, &len))) {
    Dbg(DBGGEN, DBGBAD, "accept trouble: %s", strerror(errno));
    return;
  }
  Tool_Server_NewConnection(s);
}

void Tool_Server_SelectReadConnection(Socket *skt)
{
  char *buf;
  int space_for, actually_read;
  while (1) {
    FifoAddBegin(skt->fifo_read, &buf, &space_for);
    actually_read = recv(skt->fd, buf, space_for, 0);
    if (0 == actually_read) {
    /* closed by peer */
      SocketFree(skt);
      FifoAddEnd(skt->fifo_read, 0);
      return;
    }
    if (-1 == actually_read) {
      if (errno == EWOULDBLOCK) return;
      Dbg(DBGGEN, DBGBAD, "%s [%d]: socket recv trouble: %s",
          skt->host, skt->fd,
          strerror(errno));
      SocketFree(skt);
      FifoAddEnd(skt->fifo_read, 0);
      return;
    }
    FifoAddEnd(skt->fifo_read, actually_read);
#ifdef notdef
    /* Enable this on Win32 platform when using Cygnus Cygwin.
     * (Due to failure to map GNU fcntl O_NONBLOCK to Winsock FIONBIO.)
     */
    return;
#endif
  }
}

void Tool_Server(int port)
{
  int                ls, i, retcode;
  struct sockaddr_in ls_sai;
  Socket             *lskt;

  Dbg(DBGGEN, DBGOK, "starting ThoughtTreasure server");

  SocketInit();

  if (-1 == (ls = socket(AF_INET, SOCK_STREAM, 0))) {
    Dbg(DBGGEN, DBGBAD, "listen socket trouble: %s", strerror(errno));
    return;
  }

  for (i = 0; i < 100; i++) {
    memset((char *)&ls_sai, 0, sizeof(struct sockaddr_in));
    ls_sai.sin_family = AF_INET;
    ls_sai.sin_addr.s_addr = INADDR_ANY;
    ls_sai.sin_port = htons(port);
    if (-1 !=
        (retcode =
         bind(ls, (struct sockaddr *)&ls_sai, sizeof(struct sockaddr_in)))) {
      break;
    }
    port--;
    Dbg(DBGGEN, DBGBAD, "bind trouble; trying again with port %d", port);
  }
  if (retcode == -1) {
    Dbg(DBGGEN, DBGBAD, "bind trouble; giving up");
    return;
  }

  if (-1 == listen(ls, 6)) {
    Dbg(DBGGEN, DBGBAD, "listen trouble: %s", strerror(errno));
    return;
  }

  lskt = SocketCreate(ls, "listen socket", 0);

  Dbg(DBGGEN, DBGOK, "server started; port = %d", port);
  Tool_Server_SelectLoop(ls);

  for (i = 0; i < FD_SETSIZE; i++) {
    if (sockets[i] != NULL) SocketFree(sockets[i]);
  }
  Dbg(DBGGEN, DBGOK, "server exited");
}

/* End of file. */
