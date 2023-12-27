# Makefile for building latex documents via biber

SHELL := bash
TARGET := main
BIBTEX := biber
LATEX := pdflatex --shell-escape
DEPENDENCIES := $(wildcard *.tex abstract/*.tex chap/*.tex app/*.tex figures/*)
DEPENDENCIES += $(wildcard *.sty *.bib)

##########################################################

COLORIZE := awk 'BEGIN{red="\033[31;1m";yel="\033[33m";byel="\033[33;1m";end="\033[0m"}/Rerun /||/rerunfilecheck[^.]/{e=e$$0"\n";print red""$$0""e    nd;next}/^LaTeX Warning:/||/natbib Warning:/{w=w$$0"\n";print yel""$$0""end;next}{print}END{if(w||e)print "\n"red"Warning/error summary:\n"byel""w    ""end;if(e){print red""e""end;exit 1}}'

.PHONY: all clean

all: $(TARGET).pdf

clean:
	- $(RM) -f $(TARGET){.pdf,.tex~,.ps,.dvi,.log,.aux,.toc,.bcf,.nav,.out,.snm,.bbl,.blg,-blx.bib,.tex.bbl,.tex.blg,.run.xml,.spl,.lof,.maf,.fff,.lot,.ttt,.mtc*,.ptc,.loc,.tdo,.soc} */*.aux

%.pdf: $(DEPENDENCIES)
	scripts/gather_chapters
	scripts/gather_appendices
	$(LATEX) $*
	$(BIBTEX) $*
	$(LATEX) $*
	$(LATEX) $*
