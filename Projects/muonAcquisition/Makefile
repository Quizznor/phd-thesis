# GPLv2 and 2C-BSD
# Copyright (c) Darko Veberic, 2014

hdf5 := $(shell auger-offline-config --cppflags | awk -v FS='External' '/Geant4/{print $$1}')

CPPFLAGS := -I. -I./src -I/usr/include $(shell auger-offline-config --cppflags) \
						$(hdf5)External/hdf5/1.10.1/include
CXXFLAGS := -v -ggdb3 -Wall -Wextra -Wtype-limits -std=c++17 -O0 \
            -fno-inline -fno-omit-frame-pointer -fsanitize=address \

LDFLAGS := $(shell auger-offline-config --ldflags)

SHELL := bash
SUFFIXES := .o .cc .cxx

EXES := $(basename $(wildcard *.cxx))
OBJS := $(patsubst %.cc, %.o, $(wildcard *.cc)  $(wildcard ./src/*.cc)) 
DEPS := $(patsubst %.o, %.P, $(OBJS)) $(addsuffix .P, $(EXES))

define cxx_compile_with_dependency_creation
  $(COMPILE.cc) -MD -o $@ $<
  @sed -e 's|.*:|$*.o:|' <$*.d >$*.P
  @sed -e 's/.*://' -e 's/\\$$//' <$*.d | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >>$*.P
  @rm -f $*.d
endef

define cxx_link_rule
  $(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@
endef

%.o: %.cc
	$(call cxx_compile_with_dependency_creation)

%.o: %.cxx
	$(call cxx_compile_with_dependency_creation)

%: %.o
	$(call cxx_link_rule)

.PHONY: all
all: $(EXES)

.PHONY: clean
clean:
	- $(RM) -f $(OBJS) $(addsuffix .o, $(EXES)) $(EXES) $(DEPS)

-include $(DEPS)

.PHONY: release
release: CXXFLAGS := -ggdb3 -Wall -Wextra -Wtype-limits -std=c++17 -O3
release: $(EXES)
