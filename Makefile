include Makefile.head

################ common part

BUILDDIR    = build
OBJECTS     = $(BUILDDIR)/parser.tab.o $(BUILDDIR)/lex.yy.o $(BUILDDIR)/read_back.o $(BUILDDIR)/dvm.o $(BUILDDIR)/io.o $(BUILDDIR)/graph.o $(BUILDDIR)/symbolic.o $(BUILDDIR)/distribution.o $(BUILDDIR)/print.o $(BUILDDIR)/main.o $(BUILDDIR)/buildgraph.o $(BUILDDIR)/combustion.o $(BUILDDIR)/globals.o
PARSERSRC = lex.yy.c parser.tab.c
SRCS      = lex.yy.c parser.tab.c $(COMPILINGRDIR)read_back.c $(COMPILINGRDIR)dvm.c  $(COMPILINGRDIR)io.c $(COMPILINGRDIR)graph.c $(COMPILINGRDIR)symbolic.c $(COMPILINGRDIR)distribution.c $(COMPILINGRDIR)print.c $(COMPILINGRDIR)main.c $(COMPILINGRDIR)buildgraph.c $(COMPILINGRDIR)combustion.c $(COMPILINGRDIR)globals.c

GMLDIR      = GML
LOGDIR      = LOGS
REPORTDIR   = REPORTS
SCRIPTDIR   = scripts
OUTDIR      = OUTPUT

BASETYPE='long long'
TESTFILE    = dd3.plcr
LOOP        = 10000000
UNAME_S     := $(shell uname -s)

SHARED_LIB  = shared.so
CRYPTO_DIR  = $(PEXDIR)/Crypto
CRYPTO_LIB  = $(CRYPTO_DIR)/crypto.so
CRYPTO_TESTFILE = Crypto/pkzipBrute.plcr

ifeq ($(UNAME_S),Darwin)
SHLIB_FLAGS = -dynamiclib -undefined dynamic_lookup
else
SHLIB_FLAGS = -shared
endif

PICFLAGS    = -fPIC

RUN1= printf '\043setdir "%s" ; \043open "%s"\n' "$(PEXDIR)" "$(TESTFILE)"
RUN = $(MPIR_HOME)/bin/mpirun -np $(NP) $(BUILDDIR)/$(EXECS) -- -loop $(LOOP)
RUNTEST = $(RUN1)|$(RUN)
LIB_PATH    =
LIB_LIST    = -ldl -lm -lc
CFLAGS    =$(ARCHFLAGS) $(OPTFLAGS)
CCFLAGS   = $(CFLAGS)
FFLAGS    = $(OPTFLAGS)

.PHONY: default all objects gcombustion ffi shared crypt crypto go go2 test test2 check-parallel test4 test-crypto clean linux mac docker iulia home

default: gcombustion ffi

all: default

lex.yy.c: $(COMPILINGRDIR)les.fl
	$(LEX) $(COMPILINGRDIR)les.fl

parser.tab.c: $(COMPILINGRDIR)parser.y
	$(YACC) -d -v $(COMPILINGRDIR)parser.y -b parser

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

objects: $(BUILDDIR) $(SRCS)
	$(CC) $(CFLAGS) $(OSFLAGS) -c $(SRCS) && mv *.o $(BUILDDIR)/ && mv lex.yy.c parser.tab.* $(BUILDDIR)/ 2>/dev/null || true

gcombustion: parser.tab.c objects
	$(CLINKER) $(ARCHFLAGS) $(OPTFLAGS) $(OSFLAG) -o $(BUILDDIR)/$(EXECS) $(OBJECTS) $(LIB_PATH) $(LIB_LIST)

ffi: shared crypt

shared: $(SHARED_LIB)

crypt crypto: $(CRYPTO_LIB)

$(SHARED_LIB): shared.c
	$(CC) $(OPTFLAGS) $(PICFLAGS) $(SHLIB_FLAGS) -o $@ $<

$(CRYPTO_LIB): $(CRYPTO_DIR)/crypto.c $(CRYPTO_DIR)/Swap/SwapEndian.c $(CRYPTO_DIR)/Swap/SwapEndian.h
	$(CC) $(OPTFLAGS) $(PICFLAGS) -I$(CRYPTO_DIR) $(SHLIB_FLAGS) -o $@ $(CRYPTO_DIR)/crypto.c $(CRYPTO_DIR)/Swap/SwapEndian.c

go:	NP=1
go:
	$(RUN)

go2: NP=2
go2:
	$(RUN)

test: NP=1
test:
	mkdir -p $(GMLDIR) $(LOGDIR) $(REPORTDIR) $(SCRIPTDIR)
	$(RUNTEST)

test2: NP=2
test2:
	mkdir -p $(GMLDIR) $(LOGDIR) $(REPORTDIR) $(SCRIPTDIR)
	$(RUNTEST)

check-parallel:
	mkdir -p $(GMLDIR) $(LOGDIR) $(REPORTDIR) $(SCRIPTDIR)
	seq_log=$$(mktemp $(LOGDIR)/pelcr-seq-log.XXXXXX); \
	par_log=$$(mktemp $(LOGDIR)/pelcr-par-log.XXXXXX); \
	echo "Running sequential test with $(TESTFILE)"; \
	$(RUN1)|$(MPIR_HOME)/bin/mpirun -np 1 $(BUILDDIR)/$(EXECS) -- -loop $(LOOP) > $$seq_log 2>&1; \
	echo "Running parallel test with $(TESTFILE) on $(NP) ranks"; \
	$(RUN1)|$(MPIR_HOME)/bin/mpirun -np $(NP) $(BUILDDIR)/$(EXECS) -- -loop $(LOOP) > $$par_log 2>&1; \
	seq_family=$$(awk '/family reductions/ { value = $$NF } END { print value }' $$seq_log); \
	par_family=$$(awk '/family reductions/ { sum += $$NF } END { print sum + 0 }' $$par_log); \
	if [ -z "$$seq_family" ]; then \
		echo "Unable to extract sequential family reductions"; \
		cat $$seq_log; \
		rm -f $$seq_log $$par_log; \
		exit 1; \
	fi; \
	if [ "$$par_family" -ne "$$seq_family" ]; then \
		echo "Family reductions mismatch: sequential=$$seq_family parallel_sum=$$par_family"; \
		echo "--- Sequential log ---"; \
		cat $$seq_log; \
		echo "--- Parallel log ---"; \
		cat $$par_log; \
		rm -f $$seq_log $$par_log; \
		exit 1; \
	fi; \
	echo "Family reductions match: $$seq_family"; \
	rm -f $$seq_log $$par_log

test4: NP=4
test4:
	mkdir -p $(GMLDIR) $(LOGDIR) $(REPORTDIR) $(SCRIPTDIR)
	$(RUNTEST)

test-crypto: NP=1
test-crypto: TESTFILE=$(CRYPTO_TESTFILE)
test-crypto: gcombustion crypt
	mkdir -p $(GMLDIR) $(LOGDIR) $(REPORTDIR) $(SCRIPTDIR)
	$(RUNTEST)

clean:
	/bin/rm -f y.output run.*.log core *.log *~ *~ OUTPUT/*.*  h/*~ $(SHARED_LIB) $(CRYPTO_LIB) $(CRYPTO_DIR)/*.o $(CRYPTO_DIR)/Swap/*.o
	/bin/rm -rf $(BUILDDIR) $(LOGDIR) $(REPORTDIR) $(OUTDIR)

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

iulia:
	rm Makefile.head
	ln -s Makefile.iulia Makefile.head
	make all
	
home: gcombustion
