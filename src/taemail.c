/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19941208: begun
 * 19941209: more work
 * 19941213: converted for PNodes
 * 19950222: added FirstClass parsing and improved learning code
 * 19950223: more perfecting
 *
 * todo:
 * - Learn organizations.
 * - Infer nationality from organization? (pegasus.rutgers.edu=>country-USA)
 * - Treat postmaster, root, daemon specially.
 */

#include "tt.h"
#include "lexentry.h"
#include "lexobjle.h"
#include "repbasic.h"
#include "repchan.h"
#include "repdb.h"
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

EmailHeader *EmailHeaderCreate()
{
  EmailHeader	*emh;
  emh = CREATE(EmailHeader);
  emh->header_type = N("standard-email-header");
  emh->message_id = NULL;
  TsSetNa(&emh->send_ts);
  TsSetNa(&emh->receive_ts);
  emh->return_path[0] = TERM;
  emh->from_obj = NULL;
  emh->from_address[0] = TERM;
  emh->reply_obj = NULL;
  emh->reply_address[0] = TERM;
  emh->to_objs = NULL;
  emh->cc_objs = NULL;
  TsSetNa(&emh->resent_ts);
  emh->resent_from_obj = NULL;
  emh->resent_from_address[0] = TERM;
  emh->resent_to_objs = NULL;
  emh->resent_cc_objs = NULL;
  emh->subject[0] = TERM;
  emh->newsgroup = NULL;
  emh->newsgroups = NULL;
  emh->article_number = 0L;
  emh->mime_version[0] = TERM;
  return(emh);
}

void EmailHeaderFree(EmailHeader *emh)
{
  MemFree(emh, "EmailHeader");
}

void EmailStripRe(char *s)
{
  char	*in;
  in = s;
  while (StringHeadEqualAdvance("Re:", in, &in)) {
    in = StringSkipWhitespaceNonNewline(in);
  }
  StringCpy(s, in, PHRASELEN);
}

void EmailHeaderAdjust(EmailHeader *emh)
{
  EmailStripRe(emh->subject);
  if (emh->newsgroups &&
      emh->newsgroup == NULL) {
    emh->newsgroup = emh->newsgroups->obj;
  }
}

/* For each slot, <src_emh> overrides <dest_emh> except where the slot is
 * capable of holding multiple values in which case the values from
 * <src_emh> are appended.
 */
void EmailHeaderMergeInto(EmailHeader *dest, EmailHeader *src)
{
  if (src->message_id) dest->message_id = src->message_id;
  if (TsIsSpecific(&src->send_ts)) dest->send_ts = src->send_ts;
  if (TsIsSpecific(&src->receive_ts)) dest->receive_ts = src->receive_ts;
  if (src->return_path[0]) {
    StringCpy(dest->return_path, src->return_path, PHRASELEN);
  }
  if (src->from_obj) dest->from_obj = src->from_obj;
  if (src->from_address[0]) {
    StringCpy(dest->from_address, src->from_address, PHRASELEN);
  }
  if (src->reply_obj) dest->reply_obj = src->reply_obj;
  if (src->reply_address[0]) {
    StringCpy(dest->reply_address, src->reply_address, PHRASELEN);
  }
  dest->to_objs = ObjListAppendNonequal(dest->to_objs, src->to_objs);
  dest->cc_objs = ObjListAppendNonequal(dest->cc_objs, src->cc_objs);
  if (TsIsSpecific(&src->resent_ts)) dest->resent_ts = src->resent_ts;
  if (src->resent_from_obj) dest->resent_from_obj = src->resent_from_obj;
  if (src->resent_from_address[0]) {
    StringCpy(dest->resent_from_address, src->resent_from_address, PHRASELEN);
  }
  dest->resent_to_objs =
    ObjListAppendNonequal(dest->resent_to_objs, src->resent_to_objs);
  dest->resent_cc_objs =
    ObjListAppendNonequal(dest->resent_cc_objs, src->resent_cc_objs);
  if (src->subject[0]) StringCpy(dest->subject, src->subject, PHRASELEN);
  if (src->newsgroup) dest->newsgroup = src->newsgroup;
  dest->newsgroups = ObjListAppendNonequal(dest->newsgroups, src->newsgroups);
  if (dest->article_number != 0L) dest->article_number = src->article_number;
  if (src->mime_version[0]) {
    StringCpy(dest->mime_version, src->mime_version, PHRASELEN);
  }
  EmailHeaderAdjust(dest);
}

void EmailHeaderPrint(FILE *stream, EmailHeader *emh)
{
  fprintf(stream, "<%s><%s><", M(emh->header_type), M(emh->message_id));
  TsPrint(stream, &emh->receive_ts);
  fprintf(stream, ">FROM<%s><%s>", M(emh->from_obj), emh->from_address);
  if (emh->to_objs) {
    fprintf(stream, "TO<%s>", M(emh->to_objs->obj));
  }
  fprintf(stream, "SUBJ<%s>", emh->subject);
  if (emh->newsgroup) {
    fprintf(stream, "NEWSGROUP<%s>", M(emh->newsgroup));
  }
}

Ts *EmailHeaderTs(EmailHeader *emh)
{
  if (TsIsSpecific(&emh->send_ts)) return(&emh->send_ts);
  else if (TsIsSpecific(&emh->receive_ts)) return(&emh->receive_ts);
  else if (TsIsSpecific(&emh->resent_ts)) return(&emh->resent_ts);
  else return(NULL);
}

Bool TA_EmailIsSlotnameChar(int c)
{
  return(CharIsUpper(c) || CharIsLower(c) || c == '-');
}

Bool TA_EmailIsNewsgroupChar(int c)
{
  return(CharIsLower(c) || c == '.');
}

Bool TA_EmailIsQuotationChar(int c)
{
  return(c == '>');
}

Bool TA_EmailIsNewsgroupString(char *s)
{
  if (s[0] == TERM) return(0);
  while (*s) {
    if (!TA_EmailIsNewsgroupChar(*s)) return(0);
    s++;
  }
  return(1);
}

Bool TA_EmailParseUser(char *user, /* RESULTS */ char *username, char *domain)
{
  int	found;
  user = StringReadTo1(user, username, PHRASELEN, '@', &found);
  if (found) {
    user = StringReadTo1(user, domain, PHRASELEN, TERM, NULL);
  } else {
    StringCpy(domain, "localhost", PHRASELEN);
  }
  StringAllUpperToLower(username);
    /* GARNIER => garnier, but Jim_Garnier untouched */
  return(1);
}

Bool TA_EmailParseEntity1(char *in, /* RESULTS */ char *fullname, char *user)
{
  int	found;

  /* BEGIN FirstClass formats. */
  if ((!StringAnyIn("\"@<", in)) && StringIn(SPACE, in)) {
  /* Christophe Reverd */
    StringCpy(fullname, in, PHRASELEN);
    user[0] = TERM;
    return(1);
  }
  if (StringTailEq(in, ",Internet") || StringTailEq(in, ",Usenet")) {
    if (2 == StringCountOccurrences(in, ',')) {
    /* Jim Garnier,jgarnier@dialup.francenet.fr,Internet
     * "Likey K. Rose",lkr@pogs.com,Internet
     */
      in = StringReadTo1(in, fullname, PHRASELEN, ',', &found);
      if ((!found) || fullname[0] == TERM) return(0);
      in = StringReadTo1(in, user, PHRASELEN, ',', &found);
      if ((!found) || user[0] == TERM) return(0);
      return(1);
    } else {
    /* Sylvie@francenet.fr,Internet
     * HERMES@france.com,Internet
     */
      fullname[0] = TERM;
      in = StringReadTo1(in, user, PHRASELEN, ',', &found);
      if ((!found) || user[0] == TERM) return(0);
      return(1);
    }
  }
  /* END FirstClass formats. */
   
  if (*in == DQUOTE) {
  /* "Peter Pumpkin" <ppumpkin> */
    in = StringReadTo1(in+1, fullname, PHRASELEN, DQUOTE, &found);
    if ((!found) || fullname[0] == TERM || in[0] != SPACE || in[1] != '<') {
      return(0);
    }
    in = StringReadTo1(in+2, user, PHRASELEN, '>', &found);
    if ((!found) || user[0] == TERM) return(0);
    return(1);
  }
  if (*in == '<') {
  /* From: <10266423.98452@computron.com> */
    in = StringReadTo1(in+1, user, PHRASELEN, '>', NULL);
    fullname[0] = TERM;
    return(1);
  }
  if (StringIn('<', in) && StringIn('>', in)) {
  /* Peter Huttig <ph@zache.itf.edu>
   * David Jacobson <IKZYPS1@lcavs.bitnet>
   */
    in = StringReadTo1(in, fullname, PHRASELEN, '<', &found);
    StringElimTrailingBlanks(fullname);
    if ((!found) || fullname[0] == TERM) return(0);
    in = StringReadTo1(in, user, PHRASELEN, '>', &found);
    if ((!found) || user[0] == TERM) return(0);
    return(1);
  }
  /* RZA@MOGE.COM
   * tik@sss6 (Joe Tik)
   * cyberclue@qst.edu (CYBERCLUE Moderator)
   * John.Jones@tellie.com
   * Anne_Jones@skymail.com
   */
  in = StringReadTo1(in, user, PHRASELEN, SPACE, NULL);
  if (in[0] == LPAREN) {
    in = StringReadTo1(in+1, fullname, PHRASELEN, RPAREN, &found);
    if ((!found) || user[0] == TERM) return(0);
  } else fullname[0] = TERM;
  return(1);
}

/* Parse an element of a From: or To: line into fullname and user.
 * Extract the fullname, username, and domain as best you can.
 *
 * Example inputs are shown in TA_EmailParseEntity1.
 *
 * Example results:
 * fullname = "Jim Garnier"
 * username = jgarnier
 * domain = dreamland.com
 * address = jgarnier@dreamland.com
 */
Bool TA_EmailParseEntity(char *in, /* RESULTS */ char *fullname, char *username,
                         char *domain, char *address)
{
  char	user[PHRASELEN];
  if (!TA_EmailParseEntity1(in, fullname, user)) return(0);
  if (!TA_EmailParseUser(user, username, domain)) return(0);
  if (fullname[0] == TERM) {
    if (StringIn('_', username) && StringIsAlphaOr(username, "_ ")) {
      StringCpy(fullname, username, PHRASELEN);
      StringMapChar(fullname, '_', SPACE);
    } else if (StringIn('.', username) && StringIsAlphaOr(username, ". ")) {
      StringCpy(fullname, username, PHRASELEN);
      StringMapChar(fullname, '.', SPACE);
    } /* todo: Parse usernames such as "jbucking". */
  }
  sprintf(address, "%s@%s", username, domain);
  return(1);
}

void TA_EmailAssertAddress(Ts *ts, Obj *entity, Obj *address, Discourse *dc)
{
  Obj		*obj;
  TsRange	*tsr;
  if (N("@localhost") == address) Stop();
  obj = L(N("email-address-of"), entity, address, E);
  tsr = ObjToTsRange(obj);
  TsRangeSetNa(tsr);
  tsr->startts = *ts;
  LearnAssert(obj, dc);
}

/* Resolve fullname and address into a ThoughtTreasure object.
 *
 * Notes:
 * 1 name corresponding to 2 email addresses might be 2 people,
 * or it might be 1 person. For now we assume it is 2.
 * There's no set relationship, since:
 * Fullnames are not unique identifiers.
 * People sometimes have more than one email address.
 * Domain aliases may lead to multiple email addresses.
 * People sometimes share accounts.
 *
 * Need to look at the rest of the message for signature, mailing address,
 * and phone information. Of course even this isn't perfect:
 * A person may have more than one home. They can even be on different coasts.
 * A person may have more than one phone number. Work and home numbers is one
 * example.
 */
Bool TA_EmailResolveEntity(Ts *ts, char *fullname, char *address, Discourse *dc,
                        /* RESULTS */ Obj **entity)
{
  Obj		*address_obj;
  ObjList	*in_kb_entities, *entities, *p;
  Name		*nm;

  if ((address_obj = NameToObj(address, OBJ_NO_CREATE)) &&
      ISA(N("email-address"), address_obj)) {
  /* Address already in database. */
    in_kb_entities = REI(1, &TsNA, L(N("email-address-of"), ObjWild,
                         address_obj, E));
  } else {
  /* New address. */
    address_obj = StringToObj(address, N("email-address"), 1);
    in_kb_entities = NULL;
  }

  if (fullname[0]) nm = TA_NameParseKnown(fullname, F_NULL, 0);
  else nm = NULL;
  /* todoFREE: nm */
  if (nm && (entities = DbHumanNameRetrieve(&TsNA, NULL, nm))) {
    if (in_kb_entities) {
      /* Fullname and email address are in database. */
      for (p = in_kb_entities; p; p = p->next) {
        if (ObjListIn(p->obj, entities)) {
          *entity = p->obj;
          return(1);
        }
      }
      Dbg(DBGGEN, DBGBAD, "TA_EmailParseEntity: clash");
      ObjListPrint(Log, in_kb_entities);
      Dbg(DBGGEN, DBGBAD, "------");
      ObjListPrint(Log, entities);
      return(0);
    } else {
      /* Fullname is in database but not email address.
       * Associate the email address with the fullname in database.
       */
      TA_EmailAssertAddress(ts, entities->obj, address_obj, dc);
      *entity = entities->obj;
      if (!ObjListIsLengthOne(entities)) {
        Dbg(DBGGEN, DBGBAD, "TA_EmailParseEntity: fullname <%s> not unique",
            fullname);
      }
      return(1);
    }
  }
  /* Fullname is not in database. */
  if (in_kb_entities) {
    /* Email address is in database. */
    if (nm) {
      /* Finding out someone's name where previously we only knew their
       * email address.
       */
      LexEntryNameForHuman(in_kb_entities->obj, F_NULL, nm);
    }
    *entity = in_kb_entities->obj;
    if ((!ObjListIsLengthOne(in_kb_entities)) && DbgOn(DBGGEN, DBGBAD)) {
        Dbg(DBGGEN, DBGBAD, "TA_EmailParseEntity: address <%s> not unique",
            M(address_obj));
        ObjListPrint(Log, in_kb_entities);
    }
    return(1);
  } else {
  /* Neither email address nor fullname are in database. */
    /* todo: Gender guessing. Unify name code. */
    /* todo: Need to dump new object out to outlrn.txt file. */
    if (nm) {
      if ((*entity = LearnHuman(nm, nm->fullname, dc, NULL))) {
        TA_EmailAssertAddress(ts, *entity, address_obj, dc);
        return(1);
      }
      Dbg(DBGGEN, DBGBAD, "TA_EmailParseEntity: LearnHuman failure");
      return(0);
    }
    /* Create a human with no name. */
    *entity = ObjCreateInstance(N("human"), NULL);
    /* todo: Use a more informative objname. */
    LearnObjDUMP(N("human"), *entity, M(*entity), "º", NULL, 1, 
                 OBJ_CREATE_C, NULL, dc);
    TA_EmailAssertAddress(ts, *entity, address_obj, dc);
    return(1);
  }
}

Bool TA_EmailParseAndResolveEntity(char *in, Ts *ts, Discourse *dc,
                                /* RESULTS */ char *address, Obj **entity)
{
  char	fullname[PHRASELEN];
  char	username[PHRASELEN], domain[PHRASELEN];
  if (!TA_EmailParseEntity(in, fullname, username, domain, address)) return(0);
  if (!TA_EmailResolveEntity(ts, fullname, address, dc, entity)) return(0);
  return(1);
}

/* Detect first line of an email (or news) message
 * Examples:
 * From akj@n6 Fri Mar 22 12:29:18 1995
 * Path: francenet.fr!oleane!jussieu.fr!math.ohio-state.edu!
 *  howland.reston.ans.net!news.sprintlink.net!uunet!in1.uu.net!
 *  dziuxsolim.rutgers.edu!pegasus.rutgers.edu!not-for-mail
 * Article 26880 of rec.autos.marketplace:
 *
 * Returns pointer to beginning of next line.
 */
Bool TA_EmailParseTop(char *in, /* RESULTS */ char *topentity, Ts *receive_ts,
                      char **nextp)
{
  int	found, len;
  char	path[LINELEN], number[DWORDLEN], newsgroup[PHRASELEN];
  if (StringHeadEqualAdvance("Article ", in, &in)) {
    if (!StringGetWord(number, in, DWORDLEN, &in)) return(0);
    if (!UnsignedIntIsString(number)) return(0);
    if (!StringHeadEqualAdvance("of ", in, &in)) return(0);
    if (!StringGetWord(newsgroup, in, DWORDLEN, &in)) return(0);
    len = strlen(newsgroup);
    if (newsgroup[len-1] != ':') return(0);
    newsgroup[len-1] = TERM;
    if (!TA_EmailIsNewsgroupString(newsgroup)) return(0);
    if (*in != NEWLINE) return(0);
    *nextp = in+1;
    topentity[0] = TERM;
    TsSetNa(receive_ts);
    return(1);
  }
  if (StringHeadEqual("From ", in)) {
    if (!StringGetWord(topentity, in+5, LINELEN, &in)) return(0);
    if (!TA_TimeUnixEtc(in, receive_ts, &in)) return(0);
    if (*in == NEWLINE) in++;
    *nextp = in;
    return(1);
  }
  if (StringHeadEqual("Path: ", in)) {
    in = StringReadTo1(in, path, LINELEN, NEWLINE, &found);
    if (!found) return(0);
    if (!StringIn('!', path)) return(0);
    *nextp = in;
    topentity[0] = TERM;
    TsSetNa(receive_ts);
    return(1);
  }
  return(0);
}

Bool TA_EmailParseRmailHeader(char *in, /* RESULTS */ char **nextp)
{
  Bool	found;
  found = 0;
  if (StringHeadEqualAdvance("", in, &in)) {
    if (StringSkipChar(in, NEWLINE, &in)) return(found);
    found = 1;
  }
  if (StringHeadEqualAdvance("0, unseen,,", in, &in)) {
    if (StringSkipChar(in, NEWLINE, &in)) return(found);
    found = 1;
  }
  if (StringHeadEqualAdvance("*** EOOH ***", in, &in)) {
    if (StringSkipChar(in, NEWLINE, &in)) return(found);
    found = 1;
  }
  if (found) *nextp = in;
  return(found);
}

/* Examples:
 * Subject: ideas
 * Status: RO
 * Content-Length: 368
 */
Bool TA_EmailParseStringSlot(char *in, char *slotname, int maxlen,
                          /* RESULTS */ char *slotvalue, char **nextp)
{
  int	found;
  if (StringHeadEqualAdvance(slotname, in, &in)) {
    if (!StringHeadEqualAdvance(" ", in, &in)) return(0);
    in = StringSkipWhitespaceNonNewline(in);
    if (slotvalue[0]) {
      Dbg(DBGGEN, DBGBAD, "duplicate email slot <%s> was <%s>", slotname,
          slotvalue);
    }
    in = StringReadTo1(in, slotvalue, maxlen, NEWLINE, &found);
    if (found) {
      *nextp = in;
      return(1);
    }
  }
  return(0);
}

/* Return-Path: <zahn@hub98.oul.kw.com> */
Bool TA_EmailParseAngleBracketSlot(char *in, char *slotname,
                                /* RESULTS */ char *slotvalue, char **nextp)
{
  char	*orig_in;
  int	found;
  orig_in = in;
  if (StringHeadEqualAdvance(slotname, in, &in)) {
    if (!StringHeadEqualAdvance(" <", in, &in)) goto parsefailure;
    if (slotvalue[0]) {
      Dbg(DBGGEN, DBGBAD, "duplicate email slot <%s> was <%s>", slotname,
          slotvalue);
    }
    in = StringReadTo1(in, slotvalue, PHRASELEN, '>', &found);
    if (found) {
      if (!StringSkipChar(in, NEWLINE, &in)) goto parsefailure;
      *nextp = in;
      return(1);
    }
  }
  return(0);
parsefailure:
  Dbg(DBGGEN, DBGBAD,
      "TA_EmailParseAngleBracketSlot: trouble parsing <%.64s> <%.64s>",
      orig_in, in);
  slotvalue[0] = TERM;
  return(0);
}

Bool TA_EmailParseAngleObjectSlot(char *in, char *slotname, Obj *class,
                               /* RESULTS */ Obj **slotvalue, char **nextp)
{
  char	*orig_in, buf[PHRASELEN];
  int	found;
  orig_in = in;
  if (StringHeadEqualAdvance(slotname, in, &in)) {
    if (!StringHeadEqualAdvance(" <", in, &in)) goto parsefailure;
    if (slotvalue[0]) {
      Dbg(DBGGEN, DBGBAD, "duplicate email slot <%s> was <%s>",
          slotname, slotvalue);
    }
    in = StringReadTo1(in, buf, PHRASELEN, '>', &found);
    if (found) {
      *slotvalue = StringToObj(buf, class, 0);
      if (!StringSkipChar(in, NEWLINE, &in)) goto parsefailure;
      *nextp = in;
      return(1);
    }
  }
  return(0);
parsefailure:
  Dbg(DBGGEN, DBGBAD,
      "TA_EmailParseAngleObjectSlot: trouble parsing <%.64s> <%.64s>",
      orig_in, in);
  *slotvalue = NULL;
  return(0);
}

/* Examples:
 * Received: from a1.ai.dreamland.com (a1-f0) by a2.ai.dreamland.com
 *   (4.1/TT/QQQ-1.0) id AA16532; Tue, 15 Mar 95 19:47:56 +0100
 * Received: from gateway.dreamland.com by a1.ai.dreamland.com (4.1/TT/QQQ-1.0)
 *   id AA20695; Tue, 15 Mar 95 13:47:54 EST
 * Received: from relay2.UU.NET ([192.48.96.7]) by gateway.dreamland.com
 *   with SMTP id <41486>; Tue, 15 Mar 1995 13:47:39 -0500
 * Received: from rdems.idea.com by relay2.UU.NET with SMTP 
 *   (5.61/UUNET-internet-primary) id AAwhje04888; Tue, 15 Mar 95 13:37:02 -0500
 * Received:  from rdns.rd.idea.com by rdems.idea.com (5.65/GE 1.76) id
 *   AA13325; Tue, 15 Mar 95 13:36:39 -0500
 * Received: by dreamland.com (4.1/TT/QQQ-1.0)
 *	id AA01051; Wed, 2 Mar 95 07:35:01 EST
 */
Bool TA_EmailParseReceived(char *in, /* RESULTS */ char *from, char *by,
                           Ts *ts, char **nextp)
{
  char	*orig_in;
  orig_in = in;
  if (StringHeadEqualAdvance("Received: from ", in, &in) ||
      StringHeadEqualAdvance("Received:  from ", in, &in)) {
    if (!StringGetWord(from, in, PHRASELEN, &in)) goto parsefailure;
    in = StringSkipParenthetical(in);
    if (!StringHeadEqualAdvance(" by ", in, &in)) goto parsefailure;
    if (!StringGetWord(by, in, PHRASELEN, &in)) goto parsefailure;
    if (!StringSkipToAcrossNewlines(in, ';', 1, &in)) goto parsefailure;
    if (!StringSkipChar(in, SPACE, &in)) goto parsefailure;
    if (!TA_TimeUnixEtc(in, ts, &in)) goto parsefailure;
    if (*in == NEWLINE) in++;
    *nextp = in;
    return(1);
  } else if (StringHeadEqualAdvance("Received: by ", in, &in)) {
    from[0] = TERM;
    if (!StringGetWord(by, in, PHRASELEN, &in)) goto parsefailure;
    if (!StringSkipToAcrossNewlines(in, ';', 1, &in)) goto parsefailure;
    if (!StringSkipChar(in, SPACE, &in)) goto parsefailure;
    if (!TA_TimeUnixEtc(in, ts, &in)) goto parsefailure;
    if (*in == NEWLINE) in++;
    *nextp = in;
    return(1);
  }
  return(0);
parsefailure:
  Dbg(DBGGEN, DBGBAD, "trouble parsing <%.64s> <%.64s>", orig_in, in);
  from[0] = TERM;
  by[0] = TERM;
  TsSetNa(ts);
  return(0);
}

/* Examples:
 * Date: 	Tue, 15 Mar 1994 13:37:43 -0500
 * Date: Fri, 22 Mar 91 10:50:02 EST
 * Date: 6 Jul 89 13:42:43 GMT
 * Date: Tue, 25 Jul 95 00:39:03 GMT
 */
Bool TA_EmailParseDate(char *in, char *slotname, /* RESULTS */ Ts *ts,
                       char **nextp)
{
  char	*orig_in;
  orig_in = in;
  if (StringHeadEqualAdvance(slotname, in, &in)) {
    in = StringSkipWhitespaceNonNewline(in);
    if (!TA_TimeUnixEtc(in, ts, &in)) goto parsefailure;
    if (*in == NEWLINE) in++;
    *nextp = in;
    return(1);
  }
  return(0);
parsefailure:
  Dbg(DBGGEN, DBGBAD, "TA_EmailParseDate: trouble parsing <%.64s> <%.64s>",
      orig_in, in);
  TsSetNa(ts);
  return(0);
}

/* From: lexy@ubo.krl.puz.com (lex yonicke)
 * From: <18232664.6452@computron.com>
 */
Bool TA_EmailParseFrom(char *in, char *slotname, Ts *ts, char *topentity_string,
                    Discourse *dc, /* RESULTS */ char *address, Obj **entity,
                    char **nextp)
{
  char	*orig_in, from_entity[PHRASELEN], top_address[PHRASELEN];
  Obj	*top_entity;
  orig_in = in;
  if (StringHeadEqualAdvance(slotname, in, &in)) {
    in = StringSkipWhitespaceNonNewline(in);
    in = StringReadTo(in, from_entity, PHRASELEN, NEWLINE, TERM, TERM);
    if (!TA_EmailParseAndResolveEntity(from_entity, ts, dc, address, entity)) {
      goto parsefailure;
    }
    if (topentity_string &&
        StringIn('@', topentity_string) &&
        TA_EmailParseAndResolveEntity(topentity_string, ts, dc, top_address,
                                      &top_entity)) {
      /* But for moderated Usenet news the top entity doesn't correspond to
       * the From:.
       */
      if (!streq(top_address, address)) {
        Dbg(DBGGEN, DBGBAD, "TA_EmailParseFrom: top <%s> != from <%s>",
            top_address, address);
      }
      if (top_entity && *entity && top_entity != *entity) {
        Dbg(DBGGEN, DBGBAD, "<%s> top <%s> != from <%s>",
            slotname, M(top_entity), M(*entity));
      }
    }
    *nextp = in;
    return(1);
  }
  return(0);
parsefailure:
  Dbg(DBGGEN, DBGBAD, "TA_EmailParseFrom: trouble parsing <%.64s> <%.64s>",
      orig_in, in);
  address[0] = TERM;
  *entity = NULL;
  return(0);
}

/* To: lurker (Michael Lurker), pete, johnr
 * To: harry@xjy4, zappo@pumpkin
 * Dest:            soc.culture.french
 */
Bool TA_EmailParseTo(char *in, char *slotname, char *prevslotname, Ts *ts,
                     Discourse *dc, /* RESULTS */ ObjList **to,
                     ObjList **newsgroups, char **nextp)
{
  ObjList	*r_to, *r_newsgroups;
  Obj		*to_obj, *newsgroup;
  char		*orig_in, to_entity[PHRASELEN], to_address[PHRASELEN];
  orig_in = in;
  if (prevslotname && (!streq(prevslotname, slotname))) return(0);
  if (prevslotname || StringHeadEqualAdvance(slotname, in, &in)) {
    r_to = *to;
    r_newsgroups = *newsgroups;
    while (1) {
      in = StringSkipWhitespaceNonNewline(in);
      in = StringReadTo(in, to_entity, PHRASELEN, ',', NEWLINE, TERM);
      if ((!StringIn('@', to_entity)) &&
          TA_EmailIsNewsgroupString(to_entity)) {
        if (!TA_EmailParseNewsgroup1(to_entity, &newsgroup)) goto parsefailure;
        r_newsgroups = ObjListCreate(newsgroup, r_newsgroups);
      } else {
        if (!TA_EmailParseAndResolveEntity(to_entity, ts, dc, to_address,
                                           &to_obj)) {
          goto parsefailure;
        }
        r_to = ObjListCreate(to_obj, r_to);
      }
      if ((*(in-1)) == NEWLINE) break;
    }
    if (r_to) *to = r_to;
    if (r_newsgroups) *newsgroups = r_newsgroups;
    if (r_to || r_newsgroups) {
      *nextp = in;
      return(1);
    }
  }
  return(0);
parsefailure:
  Dbg(DBGGEN, DBGBAD, "TA_EmailParseTo: trouble parsing <%.64s> <%.64s>",
      orig_in, in);
  return(0);
}

/* In-Reply-To: Jim Garnier's message of Mon, 9 Sep 94 16:24:03 EDT
 *   <9109092024.AA10073@dreamland.COM>
 * In-Reply-To: jgarnier (Jim Garnier)
 *        "silly rabbit" (Dec 10,  9:25)
 * In-reply-to: tge99@lc.cma.ca.uk's message of 13 Feb 1995 13:11:54 GMT
 * todo: Extract information here.
 */
Bool TA_EmailParseInReplyTo(char *in, char *slotname, /* RESULTS */
                            char **nextp)
{
  char		*orig_in;
  orig_in = in;
  if (StringHeadEqualAdvance(slotname, in, &in)) {
    if (streq(slotname, "In-reply-to:") ||
        (StringIsCharInLine(in, '<') &&
         StringIsCharInLine(in, '>'))) {
    /* One-line format. */
      in = StringSkipTo(in, NEWLINE, TERM, TERM);
      *nextp = in;
      return(1);
    }
    in = StringSkipTo(in, NEWLINE, TERM, TERM);
    if (StringIsCharInLine(in, DQUOTE) &&
        StringIsCharInLine(in, LPAREN) &&
        StringIsCharInLine(in, RPAREN)) {
    /* Two-line format. */
      in = StringSkipTo(in, NEWLINE, TERM, TERM);
      *nextp = in;
      return(1);
    }
    goto parsefailure;
  }
  return(0);
parsefailure:
  Dbg(DBGGEN, DBGBAD, "TA_EmailParseInReplyTo: trouble parsing <%.64s> <%.64s>",
      orig_in, in);
  return(0);
}

Bool TA_EmailParseArticleNumber1(char *word, /* RESULTS */ long *article_number)
{
  if (!IntIsString(word)) return(0);
  *article_number = atol(word);
  return(*article_number >= 1);
}

Bool TA_EmailParseArticleNumber(char *sent, /* RESULTS */ long *article_number,
                             char **nextp)
{
  char			word[PHRASELEN];
  if (!StringGetWord(word, sent, PHRASELEN, &sent)) return(0);
  if (TA_EmailParseArticleNumber1(word, article_number)) {
    *nextp = sent;
    return(1);
  }
  *article_number = LONGNA;
  return(0);
}

Bool TA_EmailParseNewsgroup1(char *word, /* RESULTS */ Obj **newsgroup)
{
  Obj	*obj;
  if (!TA_EmailIsNewsgroupString(word)) return(0);
  if ((obj = NameToObj(word, OBJ_NO_CREATE)) &&
      ISA(N("Usenet-newsgroup"), obj) &&
      RE(&TsNA, L(N("part-of"), obj, N("Usenet"), E))) {
  /* Existing newsgroup. */
    *newsgroup = obj;
    return(1);
  }
  /* New newsgroup. */
  obj = StringToObj(word, N("Usenet-newsgroup"), 1);
  LearnAssert(L(N("part-of"), obj, N("Usenet"), E), StdDiscourse);
  *newsgroup = obj;
  return(1);
}

Bool TA_EmailParseNewsgroup(char *sent, /* RESULTS */ Obj **newsgroup,
                            char **nextp)
{
  int			len;
  char			word[PHRASELEN];
  *newsgroup = NULL;
  if (!StringGetWord(word, sent, PHRASELEN, &sent)) return(0);
  len = strlen(word);
  if (word[len-1] == ':') word[len-1] = TERM;
  if (TA_EmailParseNewsgroup1(word, newsgroup)) {
    *nextp = sent;
    return(1);
  }
  return(0);
}

/* Newsgroups: alt.history.living,bit.listserv.history,soc.history */
Bool TA_EmailParseNewsgroups(char *in, char *slotname, Ts *ts, Discourse *dc,
                          /* RESULTS */ ObjList **newsgroups, char **nextp)
{
  ObjList	*r;
  Obj		*newsgroup;
  char		*orig_in, newsgroup_s[PHRASELEN];
  orig_in = in;
  if (StringHeadEqualAdvance(slotname, in, &in)) {
    r = *newsgroups;
    while (1) {
      in = StringSkipWhitespaceNonNewline(in);
      in = StringReadTo(in, newsgroup_s, PHRASELEN, ',', NEWLINE, TERM);
      if (!TA_EmailParseNewsgroup1(newsgroup_s, &newsgroup)) {
        goto parsefailure;
      }
      r = ObjListCreate(newsgroup, r);
      if ((*(in-1)) == NEWLINE) break;
    }
    if (r) {
      *newsgroups = r;
      *nextp = in;
      return(1);
    }
  }
  return(0);
parsefailure:
  Dbg(DBGGEN, DBGBAD,
      "TA_EmailParseNewsgroups: trouble parsing <%.64s> <%.64s>",
      orig_in, in);
  return(0);
}

/* Article: 12308 of comp.dcom.telecom */
Bool TA_EmailParseArticle(char *in, /* RESULTS */ long *article_number,
                          Obj **newsgroup, char **nextp)
{
  char		*orig_in;
  orig_in = in;
  if (StringHeadEqualAdvance("Article:", in, &in) ||
      StringHeadEqualAdvance("Article", in, &in)) {
    in = StringSkipWhitespaceNonNewline(in);
    if (!TA_EmailParseArticleNumber(in, article_number, &in)) goto parsefailure;
    if (!StringHeadEqualAdvance("of ", in, &in)) goto parsefailure;
    if (!TA_EmailParseNewsgroup(in, newsgroup, &in)) goto parsefailure;
    *nextp = in;
    return(1);
  }
  return(0);
parsefailure:
  Dbg(DBGGEN, DBGBAD, "TA_EmailParseArticle: trouble parsing <%.64s> <%.64s>",
      orig_in, in);
  *article_number = LONGNA;
  *newsgroup = NULL;
  return(0);
}

Bool TA_EmailParseUnimplementedSlot(char *in, /* RESULTS */ char **nextp)
{
  int	i;
  char	*orig_in, slotname[PHRASELEN];
  i = 0;
  orig_in = in;
  while (*in && TA_EmailIsSlotnameChar(*in)) {
    if (i >= PHRASELEN) return(0);
    slotname[i] = *in;
    in++;
    i++;
  }
  slotname[i] = TERM;
  if (slotname[0]) {
    if (*in == ':' && *(in + 1) == SPACE) {
      Dbg(DBGGEN, DBGBAD, "unimplemented email slot <%s> <%.64s>",
          slotname, orig_in);
      in = StringSkipTo(in, NEWLINE, TERM, TERM);
      *nextp = in;
      return(1);
    }
  }
  return(0);
}

Bool TA_EmailParseHeader1(char *in, Discourse *dc,
                          /* RESULTS */ EmailHeader *emh, char **nextp,
                          char **subj_beginp, char **subj_restp)
{
  int		found;
  char		topentity[PHRASELEN], dummy[LINELEN], received_from[PHRASELEN];
  char		received_by[PHRASELEN], *save_in;
  Ts		query_ts;
  Ts		received_ts;

  dummy[0] = TERM;
  topentity[0] = TERM;
  received_from[0] = TERM;
  received_by[0] = TERM;
  TsSetNa(&received_ts);
  *subj_beginp = NULL;
  *subj_restp = NULL;

  /* Detect email message presence. */
  found = 0;
  if (StringHeadEqualAdvance("--- Internet Message Header Follows ---\n", in,
                             &in)) {
  /* This is provided AFTER the message body by FirstClass. */
    found = 1;
    emh->header_type = N("post-email-header");
  }
  if (TA_EmailParseRmailHeader(in, &in)) found = 1;
  if (TA_EmailParseTop(in, topentity, &emh->receive_ts, &in)) found = 1;
  if (!found) return(0);

  /* Read slots. */
  while (1) {
    /* Email line-based parsing functions must return beginning of next line. */
    in = StringSkipWhitespaceNonNewline(in); /* todoSCORE */
    save_in = in;
    if (TA_EmailParseAngleBracketSlot(in, "Return-Path:",
                                      emh->return_path, &in)) {
      continue;
    }
    if (TA_EmailParseReceived(in, received_from,
                              received_by, &received_ts, &in)) {
      continue;
    }
    if (TA_EmailParseDate(in, "Date:", &emh->send_ts, &in)) continue;
    if (TsIsSpecific(&emh->send_ts)) query_ts = emh->send_ts;
    else if (TsIsSpecific(&emh->receive_ts)) query_ts = emh->receive_ts;
    else if (TsIsSpecific(&received_ts)) query_ts = received_ts;
    else TsSetNa(&query_ts);
    if (TA_EmailParseFrom(in, "From:", &query_ts,
                       emh->resent_from_address[0] ? NULL : topentity,
                       dc, emh->from_address, &emh->from_obj, &in)) {
      continue;
    }
    if (TA_EmailParseFrom(in, ">From:", &query_ts,
                       emh->resent_from_address[0] ? NULL : topentity,
                       dc, emh->from_address, &emh->from_obj, &in)) {
      continue;
    }
    if (TA_EmailParseFrom(in, "Reply-To:", &query_ts, NULL, dc,
                          emh->reply_address, &emh->reply_obj, &in)) {
      continue;
    }
    /* Message-Id: <9411211107.AA11109@dreamland.com>
     * Message-Id: <9411251519.ZM12438@dreamland.com>
     * Message-Id: <9411280924.AA07487@dreamland.com>
     * Message-ID: <1995Feb16.160040.12538@yvax.byu.edu>
     * todo: Could also parse date out of Message-Id, but this doesn't have
     * seconds, time zone, and full year, so it's not as good as the
     * Date: field.
     */
    if (TA_EmailParseAngleObjectSlot(in, "Message-Id:", N("message-ID"),
                                     &emh->message_id, &in)) {
      continue;
    }
    if (TA_EmailParseAngleObjectSlot(in, "Message-ID:", N("message-ID"),
                                     &emh->message_id, &in)) {
      continue;
    }
    if (TA_EmailParseTo(in, "To:", NULL, &query_ts, dc, &emh->to_objs,
                        &emh->newsgroups, &in)) {
      continue;
    }
    if (TA_EmailParseTo(in, "Cc:", NULL, &query_ts, dc, &emh->cc_objs,
                        &emh->newsgroups, &in)) {
      continue;
    }
    if (TA_EmailParseStringSlot(in, "Subject:", PHRASELEN, emh->subject, &in)) {
      *subj_beginp = save_in + 8;
      *subj_restp = in;
      continue;
    }
    if (TA_EmailParseInReplyTo(in, "In-Reply-To:", &in)) continue;
    if (TA_EmailParseInReplyTo(in, "In-reply-to:", &in)) continue;
    if (TA_EmailParseStringSlot(in, "Mime-Version:", PHRASELEN,
                                emh->mime_version, &in)) {
      continue;
    }
    /* Content-Type: text/plain; charset=us-ascii */
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "Content-Type:", LINELEN, dummy, &in)) {
      continue;
    }
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "Content-Length:", LINELEN, dummy, &in)) {
      continue;
    }
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "Content-length:", LINELEN, dummy, &in)) {
      continue;
    }
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "Status:", LINELEN, dummy, &in)) continue;
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "X-Mailer:", LINELEN, dummy, &in)) continue;
    /* References: <9412010704.AA06778@dreamland.com> */
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "References:", LINELEN, dummy, &in)) {
      continue;
    }

    /* Resent- slots */
    dummy[0] = TERM;
    if (TA_EmailParseAngleBracketSlot(in, "Resent-Message-Id:", dummy, &in)) {
      continue;
    }
    if (TA_EmailParseFrom(in, "Resent-From:", &query_ts, topentity, dc,
                          emh->resent_from_address, &emh->resent_from_obj,
                          &in)) {
      continue;
    }
    if (TA_EmailParseDate(in, "Resent-Date:", &emh->resent_ts, &in)) continue;
    if (TA_EmailParseTo(in, "Resent-To:", NULL, &query_ts, dc,
                        &emh->resent_to_objs, &emh->newsgroups, &in)) {
      continue;
    }
    if (TA_EmailParseTo(in, "Resent-Cc:", NULL, &query_ts, dc,
                        &emh->resent_cc_objs, &emh->newsgroups, &in)) {
      continue;
    }

    /* Usenet slots */
    if (TA_EmailParseNewsgroups(in, "Newsgroups:", &query_ts, dc,
                                &emh->newsgroups, &in)) {
      continue;
    }
    if (TA_EmailParseArticle(in, &emh->article_number, &emh->newsgroup, &in)) {
      continue;
    }
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "Path:", LINELEN, dummy, &in)) continue;
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "Sender:", LINELEN, dummy, &in)) continue;
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "Organization:", LINELEN, dummy, &in)) {
      continue;
    }
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "Lines:", LINELEN, dummy, &in)) continue;
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "Distribution:", LINELEN, dummy, &in)) {
      continue;
    }
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "Approved:", LINELEN, dummy, &in)) continue;
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "Keywords:", LINELEN, dummy, &in)) continue;
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "Summary:", LINELEN, dummy, &in)) continue;
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "X-Submissions-To:", LINELEN, dummy, &in)) {
      continue;
    }
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "X-Administrivia-To:", LINELEN, dummy,
                                &in)) {
      continue;
    }
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "X-Telecom-Digest:", LINELEN, dummy, &in)) {
      continue;
    }
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "X-TELECOM-Digest:", LINELEN, dummy, &in)) {
      continue;
    }
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "Xref:", LINELEN, dummy, &in)) continue;
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "X-Newsreader:", LINELEN, dummy, &in)) {
      continue;
    }
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "X-Authenticated:", LINELEN, dummy, &in)) {
      continue;
    }
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "X-Posted-From:", LINELEN, dummy, &in)) {
      continue;
    }
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "X-Sender:", LINELEN, dummy, &in)) continue;
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "X-Ident-Sender:", LINELEN, dummy, &in)) {
      continue;
    }
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "Content-Transfer-Encoding:", LINELEN,
                                dummy, &in)) {
      continue;
    }
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "X-Url:", LINELEN, dummy, &in)) continue;
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "X-URL:", LINELEN, dummy, &in)) continue;
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "X-Client-Port:", LINELEN, dummy, &in)) {
      continue;
    }
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "X-Nntp-Posting-Host:", LINELEN, dummy,
                                &in)) {
      continue;
    }
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "NNTP-Posting-Host:", LINELEN, dummy,
                                &in)) {
      continue;
    }
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "Nntp-Posting-Host:", LINELEN, dummy,
                                &in)) {
      continue;
    }
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "X-Nntp-Posting-User:", LINELEN, dummy,
                                &in)) {
      continue;
    }
    dummy[0] = TERM;
    if (TA_EmailParseStringSlot(in, "X-UserAgent:", LINELEN, dummy, &in)) {
      continue;
    }
    dummy[0] = TERM;

    if (TA_EmailParseUnimplementedSlot(in, &in)) continue;
    break;
  }
  EmailHeaderAdjust(emh);
  *nextp = in;
  return(1);
}

/* Examples:
 *
 *          Vendredi 17 Février 1995 9:21:53
 *          soc.culture.french Item
 *  Exp:            Hans Gerdemann,hg@sas.ppil.tuebingen.de,Usenet
 *  Titre:          Re: Anglais Adieu!
 *  Dest:           soc.culture.french
 *                  soc.culture.europe
 *
 *          Mercredi 28 Décembre 1994 22:40:48
 *          Message
 *  Exp:            Jim Garnier
 *  Titre:          France Telephony Update
 *  Dest:           telefun@ecse.wun.edu,Internet
 *  Cc:             Jim Garnier
 *
 */
Bool TA_EmailParseHeaderFirstClass(char *in, Discourse *dc,
                                   /* RESULTS */ EmailHeader *emh,
                                   char **nextp)
{
  char	word[PHRASELEN], *orig_in, *prev_slot;
  orig_in = in;

  /* Read date. */
  if (!StringHeadEqualAdvance("          ", in, &in)) return(0);
  if (!StringIn(*in, "LMJVSD")) return(0);
  if (!TA_TimeUnixEtc(in, &emh->send_ts, &in)) return(0);

  /* Read Message/Item line. */
  /* ^          soc.culture.french Item */
  if (!StringHeadEqualAdvance("\n          ", in, &in)) goto parsefailure;
  /*           ^soc.culture.french Item */
  if (!StringHeadEqualAdvance("Message\n", in, &in)) {
    if (!StringGetWord(word, in, PHRASELEN, &in)) goto parsefailure;
    /*           soc.culture.french ^Item */
    if (!StringHeadEqualAdvance("Item\n", in, &in)) goto parsefailure;
    if (!TA_EmailParseNewsgroup1(word, &emh->newsgroup)) goto parsefailure;
  }

  emh->header_type = N("FirstClass-email-header");

  /* Read slots. */
  prev_slot = NULL;
  while (1) {
    if (!StringHeadEqual("                  ", in)) prev_slot = NULL;
    in = StringSkipWhitespaceNonNewline(in); /* necessary */
    if (TA_EmailParseFrom(in, "Exp:", &emh->send_ts, NULL, dc,
                          emh->from_address, &emh->from_obj, &in)) {
      prev_slot = NULL; continue;
    }
    if (TA_EmailParseStringSlot(in, "Titre:", PHRASELEN, emh->subject, &in)) {
      prev_slot = NULL; continue;
    }
    if (TA_EmailParseTo(in, "Dest:", prev_slot, &emh->send_ts, dc,
                        &emh->to_objs, &emh->newsgroups, &in)) {
      prev_slot = "Dest:"; continue;
    }
    if (TA_EmailParseTo(in, "Cc:", prev_slot, &emh->send_ts, dc, &emh->cc_objs,
                        &emh->newsgroups, &in)) {
      prev_slot = "Cc:"; continue;
    }
    break;
  }
  EmailHeaderAdjust(emh);
  *nextp = in;
  return(1);

parsefailure:
  Dbg(DBGGEN, DBGBAD,
      "TA_EmailParseHeaderFirstClass: trouble parsing <%.64s> <%.64s>",
      orig_in, in);
  return(0);
}

Bool TA_EmailParseHeader(char *in, Discourse *dc,
                         /* RESULTS */ Channel *ch, char **nextp)
{
  char		*orig_in, *subj_begin, *subj_rest;
  PNode		*pn;
  EmailHeader	*emh;
  orig_in = in;
  emh = EmailHeaderCreate();
  subj_begin = subj_rest = NULL;
  if (TA_EmailParseHeader1(in, dc, emh, &in, &subj_begin, &subj_rest) ||
      TA_EmailParseHeaderFirstClass(in, dc, emh, &in)) {
    pn = ChannelAddPNode(ch, PNTYPE_EMAILHEADER, 1.0, emh, NULL, orig_in, in);
    if (pn && subj_begin && subj_rest) {
      pn->lowerb_subj = subj_begin - (char *)ch->buf;
      pn->upperb_subj = (subj_rest - (char *)ch->buf) - 1;
    }
    *nextp = in;
    return(1);
  }
  EmailHeaderFree(emh);
  return(0);
}

void TA_EmailProcessPostHeaders(Channel *ch)
{
  PNode			*pn;
  EmailHeader	*prev_emh;
  prev_emh = NULL;
  /* Note this relies on fifo ordering of ch->pnf corresponding to
   * text order.
   */
  for (pn = ch->pnf->first; pn; pn = pn->next) {
    if (pn->type == PNTYPE_EMAILHEADER) {
      if (pn->u.emh->header_type == N("post-email-header")) {
        if (prev_emh && prev_emh->header_type == N("FirstClass-email-header")) {
          EmailHeaderMergeInto(prev_emh, pn->u.emh);
        } else {
          Dbg(DBGGEN, DBGBAD,
              "TA_EmailProcessPostHeaders: no corresponding FC header");
          Stop();
        }
        prev_emh = NULL;
      } else prev_emh = pn->u.emh;
    }
  }
}

/*
 * >> I wonder if anyone could post a summary of all the major telephone
 * >> companies in the US besides the RBOCs.
 * > So do I.
 * >-- End of excerpt from Jim Garnier
 */
Bool TA_EmailParseQuotationLevel(char *in, Discourse *dc,
                                 /* RESULTS */ int *attr1a, char **nextp)
{
  char	address[PHRASELEN], entity_string[PHRASELEN];
  int	level, found;
  Obj	*entity;
  level = 0;
  while (TA_EmailIsQuotationChar(*in)) {
    level++;
    in++;
  }
  if (level >= 5) {
    level -= 5;
    *attr1a = 1;
  } else *attr1a = 0;
  if (StringHeadEqualAdvance("-- End of excerpt from ", in, &in)) {
    in = StringSkipWhitespaceNonNewline(in);
    in = StringReadTo1(in, entity_string, PHRASELEN, NEWLINE, &found);
    if (found) {
      if (TA_EmailParseAndResolveEntity(entity_string, &TsNA, dc, address,
                                        &entity)) {
        if (!ObjListIn(entity, DCSPEAKERS(dc))) {
          Dbg(DBGGEN, DBGDETAIL,
              "End of excerpt disagreement: <%s>", M(entity));
        }
      }
    }
    DiscourseDeicticStackPop(dc);
      /* todo: This should not be done here? */
      /* Pop isn't critical because >>> always specify level. */
    *nextp = in;
    return(1);
  }
  if (DiscourseDeicticStackLevelEmpty(dc, level)) {
  /* An unattributed quotation is the flip of the current channel. */
    DiscourseDeicticStackPush(dc, DCCLASS(dc), DCLISTENERS(dc), DCSPEAKERS(dc),
                              DCNOW(dc), DCREALTIME(dc));
      /* todo: This should not be done here? */
  } else {
    DiscourseSetDeicticStackLevel(dc, level);
      /* todo: This should not be done here? */
  }
  if (*in == SPACE) in++;
  *nextp = in;
  return(1);
}

/* (>>>>>) On Fri, 11 Dec 94 09:38:16 +0100, jgarnier (J. Garnier)
 * said:
 * On Mar 8, 11:53am, Jim Garnier wrote:
 */
Bool TA_EmailParseAttribution1(char *in, Discourse *dc, /* RESULTS */ Ts *ts,
                               Obj **message_id, char *address, Obj **entity,
                               char **nextp)
{
  char		*orig_in, entity_string[PHRASELEN];
  orig_in = in;
  if (StringHeadEqualAdvance("On ", in, &in)) {
    if ((!TA_TimeUnixEtc(in, ts, &in)) &&
        (!TA_TimeMini(in, ts, &in))) {
      goto parsefailure;
    }
    if (!StringHeadEqualAdvance(", ", in, &in)) goto parsefailure;
    in = StringSkipWhitespaceNonNewline(in);
    if ((!StringReadUntilString(in, "said:", PHRASELEN, entity_string, &in)) &&
        (!StringReadUntilString(in, "wrote:", PHRASELEN, entity_string, &in))) {
      goto parsefailure;
    }
    if (!TA_EmailParseAndResolveEntity(entity_string, ts, dc, address,
                                       entity)) {
      goto parsefailure;
    }
    if (!StringSkipChar(in, NEWLINE, &in)) goto parsefailure;
    *nextp = in;
    return(1);
  }
  return(0);
parsefailure:
  Dbg(DBGGEN, DBGBAD,
      "TA_EmailParseAttribution1: trouble parsing <%.64s> <%.64s>",
      orig_in, in);
  TsSetNa(ts);
  *message_id = NULL;
  address[0] = TERM;
  *entity = NULL;
  return(0);
}

/* In article <880227113646.2480a904@Csa5.ZGR.Gov> super@CSA5.ZGR.GOV
 * (David Helm) writes:
 */
Bool TA_EmailParseAttribution2(char *in, Discourse *dc, /* RESULTS */ Ts *ts,
                               Obj **message_id, char *address, Obj **entity,
                               char **nextp)
{
  int		found;
  char		*orig_in, entity_string[PHRASELEN], msgid[PHRASELEN];
  orig_in = in;
  if (StringHeadEqualAdvance("In article <", in, &in)) {
    in = StringReadTo1(in, msgid, PHRASELEN, '>', &found);
    if (found) *message_id = StringToObj(msgid, N("message-ID"), 0);
    else *message_id = NULL;
    in = StringSkipWhitespaceNonNewline(in);
    if (!StringReadUntilString(in, "writes:", PHRASELEN, entity_string, &in)) {
      goto parsefailure;
    }
    if (!StringSkipChar(in, NEWLINE, &in)) goto parsefailure;
    /* todo: Could parse ts out of message id. */
    TsSetNa(ts);
    if (!TA_EmailParseAndResolveEntity(entity_string, ts, dc, address,
                                       entity)) {
      goto parsefailure;
    }
    *nextp = in;
    return(1);
  }
  return(0);
parsefailure:
  Dbg(DBGGEN, DBGBAD,
      "TA_EmailParseAttribution2: trouble parsing <%.64s> <%.64s>",
      orig_in, in);
  TsSetNa(ts);
  *message_id = NULL;
  address[0] = TERM;
  *entity = NULL;
  return(0);
}

/* wmadison@puler.lokuh.edu (Welc Madison) writes:
 * Casey Leedom <casey@gauss.llnl.gov> writes:
 */
Bool TA_EmailParseAttributionLast(char *in, Discourse *dc, /* RESULTS */ Ts *ts,
                                  Obj **message_id, char *address, Obj **entity,
                                  char **nextp)
{
  char		*orig_in, entity_string[PHRASELEN];
  orig_in = in;
  if (IsWhitespace(in[0])) return(0);
  if (StringReadUntilString(in, "writes:", PHRASELEN, entity_string, &in)) {
    if (StringIn(NEWLINE, entity_string)) return(0);
    if (!StringSkipChar(in, NEWLINE, &in)) goto parsefailure;
    TsSetNa(ts);
    if (!TA_EmailParseAndResolveEntity(entity_string, ts, dc, address,
                                       entity)) {
      goto parsefailure;
    }
    *nextp = in;
    return(1);
  }
  return(0);
parsefailure:
  Dbg(DBGGEN, DBGBAD,
      "TA_EmailParseAttribution2: trouble parsing <%.64s> <%.64s>",
      orig_in, in);
  TsSetNa(ts);
  *message_id = NULL;
  address[0] = TERM;
  *entity = NULL;
  return(0);
}

/* Forms not yet handled:
 * In a message dated September 2, 1990, Remy Van Houten writes:
 * Ad hoc such as: "Begin quote...."/"End quote...."
 * Indenting.
 */
Bool TA_EmailParseAttributionA(char *in, Discourse *dc, /* RESULTS */ Ts *ts,
                               Obj **message_id, char *address, Obj **entity,
                               char **nextp)
{
  int	attr1a;
  TA_EmailParseQuotationLevel(in, dc, &attr1a, &in);
  /* todoSCORE: Not using attr1a is a relaxation. */
  if (TA_EmailParseAttribution1(in, dc, ts, message_id, address, entity,
                                nextp)) {
    return(1);
  }
  if (TA_EmailParseAttribution2(in, dc, ts, message_id, address, entity,
                                nextp)) {
    return(1);
  }
  if (TA_EmailParseAttributionLast(in, dc, ts, message_id, address, entity,
                                   nextp)) {
    return(1);
  }
  return(0);
}

/* Must be called at the beginning of each line.
 * todo: Parse attributions in TV script, direct and indirect quotations.
 */
Bool TA_EmailParseAttribution(char *in, Discourse *dc,
                              /* RESULTS */ Channel *ch, char **nextp)
{
  char			*orig_in, address[PHRASELEN];
  Obj			*speaker, *message_id;
  Ts			ts;
  orig_in = in;
  if (TA_EmailParseAttributionA(in, dc, &ts, &message_id, address, &speaker,
                                &in)) {
    ChannelAddPNode(ch, PNTYPE_ATTRIBUTION, 1.0,
                    AttributionCreate(speaker, &ts, NULL),
                    NULL, orig_in, in);
    *nextp = in;
    return(1);
  }
  return(0);
}

/* todoDATABASE: Store these as lexical items. */
Bool TA_Communicon(char *in, Discourse *dc,
                    /* RESULTS */ Channel *ch, char **nextp)
{
  char		*orig_in;
  Obj		*con, *speaker, *listener;
  orig_in = in;
  con = NULL;
  if (DCSPEAKERS(dc)) {
    speaker = DCSPEAKERS(dc)->obj;
  } else {
    speaker = ObjWild;
  }
  if (StringHeadEqualAdvance(":-)", in, &in)) {
    con = L(N("smile"), speaker, E);
  } else if (StringHeadEqualAdvance("8-)", in, &in)) {
    con = L(N("smile"), speaker, E); /* todo: Speaker wears glasses. */
  } else if (StringHeadEqualAdvance(";-)", in, &in)) {
    con = L(N("smile"), speaker, E);
  } else if (StringHeadEqualAdvance(":-(", in, &in)) {
    con = L(N("sadness"), speaker, D(.5), E);
  } else if (StringHeadEqualAdvance(":-<", in, &in)) {
    con = L(N("sadness"), speaker, D(.5), E);
  } else if (StringHeadEqualAdvance(":-|", in, &in)) {
    con = L(N("emotionless"), speaker, E);
  } else if (StringHeadEqualAdvance(":->", in, &in)) {
    con = N("sarcastic");
  } else if (StringHeadEqualAdvance(":*)", in, &in)) {
    con = L(N("drunkenness"), speaker, D(.9), E);
  } else if (StringHeadEqualAdvance(":-o", in, &in)) {
    con = L(N("surprise"), speaker, D(.9), E);
  } else if (StringHeadEqualAdvance(":-O", in, &in) ||
           StringHeadEqualAdvance(":-0", in, &in)) {
    con = L(N("shock"), speaker, D(.9), E);
  } else if (StringHeadEqualAdvance("<g>", in, &in)) {
    con = L(N("smile"), speaker, E);
  } else if (StringHeadEqualAdvance("(@@)", in, &in)) {
    con = L(N("sane"), listener, D(-.9), E);
  } else if (StringHeadEqualAdvance(":)", in, &in)) {
    con = L(N("smile"), speaker, E);
  } else if (StringHeadEqualAdvance(":(", in, &in)) {
    con = L(N("frown"), speaker, E);
  } else if (StringHeadEqualAdvance(";>", in, &in)) {
    con = N("mischievous");
  } else if (StringHeadEqualAdvance(":]", in, &in)) {
    con = L(N("smile"), speaker, E);
  } else if (StringHeadEqualAdvance(";)", in, &in)) {
    con = L(N("wink"), speaker, E);
  } else if (StringHeadEqualAdvance("-o-", in, &in)) {
    con = N("over");
  } else if (StringHeadEqualAdvance("-oo-", in, &in)) {
    con = N("over-and-out");
  }

  if (con) {
    ChannelAddPNode(ch, PNTYPE_COMMUNICON, 1.0,
                    ObjListCreate(con, NULL),
                    NULL, orig_in, in);
    *nextp = in;
    return(1);
  }
  
  return(0);
}

/* MRBEGIN */
Bool TA_StarRating(char *in, Discourse *dc,
                   /* RESULTS */ Channel *ch, char **nextp)
{
  Float	numer, denom;
  char	*orig_in;
  Obj	*con;
  numer = 0.0;
  denom = 4.0;	/* Assume 4.0 as default. */

  orig_in = in;
  /* Sense presence of rating. */
  if (StringHeadEqualAdvance("RATING (0 TO ****):", in, &in)) {
    denom = 4.0;
    in = StringSkipWhitespace(in);
  } else if (StringHeadEqualAdvance("RATING:", in, &in)) {
    in = StringSkipWhitespace(in);
  } else if (StringHeadEqualAdvance("Alternative Scale:", in, &in)) {
    in = StringSkipWhitespace(in);
  } else if (StringHeadEqualAdvance("1/2", in, &in)) {
    numer = 0.5;
    goto post;
  } else if (*in != '*') {
    /* Rating not present. */
    return(0);
  }

  /* Parse rating numerator. */
  if (*in == '0') {
    numer = 0.0;
    in++;
  } else if (*in == '*' || *in == '1') {
    while (*in == '*' || *in == '1') {
      if (*in == '*') {
        numer += 1.0;
      } else {
        in++;
        if (!StringHeadEqualAdvance("/2", in, &in)) return(0);
        numer += 0.5;
        break;
      }
      in++;
      if (*in == ' ' && *(in+1) == '1') {
      /* "* 1/2" */
        in++;
      }
    }
  } else {
    return(0);
  }

post:
  /* Parse optional rating denominator. */
  in = StringSkipWhitespace(in);
  if (StringHeadEqualAdvance("(out of four)", in, &in)) {
    denom = 4.0;
  } else if (StringHeadEqualAdvance("(out of ****)", in, &in)) {
    denom = 4.0;
  } else if (StringHeadEqualAdvance("out of ****", in, &in)) {
    denom = 4.0;
  }
  /* todo: Parse other denominators. */

  if (numer > denom) return(0);
  con = L(N("good"), ObjNA, D(Weight01toNeg1Pos1(numer/denom)), E);
  ChannelAddPNode(ch, PNTYPE_COMMUNICON, 1.0,
                  ObjListCreate(con, NULL),
                  NULL, orig_in, in);
  *nextp = in;
  return(1);
}
/* MREND */

/* End of file. */
