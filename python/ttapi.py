#
# ThoughtTreasure
# Python-based client API talks to ThoughtTreasure server
#
# Copyright 1999, 2015 Erik Thomas Mueller. All Rights Reserved.
#
# Examples:
# import tt
# import ttapi
# ttc = ttapi.TTConnection('myhostname', ttapi.PORT)
# ttc.ISA('physical-object','garage')
# ttc.isPartOf('fingernail','human')
# ttc.parents('Evian')
# ttc.children('phase-of-life')
# ttc.ancestors('Evian')
# ttc.descendants('phase-of-life')
# ttc.retrieve(2, 1, -1, 'anc', 'min-value-of eat-dinner ?')
# ttc.retrieve(-1, 1, -1, 'anc', 'min-value-of eat-dinner ?')
# ttc.retrieve(2, 1, -1, 'anc', 'r1 prep-into ?')
# t = ttc.phraseToConcepts(tt.F_ENGLISH, 'on top of')
# t[0].__dict__
# t[0].objname
# tt.featureGet(t[0].leo.lexentry.features, tt.FT_POS, tt.F_NULL)
# ttc.conceptToLexEntries('garage')
# t = ttc.tag(tt.F_ENGLISH, 'Bertie kerplunked the cereal into the basket.')
# t[1].leo.objname
# ttc.syntacticParse(tt.F_ENGLISH, 'Bertie likes cereal.')
# ttc.semanticParse(tt.F_ENGLISH, 'Bertie likes cereal.')
# ttc.generate(tt.F_ENGLISH, '[blue cereal]')
# ttc.chatterbot(tt.F_ENGLISH, 'Is the sky blue?')
# ttc.clearContext()
# ttc.status()
# ttc.bringdown()
# ttc.status()
# ttc.clearCache()
# ttc.close()
#

from socket import *
from string import strip, split
import sys
import tt

# The ThoughtTreasure Server Protocol (TTSP) registered port number
# listed by the Internet Assigned Numbers Authority (IANA).
PORT=1832

class TTConnection:
  def __init__(self, host, port):
    self.host = host
    self.port = port
    self.skt = socket(AF_INET, SOCK_STREAM)
    self.skt.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)
    self.skt.connect((host, port))
    self.cache = {}
    self.USECACHE = 1
    self.errout = sys.stdout

  def __del__(self):
    self.close()

  def __repr__(self):
    return '<TTConnection '+self.host+' '+str(self.port)+'>'

  def ISA(self, classname, objname):
    return self.query('ISA '+strip(classname)+' '+strip(objname)+'\n',
                      'Boolean')

  def isPartOf(self, part, whole):
    return self.query('IsPartOf '+strip(part)+' '+strip(whole)+'\n',
                      'Boolean')

  def parents(self, objname):
    return self.query('Parents '+strip(objname)+'\n', 'ColonString')

  def children(self, objname):
    return self.query('Children '+strip(objname)+'\n', 'ColonString')

  def ancestors(self, objname):
    return self.query('Ancestors '+strip(objname)+'\n', 'ColonString')

  def descendants(self, objname):
    return self.query('Descendants '+strip(objname)+'\n', 'ColonString')

  def retrieve(self, picki, anci, desci, mode, ptn):
    return self.query('Retrieve '+str(picki)+' '+str(anci)+
                      ' '+str(desci)+' '+strip(mode)+
                      ' '+strip(ptn)+'\n',
                      'AssertionLines')

  def phraseToConcepts(self, feat, phrase):
    return self.query('PhraseToConcepts '+strip(feat)+
                      ' '+strip(phrase)+'\n',
                      'Leos')

  def conceptToLexEntries(self, objname):
    return self.query('ConceptToLexEntries '+strip(objname)+'\n',
                      'Leos')

  def tag(self, feat, text):
    return self.query('Tag '+strip(feat)+' '+text+' \n',# REALLY trailing space
                      'PNodes')

  def syntacticParse(self, feat, text):
    return self.query('SyntacticParse '+strip(feat)+
                      ' '+strip(text)+' \n',# REALLY trailing space
                      'AssertionLines')

  def semanticParse(self, feat, text):
    return self.query('SemanticParse '+strip(feat)+
                      ' '+strip(text)+' \n',# REALLY trailing space
                      'AssertionLines')

  def generate(self, feat, assertion):
    return self.query('Generate '+strip(feat)+' '+strip(assertion)+'\n',
                      'String')

  def chatterbot(self, feat, text):
    return self.query('Chatterbot '+strip(feat)+' '+strip(text)+' \n',
                      'String') # REALLY trailing space

  def clearContext(self):
    return self.query1('ClearContext\n', 'Boolean')

  def status(self):
    try:
      return self.query1('Status\n', 'String')
    except:
      return "down"

  def bringdown(self):
    self.skt.send('Bringdown\n')
    return 1

  def clearCache(self):
    self.cache = {}

  def close(self):
    if self.skt:
      self.skt.close()
      self.skt = None

  # Derived functions -- not part of TTSP

  def phraseISA(self, classname, feat, phrase):
    for e in self.phraseToConcepts(feat, phrase):
      if self.ISA(classname, e.objname): return 1
    return 0

  def relValue(self, prop, obj, default, numberClass):
    for e in self.retrieve(2, 1, -1, 'anc', prop+' '+obj+' ?'):
      r=tt.number(e, numberClass)
      if r: return r
    return default

  def phraseToConceptNames(self, feat, phrase):
    return map((lambda x: x.objname), self.phraseToConcepts(feat, phrase))

  def surfaceArea(self, objname):
    h = self.relValue('height-of', objname, 0.0, 'unit-of-distance')
    w = self.relValue('width-of', objname, 0.0, 'unit-of-distance')
    l = self.relValue('length-of', objname, 0.0, 'unit-of-distance')
    return max([h*w,h*l,w*l])

  def getContrast(self, obj0, contrast):
    for obj in self.parents(obj0):
      if contrast in self.parents(obj): return obj
      t=self.getContrast(obj, contrast)
      if t: return t
    return None

  def getContrasts(self, obj0, contrast):
    r = []
    for obj in self.parents(obj0):
      if contrast in self.parents(obj): r.append(obj)
      else:
        for e in self.getContrasts(obj, contrast): r.append(e)
    return r

  # Internal functions
  def query(self, query, queryType):
    if self.USECACHE:
      if self.cache.has_key(query): return self.cache[query]
      r = self.query1(query, queryType)
      self.cache[query] = r;
      return r
    else:
      return self.query1(query, queryType)

  def query1(self, query, queryType):
    if queryType == "Boolean":
      return self.queryBoolean(query)
    elif queryType == "String":
      return self.queryString(query)
    elif queryType == "ColonString":
      return self.queryColonString(query)
    elif queryType == "AssertionLines":
      return self.queryAssertionLines(query)
    elif queryType == "Leos":
      return self.queryLeos(query)
    elif queryType == "PNodes":
      return self.queryPNodes(query)
    else:
      self.errout.write('unknown queryType <'+queryType+'>')
      return None

  def queryBoolean(self, query):
    self.skt.send(query)
    line = self.rdline()
    if line[0:6] == 'error:':
      self.errout.write(line+'\n')
      return 0
    return int(line)

  def queryString(self, query):
    self.skt.send(query)
    line = self.rdline()
    if line[0:6] == 'error:':
      self.errout.write(line+'\n')
      return ""
    return strip(line)

  def queryColonString(self, query):
    self.skt.send(query)
    line = self.rdline()
    if line[0:6] == 'error:':
      self.errout.write(line+'\n')
      return 0
    if line=='': return []
    return split(line, ':')

  def queryAssertionLines(self, query):
    self.skt.send(query)
    return map(tt.assertionParse,self.rdlines())

  def queryLeos(self, query):
    self.skt.send(query)
    r = []
    lines = self.rdlines()
    for x in lines:
      line = split(x, ':')
      le = TTLexEntry(line[0], line[1], line[2], line[3]);
      leo = TTLexEntryToObj(le, line[4], line[5]);
      r.append(leo)
    return r

  def queryPNodes(self, query):
    self.skt.send(query)
    r = []
    lines = self.rdlines()
    for x in lines:
      line = split(x, ':')
      le = TTLexEntry(line[0], line[1], line[2], line[3]);
      leo = TTLexEntryToObj(le, line[4], line[5])
      pn = TTPNode(featureGet(le.features, FT_POS, F_NULL),
                   leo, int(line[8]), int(line[9]));
      r.append(pn)
    return r

  def rdline(self):
    r = '';
    while 1:
      c = self.skt.recv(1); # todo: buffering
      if c == '\n': return r
      r = r+c

  def rdlines(self):
    r = [];
    while 1:
      line = self.rdline()
      if line[0:6] == 'error:':
        errout.write(line+'\n')
        return []
      if line == '.': break
      r.append(line)
    return r

class TTPNode:
  def __init__(self, feature, leo, startpos, endpos):
    self.feature = feature
    self.leo = leo
    self.startpos = startpos
    self.endpos = endpos
  def __repr__(self):
    r = self.feature
    if self.leo != None: r = r+':'+str(self.leo)
    if self.startpos != -1: r = r+':'+str(self.startpos)
    if self.endpos != -1: r = r+':'+str(self.endpos)
    return r

class TTLexEntry:
  def __init__(self, citationForm, features, inflection, inflFeatures):
    self.citationForm = citationForm
    self.features = features
    self.inflection = inflection
    self.inflFeatures = inflFeatures
  def __repr__(self):
    return (self.citationForm+':'+
            self.features+':'+
            self.inflection+':'+
            self.inflFeatures)

class TTLexEntryToObj:
  def __init__(self, lexentry, objname, features):
    self.lexentry = lexentry
    self.objname = objname
    self.features = features
  def __repr__(self):
    return (str(self.lexentry)+':'+
            self.objname+':'+
            self.features)

# End of file.
