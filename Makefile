NAME = 9cc
SRCS = main.cpp
OBJS = $(SRCS:.cpp=.o)
CXX  = c++
CXXFLAGS = -std=c++20 -g -static -Wall -Wextra

.PHONY: all
all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

.PHONY: test
test: $(NAME)
	./test.bash ./$<

.PHONY: clean
clean:
	rm -f ./bin/9cc *.o *~ *tmp*
