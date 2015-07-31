/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * Derivational morphology.
 *
 * 19951010: begun
 * 19951012: more work
 * 19951021: debugging and mods
 * 19960203: more work
 *
 * todo: 
 * - Use corpus as stimulus: derive words present in corpus but missing
 *   from ThoughtTreasure's lexicon.
 * - Implement processing of conventional metaphors as outlined in notebook.
 */

#include "tt.h"
#include "lexentry.h"
#include "lexwf2.h"
#include "repbasic.h"
#include "repdb.h"
#include "repobj.h"
#include "repobjl.h"
#include "repstr.h"
#include "semparse.h"
#include "utildbg.h"
#include "utillrn.h"

#define MIN_LHS_LEN	2

Affix *WordForm2Suffixes, *WordForm2Prefixes;

Affix *AffixCreate(char *word, char *features, int lang, Obj *obj,
                   Affix *next)
{
  HashTable	*ht;
  Affix		*af;
  ht = LexEntryLangHt1(lang);
  af = CREATE(Affix);
  af->word = HashTableIntern(ht, word);
  af->features = HashTableIntern(ht, features);
  af->lang = lang;
  af->obj = obj;
  af->next = next;
  return(af);
}

char *LexEntryRoot(LexEntry *le)
{
  return(le->srcphrase);
}

int WordForm2EnglishSuffixesCnt, WordForm2EnglishPrefixesCnt;
int WordForm2FrenchSuffixesCnt, WordForm2FrenchPrefixesCnt;

void Lex_WordForm2Init()
{
  WordForm2Suffixes = WordForm2Prefixes = NULL;
  WordForm2EnglishSuffixesCnt = WordForm2EnglishPrefixesCnt =
  WordForm2FrenchSuffixesCnt = WordForm2FrenchPrefixesCnt = 0;
}

void Lex_WordForm2Enter(char *srcphrase, char *features, int pos, int lang,
                        Obj *obj)
{
  /* todo: WordForm2Suffixes/Prefixes sorted by decreasing length.
   * (But why? We try all possibilities anyway.)
   */
  if (IsPhrase(srcphrase)) {
    Dbg(DBGLEX, DBGBAD, "phrasal suffix <%s> ignored", srcphrase);
    return;
  }
  if (pos == F_PREFIX) {
    WordForm2Prefixes = AffixCreate(srcphrase, features, lang, obj,
                                    WordForm2Prefixes);
    if (lang == F_FRENCH) {
      WordForm2FrenchPrefixesCnt++;
    } else {
      WordForm2EnglishPrefixesCnt++;
    }
  } else if (pos == F_SUFFIX) {
    WordForm2Suffixes = AffixCreate(srcphrase, features, lang, obj,
                                    WordForm2Suffixes);
    if (lang == F_FRENCH) {
      WordForm2FrenchSuffixesCnt++;
    } else {
      WordForm2EnglishSuffixesCnt++;
    }
  }
}

void Lex_WordForm2DoneLoading()
{
  Dbg(DBGLEX, DBGDETAIL, "English: %d prefixes and %d suffixes",
      WordForm2EnglishPrefixesCnt, WordForm2EnglishSuffixesCnt);
  Dbg(DBGLEX, DBGDETAIL, "French: %d prefixes and %d suffixes",
      WordForm2FrenchPrefixesCnt, WordForm2FrenchSuffixesCnt);
}

void Lex_WordForm2Derive4(HashTable *ht, int is_prefix, char *affix_word,
                          char *affix_features, Obj *affix_obj,
                          Obj *lhs_obj,
                          char *lhs_word, char *lhs_feature,
                          char *lhs_leo_features, LexEntry *rhs_le,
                          char *rhs_word, /* RESULTS */ int *found)
{
  int	lhs_pos_const, lhs_pos_ptn, rhs_pos, is_le0, affix_style;
  char	feat[FEATLEN];
  Obj	*rule_obj, *lhs_class, *rhs_obj, *rhs_class, *rhs_feat, *rhs_assertion;
  Obj	*affix_weight, *lhs_pos_obj, *rhs_pos_obj;

  Dbg(DBGLEX, DBGHYPER, "        attempt to derive 4 <%s> <%s> <%s> <%s> <%s>",
      M(affix_obj), M(lhs_obj), lhs_word, affix_word, rhs_word);
  lhs_pos_const = FeatureGet(lhs_feature, FT_POS);
  rule_obj = affix_obj;
  if (ISA(N("attribute"), affix_obj)) {
    rule_obj = N("affix-attribute");
  } else if (ISA(N("adverb-of-absolute-degree"), affix_obj)) {
    if (lhs_pos_const == F_ADJECTIVE) rule_obj = N("affix-intensifier-adj");
    else if (lhs_pos_const == F_ADVERB) rule_obj = N("affix-intensifier-adv"); 
  }
  rhs_pos_obj = DbGetRelationValue(&TsNA, NULL, N("rhs-pos-of"), rule_obj,
                                   NULL);
  if (rhs_pos_obj) {
    rhs_pos = ConToFeat(rhs_pos_obj);
  } else {
    rhs_pos = lhs_pos_const;
      /* For N("suffix-any-to-any") and N("affix-intensifier-*"). */
  }
  if (rhs_pos != FeatureGet(rhs_le->features, FT_POS)) {
    Dbg(DBGLEX, DBGHYPER, "rhs_pos <%c> != pos(rhs_le) <%c>",
        (char)rhs_pos, (char)FeatureGet(rhs_le->features, FT_POS));
    return;
  }
  lhs_class = DbGetRelationValue(&TsNA, NULL, N("lhs-class-of"), rule_obj,
                                 NULL);
  if (lhs_class == N("r1")) {
    lhs_class = DbGetRelationValue(&TsNA, NULL, N("r1"), affix_obj, NULL);
  }
  lhs_pos_obj = DbGetRelationValue(&TsNA, NULL, N("lhs-pos-of"),
                                   rule_obj, NULL);
  if (lhs_pos_obj) {
    lhs_pos_ptn = ConToFeat(lhs_pos_obj);
  } else {
    lhs_pos_ptn = lhs_pos_const;
      /* for N("suffix-any-to-any") */
  }
  rhs_class = DbGetRelationValue(&TsNA, NULL, N("rhs-class-of"), rule_obj,
                                 NULL);
  rhs_feat = DbGetRelationValue(&TsNA, NULL, N("rhs-feat-of"), rule_obj, NULL);
  if ((lhs_class == NULL || ISAP(lhs_class, lhs_obj)) &&
      (lhs_pos_ptn == F_NULL || lhs_pos_ptn == lhs_pos_const)) {
    *found = 1;
    if (rhs_class == NULL) {
    /* NULL rhs_class means use <lhs_obj> as parent of <rhs_obj> */
      rhs_class = lhs_obj;	/* "poetess" */
    }
    if (rhs_class == N("rhs-class-synonym")) {
      rhs_class = ObjGetAParent(lhs_obj);
      rhs_obj = lhs_obj;
    } else {
      rhs_obj = ObjCreateInstanceText(rhs_class, rhs_word, 0, OBJ_CREATE_A,
                                      &is_le0, NULL);
    }
    feat[0] = rhs_pos;
    feat[1] = LexEntryHtLang(ht);
    feat[2] = TERM;
    if (rhs_feat && ObjIsString(rhs_feat)) {
      StringCat(feat, ObjToString(rhs_feat), FEATLEN);
    }
    affix_style = FeatureGet(affix_features, FT_STYLE);
    /* todo: Also carry over: pejorative. */
    if (affix_style != F_NULL) {
      StringAppendChar(feat, FEATLEN, affix_style);
    }
    LearnObjDUMP(rhs_class, rhs_obj, rhs_word, feat, NULL, 0, OBJ_CREATE_A,
                 NULL, StdDiscourse);
    LexEntryLinkToObj(rhs_le, rhs_obj, feat, ht, NULL, 0, NULL, NULL, NULL);
    if ((rhs_assertion = DbGetRelationValue(&TsNA, NULL, N("rhs-assertion-of"),
                                            rule_obj, NULL))) {
      affix_weight = D(WEIGHT_DEFAULT);
      rhs_assertion = ObjSubst(rhs_assertion, N("affix-obj"), affix_obj);
      rhs_assertion = ObjSubst(rhs_assertion, N("affix-weight"), affix_weight);
      rhs_assertion = ObjSubst(rhs_assertion, N("lhs-obj"), lhs_obj);
      rhs_assertion = ObjSubst(rhs_assertion, N("rhs-obj"), rhs_obj);
                /* todo: Freeing. */
      LearnAssert(rhs_assertion, NULL);
    }
    if (DbgOn(DBGLEX, DBGDETAIL)) {
      IndentPrint(Log);
      if (is_prefix) {
        fprintf(Log, "/%s/+/%s/", affix_word, lhs_word);
      } else {
        fprintf(Log, "/%s/+/%s/", lhs_word, affix_word);
      }
      fprintf(Log, " -> /%s/ <%s>\n", rhs_word, M(rhs_obj));
    }
  } else {
    if (DbgOn(DBGLEX, DBGHYPER)) {
      if (lhs_class != NULL && !ISAP(lhs_class, lhs_obj)) {
        Dbg(DBGLEX, DBGHYPER, "        failed !ISAP <%s> <%s>",
            M(lhs_class), M(lhs_obj));
      }
      if (lhs_pos_ptn != F_NULL && lhs_pos_ptn != lhs_pos_const) {
        Dbg(DBGLEX, DBGHYPER, "        failed <%c> != <%c>",
            (char)lhs_pos_ptn, (char)lhs_pos_const);
      }
    }
  }
}

void Lex_WordForm2Derive3(HashTable *ht, int is_prefix, int maxlevel,
                          int derive_depth, Affix *af, char *lhs_word,
                          LexEntry *rhs_le, char *rhs_word,
                          /* RESULTS */ int *found)
{
  int		lhs_freeme;
  LexEntryToObj	*leo;
  IndexEntry	*lhs_ie, *lhs_ie1;
  Obj		*prev_obj;
  Dbg(DBGLEX, DBGHYPER, "      attempt to derive 3 <%s> <%s>", lhs_word,
      rhs_word);
  lhs_ie = LexEntryFindPhrase1(ht, lhs_word, maxlevel, 1, derive_depth+1,
                               &lhs_freeme);
  /* lhs_ie: disk.Nz/disk.Vz/ */
  prev_obj = NULL;
  for (lhs_ie1 = lhs_ie; lhs_ie1; lhs_ie1 = lhs_ie1->next) {
    if (lhs_ie1->lexentry == NULL) continue;
    for (leo = lhs_ie1->lexentry->leo; leo; leo = leo->next) {
/* This is wrong, since this le could be a different part of speech, and
   we have in fact gotten burned by this.
      if (leo->obj == prev_obj) continue;
 */
      prev_obj = leo->obj;
      Dbg(DBGLEX, DBGHYPER, "      lhs le <%s>.<%s>",
          lhs_ie1->lexentry->srcphrase,
          lhs_ie1->lexentry->features);
      Lex_WordForm2Derive4(ht, is_prefix, af->word, af->features, af->obj,
                           leo->obj, lhs_word, lhs_ie1->lexentry->features,
                           leo->features, rhs_le, rhs_word, found);
    }
  }
  if (lhs_freeme) IndexEntryFree(lhs_ie);
}

#ifdef notdef
/* todo: Use this in lexical gap generation.
 * Could verify existence of created word against corpus to find
 * the right suffix (among synonym suffixes).
 */
Bool Lex_WordForm2EnglishAffixGen(int is_prefix, char *lhs_word0,
                                  char *affix_word,
                                  /* RESULTS */ char *lhs_word)
{
  int	len;
  len = strlen(lhs_word0);
  if (is_prefix) {
  } else {
    if (LexEntryIsEnglishVowel(affix_word[0]) &&
        LexEntryIsEnglishVowel(lhs_word0[len-2]) &&
        (!LexEntryIsEnglishVowel(lhs_word0[len-1]))) {
    /* _VC+V -> _VCC+V_
     * red+en -> <redd>+en
     */
      StringCpy(lhs_word, lhs_word0, DWORDLEN);
      StringAppendChar(lhs_word, DWORDLEN, lhs_word0[len-1]);
      return(1);
    } else if (LexEntryIsEnglishVowel(affix_word[0]) &&
               ('e' == lhs_word0[len-1])) {
    /* _e+V_ -> _+V_
     * molecule+ar -> <molecul>+ar
     * active+ate -> <activ>+ate
     * complete+ion -> <complet>+ion
     */
      StringCpy(lhs_word, lhs_word0, DWORDLEN);
      lhs_word[len-1] = TERM;
      return(1);
    } else if ((!LexEntryIsEnglishVowel(affix_word[0])) &&
               (!LexEntryIsEnglishVowel(lhs_word0[len-1]))) {
    /* _C+C_ -> _Ca+C_
     * chomp+rama -> <chompa>+rama
     */
      StringCpy(lhs_word, lhs_word0, DWORDLEN);
      if (LexEntryIsEnglishVowel(lhs_word0[len-2])) {
      /* _VC +C _ -> _VC C a+C _
       *    0  1        0 0   1
       * yum+rama -> <yumma>+ramma
       */
        StringAppendChar(lhs_word, DWORDLEN, lhs_word0[len-1]);
      }
      StringAppendChar(lhs_word, DWORDLEN, 'a');
      return(1);
    } else if (lhs_word0[len-1] == affix_word[0]) {
    /* _X+X_ -> _+X_
     * flavor+rama -> <flavo>+rama
     */
      StringCpy(lhs_word, lhs_word0, DWORDLEN);
      lhs_word0[len-1] = TERM;
      return(1);
    } else if (lhs_word0[len-1] == 'y' &&
               (!LexEntryIsEnglishVowel(affix_word[0]))) {
    /* _y+C_ -> _i+C_
     * dirty+ness -> <dirti>+ness
     */
      StringCpy(lhs_word, lhs_word0, DWORDLEN);
      lhs_word0[len-1] = 'i';
      return(1);
    }
  }
  return(0);
}
#endif

/* lhs_word0 affix_word rhs_word     lhs_word
 * --------- ---------- --------  => --------
 * yummo     rama       yummorama    yum
 * chompa    rama       chomparama   chomp
 * dulls     ville      dullsville   dull   (todo: specific to ville?)
 * flavo     rama       flavorama    flavor (todo: should this always be done?)
 * dirti     ly         dirtily      dirty
 * redd      en         redden       red
 * molecul   ar         molecular    molecule (todo: should this always bedone?)
 *
 * todo:
 * Not sure how to treat the following, since we don't really know what
 * suffix to add, and matching on initial substrings might cause too
 * many matches, though maybe not (cf clipping):
 * terr      ify        terrify      terror
 * delic     ify        delicify     delicious
 */
Bool Lex_WordForm2EnglishAffixParse(int is_prefix, char *lhs_word0,
                                    char *affix_word,
                                    /* RESULTS */ char *lhs_word)
{
  int	len;
  len = strlen(lhs_word0);
  if (is_prefix) {
  } else {
    if (lhs_word0[len-1] == 'i' &&
        (!LexEntryIsEnglishVowel(affix_word[0]))) {
    /* dirti+ness -> <dirty>+ness */
      StringCpy(lhs_word, lhs_word0, DWORDLEN);
      lhs_word[len-1] = 'y';
      return(1);
    } else if (streq("ville", affix_word)) {
    /* dulls+ville -> <dull>+ville */
      StringCpy(lhs_word, lhs_word0, DWORDLEN);
      lhs_word[len-1] = TERM;
      return(1);
    } else if ((!LexEntryIsEnglishVowel(affix_word[0])) &&
        (lhs_word0[len-2] == lhs_word0[len-1]) &&
        (!LexEntryIsEnglishVowel(lhs_word0[len-1]))) {
    /* _C C +V_ -> _C +V_ 
     *   0 0         0
     * redd+en -> <red>+en
     */
      StringCpy(lhs_word, lhs_word0, DWORDLEN);
      lhs_word[len-1] = TERM;
      return(1);
    } else if ((!LexEntryIsEnglishVowel(affix_word[0])) &&
               (!LexEntryIsEnglishVowel(lhs_word0[len-3])) &&
               (lhs_word0[len-3] == lhs_word0[len-2]) &&
               LexEntryIsEnglishVowel(lhs_word0[len-1])) {
    /* _VC C V+C _ -> _VC +C _
     *    0 0   1        0  1        
     * yummo+rama -> <yum>+rama
     */
      StringCpy(lhs_word, lhs_word0, DWORDLEN);
      lhs_word[len-2] = TERM;
      return(1);
    } else if ((!LexEntryIsEnglishVowel(affix_word[0])) &&
               (!LexEntryIsEnglishVowel(lhs_word0[len-2])) &&
               LexEntryIsEnglishVowel(lhs_word0[len-1])) {
    /* _CV+C_ -> _C+C_
     * chompa+rama -> <chomp>+rama
     */
      StringCpy(lhs_word, lhs_word0, DWORDLEN);
      lhs_word[len-1] = TERM;
      return(1);
    } else if (LexEntryIsEnglishVowel(affix_word[0]) &&
               LexEntryIsEnglishVowel(lhs_word0[len-2]) &&
               (!LexEntryIsEnglishVowel(lhs_word0[len-1]))) {
    /* _VC+V_ -> _VCe+V_
     * molecul+ar -> <molecule>+ar
     */
      StringCpy(lhs_word, lhs_word0, DWORDLEN);
      StringAppendChar(lhs_word, DWORDLEN, 'e');
      return(1);
    } else if (LexEntryIsEnglishVowel(affix_word[0]) &&
               LexEntryIsEnglishVowel(lhs_word0[len-1]) &&
               (!LexEntryIsEnglishVowel(lhs_word0[len-2]))) {
    /* _CV+V_ -> _CVe+V_
     * tru+ize -> <true>+ize
     */
      StringCpy(lhs_word, lhs_word0, DWORDLEN);
      StringAppendChar(lhs_word, DWORDLEN, 'e');
      return(1);
    } else {
    /* flavo+rama -> <flavor>+rama */
      StringCpy(lhs_word, lhs_word0, DWORDLEN);
      lhs_word[len] = affix_word[0];
      lhs_word[len+1] = TERM;
      return(1);
    }
  }
  return(0);
}

void Lex_WordForm2Derive2(HashTable *ht, int is_prefix, int maxlevel,
                          int derive_depth, Affix *af, char *lhs_word0,
                          LexEntry *rhs_le, char *rhs_word,
                          /* RESULTS */ int *found)
{
  char	lhs_word[DWORDLEN];
  Dbg(DBGLEX, DBGHYPER, "    attempt to derive 2 <%s> <%s>", lhs_word0,
      rhs_word);
  if (ht == EnglishIndex) {
    if (Lex_WordForm2EnglishAffixParse(is_prefix, lhs_word0, af->word,
                                       lhs_word)) {
      if (streq(lhs_word, lhs_word0)) Stop();
      Lex_WordForm2Derive3(ht, is_prefix, maxlevel, derive_depth, af, lhs_word,
                           rhs_le, rhs_word, found);
    }
    Lex_WordForm2Derive3(ht, is_prefix, maxlevel, derive_depth, af, lhs_word0,
                         rhs_le, rhs_word, found);
  } else if (ht == FrenchIndex) {
    Lex_WordForm2Derive3(ht, is_prefix, maxlevel, derive_depth, af, lhs_word0,
                         rhs_le, rhs_word, found);
  }
}

IndexEntry *Lex_WordForm2Derive1A(HashTable *ht, char *rhs_word0, int maxlevel,
                                  int derive_depth, LexEntry *rhs_le,
                                  IndexEntry *ie_rest)
{
  int		lang, found, rhs_len, lhs_len, af_len;
  char		lhs_word[DWORDLEN], *rhs_word;
  Affix		*af;

  lang = LexEntryHtLang(ht);

  found = 0;

  Dbg(DBGLEX, DBGHYPER, "  attempt to derive 1A <%s>.<%s>",
      rhs_le->srcphrase, rhs_le->features);

  /* rhs_le:
   * trouvailler.Vy/
   * delicify.Vz/
   * confuserette.Nz/
   * diskette.Nz/
   */
  rhs_word = LexEntryRoot(rhs_le);
  rhs_len = strlen(rhs_word);
  for (af = WordForm2Suffixes; af; af = af->next) {
    if (lang != af->lang) continue;
    af_len = strlen(af->word);
    if (af_len + MIN_LHS_LEN >= rhs_len) continue;
    if (streq(rhs_word+rhs_len-af_len, af->word)) {
      lhs_len = rhs_len-af_len;
      memcpy(lhs_word, rhs_word, lhs_len);
      lhs_word[lhs_len] = TERM;
      Lex_WordForm2Derive2(ht, 0, maxlevel, derive_depth, af, lhs_word,
                           rhs_le, rhs_word, &found);
    }
  }
  for (af = WordForm2Prefixes; af; af = af->next) {
    if (lang != af->lang) continue;
    af_len = strlen(af->word);
    if (af_len + MIN_LHS_LEN >= rhs_len) continue;
    if (0 == strncmp(rhs_word, af->word, af_len)) {
      lhs_len = rhs_len-af_len;
      memcpy(lhs_word, rhs_word + af_len, lhs_len);
      lhs_word[lhs_len] = TERM;
      Lex_WordForm2Derive2(ht, 1, maxlevel, derive_depth, af, lhs_word,
                           rhs_le, rhs_word, &found);
    }
  }

  if (found) {
    return(IndexEntryCreate(rhs_le, rhs_le->features, ie_rest));
  } else {
  /* todo: Delete rhs_le if the derivation fails: This is the spell correct
   * case. Perhaps defer IndexEntry indexing until here.
   */
    return(ie_rest);
  }
}

/* todoFUNNY: Derive_depth too high. */
IndexEntry *Lex_WordForm2Derive1(HashTable *ht, char *rhs_word0, int maxlevel,
                                 int derive_depth)
{
  LexEntry	*rhs_le1, *rhs_le2, *rhs_le3, *rhs_le4, *rhs_le5;
  IndexEntry	*r;
/*
  if (!AnaMorphOn) {
    Dbg(DBGLEX, DBGBAD, "unknown word: <%s>", rhs_word0);
    r = NULL;
    goto done;
  }
 */
  if (strlen(rhs_word0) > WORDLEN) {
    Dbg(DBGLEX, DBGDETAIL, "word too long: <%s>", rhs_word0);
    return(NULL);
  }
  if (DbgOn(DBGLEX, DBGDETAIL)) {
    IndentUp();
    IndentPrint(Log);
    fprintf(Log, "DERIVE <%s>\n", rhs_word0);
  }
  /* rhs_word0:
   * "trouvaillâtes"
   * "delicifies"
   * "confuserettes"
   */
  if (ht == EnglishIndex) {
    LexEntryInflectWord0(rhs_word0, F_ENGLISH, FS_ENGLISH, ht, &rhs_le1,
                         &rhs_le2, &rhs_le3, &rhs_le4, &rhs_le5);
  } else {
    /* todo: Could send gender and number if a determiner precedes. */
    LexEntryInflectWord0(rhs_word0, F_FRENCH, FS_FRENCH, ht, &rhs_le1,
                         &rhs_le2, &rhs_le3, &rhs_le4, &rhs_le5);
  }
  r = NULL;
  if (rhs_le1 == NULL && rhs_le2 == NULL && rhs_le3 == NULL &&
      rhs_le4 == NULL && rhs_le5 == NULL) {
    Dbg(DBGLEX, DBGBAD, "trouble inflecting: <%s>", rhs_word0);
    goto done;
  }
  if (rhs_le1) {
    r = Lex_WordForm2Derive1A(ht, rhs_word0, maxlevel, derive_depth,
                              rhs_le1, r);
  }
  if (rhs_le2) {
    r = Lex_WordForm2Derive1A(ht, rhs_word0, maxlevel, derive_depth,
                              rhs_le2, r);
  }
  if (rhs_le3) {
    r = Lex_WordForm2Derive1A(ht, rhs_word0, maxlevel, derive_depth,
                              rhs_le3, r);
  }
  if (rhs_le4) {
    r = Lex_WordForm2Derive1A(ht, rhs_word0, maxlevel, derive_depth,
                              rhs_le4, r);
  }
  if (rhs_le5) {
    r = Lex_WordForm2Derive1A(ht, rhs_word0, maxlevel, derive_depth,
                              rhs_le5, r);
  }
done:
  if (DbgOn(DBGLEX, DBGDETAIL)) {
    IndentPrint(Log);
    if (r) {
      fprintf(Log, "DERIVED <%s>\n", rhs_word0);
    } else {
      if (derive_depth == 0) fprintf(Log, "UNABLE TO DERIVE <%s>\n", rhs_word0);
    }
    IndentDown();
  }
  return(r);
}

IndexEntry *Lex_WordForm2Derive(HashTable *ht, char *rhs_word0, int maxlevel,
                                int derive_depth)
{
  if (derive_depth == 0) IndentInit();
  return(Lex_WordForm2Derive1(ht, rhs_word0, maxlevel, derive_depth));
}

/* TESTING */

int Lex_WordForm2Test(char *fn, HashTable *ht)
{
  char  line[LINELEN];
  FILE	*stream;
  if (NULL == (stream = StreamOpen(fn, "r"))) return(0);
  while (fgets(line, LINELEN, stream)) {
    line[strlen(line)-1] = TERM;
    Lex_WordForm2Derive(ht, line, 1, 0);
  }
  StreamClose(stream);
  return(1);
}

/* End of file. */
