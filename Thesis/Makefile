# Makefile for building latex documents (c) Darko Veberic
# $Id: Makefile 3255 2014-01-14 10:20:10Z veberic $

USEPDFLATEX := 1
USEBIBTEX := 1

# TARGET := $(basename $(shell grep -le '^\s*\\document\(class\|style\)' *.tex))
TARGET := main
DEPENDENCIES := $(wildcard ./*.tex figures/* chap/*.tex app/*.tex include/* title/*)

##########################################################
SHELL := bash
DEPENDENCIES += $(wildcard *.sty chap/*.tex title/*.tex abstract/*.tex) 

LATEX := $(if $(filter 1, $(USEPDFLATEX)), pdflatex --shell-escape, latex)

define basic_latex_rule
  for i in 1 2 3 ; do $(LATEX) $< && break; done 
endef
# $< | $(COLORIZE) && break ; done

ifeq ($(USEBIBTEX), 1)
define latex_rule
  $(LATEX) $< ; bibtex $*
  $(call basic_latex_rule)
endef
else
define latex_rule
  $(call basic_latex_rule)
endef
endif

.PHONY: all clean

all: $(TARGET).pdf

# dependencies
$(TARGET).pdf: $(DEPENDENCIES)

clean:
	- $(RM) -f $(TARGET){.pdf,.tex~,.ps,.dvi,.log,.aux,.toc,.bcf,.nav,.out,.snm,.bbl,.blg,.spl,.lof,.maf,.fff,.lot,.ttt,.mtc*,.ptc,.loc,.tdo,.soc} */*.aux

# general rules
ifeq ($(USEPDFLATEX), 1)
%.pdf: %.tex $(DEPENDENCIES)
	$(call latex_rule)

%.ps: %.pdf
	pdf2ps $<
else
%.dvi: %.tex $(DEPENDENCIES)
	$(call latex_rule)

%.pdf: %.dvi
	dvipdf $<

%.ps: %.dvi
	dvips $< -o
endif

# embed pdf fonts
%.embedded_fonts.pdf: %.pdf
	gs -dSAFER -dNOPLATFONTS -dNOPAUSE -dBATCH -sDEVICE=pdfwrite -sPAPERSIZE=a4 -dCompatibilityLevel=1.4 -dPDFSETTINGS=/printer -dCompatibilityLevel=1.4 -dMaxSubsetPct=100 -dSubsetFonts=true -dEmbedAllFonts=true -sOutputFile=$@ -f $<
