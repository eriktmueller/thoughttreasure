#!/usr/bin/python

import cgi
import ttkb
import os

os.environ['TTROOT']='..'
form = cgi.FieldStorage()

# Collect options
options=[]
for o in form.keys():
  if o[0]=='-':
    options.append(o)

qtype=''
qword=''
if form.has_key('text2script'):
  qword = form['text'].value
  qtype = '-text2script'
elif form.has_key('qword') and form.has_key('qtype'):
  qword = form['qword'].value
  qtype = '-'+form['qtype'].value
else:
  if form.has_key('obj'):
    qword = form['obj'].value
    qtype = '-obj'
  if form.has_key('le'):
    qword = form['le'].value
    qtype = '-le'
  if form.has_key('word'):
    qword = form['word'].value
    qtype = '-word'

print 'Content-type: text/html'
print
print '<html>'
print '<head>'
print '<title>ThoughtTreasure knowledge base ('+qtype+' '+qword+')</title>'
print '</head>'
print '<body bgcolor="#ffffff">'
print '<HR NOSHADE SIZE=5>'

try:
  ttkb.cmd(['ttkb',qword,qtype,'-html']+options)
except:
  print 'An error occurred processing the query:'
  print '<pre>'
  for key in form.keys():
    print key+': &lt;'+form[key].value+'&gt;'
  print '</pre>'

print '<HR NOSHADE SIZE=3>'
print '<A HREF="query.htm">New query</A><BR>'
print '</body>'
print '</html>'
