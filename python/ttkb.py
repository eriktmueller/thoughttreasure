# -*- coding: iso-8859-1 -*-
#
# ThoughtTreasure
# Python interface to ThoughtTreasure knowledge base files
#
# Copyright 1999, 2015 Erik Thomas Mueller.
# All Rights Reserved.
#
# export TTROOT=(the root directory of ThoughtTreasure)
# export PATH=$PATH:$TTROOT/python
#
# Examples:
# import os
# import ttkb
# h=ttkb.TTKB(os.environ['TTROOT'],[])
# le=h.leGet('bromide-Nz',[])
# obj=h.objGet('green')
# infl=h.inflGet('grasp')
# ttkb.inflIsType(h,'Erik','given-name')
#
# import ttkb
# ttkb.cmd(['ttkb','set','-frame'])
#

import os
import stat
import string
import sys
import time
import urllib
import tt

def dbg(s):
  ts="%.4d%.2d%.2dT%.2d%.2d%.2d" % time.localtime(time.time())[:6]
  sys.stdout.write(ts+': '+s+'\n')
  sys.stdout.flush()

class TTKB:
  def __init__(self, ttroot, options):
    self.ttroot = ttroot
    self.OK = 1
    try:
      self.le = open(ttroot+'/ttkb/le.txt')
      self.obj = open(ttroot+'/ttkb/obj.txt')
      self.infl = open(ttroot+'/ttkb/infl.txt')
    except:
      printLine(self,ttroot+'/ttkb/*.txt not found',options)
      printLine(self,'You may need to set TTROOT',options)
      printLine(self,'You may need to run $TTROOT/runtime/grind.sh',options)
      self.OK = 0
    self.clearCache()
    self.limitReset(options)

  def clearCache(self):
    self.leCache = {}
    self.objCache = {}
    self.inflCache = {}

  def leGet(self, leUid, options):
    if self.leCache.has_key(leUid):
      return self.leCache[leUid]
    else:
      s=binsearch(self.le, leUid, 0)
      if not s:
        return None
      le = leCreate(s, options)
      self.leCache[leUid] = le
      return le

  def objGet(self, objname):
    if self.objCache.has_key(objname):
      return self.objCache[objname]
    else:
      s=binsearch(self.obj, objname, 0)
      if not s: return None
      obj=Obj(s)
      self.objCache[objname] = obj
      return obj

  def inflGet(self, phrase):
    phrase=addUnderscore(phrase)
    try:
      return self.inflCache[phrase]
    except:
      x=binsearch(self.infl, phrase, 1)
      if x==None: return None
      r=readInflections(x)
      self.inflCache[phrase] = r
      return r
  def grep(self,ptn,options):
    if self.limitIsReached(options): return
    printLine(self,'ThoughtTreasure words:',options)
    newline(options)
    self.le.seek(0)
    done=[]
    while 1:
      if self.limitIsReached(options): return
      s = self.le.readline()
      if not s: break
      if -1 != string.find(s,ptn):
        toks=string.split(s,' ')
        word=leUidToWord(toks[0])
        if word not in done:
          done.append(word)
          printLine(self,wordString(word,options),options)
    newline(options)
    printLine(self,'ThoughtTreasure objects:',options)
    newline(options)
    self.obj.seek(0)
    done=[]
    while 1:
      if self.limitIsReached(options): return
      s = self.obj.readline()
      if not s: break
      if -1 != string.find(s,ptn):
        toks=string.split(s,' ')
        objname=toks[0]
        if objname not in done:
          done.append(objname)
          printLine(self,objnameString(objname,options),options)
  def limitReset(self,options):
    self.cnt = 0
    self.limitReached = 0
    if '-limit' in options:
      self.limit = 300000
    else:
      self.limit = -1
  def limitIncr(self,n):
    self.cnt = self.cnt+n  
  def limitIsReached(self,options):
    if ((self.limit != -1) and 
        (self.cnt > self.limit)):
      newline(options)
      if not self.limitReached:
        printLine(self,'Sorry, length limit reached.',options)
      self.limitReached = 1
      return 1
    return 0
  def __repr__(self):
    return '<TTKB '+self.ttroot+'>'

def leCreate(s, options):
  toks=string.split(s,' ')
  le=LE(toks[0], toks[1][1:-1], toks[2], options)
  le.leos = leoRead(le, 3, toks, options)
  return le

class LE:
  def __init__(self, uid, feat, phraseSeps, options):
    self.uid = uid
    self.word = leUidToWord(uid)
    self.feat = feat
    self.phraseSeps = phraseSeps
  def __repr__(self):
    r='<LE '+self.uid+' /'+self.feat+'/';
    for leo in self.leos:
      r=r+' '+leo.repr1()
    return r+'>'

def readtoken(s, i, delim):
  r=''
  l=len(s)
  while i<l:
    if s[i] in delim: break
    r=r+s[i]
    i=i+1
  return (r, i)

class Obj:
  def __init__(self, s):
    (self.objname,i)=readtoken(s,0,' ')
    i=i+1
    self.leUids=[]
    l=len(s)
    while i<l:
      (le,i1)=readtoken(s,i,' ')
      if '[' in le: break;
      self.leUids.append(le)
      i=i1+1
    self.assertions=readAssertions(s,i)
    self.words0=None
    self.childrenCache=None
    self.parentsCache=None
    self.isaCache={}
  def words(self,h,options):
    if not self.words0: self.words0 = self.words1(h, options)
    return self.words0
  def words1(self,h,options):
    langs=tt.F_ENGLISH+tt.F_FRENCH
    d={}
    for lang in langs: d[lang]=[]
    for leUid in self.leUids:
      le=h.leGet(leUid, options)
      lang=tt.featureGet(le.feat,tt.FT_LANG,langs[0])
      if d.has_key(lang):
        for leo in le.leos:
          if leo.objname != self.objname: continue
          word=leo.expword
          if word not in d[lang]:
            d[lang].append(word)
    r=''
    first=1
    for lang in langs:
      if len(d[lang])==0: continue
      if len(langs)>1:
        if first: first=0
        else: r=r+'; '
        r=r+'['+featExpand(lang)+'] '+string.join(d[lang],', ')
      else:
        r=r+string.join(d[lang],', ')
    return r
  def children(self):
    if self.childrenCache==None:
      self.childrenCache=self.assertionValues(['ako','isa'],2,1)
    return self.childrenCache
  def parents(self):
    if self.parentsCache==None:
      self.parentsCache=self.assertionValues(['ako','isa'],1,2)
    return self.parentsCache
  def isScript(self,h):
    return (self.isa(h,'action') and
            len(self.assertionTuples(h,'event01-of',1))>0)
  def isa(self,h,typ):
    if self.isaCache.has_key(typ):
      return self.isaCache[typ]
    r=self.isa1(h,typ,{},30)
    self.isaCache[typ]=r
    return r
  def isa1(self,h,typ,visited,depth):
    if depth==0: return 0
    if visited.has_key(self.objname): return 0
    visited[self.objname]=1
    if self.objname==typ: return 1
    for parent in self.parents():
      obj1=h.objGet(parent)
      if obj1:
        if obj1.isa1(h,typ,visited,depth-1): return 1
    return 0
  def assertionValues(self,preds,sloti,slotj):
    r=[]
    for a in self.assertions:
      if a[0] in preds and a[sloti]==self.objname:
        r.append(a[slotj])
    return r
  def assertionTuples(self,h,cls,sloti):
    r=[]
    for a in self.assertions:
      if isa(h,a[0],cls) and a[sloti]==self.objname:
        r.append(a)
    return r
  def fmt(self,h,options):
    return objnameString(self.objname,options)+'; '+self.words(h,options)
  def __repr__(self):
    r='<OBJ '+self.objname
    for leUid in self.leUids:
      r=r+' '+leUid
    for a in self.assertions:
      r=r+' '+str(a)
    return r+'>'

def inflIsType(h,s,typ):
  infls=h.inflGet(s)
  if not infls: return 0
  for infl in infls:
    if leIsType(h,infl.leUid,typ):
      return 1
  return 0

def leIsType(h,leUid,typ):
  le=h.leGet(leUid, [])
  for leo in le.leos:
    if isa(h,leo.objname,typ): return 1
  return 0

def isa(h,objname1,objname2):
  obj1=h.objGet(objname1)
  if obj1:
    return obj1.isa(h,objname2)
  return 0

def descendants(h,objname):
  obj=h.objGet(objname)
  if not obj: return []
  r=[]
  for child in obj.children():
    if child not in r: r.append(child)
    r1=descendants(h,child)
    for e in r1:
      if e not in r: r.append(e)
  return r

def descendantWords(h,objname,langs):
  objs=descendants(h,objname)
  r=[]
  for objname in objs:
    obj=h.objGet(objname)
    if not obj: continue
    for leUid in obj.leUids:
      w=leUidToWord(leUid)
      f=leUidToFeat(leUid)
      if tt.featureGet(f, tt.FT_LANG, tt.F_NULL) in langs:
        if w not in r: r.append(w)
  return r

qaoptions=['-hrole2event','-prole2event','-role2loc',
           '-action2loc','-action2dur','-action2cost',
           '-action2period','-action2result','-action2event',
           '-text2script']
alloptions=['-anc','-assert','-child','-desc','-frame','-parent',
            '-obj','-word','-le','-grep','-html','-limit']+qaoptions

def optionsCGIString(options):
  s=''
  for o in options:
    if o[0] == '-':
      if o in qaoptions:
        o='-assert'
      if o not in ['ttkb','-obj','-word','-le','-grep','-html']:
        s=s+'&'+o+'=1'
  return s

def leString(s0,le,options):
  if '-html' in options:
    s='le='+urllib.quote(le.uid)+optionsCGIString(options)
    return '<a href="query.cgi?'+s+'">'+s0+'</a>'
  else:
    return s0

def wordString(word,options):
  if '-html' in options:
    s='word='+urllib.quote(word)+optionsCGIString(options)
    return '<a href="query.cgi?'+s+'">'+word+'</a>'
  else:
    return word

def objnameString(objname,options):
  if '-html' in options:
    s='obj='+urllib.quote(objname)+optionsCGIString(options)
    return '<a href="query.cgi?'+s+'"><tt>'+objname+'</tt></a>'
  else:
    return objname

def objnameStringNoLink(objname,options):
  if '-html' in options:
    return '<tt>'+objname+'</tt>'
  else:
    return objname

def readInflections(lines):
  r=[]
  for l in lines:
    toks=string.split(l,' ')
    r.append(Infl(toks[1][1:-1],toks[2][:-1]))
  return r

class Infl:
  def __init__(self, feat, leUid):
    self.feat=feat
    self.leUid=leUid
  def __repr__(self):
    return '<Infl /'+self.feat+'/ '+self.leUid+'>'

def readAssertions(s,i):
  nest=0
  l=len(s)
  r=[]
  toparse=''
  while i<l:
    if s[i]=='@':
      toparse=toparse+s[i]
      i=i+1
      while i<l:
        toparse=toparse+s[i]
        if s[i]=='|': break
        i=i+1
    elif s[i]=='[':
      nest=nest+1
      toparse=toparse+s[i]
    elif s[i]==']':
      nest=nest-1
      toparse=toparse+s[i]
      if nest==0:
        r.append(tt.assertionParse(toparse))
        i=i+1
        if s[i]!=' ': i=i-1
        toparse=''
    else:
      toparse=toparse+s[i]
    i=i+1
  r.sort()
  return r

def assertionFmt(x):
  return assertionFmt1(x,None,[])

def assertionFmt1(x,y,options):
  if x==y: return '^'
  if type(x)!=type([]):
    return objnameString(str(x),options)
  r='['
  cnt=0
  for e in x:
    if ('-limit' in options) and (cnt>50):
      r=r+' ...'
      break
    if cnt!=0:
      r=r+' '
    cnt=cnt+1
    r=r+assertionFmt1(e,y,options)
  return r+']'

class Theta:
  def __init__(self, s):
    toks=string.split(s,':')
    if toks[0]=='': self.slotnum=-1
    else: self.slotnum=int(toks[0])
    self.case=toks[1]
    self.leUid=toks[2]
    self.subcat=toks[3]
    self.trpos=toks[4]
    self.optional=int(toks[5])
  def repr1(self):
    return (str(self.slotnum)+':'+
            self.case+':'+
            self.leUid+':'+
            self.subcat+':'+
            self.trpos+':'+
            str(self.optional))
  def __repr__(self):
    return '<Theta '+self.repr1()+'>'

class LEO:
  def __init__(self, le, objname, feat, thetas, options):
    self.le=le
    self.objname=objname
    self.feat=feat
    self.thetas=thetas
    self.expword=self.explExpandWord(options)
  def explExpandWord(self,options):
    r=''
    r=self.explExpandWord1(r,['expl'],tt.TRPOS_PRE_VERB)
    if r: r=r+' '
    r=r+self.le.word
    r=self.explExpandWord1(r,['expl'],tt.TRPOS_POST_VERB_PRE_OBJ)
    r=self.explExpandWord1(r,['obj'],tt.TRPOS_NA)
    r=self.explExpandWord1(r,['expl'],tt.TRPOS_POST_VERB_POST_OBJ)
    r=self.explExpandWord1(r,['iobj'],tt.TRPOS_NA)
    r=string.replace(r, 'Ó', 'oe')
    return leString(r,self.le,options)
  def explExpandWord1(self,r,cases,trpos):
    for theta in self.thetas:
      if theta.case not in cases: continue
      if theta.case in ['aobj','bobj','pobj','dobj']: continue
      if theta.case=='expl' and theta.trpos!=trpos: continue
      if theta.leUid!='': r=r+' '+leUidToWord(theta.leUid)
    return r
  def hasExpl(self):
    for theta in self.thetas:
      if theta.case=='expl':
        return 1
    return 0
  def hasObj(self):
    for theta in self.thetas:
      if theta.case=='obj':
        return 1
    return 0
  def hasRequiredIObj(self):
    for theta in self.thetas:
      if theta.case=='iobj' and not theta.optional:
        return 1
    return 0
  def repr1(self):
    r='/'+self.feat+'/ '+self.objname
    for theta in self.thetas:
      r=r+' '+theta.repr1()
    return r
  def __repr__(self):
    return '<LEO '+self.repr1()+'>'

def leoRead(le, i, toks, options):
  r=[]
  length = len(toks)
  while i < length:
    objname=toks[i]
    if objname=='':  break
    i=i+1
    feat=toks[i][1:-1]
    i=i+1
    thetas=[]
    while ':' in toks[i]:
      thetas.append(Theta(toks[i]))
      i=i+1
    r.append(LEO(le, objname,feat,thetas, options))
  return r

def binsearch(file, key, multi):
  low=0
  high=os.stat(file.name)[stat.ST_SIZE]
  key=key+' '
  l=len(key)
  while high>=low:
    mid=(high+low)/2
    seekto=max(0,mid-1)
    file.seek(seekto)
    s=file.readline()
    if seekto>0: s=file.readline()
    i=cmp(key,s[:l])
    if i==0:
      if not multi: return s[:-1]
      file.seek(max(0,file.tell()-8192))
      s=file.readline()
      while s[:l]!=key: s=file.readline()
      r=[]
      while s[:l]==key:
        r.append(s)
        s=file.readline()
      return r
    if i<0: high=mid-1
    else: low=mid+1
  return None

def stringElim(s, set):
  r=''
  for c in s:
    if c not in set:
      r=r+c
  return r

def featExpand(feat):
  pos=tt.featureGet(feat,tt.FT_POS,tt.F_NULL)
  feat=stringElim(feat,'¸%@'+tt.FT_POS)
  if pos!=tt.F_NULL: feat=feat+pos
  r=[]
  for f in feat:
    if tt.FEATDICT.has_key(f):
      r.append(tt.FEATDICT[f])
  if pos==tt.F_NULL:
    return string.join(r,', ')
  if len(r)==0:
    return ''
  elif len(r)==1:
    return r[0]
  else:
    return string.join(r[:-1],', ')+' '+r[-1:][0]

def leUidToWord(leUid):
  return string.replace(string.replace(string.split(leUid, '-')[0], '_', ' '),
                        'Ó', 'oe')

def leUidToFeat(leUid):
  return string.split(leUid, '-')[1]

def wordToLeUid(word,feat):
  return addUnderscore(word)+'-'+feat

def elimUnderscore(s):
  return string.replace(s, '_', ' ')

def addUnderscore(s):
  return string.replace(s, ' ', '_')

def frameFmt(thetas):
  r='   '
  r=frameFmt1(r,thetas,['subj','aobj','expl'],tt.TRPOS_PRE_VERB)
  r=r+' ----'
  r=frameFmt1(r,thetas,['expl'],tt.TRPOS_POST_VERB_PRE_OBJ)
  r=frameFmt1(r,thetas,['obj'],tt.TRPOS_NA)
  r=frameFmt1(r,thetas,['expl'],tt.TRPOS_POST_VERB_POST_OBJ)
  r=frameFmt1(r,thetas,['iobj'],tt.TRPOS_NA)
  return r

def frameFmt1(r,thetas,cases,trpos):
  for theta in thetas:
    if theta.case not in cases: continue
    if theta.case in ['aobj','bobj','pobj','dobj']: continue
    if theta.case=='expl' and theta.trpos!=trpos: continue
    if theta.optional:
      op = ' ('
      cp = ')'
    else:
      op = ' '
      cp = ''
    r=r+op
    if theta.case=='subj':
      r=r+'S'
      if theta.slotnum!=1: r=r+'['+str(theta.slotnum)+']'
      r=r+cp
    else:
      wordprinted=0
      if theta.leUid!='':
        r=r+leUidToWord(theta.leUid)
        wordprinted=1
      if theta.subcat != '':
        if wordprinted: r=r+' '
        r=r+'+'+featExpand(theta.subcat)
      else:
        if theta.leUid!='':
          if theta.case!='expl':
            if wordprinted: r=r+' '
            r=r+'IO'
            if theta.slotnum!=3: r=r+'['+str(theta.slotnum)+']'
        else:
          if wordprinted: r=r+' '
          r=r+'O'
          if theta.slotnum!=2: r=r+'['+str(theta.slotnum)+']'
      r=r+cp
  return r

def newline(options):
  if '-html' in options:
    print '<p>'
  else:
    print

def printLine(h,s,options):
  if h: h.limitIncr(len(s))
  print s
  if '-html' in options:
    print '<br>'

def cmdObj(h,objname,options):
  if h.limitIsReached(options): return
  obj=h.objGet(objname)
  if not obj:
    printLine(h,'Object '+objname+' not found.',options)
    return
  printLine(h,'ThoughtTreasure object: '+objnameStringNoLink(objname,options),
            options)
  newline(options)
  s=obj.words(h,options)
  if s: printLine(h,s, options)
  if '-assert' in options:
    cmdAssertions(h,obj,None,None,options)
  visited={}
  if '-parent' in options or '-anc' in options:
    cmdParents(h,1,objname,options,visited)
  visited={}
  if '-child' in options or '-desc' in options:
    cmdChildren(h,1,objname,options,visited)

def cmdAssertions(h,obj,involving,predClass,options):
  if h.limitIsReached(options): return
  if involving!=None:
    for a in obj.assertions:
      if h.limitIsReached(options): return
      if deepIn(involving,a) and isa(h,a[0],predClass):
        printLine(h,assertionFmt1(a,obj.objname,options),options)
  elif predClass!=None:
    for a in obj.assertions:
      if h.limitIsReached(options): return
      if isa(h,a[0],predClass):
        printLine(h,assertionFmt1(a,obj.objname,options),options)
  else:
    for a in obj.assertions:
      if h.limitIsReached(options): return
      printLine(h,assertionFmt1(a,obj.objname,options),options)

def deepIn(a,x):
  if type([])!=type(x): return 0
  if a in x: return 1
  for e in x:
    if deepIn(a,e): return 1
  return 0

def leoUniq(leos,options):
  if '-frame' in options:
    r = []
    for leo in leos:
      if leo not in r:
        r.append(leo)
    return r
  r = []
  objnames = []
  for leo in leos:
    if leo.objname not in objnames:
      r.append(leo)
      objnames.append(leo.objname)
  return r

def leoHeader(h,N,leo,obj,options):
  if N!=None: s=str(N)+'.'
  else: s=''
  s=s+' ('+leo.expword+')'
  fe=featExpand(leo.feat)
  if fe!='':
    s=s+' ['+fe+']'
  s=s+' '+obj.fmt(h,options)
  return s

def cmdLeo(h,N,leo,options):
  obj=h.objGet(leo.objname)
  cmdLeo1(h,N,leo,obj,options)

def cmdLeo1(h,N,leo,obj,options):
  if h.limitIsReached(options): return
  obj=h.objGet(leo.objname)
  s=leoHeader(h,N,leo,obj,options)
  printLine(h,s,options)
  if '-frame' in options and leo.thetas:
    printLine(h,indentString(10,options)+'*> '+frameFmt(leo.thetas),options)
  if '-assert' in options:
    cmdAssertions(h,obj,None,None,options)
  visited={}
  if '-parent' in options or '-anc' in options:
    cmdParents(h,1,leo.objname,options,visited)
  visited={}
  if '-child' in options or '-desc' in options:
    cmdChildren(h,1,leo.objname,options,visited)

def indentString(indent,options):
  if '-html' in options:
    return '&nbsp;&nbsp;'*indent
  else:
    return ' '*indent

def cmdChildren(h,indent,objname,options,visited):
  if h.limitIsReached(options): return
  if visited.has_key(objname): return
  visited[objname]=1
  obj=h.objGet(objname)
  children=obj.children()
  for c in children:
    if h.limitIsReached(options): return
    child=h.objGet(c)
    if not child: continue
    printLine(h,indentString(3*indent,options)+'=> '+
              child.fmt(h,options),options)
    if '-assert' in options:
      cmdAssertions(h,child,None,None,options)
    if '-desc' in options:
      cmdChildren(h,indent+1,c,options,visited)

def cmdParents(h,indent,objname,options,visited):
  if h.limitIsReached(options): return
  if visited.has_key(objname): return
  visited[objname]=1
  obj=h.objGet(objname)
  parents=obj.parents()
  for c in parents:
    if h.limitIsReached(options): return
    parent=h.objGet(c)
    if not parent: continue
    printLine(h,indentString(3*indent,options)+'<= '+parent.fmt(h,options),
              options)
    if '-assert' in options:
      cmdAssertions(h,parent,None,None,options)
    if '-anc' in options:
      cmdParents(h,indent+1,c,options,visited)

def cmdInfl(h,infl,done,word,options):
  le=h.leGet(infl.leUid, options)
  cmdLe1(h,le,done,word,infl,options)

def cmdLe(h,leuid,options):
  le=h.leGet(leuid, options)
  if not le:
    printLine(h,'Lexical entry '+leuid+' not found.',options)
    return
  leword=leUidToWord(le.uid)
  printLine(h,'ThoughtTreasure lexical entry: '+leuid,options)
  newline(options)
  cmdLe1(h,le,[],leword,None,options)

def cmdLe1(h,le,done,word,infl,options):
  if h.limitIsReached(options): return
  leos=leoUniq(le.leos,options)
  leword=leUidToWord(le.uid)
  if '-frame' in options:
    if len(leos)==1: sen='argument structure frame'
    else: sen='argument structure frames'
  else:
    if len(leos)==1: sen='sense'
    else: sen='senses'
  if infl:
    s='The '+featExpand(infl.feat)+' '
    if word==leword: s=s+word
    else: s=s+word+' of '+leword
    s=s+' has '+str(len(leos))+' '+sen+'.'
  elif le:
    s='The '+featExpand(le.feat)+' '
    s=s+word
    s=s+' has '+str(len(leos))+' '+sen+'.'
  else:
    s=str(len(leos))+' '+sen+'.'
  printLine(h,s,options)
  N=1
  if len(leos)==0: return
  for leo in leos:
    if h.limitIsReached(options): return
    if str(leo) not in done:
      done.append(str(leo))
      cmdLeo(h,N,leo,options)
    N=N+1
  newline(options)

def phraseCanon(s):
  return string.join(string.split(tt.whiteToSpace(
           string.replace(s,'.',' '))),' ')

def cmdWord(h,word,options):
  if h.limitIsReached(options): return
  word=phraseCanon(word)
  infls=h.inflGet(word)
  if infls==None or len(infls)==0:
    printLine(h,'Word '+word+' not found.',options)
    return
  printLine(h,'ThoughtTreasure word: '+word,options)
  newline(options)
  done=[]
  for infl in infls:
    if h.limitIsReached(options): return
    cmdInfl(h,infl,done,word,options)

def getword(argv):
  for x in argv:
    if len(x) > 0 and x[0] != '-': return x
  return None

def checkoptions(argv):
  for x in argv:
    if len(x) > 0:
      if x[0] == '-' and x not in alloptions:
        printLine(None,'Unknown option'+x,argv)
  return None

def usage(options):
  newline(options)
  printLine(None,'Usage: ttkb key [-keytype] [-searchtype...] [-html]',options)
  printLine(None,'     -html    Dump in HTML format',options)
  printLine(None,'keytype can be:',options)
  printLine(None,'     -grep    Pattern ',options)
  printLine(None,'     -le      Lexical entry uid',options)
  printLine(None,'     -obj     Object name',options)
  printLine(None,'     -word    Word (default)',options)
  printLine(None,'searchtype can be:',options)
  printLine(None,'     -anc     Show ancestor concepts',options)
  printLine(None,'     -assert  Show assertions',options)
  printLine(None,'     -child   Show child concepts',options)
  printLine(None,'     -desc    Show descendant concepts',options)
  printLine(None,'     -frame   Show argument structure frames',options)
  printLine(None,'     -parent  Show parent concepts',options)
  newline(options)
  printLine(None,'Examples:',options)
  printLine(None,"ttkb 'carry-on bag' # Show meanings",options)
  printLine(None,'ttkb put -frame # Show argument structure frames',options)
  printLine(None,'ttkb food -desc # Show hierarchy of foods',options)
  printLine(None,'ttkb tennis-ball -obj -assert',options)
  printLine(None,'  # Show all assertions regarding an object',options)
  printLine(None,'ttkb tennis-ball -obj -anc -assert',options)
  printLine(None,'  # Show all assertions regarding an object and its ancestors',
            options)
  newline(options)

def cmdQARole(h,leo,objname,word,roleClass,predClasses,options):
  if h.limitIsReached(options): return
  obj=h.objGet(objname)
  if not obj: return
  if roleClass!=None:
    if roleClass=='physical-object':
      if obj.isa(h,'human') or not obj.isa(h,roleClass): return 0
    else:
      if not obj.isa(h,roleClass): return 0
  scripts=obj.assertionTuples(h,'role-relation',2)
  headerShown=0
  found=0
  for e in scripts:
    if h.limitIsReached(options): return
    if not headerShown:
      s='ThoughtTreasure role '+leoHeader(h,None,leo,obj,options)
      if found: newline(options)
      printLine(h,s,options)
      headerShown=1
      found=1
    eobj=h.objGet(e[1])
    newline(options)
    printLine(h,'In script '+eobj.fmt(h,options),options)
    for predClass in predClasses:
      (pc,useInvolving)=predClass
      if useInvolving:
        cmdAssertions(h,eobj,leo.objname,pc,options)
      else:
        cmdAssertions(h,eobj,None,pc,options)
  return found

def cmdQAAction(h,leo,objname,word,predClasses,visited,options):
  if h.limitIsReached(options): return
  if visited.has_key(objname): return
  obj=h.objGet(objname)
  if not obj: return
  if not obj.isa(h,'action'): return
  visited[objname]=1
  s='ThoughtTreasure script '+leoHeader(h,None,leo,obj,options)
  if len(visited.keys())>1: newline(options)
  printLine(h,s,options)
  for predClass in predClasses:
    if h.limitIsReached(options): return
    (pc,useInvolving)=predClass
    if useInvolving:
      cmdAssertions(h,obj,leo.objname,pc,options)
    else:
      cmdAssertions(h,obj,None,pc,options)
  return 1

def cmdQA(h,word,roleClass,predClasses,typ,options):
  if h.limitIsReached(options): return
  word=phraseCanon(word)
  leos=semtag(h,word,5,1,tt.F_ENGLISH,options)
  if not leos:
    printLine(h,'Word '+word+' not found.',options)
    return
  visited={}
  found=0
  for leo in leos:
    if h.limitIsReached(options): return
    if typ=='role':
      if cmdQARole(h,leo,leo.objname,word,roleClass,predClasses,options):
        found=1
    else:
      if cmdQAAction(h,leo,leo.objname,word,predClasses,visited,options):
        found=1
  if not found:
    printLine(h,'No relevant information about '+word+'.',options)

def dictInc(d,key,n,e):
  try:
    d[key][0]=d[key][0]+n
    d[key][1].append(e)
  except:
    d[key]=[n,[e]]

def actionRelToScripts(h,obj):
  r=[]
  for e in obj.assertionTuples(h,'action-relation',2):
    if e[1] not in r: r.append(e[1])
  return r

# simplified script recognizer - does shallow search
def cmdTextToScript(h,word,options):
  if h.limitIsReached(options): return
  word=phraseCanon(word)
  leos=semtag(h,word,5,0,tt.F_ENGLISH,options)
  global dGlob
  dGlob={}
  i=0
  for leo in leos:
    #dbg(str(int(((100.0*i)/len(leos))))+' '+str(leo))
    if h.limitIsReached(options): return
    obj=h.objGet(leo.objname)
    if not obj: continue
    if ((not leo.hasExpl()) and 
        (not leo.hasRequiredIObj()) and
        leo.le.word in ['be','have']):
       continue
    if obj.isScript(h):
      dictInc(dGlob,leo.objname,2.0,leo)
    else:
      if obj.isa(h,'linguistic-concept'): continue
      scripts=actionRelToScripts(h,obj)
      for script in scripts:
        dictInc(dGlob,script,1.0,leo)
    i=i+1
  keys=dGlob.keys()
  if len(keys)==0:
    printLine(h,'No script recognized.',options)
    return
  keys.sort(lambda x,y:-cmp(dGlob[x][0],dGlob[y][0]))
  s=''
  for key in keys:
    s=s+'score '+str(dGlob[key][0])+' for script '
    s=s+objnameString(key,options)+' based on:\n<ul>'
    for leo in dGlob[key][1]:
      s=s+'<li>('+leo.expword+') '+objnameString(leo.objname,options)+'\n'
    s=s+'</ul>\n'
  printLine(h,s,options)

# simplified semantic tagger - does not check all constraints
def semtag(h,s,maxphraselen,firstOnly,lang,options):
  words=string.split(phraseCanon(s))
  i=0
  length=len(words)
  r=[]
  while i<length:
    leos=phrasalVerbGet(h,words,i,maxphraselen,lang,options)
    for leo in leos: r.append(leo)
    leos=phraseGet(h,words,i,maxphraselen,lang,options)
    for leo in leos: r.append(leo)
    if firstOnly and r: break
    i=i+1
  return leoUniq(r,options)

def wordPrefixMatches(words,i,maxphraselen,s):
  j=maxphraselen
  r=[]
  while j>0:
    if (i+j)<=len(words):
      phrase=string.join(words[i:i+j],' ')
      if phrase==s:
        return (1,j)
    j=j-1
  return (0,-1)

def phrasalVerbGet(h,words,i,maxphraselen,lang,options):
  head=words[i]
  headInfls=h.inflGet(head)
  if not headInfls: return []
  r=[]
  for headInfl in headInfls:
    if lang not in headInfl.feat: continue
    headLe=h.leGet(headInfl.leUid,options)
    for leo in headLe.leos:
      hasE=leo.hasExpl()
      hasI=leo.hasRequiredIObj()
      hasO=leo.hasObj()
      length=0
      if hasE:
        for theta in leo.thetas:
          if theta.case=='expl':
            if theta.trpos==tt.TRPOS_POST_VERB_PRE_OBJ:
              (success,t)=wordPrefixMatches(words,i+1,maxphraselen,
                                            leUidToWord(theta.leUid))
              if success:
                length=t
            elif theta.trpos==tt.TRPOS_POST_VERB_POST_OBJ:
              j=maxphraselen
              while j>0:
                (success,t)=wordPrefixMatches(words,i+1+j,maxphraselen,
                                              leUidToWord(theta.leUid))
                if success:
                  length=t+j
                  break
                j=j-1
        if not length:
          continue
      if hasI:
        found=0
        for theta in leo.thetas:
          if theta.case=='iobj' and not theta.optional:
            if hasO:
              if (leUidToWord(theta.leUid) in
                  words[i+length:i+length+maxphraselen]):
                found=1
            else:
              if ((i+length+1)<len(words) and
                  leUidToWord(theta.leUid)==words[i+length+1]):
                found=1
        if not found:
          continue
      if hasE or hasI:
        r.append(leo)
  return leoUniq(r,options)

def phraseGet(h,words,i,maxphraselen,lang,options):
  j=maxphraselen
  r=[]
  while j>0:
    if (i+j)<=len(words):
      phrase=string.join(words[i:i+j],' ')
      infls=h.inflGet(phrase)
      if infls:
        for infl in infls:
          if lang not in infl.feat: continue
          le=h.leGet(infl.leUid, options)
          for leo in le.leos:
            if (not leo.hasExpl()) and (not leo.hasRequiredIObj()):
              r.append(leo)
    j=j-1
  return leoUniq(r,options)

def cmd(argv):
  if len(argv)<=1:
    usage(argv)
    sys.exit(0)
  checkoptions(argv)
  var='TTROOT'
  if not os.environ.has_key(var):
    printLine(None,'You need to set the environment variable '+var+':',argv)
    printLine(None,'   export '+var+'=(the root directory of ThoughtTreasure)',argv)
    printLine(None,'You might also want to add this command to your PATH:',argv)
    printLine(None,'   export PATH=$PATH:$TTROOT/python',argv)
    newline(argv)
    ttroot='/usr/local/tt'
  else:
    ttroot=os.environ[var]
  h=TTKB(ttroot,argv)
  if not h.OK:
    sys.exit(1)
  word=getword(argv[1:])
  if not word:
    usage(argv)
    sys.exit(1)
  found=0
  if '-word' in argv:
    cmdWord(h,word,argv)
    found=1
  if '-obj' in argv:
    cmdObj(h,word,argv)
    found=1
  if '-le' in argv:
    cmdLe(h,word,argv)
    found=1
  if '-grep' in argv:
    h.grep(word,argv)
    found=1
  if '-hrole2event' in argv:
    cmdQA(h,word,'human',
          [['role-relation',1],
           ['event-assertion-relation',1]],'role',argv)
    found=1
  if '-prole2event' in argv:
    cmdQA(h,word,'physical-object',
          [['role-relation',1],
           ['event-assertion-relation',1]],'role',argv)
    found=1
  if '-role2loc' in argv:
    cmdQA(h,word,None,
          [['role-relation',1],
           ['performed-in',0]],'role',argv)
    found=1
  if '-action2loc' in argv:
    cmdQA(h,word,None,[['performed-in',0]],'action',argv)
    found=1
  if '-action2dur' in argv:
    cmdQA(h,word,None,[['duration-of',0]],'action',argv)
    found=1
  if '-action2cost' in argv:
    cmdQA(h,word,None,[['cost-of',0]],'action',argv)
    found=1
  if '-action2period' in argv:
    cmdQA(h,word,None,[['period-of',0]],'action',argv)
    found=1
  if '-action2result' in argv:
    cmdQA(h,word,None,[['result-of',0],['goal-of',0]],'action',argv)
    found=1
  if '-action2event' in argv:
    cmdQA(h,word,None,[['event-assertion-relation',0]],'action',argv)
    found=1
#  if '-text2script' in argv:
#    cmdTextToScript(h,word,argv)
#    found=1
  if not found:
    cmdWord(h,word,argv)

# End of file.
