# Makefile for building latex documents (c) Darko Veberic
# $Id: Makefile 3255 2014-01-14 10:20:10Z veberic $

USEPDFLATEX := 1
USEBIBTEX := 1

# TARGET := $(basename $(shell grep -le '^\s*\\document\(class\|style\)' *.tex))
TARGET := main
DEPENDENCIES := $(wildcard ./*.tex figures/* chap/*.tex app/*.tex include/*)

##########################################################
SHELL := bash
DEPENDENCIES += $(wildcard *.sty *.bib chap/*.tex title/*.tex abstract/*.tex) 

LATEX := $(if $(filter 1, $(USEPDFLATEX)), pdflatex --shell-escape, latex)

COLORIZE := awk 'BEGIN{red="\033[31;1m";yel="\033[33m";byel="\033[33;1m";end="\033[0m"}/Rerun /||/rerunfilecheck[^.]/{e=e$$0"\n";print red""$$0""end;next}/^LaTeX Warning:/||/natbib Warning:/{w=w$$0"\n";print yel""$$0""end;next}{print}END{if(w||e)print "\n"red"Warning/error summary:\n"byel""w""end;if(e){print red""e""end;exit 1}}'

define basic_latex_rule
  for i in 1 2 3 ; do $(LATEX --shell-escape) 
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
	- $(RM) -f $(TARGET){.tex~,.ps,.pdf,.dvi,.log,.aux,.toc,.nav,.out,.snm,.bbl,.blg,.spl,.lof,.maf,.fff,.lot,.ttt} */*.aux

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
