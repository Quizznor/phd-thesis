# Makefile for building latex documents via biber

SHELL := bash
TARGET := main
BIBTEX := biber
LATEX := latexmk -pdf
DEPENDENCIES := $(wildcard *.tex abstract/*.tex chap/*.tex app/*.tex figures/* include/*.tex)
DEPENDENCIES += $(wildcard *.sty *.bib)

##########################################################

.PHONY: all clean

all: $(TARGET).pdf

clean:
	- $(RM) -f $(TARGET){.fls,.fdb_latexmk,.pdf,.tex~,.ps,.dvi,.log,.aux,.toc,.bcf,.nav,.out,.snm,.bbl,.blg,-blx.bib,.tex.bbl,.tex.blg,.run.xml,.spl,.lof,.maf,.fff,.lot,.ttt,.mtc*,.ptc,.loc,.tdo,.soc} */*.aux

%.pdf: $(DEPENDENCIES)
	scripts/gather_acronyms
	scripts/gather_chapters
	scripts/gather_appendices
	$(LATEX) $*
