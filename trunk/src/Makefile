# Makefile für AProf

# SIM_CLI
#  Activates a shell simulation in case the profiler is started from
#  the Workbench.

# Global compiler flags, i.e. for SourceLevel & Optimised modules
CCFLAGS = -fm -wc -mcd -dSIM_CLI=1
# Only optimised modules
COFLAGS = $(CCFLAGS) -so
# Only SourceLevel modules
CFLAGS = $(CCFLAGS) -bs

AFLAGS = -c -d
LNFLAGS = -w +a +s

INCS = dsp.h pro.h

# Rule for optimised modules
.c.oo:
   cc $(COFLAGS) -o $@ $*.c
	
#oobjs : Optimiesed modules - no source level debugging
OOBJS	= FreeDosObject.oo p3traphandler.oo p3menu.oo \
          p3amigaguide.oo p3rdsym.oo p3text.oo p3gui.oo\
          p3prefs.oo p3exec.oo p3break.oo\
          p3rexx.oo p3search.oo dsp.oo p3symdis.oo\
          p3load.oo p3funcs.oo p3timer.oo 

# Standard modules SDB possible (C/ASS)
OBJS = p3main.o p3trap.o p3xseglist.o
 
AProf: $(OOBJS) $(OBJS)
   ln -t -o AProf $(LNFLAGS) $(OOBJS) -g $(OBJS) -lbinzl -lml -lcl

# Additional dependecies
p3funcs.oo: version.h
p3funcs.o: version.h
p3main.oo: version.h
p3main.o: version.h


$(OBJS) $(OOBJS): $(INCS)

