
CXX = c++
CXXFLAGS = -fPIC -Wall -std=c++11 -O2 -g

EXEC = evaluate

OBJS = main.o evaluator.o

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CXX) $(CXXFLAGS)   -o $@ $^

clean:
	rm -f $(OBJS) $(EXEC)
