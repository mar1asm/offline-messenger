
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

extern int errno;

int port;
char *myUsername;

int main ( int argc, char *argv[] ) {
  int sd;		     // descriptorul de socket
  struct sockaddr_in server; // structura folosita pentru conectare
  char command[ 100 ], msg[ 100 ], rasp[ 100 ], username[ 100 ],
      password[ 100 ];
  int loggedIn = 0;

  if ( argc != 3 ) {
    printf ( "Sintaxa: %s <adresa_server> <port>\n", argv[ 0 ] );
    return -1;
  }

  port = atoi ( argv[ 2 ] ); /* stabilim portul */

  if ( ( sd = socket ( AF_INET, SOCK_STREAM, 0 ) ) == -1 ) {
    perror ( "Eroare la socket().\n" );
    return errno;
  }

  server.sin_family = AF_INET;			    /* familia socket-ului */
  server.sin_addr.s_addr = inet_addr ( argv[ 1 ] ); /* adresa IP a serverului */
  server.sin_port = htons ( port );		    /* portul de conectare */

  if ( connect ( sd, ( struct sockaddr * ) &server,
		 sizeof ( struct sockaddr ) ) == -1 ) {
    perror ( "[client]Eroare la connect().\n" );
    return errno;
  }
  printf ( "Connected successfully;\n" );
  printf ( "===== Wellcome to your Messenger ! =====\n" );
  printf ( "          MENU                \n" );

  while ( ! loggedIn ) {
    printf ( "         Choose your action :            \n" );
    printf ( "1.Login\n" );
    printf ( "2.Sign up\n" );
    printf ( "3.Exit\n" );

    bzero ( command, 100 );
    printf ( "Introduce your command: " );
    fflush ( stdout );
    // citirea de la tastatura
    read ( 0, command, 100 );

    if ( strstr ( command, "Login" ) != NULL ) {
      bzero ( msg, 100 );
      // username-ul
      bzero ( username, 100 );
      printf ( "Your username:" );
      fflush ( stdout );
      read ( 0, username, 100 );
      username[ strlen ( username ) - 1 ] = '\0';
      myUsername = username;

      // parola
      printf ( "Your password:" );
      bzero ( password, 100 );
      fflush ( stdout );
      read ( 0, password, 100 );
      password[ strlen ( password ) - 1 ] = '\0';

      int size = strlen ( "login" );
      if ( write ( sd, &size, sizeof ( size ) ) <= 0 ) {
	perror ( "[client]Eroare la write() catre server.\n" );
	return errno;
      }
      if ( write ( sd, "login", size ) <= 0 ) {
	perror ( "[client]Eroare la write() catre server.\n" );
	return errno;
      }

      size = strlen ( username );
      if ( write ( sd, &size, sizeof ( size ) ) <= 0 ) {
	perror ( "[client]Eroare la write() catre server.\n" );
	return errno;
      }
      if ( write ( sd, username, size ) <= 0 ) {
	perror ( "[client]Eroare la write() catre server.\n" );
	return errno;
      }

      size = strlen ( password );
      if ( write ( sd, &size, sizeof ( size ) ) <= 0 ) {
	perror ( "[client]Eroare la write() catre server.\n" );
	return errno;
      }
      if ( write ( sd, password, size ) <= 0 ) {
	perror ( "[client]Eroare la write() catre server.\n" );
	return errno;
      }

      if ( read ( sd, &size, sizeof ( size ) ) <= 0 ) {
	perror ( "[client]Eroare la read() de la server.\n" );
	return errno;
      }
      char answer[ size + 1 ];
      if ( read ( sd, answer, size ) <= 0 ) {
	perror ( "[client]Eroare la read() de la server.\n" );
	return errno;
      }
      answer[ size ] = '\0';
      if ( ! strcmp ( answer, "loggedIn" ) ) {
	loggedIn = 1;
	printf ( "Te-ai conectat cu succes la server\n" );
      } else {
	printf ( "Date de conectare gresite\n" );
      }
    }

    if ( strstr ( command, "Signup" ) != NULL ) {
      bzero ( msg, 100 );
      // username-ul
      bzero ( username, 100 );
      printf ( "Your username:" );
      fflush ( stdout );
      read ( 0, username, 100 );
      username[ strlen ( username ) - 1 ] = '\0';

      // parola
      printf ( "Your password:" );
      bzero ( password, 100 );
      fflush ( stdout );
      read ( 0, password, 100 );
      password[ strlen ( password ) - 1 ] = '\0';

      int size = strlen ( "signup" );
      if ( write ( sd, &size, sizeof ( size ) ) <= 0 ) {
	perror ( "[client]Eroare la write() catre server.\n" );
	return errno;
      }
      if ( write ( sd, "signup", size ) <= 0 ) {
	perror ( "[client]Eroare la write() catre server.\n" );
	return errno;
      }

      size = strlen ( username );
      if ( write ( sd, &size, sizeof ( size ) ) <= 0 ) {
	perror ( "[client]Eroare la write() catre server.\n" );
	return errno;
      }
      if ( write ( sd, username, size ) <= 0 ) {
	perror ( "[client]Eroare la write() catre server.\n" );
	return errno;
      }

      size = strlen ( password );
      if ( write ( sd, &size, sizeof ( size ) ) <= 0 ) {
	perror ( "[client]Eroare la write() catre server.\n" );
	return errno;
      }
      if ( write ( sd, password, size ) <= 0 ) {
	perror ( "[client]Eroare la write() catre server.\n" );
	return errno;
      }

      if ( read ( sd, &size, sizeof ( size ) ) <= 0 ) {
	perror ( "[client]Eroare la read() de la server.\n" );
	return errno;
      }
      char answer[ size + 1 ];
      if ( read ( sd, answer, size ) <= 0 ) {
	perror ( "[client]Eroare la read() de la server.\n" );
	return errno;
      }
      answer[ size ] = '\0';
      if ( ! strcmp ( answer, "signedup" ) ) {
	loggedIn = 1;
	printf ( "Cont creat cu success\n" );
      } else {
	printf ( "Exista deja un user cu acelasi username\n" );
      }
    }

    if ( strcmp ( command, "Exit" ) == 0 ) {
      break;
    }
  }

  while ( loggedIn ) {
    printf ( "         Choose your action :            \n" );
    printf ( "1.All\t See all users\n" );
    printf ( "2.Online\t See online users\n" );
    printf ( "3.Unread\t See unread messages\n" );
    printf ( "4.Send\t Send a message\n" );
    printf ( "5.Conv\t Asta e ca sa vezi conv cu un user\n" );
    printf ( "4.Exit\n" );

    bzero ( command, 100 );
    printf ( "Introduce your command: " );
    fflush ( stdout );
    // citirea de la tastatura
    read ( 0, command, 100 );

    if ( strstr ( command, "All" ) != NULL ) {
      int size = strlen ( "show all" );
      if ( write ( sd, &size, sizeof ( size ) ) <= 0 ) {
	perror ( "[client]Eroare la write() catre server.\n" );
	return errno;
      }
      if ( write ( sd, "show all", size ) <= 0 ) {
	perror ( "[client]Eroare la write() catre server.\n" );
	return errno;
      }
      int numberOfUsers;
      if ( read ( sd, &numberOfUsers, sizeof ( numberOfUsers ) ) <= 0 ) {
	perror ( "[client]Eroare la read() de la server.\n" );
	return errno;
      }

      printf ( "Sunt inregistrati %d useri:\n", numberOfUsers );
      for ( int i = 0; i < numberOfUsers; i++ ) {

	if ( read ( sd, &size, sizeof ( size ) ) <= 0 ) {
	  perror ( "[client]Eroare la read() de la server.\n" );
	  return errno;
	}
	char answer[ size + 1 ];
	if ( read ( sd, answer, size ) <= 0 ) {
	  perror ( "[client]Eroare la read() de la server.\n" );
	  return errno;
	}
	answer[ size ] = '\0';
	printf ( "%s\n", answer );
      }
    }

    if ( strstr ( command, "Online" ) != NULL ) {
      int size = strlen ( "show online" );
      if ( write ( sd, &size, sizeof ( size ) ) <= 0 ) {
	perror ( "[client]Eroare la write() catre server.\n" );
	return errno;
      }
      if ( write ( sd, "show online", size ) <= 0 ) {
	perror ( "[client]Eroare la write() catre server.\n" );
	return errno;
      }
      int numberOfUsers;
      if ( read ( sd, &numberOfUsers, sizeof ( numberOfUsers ) ) <= 0 ) {
	perror ( "[client]Eroare la read() de la server.\n" );
	return errno;
      }

      printf ( "Sunt online %d useri:\n", numberOfUsers );
      for ( int i = 0; i < numberOfUsers; i++ ) {

	if ( read ( sd, &size, sizeof ( size ) ) <= 0 ) {
	  perror ( "[client]Eroare la read() de la server.\n" );
	  return errno;
	}
	char answer[ size + 1 ];
	if ( read ( sd, answer, size ) <= 0 ) {
	  perror ( "[client]Eroare la read() de la server.\n" );
	  return errno;
	}
	answer[ size ] = '\0';
	printf ( "%s\n", answer );
      }
    }

    if ( strstr ( command, "Send" ) != NULL ) {
      int size = strlen ( "send" );
      if ( write ( sd, &size, sizeof ( size ) ) <= 0 ) {
	perror ( "[client]Eroare la write() catre server.\n" );
	return errno;
      }
      if ( write ( sd, "send", size ) <= 0 ) {
	perror ( "[client]Eroare la write() catre server.\n" );
	return errno;
      }

      size = strlen ( myUsername );
      if ( write ( sd, &size, sizeof ( size ) ) <= 0 ) {
	perror ( "[client]Eroare la write() catre server.\n" );
	return errno;
      }
      if ( write ( sd, &myUsername, size ) <= 0 ) {
	perror ( "[client]Eroare la write() catre server.\n" );
	return errno;
      }

      // cui trimit
      char sendToUsername[ 100 ];
      bzero ( sendToUsername, 100 );
      printf ( "To:" );
      fflush ( stdout );
      read ( 0, sendToUsername, 100 );
      sendToUsername[ strlen ( sendToUsername ) - 1 ] = '\0';

      size = strlen ( sendToUsername );
      if ( write ( sd, &size, sizeof ( size ) ) <= 0 ) {
	perror ( "[client]Eroare la write() catre server.\n" );
	return errno;
      }
      if ( write ( sd, sendToUsername, size ) <= 0 ) {
	perror ( "[client]Eroare la write() catre server.\n" );
	return errno;
      }

      // mesaj
      char message[ 100 ];
      printf ( "Message:" );
      bzero ( message, 100 );
      fflush ( stdout );
      read ( 0, message, 100 );
      message[ strlen ( message ) - 1 ] = '\0';

      size = strlen ( message );
      if ( write ( sd, &size, sizeof ( size ) ) <= 0 ) {
	perror ( "[client]Eroare la write() catre server.\n" );
	return errno;
      }
      if ( write ( sd, message, size ) <= 0 ) {
	perror ( "[client]Eroare la write() catre server.\n" );
	return errno;
      }
      return 0;
    }
  }

  close ( sd );
  return 0;
}

/* while(1)
    {

	    bzero(&msg, sizeof(msg));
	    bzero(&rasp, sizeof(rasp));
	    if (read(sd, &size, sizeof(size)) < 0) {
		printf("[Client] Error! Could not read message from
   [Server].\n");
	    }
	    if (read(sd, rasp, size) < 0) {
		printf("[Client] Error! Could not read message from
   [Server].\n");
	    }

	    printf("%s", rasp);
	    bzero(&rasp, sizeof(rasp));
	    read(0, msg, sizeof(msg));

	    size = (int) (strlen(msg));
	    if (write(sd, &size, sizeof(size)) <= 0) {
		printf("[Client] Error! Could not send message to [Server].\n");
	    }
	    if (write(sd, msg, size) <= 0) {
		printf("[Client] Error! Could not send message to [Server].\n");
	    }

	close (sd);
    }*/
