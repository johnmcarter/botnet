all:  bot server 
	
bot: bot.c 
	gcc -Wall -pthread -g -o bot bot.c

server: server.c
	gcc -Wall -pthread -g -o server server.c

clean: 
	rm bot server 