
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
int sd; // descriptorul de socket

int sending (
    char message[] ) { // ca sa nu scriu de 100 ori aceeasi chestie, am facut o
		       // functie care trimite mesajul la server. returneaza 0
		       // daca totul e ok si altceva in caz de eroare
  int size = strlen ( message );
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

char *receiving ( ) {
  int size;
  char *error = "error";
  if ( read ( sd, &size, sizeof ( size ) ) <= 0 ) {
    perror ( "[client]Eroare la write() catre server.\n" );
    return error;
  }

  char *message = ( char * ) malloc ( size + 1 );
  if ( read ( sd, message, size ) <= 0 ) {
    perror ( "[client]Eroare la write() catre server.\n" );
    free ( message );
    return error;
  }

  message[ size ] = '\0';
  return message;
}

int login ( int *loggedIn ) {

  char username[ 100 ];
  bzero ( username, 100 );
  printf ( "Your username:" );
  fflush ( stdout );
  read ( 0, username, 100 );

  username[ strlen ( username ) - 1 ] = '\0';
  char password[ 100 ];
  bzero ( password, 100 );
  printf ( "Your password:" ); // add terminos ca sa nu se vada parola
  fflush ( stdout );
  read ( 0, password, 100 );

  password[ strlen ( password ) - 1 ] = '\0';
  if ( sending ( "login" ) )
    return errno;

  if ( sending ( username ) )
    return errno;

  if ( sending ( password ) )
    return errno;

  char *answer;
  if ( ! strcmp ( answer = receiving ( ), "error" ) )
    return errno;

  if ( ! strcmp ( answer, "loggedIn" ) ) {
    *loggedIn = 1;
    myUsername = ( char * ) malloc ( strlen ( username ) + 1 );
    strcpy ( myUsername, username );
    myUsername[ strlen ( username ) ] = '\0';
    printf ( "Te-ai conectat cu succes la server\n" );
  } else {
    printf ( "Date de conectare gresite\n" );
  }
  return 0;
}

int signup ( ) { // todo: sa introduc de 2 ori parola si sa apara cu stelute sau
		 // deloc
  char username[ 100 ];
  // username-ul
  bzero ( username, 100 );
  printf ( "Your username:" );
  fflush ( stdout );
  read ( 0, username, 100 );
  username[ strlen ( username ) - 1 ] = '\0';

  // parola
  char password[ 100 ];
  printf ( "Your password:" );
  bzero ( password, 100 );
  fflush ( stdout );
  read ( 0, password, 100 );
  password[ strlen ( password ) - 1 ] = '\0';

  if ( sending ( "signup" ) )
    return errno;

  if ( sending ( username ) )
    return errno;

  if ( sending ( password ) )
    return errno;

  char *answer;
  if ( ! strcmp ( ( answer = receiving ( ) ), "error" ) )
    return errno;

  if ( ! strcmp ( answer, "signedup" ) ) {
    printf ( "Cont creat cu success\n" );
  } else {
    printf ( "Exista deja un user cu acelasi username\n" );
  }
  return 0;
}

int showAllUsers ( ) {

  if ( sending ( "show all" ) )
    return errno;

  int numberOfUsers;
  if ( read ( sd, &numberOfUsers, sizeof ( numberOfUsers ) ) < 0 ) {
    perror ( "[client] Eroare la read de la server" );
    return errno;
  }

  printf ( "Sunt inregistrati %d useri:\n", numberOfUsers );
  for ( int i = 0; i < numberOfUsers; i++ ) {

    char *user;
    if ( ! strcmp ( ( user = receiving ( ) ), "error" ) )
      return errno;
    printf ( "%s\n", user );
  }
  return 0;
}

int showOnlineUsers ( ) {
  if ( sending ( "show online" ) )
    return -1;

  int numberOfUsers;
  if ( read ( sd, &numberOfUsers, sizeof ( numberOfUsers ) ) < 0 ) {
    perror ( "[client] Eroare la read de la server" );
    return errno;
  }

  printf ( "Sunt online %d useri:\n", numberOfUsers );
  for ( int i = 0; i < numberOfUsers; i++ ) {

    char *user;
    if ( ! strcmp ( ( user = receiving ( ) ), "error" ) )
      return errno;
    printf ( "%s\n", user );
  }
  return 0;
}

int requestUnreadMessages ( ) {
  if ( sending ( "unread" ) )
    return errno;
  if ( sending ( myUsername ) )
    return errno;

  int numberOfNewMessages;
  if ( read ( sd, &numberOfNewMessages, sizeof ( numberOfNewMessages ) ) < 0 ) {
    perror ( "[client] Eroare la read de la server" );
    return errno;
  }
  printf ( "Ai %d mesaje noi.\n", numberOfNewMessages );
  for ( int i = 0; i < numberOfNewMessages; i++ ) {
    char *user;
    if ( ! strcmp ( ( user = receiving ( ) ), "error" ) )
      return errno;
    printf ( "Ai un mesaj nou de la %s\n", user );
    char *message;
    if ( ! strcmp ( ( message = receiving ( ) ), "error" ) )
      return errno;
    printf ( "Mesajul este:\n%s\n", message );
    int ok = 0;
    do {
      printf ( "Doresti sa raspunzi? yes/no\n" );
      char answer[ 100 ];
      bzero ( answer, 100 );
      fflush ( stdout );
      read ( 0, answer, 100 );
      answer[ strlen ( answer ) - 1 ] = '\0';

      if ( ! strcmp ( answer, "yes" ) ) {
	ok = 1;
	printf ( "Scrie mesajul\n" );
	char answer[ 100 ];
	bzero ( answer, 100 );
	fflush ( stdout );
	read ( 0, answer, 100 );
	answer[ strlen ( answer ) - 1 ] = '\0';
	if ( sending ( "send" ) )
	  return errno;

	if ( sending ( myUsername ) )
	  return errno;

	if ( sending ( user ) )
	  return errno;

	if ( sending ( answer ) )
	  return errno;

	char *answerFromServer;
	if ( ! strcmp ( ( answerFromServer = receiving ( ) ), "error" ) )
	  return errno;

	if ( ! strcmp ( answerFromServer, "sent" ) ) {
	  printf ( "Mesaj trimis cu succes\n" );
	} else {
	  printf ( "Eroare la trimiterea mesajului catre utilizatorul %s\n",
		   user );
	}
      }

      if ( ! strcmp ( answer, "no" ) ) {
	ok = 1;
	if ( sending ( "skip" ) )
	  return errno;
      }
    } while ( ! ok );
  }
  return 0;
}

int sendMessageToClient ( ) {
  if ( sending ( "send" ) )
    return errno;

  if ( sending ( myUsername ) )
    return errno;

  // cui trimit
  char sendToUsername[ 100 ];
  bzero ( sendToUsername, 100 );
  printf ( "To:" );
  fflush ( stdout );
  read ( 0, sendToUsername, 100 );
  sendToUsername[ strlen ( sendToUsername ) - 1 ] = '\0';

  if ( sending ( sendToUsername ) )
    return errno;

  // mesaj
  char message[ 100 ];
  printf ( "Message:" );
  bzero ( message, 100 );
  fflush ( stdout );
  read ( 0, message, 100 );
  message[ strlen ( message ) - 1 ] = '\0';

  if ( sending ( message ) )
    return errno;

  char *answer;
  if ( ! strcmp ( ( answer = receiving ( ) ), "error" ) )
    return errno;

  if ( ! strcmp ( answer, "sent" ) ) {
    printf ( "Mesaj trimis cu succes\n" );
  } else {
    printf ( "Eroare la trimiterea mesajului catre utilizatorul %s\n",
	     sendToUsername );
  }
  return 0;
}

int requestConversation ( ) {
  if ( sending ( "view" ) )
    return errno;
  if ( sending ( myUsername ) )
    return errno;
  char message[ 100 ];
  printf ( "With:" );
  bzero ( message, 100 );
  fflush ( stdout );
  read ( 0, message, 100 );
  message[ strlen ( message ) - 1 ] = '\0';

  if ( sending ( message ) )
    return errno;
  int numberOfMessages;
  if ( ( read ( sd, &numberOfMessages, sizeof ( numberOfMessages ) ) ) < 0 ) {
    perror ( "[client] Eroare la read de la server" );
    return errno;
  }

  for ( int i = 0; i < numberOfMessages; i++ ) {
    char *sender;
    if ( ! strcmp ( ( sender = receiving ( ) ), "error" ) )
      return errno;
    char *chatMessage;
    if ( ! strcmp ( ( chatMessage = receiving ( ) ), "error" ) )
      return errno;
    printf ( "%s: %s", sender, chatMessage );
  }
}

int main ( int argc, char *argv[] ) {
  struct sockaddr_in server; // structura folosita pentru conectare
  char command[ 100 ];
  int loggedIn = 0;
  int quitted = 0;

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

  while ( ! quitted ) {
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
      command[ strlen ( command ) - 1 ] = '\0';

      if ( ! strcmp ( command, "Login" ) ) {
	// username-ul
	if ( login ( &loggedIn ) )
	  return -1;
      }

      if ( ! strcmp ( command, "Signup" ) ) {
	if ( signup ( ) )
	  return -1;
      }

      if ( ! strcmp ( command, "Exit" ) ) {
	sending ( "exit" );
	quitted = 1;
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
      printf ( "6.Logout\t Te scote din cont dar nu termina programul\n" );
      printf ( "7.Exit\t Termina programul.\n" );

      bzero ( command, 100 );
      printf ( "Introduce your command: " );
      fflush ( stdout );
      // citirea de la tastatura
      read ( 0, command, 100 );
      command[ strlen ( command ) - 1 ] = '\0';

      if ( ! strcmp ( command, "All" ) ) {
	if ( showAllUsers ( ) )
	  return -1;
      }

      if ( ! strcmp ( command, "Online" ) ) {
	if ( showOnlineUsers ( ) )
	  return -1;
      }

      if ( ! strcmp ( command, "Send" ) ) {
	if ( sendMessageToClient ( ) )
	  return errno;
      }

      if ( ! strcmp ( command, "Unread" ) ) {
	if ( requestUnreadMessages ( ) )
	  return errno;
      }
      if ( ! strcmp ( command, "Conv" ) ) {
	if ( requestConversation ( ) )
	  return errno;
      }
      if ( ! strcmp ( command, "Logout" ) ) {
	loggedIn = 0;
	break;
      }

      if ( ! strcmp ( command, "Exit" ) ) {
	sending ( "exit" );
	quitted = 1;
	break;
      }
    }
  }

  close ( sd );
  return 0;
}
