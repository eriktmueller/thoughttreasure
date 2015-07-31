# -*- coding: iso-8859-1 -*-
#
# Translate ThoughtTreasure KB to CycL
# Copyright 1999, 2015 Erik Thomas Mueller.
# All Rights Reserved.
#

import os
import string
import sys
import time
import tt
import ttkb

LOAD_DEBUG=1
LINES_DONE=0
LINES_TOTAL=370692

TT_TO_CYC={
  'ako': 'genls',
  'isa': 'isa',
}

def fmtpc(x):
  int(x*10000)/100.0

def wrap(s):
  if LOAD_DEBUG:
    return '(fif '+s+' nil (print "fail '+str(LINES_DONE+1)+'"))'
  else:
    return s

def outln(out,s):
  global LINES_DONE
  out.write(s+'\n')
  LINES_DONE=LINES_DONE+1
  if LINES_DONE!=0 and ((LINES_DONE%(LINES_TOTAL/10))==0):
    out.write('(print "'+str(LINES_DONE)+' lines loaded")\n')
    #out.write('(print "'+str(int(.5+(100*(LINES_DONE/(0.0+LINES_TOTAL)))))+'% loaded")\n')

BaseKB='#$BaseKB'
TTMt='#$ThoughtTreasureMt'
GEnglishMt='#$TTGeneralEnglishMt'
GFrenchMt='#$TTGeneralFrenchMt'

def today():
  return "%.4d%.2d%.2d" % time.localtime(time.time())[:3]

def run():
  h=ttkb.TTKB(os.environ['TTROOT'],[])
  out=open('../ttkb/tt0.00023.cycl','w')
  hr(out)
  for e in open('preamble.txt').readlines():
    outln(out,'; '+e[:-1])
  outln(out,'; '+today())
  outln(out,';')
  hr(out)
  outln(out,';')
  for e in open('gpl.txt').readlines():
    outln(out,'; '+e[:-1])
  outln(out,';')
  hr(out)
  dumpassertions1(h,out)
  dumpmappings(h,out)
  dumpassertions2(h,out)
  collectwords(h,out)
  dumpwords(h,out)

def dumpmappings(h,out):
  for e in open('tt2cyc.map').readlines():
    (tt,cyc)=string.split(e[:-1],'|')
    tt='#$'+ttnameToCycName(tt)
    cyc='#$'+cyc
    cycassert(out,['#$TT-thoughtTreasureToCyc',tt,cyc],TTMt)
    cycassert(out,['#$TT-cycToThoughtTreasure',cyc,tt],TTMt)

def hr(out):
  outln(out,';--------------------------------------------------------------------------')

DEFR={
  'à': 'a',
  'é': 'e',
  'è': 'e',
  'ê': 'e',
  'î': 'i',
  'ï': 'i',
  'Ó': 'oe',
  'ô': 'o',
  'À': 'A',
  'Î': 'I',
  'É': 'E',
}

LETTERDIGIT=string.uppercase[:26]+string.lowercase[:26]+string.digits
VALIDCYC='-_'+LETTERDIGIT

def mtgenls(out,submt,supermt):
  isa(out,submt,'#$Microtheory',BaseKB)
  cycassert(out,['#$genlMt',submt,supermt],BaseKB)

def genls(out,subx,superx,mt):
  cycassert(out,['#$genls',subx,superx],mt)

def isa(out,subx,superx,mt):
  cycassert(out,['#$isa',subx,superx],mt)

COLL_DONE={}
def ensurecollection(out,con):
  if PRED_DONE.has_key(con[2:]):
    return None
  if CYC_CONSTS.has_key(con[2:]):
    return None
  if COLL_DONE.has_key(con[2:]):
    return None
  COLL_DONE[con[2:]]=1
  isa(out,con,'#$Collection',TTMt)

def dumpassertions1(h,out):
  mtgenls(out,TTMt,BaseKB)
  mtgenls(out,GEnglishMt,TTMt)
  mtgenls(out,GFrenchMt,TTMt)
  genls(out,'#$FrenchWord','#$LinguisticObjectType',TTMt)

def dumpassertions2(h,out):
  instream=open('../ttkb/obj.txt')
  skipheader(instream)
  while 1:
    s=instream.readline()
    if not s: break
    obj=ttkb.Obj(s[:-1])
    if not obj.objname: continue
    if obj.objname=='?': continue
    dumpobj(h,out,obj)
  return None

DONEA={}

def dumpobj(h,out,obj):
  if not obj.objname: return None
  con=ttnameToCycName(obj.objname)
  for a0 in obj.assertions:
    if bada(a0): continue
    a1=str(a0)
    if DONEA.has_key(a1): continue
    cycassert(out,ttassertionToCycAssertion(a0),TTMt)
    DONEA[a1]=1

def bada(a):
  if a[0] in ['isa','ako']:
    if len(a)!=3: return 1
    if '?' in a: return 1
    if '' in a: return 1
    if '""' in a: return 1
    if 'isa' in a[1:]: return 1
    if 'ako' in a[1:]: return 1
  return 0

def skipheader(instream):
  instream.readline()
  instream.readline()
  instream.readline()

def ttnameToCycName(objname):
  if TT_TO_CYC.has_key(objname):
    return TT_TO_CYC[objname]
  r='TT-'
  for e in objname:
    if e in VALIDCYC: r=r+e
    else: r=r+'_'
  return r

def ttassertionToCycAssertion(a):
  r=[]
  for e in a:
    if type(e)==type(''):
      if e[:8]=='"http://':
        r.append(e)
      elif e[:7]=='"ftp://':
        r.append(e)
      elif ':' in e:
        r.append(ttcolonToCyc(e))
      elif '"' in e:
        r.append(ttStringToCycString(e))
      else:
        r.append(ttTermToCycTerm(e))
    elif type(e)==type([]):
      if e[0]=='GRIDSUBSPACE':
        r.append('"GRIDSUBSPACE:'+
                 string.join(map(lambda x:x[:-1],e[1:]),':')+
                '"')
      elif e[0]=='TSRANGE':
        r.append('"TSRANGE:'+e[1]+':'+e[2]+'"')
      else:
        r.append(ttassertionToCycAssertion(e))
    else:
      print 'unhandled',e
  return r

def ttTermToCycTerm(x):
  if x[0]=='?':
    return x
  elif x[-4:]=='coor':
    return '"'+x[:-4]+'"'
  elif len(x)>1 and x[-1]=='u' and alldigit(x[:-1]):
    return x[:-1]
  elif len(x)==15 and x[8]=='T' and alldigit(x[:8]) and alldigit(x[9:]):
    return '"'+x+'"'
  else:
    return '#$'+ttnameToCycName(x)

def alldigit(x):
  for e in x:
    if e not in string.digits: return 0
  return 1

def ttStringToCycString(x):
  return '"'+string.replace(x,'"','')+'"'

def ttcolonToCyc(x):
  t=string.split(x,':')
  if len(t)!=3:
    #print 'wrong length',t
    t=(t+['','',''])[:3]
  (typ,units,x)=t
  if typ=='STRING':
    return '"'+string.replace(x,'"','')+'"'
  elif typ=='NUMBER':
    return x
  #print 'unknown ttcolon',x
  return '"'+string.replace(x,'"','')+'"'
  
def listToString(x):
  if type(x)!=type([]):
    return str(x)
  r='('
  cnt=0
  for e in x:
    if cnt!=0: r=r+' '
    r=r+listToString(e)
    cnt=cnt+1
  return r+')'

CONSTANTS={}

def createConstant(out,x):
  if CYC_CONSTS.has_key(x):
    return None
  if CONSTANTS.has_key(x):
    return None
  createConstant1(out,x)

def createConstant1(out,x):
  CONSTANTS[x]=1
  outln(out,'(create-constant "'+x+'")')

PRED_DONE={
  'genls': 1,
  'isa': 1,
}
def preddefine(out,pred):
  if PRED_DONE.has_key(pred): return None
  PRED_DONE[pred]=1
  isa(out,'#$'+pred,'#$Predicate',TTMt)
  isa(out,'#$'+pred,'#$VariableArityRelation',TTMt)

def createConstants(out,x):
  i=0
  r=[]
  for e in x:
    if type(e)==type(''):
      if i==0:
        if not CYC_CONSTS.has_key(e[2:]):
          if e[0:5]=='#$TT-':
            e=e[0:4]+'Pred'+e[4:]
          preddefine(out,e[2:])
      if e[0:2]=='#$':
        createConstant(out,e[2:])
      r.append(e)
    elif type(e)==type([]):
      r.append(createConstants(out,e))
    i=i+1
  return r

def cycassert(out,x,mt):
  x=createConstants(out,x)
  if x[0]=='#$isa':
    ensurecollection(out,x[2])
  elif x[0]=='#$genls':
    if x[1]=='#$genls': return None
    if x[2]=='#$genls': return None
    ensurecollection(out,x[1])
    ensurecollection(out,x[2])
  outln(out,wrap("(cyc-assert '"+listToString(x)+" "+mt+')'))

#-----------------------------------

def readconsts():
  d={}
  for e in open('AllConstants.txt').readlines():
    d[e[2:-1]]=1
  return d

CYC_CONSTS=readconsts()

# todo: language
def citationFormToConstant(out,citationForm):
  con=citationFormToConstant1(citationForm)
  if CYC_CONSTS.has_key(con):
    return con
  if CONSTANTS.has_key(con):
    return con
  if con!='TTWord-':
    createConstant1(out,con)
  return con

def citationFormToConstant1(s):
  r=''
  cap=1
  for e in s:
    if DEFR.has_key(e): e=DEFR[e]
    if e=='oe':
      if cap: r=r+'OE'
      else: r=r+e
      cap=0
    elif e in LETTERDIGIT:
      if cap: r=r+string.upper(e)
      else: r=r+e
      cap=0
    else:
      cap=1
  return 'TTWord-'+r

def dapp(d,key,x):
  if d.has_key(key):
    d[key].append(x)
  else:
    d[key]=[x]

def readinflections():
  d={}
  instream=open('../ttkb/infl.txt')
  skipheader(instream)
  while 1:
    s=instream.readline()
    if not s: break
    toks=string.split(s,' ')
    feat=toks[1][1:-1]
    if (tt.F_INFREQUENT not in feat): # and (tt.F_INFL_CHECKED in feat):
      dapp(d,toks[2][:-1],(string.replace(toks[0],'_',' '),feat))
  return d

LEUID2INFLS=readinflections()

def leuid2infls(leuid):
  if LEUID2INFLS.has_key(leuid):
    return LEUID2INFLS[leuid]
  return []

def leuid2citationForm(leuid):
  return ttkb.leUidToWord(leuid)

CIT2WORD={}

class Word:
  def __init__(self,out,citationForm,con):
    self.isEnglish=0
    self.isFrench=0
    self.citationForm=citationForm
    self.con=con
    self.senses=[]
    self.infls=[]

  def addsense(self,leo,feat):
    self.senses.append((len(self.senses),leo,feat))

  def addinfl(self,infl,feat):
    self.infls.append((infl,feat))

  def __repr__(self):
    return '<Word '+self.citationForm+' #$'+self.con+'>'

WORDS=[]

def wordget(out,citationForm):
  if not citationForm: return None
  if CIT2WORD.has_key(citationForm):
    return CIT2WORD[citationForm]
  con=citationFormToConstant(out,citationForm)
  if con=='TTWord-': return None
  word=Word(out,citationForm,con)
  CIT2WORD[citationForm]=word
  WORDS.append(word)
  return word

def collectwords(h,out):
  instream=open('../ttkb/le.txt')
  skipheader(instream)
  while 1:
    s=instream.readline()
    if not s: break
    le=ttkb.leCreate(s[:-1],[])
    if le!=None:
      citationForm=leuid2citationForm(le.uid)
      if citationForm!=None:
        word=wordget(out,citationForm)
        if word!=None:
          lang=le.uid[-1]
          if tt.F_FRENCH==lang: word.isFrench=1
          if tt.F_ENGLISH==lang: word.isEnglish=1
          for leo in le.leos:
            word.addsense(leo,le.feat)
          for (infl,feat) in leuid2infls(le.uid):
            word.addinfl(infl,feat)

def dumpwords(h,out):
  for word in WORDS:
    con='#$'+word.con
    if word.isEnglish:
      isa(out,con,'#$EnglishWord',GEnglishMt)
    if word.isFrench:
      isa(out,con,'#$FrenchWord',GFrenchMt)
    dumpposinflections(h,out,word,con)
    dumpdenotationtheta(h,out,word,con)

def dumpposinflections(h,out,word,con):
  for (infl,feat) in word.infls:
    lang=tt.featureGet(feat,tt.FT_LANG,'')
    pos=tt.featureGet(feat,tt.FT_POS,'')
    gmt=langToGMt(lang)
    cycpos=ttPosToCycPos(pos)
    if cycpos!=None:
      cycassert(out,['#$TT-posForms',con,cycpos],gmt)
    gender=tt.featureGet(feat,tt.FT_GENDER,'')
    person=tt.featureGet(feat,tt.FT_PERSON,'')
    number=tt.featureGet(feat,tt.FT_NUMBER,'')
    tense=tt.featureGet(feat,tt.FT_TENSE,'')
    mood=tt.featureGet(feat,tt.FT_MOOD,'')
    degree=tt.featureGet(feat,tt.FT_DEGREE,'')
    checked=tt.F_INFL_CHECKED in feat
    pred='#$TT-infl'+featsToCyc(pos,gender,person,number,tense,mood,degree,checked)
    cycassert(out,[pred,con,'"'+infl+'"'], gmt)

def ttPosToCycPos(pos):
  if pos==tt.F_ADVERB:
    return '#$Adverb'
  elif pos==tt.F_ADJECTIVE:
    return '#$Adjective'
  elif pos==tt.F_VERB:
    return '#$Verb'
  elif pos==tt.F_NOUN:
    return '#$Noun'
  elif pos==tt.F_DETERMINER:
    return '#$Determiner'
  elif pos==tt.F_PRONOUN:
    return '#$Pronoun'
  elif pos==tt.F_CONJUNCTION:
    return '#$Conjunction'
  elif pos==tt.F_PREPOSITION:
    return '#$Preposition'
  elif pos==tt.F_INTERJECTION:
    return '#$Interjection-SpeechPart'
  return None

def posToCyc(pos):
  if pos==tt.F_ADVERB:
    return 'Adverb'
  elif pos==tt.F_ADJECTIVE:
    return 'Adjective'
  elif pos==tt.F_VERB:
    return 'Verb'
  elif pos==tt.F_NOUN:
    return 'Noun'
  elif pos==tt.F_DETERMINER:
    return 'Determiner'
  elif pos==tt.F_PRONOUN:
    return 'Pronoun'
  elif pos==tt.F_CONJUNCTION:
    return 'Conjunction'
  elif pos==tt.F_PREPOSITION:
    return 'Preposition'
  elif pos==tt.F_INTERJECTION:
    return 'Interjection'
  elif pos==tt.F_ELEMENT:
    return 'Element'
  elif pos==tt.F_S_POS:
    return 'Sentence'
  elif pos==tt.F_EXPLETIVE:
    return 'Expletive'
  elif pos==tt.F_PREFIX:
    return 'Prefix'
  elif pos==tt.F_SUFFIX:
    return 'Suffix'
  return 'Other'

def genderToCyc(gender):
  if gender==tt.F_MASCULINE:
    return 'Masculine'
  elif gender==tt.F_FEMININE:
    return 'Feminine'
  elif gender==tt.F_NEUTER:
    return 'Neuter'
  return ''

def personToCyc(person):
  if person==tt.F_FIRST_PERSON:
    return 'FirstPerson'
  elif person==tt.F_SECOND_PERSON:
    return 'SecondPerson'
  elif person==tt.F_THIRD_PERSON:
    return 'ThirdPerson'
  return ''

def numberToCyc(number):
  if number==tt.F_SINGULAR:
    return 'Singular'
  elif number==tt.F_PLURAL:
    return 'Plural'
  return ''

def tenseToCyc(tense):
  if tense==tt.F_PRESENT:
    return 'Present'
  elif tense==tt.F_IMPERFECT:
    return 'Past'
  elif tense==tt.F_PASSE_SIMPLE:
    return 'PasseSimple'
  elif tense==tt.F_FUTURE:
    return 'Future'
  elif tense==tt.F_CONDITIONAL:
    return 'Conditional'
  return ''

def moodToCyc(mood):
  if mood==tt.F_SUBJUNCTIVE:
    return 'Subjunctive'
  elif mood==tt.F_IMPERATIVE:
    return 'Imperative'
  return ''

def degreeToCyc(degree):
  if degree==tt.F_POSITIVE:
    return ''
  elif degree==tt.F_COMPARATIVE:
    return 'Comparative'
  elif degree==tt.F_SUPERLATIVE:
    return 'Superlative'
  return ''

def langToGMt(lang):
  if lang==tt.F_FRENCH: return GFrenchMt
  return GEnglishMt

def featsToCyc(pos,gender,person,number,tense,mood,degree,checked):
  return posToCyc(pos)+genderToCyc(gender)+personToCyc(person)+numberToCyc(number)+tenseToCyc(tense)+moodToCyc(mood)+degreeToCyc(degree)+checkedToCyc(checked)

def checkedToCyc(checked):
  if checked: return ''
  return 'Unchecked'

#def isproper(infl):
#  if infl[0] not in string.uppercase: return 0
#  if len(infl)<2: return 0
#  return infl[1] in string.lowercase

# leo.objname
# leo.feat
# leo.thetas
def dumpdenotationtheta(h,out,word,con):
  for (sensenum,leo,lefeat) in word.senses:
    lang=tt.featureGet(lefeat,tt.FT_LANG,'')
    pos=tt.featureGet(lefeat,tt.FT_POS,'')
    gmt=langToGMt(lang)
    concept='#$'+ttnameToCycName(leo.objname)
    obj=h.objGet(leo.objname)
    cycpos=ttPosToCycPos(pos)
    if cycpos==None:
      continue
    if leo.le.word==leo.expword:
      cycassert(out,['#$TT-denotation',con,cycpos,str(sensenum),concept],gmt)
    for c in leo.feat:
      if tt.FEATDICT.has_key(c):
        pred='#$TT-thetaRoleFeat-'+toPredName(tt.FEATDICT[c])
        cycassert(out,[pred,con,cycpos,str(sensenum)],gmt)
    for theta in leo.thetas:
      slotnum=str(theta.slotnum)
      case='#$'+ttnameToCycName(theta.case)
      if theta.leUid!='':
        t=leuid2citationForm(theta.leUid)
        if word!=None:
          word='"'+t+'"'
        else:
          word='""'
      else:
        word='""'
      trpos='"'+theta.trpos+'"'
      optional=str(theta.optional)
      selres=selrestrict(h, obj, theta.slotnum)
      for c in theta.subcat:
        if tt.FEATDICT.has_key(c):
          pred='#$TT-thetaRoleSubcat-'+toPredName(tt.FEATDICT[c])
          cycassert(out,[pred,con,cycpos,str(sensenum),slotnum],gmt)
      cycassert(out,['#$TT-thetaRole',con,cycpos,str(sensenum),concept,
                     slotnum,case,selres,word,trpos,optional],gmt)

def selrestrict(h,obj,slotnum):
  if obj==None: 
    return '#$'+ttnameToCycName('concept')
  rel='r'+str(slotnum)
  t=selrestrict1(h,obj,rel,0)
  if t: return '#$'+t
  return '#$'+ttnameToCycName('concept')

def selrestrict1(h,obj,rel,depth):
  if depth>50:
    print 'selrestrict1: max depth reached'
    return None
  t=obj.assertionValues([rel],1,2)
  if len(t)>0:
    if type(t[0])!=type(''):
      return ttnameToCycName('concept')
    else:
      return ttnameToCycName(t[0])
  for parent in obj.parents():
    obj1=h.objGet(parent)
    if obj1!=None:
      t=selrestrict1(h,obj1,rel,depth+1)
      if t: return t
  return None

def toPredName(s): 
  r=''
  cap=1
  for e in s:
    if DEFR.has_key(e): e=DEFR[e]
    if e=='oe':
      if cap: r=r+'OE'
      else: r=r+e
      cap=0
    elif e in LETTERDIGIT:
      if cap: r=r+string.upper(e)
      else: r=r+e
      cap=0
    else:
      cap=1
  return r

run()

# End of file.
