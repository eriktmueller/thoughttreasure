/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 */

#include "tt.h"
#include "lexentry.h"
#include "repdb.h"
#include "repobj.h"
#include "repobjl.h"
#include "uascript.h"

Bool IsScript(Obj *obj)
{
  return ISA(N("action"), obj) &&
         RE(&TsNA, L(N("event01-of"), obj, ObjWild, E));
}

ObjList *ScriptGetAll()
{
  Obj *obj;
  ObjList *r;
  r = NULL;
  for (obj = Objs; obj; obj = obj->next) {
    if (!ObjIsSymbol(obj)) continue;
    if (!ISA(N("concept"), obj)) continue;
    if (IsScript(obj)) {
      r = ObjListCreate(obj, r);
    }
  }
  return r;
}

ObjListList *ScriptGetAllWithConcepts()
{
  Obj *obj;
  ObjListList *r;
  r = NULL;
  for (obj = Objs; obj; obj = obj->next) {
    if (!ObjIsSymbol(obj)) continue;
    if (!ISA(N("concept"), obj)) continue;
    if (IsScript(obj)) {
      r = ObjListListCreate(ScriptSimilarityConcepts(obj), r);
      r->obj = obj;
    }
  }
  return r;
}

ObjList *ScriptSimilarityAssertion(Obj *script, Obj *rel, ObjList *cons)
{
  int i, len;
  Obj *obj;
  ObjList *objs, *p;
  objs = RD(&TsNA, L(rel, script, ObjWild, E), 0);
  for (p = objs; p; p = p->next) {
    obj = I(p->obj, 2);
    len = ObjLen(obj);
    for (i = 0; i < len; i++) {
      cons = ObjListAddIfNotIn(cons, I(obj, i));
    }
  }
  return cons;
}

ObjList *ScriptSimilarityRelation(Obj *script, Obj *rel, ObjList *cons)
{
  ObjList *objs, *p;
  objs = RD(&TsNA, L(rel, script, ObjWild, E), 0);
  for (p = objs; p; p = p->next) {
    cons = ObjListAddIfNotIn(cons, I(p->obj, 2));
  }
  return cons;
}

ObjList *ScriptSimilarityConcepts(Obj *script)
{
  ObjList *cons;
  cons = NULL;
  cons = ObjListCreate(script, cons);
  cons = ScriptSimilarityAssertion(script, N("event-assertion-relation"), cons);
  cons = ScriptSimilarityAssertion(script, N("entry-condition-of"), cons);
  cons = ScriptSimilarityAssertion(script, N("result-of"), cons);
  cons = ScriptSimilarityAssertion(script, N("goal-of"), cons);
  cons = ScriptSimilarityAssertion(script, N("emotion-of"), cons);
  cons = ScriptSimilarityRelation(script, N("institution-of"), cons);
  cons = ScriptSimilarityRelation(script, N("performed-in"), cons);
  cons = ScriptSimilarityRelation(script, N("related-concept-of"), cons);
  return cons;
}

Float ScriptSimilarity(FILE *stream, Obj *script, ObjList *cons, Obj *con)
{
  Float   numer, denom, t;
  ObjList *p;
  numer = denom = 0.0;
  for (p = cons; p; p = p->next) {
    t = ObjPathLengthSimilarity(con, p->obj);
    if (t > 0.0) fprintf(stream, " %s", M(p->obj));
    numer += t;
    denom += 1.0;
  }
  if (denom == 0.0) return 0.0;
  /* todo: Is the average the right metric? */
  return numer/denom;
}

void ScriptPrintPolysemous(FILE *stream, int lang)
{
  ObjListList *poly, *p0;
  ObjList     *scripts, *p1, *p2;
  Float       ss;
  poly = LexEntryPolysemous(lang);
  scripts = ScriptGetAllWithConcepts();
  for (p0 = poly; p0; p0 = p0->next) {
    fprintf(stream, "========== LEXICAL ENTRY <%s> ==========\n", M(p0->obj));
    for (p1 = p0->u.objs; p1; p1 = p1->next) {
      fprintf(stream, "MEANING <%s>:", M(p1->obj));
      for (p2 = scripts; p2; p2 = p2->next) {
        ss = ScriptSimilarity(stream, p2->obj, p2->u.objs, p1->obj);
        if (ss > 0.0) {
          fprintf(stream, " %s %g", M(p2->obj), ss);
        }
      }
      fputc(NEWLINE, stream);
    }
  }
  ObjListFree(scripts);
}

/* End of file. */
