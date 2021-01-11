all:
	gcc -o client client_mess.c `mysql_config --cflags --libs`
	gcc -o server server_messenger.c  `mysql_config --cflags --libs`
clean:
	rm -f *~client server