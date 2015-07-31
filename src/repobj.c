/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19920918: started old cdr-coded lists
 * 19920919: continued old lists
 * 19920929: old unifier
 * 19920930: old instantiation
 * 19921007: old prover
 * 19921009: old pretty printer and rule trace in prover
 * 19921010: rule trace in old prover
 * 19940419: new objects begun
 * 19950330: added Tsr objects
 * 19951027: added pn_list to objects; cosmetic changes to code
 * 19951111: reorganized Obj from 108 bytes into 76 nicer bytes
 */

#include "tt.h"
#include "lexentry.h"
#include "lexobjle.h"
#include "repbasic.h"
#include "repcxt.h"
#include "repdb.h"
#include "repgrid.h"
#include "repmisc.h"
#include "repobj.h"
#include "repobjl.h"
#include "repspace.h"
#include "repstr.h"
#include "reptext.h"
#include "reptime.h"
#include "reptrip.h"
#include "semparse.h"
#include "synpnode.h"
#include "toolsvr.h"
#include "utildbg.h"
#include "utilhtml.h"

HashTable *ObjHash;
Obj *ObjWild, *ObjNA, *Objs, *OBJDEFER;
Bool IncreaseMsg;
long ObjParentLinkCnt;

/******************************************************************************
 * INITIALIZATION
 ******************************************************************************/

void ObjInit()
{
  Dbg(DBGOBJ, DBGDETAIL, "ObjInit", E);
  IncreaseMsg = 0;
  ObjParentLinkCnt = 0;
  Objs = NULL;
  ObjHash = HashTableCreate(30001L);
  ObjWild = NameToObj("?", OBJ_CREATE_A);
  ObjNA = N("na");
  ObjAddIsa(ObjWild, NameToObj("concept", OBJ_CREATE_A));
  OBJDEFER = NameToObj("OBJDEFER", OBJ_CREATE_A);
}

/******************************************************************************
 * CREATING
 ******************************************************************************/

Obj *ObjCreateRawNonlist()
{
  Obj *obj;
  obj = CREATE(Obj);
  obj->u1.nlst.numparents = obj->u1.nlst.numchildren = 0;
  obj->u1.nlst.maxparents = obj->u1.nlst.maxchildren = 0;
  obj->u1.nlst.parents = obj->u1.nlst.children = NULL;
  obj->u2.any = NULL;
  obj->next = Objs;
  if (Objs) Objs->prev = obj;
  obj->prev = NULL;
  obj->ole = NULL;
  Objs = obj;
  return(obj);
}

Obj *ObjCreateRawList(int force_malloc)
{
  Obj *obj;
  obj = CREAT(Obj, force_malloc);
  obj->u1.lst.asserted = 0;
  obj->u1.lst.pn_list = NULL;
  obj->u1.lst.justification = NULL;
  obj->u1.lst.superseded_by = NULL;
  TsRangeSetNa(&obj->u2.tsr);
  obj->next = Objs;
  if (Objs) Objs->prev = obj;
  obj->prev = NULL;
  obj->ole = NULL;
  Objs = obj;
  return(obj);
}

void ObjFindUnusedName(Obj *parent, char *prefix,
                       /* RESULTS */ char *name, int *is_le0)
{
  char	name1[OBJNAMELEN];
  if (name[0] == TERM) {
    if (is_le0) *is_le0 = 0;
    if (prefix && parent) {
      sprintf(name, "%s-%s", prefix, ObjToName(parent));
    } else if (parent) {
      StringGenSym(name, OBJNAMELEN, ObjToName(parent), NULL);
    } else {
      StringGenSym(name, OBJNAMELEN, "gensym", NULL);
    }
  }
  StringCpy(name1, name, OBJNAMELEN);
  while (1) {
    if (NULL == NameToObj(name1, OBJ_NO_CREATE)) break;
    StringGenSym(name1, OBJNAMELEN, name, NULL);
    if (is_le0) *is_le0 = 0;
  }
  StringCpy(name, name1, OBJNAMELEN);
}

/* Example:
 * parent: N("media-object-book")
 * text:   "Principles of Artificial Intelligence"
 * create_flag: OBJ_CREATE_C
 */
Obj *ObjCreateInstanceText(Obj *parent, char *text, int existing_ok,
                           int create_flag, /* RESULTS */ int *is_le0,
                           int *existing)
{
  Obj		*obj;
  char		name[PHRASELEN];
  if (is_le0) *is_le0 = 1;
  LexEntryToObjName(text, name, 0, 0);
  if (existing_ok) {
    if ((obj = NameToObj(name, OBJ_NO_CREATE))) {
    /* <name> already exists. */
      if (existing) *existing = 1;
      if (!ISA(parent, obj)) ObjAddIsa(obj, parent);
      return(obj);
    }
  } else {
    ObjFindUnusedName(parent, NULL, name, is_le0);
  }
  /* <name> doesn't exist yet */
  if (existing) *existing = 0;
  obj = NameToObj(name, create_flag);
  ObjAddIsa(obj, parent);
  return(obj);
}

/* Example:
 * parent: N("media-object-book")
 * name:   "Principles-of-Artificial-Intelligence"
 * create_flag: OBJ_CREATE_C
 */
Obj *ObjCreateInstanceNamed(Obj *parent, char *name, int create_flag)
{
  Obj		*obj;
  ObjFindUnusedName(parent, NULL, name, NULL);
  obj = NameToObj(name, create_flag);
  ObjAddIsa(obj, parent);
  return(obj);
}

Obj *ObjCreateInstance(Obj *parent, char *prefix)
{
  Obj *child;
  char name[OBJNAMELEN];

  name[0] = TERM;
  ObjFindUnusedName(parent, prefix, name, NULL);
  Dbg(DBGOBJ, DBGDETAIL, "creating concrete instance <%s>", name);
  child = NameToObj(name, OBJ_CREATE_C);
  ObjAddIsa(child, parent);
  return(child);
}

Obj *ObjCreateList(Obj *elem0, ...)
{
  va_list ap;
  int i, len;
  Obj *elems[MAXLISTLEN];
  Obj *elem, *obj, **list;
  Dbg(DBGOBJ, DBGHYPER, "ObjCreateList");
  if (elem0 == NULL) {
    len = 0;
  } else {
    if (!ObjIsOK(elem0)) {
      elem = elem0;
      goto bad;
    }
#ifdef SUNOS
    va_start(ap);
#else
    va_start(ap, elem0);
#endif
    elems[0] = elem0;
    for (len = 1; ; len++) {
      elem = (Obj *)va_arg(ap, Obj *);
      if (elem == NULL) break;
      if (!ObjIsOK(elem)) goto bad;
      if (len >= MAXLISTLEN) {
        if (!IncreaseMsg) {
          Dbg(DBGOBJ, DBGBAD, "ObjCreateList: increase MAXLISTLEN");
        }
        IncreaseMsg = 1;
        Stop();
        goto out;
      }
      elems[len] = elem;
    }
    va_end(ap);
  }
out:
  obj = ObjCreateRawList(0);
  obj->type = OBJTYPELIST;
  obj->u1.lst.len = len;
  list = (Obj **)MemAlloc(len*sizeof(Obj *), "Obj* list");
  for (i = 0; i < len; i++) list[i] = elems[i];
  obj->u1.lst.list = list;
  return(obj);
bad:
  if (elem->type == OBJTYPEDESTROYED) {
    Dbg(DBGOBJ, DBGBAD, "ObjCreateList: element %d destroyed", len);
  } else {
    Dbg(DBGOBJ, DBGBAD, "ObjCreateList: element %d bad", len);
  }
  Stop();
  goto out;
}

Obj *ObjCreateList1(Obj **elems, int len)
{
  int i;
  Obj *obj, **list;
  obj = ObjCreateRawList(0);
  obj->type = OBJTYPELIST;
  obj->u1.lst.len = len;
  list = (Obj **)MemAlloc(len*sizeof(Obj *), "Obj* list");
  for (i = 0; i < len; i++) list[i] = elems[i];
  obj->u1.lst.list = list;
  return(obj);
}

PNode *PNodeCheck(PNode *pn)
{
  if (pn && PNodeIsOK(pn)) return(pn);
  else return(NULL);
}

Obj *ObjCheck(Obj *obj)
{
  if (obj && ObjIsOK(obj)) return(obj);
  else return(NULL);
}

Obj *ObjCreateListPNode1(Obj **elems, PNode **pn_elems, int len)
{
  int	i;
  Obj	*obj, **list;
  PNode	**pn_list;
  obj = ObjCreateRawList(0);
  obj->type = OBJTYPELIST;
  obj->u1.lst.len = len;
  list = (Obj **)MemAlloc(len*sizeof(Obj *), "Obj* list");
  pn_list = (PNode **)MemAlloc(len*sizeof(PNode *), "PNode* list");
  for (i = 0; i < len; i++) {
    list[i] = elems[i];
    pn_list[i] = PNodeCheck(pn_elems[i]);
  }
  obj->u1.lst.list = list;
  obj->u1.lst.pn_list = pn_list;
  return(obj);
}

int ObjCreateFlagToDbFileChar(int create_flag)
{
  if (create_flag == 1) return(TREE_LEVEL);
  else if (create_flag == 2) return(TREE_LEVEL_CONCRETE);
  else if (create_flag == 3) return(TREE_LEVEL_CONTRAST);
  else return('?');
}

int ObjCreateFlag(Obj *obj)
{
  if (obj->type == OBJTYPEASYMBOL) return(OBJ_CREATE_A);
  else if (obj->type == OBJTYPEACSYMBOL) return(OBJ_CREATE_AC);
  else if (obj->type == OBJTYPECSYMBOL) return(OBJ_CREATE_C);
  else return(OBJ_CREATE_A);
}

/* OBJ_NO_CREATE = do not create if nonexistent
 * OBJ_CREATE_A = create as abstract object if nonexistent
 * OBJ_CREATE_C = create as concrete object if nonexistent
 * OBJ_CREATE_AC = create as abstract contrast object if nonexistent
 */
Obj *NameToObj(char *name, int flag)
{
  Obj *obj;

  if ((obj = (Obj *)HashTableGet(ObjHash, name))) return(obj);
  if (flag == OBJ_NO_CREATE) return(NULL);
  Dbg(DBGOBJ, DBGHYPER, "creating <%s>", name);
  obj = ObjCreateRawNonlist();
  obj->u1.nlst.name = StringCopy(name, "char Obj name");
  HashTableSet(ObjHash, obj->u1.nlst.name, obj);
  if (flag == OBJ_CREATE_AC) obj->type = OBJTYPEACSYMBOL;
  else if (flag == OBJ_CREATE_C) obj->type = OBJTYPECSYMBOL;
  else obj->type = OBJTYPEASYMBOL;
  return(obj);
}

Obj *StringToObj(char *s, Obj *parent, int use_string_as_name)
{
  Obj	*obj;
  char	name[PHRASELEN];
  if (s == NULL) s = "";
  if (use_string_as_name) {
    StringCpy(name, s, PHRASELEN);
  } else {
    name[0] = TERM;
  }
  ObjFindUnusedName(parent, "string", name, NULL);
  if ((obj = NameToObj(name, OBJ_CREATE_C))) {
    obj->type = OBJTYPESTRING;
    obj->u2.s = HashTableIntern(EnglishIndex, s);
    if (parent) ObjAddIsa1(obj, parent);
    return(obj);
  }
  Dbg(DBGTREE, DBGBAD, "StringToObj");
  return(NULL);
}

Obj *NumberToObj(Float number)
{
  Obj *obj;
  Dbg(DBGOBJ, DBGHYPER, "NumberToObj", E);
  obj = ObjCreateRawNonlist();
  obj->u1.nlst.name = "a-number";
  obj->type = OBJTYPENUMBER;
  obj->u2.number = number;
  return(obj);
}

Obj *NumberToObjClass(Float number, Obj *parent)
{
  Obj	*obj;
  obj = NumberToObj(number);
  if (parent) ObjAddIsa1(obj, parent);
  return(obj);
}

Obj *GridSubspaceToObj(GridSubspace *gr)
{
  Obj *obj;
  obj = ObjCreateRawNonlist();
  obj->u1.nlst.name = "a-gridsubspace";
  obj->type = OBJTYPEGRIDSUBSPACE;
  obj->u2.gridsubspace = gr;
  return(obj);
}

Obj *TripLegToObj(TripLeg *gr)
{
  Obj *obj;
  Dbg(DBGOBJ, DBGHYPER, "TripLegToObj", E);
  obj = ObjCreateRawNonlist();
  obj->u1.nlst.name = "a-tripleg";
  obj->type = OBJTYPETRIPLEG;
  obj->u2.tripleg = gr;
  return(obj);
}

Obj *TsRangeToObj(TsRange *tsr)
{
  Obj *obj;
  obj = ObjCreateRawNonlist();
  obj->u1.nlst.name = "a-tsr";
  obj->type = OBJTYPETSR;
  obj->u2.tsr = *tsr;
  return(obj);
}

Obj *HumanNameToObj(Name *nm)
{
  Obj *obj;
  obj = ObjCreateRawNonlist();
  obj->u1.nlst.name = "a-name";
  obj->type = OBJTYPENAME;
  obj->u2.nm = nm;
  return(obj);
}

Obj *ObjAtGridCreate(Obj *obj, Obj *grid, GridCoord row, GridCoord col)
{
  Grid	*ingr;
  GridSubspace	*gs;
  if (NULL == grid) {
    Dbg(DBGOBJ, DBGBAD, "ObjAt: NULL grid", M(grid));
    return(L(N("at-grid"), obj, ObjWild, ObjWild, E));
  } else if (NULL == (ingr = ObjToGrid(grid))) {
    Dbg(DBGOBJ, DBGBAD, "ObjAt: <%s> not a grid", M(grid));
    return(L(N("at-grid"), obj, ObjWild, ObjWild, E));
  }
  gs = GridSubspaceCreate(1, ingr);
  gs->len = 1;
  gs->rows[0] = row;
  gs->cols[0] = col;
  return(L(N("at-grid"), obj, grid, GridSubspaceToObj(gs), E));
}

/******************************************************************************
 * COPYING
 ******************************************************************************/

Obj *ObjCopyList(Obj *old)
{
  int	i, len;
  Obj	*new, **list;
  PNode **pn_list;
  if (old == NULL) return(NULL);
  if (old->type != OBJTYPELIST) {
    Dbg(DBGOBJ, DBGBAD, "ObjCopyList: nonlist <%s>", M(old));
    return(L(ObjNA, E));
  }
  new = ObjCreateRawList(1);
  new->type = OBJTYPELIST;
  len = old->u1.lst.len;
  new->u1.lst.len = len;
  list = (Obj **)MemAlloc1(len*sizeof(Obj *), "Obj* list", 1);
  if (old->u1.lst.pn_list) {
    pn_list = (PNode **)MemAlloc1(len*sizeof(PNode *), "PNode* list", 1);
    for (i = 0; i < len; i++) {
      list[i] = old->u1.lst.list[i];
      pn_list[i] = old->u1.lst.pn_list[i];
    }
    new->u1.lst.pn_list = pn_list;
  } else {
    for (i = 0; i < len; i++) list[i] = old->u1.lst.list[i];
  }
  new->u1.lst.list = list;
  new->u2.tsr = old->u2.tsr;
  return(new);
}

/* todo: This is nondeep. Shouldn't it be replaced by the deep version? */
Obj *ObjCopyListWithSubst(Obj *old, Obj *from, Obj *to)
{
  int	i, len;
  Obj	*new, **list;
  PNode **pn_list;
  if (old->type != OBJTYPELIST) {
    Dbg(DBGOBJ, DBGBAD, "ObjCopyListWithSubst: nonlist <%s>", M(old));
    return(NULL);
  }
  new = ObjCreateRawList(0);
  new->type = OBJTYPELIST;
  len = old->u1.lst.len;
  new->u1.lst.len = len;
  list = (Obj **)MemAlloc(len*sizeof(Obj *), "Obj* list");
  if (old->u1.lst.pn_list) {
    pn_list = (PNode **)MemAlloc1(len*sizeof(PNode *), "PNode* list", 1);
    for (i = 0; i < len; i++) {
      if (old->u1.lst.list[i] == from) list[i] = to;
      else list[i] = old->u1.lst.list[i];
      pn_list[i] = old->u1.lst.pn_list[i];
    }
    new->u1.lst.pn_list = pn_list;
  } else {
    for (i = 0; i < len; i++) {
      if (old->u1.lst.list[i] == from) list[i] = to;
      else list[i] = old->u1.lst.list[i];
    }
  }
  new->u1.lst.list = list;
  new->u2.tsr = old->u2.tsr;
  return(new);
}

Obj *ObjGridCopy(Obj *old, char *name)
{
  Obj	*new;
  new = ObjCreateInstanceNamed(N("grid"), name, OBJ_CREATE_C);
  if (old->type == OBJTYPEGRID) {
    new->u2.grid = GridCopy(old->u2.grid);
    new->u2.grid->grid = new;
    new->type = OBJTYPEGRID;
  } else {
    Dbg(DBGOBJ, DBGBAD, "ObjCopyGrid: nongrid <%s>", M(old));
  } 
  return(new);
}

Obj *ObjGridSubspaceCopy(Obj *old_obj)
{
  GridSubspace	*old_gs, *new_gs;
  if (NULL == (old_gs = ObjToGridSubspace(old_obj))) {
    Dbg(DBGOBJ, DBGBAD, "ObjGridSubspaceCopy <%s>", M(old_obj));
    return(NULL);
  }
  new_gs = GridSubspaceCopyShared(old_gs);
  return(GridSubspaceToObj(new_gs));
}

/******************************************************************************
 * FREEING
 ******************************************************************************/

/* Currently this is only for lists. */
void ObjFree(Obj *obj)
{
/*
  fputs("****FREEING ", Log);
  ObjPrint1(Log, obj, NULL, 5, 1, 0, 1, 0);
  fputc(NEWLINE, Log);
 */

  if (obj->type != OBJTYPELIST || obj->ole) {
    Dbg(DBGOBJ, DBGBAD, "ObjFree: unable to free <%s>", M(obj));
    Stop();
    return;
  }
  if (obj == Objs) {
    Objs = obj->next;
    if (obj->next) obj->next->prev = NULL;
  } else if (obj->prev) {
    obj->prev->next = obj->next;
    if (obj->next) obj->next->prev = obj->prev;
  }
  MemFree(obj->u1.lst.list, "Obj* list");
  if (obj->u1.lst.pn_list) {
    MemFree(obj->u1.lst.pn_list, "PNode* list");
  }
  obj->type = OBJTYPEDESTROYED;
  MemFree(obj, "Obj");
}

/******************************************************************************
 * ACCESSORS
 ******************************************************************************/

char *ObjToName(Obj *obj)
{
  if (obj == NULL) return("");
  if (!ObjIsOK(obj)) return("");
  if (obj->type == OBJTYPELIST) return("a-list");
  return(obj->u1.nlst.name);
}

char *ObjToString(Obj *obj)
{
  if (obj == NULL) return("");
  if (obj->type != OBJTYPESTRING) {
    if (obj->type != OBJTYPELIST) return(obj->u1.nlst.name);
    return("");
  }
  return(obj->u2.s);
}

Obj *ObjToStringClass(Obj *obj)
{
  Obj	*r;
  if (obj == NULL) return(NULL);
  if (obj->type != OBJTYPESTRING) return(NULL);
  if ((r = ObjGetAParent(obj))) return(r);
  return(N("string"));
}

Float ObjToNumber(Obj *obj)
{
  if (obj == NULL) return(FLOATNA);
  if (obj->type != OBJTYPENUMBER) return(FLOATNA);
  return(obj->u2.number);
}

Dur ObjToDur(Obj *obj, Dur def)
{
  if (obj == NULL) return(def);
  if (obj->type != OBJTYPENUMBER) return(def);
  return((Dur)obj->u2.number);
}

Obj *ObjToNumberClass(Obj *obj)
{
  Obj	*r;
  if (obj == NULL) return(NULL);
  if (obj->type != OBJTYPENUMBER) return(NULL);
  if ((r = ObjGetAParent(obj))) return(r);
  return(N("u"));
}

Grid *ObjToGrid(Obj *obj)
{
  if (obj == NULL) return(NULL);
  if (obj->type != OBJTYPEGRID) return(NULL);
  return(obj->u2.grid);
}

GridSubspace *ObjToGridSubspace(Obj *obj)
{
  if (obj == NULL) return(NULL);
  if (obj->type != OBJTYPEGRIDSUBSPACE) return(NULL);
  return(obj->u2.gridsubspace);
}

TripLeg *ObjToTripLeg(Obj *obj)
{
  if (obj == NULL) return(NULL);
  if (obj->type != OBJTYPETRIPLEG) return(NULL);
  return(obj->u2.tripleg);
}

TsRange *ObjToTsRange(Obj *obj)
{
  TsRange	*tsr;
  if (obj == NULL ||
      (obj->type != OBJTYPELIST && obj->type != OBJTYPETSR)) {
  /* todo */
    tsr = CREATE(TsRange);
    TsRangeSetAlways(tsr);
    return(tsr);
  }
  return(&obj->u2.tsr);
}

Name *ObjToHumanName(Obj *obj)
{
  if (obj == NULL) return(NULL);
  if (obj->type != OBJTYPENAME) return(NULL);
  return(obj->u2.nm);
}

ObjList *ObjToJustification(Obj *obj)
{
#ifdef maxchecking
  if (obj->type != OBJTYPELIST) {
    Dbg(DBGOBJ, DBGBAD, "ObjToJustification");
    return(NULL);
  }
#endif
  return(obj->u1.lst.justification);
}

ObjList *ObjToSuperseded(Obj *obj)
{
#ifdef maxchecking
  if (obj->type != OBJTYPELIST) {
    Dbg(DBGOBJ, DBGBAD, "ObjToSuperseded");
    return(NULL);
  }
#endif
  return(obj->u1.lst.superseded_by);
}

Bool ObjSupersededIn(Obj *obj, Context *cx)
{
  ObjList  *p;
  for (p = ObjToSuperseded(obj); p; p = p->next) {
    if (ContextIsAncestor(p->obj->u2.tsr.cx, cx)) return 1;
  }
  return 0;
}

Bool ObjIsOK1(Obj *obj)
{
  if (!ObjTypeOK(obj->type)) {
    Dbg(DBGTREE, DBGBAD, "bad Obj type %d", obj->type);
    return(0);
  }
  if (obj->type == OBJTYPELIST) {
    if (obj->u1.lst.asserted != 0 && obj->u1.lst.asserted != 1) {
      Dbg(DBGTREE, DBGBAD, "bad Obj list asserted");
      return(0);
    }
    if (obj->u1.lst.len < 0) {
      Dbg(DBGTREE, DBGBAD, "bad Obj list length");
      return(0);
    }
  } else {
    if (obj->u1.nlst.numparents < 0 || obj->u1.nlst.numchildren < 0) {
      Dbg(DBGTREE, DBGBAD, "bad Obj numparents or numchildren");
      return(0);
    }
  }
  return(1);
}

Bool ObjIsOK(Obj *obj)
{
  if (!ObjIsOK1(obj)) {
    Dbg(DBGTREE, DBGBAD, "bad Obj");
    Stop();
    return(0);
  }
  return(1);
}

Bool ObjIsOKDeep(Obj *obj)
{
  int	i, len;
  if (!ObjIsOK(obj)) return(0);
  for (i = 0, len = ObjLen(obj); i < len; i++) {
    if (!ObjIsOKDeep(I(obj, i))) return(0);
  }
  return(1);
}

int ook(Obj *obj)
{
  return(ObjIsOK1(obj));
}

int ObjNumParents(Obj *obj)
{
  if (obj->type == OBJTYPELIST) return(0);
  return(obj->u1.nlst.numparents);
}

Obj *ObjIthParent(Obj *obj, int i)
{
  if (obj->type == OBJTYPELIST) return(NULL);
  return(obj->u1.nlst.parents[i]);
}

int ObjNumChildren(Obj *obj)
{
  if (obj->type == OBJTYPELIST) return(0);
  return(obj->u1.nlst.numchildren);
}

Obj *ObjIthChild(Obj *obj, int i)
{
  if (obj->type == OBJTYPELIST) return(NULL);
  return(obj->u1.nlst.children[i]);
}

Bool ObjIsParent(Obj *parent, Obj *child)
{
  int i;
  if (child->type == OBJTYPELIST) return(0);
  for (i = 0; i < child->u1.nlst.numparents; i++) {
    if (parent == child->u1.nlst.parents[i]) return(1);
  }
  return(0);
}

Obj *ObjGetUniqueParent(Obj *obj)
{
  if (obj->type == OBJTYPELIST) return(NULL);
  if (obj->u1.nlst.numparents != 1) {
    Dbg(DBGOBJ, DBGBAD, "no unique parent found for <%s>", M(obj));
    return(NULL);
  }
  return(obj->u1.nlst.parents[0]);
}

Obj *ObjGetUniqueParent1(Obj *obj)
{
  if (obj->type == OBJTYPELIST) return(NULL);
  if (obj->u1.nlst.numparents != 1) {
    return(NULL);
  }
  return(obj->u1.nlst.parents[0]);
}

Obj *ObjGetAParent(Obj *obj)
{
  if (obj->type == OBJTYPELIST) return(NULL);
  if (obj->u1.nlst.numparents == 0) return(NULL);
  return(obj->u1.nlst.parents[0]);
}

Bool ObjHasCommonParent(Obj *obj1, Obj *obj2)
{
  ObjList *parents1, *parents2, *p;
  parents1 = ObjParents(obj1, OBJTYPEANY);
  parents2 = ObjParents(obj2, OBJTYPEANY);
  if (ObjListIn(obj1, parents2)) return 1;
  if (ObjListIn(obj2, parents1)) return 1;
  for (p = parents1; p; p = p->next) {
    if (ObjListIn(p->obj, parents2)) return 1;
  }
  return 0;
}

Obj *ObjIth(Obj *obj, int i)
{
  if (obj == NULL || obj->type != OBJTYPELIST ||
      i >= obj->u1.lst.len || i < 0) {
    return(NULL);
  }
  return(obj->u1.lst.list[i]);
}

PNode *ObjIthPNode(Obj *obj, int i)
{
  if (obj == NULL || obj->type != OBJTYPELIST ||
      i >= obj->u1.lst.len || i < 0 || obj->u1.lst.pn_list == NULL) {
    return(NULL);
  }
  return(obj->u1.lst.pn_list[i]);
}

Bool ObjIsForLexEntry(Obj *obj, LexEntry *le)
{
  ObjToLexEntry	*ole;
  for (ole = obj->ole; ole; ole = ole->next) {
    if (ole->le == le) return(1);
  }
  return(0);
}

/******************************************************************************
 * MODIFIERS
 ******************************************************************************/

void ObjMakeConcrete(Obj *obj)
{
  if (obj->type == OBJTYPECSYMBOL) return;
  Dbg(DBGTREE, DBGHYPER, "making <%s> concrete", M(obj));
  obj->type = OBJTYPECSYMBOL;
}

void ObjMakeAbstract(Obj *obj)
{
  if (obj->type == OBJTYPEASYMBOL) return;
  Dbg(DBGTREE, DBGBAD, "making <%s> abstract", M(obj));
  obj->type = OBJTYPEASYMBOL;
}

/* obj1's tsr <--- obj2's tsr */
void ObjTsRangeSetFrom(Obj *obj1, Obj *obj2)
{
  TsRange	*tsr1, *tsr2;
  tsr1 = ObjToTsRange(obj1);
  tsr2 = ObjToTsRange(obj2);
  *tsr1 = *tsr2;
}

Obj *ObjPastify(Obj *obj, Discourse *dc)
{
  if (!ObjContainsTsRange(obj)) return(obj);
  TsSetNegInf(&obj->u2.tsr.startts);
  obj->u2.tsr.stopts = *DCNOW(dc);
  return(obj);
}

void ObjSetTsRange(/* RESULTS */ Obj *obj, /* INPUT */ TsRange *tsr)
{
  if (!ObjContainsTsRange(obj)) return;
  obj->u2.tsr = *tsr;
}

void ObjSetTsRangeContext(/* RESULTS */ Obj *obj, /* INPUT */ Context *cx)
{
  if (!ObjContainsTsRange(obj)) return;
  TsRangeSetContext(&obj->u2.tsr, cx);
}

Obj *ObjTsAdd(Ts *ts, Obj *obj)
{
  if (!ObjContainsTsRange(obj)) return(obj);
  TsRangeSetNa(&obj->u2.tsr);
  obj->u2.tsr.startts = *ts;
  return(obj);
}

/* todo: Change this to ObjSetTsStartStop. */
Obj *ObjTsStartStop(Ts *ts, Obj *obj)
{
  if (!ObjContainsTsRange(obj)) return(obj);
  TsRangeSetNa(&obj->u2.tsr);
  obj->u2.tsr.startts = *ts;
  obj->u2.tsr.stopts = *ts;
  return(obj);
}

Obj *ObjTsRangeAdd(TsRange *tsr, Obj *obj)
{
  if (!ObjContainsTsRange(obj)) return(obj);
  obj->u2.tsr = *tsr;
  return(obj);
}

/*
void ObjSetTsRangeDeep(Obj *obj, TsRange *tsr)
{
  int	i, len;
  if (!ObjContainsTsRange(obj)) return(obj);
  obj->u2.tsr = *tsr;
  for (i = 0, len = ObjLen(obj); i < len; i++) {
    ObjSetTsRangeDeep(I(obj, i), tsr);
  }
}
*/

void ObjSetJustification(Obj *obj, ObjList *justification)
{
#ifdef maxchecking
  if (obj->type != OBJTYPELIST) {
    Dbg(DBGOBJ, DBGBAD, "ObjSetJustification");
    return;
  }
#endif
  obj->u1.lst.justification = justification;
}

void ObjAppendJustification(Obj *obj, ObjList *justification)
{
#ifdef maxchecking
  if (obj->type != OBJTYPELIST) {
    Dbg(DBGOBJ, DBGBAD, "ObjAppendJustification");
    return;
  }
#endif
  obj->u1.lst.justification =
    ObjListAppendDestructive(obj->u1.lst.justification, justification);
}

void ObjAddParent(Obj *obj, Obj *parent)
{
  ObjParentLinkCnt++;
#ifdef maxchecking
  if (obj->type == OBJTYPELIST) {
    Dbg(DBGOBJ, DBGBAD, "ObjAddParent 0");
    return;
  }
#endif
  if (obj->u1.nlst.parents == NULL) {
#ifdef maxchecking
    if (obj->u1.nlst.numparents != 0 || obj->u1.nlst.maxparents != 0) {
      Dbg(DBGOBJ, DBGBAD, "ObjAddParent 1");
      return;
    }
#endif
    obj->u1.nlst.maxparents = 1;
    obj->u1.nlst.parents =
      (Obj **)MemAlloc(obj->u1.nlst.maxparents*sizeof(Obj *),
                       "Obj* parents");
  } else if (obj->u1.nlst.numparents >= obj->u1.nlst.maxparents) {
#ifdef maxchecking
    if (obj->u1.nlst.numparents > obj->u1.nlst.maxparents) {
      Dbg(DBGOBJ, DBGBAD, "ObjAddParent 2");
      return;
    }
#endif
    obj->u1.nlst.maxparents = 2*obj->u1.nlst.maxparents;
    obj->u1.nlst.parents =
      (Obj **)MemRealloc(obj->u1.nlst.parents,
                         obj->u1.nlst.maxparents*sizeof(Obj *),
                         "Obj* parents");
  }
  obj->u1.nlst.parents[obj->u1.nlst.numparents] = parent;
  obj->u1.nlst.numparents++;
}

void ObjAddChild(Obj *obj, Obj *child)
{
#ifdef maxchecking
  if (obj->type == OBJTYPELIST) {
    Dbg(DBGOBJ, DBGBAD, "ObjAddChild 0");
    return;
  }
#endif
  if (obj->u1.nlst.children == NULL) {
#ifdef maxchecking
    if (obj->u1.nlst.numchildren != 0 || obj->u1.nlst.maxchildren != 0) {
      Dbg(DBGOBJ, DBGBAD, "ObjAddChild 1");
      return;
    }
#endif
    obj->u1.nlst.maxchildren = 1;
    obj->u1.nlst.children =
      (Obj **)MemAlloc(obj->u1.nlst.maxchildren*sizeof(Obj *),
                       "Obj* children");
  } else if (obj->u1.nlst.numchildren >= obj->u1.nlst.maxchildren) {
#ifdef maxchecking
    if (obj->u1.nlst.numchildren > obj->u1.nlst.maxchildren) {
      Dbg(DBGOBJ, DBGBAD, "ObjAddChild 2");
      return;
    }
#endif
    obj->u1.nlst.maxchildren = 2*obj->u1.nlst.maxchildren;
    obj->u1.nlst.children = (Obj **)MemRealloc(obj->u1.nlst.children,
                               obj->u1.nlst.maxchildren*sizeof(Obj *),
                               "Obj* children");
  }
  obj->u1.nlst.children[obj->u1.nlst.numchildren] = child;
  obj->u1.nlst.numchildren++;
}

void ObjAddIsa1(Obj *obj, Obj *parent)
{
  Dbg(DBGOBJ, DBGHYPER, "ObjAddIsa <%s> <%s>", M(obj), M(parent), E);
  ObjAddParent(obj, parent);
  ObjAddChild(parent, obj);
}

void ObjAddIsa(Obj *obj, Obj *parent)
{
  if (ISA(parent, obj)) {
    Dbg(DBGOBJ, DBGBAD, "<%s> already has ancestor <%s>",
        M(obj), M(parent), E);
    return;
  }
  ObjAddIsa1(obj, parent);
}

/* Whoever calls this should be doing the Learning. */
Bool ObjReclassify(Obj *obj, Obj *fromparent, Obj *toparent)
{
  /* todo: Delete the fromparent<---->obj links. But actually this
   * may give more graceful behavior in the face of errors, as
   * in "Jim est qui?"
   */
  ObjAddIsa1(obj, toparent);
  return(1);
}

Bool ObjSetIth(Obj *obj, int i, Obj *value)
{
  int	j;
  if (obj->type != OBJTYPELIST || i < 0) return(0);
  if (i >= obj->u1.lst.len) {
  /* Old indices = 0 1, old length = 2.
   * New indices = 0 1 2 3 4 5, new i = 5, new length = 6.
   */
    if ((i+1) >= MAXLISTLEN) {
      Dbg(DBGOBJ, DBGBAD, "ObjSetIth: %d > MAXLISTLEN", i+1);
      return(0);
    }
    obj->u1.lst.list =
      (Obj **)MemRealloc(obj->u1.lst.list, (i+1)*sizeof(Obj *), "Obj* list");
    for (j = obj->u1.lst.len; j < i; j++) {
      obj->u1.lst.list[j] = ObjNA;
    }
    obj->u1.lst.len = i+1;
  }
  obj->u1.lst.list[i] = value;
  return(1);
}

Bool ObjSetIthPNode(Obj *obj, int i, PNode *value)
{
  int	j, len;
  if (obj->type != OBJTYPELIST || i >= obj->u1.lst.len || i < 0) {
    return(0);
  }
  if (obj->u1.lst.pn_list == NULL) {
    obj->u1.lst.pn_list =
      (PNode **)MemAlloc(obj->u1.lst.len*sizeof(PNode *), "PNode* list");
    for (j = 0, len = obj->u1.lst.len; j < len; j++) {
      obj->u1.lst.pn_list[j] = NULL;
    }
  }
  obj->u1.lst.pn_list[i] = PNodeCheck(value);
  return(1);
}

/******************************************************************************
 * ISA
 ******************************************************************************/

Bool ISA1(Obj *anc, Obj *des, int depth)
{
  int i;
#ifdef maxchecking
  if (!ObjIsOK(anc)) {
    return(0);
  }
  if (!ObjIsOK(des)) {
    return(0);
  }
#endif
  if (anc == des) return(1);
  if (depth == 0) return(0);
  if (des->type == OBJTYPELIST) return(0);
  for (i = 0; i < des->u1.nlst.numparents; i++) {
    if (ISA1(anc, des->u1.nlst.parents[i], depth-1)) {
      return(1);
    }
  }
  return(0);
}

Bool ISA(Obj *anc, Obj *des)
{
  if ((!anc) || (!des)) return(0);
  return(ISA1(anc, des, MAXISADEPTH));
}

Bool ISAP(Obj *class, Obj *obj)
{
  int		i, len;
  Obj		*noun;
  ObjList	*props;
  if ((!class) || (!obj)) return(0);
  if (ISA(N("F68"), I(obj, 0))) {
  /* Determiner: "the cat" recurse on "cat" */
    return(ISAP(class, I(obj, 1)));
  }
  if (ObjIsList(class)) {
    if (I(class, 0) == N("or")) {
      for (i = 1, len = ObjLen(class); i < len; i++) {
        if (ISAP(I(class, i), obj)) return(1);
      }
      return(0);
    } else if (I(class, 0) == N("and")) {
      for (i = 1, len = ObjLen(class); i < len; i++) {
        if (!ISAP(I(class, i), obj)) return(0);
      }
      return(1);
    } else if (I(class, 0) == N("not")) {
      for (i = 1, len = ObjLen(class); i < len; i++) {
        if (ISAP(I(class, i), obj)) return(0);
      }
      return(1);
    }
    return(0);
  }
  if (obj == ObjNA) return(1);
  if (class == N("nonhuman")) return(!ISAP(N("human"), obj));
           /* todo: --> [not human] */
  if (class == N("location")) {
    return(ISAP(N("physical-object"), obj) ||
           ISAP(N("polity"), obj));
  }
  if (ObjIsVar(obj)) return(1);
  if (ISA(N("number"), class) && ObjIsNumber(obj)) return(1);
  if (ObjIsNumber(class) && ObjIsNumber(obj)) {
    return(ObjToNumber(class) == ObjToNumber(obj));
  }
  if (class == N("list")) return(ObjIsList(obj));
  if (class == N("string")) return(ObjIsString(obj));
  if (class == N("time-range") && ObjIsTsRange(obj)) return(1);
  if (class == N("concept")) return(1);
  if (ObjIsList(obj)) {
    if ((props = ObjIntensionParse(obj, 1, &noun))) {
    /* todo: Perhaps use props? For instance if props = [isa X C] */
      ObjListFree(props);
      return(ISAP(class, noun));
    } else if (N("and") == I(obj, 0)) {
      for (i = 0, len = ObjLen(obj); i < len; i++) {
        if (ISAP(class, I(obj, i))) {
          return(1);
        }
      }
      return(0); 
    } else {
    /* Recurse on proposition, so that
     * 1 == ISAP(N("action"), L(N("ptrans"), E))
     */
      return(ISAP(class, I(obj, 0)));
    }
  }
  return(ISA(class, obj));
}

Bool ISADeep(Obj *anc, Obj *obj)
{
  int	i, len;
  if (obj->type != OBJTYPELIST) return(ISA(anc, obj));
  for (i = 0, len = obj->u1.lst.len; i < len; i++) {
    if (ISADeep(anc, obj->u1.lst.list[i])) return(1);
  }
  return(0);
}

Bool ObjBarrierISA(Obj *obj)
{
  char	*name;
  ObjList *objs;
  name = ObjToName(obj);
  if (name[0] == ':' && name[1] == ':') return 1; /* todo: Mac-specific */
  if (StringIn('/', name)) return 1; /* todo: Unix-specific */
  objs = RE(&TsNA, L(N("barrier-isa"), obj, E));
  if (objs) {
    ObjListFree(objs);
    return 1;
  }
  ObjListFree(objs);
  return 0;
}

#ifdef notdef
Bool ObjIsAscriptiveAttachmentDeep(Obj *obj)
{
  int		i, len;
  ObjList	*objs;
  if (obj->type != OBJTYPELIST) {
    if ((objs = Sem_ParseGetAttachments(obj))) {
    /* todo: This isn't right. */
      ObjListFree(objs);
      return(1);
    }
    ObjListFree(objs);
    return(0);
  }
  for (i = 0, len = obj->u1.lst.len; i < len; i++) {
    if (ObjIsAscriptiveAttachmentDeep(obj->u1.lst.list[i])) return(1);
  }
  return(0);
}
#endif

ObjList *ObjCommonAncestors1(Obj *obj1, Obj *obj2, int depth, int maxdepth,
                             ObjList *r)
{
  int		i;
  if (obj1->type == OBJTYPELIST ||
      depth >= maxdepth ||
      ObjBarrierISA(obj1)) {
    ObjListFree(r);
    return(NULL);
  }
  for (i = 0; i < obj1->u1.nlst.numparents; i++) {
    if (ISA(obj1->u1.nlst.parents[i], obj2)) {
      r = ObjListCreate(obj1->u1.nlst.parents[i], r);
    } else {
      r = ObjCommonAncestors1(obj1->u1.nlst.parents[i], obj2, depth+1,
                              maxdepth, r);
    }
  }
  return(r);
}

ObjList *ObjCommonAncestors(Obj *obj1, Obj *obj2)
{
  return(ObjListUniquifyDest(ObjCommonAncestors1(obj1, obj2, 0, MAXISADEPTH,
                                                 NULL)));
}

ObjList *ObjDescendants1(Obj *obj, int type, int depth, int maxdepth,
                         ObjList *r)
{
  int	i;
  Obj	*obj1;
  if (obj->type == OBJTYPELIST) return(r);
  if (depth >= maxdepth) return(r);
  for (i = 0; i < obj->u1.nlst.numchildren; i++) {
    obj1 = obj->u1.nlst.children[i];
    if (type == OBJTYPEANY || obj1->type == type) {
      if (!ObjListIn(obj1, r)) {
        r = ObjListCreate(obj1, r);
      }
    }
    r = ObjDescendants1(obj1, type, depth+1, maxdepth, r);
  }
  return(r);
}

ObjList *ObjDescendants(Obj *obj, int type)
{
  return(ObjDescendants1(obj, type, 0, INTPOSINF, NULL));
}

ObjList *ObjChildren(Obj *obj, int type)
{
  return(ObjDescendants1(obj, type, 0, 1, NULL));
}

ObjList *ObjAncestors1(Obj *obj, int type, int depth, int maxdepth, ObjList *r)
{
  int	i;
  Obj	*obj1;
  if (obj->type == OBJTYPELIST) return(r);
  if (depth >= maxdepth) return(r);
  for (i = 0; i < obj->u1.nlst.numparents; i++) {
    obj1 = obj->u1.nlst.parents[i];
    if (obj1->u1.nlst.name[0] == ':') continue;
    if (type == OBJTYPEANY || obj1->type == type) r = ObjListCreate(obj1, r);
    r = ObjAncestors1(obj1, type, depth+1, maxdepth, r);
  }
  return(r);
}

ObjList *ObjAncestors(Obj *obj, int type)
{
  return(ObjAncestors1(obj, type, 0, INTPOSINF, NULL));
}

ObjList *ObjAncestorsOfDepth1(Obj *obj, int type, int desired_depth,
                              int depth, int maxdepth, ObjList *r)
{
  int	i;
  Obj	*obj1;
  if (obj->type == OBJTYPELIST) return(r);
  if (depth >= maxdepth) return(r);
  for (i = 0; i < obj->u1.nlst.numparents; i++) {
    obj1 = obj->u1.nlst.parents[i];
    if (obj1->u1.nlst.name[0] == ':') continue;
    if ((type == OBJTYPEANY || obj1->type == type) &&
        (depth == desired_depth)) {
      r = ObjListCreate(obj1, r);
    }
    r = ObjAncestorsOfDepth1(obj1, type, desired_depth, depth+1, maxdepth, r);
  }
  return(r);
}

ObjList *ObjAncestorsOfDepth(Obj *obj, int type, int desired_depth)
{
  return(ObjAncestorsOfDepth1(obj, type, desired_depth, 0, INTPOSINF, NULL));
}

ObjList *ObjParents(Obj *obj, int type)
{
  return(ObjAncestors1(obj, type, 0, 1, NULL));
}

long ObjNumDescendants(Obj *obj, int type)
{
  int	i;
  long	r;
  Obj	*obj1;
  if (obj->type == OBJTYPELIST) return(0L);
  r = 0L;
  for (i = 0; i < obj->u1.nlst.numchildren; i++) {
    obj1 = obj->u1.nlst.children[i];
    if (type == OBJTYPEANY || obj1->type == type) r++;
    r += ObjNumDescendants(obj1, type);
  }
  return(r);
}

/* Return those most general noncontrast ancestors of objects that are not
 * an ancestor of all the objects. (e.g., telephones)
 */
ObjList *ObjCutClasses(ObjList *objs)
{
  ObjList	*ancest, *p1, *p2, *r;
  r = NULL;
  for (p1 = objs; p1; p1 = p1->next) {
    ancest = ObjAncestors(p1->obj, OBJTYPEANY);
    for (p2 = ancest; p2; p2 = p2->next) {
      if (ObjListIn(p2->obj, r)) continue;
      if (ObjIsContrast(p2->obj)) continue;
      if (!ObjList_AndISA(p2->obj, objs)) {
        r = ObjListCreate(p2->obj, r);
      }
    }
    ObjListFree(ancest);
  }
  return(ObjListRemoveLessGeneralISA(r));
}

/******************************************************************************
 * SIMILARITY
 ******************************************************************************/

Bool ObjEqual(Obj *obj1, Obj *obj2)
{
  if (obj1 == obj2) return(1);
  if (obj1->type == OBJTYPENUMBER && obj2->type == OBJTYPENUMBER) {
    return(obj1->u2.number == obj2->u2.number &&
           ObjToNumberClass(obj1) == ObjToNumberClass(obj2));
  }
  if (obj1->type == OBJTYPETSR && obj2->type == OBJTYPETSR) {
    return(TsRangeEqual(&obj1->u2.tsr, &obj2->u2.tsr));
  }
  if (obj1->type == OBJTYPESTRING && obj2->type == OBJTYPESTRING) {
    return(obj1->u2.s == obj2->u2.s);
      /* These are interned, so no need to run streq. */
  }
  if (obj1->type == OBJTYPENAME && obj2->type == OBJTYPENAME) {
    if (obj1->u2.nm == obj2->u2.nm) return(1);
    /* todo: Compare name contents for equality. */
  }
  return(0);
}

Bool ObjSimilarList(Obj *obj1, Obj *obj2)
{
  int	i, len;
  if (ObjEqual(obj1, obj2)) return(1);
  if (obj1->type != OBJTYPELIST || obj2->type != OBJTYPELIST ||
      obj1->u1.lst.len != obj2->u1.lst.len) {
    return(0);
  }
  for (i = 0, len = obj1->u1.lst.len; i < len; i++) {
    if (!ObjSimilarList(obj1->u1.lst.list[i], obj2->u1.lst.list[i])) {
      return(0);
    }
  }
  if (!TsRangeEqual(ObjToTsRange(obj1), ObjToTsRange(obj2))) return(0);
  return(1);
}

Bool ObjSimilarListNoTsRange(Obj *obj1, Obj *obj2)
{
  int	i, len;
  if (ObjEqual(obj1, obj2)) return(1);
  if (obj1->type != OBJTYPELIST || obj2->type != OBJTYPELIST ||
      obj1->u1.lst.len != obj2->u1.lst.len) {
    return(0);
  }
  for (i = 0, len = obj1->u1.lst.len; i < len; i++) {
    if (!ObjSimilarListNoTsRange(obj1->u1.lst.list[i], obj2->u1.lst.list[i])) {
      return(0);
    }
  }
  return(1);
}

Bool ObjIsSpecifPart(Obj *spec, Obj *gen)
{
  if (ISAP(gen, spec)) return(1);
  if (DbIsPartOf(&TsNA, NULL, spec, gen)) return(1);
  return(0);
}

/* obj1: specific -- descendant
 * obj2: general -- ancestor
 */
Bool ObjIsSpecif(Obj *obj1, Obj *obj2)
{
  int	i, len;
  if (ISA(obj2, obj1)) return(1);
  if (obj1->type == OBJTYPENUMBER && obj2->type == OBJTYPENUMBER) {
    return(obj1->u2.number == obj2->u2.number);
  }
  if (obj1->type != OBJTYPELIST || obj2->type != OBJTYPELIST ||
      obj1->u1.lst.len != obj2->u1.lst.len) {
    return(0);
  }
  if (!TsRangeEqual(ObjToTsRange(obj1), ObjToTsRange(obj2))) return(0);
  for (i = 0, len = obj1->u1.lst.len; i < len; i++) {
    if (!ObjIsSpecif(obj1->u1.lst.list[i], obj2->u1.lst.list[i])) {
      return(0);
    }
  }
  return(1);
}

Float ObjSimilarity(Obj *obj1, Obj *obj2)
{
  int	i, len, len1, len2;
  Float	r, weight;
  if (obj1 == obj2) return(1.0);
  /* todo: Return ancestor distance. */
  if (obj1->type == OBJTYPENUMBER && obj2->type == OBJTYPENUMBER) {
    if (obj1->u2.number == obj2->u2.number) return(1.0);
    else return(0.0);
  }
  if (obj1->type != OBJTYPELIST || obj2->type != OBJTYPELIST) return(0.0);
  if (I(obj1, 0) != I(obj2, 0)) return(0.0);	/* todo: Ancestor distance. */
  r = 0.5;
  len1 = ObjLen(obj1);
  len2 = ObjLen(obj2);
  weight = 2.0/IntMax(len1, len2);
  for (i = 1, len = IntMin(len1, len2); i < len; i++) {
    r += weight*ObjSimilarity(I(obj1, i), I(obj2, i));
  }
/*
  DbgOP(DBGOBJ, DBGDETAIL, obj1);
  DbgOP(DBGOBJ, DBGDETAIL, obj2);
  Dbg(DBGOBJ, DBGDETAIL, "similarity = %g", r);
*/
  return(r);
}

ObjList *ObjShortestPath(Obj *from, Obj *to, int depth, int maxdepth,
                         /* RESULT */ ObjList **visited)
{
  ObjList *parents, *children, *p, *path, *minpath;
  if (ObjBarrierISA(from)) return NULL;
  if (depth >= maxdepth) return NULL;
  if (ObjListIn(from, *visited)) return NULL;

  if (from == to) return ObjListCreate(from, NULL);
  *visited = ObjListCreate(from, *visited);

  minpath = NULL;

  parents = ObjParents(from, OBJTYPEANY);
  for (p = parents; p; p = p->next) {
    if (NULL != (path = ObjShortestPath(p->obj, to, depth+1, maxdepth,
                                        visited))) {
      if (minpath == NULL || ObjListLen(path) < ObjListLen(minpath)) {
        minpath = path;
      }
    }
  }
  ObjListFree(parents);

  children = ObjChildren(from, OBJTYPEANY);
  for (p = children; p; p = p->next) {
    if (NULL != (path = ObjShortestPath(p->obj, to, depth+1, maxdepth,
                                        visited))) {
      if (minpath == NULL || ObjListLen(path) < ObjListLen(minpath)) {
        minpath = path;
      }
    }
  }
  ObjListFree(children);

  if (minpath == NULL) return NULL;
  return ObjListCreate(from, minpath);
}

int ObjShortestPathLen1(Obj *from, Obj *to, int depth, int maxdepth,
                        /* RESULT */ ObjList **visited)
{
  int t, min;
  ObjList *parents, *children, *p;
  if (ObjBarrierISA(from)) return -1;
  if (depth >= maxdepth) return -1;
  if (ObjListIn(from, *visited)) return -1;
  if (ObjListLen(*visited) > 100) return -1;

  if (from == to) return 1;
  *visited = ObjListCreate(from, *visited);

  min = -1;

  parents = ObjParents(from, OBJTYPEANY);
  for (p = parents; p; p = p->next) {
    if (-1 != (t = ObjShortestPathLen1(p->obj, to, depth+1, maxdepth,
                                       visited))) {
      if (min == -1 || t < min) {
        min = t;
      }
    }
  }
  ObjListFree(parents);

  children = ObjChildren(from, OBJTYPEANY);
  for (p = children; p; p = p->next) {
    if (-1 != (t = ObjShortestPathLen1(p->obj, to, depth+1, maxdepth,
                                       visited))) {
      if (min == -1 || t < min) {
        min = t;
      }
    }
  }
  ObjListFree(children);

  if (min == -1) return min;
  return min + 1;
}

/* Returns number of nodes in path. */
int ObjShortestPathLen(Obj *from, Obj *to)
{
  int     maxdepth, r;
  ObjList *visited;
/*
  ObjList *path;
*/
  if (from == to) return 1;
  for (maxdepth = 2; maxdepth <= 5; maxdepth++) {
/*
    visited = NULL;
    path = ObjShortestPath(from, to, 0, maxdepth, &visited);
    if (path != NULL) {
      fprintf(Log, "\nPATH:\n");
      ObjListPrint(Log, path);
    }
    ObjListFree(visited);
*/

    visited = NULL;
    r = ObjShortestPathLen1(from, to, 0, maxdepth, &visited);
    ObjListFree(visited);
    if (r != -1) return r;
  }
  return -1;
}

/* cf Leacock and Chodorow (1998, p. 275) */
Float ObjPathLengthSimilarity(Obj *obj1, Obj *obj2)
{
  int t;
  if (-1 == (t = ObjShortestPathLen(obj1, obj2))) {
    return 0.0;
  }
  return -log(t/(2.0*MAXISADEPTH));
}

/******************************************************************************
 * FINDING
 ******************************************************************************/

int ObjFindInList(Obj *obj, Obj *objlist)
{
  int	i, len;
  if (objlist->type != OBJTYPELIST) return(0);
  for (i = 0, len = objlist->u1.lst.len; i < len; i++) {
    if (objlist->u1.lst.list[i] == obj) return(i);
  }
  return(-1);
}

Bool ObjIn(Obj *obj1, Obj *obj2)
{
  int  i, len;
  if (obj2->type != OBJTYPELIST) return(0);
  for (i = 0, len = obj2->u1.lst.len; i < len; i++) {
    if (obj1 == obj2->u1.lst.list[i]) return(1);
  }
  return(0);
}

Bool ObjInDeep(Obj *obj1, Obj *obj2)
{
  int  i, len;
  if (obj2->type != OBJTYPELIST) return(0);
  for (i = 0, len = obj2->u1.lst.len; i < len; i++) {
    if (obj1 == obj2->u1.lst.list[i]) return(1);
    if (ObjInDeep(obj1, obj2->u1.lst.list[i])) return(1);
  }
  return(0);
}

int ObjDepth(Obj *obj)
{
  int  i, len, t, depth;
  if (obj->type != OBJTYPELIST) return(0);
  depth = 0;
  for (i = 0, len = obj->u1.lst.len; i < len; i++) {
    if ((t = 1+ObjDepth(obj->u1.lst.list[i])) > depth) depth = t;
  }
  return(depth);
}

int ObjFindDescendantInList(Obj *obj, Obj *objlist)
{
  int	i, len;
  if (objlist->type != OBJTYPELIST) return(-1);
  for (i = 0, len = objlist->u1.lst.len; i < len; i++)
    if (ISA(obj, objlist->u1.lst.list[i])) return(i);
  return(-1);
}

/******************************************************************************
 * WEIGHTS
 ******************************************************************************/

/* Input:  weights range from -1.0 to 1.0.
 * Output: ranges from 0.0 = no concord to 1.0 = total concord.
 */
Float WeightConcordance(Float w1, Float w2)
{
  return(1.0 - (0.5*FloatAbs(w1-w2)));
}

Float Weight01toNeg1Pos1(Float weight)
{
  return((2.0*weight) - 1.0);
}

Float WeightNeg1Pos1to01(Float weight)
{
  return(0.5*(weight + 1.0));
}

/* todo: Improve this, use theta-roles; cf ObjModGetPropValue. */
Float ObjWeight(Obj *obj)
{
  Obj *o;
  if (obj == NULL) return(WEIGHT_DEFAULT);
  if (obj->type != OBJTYPELIST) return(WEIGHT_DEFAULT);
  if (obj->u1.lst.len < 3) return(WEIGHT_DEFAULT);
  o = obj->u1.lst.list[obj->u1.lst.len-1];
  if (o->type != OBJTYPENUMBER) return(WEIGHT_DEFAULT);
  return(o->u2.number);
}

/* todo: Recode. */
Obj *ObjCopyWithoutWeight(Obj *obj)
{
  Obj	*o;
  if (obj == NULL) return(NULL);
  if (obj->type != OBJTYPELIST) return(NULL);
  if (obj->u1.lst.len < 3) return(ObjCopyList(obj));
  if (obj->u1.lst.len == 3) {
    o = I(obj, 2);
    if (o->type == OBJTYPENUMBER) {
      return(L(I(obj, 0), I(obj, 1), E));
    }
    return(ObjCopyList(obj));
  }
  if (obj->u1.lst.len == 4) {
    o = I(obj, 3);
    if (o->type == OBJTYPENUMBER) {
      return(L(I(obj, 0), I(obj, 1), I(obj, 2), E));
    }
    return(ObjCopyList(obj));
  }
  return(ObjCopyList(obj));
}

Float ObjWeightConcordance(Obj *obj1, Obj *obj2)
{
  return(WeightConcordance(ObjWeight(obj1), ObjWeight(obj2)));
}

/* todo: This needs work: object length needs to be increased if it
 * doesn't already contain a weight.
 */
void ObjWeightSet(Obj *obj, Float weight)
{
  Obj *o;
  if (obj->type != OBJTYPELIST) return;
  if (obj->u1.lst.len < 3) return;	/* todo */
  o = obj->u1.lst.list[obj->u1.lst.len-1];
  if (o->type != OBJTYPENUMBER) return;
  obj->u1.lst.list[obj->u1.lst.len-1] = NumberToObj(weight);
}

Float ObjWeightIncrement(Obj *obj, Float incr)
{
  Float	weight;
  weight = ObjWeight(obj);
  weight += incr;
  ObjWeightSet(obj, weight);
  return(weight);
}

Float ObjModGetPropValue(Obj *prop)
{
  Float	value;
  /* todo: Improve this code; really should use r2=number to determine this
   * else can base on whether attribute or relation.
   */
  if (FLOATNA != (value = ObjToNumber(I(prop, 4)))) {
    return(value);
  } else if (FLOATNA != (value = ObjToNumber(I(prop, 3)))) {
    return(value);
  } else if (FLOATNA != (value = ObjToNumber(I(prop, 2)))) {
    return(value);
  } else {
    return(WEIGHT_DEFAULT);
  }
}

/******************************************************************************
 * PRINTING
 ******************************************************************************/

/* For debugging. */
void po(Obj *obj)
{
  ObjPrint1(stdout, obj, NULL, 5, 1, 0, 1, 0);
  fputc(NEWLINE, stdout);
  ObjPrint1(Log, obj, NULL, 5, 1, 0, 1, 0);
  fputc(NEWLINE, Log);
}

void pp(Obj *obj)
{
  ObjPrettyPrint(stdout, obj);
  ObjPrettyPrint(Log, obj);
}

void pol(ObjList *objs)
{
  ObjListPrettyPrint(stdout, objs);
  fputc(NEWLINE, stdout);
  ObjListPrettyPrint(Log, objs);
  fputc(NEWLINE, Log);
}

void polol(ObjListList *objs)
{
  ObjListListPrint(stdout, objs, 0, 0, 0);
  fputc(NEWLINE, stdout);
  ObjListListPrint(Log, objs, 0, 0, 0);
  fputc(NEWLINE, Log);
}

/* STRING:email-address:"xxx@yyy.zzz" */
void ObjPrintString(FILE *stream, Obj *obj)
{
  Obj	*class;
  if (N("string") == (class = ObjToStringClass(obj))) {
    fprintf(stream, "\"%s\"", ObjToString(obj));
  } else {
    fprintf(stream, "STRING:%s:\"%s\"", M(class), ObjToString(obj));
  }
}

/* NUMBER:number:23
 * NUMBER:USD:1.50
 * NUMBER:byte:1024
 */
void ObjPrintNumber(FILE *stream, Obj *obj)
{
  fprintf(stream, "NUMBER:%s:%g", M(ObjToNumberClass(obj)),
          ObjToNumber(obj));
}

void ObjPrint1(FILE *stream, Obj *obj, PNode *pn, int todepth, int show_tsr,
               int show_addr, int indicate_pn, int html)
{
  int i;
  if (!obj) {
    fputs("NullObj", stream);
    return;
  }
  if (!ObjIsOK(obj)) {
    fputs("BadObj", stream);
    return;
  }
  if (indicate_pn && pn) fputc('*', stream);
  if (show_addr) fprintf(stream, "0x%p:", obj);
  if (show_tsr) {
    if (obj->type == OBJTYPELIST && (!TsRangeIsNa(&obj->u2.tsr))) {
      TsRangePrint(stream, &obj->u2.tsr);
      fputc(TREE_SLOT_SEP, stream);
    }
  }
  switch (obj->type) {
    case OBJTYPEDESTROYED:
      fputs("DESTROYED", stream);
      break;
    case OBJTYPEASYMBOL:
    case OBJTYPEACSYMBOL:
    case OBJTYPECSYMBOL:
      if (html) {
        HTML_LinkToObj(stream, obj, 0, 1, 0);
      } else {
        fputs(obj->u1.nlst.name, stream);
      }
      break;
    case OBJTYPESTRING:
      ObjPrintString(stream, obj);
      break;
    case OBJTYPENUMBER:
      ObjPrintNumber(stream, obj);
      break;
    case OBJTYPELIST:
      if (todepth >= 0) {
        fputc(LBRACKET, stream);
        for (i = 0; i < obj->u1.lst.len; i++) {
          ObjPrint1(stream, I(obj, i), PNI(obj, i), todepth-1, show_tsr,
                    show_addr, indicate_pn, html);
          if (i != obj->u1.lst.len-1) fputc(SPACE, stream);
        }
        fputc(RBRACKET, stream);
      }
      break;
    case OBJTYPEGRID:
      fputs(obj->u1.nlst.name, stream);
      break;
    case OBJTYPEGRIDSUBSPACE:
      GridSubspacePrint(stream, &TsNA, obj->u2.gridsubspace, 0);
      break;
    case OBJTYPETSR:
      TsRangePrint(stream, &obj->u2.tsr);
      break;
    case OBJTYPETRIPLEG:
      if (!obj->u2.tripleg) Stop();
      else TripLegShortPrint(stream, obj->u2.tripleg);
      break;
    case OBJTYPENAME:
      /* todo: parsing NAME's? */
      fprintf(stream, "NAME:\"%s\"", obj->u2.nm->fullname);
      break;
    default:
      fputs("UNKNOWN", stream);
  }
}

void ObjPrint(FILE *stream, Obj *obj)
{
  ObjPrint1(stream, obj, NULL, 5, 0, 0, 0, 0);
}

void ObjPrintT(FILE *stream, Obj *obj)
{
  ObjPrint1(stream, obj, NULL, 5, 1, 0, 0, 0);
}

void ObjPrettyPrint1(FILE *stream, Obj *obj, PNode *pn, int column, int indent,
                     int show_tsr, int show_addr, int indicate_pn, int html)
{
  int  i, len;

  if (indent) StreamPrintSpaces(stream, column);
  if (!obj) {
    fputs("NULL Obj", stream);
  } else if (obj->type != OBJTYPELIST || ObjDepth(obj) <= 1) {
    if (show_addr) fprintf(stream, "0x%p:", obj);
    ObjPrint1(stream, obj, pn, 5, show_tsr, show_addr, indicate_pn, html);
  } else {
    if (indicate_pn && pn) fputc('*', stream);
    if (show_addr) fprintf(stream, "0x%p:", obj);
    if (show_tsr && (!TsRangeIsNa(&obj->u2.tsr))) {
      TsRangePrint(stream, &obj->u2.tsr);
      fputc(TREE_SLOT_SEP, stream);
    }
    fputc(LBRACKET, stream);
    for (i = 0, len = obj->u1.lst.len; i < len; i++) {
      ObjPrettyPrint1(stream, obj->u1.lst.list[i], PNI(obj, i), 
                      column+1, i != 0, show_tsr, show_addr, indicate_pn,
                      html);
      if (i != len - 1) {
        if (html) fputs("<br>\n", stream);
        else fputc(NEWLINE, stream);
      }
    }
    fputc(RBRACKET, stream);
  }
}

void ObjPrettyPrint(FILE *stream, Obj *obj)
{
  ObjPrettyPrint1(stream, obj, NULL, 0, 1, 1, 0, 1, 0);
  fputc(NEWLINE, stream);
}

void ObjPrintAncestors1(FILE *stream, Obj *obj, int depth, int maxdepth)
{
  int i;
  if (obj->type == OBJTYPELIST) return;
  if (depth >= maxdepth) return;
  if (depth < 2) fprintf(stream, "%s ", M(obj));
  else fprintf(stream, "%d:%s ", depth, M(obj));
  for (i = 0; i < obj->u1.nlst.numparents; i++) {
    ObjPrintAncestors1(stream, obj->u1.nlst.parents[i], depth+1, maxdepth);
  }
}

void ObjPrintDescendants1(FILE *stream, Obj *obj, int depth, int maxdepth)
{
  int i;
  if (obj->type == OBJTYPELIST) return;
  if (depth >= maxdepth) return;
  if (depth < 2) fprintf(stream, "%s ", M(obj));
  else fprintf(stream, "%d:%s ", depth, M(obj));
  for (i = 0; i < obj->u1.nlst.numchildren; i++) {
    ObjPrintDescendants1(stream, obj->u1.nlst.children[i], depth+1, maxdepth);
  }
}

void ObjSocketPrintString(Socket *skt, Obj *obj)
{
  Obj	*class;
  if (N("string") != (class = ObjToStringClass(obj))) {
    SocketWrite(skt, "STRING:");
    SocketWrite(skt, M(class));
    SocketWrite(skt, ":");
  }
  SocketWrite(skt, "\"");
  SocketWrite(skt, ObjToString(obj));
  SocketWrite(skt, "\"");
}

void ObjSocketPrintNumber(Socket *skt, Obj *obj)
{
  char buf[PHRASELEN];
#ifdef _GNU_SOURCE
  snprintf(buf, PHRASELEN, "NUMBER:%s:%g", M(ObjToNumberClass(obj)),
           ObjToNumber(obj));
#else
  sprintf(buf, "NUMBER:%s:%g", M(ObjToNumberClass(obj)),
          ObjToNumber(obj));
#endif
  SocketWrite(skt, buf);
}

void ObjSocketPrint(Socket *skt, Obj *obj)
{
  int i;
  if (!obj) { SocketWrite(skt, "NULL"); return; }
  if (obj->type == OBJTYPELIST && (!TsRangeIsNa(&obj->u2.tsr))) {
    TsRangeSocketPrint(skt, &obj->u2.tsr);
    SocketWrite(skt, "|");
  }
  switch (obj->type) {
    case OBJTYPEDESTROYED:
      SocketWrite(skt, "DESTROYED");
      break;
    case OBJTYPEASYMBOL:
    case OBJTYPEACSYMBOL:
    case OBJTYPECSYMBOL:
      SocketWrite(skt, obj->u1.nlst.name);
      break;
    case OBJTYPESTRING:
      ObjSocketPrintString(skt, obj);
      break;
    case OBJTYPENUMBER:
      ObjSocketPrintNumber(skt, obj);
      break;
    case OBJTYPELIST:
      SocketWrite(skt, "[");
      for (i = 0; i < obj->u1.lst.len; i++) {
        ObjSocketPrint(skt, I(obj, i));
        if (i != obj->u1.lst.len-1) SocketWrite(skt, " ");
      }
      SocketWrite(skt, "]");
      break;
    case OBJTYPEGRID:
      SocketWrite(skt, obj->u1.nlst.name);
      break;
    case OBJTYPEGRIDSUBSPACE:
      GridSubspaceSocketPrint(skt, obj->u2.gridsubspace);
      break;
    case OBJTYPETSR:
      TsRangeSocketPrint(skt, &obj->u2.tsr);
      break;
    case OBJTYPETRIPLEG:
      SocketWrite(skt, "TRIPLEG"); /* todo */
      break;
    case OBJTYPENAME:
      SocketWrite(skt, "NAME:\"");
      SocketWrite(skt, obj->u2.nm->fullname);
      SocketWrite(skt, "\"");
      break;
    default:
      SocketWrite(skt, "UNKNOWN");
  }
}

void ObjTextPrintString(Text *text, Obj *obj)
{
  Obj	*class;
  if (N("string") != (class = ObjToStringClass(obj))) {
    TextPrintf(text, "STRING:");
    TextPrintf(text, M(class));
    TextPrintf(text, ":");
  }
  TextPrintf(text, "\"");
  TextPrintf(text, ObjToString(obj));
  TextPrintf(text, "\"");
}

void ObjTextPrintNumber(Text *text, Obj *obj)
{
  TextPrintf(text, "NUMBER:%s:%g", M(ObjToNumberClass(obj)),
             ObjToNumber(obj));
}

void ObjTextPrint(Text *text, Obj *obj)
{
  int i;
  if (!obj) { TextPrintf(text, "NULL"); return; }
  if (obj->type == OBJTYPELIST && (!TsRangeIsNa(&obj->u2.tsr))) {
    TsRangeTextPrint(text, &obj->u2.tsr);
    TextPrintf(text, "|");
  }
  switch (obj->type) {
    case OBJTYPEDESTROYED:
      TextPrintf(text, "DESTROYED");
      break;
    case OBJTYPEASYMBOL:
    case OBJTYPEACSYMBOL:
    case OBJTYPECSYMBOL:
      TextPrintf(text, obj->u1.nlst.name);
      break;
    case OBJTYPESTRING:
      ObjTextPrintString(text, obj);
      break;
    case OBJTYPENUMBER:
      ObjTextPrintNumber(text, obj);
      break;
    case OBJTYPELIST:
      TextPrintf(text, "[");
      for (i = 0; i < obj->u1.lst.len; i++) {
        ObjTextPrint(text, I(obj, i));
        if (i != obj->u1.lst.len-1) TextPrintf(text, " ");
      }
      TextPrintf(text, "]");
      break;
    case OBJTYPEGRID:
      TextPrintf(text, obj->u1.nlst.name);
      break;
    case OBJTYPEGRIDSUBSPACE:
      GridSubspaceTextPrint(text, obj->u2.gridsubspace);
      break;
    case OBJTYPETSR:
      TsRangeTextPrint(text, &obj->u2.tsr);
      break;
    case OBJTYPETRIPLEG:
      TextPrintf(text, "TRIPLEG"); /* todo */
      break;
    case OBJTYPENAME:
      TextPrintf(text, "NAME:\"");
      TextPrintf(text, obj->u2.nm->fullname);
      TextPrintf(text, "\"");
      break;
    default:
      TextPrintf(text, "UNKNOWN");
  }
}

/******************************************************************************
 * READING
 ******************************************************************************/

#define WHITE_SPACE(c) (((c) == SPACE) || ((c) == '\t') || \
                        ((c) == '\n') || ((c) == '\r'))

#define GETC(c, stream) { \
     c = getc(stream); \
     while (c == ';') { \
       c = getc(stream); \
       while ((c != '\n') && (c != EOF)) c = getc(stream); \
       if (c != EOF) c = getc(stream); \
     } \
   }

#define GET_NONWHITE(c, stream) { \
     GETC(c, stream); while (WHITE_SPACE(c)) GETC(c, stream); }

/* todo: Simplify this. */
Obj *ObjRead(FILE *stream)
{
  int		c, len;
  char		buf[PARAGRAPHLEN];
  Obj		*m;
  Obj		*elems[MAXLISTLEN];

  GET_NONWHITE(c, stream)
  if (c == EOF) return(NULL);
  if (c == LBRACKET) {
    GET_NONWHITE(c, stream);
    if (c == RBRACKET) return(ObjWild);
    len = 0;
    while (c != RBRACKET) {
      ungetc(c, stream);
      if (NULL == (m = ObjRead(stream))) break;
      if (len >= MAXLISTLEN) {
        if (!IncreaseMsg) {
          Dbg(DBGOBJ, DBGBAD, "increase MAXLISTLEN");
        }
        IncreaseMsg = 1;
        break;
      }
      elems[len] = m;
      len++;
      GET_NONWHITE(c, stream)
    }
    return(ObjCreateList1(elems, len));
  } else if (c == DQUOTE) {
    buf[0] = c; len = 1;
    GETC(c, stream)
    while (c != EOF && c != DQUOTE) {
    /* todo: Escape characters. */
      if (len >= PARAGRAPHLEN-1) {
        Dbg(DBGOBJ, DBGBAD, "increase PARAGRAPHLEN");
        break;
      }
      buf[len] = c; len++;
      GETC(c, stream)
    }
    if (c == DQUOTE) {
      buf[len] = c; len++;
    }
  } else {
    buf[0] = c; len = 1;
    GETC(c, stream)
    while (c != EOF && (!WHITE_SPACE(c)) && c != LBRACKET && c != RBRACKET) {
      if (len >= PARAGRAPHLEN-1) {
        Dbg(DBGOBJ, DBGBAD, "increase PARAGRAPHLEN");
        break;
      }
      buf[len] = c; len++;
      if (c == DQUOTE) {
      /* MALE:"Jim Garnier" */
        GETC(c, stream)
        while (c != EOF && c != DQUOTE) {
          if (len >= PARAGRAPHLEN-1) {
            Dbg(DBGOBJ, DBGBAD, "increase PARAGRAPHLEN");
            break;
          }
          buf[len] = c; len++;
          GETC(c, stream)
        }
        if (c == DQUOTE) {
          buf[len] = c; len++;
        }
      }
      GETC(c, stream)
    }
  }
  if (c == LBRACKET || c == RBRACKET) ungetc(c, stream);
  buf[len] = TERM;
  return(TokenToObj(buf));
}

Obj *ObjReadString(char *s)
{
  char outfn[FILENAMELEN];
  FILE *stream;
  Obj  *obj;

  FileTemp("/tmp/ObjReadString", FILENAMELEN, outfn);
  if (NULL == (stream = StreamOpen(outfn, "w+"))) return NULL;
  fputs(s, stream);
  StreamClose(stream);

  if (NULL == (stream = StreamOpen(outfn, "r"))) return NULL;
  obj = ObjRead(stream);
  StreamClose(stream);

  unlink(outfn);
  return(obj);
}

#define TREE_SLOT_SEP           ((uc)'|')
#define TREE_TS_SLOT            '@'

/* [President-of United-States Bill Clinton]
 * or
 * @19930120:na|[President-of United-States Bill Clinton]
 */
Obj *ObjReadStringWithTSR(char *s)
{
  char    buf[DWORDLEN];
  Obj     *obj;
  TsRange tsr;
  if (s[0]==TREE_TS_SLOT) {
    s = StringReadTo(s, buf, DWORDLEN, TREE_SLOT_SEP, TERM, TERM);
    TsRangeParse(&tsr, s);
  } else {
    TsRangeSetAlways(&tsr);
  }
  obj = ObjReadString(s);
  if (!obj) return NULL;
  ObjSetTsRange(obj, &tsr);
  return obj;
}

Obj *ObjEnterList()
{
  int	len;
  char	buf[DWORDLEN];
  Obj	*elems[MAXLISTLEN];
  len = 0;
  while (1) {
    if (len >= MAXLISTLEN) break;
    if (!StreamAskStd("Next element: ", DWORDLEN, buf)) return(NULL);;
    if (buf[0] == TERM) break;
    elems[len] = TokenToObj(buf);
    len++;
  }
  if (len == 0) return(NULL);
  else return(ObjCreateList1(elems, len));
}

/******************************************************************************
 * BINDINGS
 ******************************************************************************/

Bd *BdCreate()
{
  Bd	*bd;
  bd = CREATE(Bd);
  bd->elems = NULL;
  return(bd);
}

void BdFree(Bd *bd)
{
  BdElem	*e, *n;
  e = bd->elems;
  while (e) {
    n = e->next;
    MemFree(e, "BdElem");
    e = n;
  }
  MemFree(bd, "Bd");
}

BdElem *BdElemCreate(Obj *var, Obj *val, BdElem *next)
{
  BdElem	*e;
  e = CREATE(BdElem);
  e->var = var;
  e->val = val;
  e->next = next;
  return(e);
}

Bd *BdCopy(Bd *bd)
{
  BdElem	*e;
  Bd		*r;
  r = BdCreate();
  for (e = bd->elems; e; e = e->next) {
    r->elems = BdElemCreate(e->var, e->val, r->elems);
  }
  return(r);
}

Bd *BdCopyAppend(Bd *bd1, Bd *bd2)
{
  BdElem	*e;
  Bd		*r;
  r = BdCreate();
  for (e = bd1->elems; e; e = e->next) {
    r->elems = BdElemCreate(e->var, e->val, r->elems);
  }
  for (e = bd2->elems; e; e = e->next) {
    r->elems = BdElemCreate(e->var, e->val, r->elems);
  }
  return(r);
}

Bd *BdAssign(Bd *bd, Obj *var, Obj *val)
{
  BdElem	*e;
  for (e = bd->elems; e; e = e->next) {
    if (var == e->var) {
      e->val = val;
      return(bd);
    }
  }
  bd->elems = BdElemCreate(var, val, bd->elems);
  return(bd);
}

Obj *BdLookup(Bd *bd, Obj *var)
{
  BdElem	*e;
  for (e = bd->elems; e; e = e->next) {
    if (var == e->var) return(e->val);
  }
  return(NULL);
}

void BdPrint(FILE *stream, Bd *bd)
{
  BdElem	*e;
  if (!bd) return;
  for (e = bd->elems; e; e = e->next) {
    IndentPrint(stream);
    fputs(M(e->var), stream);
    fputc(':', stream);
    ObjPrint(stream, e->val);
    fputc(NEWLINE, stream);
  }
  IndentPrint(stream);
  fputs("----\n", stream);
}

/******************************************************************************
 * MATCHING/UNIFICATION
 ******************************************************************************/

Bool ObjMatchItem(Obj *ptn, Obj *obj)
{
  if (ObjIsVar(ptn)) return(1);
  if (ISA(ptn, obj)) return(1);
  return(ObjMatchList(ptn, obj));
}

Bool ObjMatchList(Obj *ptn, Obj *obj)
{
  int i;
  if (ptn->type != OBJTYPELIST || obj->type != OBJTYPELIST) return(0);
  if (ptn->u1.lst.len != obj->u1.lst.len) return(0);
  for (i = 0; i < ptn->u1.lst.len; i++) {
    if (!ObjMatchItem(ObjIth(ptn, i), ObjIth(obj, i))) return(0);
  }
  return(1);
}

Bd *ObjUnify(Obj *obj1, Obj *obj2)
{
  if (DbgOn(DBGUNIFY, DBGDETAIL)) {
    IndentInit();
  }
  return(ObjUnify1(obj1, obj2, BdCreate()));
}

Bd *ObjUnify1(Obj *obj1, Obj *obj2, Bd *bd)
{
  Bd  *r;

  if (DbgOn(DBGUNIFY, DBGDETAIL)) {
    IndentUp();
    IndentPrint(Log);
    ObjPrint(Log, obj1);
    ObjPrint(Log, obj2);
    fputc(NEWLINE, Log);
  }
  r = ObjUnify2(obj1, obj2, bd);
  if (DbgOn(DBGUNIFY, DBGDETAIL)) {
    BdPrint(Log, r);
    IndentDown();
  }
  return(r);
}

Obj *ObjVarToClass(Obj *var)
{
  if (var->type == OBJTYPELIST) return(ObjNA);
  return(NameToObj((var->u1.nlst.name)+1, OBJ_NO_CREATE));
}

Obj *ObjClassToVar(Obj *class)
{
  char	buf[OBJNAMELEN];
  if (class->type == OBJTYPELIST) return(ObjWild);
  buf[0] = '?';
  StringCpy(buf+1, class->u1.nlst.name, OBJNAMELEN-1);
  return(N(buf));
}

Bd *ObjUnifyVar(Obj *obj1, Obj *obj2, Bd *bd)
{
  Obj  *val, *class;
  if (obj1 == obj2 || obj1 == ObjWild || obj2 == ObjWild) {
    return(bd);
  } else if ((val = BdLookup(bd, obj1))) {
    return(ObjUnify1(val, obj2, bd));
  } else {
    if (obj1 == N("?nonhuman")) {
      /* If this kind of thing gets frequent, we could add another
       * variable char.
       */
      if (ISAP(N("human"), obj2)) return(NULL);
    } else if ((class = ObjVarToClass(obj1))) {
      if (!ISAP(class, obj2)) return(NULL);
    }
    return(BdAssign(bd, obj1, obj2));
  }
}

Bd *ObjUnify2(Obj *obj1, Obj *obj2, Bd *bd)
{
  int  i, len;

  if (ISA(obj1, obj2)) return(bd);
  else if (ObjIsVar(obj1)) return(ObjUnifyVar(obj1, obj2, bd));
  else if (ObjIsVar(obj2)) return(ObjUnifyVar(obj2, obj1, bd));
  else if (ObjIsList(obj1) && ObjIsList(obj2)) {
  /* Same logic as ObjUnifyQuick, for retrieval harmony. But in
   * other situations?
   */
    if (ObjLen(obj1) < ObjLen(obj2)) len = ObjLen(obj1);
    else len = ObjLen(obj2);
    for (i = 0; i < len; i++) {
      if (!(bd = ObjUnify1(I(obj1, i), I(obj2, i), bd)))
        return(NULL);
    }
    return(bd);
  }
  return(NULL);
}

/* For use by database retrieval. */
Bool ObjUnifyQuick(register Obj *ptn, register Obj *obj)
{
  register int i, len;
  if (ObjLen(ptn) < ObjLen(obj)) {
    len = ObjLen(ptn);
  } else if (ObjLen(ptn) == ObjLen(obj)+1) {
  /* Added 19950731 todo: Improve this. cf ObjWeight. */
    if (ObjIsNumber(I(ptn, ObjLen(ptn)-1))) {
      len = ObjLen(obj);
    } else {
      return(0);
    }
  } else if (ObjLen(ptn) > ObjLen(obj)) {
    return(0);	/* Added 19950730. */
  } else {
    len = ObjLen(obj);
  }
  for (i = 0; i < len; i++) {
    if (ObjIsNotVar(I(ptn, i)) && (!ObjEqual(I(ptn, i), I(obj, i)))) return(0);
  }
  return(1);
}

/******************************************************************************
 * INSTANTIATION
 ******************************************************************************/

Obj *ObjInstan(Obj *obj, Bd *bd)
{
  int 	i, len;
  Obj	*r, *elems[MAXLISTLEN];

  if (ObjIsVar(obj)) {
    if ((r = BdLookup(bd, obj))) return(ObjInstan(r, bd));
    else return(obj);
  } else if (ObjIsList(obj)) {
    for (i = 0, len = ObjLen(obj); i < len; i++) {
      elems[i] = ObjInstan(I(obj, i), bd);
    }
    return(ObjCreateList1(elems, len));
  } else return(obj);
}

/* Same as ObjSubst but indicated whether <from> found.
 * Caller must initialize <*found> to 0
 */
Obj *ObjSubst1(Obj *obj, Obj *from, Obj *to, PNode *to_pn,
               /* RESULTS */ int *found)
{
  int 	i, len;
  Obj	*elems[MAXLISTLEN];
  PNode	*pn_elems[MAXLISTLEN];
  if (ObjIsList(obj)) {
    for (i = 0, len = ObjLen(obj); i < len; i++) {
      if (I(obj, i) == from) {
        if (found) *found = 1;
        elems[i] = to;
        if (to_pn) pn_elems[i] = to_pn;
        else pn_elems[i] = PNI(obj, i);
      } else {
        elems[i] = ObjSubst1(I(obj, i), from, to, to_pn, found);
        pn_elems[i] = PNI(obj, i);
      }
    }
    return(ObjCreateListPNode1(elems, pn_elems, len));
  } else return(obj);
}

Obj *ObjSubst(Obj *obj, Obj *from, Obj *to)
{
  return(ObjSubst1(obj, from, to, NULL, NULL));
}

Obj *ObjSubstSimilar1(Obj *obj, Obj *from, Obj *to, /* RESULTS */ int *found)
{
  int 	i, len;
  Obj	*elems[MAXLISTLEN];
  PNode	*pn_elems[MAXLISTLEN];
  if (ObjSimilarList(obj, from)) {
    if (found) *found = 1;
    return(to);
  }
  if (ObjIsList(obj)) {
    for (i = 0, len = ObjLen(obj); i < len; i++) {
      elems[i] = ObjSubstSimilar1(I(obj, i), from, to, found);
      pn_elems[i] = PNI(obj, i);
    }
    return(ObjCreateListPNode1(elems, pn_elems, len));
  } else return(obj);
}

Obj *ObjSubstSimilar(Obj *obj, Obj *from, Obj *to)
{
  return(ObjSubstSimilar1(obj, from, to, NULL));
}

Obj *ObjEliminateArticles(Obj *obj)
{
  int 	i, len;
  Obj	*elems[MAXLISTLEN];
  PNode	*pn_elems[MAXLISTLEN];
  if (ISA(N("determiner-article"), I(obj, 0))) {
    return(ObjEliminateArticles(I(obj, 1)));
  }
  if (ObjIsList(obj)) {
    for (i = 0, len = ObjLen(obj); i < len; i++) {
      elems[i] = ObjEliminateArticles(I(obj, i));
      pn_elems[i] = PNI(obj, i);
    }
    return(ObjCreateListPNode1(elems, pn_elems, len));
  } else return(obj);
}

Obj *ObjSubstPNode(Obj *obj, Obj *from, Obj *to, PNode *pn_to)
{
  int 	i, len;
  Obj	 *elems[MAXLISTLEN];
  PNode	*pn_elems[MAXLISTLEN];
  if (ObjIsList(obj)) {
    for (i = 0, len = ObjLen(obj); i < len; i++) {
      if (I(obj, i) == from) {
        elems[i] = to;
        pn_elems[i] = pn_to;
      } else {
        elems[i] = ObjSubstPNode(I(obj, i), from, to, pn_to);
        pn_elems[i] = PNI(obj, i);
      }
    }
    return(ObjCreateListPNode1(elems, pn_elems, len));
  } else return(obj);
}

ObjList *ObjListSubst(ObjList *objs, Obj *from, Obj *to)
{
  ObjList	*p, *r;
  r = NULL;
  for (p = objs; p; p = p->next) {
    r = ObjListCreate(ObjSubst(p->obj, from, to), r);
  }
  return(r);
}

ObjList *ObjAnalogize(Obj *src, Obj *tgt)
{
  ObjList *map;
  map = NULL;
  if (!ObjAnalogize1(src, tgt, &map)) {
    ObjListFree(map);
    return NULL;
  }
  return map;
}

Bool ObjAnalogize1(Obj *src, Obj *tgt, /* OUTPUT */ ObjList **mapp)
{
  int i, len_src, len_tgt;
  Obj *maps_to, *maps_from;
  if (src == tgt) return 1;
  if (ObjIsList(src)) {
    if (!ObjIsList(tgt)) return 0;
    len_src = ObjLen(src);
    len_tgt = ObjLen(tgt);
    if (len_src != len_tgt) return 0;
    if (I(src, 0) != I(tgt, 0)) return 0;
    for (i = 1; i < len_src; i++) {
      if (!ObjAnalogize1(I(src, i), I(tgt, i), mapp)) return 0;
    }
  } else {
    if (!ObjHasCommonParent(src, tgt)) return 0;
      /* disallow if parent not the same */
    maps_to = ObjListMapLookup(*mapp, src);
    if (maps_to) {
      if (maps_to != tgt) return 0; /* disallow conflicts */
    } else {
      maps_from = ObjListMapLookupInv(*mapp, tgt);
      if (maps_from == src) return 0; /* disallow many-to-one */
      *mapp = ObjListMapAdd(*mapp, src, tgt);
    }
  }
  return 1;
}

Obj *ObjSubsts(Obj *from, ObjList *map, Grid *grid)
{
  int 		i, len;
  Obj		*r, *elems[MAXLISTLEN], *to;
  PNode		*pn_elems[MAXLISTLEN];
  if (from == NULL) return NULL;
  if (ObjIsGridSubspace(from) && grid) {
    r = ObjGridSubspaceCopy(from);
    r->u2.gridsubspace->grid = grid;
    return(r);
  }
  if ((to = ObjListMapLookup(map, from))) {
    return(to);
  }
  if (ObjIsList(from)) {
    for (i = 0, len = ObjLen(from); i < len; i++) {
      elems[i] = ObjSubsts(I(from, i), map, grid);
      pn_elems[i] = PNI(from, i);
    }
    r = ObjCreateListPNode1(elems, pn_elems, len);
    ObjTsRangeSetFrom(r, from);
    return r;
  }
  return(from);
}

/******************************************************************************
 * UNROLL
 ******************************************************************************/

void ObjUnroll1(Obj *obj, /* INPUT AND OUTPUT */ ObjList **cons,
                ObjList **modcons)
{
  int		i, j, len, len1, len2;
  ObjList	*r, *objs, *p, *q, *r1;
  r = *cons;
  if (ObjIsList(obj)) {
    len = ObjLen(obj);
    if (ISA(N("copula"), I(obj, 0))) {
    /* "What is my name?"
     * Don't unroll copula "equations".
     */
      *cons = ObjListAddIfSimilarNotIn(r, ObjCopyList(obj));
      return;
    } else if (I(obj, 0) == N("and")) {
      for (i = 1; i < len; i++) {
        ObjUnroll1(I(obj, i), &r, modcons);
      }
      *cons = r;
      return;
/* Don't unroll such-that's (intensions) anymore. Anaphora and various
   agents are responsible for processing intensions.
    } else if (I(obj, 0) == N("such-that")) {
      r = ObjListAddIfSimilarNotIn(r, I(obj, 1));
      for (i = 2; i < len; i++) {
        ObjUnroll1(I(obj, i), modcons, modcons);
      }
      *cons = r;
      return;
 */
    } else {
      /* Cartesian product. */
      r1 = ObjListCreate(ObjCopyList(obj), NULL);
      len1 = 1;
      for (i = 0; i < len; i++) {
        objs = NULL;
        ObjUnroll1(I(obj, i), &objs, modcons);
        len2 = ObjListLen(objs);
        if (len2 <= 0) {
          Dbg(DBGOBJ, DBGBAD, "ObjUnroll1");
        } else if (len2 > 1) {
          r1 = ObjListReplicateNTimes(r1, len2);
        }
        for (p = objs, q = r1; p; p = p->next) {
          for (j = 0; j < len1; j++) {
            ObjSetIth(q->obj, i, p->obj);
            q = q->next;
          }
        }
        len1 = len1*len2;
        ObjListFree(objs);
      }
      *cons = ObjListAppendDestructive(r1, r);
      return;
    }
  }
  r = ObjListAddIfSimilarNotIn(r, obj);
  *cons = r;
}

ObjList *ObjUnroll(Obj *obj)
{
  ObjList *cons, *modcons, *r;
  cons = NULL;
  modcons = NULL;
  ObjUnroll1(obj, &cons, &modcons);
  /* Result has <modcons> before <cons> on theory that <modcons>
   * set the stage. cf "mother-of"
   * todoTHREAD: could try both possibilities and see which one makes
   * more sense.
   */
  r = ObjListUniquify(cons);
  r = ObjListAppendNonsimilar(r, modcons);
  ObjListFree(cons);
  ObjListFree(modcons);
  return(r);
}

/******************************************************************************
 * CREATION/PARSING UTILITIES
 ******************************************************************************/

Obj *ObjModCreateProp(Obj *adj, Obj *noun, Obj *value)
{
  if (value) return(L(adj, noun, value, E));
  else return(L(adj, noun, E));
}

/* Examples:
 * INPUT noun = Jim props = [funny Jim] [smart Jim]
 * OUTPUT = [such-that Jim [funny Jim] [smart Jim]]
 */
Obj *ObjIntension(Obj *noun, ObjList *props)
{
  int		i;
  Obj		*elems[MAXLISTLEN];
  PNode		*pn_elems[MAXLISTLEN];
  ObjList	*p;
  if (props == NULL) return(noun);
  elems[0] = N("such-that");
  pn_elems[0] = NULL;
  elems[1] = noun;
  pn_elems[1] = NULL;
  for (i = 2, p = props; p; i++, p = p->next) {
    if (i >= MAXLISTLEN) {
      if (!IncreaseMsg) {
        Dbg(DBGOBJ, DBGBAD, "ObjModCreate: increase MAXLISTLEN");
      }
      IncreaseMsg = 1;
      break;
    }
    elems[i] = ObjCheck(p->obj);
    pn_elems[i] = PNodeCheck(p->u.sp.pn);
  }
  return(ObjCreateListPNode1(elems, pn_elems, i));
}

ObjList *ObjListIntension(Obj *noun, ObjList *props, Float score,
                         LexEntryToObj *leo, PNode *pn,
                         ObjList *next)
{
  return(ObjListCreateSP(ObjIntension(noun, props),
                         ScoreCombine(score, ObjListSPScoreCombine(props)),
                         leo, pn, ObjListSPAnaphorAppend(props), next));
}

/* Examples:
 * INPUT noun = Jim prop = [funny Jim]
 * OUTPUT = [such-that Jim [funny Jim]]
 */
Obj *ObjIntension1(Obj *noun, Obj *prop)
{
  return(L(N("such-that"), noun, prop, E));
}

/* Examples:
 * INPUT obj = [such-that Jim [funny Jim] [smart Jim]]
 * OUTPUT result = [funny Jim] [smart Jim]
 *        noun = Jim
 *
 * INPUT obj = [funny Jim]
 * OUTPUT result = NULL
 *        noun = [funny Jim]
 */
ObjList *ObjIntensionParse(Obj *obj, int article_ok, /* RESULTS */ Obj **noun)
{
  int		i, len;
  Obj		*prop;
  ObjList	*r;
  if (I(obj, 0) != N("such-that") || ObjLen(obj) < 3) goto failure;
  if (ObjIsList(I(obj, 1))) {
    if (article_ok &&
        ISA(N("determiner-article"), I(I(obj, 1), 0)) &&
        I(I(obj, 1), 1)) {
      *noun = I(I(obj, 1), 1);
      r = NULL;
      for (i = 2, len = ObjLen(obj); i < len; i++) {
        prop = ObjEliminateArticles(I(obj, i));
        r = ObjListCreateSP(prop, SCORE_MAX, NULL, PNI(obj, i), NULL, r);
      }
      return(r);
    }
  } else {
    *noun = I(obj, 1);
    r = NULL;
    for (i = 2, len = ObjLen(obj); i < len; i++) {
      prop = I(obj, i);
      r = ObjListCreateSP(prop, SCORE_MAX, NULL, PNI(obj, i), NULL, r);
    }
    return(r);
  }
failure:
  *noun = obj;
  return(NULL);
}

/* Examples:
 * INPUT adverb = degree-interrogative-adverb cons = [big adjective-argument]
 * OUTPUT = [degree-interrogative-adverb [big adjective-argument]]
 */
Obj *ObjAdverb(Obj *adverb, ObjList *cons)
{
  int		i;
  Obj		*elems[MAXLISTLEN];
  ObjList	*p;
  if (cons == NULL) return(adverb);
  elems[0] = adverb;
  for (i = 1, p = cons; p; i++, p = p->next) {
    if (i >= MAXLISTLEN) {
      if (!IncreaseMsg) {
        Dbg(DBGOBJ, DBGBAD, "ObjIntAdvConstruct: increase MAXLISTLEN");
      }
      IncreaseMsg = 1;
      break;
    }
    elems[i] = p->obj;
  }
  return(ObjCreateList1(elems, i));
}

Obj *ObjAdverb1(Obj *adverb, Obj *con)
{
  return(L(adverb, con, E));
}

/* Examples:
 * class: color
 * interrogative: interrogative-identification-determiner
 * props: [bright color]
 * result: [question-element color interrogative-identification-determiner
 *          [bright color]]
 */
Obj *ObjQuestionElement(Obj *class, Obj *interrogative, ObjList *props)
{
  int		i;
  Obj		*elems[MAXLISTLEN];
  ObjList	*p;
  elems[0] = N("question-element");
  elems[1] = class;
  elems[2] = interrogative;
  for (i = 3, p = props; p; i++, p = p->next) {
    if (i >= MAXLISTLEN) {
      if (!IncreaseMsg) {
        Dbg(DBGOBJ, DBGBAD, "ObjQuestionElement: increase MAXLISTLEN");
      }
      IncreaseMsg = 1;
      break;
    }
    elems[i] = p->obj;
  }
  return(ObjCreateList1(elems, i));
}

Bool ObjQuestionElementParse(Obj *obj, /* RESULTS */ Obj **class,
                             Obj **interrogative, ObjList **props)
{
  int		i, len;
  Obj		*prop;
  ObjList	*r;
  if (I(obj, 0) == N("question-element") && I(obj, 1) && I(obj, 2)) {
    *class = I(obj, 1);
    *interrogative = I(obj, 2);
    r = NULL;
    for (i = 3, len = ObjLen(obj); i < len; i++) {
      prop = I(obj, i);
      r = ObjListCreate(prop, r);
    }
    *props = r;
    return(1);
  }
  return(0);
}

Obj *ObjMakeAnd(Obj *obj1, Obj *obj2)
{
  Obj		*r, *object1, *object2;
  ObjList	*props1, *props2;
  props1 = ObjIntensionParse(obj1, 0, &object1);
  props2 = ObjIntensionParse(obj2, 0, &object2);
  if (object1 == object2 && props1 && props2) {
    props1 = ObjListAppendDestructive(props1, props2);
    r = ObjIntension(object1, props1);
    ObjListFree(props1);
    return(r);
  }
  ObjListFree(props1);
  ObjListFree(props2);
  return(L(N("and"), obj1, obj2, E));
}

/* todo: [or [or A B] C] => [or A B C] etc. */
Obj *ObjMakeCoordConj(Obj *conj, Obj *obj1, Obj *obj2)
{
  if (obj2 == NULL) return(obj1);
  if (obj1 == NULL) return(obj2);
  if (conj == N("and")) return(ObjMakeAnd(obj1, obj2));
  else return(L(conj, obj1, obj2, E));
}

/* Example:
 * existing: [and a b]       a
 * new:      c               b
 * result:   [and a b c]     [and a b]
 */
Obj *ObjMakeAndList(Obj *existing, Obj *new)
{
  int	i, len;
  Obj	*elems[MAXLISTLEN], *r;
  if (I(existing, 0) == N("and")) {
    for (i = 0, len = ObjLen(existing); i < len && i < MAXLISTLEN; i++) {
      elems[i] = I(existing, i);
    }
    if (len >= MAXLISTLEN) {
      if (len > MAXLISTLEN) {
        Dbg(DBGOBJ, DBGBAD, "ObjMakeAndList: len > MAXLISTLEN!!");
      } else {
        if (!IncreaseMsg) {
          Dbg(DBGOBJ, DBGBAD, "ObjMakeAndList: increase MAXLISTLEN");
        }
        IncreaseMsg = 1;
      }
    } else {
      elems[len] = new;
      len++;
    }
    r = ObjCreateList1(elems, len);
  } else {
    r = L(N("and"), existing, new, E);
  }
  ObjTsRangeSetFrom(r, new);	/* todo: What if the ts's differ? */
  return(r);
}

Obj *ObjListToPropObj(ObjList *objs, Obj *prop)
{
  int		len;
  Obj		*elems[MAXLISTLEN], *r;
  ObjList	*p;
  if (objs == NULL) return(ObjWild);
  if (ObjListIsLengthOne(objs)) return(objs->obj);
  elems[0] = prop;
  len = 1;
  for (p = objs; p; p = p->next) {
    if (len >= MAXLISTLEN) {
      if (!IncreaseMsg) {
        Dbg(DBGOBJ, DBGBAD, "ObjListToPropObj: increase MAXLISTLEN");
      }
      IncreaseMsg = 1;
      break;
    }
    elems[len] = p->obj;
    len++;
  }
  r = ObjCreateList1(elems, len);
  ObjTsRangeSetFrom(r, objs->obj);	/* todo: What if the ts's differ? */
  return(r);
}

Obj *ObjListToAndObj(ObjList *objs)
{
  return(ObjListToPropObj(objs, N("and")));
}

Obj *ObjListToOrObj(ObjList *objs)
{
  return(ObjListToPropObj(objs, N("or")));
}

ObjList *PropObjToObjList(Obj *obj, Obj *prop)
{
  int		i, len;
  ObjList	*r;
  if (prop == I(obj, 0)) {
    r = NULL;
    for (i = 1, len = ObjLen(obj); i < len; i++) {
      r = ObjListCreate(I(obj, i), r);
    }
    return(r);
  } else {
    return(ObjListCreate(obj, NULL));
  }
}

ObjList *AndObjToObjList(Obj *obj)
{
  return(PropObjToObjList(obj, N("and")));
}

ObjList *OrObjToObjList(Obj *obj)
{
  return(PropObjToObjList(obj, N("or")));
}

Obj *ObjMakeTemporal(Obj *rel, Obj *obj1, Obj *obj2)
{
  if (TsRangeIsNonNa(ObjToTsRange(obj1)) && TsRangeIsNonNa(ObjToTsRange(obj2))) {
  /* Tsranges are fully specified; no point in carrying around the temporal
   * relation.
   */
    return(ObjMakeAnd(obj1, obj2));
  } else {
    return(L(rel, obj1, obj2, E));
  }
}

/******************************************************************************
 * DEBUGGING TOOLS
 ******************************************************************************/

void ObjPrintQuery(FILE *stream, Obj *obj, int maxdepth)
{
  ObjList	*objs;
  ObjPrint(stream, obj);
  fprintf(stream, "\n");
  fprintf(stream, "ancestors: ");
  ObjPrintAncestors1(stream, obj, 0, maxdepth);
  fprintf(stream, "\n");
  fprintf(stream, "descendants: ");
  ObjPrintDescendants1(stream, obj, 0, maxdepth);
  fprintf(stream, "\n");
  ObjToLexEntryPrint(stream, obj->ole);
  fprintf(stream, "assertions involving:\n");
  objs = DbRetrieveInvolving(&TsNA, NULL, obj, 1, NULL);
  ObjListPrettyPrint(stream, objs);
  ObjListFree(objs);
}

void ObjQueryTool()
{
  char		buf[DWORDLEN];
  int		maxdepth;
  Obj		*obj;
  printf("Welcome to the Obj query tool.\n");
/*
  if (!StreamAskStd("Maximum depth: ", DWORDLEN, buf2)) return;
  maxdepth = IntParse(buf2, -1);
 */
  maxdepth = MAXISADEPTH;
  while (1) {
    if (!StreamAskStd("Enter object name: ", DWORDLEN, buf)) return;
    if (buf[0] && (obj = NameToObj(buf, OBJ_NO_CREATE))) {
      ObjPrintQuery(stdout, obj, maxdepth);
      ObjPrintQuery(Log, obj, maxdepth);
    } else fprintf(stdout, "Obj not found\n");
  }
}

void ObjValidate()
{
  Obj	*obj;
  char	*name;
  long	cnt;
  cnt = 0;
  for (obj = Objs; obj; obj = obj->next) {
    ObjIsOK(obj);
    cnt++;
#ifdef notdef
    /* This is often OK: NBC etc. */
    if (ObjIsConcrete(obj) && (ObjNumChildren(obj) > 0)) {
      Dbg(DBGOBJ, DBGBAD, "concrete object <%s> has children", M(obj));
    }
#endif
    if (ObjIsSymbol(obj) &&
        (obj != OBJDEFER) &&
        ((!ObjIsList(obj)) && ObjNumParents(obj) == 0) &&
        (!IsUnprocessedTail(name = ObjToName(obj))) &&
        (!StringIn('@', name)) && /* email address */
        (!StringIn('.', name)) && /* news group */
        (name[0] != '.') && (name[0] != '?') &&
        (!(name[0] == ':' && name[1] == ':')) &&
        (!(name[0] == '/' && name[strlen(name)-1] == '/'))) {
      Dbg(DBGOBJ, DBGBAD, "<%s> has no parents", M(obj));
    }
  }
  Dbg(DBGOBJ, DBGDETAIL, "%ld objects", cnt);
}

void ov()
{
  ObjValidate();
}

/* End of file. */
