CC=clang++
CFLAGS = -Wall -g
CPPFLAGS = -std=c++11
LDFLAGS= -g
LDLIBS=

all: dcso

dcso: main.o parse_ast.o logic_ast.o
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

%.o: %.cc $(wildcard *.h)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

clean:
	rm *.o
	rm dcso
