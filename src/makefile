#
# ThoughtTreasure
# Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
# All Rights Reserved.
#
CC		= gcc
PLATFORM	= GCC
STDLIBS		= -lm
CFLAGS		= -D$(PLATFORM) -g -Wno-invalid-source-encoding
#
# End of user configurable options
#
MAKE		= make
BINDIR		= ../bin
BINARIES	= tt
STDOBJS		= appassoc.o \
		appsimul.o \
		apptrans.o \
		compnoun.o \
		lexalgm.o \
		lexanam.o \
		lextheta.o \
		lexentry.o \
		lexitem.o \
		lexmorph.o \
		lexobjle.o \
		lexwf1.o \
		lexwf2.o \
		lexwf3.o \
		pa.o \
		paatrans.o \
		pacloth.o \
		paenter.o \
		pagrasp.o \
		pamtrans.o \
		paphone.o \
		paptrans.o \
		repactor.o \
		repbasic.o \
		repchan.o \
		repcloth.o \
		repcxt.o \
		repdb.o \
                repdbf.o \
                repfifo.o \
		repgrid.o \
		repgroup.o \
		repprove.o \
		repmisc.o \
		repobj.o \
		repobjl.o \
		repspace.o \
		repstr.o \
		repsubgl.o \
		reptext.o \
		reptime.o \
		reptrip.o \
		semanaph.o \
		semaspec.o \
		semcase.o \
		semdisc.o \
		semgen1.o \
		semgen2.o \
		semparse.o \
		semstyle.o \
		semtense.o \
		synbase.o \
		synfilt.o \
		synparse.o \
		synpnode.o \
		syntrans.o \
		synxbar.o \
		ta.o \
		taemail.o \
		tale.o \
		taname.o \
		taprod.o \
		tatable.o \
		tatagger.o \
		tatime.o \
		toolapi.o \
		toolcorp.o \
		toolfilt.o \
		toolgrd.o \
		toolprove.o \
		toolrpt.o \
		toolsh.o \
		toolstat.o \
		toolsvr.o \
		tooltest.o \
		ua.o \
		uaanal.o \
		uaasker.o \
		uadict.o \
		uaemot.o \
		uafriend.o \
		uagoal.o \
		uaoccup.o \
		uaquest.o \
		uarel.o \
		uascript.o \
		uaspace.o \
		uatime.o \
		uatrade.o \
		uaweath.o \
		uapaappt.o \
		uapagroc.o \
		uapashwr.o \
		uapaslp.o \
		utilbb.o \
		utilchar.o \
		utildbg.o \
		utilhtml.o \
		utillrn.o \
		utilmain.o \
                utilrpt.o \
		utiltype.o

all:		$(BINARIES)

tt:		$(STDOBJS)
		$(CC) -o tt $(CFLAGS) $(STDOBJS) $(STDLIBS)

install:	
		mv tt tt.exe $(BINDIR)

clean:		
		rm -f *~ *.o $(BINARIES) log core out* display tmp gmon.out

appassoc.o:		appassoc.c tt.h
appsimul.o:		appsimul.c tt.h
apptrans.o:		apptrans.c tt.h
compnoun.o:		compnoun.c tt.h
lexalgm.o:		lexalgm.c tt.h
lexanam.o:		lexanam.c tt.h
lextheta.o:		lextheta.c tt.h
lexentry.o:		lexentry.c tt.h
lexitem.o:		lexitem.c tt.h
lexmorph.o:		lexmorph.c tt.h
lexobjle.o:		lexobjle.c tt.h
lexwf1.o:		lexwf1.c tt.h
lexwf2.o:		lexwf2.c tt.h
lexwf3.o:		lexwf3.c tt.h
pa.o:			pa.c tt.h
paatrans.o:		paatrans.c tt.h
pacloth.o:		pacloth.c tt.h
paenter.o:		paenter.c tt.h
pagrasp.o:		pagrasp.c tt.h
pamtrans.o:		pamtrans.c tt.h
paphone.o:		paphone.c tt.h
paptrans.o:		paptrans.c tt.h
repactor.o:		repactor.c tt.h
repbasic.o:		repbasic.c tt.h
repchan.o:		repchan.c tt.h
repcloth.o:		repcloth.c tt.h
repcxt.o:		repcxt.c tt.h
repdb.o:		repdb.c tt.h
repdbf.o:		repdbf.c tt.h
repfifo.o:		repfifo.c tt.h
repgrid.o:		repgrid.c tt.h
repgroup.o:		repgroup.c tt.h
repprove.o:		repprove.c tt.h
repmisc.o:		repmisc.c tt.h
repobj.o:		repobj.c tt.h
repobjl.o:		repobjl.c tt.h
repspace.o:		repspace.c tt.h
repstr.o:		repstr.c tt.h
repsubgl.o:		repsubgl.c tt.h
reptext.o:		reptext.c tt.h
reptime.o:		reptime.c tt.h
reptrip.o:		reptrip.c tt.h
semanaph.o:		semanaph.c tt.h
semaspec.o:		semaspec.c tt.h
semcase.o:		semcase.c tt.h
semdisc.o:		semdisc.c tt.h
semgen1.o:		semgen1.c tt.h
semgen2.o:		semgen2.c tt.h
semparse.o:		semparse.c tt.h
semstyle.o:		semstyle.c tt.h
semtense.o:		semtense.c tt.h
synbase.o:		synbase.c tt.h
synfilt.o:		synfilt.c tt.h
synparse.o:		synparse.c tt.h
synpnode.o:		synpnode.c tt.h
syntrans.o:		syntrans.c tt.h
synxbar.o:		synxbar.c tt.h
ta.o:			ta.c tt.h
taemail.o:		taemail.c tt.h
tale.o:			tale.c tt.h
taname.o:		taname.c tt.h
taprod.o:		taprod.c tt.h
tatable.o:		tatable.c tt.h
tatagger.o:		tatagger.c tt.h
tatime.o:		tatime.c tt.h
toolapi.o:		toolapi.c tt.h
toolcorp.o:		toolcorp.c tt.h
toolfilt.o:		toolfilt.c tt.h
toolgrd.o:		toolgrd.c tt.h
toolprove.o:		toolprove.c tt.h
toolrpt.o:		toolrpt.c tt.h
toolsh.o:		toolsh.c tt.h
toolstat.o:		toolstat.c tt.h
toolsvr.o:		toolsvr.c tt.h
tooltest.o:		tooltest.c tt.h
ua.o:			ua.c tt.h
uaanal.o:		uaanal.c tt.h
uaasker.o:		uaasker.c tt.h
uadict.o:		uadict.c tt.h
uaemot.o:		uaemot.c tt.h
uafriend.o:		uafriend.c tt.h
uagoal.o:		uagoal.c tt.h
uaoccup.o:		uaoccup.c tt.h
uaquest.o:		uaquest.c tt.h
uarel.o:		uarel.c tt.h
uascript.o:		uascript.c tt.h
uaspace.o:		uaspace.c tt.h
uatime.o:		uatime.c tt.h
uatrade.o:		uatrade.c tt.h
uaweath.o:		uaweath.c tt.h
uapaappt.o:		uapaappt.c tt.h
uapagroc.o:		uapagroc.c tt.h
uapashwr.o:		uapashwr.c tt.h
uapaslp.o:		uapaslp.c tt.h
utilbb.o:		utilbb.c tt.h
utilchar.o:		utilchar.c tt.h
utildbg.o:		utildbg.c tt.h
utilhtml.o:		utilhtml.c tt.h
utillrn.o:		utillrn.c tt.h
utilmain.o:		utilmain.c tt.h
utilrpt.o:		utilrpt.c tt.h
utiltype.o:		utiltype.c tt.h

# End of file.
