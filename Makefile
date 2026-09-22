# Tscheme: A Tiny Scheme Interpreter
# Copyright (c) 1995-2013 Takuo WATANABE (Tokyo Institute of Technology)
# 
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
# 
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

HDRS = tscheme.h
SRCS = main.c storage.c object.c eval.c subrs.c io.c error.c misc.c read.c
OBJS = $(SRCS:%.c=%.o)
TARGET = tscheme
INITSCM = init.scm

# PREFIX = /usr/local
PREFIX = /opt
BINDIR = $(PREFIX)/bin
LIBDIR = $(PREFIX)/lib/tscheme

CC = gcc
DBGFLAGS = -g #-DDEBUG
OPTFLAGS =
CFLAGS = -std=c99 -pedantic -Wall -Werror $(DBGFLAGS) $(OPTFLAGS)
CPPFLAGS = -DINIT_FILE=\"$(LIBDIR)/$(INITSCM)\"
LDFLAGS =

RM = rm -f


MKINIT_CMD = '(begin (load "simplify.scm") (sys:make-init "init.scm"))'
MKINIT_CMD0 = '(begin (load "simplify.scm") (sys:make-init "init0.scm"))'
MKINIT_CMD1 = '(begin (load "simplify.scm") (sys:make-init "init1.scm"))'

%.o: %.c $(HDRFILES)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

.PHONY: all clean allclean remake-init0

all: $(TARGET) $(INITSCM)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

init.scm: init0.scm $(TARGET)
	echo $(MKINIT_CMD1) | ./$(TARGET) -i init0.scm
	echo $(MKINIT_CMD) | ./$(TARGET) -i init1.scm
	diff -s init.scm init1.scm

remake-init0: init-src.scm simplify.scm
	echo $(MKINIT_CMD0) | ./$(TARGET)

install: $(TARGET) $(INITSCM)
	install -d $(BINDIR) $(LIBDIR)
	install -c -s $(TARGET) $(BINDIR)
	install -c $(INITSCM) $(LIBDIR)

clean:
	$(RM) $(TARGET)
	$(RM) $(OBJS)
	$(RM) $(INITSCM)
	$(RM) init1.scm

allclean: clean
	$(RM) *~
	$(RM) *.o
	$(RM) core *.core

# -*- EOF -*-
