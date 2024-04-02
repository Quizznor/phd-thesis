# General Makefile for the OfflineUser package
#
# If the environment variable AUGEROFFLINEROOT is not set
# AND the executable 'auger-offline-config' is not in your PATH
# the definition of the following variable is required
#
# AUGEROFFLINEROOT = @prefix@
#
# Replace the wildcard expression with .cc file list if you do
# not want to compile all .cc files in this directory
#
USER_SRCS := $(wildcard *.cc)
#
# All .xml.in files will be transformed into .xml with correct
# config and schema locations
#
USER_XMLS := $(patsubst %.xml.in,%.xml,$(wildcard *.xml.in))
#
# Give your executable a name
#
EXE := userAugerOffline
#
#############################################################

## You should not need to change anything below this line ###

# Authors: T. Paul, S. Argiro, L. Nellen, D. Veberic
# Send bug reports to http://www.auger.unam.mx/bugzilla/

SHELL := bash

.PHONY: all depend clean

ifndef SD_SIM_CALIBRATION_N_SIMULATIONS
  SD_SIM_CALIBRATION_N_SIMULATIONS := 5000
endif

ifdef AUGEROFFLINEROOT
  AUGEROFFLINECONFIG := $(AUGEROFFLINEROOT)/bin/auger-offline-config
else
  AUGEROFFLINECONFIG := auger-offline-config
endif

OBJS := $(USER_SRCS:.cc=.o)

CPPFLAGS := $(shell $(AUGEROFFLINECONFIG) --cppflags)
CXXFLAGS := $(shell $(AUGEROFFLINECONFIG) --cxxflags)
LDFLAGS := $(shell $(AUGEROFFLINECONFIG) --ldflags)
MAIN := $(shell $(AUGEROFFLINECONFIG) --main)
CONFIGFILES := $(shell $(AUGEROFFLINECONFIG) --config)
XMLSCHEMALOCATION := $(shell $(AUGEROFFLINECONFIG) --schema-location)

all: $(EXE) $(USER_XMLS)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(MAIN) $(CXXFLAGS) $(LDFLAGS) -lMinuit

%: %.in
	@echo -n "Generating $@ file..." 
	@sed -e 's!@''CONFIGDIR@!$(CONFIGFILES)!g;s!@''SCHEMALOCATION@!$(XMLSCHEMALOCATION)!g;s!@''SD_SIM_CALIBRATION_N_SIMULATIONS@!$(SD_SIM_CALIBRATION_N_SIMULATIONS)!g' $< >$@
	@echo "done"

#############################################################
# gcc can generate the dependency list

depend: Make-depend

Make-depend: $(USER_SRCS)
	$(CPP) $(CPPFLAGS) -MM $^ >$@

clean:
	- rm -f *.o  *.so *.ps core $(USER_XMLS) Make-depend

#############################################################
# 'make run' will run the thing

run_all_cases: $(EXE) $(USER_XMLS)
	for b in bootstrap_*.xml ; do \
          log="$$(sed 's/bootstrap_/run_/;s/\.xml/.log/' <<< "$$b")" ;\
          ./$(EXE) -b $$b |& tee $$log ;\
        done

run: $(EXE) $(USER_XMLS)
	for b in bootstrap_UUB_Scintillator_G4StationSimulatorOG.xml bootstrap_UUB_WCDLarge_G4StationSimulatorOG.xml bootstrap_UUB_WCDSmall_G4StationSimulatorOG.xml; do \
          log="$$(sed 's/bootstrap_/run_/;s/\.xml/.log/' <<< "$$b")" ;\
          ./$(EXE) -b $$b |& tee $$log ;\
        done

#############################################################
# the lines below are for running with debugger 'make run_gdb'

.INTERMEDIATE: gdb.cmdl

# batch mode gdb needs a file with commands
gdb.cmdl:
	echo "r -b bootstrap.xml" >$@

run_gdb: gdb.cmdl $(EXE) $(USER_XMLS)
	gdb -batch -x $< ./$(EXE) && touch $@

-include Make-depend
