/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
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
#include "semgen1.h"
#include "semgen2.h"
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

/* <model> is a specific Obj: N("1987-Maser-Biturbo-Spyder")
 * There may be other Objs with the same name which are a product of <brand>.
 * <this_brand>: <model> is a product of <this_brand>
 * <another_brand>: Another brand that <model> is a product of.
 */
void ModelOfThisOrAnotherBrand(Obj *brand, Obj *model, /* RESULTS */
                               int *this_brand, Obj **another_brand)
{
  ObjList	*objs, *p;
  *this_brand = 0;
  *another_brand = NULL;
  objs = RN(&TsNA, L(N("product-of"), ObjWild, model, E), 2);
  for (p = objs; p; p = p->next) {
    if (I(p->obj, 1) == brand) {
      *this_brand = 1;
    } else {
      *another_brand = I(p->obj, 1);
    }
  }
  ObjListFree(objs);
}

/* In db:
 *   [isa car82 Convertible]
 *   [isa car82 Fiat-124-Spider]
 *   [isa car23 Alfa-Romeo-Spider]
 *   [product-of Fiat car82]
 *   [white car82]
 *   1975:[create ? car82]
 * Parsed: "white 1975 Fiat 124 Spider"
 *   brand: N("Fiat")
 *   year: 1975
 *   isas:  {N("Fiat-124-Spider") N("Alfa-Romeo-Spyder")}
 *          {N("Convertible")}
 *   attributes: N("white")
 *   new_isas: NULL
 */
ObjList *TA_ProductFindExisting(Obj *brand, ObjListList *isas,
                                ObjListList *attributes, Ts *ts_created)
{
  ObjListList	*p1, *p2;
  ObjList	*r;
  Intension	itn;
  r = NULL;
  /* This gets off the ground by finding an <isa> which is a product of <brand>.
   * Then we consider all descendants of that class to find one satisfying the
   * given properties.
   */
  for (p1 = isas; p1; p1 = p1->next) {
    /* p1->u.objs: {N("Fiat-124-Spider") N("Alfa-Romeo-Spyder")} */
    for (p2 = p1->u.objs; p2; p2 = p2->next) {
      if (RAD(&TsNA, L(N("product-of"), brand, p2->obj, E), 2, 2) ||
             /* desk-stand + [product-of WE-number-3-desk-stand WE] */
          RN(&TsNA, L(N("product-of"), brand, p2->obj, E), 2)) {
             /* WE-500 + [product-of WE-phone WE] */
       IntensionInit(&itn);
       itn.class = p2->obj;
       itn.isas = isas;
       itn.skip_this_isa = p1->u.objs;
       itn.attributes = attributes;
       itn.ts_created = ts_created;
       r = DbFindIntension(&itn, r);
      }
    }
  }
  return(ObjListEnsureProductOfDest(r, brand));	/* todo: do this earlier? */
}

ObjList *TA_ProductLearnNew(Obj *brand, ObjListList *isas,
                            ObjListList *attributes, ObjList *new_isas,
                            int year, Ts *ts_created, char *input_text,
                            Discourse *dc)
{
  ObjListList	*p;
  ObjList		*q;
  Obj			*class, *obj;
  char			le[PHRASELEN];

  le[0] = TERM;
  for (p = attributes; p; p = p->next) {
    if (p->u.objs == NULL) continue;
    /* Select first possibility at random. */
    GenConceptString(p->u.objs->obj, N("empty-article"), F_ADJECTIVE, F_NULL,
                     DC(dc).lang, F_NULL, F_NULL, F_NULL, PHRASELEN, 1, 0, dc,
                     StringEndOf(le));
    StringAppendChar(le, PHRASELEN, SPACE);
  }
  /* le = "red" */
  if (year > 0) sprintf(StringEndOf(le), "%d ", year);
  /* le = "red 1975" */
  /* todo: The brand may already contained in the isas. For now I am
   * using ¹ but it might be better (?) to add yet another flag to lexical
   * entries (= canonical TA_Product entry).
   */
  GenConceptString(brand, N("empty-article"), F_NOUN, F_NULL, DC(dc).lang,
                   F_NULL, F_NULL, F_NULL, PHRASELEN, 1, 0, dc,
                   StringEndOf(le));
  StringAppendChar(le, PHRASELEN, SPACE);
  /* le = "red 1975 Alfa Romeo" */
  class = NULL;
  /* New model numbers. */
  for (q = new_isas; q; q = q->next) {
    if (class == NULL) class = q->obj;
    StringCat(le, ObjToString(q->obj), PHRASELEN);
    StringAppendChar(le, PHRASELEN, SPACE);
  }
  /* Model numbers or generic terms. */
  for (p = isas; p; p = p->next) {
    if (p->u.objs == NULL) continue;
    if (class == NULL) class = p->u.objs->obj;
    /* Select first possibility at random. */
    GenConceptString(p->u.objs->obj, N("empty-article"), F_NOUN, F_NULL,
                     DC(dc).lang, F_NULL, F_NULL, F_NULL, PHRASELEN, 1, 0, dc,
                     StringEndOf(le));
    StringAppendChar(le, PHRASELEN, SPACE);
  }
  StringElimTrailingBlanks(le);
  if (class == NULL) return(NULL);
  /* Learn the object.
   * class: N("Alfa-Romeo-Spider") le: "red 1975 Alfa Romeo Spider"
   */
  if (NULL == (obj = LearnObjText(class, le, F_NULL, F_SINGULAR, F_NOUN,
                                  DC(dc).lang, 0, OBJ_CREATE_A, input_text,
                                  dc))) {
    return(NULL);
  }
  /* Learn ISAs. */
  for (p = isas; p; p = p->next) {
    if (p->u.objs == NULL || p->u.objs->obj == class) continue;
    LearnObjClass(obj, NULL, p->u.objs->obj);
  }
  for (q = new_isas; q; q = q->next) {
    if (q->obj == class) continue;
    LearnObjClass(obj, NULL, q->obj);
  }
  /* Learn attributes. */
  for (p = attributes; p; p = p->next) {
    if (p->u.objs == NULL) continue;
    LearnAssert(L(p->u.objs->obj, obj, E), dc);
  }
  if (ts_created) {
    LearnAssertTs(ts_created, L(N("create"), brand, obj, E), dc);
  }
  LearnAssert(L(N("product-of"), brand, obj, E), dc);
  return(ObjListCreate(obj, NULL));
}

/* s:     "500"
 * class: N("electronic-device")
 * isas:
 *   Spyder
 *   Trendline
 *   Centris
 *   Centris 650
 *   FM2
 *   desk stand
 *   telephone
 *   convertible
 * attributes:
 *   red, green, blue
 * year:
 *   1978
 */
Bool TA_ProductElement1(Obj *brand, char *in, char *in_base, Obj *class,
                        int numwords, int is_fwd, Discourse *dc,
                        /* RESULTS */ ObjList **isas, ObjList **attributes,
                        ObjList **new_isas, int *year, char *punc,
                        char **nextp)
{
  int			ok, found, freeme, this_brand1, this_brand, phraselen;
  char			phrase[PHRASELEN];
  Obj			*another_brand1, *another_brand;
  IndexEntry	*ie, *orig_ie;
  LexEntryToObj	*leo;
  *isas = NULL;
  *attributes = NULL;
  if (is_fwd) {
    ok = StringGetNWords_LeNonwhite(phrase, punc, in, DWORDLEN, numwords, &in);
  } else {
    punc[0] = TERM;
    /* todo: need to add punc to below too. cf "And a Fiat 9?
     * A black WE 1020AL?"
     */
    ok = StringGetNWords_LeN_Back(phrase, in, in_base, PHRASELEN, numwords,
                                  &in);
  }
  if (ok) {
    StringElims(phrase, LE_NONWHITESPACE, NULL);
      /* todo: Inelegant. To remove '.'. */
    if (numwords == 1 && is_fwd == 0 && *year == -1 &&
        TA_TimeYear3(phrase, year)) {
    /* "78" "'78" "1978" recognized as year going backward. */
      goto success;
    }
    if (numwords == 1 && is_fwd == 1 && *year == -1 &&
        (!streq(phrase, "1800")) && (!streq(phrase, "2000"))
             /* todoSCORE: 1800 and 2000 often model numbers. */
        && TA_TimeYear4(phrase, year)) {
    /* "1978" recognized as year going forward. */
      goto success;
    }
    found = this_brand = 0;
    another_brand = NULL;
    phraselen = strlen(phrase);
    for (ie = orig_ie = LexEntryFindPhrase(DC(dc).ht, phrase, INTPOSINF,
                                           0, 0, &freeme);
        ie; ie = ie->next) {
      for (leo = ie->lexentry->leo; leo; leo = leo->next) {
        if (is_fwd &&
            (N("model-number-of") == leo->obj ||
             N("brand-name-of") == leo->obj)) {
        /* Ignore (but accept) words such as "series" "model". */
          found = 1;
        } else if (is_fwd == 0 && phraselen > 1 &&
                   (ISA(N("color"), leo->obj) ||
                    ISA(N("chemical"), leo->obj) ||
                    ISA(N("directional-pattern"), leo->obj))) {
        /* todo */
          found = 1;
          if (!ObjListIn(leo->obj, *attributes)) {
            *attributes = ObjListCreate(leo->obj, *attributes);
          }
        } else if (is_fwd && ISA(class, leo->obj)) {
        /* Accept isa (trademark or descriptive term). */
          ModelOfThisOrAnotherBrand(brand, leo->obj, &this_brand1,
                                    &another_brand1);
          if (this_brand1) this_brand = 1;
          if (another_brand1) another_brand = another_brand1;
          if (!another_brand1) {
            found = 1;
            if (!ObjListIn(leo->obj, *isas)) {
              *isas = ObjListCreate(leo->obj, *isas);
            }
          }
        }
      }
    }
    if (another_brand && !this_brand) {
    /* another_brand: N("ITT")
     * this_brand:    N("WE")
     * phrase:        "Trimline"
     * <Trimline> applied generically to <ITT> product, is <WE> trademark
     * (Possible actions: Add Trimline as an ITT product OR
     *                    Remove Trimline from WE and add Trimline as a
     *                    generic term.)
     * 19951109120855: It seems that in this case, the product isn't
     * recognized. cf Fiat Spyder without Spyder in lexicon under
     * Fiat-Spider.
     */
      Dbg(DBGGEN, DBGBAD,
          "warning: <%s> applied generically to <%s> product, is TM of <%s>",
          phrase, M(brand), M(another_brand));
    }
    if (freeme) IndexEntryFree(orig_ie);
    if (found)  goto success;
    /* Attempt new model number such as "850", "2000", "2000S", "RX", "GT" */
    if (numwords == 1 && is_fwd && /* todoSCORE */
        StringIsDigitOrUpper(phrase) && strlen(phrase) < 6 &&
        (!streq(phrase, "FS")) && /* for sale */
        (!streq(phrase, "WTB")) && /* wanted to buy */
        (!StringTailEq(phrase, "K"))) {
    /* K = for cars, mileage; for computers, memory. */
      *new_isas = ObjListCreate(StringToObj(phrase, class, 1), *new_isas);
      goto success;
    }
  }
  return(0);

success:
  *nextp = in;
  return(1);
}

Bool TA_ProductElement(Obj *brand, char *in, char *in_base, Obj *class,
                       int is_fwd, Discourse *dc, /* RESULTS */ ObjList **isas,
                       ObjList **attributes, ObjList **new_isas, int *year,
                       char *punc, char **nextp)
{
  if (TA_ProductElement1(brand, in, in_base, class, 2, is_fwd, dc, isas,
                         attributes, new_isas, year, punc, nextp)) {
    return(1);
  }
  if (TA_ProductElement1(brand, in, in_base, class, 1, is_fwd, dc, isas,
                         attributes, new_isas, year, punc, nextp)) {
    return(1);
  }
  return(0);
}

/* A product name is built out of:
 *   brand (company)
 *   model
 *   color
 *   year
 *   product class (telephone, desk stand, multiline phone, turbo, SLR,
 *                  workstation)
 *
 * Examples:
 *   Western Electric telephone
 *   Western Electric 500 set
 *   WE 302
 *   Western 51AL desk stand
 *   chrome Nikon FM2
 *   Apple Macintosh Centris 650
 *   Sun workstation
 *   Sun Sparcstation 2
 *   Adobe Type Library
 *   Microsoft Word
 *   ElectroVoice 635A microphone
 *   Levi's 501 jeans
 *   WE multiline desk phone
 *   1978 Fiat Spider
 *   Fiat Spider 2000
 *   '78 Fiat Spider
 *   red 1978 Fiat Spyder convertible
 *   69 Fiat 124 Spider
 *   71 Fiat 850 Sport Coupe
 *   75 Alfa Alfetta GT
 *   67 Alfa GTV
 *   67 Alfa Romeo GTV
 *   62 Alfa Giulietta Spider
 *
 * This is triggered by brand (company) name.
 *
 * See the procedure for adding products to the database described in
 * the book.
 *
 * todo: Parse common models without the brand name: "a couple of Trimlines",
 * "a 1975 Alfetta GT".
 */
Bool TA_Product(char *in, char *in_base, Discourse *dc,
                /* RESULTS */ Channel *ch, char **nextp)
{
  char			*start_in, *p_bwd, *p_fwd, punc[PUNCLEN];
  int			year, cnt;
  Ts			ts1, *ts;
  Obj			*brand, *product;
  ObjListList		*isas, *attributes;
  ObjList		*new_isas, *isas1, *attributes1, *r;
  start_in = in;
  if ((brand = TA_FindPhraseForward(in, N("company"), 2, dc, &in)) ||
      (brand = TA_FindPhraseForward(in, N("company"), 1, dc, &in))) {
    if ((product = DbGetRelationValue1(&TsNA, NULL, N("manufacturer-of"),
                                       brand, NULL))) {
    /* todo: Allow several product classes. This would allow us to be more
     * specific, which in turn reduces spurious isas (such as
     * phone-rotary-dial).
     */
      year = -1;
      isas = attributes = NULL;
      new_isas = NULL;
      cnt = 0;
      p_bwd = start_in-1;
      while (TA_ProductElement(brand, p_bwd, in_base, product, 0, dc, &isas1,
                               &attributes1, &new_isas, &year, punc, &p_bwd)) {
        cnt++;
        if (isas1) {
          isas = ObjListListCreate(isas1, isas);
        }
        if (attributes1) {
          attributes = ObjListListCreate(attributes1, attributes);
        }
        if (StringAnyIn(",!?", punc)) break;
      }
      p_fwd = in;
      while (TA_ProductElement(brand, p_fwd, in_base, product, 1, dc, &isas1,
                               &attributes1, &new_isas, &year, punc, &p_fwd)) {
        cnt++;
        if (isas1) {
          isas = ObjListListCreate(isas1, isas);
        }
        if (attributes1) {
          attributes = ObjListListCreate(attributes1, attributes);
        }
        if (StringAnyIn(",!?", punc)) break;
      }
      if (cnt == 0 || cnt > 6) goto failure_free;
      isas = ObjListListISACondenseDest(isas);
      if (year == -1) {
        ts = NULL;
      } else {
        YearToTs(year, &ts1);
        ts = &ts1;
      }
      if (new_isas == NULL &&
          (r = TA_ProductFindExisting(brand, isas, attributes, ts))) {
      /* The parsed product already exists.
       * todo: Learn new_isas as fractal modifications?
       */
        Dbg(DBGGEN, DBGDETAIL, "TA_Product found existing", M(brand));
        goto success;
      }
      new_isas = ObjListReverseDest(new_isas);
      if ((r = TA_ProductLearnNew(brand, isas, attributes, new_isas, year, ts,
                                  p_bwd+1, dc))) {
        Dbg(DBGGEN, DBGDETAIL, "TA_Product learned new", M(brand));
        goto success;
      }
    } else if (ObjIsAbstract(brand)) {
    /* Don't generate todo below. */
    } else {
      Dbg(DBGGEN, DBGBAD, "todo: ==*%s//|manufacturer-of¤___| <%.30s>",
          M(brand), start_in);
    }
  }
  return(0);

success:
  ObjListPrint(Log, r);
  ObjListListFree(isas);
  ObjListListFree(attributes);
  ObjListFree(new_isas);
  ChannelAddPNode(ch, PNTYPE_PRODUCT, 1.0, r, NULL, p_bwd+1, p_fwd);
  *nextp = p_fwd;
  return(1);

failure_free:
  ObjListListFree(isas);
  ObjListListFree(attributes);
  ObjListFree(new_isas);
  return(0);
}

void TA_ProductTrade(PNode *pns)
{
  PNode	*pn, *emh, *product, *price, *telno;
  Obj	*product_obj, *price_obj, *telno_obj, *from_address_obj;
  Obj	*buysell_obj, *buysell, *newsgroup, *message_id, *url;
  Ts	*ts;
  emh = product = price = telno = NULL;
  buysell_obj = NULL;
  for (pn = pns; pn; pn = pn->next) {
    switch (pn->type) {
      case PNTYPE_EMAILHEADER:
        if (emh && product && price && buysell_obj == NULL) {
        /* If we have a product and price but no buysell_obj, assume
         * N("for-sale"). todoSCORE
         */
          buysell_obj = N("for-sale");
        }
        if (buysell_obj && emh && product) {
          if (product->u.product->obj) product_obj = product->u.product->obj;
          else break;
          if (price) price_obj = price->u.number;
          else price_obj = ObjNA;
          if (telno) telno_obj = telno->u.telno;
          else telno_obj = ObjNA;
          if (emh->u.emh->from_address[0]) {
            from_address_obj =
              StringToObj(emh->u.emh->from_address, N("email-address"), 1);
          } else {
            from_address_obj = ObjNA;
          }
          /* todo: Location. */
          /* todo: Sanity check the price with other similar products:
           * Of course, we don't want to filter out good deals!
           */
          if (emh->u.emh->message_id) message_id = emh->u.emh->message_id;
          else message_id = ObjNA;
          if (emh->u.emh->newsgroup) newsgroup = emh->u.emh->newsgroup;
          else newsgroup = ObjNA;
          url = ObjNA; /* todo: if inside HTML file */
          buysell = L(buysell_obj, product_obj, price_obj, emh->u.emh->from_obj,
                      ObjNA, telno_obj, from_address_obj,
                      newsgroup, message_id, url, E);
          if (!ZRE(&TsNA, buysell)) {
            if ((ts = EmailHeaderTs(emh->u.emh))) {
              ObjTsStartStop(ts, buysell);
            }
            LearnAssert(buysell, StdDiscourse);
          }
        }
        emh = pn;
        product = price = telno = NULL;
        buysell_obj = NULL;
        break;
      case PNTYPE_PRODUCT:
        product = pn;
        break;
      case PNTYPE_NUMBER:
        price = pn;
        break;
      case PNTYPE_TELNO:
        telno = pn;
        break;
      case PNTYPE_LEXITEM:
        if (LexEntryConceptIsAncestor(N("for-sale"),
                                      PNodeLeftmostLexEntry(pn))) {
          buysell_obj = N("for-sale");
        } else if (LexEntryConceptIsAncestor(N("wanted-to-buy"),
                                             PNodeLeftmostLexEntry(pn))) {
          buysell_obj = N("wanted-to-buy");
        }
        break;
      default:
        break;
    }
  }
}

/* $6000
 * $.35
 * $ 2.00
 * $51,900...................................
 *
 * todo: Other currencies besides $
 * todo: Capture various modifiers such as:
 *   $2,300 negotiable
 *   asking $3,800
 *   Asking: $7,800
 *   asking $1950.00.
 *   ===$18,000 FIRM ====
 *   $3000 or best offer.
 *   Will consider any offers.
 *   A$33/year local or A$20/year interstate or
 *   costs about $25.00 to join
 *   Price: $3000 b/o
 *   $3000 OBO
 */
Bool TA_Price(char *in, char *in_base, Discourse *dc,
              /* RESULTS */ Channel *ch, char **nextp)
{
  char	*start_in;
  Float	price, mult;
  start_in = in;
  if (*in != '$') return(0);
  in++;
  in = StringSkipspace(in);
  price = 0.0;
  while (Char_isdigit(*in) || *in == ',') {
    if (Char_isdigit(*in)) {
      price = price*10.0 + ((*in) - '0');
    }
    in++;
  }
  if (*in == '.') {
    in++;
    mult = 0.10;
    while (Char_isdigit(*in)) {
      price += mult*((*in) - '0');
      mult = 0.1 * mult;
      in++;
    }
  }
  if (in < start_in+2) return(0);
  ChannelAddPNode(ch, PNTYPE_NUMBER, 1.0, NumberToObjClass(price, N("USD")),
                  NULL, start_in, in);
  *nextp = in;
  return(1);
}

Bool TA_Telno(char *in, Discourse *dc, /* RESULTS */ Channel *ch, char **nextp)
{
  char	digits[PHRASELEN], pstn[PHRASELEN];
  if (StringPhoneMatch("(NXX) NXX=XXXX", in, digits, nextp) ||
      StringPhoneMatch("(NXX)NXX=XXXX", in, digits, nextp) ||
      StringPhoneMatch("NXX=NXX=XXXX", in, digits, nextp)) {
    sprintf(pstn, "1%s", digits);
  } else if (StringPhoneMatch("1=NXX=NXX=XXXX", in, digits, nextp) ||
             StringPhoneMatch("+1=NXX=NXX=XXXX", in, digits, nextp)) {
    StringCpy(pstn, digits, PHRASELEN);
  } else if (StringPhoneMatch("19=1=NXX=NXX=XXXX", in, digits, nextp)) {
    StringCpy(pstn, digits+2, PHRASELEN);
  } else if (StringPhoneMatch("16 (1) XX=XX=XX=XX", in, digits, nextp) ||
             StringPhoneMatch("16=1=XX=XX=XX=XX", in, digits, nextp)) {
    sprintf(pstn, "33%s", digits+2);
  } else if (StringPhoneMatch("16=XX=XX=XX=XX", in, digits, nextp)) {
    sprintf(pstn, "33%s", digits+2);
  } else if (StringPhoneMatch("(1) XX=XX=XX=XX", in, digits, nextp) ||
             StringPhoneMatch("1=XX=XX=XX=XX", in, digits, nextp)) {
    sprintf(pstn, "33%s", digits);
  } else if (StringPhoneMatch("XX=XX=XX=XX", in, digits, nextp)) {
    sprintf(pstn, "331%s", digits);
  } else return(0);
  ChannelAddPNode(ch, PNTYPE_TELNO, 1.0, StringToObj(pstn, N("pstn"), 0),
                  NULL, in, *nextp);
  return(1);
}

/* Examples:
 * {L'Aurore}
 * {Paris-Match}
 * {La passion de Maximilien Kolbe}
 * _Cyc Programmer's Manual_
 * L'émission "Hot Country Night"
 * une chanson, "I will always love you"
 * l'album "Dangerous"
 * the "Dangerous" album
 * le film "Horizons lointains"
 * todo:
 * - l'article "Chet Atkins History" de Pierre Danielou
 * - le livre d'Art Kleps entitulé _Millbrook_
 *   (class DE name) (entitulé media-object)
 */
Bool TA_MediaObject(char *in, char *in_base, Discourse *dc,
                    /* RESULTS */ Channel *ch, char **nextp)
{
  int		openchar, closechar, len;
  char		buf[PHRASELEN], buf2[PHRASELEN], *start_in;
  Obj		*obj, *class;
  ObjList	*r;
  if (!CharIsOpening(*in)) {
    return(0);
  }
  if (*in == SQUOTE) return(0);
  if (in > in_base && CharIsAlpha(*(in-1))) return(0);
  openchar = *in;
  closechar = CharClosing(openchar);
  start_in = in+1;
  if (!StringReadToRequireTerm(start_in, closechar, TERM, PHRASELEN, buf,
                               &in)) {
    return(0);
  }
  len = strlen(buf);
  if (len == 0) return(0);
  if (len > 80) {
  /* Too long to be a media object name. todoSCORE */
    return(0);
  }
  if (StringPercentAlpha(buf) < 70.0) {
  /* todoSCORE */
    return(0);
  }
  StringElimDupWhitespaceDest(buf);
  StringCpy(buf2, buf, PHRASELEN);
  StringMapLeWhitespaceToSpace(buf2);
  r = TA_FindPhraseTryBoth(DC(dc).ht, buf2, N("media-object"));
  if (r) goto success;
  if (!(class = TA_FindNearbyClassClue(start_in-1, in, in_base,
                                       N("media-object"), dc))) {
    class = N("media-object");
  }
  if ((obj = LearnObjText(class, buf, F_NULL, F_SINGULAR, F_NOUN,
                          DC(dc).lang, 0, OBJ_CREATE_C, start_in-1, dc))) {
    r = ObjListCreate(obj, NULL);
    goto success;
  }
  return(0);

success:
  ChannelAddPNode(ch, PNTYPE_MEDIA_OBJ, 1.0, r, NULL, start_in, in);
  *nextp = in;
  return(1);
}

/* End of file. */
