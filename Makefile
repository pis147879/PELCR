include Makefile.head

################ common part

OBJECTS   = parser.tab.o lex.yy.o read_back.o dvm.o io.o graph.o symbolic.o distribution.o print.o main.o buildgraph.o combustion.o
PARSERSRC = lex.yy.c parser.tab.c
SRCS      = lex.yy.c parser.tab.c $(COMPILINGRDIR)read_back.c $(COMPILINGRDIR)dvm.c  $(COMPILINGRDIR)io.c $(COMPILINGRDIR)graph.c $(COMPILINGRDIR)symbolic.c $(COMPILINGRDIR)distribution.c $(COMPILINGRDIR)print.c $(COMPILINGRDIR)main.c $(COMPILINGRDIR)buildgraph.c $(COMPILINGRDIR)combustion.c

BASETYPE='long long'
TESTFILE    = "dd4.plcr"

RUN1= echo "\#setdir \"$(PEXDIR)\" ; \#open \"$(TESTFILE)\""
RUN = $(MPIR_HOME)/bin/mpirun -np $(NP) $(EXECS) -- -I ciccio -loop 10000000 -o GML/prova -v -t
RUNTEST = $(RUN1)|$(RUN)
LIB_PATH    =
LIB_LIST    = -ldl -lm -ll -lc
CFLAGS    =$(ARCHFLAGS) $(OPTFLAGS)
CCFLAGS   = $(CFLAGS)
FFLAGS    = $(OPTFLAGS)

default: gcombustion

all: default

lex.yy.c: $(COMPILINGRDIR)les.fl
	$(LEX) $(COMPILINGRDIR)les.fl

y.tab.c: $(COMPILINGRDIR)parser.y
	$(YACC) -d -v $(COMPILINGRDIR)parser.y -b y

objects: $(SRCS)
	$(CC) $(CFLAGS) $(OSFLAGS) -c $(SRCS)

gcombustion: y.tab.c objects
	$(CLINKER) $(ARCHFLAGS) $(OPTFLAGS) $(OSFLAG) -o $(EXECS) $(OBJECTS) $(LIB_PATH) $(LIB_LIST)

go:	NP=1
go:
	$(RUN)

go2: NP=2
go2:
	$(RUN)

test: NP=1
test:
	$(RUNTEST)

test2: NP=2
test2:
	$(RUNTEST)

test4: NP=4
test4:
	$(RUNTEST)	

clean:
	/bin/rm -f y.output y.tab.h y.tab.c lex.yy.c *.o run.*.log core combustion combustion.home combustion.ultra combustion.linux combustion.capital combustion.mac *.log combustion*.tex *.idx *.aux *.scn combustion*.dvi *.toc *~ DAT/*.* GML/* h/*~ ./Crypto/*.o ./Crypto/*.so *.so

linux:
	rm Makefile.head
	ln -s Makefile.linux Makefile.head
	make all

mac:
	rm Makefile.head
	ln -s Makefile.mac Makefile.head
	make all
 
docker:
	rm Makefile.head
	ln -s Makefile.docker Makefile.head
	make all
	
home: gcombustion
