all:client server

FLAG=-Wall -Wextra -Werror -pedantic -g

server:server.c
	g++ $(FLAG) server.c -o server

client:client.cpp
	g++ $(FLAG) client.cpp -o client

clean:
	rm -rf server client

