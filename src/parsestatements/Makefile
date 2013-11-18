slony_subdir = src/parsestatements
slony_top_builddir = ../..
include $(slony_top_builddir)/Makefile.global

CPPFLAGS:=$(CPPFLAGS_CLIENT)

PROG=test-scanner
FLEX=flex

ifeq ($(PORTNAME), win32)
SUFFIX=.win32
endif

all: $(PROG) test

scanner.o: scanner.c scanner.h

$(PROG): $(PROG).o scanner.o
	$(CC) $(CFLAGS) -o $(PROG) test-scanner.c scanner.o

test-scanner.o: test-scanner.c

test: test-scanner
	./test-scanner < /dev/null > emptytestresult.log
#	cmp ./emptytestresult.log emptytestresult.expected$(SUFFIX)
	./test-scanner < ./test_sql.sql > test_sql.log
#	cmp ./test_sql.log ./test_sql.expected$(SUFFIX)
	./test-scanner < ./cstylecomments.sql > cstylecomments.log
#	cmp ./cstylecomments.log ./cstylecomments.expected$(SUFFIX)

install:

maintainer-clean: clean
distclean: clean

clean:
	rm -f scanner.o test-scanner test-scanner.o
	rm -f *.log
