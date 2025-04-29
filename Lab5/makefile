flags = -Wall -Wextra -Wpedantic -std=c11
daemon_server: daemon_server.c
	gcc daemon_server.c -o daemon_server $(flags)

client: client.c
	gcc client.c -o client $(flags)

all: daemon_server client

clean:
	rm -rf daemon_server
	rm -rf client
