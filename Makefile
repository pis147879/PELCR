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

MAXPENDING ?=
ifneq ($(strip $(MAXPENDING)),)
PENDING_FLAGS = -DMAXPENDING=$(MAXPENDING)
endif

# Build with arbitrary-precision user values using: make GMP=1 [GMP_VALUE_DIGITS=...]
ifeq ($(GMP),1)
GMP_CFLAGS  := $(shell pkg-config --cflags gmp 2>/dev/null)
GMP_LIBS    := $(shell pkg-config --libs gmp 2>/dev/null || echo -lgmp)
GMP_VALUE_DIGITS ?= 256
USER_TYPE_FLAGS = -DPELCR_GMP_USERTYPE -DMAXUSERVALUELEN=$(GMP_VALUE_DIGITS) $(GMP_CFLAGS)
FFI_TARGETS = shared
else
GMP_LIBS    =
USER_TYPE_FLAGS = -DUSERTYPE=$(BASETYPE)
FFI_TARGETS = shared crypt
endif

# Weights of Lambda*_nd may have two components ("u1|u0"); default length 100 (h/mydefs.h).
# Override with: make MAXLENWEIGHT=512
MAXLENWEIGHT ?=
ifneq ($(strip $(MAXLENWEIGHT)),)
USER_TYPE_FLAGS += -DMAXLENWEIGHT=$(MAXLENWEIGHT)
endif

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

NOW = { perl -MTime::HiRes=time -e 'printf "%.3f\n", time' 2>/dev/null || python3 -c 'import time; print("%.3f" % time.time())'; }

RUN1= printf '\043setdir "%s" ; \043open "%s"\n' "$(PEXDIR)" "$(TESTFILE)"
RUN = $(MPIR_HOME)/bin/mpirun -np $(NP) $(BUILDDIR)/$(EXECS) -- -loop $(LOOP)
RUNTEST = $(RUN1)|$(RUN)
LIB_PATH    =
LIB_LIST    = -ldl -lm -lc $(GMP_LIBS)
CFLAGS    =$(ARCHFLAGS) $(OPTFLAGS)
CCFLAGS   = $(CFLAGS)
FFLAGS    = $(OPTFLAGS)

.PHONY: default all objects gcombustion ffi shared crypt crypto go go2 test test2 check-parallel test4 test-crypto test-nd run nf clean linux mac docker iulia home FORCE

default: gcombustion ffi

all: default

lex.yy.c: $(COMPILINGRDIR)les.fl
	$(LEX) $(COMPILINGRDIR)les.fl

parser.tab.c: $(COMPILINGRDIR)parser.y
	$(YACC) -d -v $(COMPILINGRDIR)parser.y -b parser

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# a compilation error stops the build (the objects of an older build are not linked)
objects: $(BUILDDIR) $(SRCS)
	$(CC) $(CFLAGS) $(OSFLAGS) -c $(SRCS)
	mv *.o $(BUILDDIR)/
	-mv lex.yy.c parser.tab.* $(BUILDDIR)/ 2>/dev/null

gcombustion: parser.tab.c objects
	$(CLINKER) $(ARCHFLAGS) $(OPTFLAGS) $(OSFLAG) -o $(BUILDDIR)/$(EXECS) $(OBJECTS) $(LIB_PATH) $(LIB_LIST)

# the executable, rebuilt only when a source changed (used by run and nf)
$(BUILDDIR)/$(EXECS): $(filter-out lex.yy.c parser.tab.c,$(wildcard *.c)) $(wildcard h/*.h) parser.y les.fl
	$(MAKE) gcombustion

ffi: $(FFI_TARGETS)

shared: $(SHARED_LIB)

crypt crypto: $(CRYPTO_LIB)

$(SHARED_LIB): shared.c FORCE
	$(CC) $(OPTFLAGS) $(PICFLAGS) $(SHLIB_FLAGS) -o $@ $< $(GMP_LIBS)

FORCE:

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

# make check-parallel NP=16 TESTFILE=dd3.plcr: the same term on 1 rank and on NP ranks; the family
# reductions must be the same.  Prints the elapsed (wall-clock) times, the speedup T1/TNP and the
# efficiency speedup/NP (100 % = ideal speedup NP).  The times include the start of mpirun.
check-parallel:
	mkdir -p $(GMLDIR) $(LOGDIR) $(REPORTDIR) $(SCRIPTDIR)
	@seq_log=$$(mktemp $(LOGDIR)/pelcr-seq-log.XXXXXX); \
	par_log=$$(mktemp $(LOGDIR)/pelcr-par-log.XXXXXX); \
	echo "Running sequential test with $(TESTFILE)"; \
	t0=$$($(NOW)); \
	$(RUN1)|$(MPIR_HOME)/bin/mpirun $(MPIRUNFLAGS) -np 1 $(BUILDDIR)/$(EXECS) -- -loop $(LOOP) > $$seq_log 2>&1; \
	t1=$$($(NOW)); \
	echo "Running parallel test with $(TESTFILE) on $(NP) ranks"; \
	$(RUN1)|$(MPIR_HOME)/bin/mpirun $(MPIRUNFLAGS) -np $(NP) $(BUILDDIR)/$(EXECS) -- -loop $(LOOP) > $$par_log 2>&1; \
	t2=$$($(NOW)); \
	seq_family=$$(awk '/family reductions/ { value = $$NF } END { print value }' $$seq_log); \
	par_family=$$(awk '/family reductions/ { sum += $$NF } END { print sum + 0 }' $$par_log); \
	if [ -z "$$seq_family" ]; then \
		echo "Unable to extract sequential family reductions"; \
		cat $$seq_log; \
		rm -f $$seq_log $$par_log; \
		exit 1; \
	fi; \
	awk -v t0=$$t0 -v t1=$$t1 -v t2=$$t2 -v np=$(NP) 'BEGIN { \
		seq = t1 - t0; par = t2 - t1; s = (par > 0) ? seq / par : 0; \
		printf "elapsed time  1 rank : %9.3f s\n", seq; \
		printf "elapsed time %2d ranks: %9.3f s\n", np, par; \
		printf "speedup %.2f (ideal %d), efficiency %.1f %% of the ideal speedup\n", s, np, 100 * s / np }'; \
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

# NDPELCR: read-back of terms with choices against nf(t) and EX(tau(t)) of the ND-PELCR model
# (needs NDPELCR_PYTHON=.../ND-VM-PELCR/python, see agents/readback/README.md)
NDRANDOM ?= 50
test-nd: gcombustion
	python3 agents/readback/test_c_readback.py --exe $(BUILDDIR)/$(EXECS) --random $(NDRANDOM)

# Run one term with the C machine only, no Python:
#   make run FILE=nd/two-ab.plcr                     a file (in $(PEXDIR), or a path from here)
#   make run EXPR='(\f.\x.(f)(f)x)[a + b]'           a term (EXPR: TERM is the type of the terminal)
#   make run FILE=nd/delta-I-delta.plcr FIRES=3000   stop after 3000 compositions
#   make run FILE=dd4.plcr NP=16                     16 MPI ranks (mpirun; MPIRUNFLAGS=--oversubscribe
#                                                    if the machine has fewer cores)
# One rank: the final graph goes to $(RUNDIR)/<name>.json (-readback) and $(RUNDIR)/<name>.gml (-gml).
# Several ranks: each rank has its part of the graph, so no read-back; the statistics of the ranks
# are summed.  The output of PELCR goes to $(RUNDIR)/<name>.log; the statistics are printed.
# make nf ...: the same run, then the normal form, read back from the final graph by the decoder of
# the ND-PELCR article (Python, agents/readback/pelcr_c_readback.py).
PEXDIR  ?= pelcrexamples
RUNDIR  ?= $(OUTDIR)/run
FIRES   ?= 0
RUNNAME  = $(if $(strip $(EXPR)),term,$(basename $(notdir $(FILE))))
RUNFIRES = $(if $(filter-out 0,$(FIRES)),-fires $(FIRES))
RUNNP    = $(if $(strip $(NP)),$(strip $(NP)),1)
RUNMPI   = $(if $(filter-out 1,$(RUNNP)),$(MPIR_HOME)/bin/mpirun $(MPIRUNFLAGS) -np $(RUNNP))
RUNDUMP  = $(if $(filter-out 1,$(RUNNP)),,-readback $(RUNDIR)/$(RUNNAME).json -gml $(RUNDIR)/$(RUNNAME).gml)

run: $(BUILDDIR)/$(EXECS)
	@if [ -z "$(strip $(FILE)$(EXPR))" ]; then \
	  printf '%s\n' "usage: make run FILE=nd/two-ab.plcr   or   make run EXPR='(\f.\x.(f)(f)x)[a + b]'   [FIRES=N] [NP=N]"; exit 2; fi
	@mkdir -p $(LOGDIR) $(RUNDIR)
	@if [ -n '$(strip $(EXPR))' ]; then \
	  printf '%s ;\n\043quit ;\n' '$(EXPR)' > $(RUNDIR)/term.plcr; dir=$(RUNDIR); file=term.plcr; \
	elif [ -f '$(FILE)' ]; then dir=$$(dirname '$(FILE)'); file=$$(basename '$(FILE)'); \
	elif [ -f '$(PEXDIR)/$(FILE)' ]; then dir=$(PEXDIR); file='$(FILE)'; \
	else echo "$(FILE): not found (neither here nor in $(PEXDIR))"; exit 2; fi; \
	rm -f $(RUNDIR)/$(RUNNAME).json $(RUNDIR)/$(RUNNAME).gml; \
	t0=$$($(NOW)); \
	printf '\043setdir "%s" ; \043open "%s"\n' "$$dir" "$$file" | \
	  $(RUNMPI) $(BUILDDIR)/$(EXECS) -- -loop $(LOOP) $(RUNFIRES) $(RUNDUMP) > $(RUNDIR)/$(RUNNAME).log 2>&1; \
	t1=$$($(NOW)); \
	awk -v t0=$$t0 -v t1=$$t1 -v np=$(RUNNP) 'BEGIN { printf "elapsed time (%d rank%s): %.3f s\n", np, (np > 1) ? "s" : "", t1 - t0 }'; \
	if [ $(RUNNP) -gt 1 ]; then \
	  grep -a -E 'ERROR' $(RUNDIR)/$(RUNNAME).log; \
	  awk '/final nodes/ {n += $$NF; r++} /edge compositions/ {c += $$NF} /^\([0-9]+\) fires/ {f += $$NF} \
	       /family reductions/ {m += $$NF} END {if (r) printf "%d ranks: final nodes %d, edge compositions %d, fires %d, family reductions %d\n", r, n, c, f, m; \
	       else {print "no statistics"; exit 1}}' $(RUNDIR)/$(RUNNAME).log || { tail -20 $(RUNDIR)/$(RUNNAME).log; exit 1; }; \
	  echo "output: $(RUNDIR)/$(RUNNAME).log"; \
	  exit 0; fi; \
	grep -a -E 'final nodes|edge compositions|^\(0\) fires|family reductions|ERROR' $(RUNDIR)/$(RUNNAME).log; \
	if [ -f $(RUNDIR)/$(RUNNAME).json ]; then \
	  echo "final graph: $(RUNDIR)/$(RUNNAME).json, $(RUNDIR)/$(RUNNAME).gml   output: $(RUNDIR)/$(RUNNAME).log"; \
	else echo "no final graph, see $(RUNDIR)/$(RUNNAME).log:"; tail -20 $(RUNDIR)/$(RUNNAME).log; exit 1; fi

nf:
	@if [ $(RUNNP) -gt 1 ]; then echo "make nf: the read-back needs one rank (NP=1)"; exit 2; fi
	@$(MAKE) --no-print-directory run
	@python3 agents/readback/pelcr_c_readback.py $(if $(filter-out 0,$(FIRES)),--partial) $(RUNDIR)/$(RUNNAME).json

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
