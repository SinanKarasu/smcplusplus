# $Id: Makefile,v 1.4 1993/06/09 16:26:50 rmartin Exp $
# Makefile modernized for clang++ on Unix/macOS

PROGRAM       = smc
CXX           = clang++
CXXFLAGS      = -std=c++20 -g
LEX           = lex
YACC          = yacc
LIBS          = -ll

YACCFILE      = smy.y
YACC_C        = $(YACCFILE:.y=.cc)
YACC_H        = $(YACCFILE:.y=.h)

LEXFILE       = sml.l
LEX_C         = $(LEXFILE:.l=.cc)

SOURCES       = sm.cc $(LEX_C) $(YACC_C)
OBJECTS       = $(SOURCES:.cc=.o)

################################################################

all: $(PROGRAM)

$(PROGRAM): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) $(LIBS)

$(LEX_C): $(LEXFILE) $(YACC_H)
	$(LEX) -t < $(LEXFILE) > $(LEX_C)

$(YACC_C) $(YACC_H): $(YACCFILE)
	$(YACC) -d $(YACCFILE)
	mv y.tab.c $(YACC_C)
	mv y.tab.h $(YACC_H)

clean:
	rm -f *.o $(PROGRAM) $(LEX_C) $(YACC_C) $(YACC_H)
	rm -rf *.dSYM
.SUFFIXES: .cc .o

.cc.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<
