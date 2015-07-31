/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19940510: begun
 * 19940511: first testing
 * 19940512: more work
 *
 * todo:
 * - Weather and seasons should affect clothing choice.
 * - Take into account color percentages.
 * - Zipping and buttoning.
 * - Doing the laundry.
 */

#include "tt.h"
#include "repbasic.h"
#include "repdb.h"
#include "repmisc.h"
#include "repobj.h"
#include "repobjl.h"
#include "repspace.h"
#include "reptime.h"
#include "utildbg.h"

/* LAYERS */

/* todoDATABASE */
int ClothingLayerNumber(char *layer)
{
  if (streq(layer, "underlayerpre")) return(1);
  else if (streq(layer, "underlayer")) return(2);
  else if (streq(layer, "underlayerpost")) return(3);
  else if (streq(layer, "shirtlayer")) return(4);
  else if (streq(layer, "tielayer")) return(5);
  else if (streq(layer, "vestlayer")) return(6);
  else if (streq(layer, "jacketlayer")) return(7);
  else if (streq(layer, "coatlayer")) return(8);
  else if (streq(layer, "overlayer")) return(9);
  else if (streq(layer, "overlayerpost")) return(10);
  else return(11);
}

Bool ClothingIsOver(Ts *ts, Obj *clothing1, Obj *clothing2)
{
  int	r;
  Obj	*layer1, *layer2;
  layer1 = RADNI(0, ts, L(N("clothing-layer"), clothing1, E), 1, 0);
  layer2 = RADNI(0, ts, L(N("clothing-layer"), clothing2, E), 1, 0);
  r = layer1 && layer2 &&
      ClothingLayerNumber(M(layer1)) > ClothingLayerNumber(M(layer2));
  Dbg(DBGCLOTHING, DBGDETAIL, "ClothingIsOver <%s> <%s> <%s> <%s> %d",
     M(clothing1), M(clothing2), M(layer1), M(layer2), r);
  return(r);
}

/* Find any item of higher layer covering same area of body.
 * Called by PA_PutOnClothes.
 */
Obj *ClothingFindOver(Ts *ts, Obj *human, Obj *clothing)
{
  Obj *r;
  ObjList *objs1, *objs2, *p, *q;
  objs1 = RAD(ts, L(N("clothing-area"), clothing, E), 1, 0);
  objs2 = RE(ts, L(N("wearing-of"), human, ObjWild, E));
  for (p = objs1; p; p = p->next) {
    for (q = objs2; q; q = q->next) {
      Dbg(DBGCLOTHING, DBGDETAIL, "try retrieving [%s %s]",
          M(I(p->obj, 0)), M(I(q->obj, 2)));
      if ((I(q->obj, 2) != clothing) &&
          RN(ts, L(I(p->obj, 0), I(q->obj, 2), E), 1) &&
          ClothingIsOver(ts, I(q->obj, 2), clothing)) {
        r = I(q->obj, 2);
        ObjListFree(objs1);
        ObjListFree(objs2);
        return(r);
      }
    }
  }
  ObjListFree(objs1);
  ObjListFree(objs2);
  return(NULL);
}

/* OUTFIT SELECTION */

/* Find candidate clothing of a given class. */
ObjList *ClothingFindItems(Ts *ts, Obj *human, Obj *class, Obj *notequal)
{
  ObjList	*objs, *p, *r;
  r = NULL;
  objs = SpaceFindNearby(ts, NULL, class, human);
  for (p = objs; p; p = p->next) {
    if (notequal != p->obj) {
      if (1 || IsOwnerOf(ts, NULL, p->obj, human)) { /* todo */
        r = ObjListCreate(p->obj, r);
      }
    }
  }
  ObjListFree(objs);
  objs = RD(ts, L(N("wearing-of"), human, class, E), 2);
  for (p = objs; p; p = p->next) {
    if (notequal != I(p->obj, 2)) {
      r = ObjListCreate(I(p->obj, 2), r);
    }
  }
  ObjListFree(objs);
  return(r);
}

void ColorGetHLS(Obj *color, /* RESULTS */ Float *hue, Float *luminance,
                 Float *saturation)
{
  Float	freq, lightmin, lightmax;
  
  lightmin = FreqParse("780", "ang");
  lightmax = FreqParse("380", "ang");
  freq = ObjToNumber(DbGetRelationValue(&TsNA, NULL, N("frequency-of"), color,
                     NumberToObjClass(lightmin, N("hertz"))));
  *hue = (freq - lightmin)/(lightmax - lightmin);
  *luminance = ObjToNumber(DbGetRelationValue(&TsNA, NULL, N("luminance-of"),
                           color, NumberToObj(WEIGHT_DEFAULT)));
  *saturation = ObjToNumber(DbGetRelationValue(&TsNA, NULL, N("saturation-of"),
                            color, NumberToObj(WEIGHT_DEFAULT)));
}

/* todo: Need to do color wraparound at red-violet: mod .5? */
Float ColorHarmony(Obj *color1, Obj *color2)
{
  Float	hue1, luminance1, saturation1, hue2, luminance2, saturation2;
  ColorGetHLS(color1, &hue1, &luminance1, &saturation1);
  ColorGetHLS(color2, &hue2, &luminance2, &saturation2);
  return(1.0 -
         (CoordEuclideanDistance3(hue1, luminance1, saturation1,
                                  hue2, luminance2, saturation2) /
          pow(3.0, 0.5)));
}

Bool ColorCombo(char *color1, char *color2, Obj *item1, Obj *item2)
{
  return((ISA(N(color1), item1) && ISA(N(color2), item2)) ||
         (ISA(N(color2), item1) && ISA(N(color1), item2)));
}

/* todoDATABASE */
Float ClothingColorMatchness1(Obj *color1, Obj *color2)
{
  if (color1 == color2) return(1.0);
  if (ISA(N("white"), color1)) return(.9);
  if (ISA(N("white"), color2)) return(.9);
  if (ColorCombo("red", "violet", color1, color2)) return(.8);
  if (ColorCombo("red", "black", color1, color2)) return(.8);
  if (ColorCombo("black", "white", color1, color2)) return(.8);
  if (ColorCombo("gray", "white", color1, color2)) return(.8);
  if (ColorCombo("gray", "black", color1, color2)) return(.8);
  return(ColorHarmony(color1, color2));
}

Float ClothingColorMatchness(Ts *ts, Obj *item1, Obj *item2)
{
  Float	r, sum, weight, matchness, weightsum;
  ObjList *colors1, *colors2, *p1, *p2;
  Dbg(DBGCLOTHING, DBGHYPER, "ClothingColorMatchness <%s> <%s>",
      M(item1), M(item2));
  colors1 = DbRetrievalAncDesc(ts, NULL, L(N("color"), item1, ObjWild, E), 1, 0,
                               NULL, NULL, 2, 5, 1);
  colors2 = DbRetrievalAncDesc(ts, NULL, L(N("color"), item2, ObjWild, E), 1, 0,
                               NULL, NULL, 2, 5, 1);
  sum = weightsum = 0.0;
  for (p1 = colors1; p1; p1 = p1->next) {
    for (p2 = colors2; p2; p2 = p2->next) {
      weight = ObjWeight(p1->obj)*ObjWeight(p2->obj);
      weightsum += weight;
      matchness = ClothingColorMatchness1(I(p1->obj, 0), I(p2->obj, 0));
      sum += weight*matchness;
      Dbg(DBGCLOTHING, DBGHYPER, "<%s> <%s> weight %g matchness %g",
          M(I(p1->obj, 0)), M(I(p2->obj, 0)), weight, matchness);
    }
  }
  ObjListFree(colors1);
  ObjListFree(colors2);
  r = sum/weightsum;
  Dbg(DBGCLOTHING, DBGHYPER, "result %g", r);
  return(r);
}

Float ClothingMaterialMatchness(Ts *ts, Obj *item1, Obj *item2)
{
  return(1.0); /* todo */
}

Float ClothingMatchness(Ts *ts, Obj *item1, Obj *item2)
{
  if (item1 == NULL || item2 == NULL) return(1.0);
  return(ClothingColorMatchness(ts, item1, item2) *
         ClothingMaterialMatchness(ts, item1, item2));
}

Float ClothingCleanness(Ts *ts1, Obj *obj)
{
  return(1.0); /* todo: Have to keep track of last time cleaned. */
}

Float ClothingForgottenness(Ts *ts1, Obj *obj)
{
  Ts		*ts2;
  ObjList	*objs, *p;
  Dur		diff, leastdiff;
  Float		d;
  objs = RE(&TsNA, L(N("wearing-of"), ObjWild, obj, E));
  /* todo: Only bother retrieving back a week. */
  leastdiff = DURPOSINF;
  for (p = objs; p; p = p->next) {
    ts2 = &ObjToTsRange(p->obj)->stopts;
    if (TsIsSpecific(ts2) && TsLT(ts2, ts1)) {
      diff = TsMinus(ts1, ts2);
      if (diff < leastdiff) leastdiff = diff;
    }
  }
  ObjListFree(objs);
  d = ((Float)leastdiff) / ((Float)SECONDSPERDAY);
  if (d > 9) return(1.0);
  else if (d < 0) {
    Dbg(DBGCLOTHING, DBGBAD, "negative leastdiff");
    return(.1);
  } else return(.1*d);
}

/* Find the best available item of clothing matching other items. */
Obj *ClothingFindBest(Ts *ts, Obj *human, Obj *class,
                      Obj *notequal, Obj *match1, Obj *match2)
{
  ObjList	*objs, *p;
  Float		w1, w2, w3, w4;
  Float		desirability, bestdesirability;
  Obj		*best;
  best = NULL;
  bestdesirability = FLOATNEGINF;
  objs = ClothingFindItems(ts, human, class, notequal);
  for (p = objs; p; p = p->next) {
    w1 = ClothingMatchness(ts, match1, p->obj);
    w2 = ClothingMatchness(ts, match2, p->obj);
    w3 = ClothingCleanness(ts, p->obj);
    w4 = ClothingForgottenness(ts, p->obj);
    desirability = w1*w2*w3*w4;
    Dbg(DBGCLOTHING, DBGHYPER,
        "<%s> match1 %g match2 %g cleanness %g forgottenness %g -> %g",
        M(p->obj), w1, w2, w3, w4, desirability);
    if (desirability > bestdesirability) {
      best = p->obj;
      bestdesirability = desirability;
    }
  }
  ObjListFree(objs);
  return(best);
}

Obj *ClothingFindPairedItem(Ts *ts, Obj *human, Obj *obj)
{
  ObjList	*objs;
  Obj		*parent, *r;
  if (obj == NULL) return(NULL);
  if (!(parent = ObjGetUniqueParent(obj))) return(NULL);
  objs = ClothingFindItems(ts, human, parent, obj);
  if (objs) r = objs->obj;
  else r = NULL;
  ObjListFree(objs);
  return(r);
}

ObjList *ClothingAddItem(Obj *item, ObjList *objs)
{
  if (item) return(ObjListCreate(item, objs));
  else return(objs);
}

ObjList *ClothingMaleBusiness(Ts *ts, Obj *human)
{
  Obj		*jacket, *pants, *shirt, *tie, *shoe1, *shoe2, *sock1, *sock2;
  ObjList	*r;

  /* todo: Need to pair items from same suit properly. */
  jacket = ClothingFindBest(ts, human, N("men-s-suit-jacket"), NULL, NULL,
                            NULL);
  pants = ClothingFindBest(ts, human, N("men-s-suit-pants"), NULL, jacket,
                           NULL);

  tie = ClothingFindBest(ts, human, N("tie"), NULL, jacket, NULL);
  shirt = ClothingFindBest(ts, human, N("dress-shirt"), NULL, tie, NULL);
  shoe1 = ClothingFindBest(ts, human, N("Oxford-shoe"), NULL, NULL, NULL);
  shoe2 = ClothingFindPairedItem(ts, human, shoe1);
  sock1 = ClothingFindBest(ts, human, N("executive-length-sock"), NULL,
                           NULL, NULL);
  sock2 = ClothingFindPairedItem(ts, human, sock1);
  r = NULL;
  r = ClothingAddItem(ClothingFindBest(ts, human, N("men-s-overcoat"), NULL,
                                       NULL, NULL), r);
  r = ClothingAddItem(jacket, r);
  r = ClothingAddItem(shoe1, r);
  r = ClothingAddItem(shoe2, r);
  r = ClothingAddItem(tie, r);
  r = ClothingAddItem(ClothingFindBest(ts, human, N("belt"), NULL,
                                       NULL, NULL), r);
  r = ClothingAddItem(pants, r);
  r = ClothingAddItem(shirt, r);
  r = ClothingAddItem(sock1, r);
  r = ClothingAddItem(sock2, r);
  r = ClothingAddItem(ClothingFindBest(ts, human, N("undershirt"), NULL,
                                       NULL, NULL), r);
  r = ClothingAddItem(ClothingFindBest(ts, human, N("men-s-briefs"), NULL,
                                       NULL, NULL), r);
  return(r);
}

ObjList *ClothingMaleCasual(Ts *ts, Obj *human)
{
  Obj	*jacket, *pants, *shirt, *shoe1, *shoe2, *sock1, *sock2;
  ObjList	*r;

  jacket = ClothingFindBest(ts, human, N("jacket"), NULL, NULL, NULL);
  pants = ClothingFindBest(ts, human, N("pants"), NULL, jacket, NULL);

  shirt = ClothingFindBest(ts, human, N("shirt"), NULL, NULL, NULL);
  shoe1 = ClothingFindBest(ts, human, N("shoe"), NULL, NULL, NULL);
  shoe2 = ClothingFindPairedItem(ts, human, shoe1);
  sock1 = ClothingFindBest(ts, human, N("socks"), NULL, NULL, NULL);
  sock2 = ClothingFindPairedItem(ts, human, sock1);
  r = NULL;
  r = ClothingAddItem(ClothingFindBest(ts, human, N("men-s-overcoat"), NULL,
                                       NULL, NULL), r);
  r = ClothingAddItem(jacket, r);
  r = ClothingAddItem(shoe1, r);
  r = ClothingAddItem(shoe2, r);
  r = ClothingAddItem(ClothingFindBest(ts, human, N("belt"), NULL,
                                       NULL, NULL), r);
  r = ClothingAddItem(pants, r);
  r = ClothingAddItem(shirt, r);
  r = ClothingAddItem(sock1, r);
  r = ClothingAddItem(sock2, r);
  r = ClothingAddItem(ClothingFindBest(ts, human, N("undershirt"), NULL,
                                       NULL, NULL), r);
  r = ClothingAddItem(ClothingFindBest(ts, human, N("men-s-briefs"), NULL,
                                       NULL, NULL), r);
  return(r);
}

ObjList *ClothingMaleSportclothes(Ts *ts, Obj *human)
{
  Obj	*jacket, *pants, *shirt, *shoe1, *shoe2, *sock1, *sock2;
  ObjList	*r;

  jacket = ClothingFindBest(ts, human, N("men-s-sport-jacket"), NULL,
                            NULL, NULL);
  pants = ClothingFindBest(ts, human, N("men-s-dress-pants"), NULL,
                           jacket, NULL);

  shirt = ClothingFindBest(ts, human, N("dress-shirt"), NULL, jacket, NULL);
  shoe1 = ClothingFindBest(ts, human, N("Oxford-shoe"), NULL, NULL, NULL);
  shoe2 = ClothingFindPairedItem(ts, human, shoe1);
  sock1 = ClothingFindBest(ts, human, N("executive-length-sock"), NULL,
                           NULL, NULL);
  sock2 = ClothingFindPairedItem(ts, human, sock1);
  r = NULL;
  r = ClothingAddItem(ClothingFindBest(ts, human, N("men-s-overcoat"), NULL,
                                       NULL, NULL), r);
  r = ClothingAddItem(jacket, r);
  r = ClothingAddItem(shoe1, r);
  r = ClothingAddItem(shoe2, r);
  r = ClothingAddItem(ClothingFindBest(ts, human, N("belt"), NULL,
                                       NULL, NULL), r);
  r = ClothingAddItem(pants, r);
  r = ClothingAddItem(shirt, r);
  r = ClothingAddItem(sock1, r);
  r = ClothingAddItem(sock2, r);
  r = ClothingAddItem(ClothingFindBest(ts, human, N("undershirt"), NULL,
                                       NULL, NULL), r);
  r = ClothingAddItem(ClothingFindBest(ts, human, N("men-s-briefs"), NULL,
                                       NULL, NULL), r);
  return(r);
}

ObjList *ClothingMaleSportswear(Ts *ts, Obj *human)
{
  Obj	*pants, *shirt, *shoe1, *shoe2, *sock1, *sock2;
  ObjList	*r;

  shirt = ClothingFindBest(ts, human, N("T-shirt"), NULL, NULL, NULL);
  pants = ClothingFindBest(ts, human, N("gym-shorts"), NULL, shirt, NULL);
  shoe1 = ClothingFindBest(ts, human, N("sneakers"), NULL, NULL, NULL);
  shoe2 = ClothingFindPairedItem(ts, human, shoe1);
  sock1 = ClothingFindBest(ts, human, N("sweat-socks"), NULL, NULL, NULL);
  sock2 = ClothingFindPairedItem(ts, human, sock1);
  r = NULL;
  r = ClothingAddItem(shoe1, r);
  r = ClothingAddItem(shoe2, r);
  r = ClothingAddItem(pants, r);
  r = ClothingAddItem(shirt, r);
  r = ClothingAddItem(sock1, r);
  r = ClothingAddItem(sock2, r);
  r = ClothingAddItem(ClothingFindBest(ts, human, N("men-s-briefs"), NULL,
                                       NULL, NULL), r);
  return(r);
}

ObjList *ClothingFemaleChildCasual(Ts *ts, Obj *human)
{
  Obj		*jacket, *pants, *shirt, *shoe1, *shoe2, *sock1, *sock2;
  ObjList	*r;

  jacket = ClothingFindBest(ts, human, N("coat"), NULL, NULL, NULL);
  pants = ClothingFindBest(ts, human, N("pants"), NULL, jacket, NULL);

  shirt = ClothingFindBest(ts, human, N("shirt"), NULL, NULL, NULL);
  shoe1 = ClothingFindBest(ts, human, N("shoe"), NULL, NULL, NULL);
  shoe2 = ClothingFindPairedItem(ts, human, shoe1);
  sock1 = ClothingFindBest(ts, human, N("socks"), NULL, NULL, NULL);
  sock2 = ClothingFindPairedItem(ts, human, sock1);
  r = NULL;
  r = ClothingAddItem(jacket, r);
  r = ClothingAddItem(shoe1, r);
  r = ClothingAddItem(shoe2, r);
  r = ClothingAddItem(pants, r);
  r = ClothingAddItem(shirt, r);
  r = ClothingAddItem(sock1, r);
  r = ClothingAddItem(sock2, r);
  r = ClothingAddItem(ClothingFindBest(ts, human, N("women-s-briefs"), NULL,
                                       NULL, NULL), r);
  return(r);
}

/* todo: More outfits. */
ObjList *ClothingSelectOutfit(Ts *ts, Obj *human, Obj *script)
{
  Float	age;
  Obj	*sex;
  age = AgeOf(ts, human);
  if (age == FLOATNA) {
    Dbg(DBGCLOTHING, DBGDETAIL, "age of <%s> unknown", M(human));
  } else {
    Dbg(DBGCLOTHING, DBGDETAIL, "<%s> is %g years old", M(human), age);
  }
  while (ObjIsList(script)) script = I(script, 0);
  sex = DbGetEnumValue(ts, NULL, N("sex"), human, N("male"));
  if (ISA(N("business-script"), script)) {
    if (ISA(N("male-like"), sex)) return(ClothingMaleBusiness(ts, human));
  }
  if (ISA(N("play-sport"), script)) {
    if (ISA(N("male-like"), sex)) return(ClothingMaleSportswear(ts, human));
  }
  if (ISA(N("male-like"), sex)) return(ClothingMaleCasual(ts, human));
  if (ISA(N("female-like"), sex)) {
    if (age != FLOATNA && age < 12.0) {
      return(ClothingFemaleChildCasual(ts, human));
    }
  }
  Dbg(DBGCLOTHING, DBGBAD, "No outfit found for script <%s>", M(script));
  return(NULL);
}

/* End of file. */
