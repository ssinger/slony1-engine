PROG=test-scanner
FLEX=flex

all: $(PROG) test

scanner.o: scanner.c scanner.h

$(PROG): $(PROG).o scanner.o
	$(CC) $(CFLAGS) -o $(PROG) test-scanner.c scanner.o

test-scanner.o: test-scanner.c

test: test-scanner
	./test-scanner < /dev/null > emptytestresult.log
	cmp ./emptytestresult.log emptytestresult.expected
	./test-scanner < ./test_sql.sql > test_sql.log
	cmp ./test_sql.log ./test_sql.expected

install:
