example: example.cpp
	g++ -Wall -Wextra -Werror -std=c++14 -o "$@" "$<"

clean:
	rm -f example
