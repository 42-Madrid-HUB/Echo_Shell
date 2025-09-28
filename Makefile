NAME = exam-shell
SRCS = srcs/main.cpp srcs/shell.cpp srcs/utils.cpp srcs/log.cpp srcs/menu.cpp srcs/norm.cpp srcs/debug.cpp
OBJS = $(SRCS:.cpp=.o)
CXX = g++
CXXFLAGS = -Wall -Wextra -Werror -std=c++17 -pthread

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -lreadline -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
