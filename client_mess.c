
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <termios.h>
#include <unistd.h>

#define RED "\x1B[31m"
#define GREEN "\x1B[32m"
#define BLUE "\x1B[34m"
#define NORMAL "\x1B[0m"
#define YEL "\x1b[33m"
#define BYEL "\x1b[43m"
#define CYAN "\x1b[36m"

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
    perror ( RED " [client]Eroare la write() catre server.\n" NORMAL );
    return errno;
  }
  if ( write ( sd, message, size ) <= 0 ) {
    perror ( RED "[client]Eroare la write() catre server.\n" NORMAL );
    return errno;
  }
  return 0;
}

char *receiving ( ) {
  int size;
  char *error = "error";
  if ( read ( sd, &size, sizeof ( size ) ) <= 0 ) {
    perror ( RED "[client]Eroare la write() catre server.\n" NORMAL );
    return error;
  }

  char *message = ( char * ) malloc ( size + 1 );
  if ( read ( sd, message, size ) <= 0 ) {
    perror ( RED "[client]Eroare la write() catre server.\n" NORMAL );
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

  static struct termios old_terminal;
  static struct termios new_terminal;
  tcgetattr ( STDIN_FILENO, &old_terminal );
  new_terminal = old_terminal;
  new_terminal.c_lflag &= ~( ECHO );
  tcsetattr ( STDIN_FILENO, TCSANOW, &new_terminal );

  char password[ 100 ];
  printf ( "Your password: " );
  bzero ( password, 100 );
  fflush ( stdout );
  if ( fgets ( password, BUFSIZ, stdin ) == NULL )
    password[ 0 ] = '\0';
  else
    password[ strlen ( password ) - 1 ] = '\0';
  tcsetattr ( STDIN_FILENO, TCSANOW, &old_terminal );
  printf (
      "\nIn scopul prezentarii proiectului, parola introdusa a fost: %s \n",
      password );

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
    printf ( GREEN "Te-ai conectat cu succes la server\n" NORMAL );
  } else {
    printf ( RED "Date de conectare gresite\n" NORMAL );
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

  static struct termios old_terminal;
  static struct termios new_terminal;
  tcgetattr ( STDIN_FILENO, &old_terminal );
  new_terminal = old_terminal;
  new_terminal.c_lflag &= ~( ECHO );
  tcsetattr ( STDIN_FILENO, TCSANOW, &new_terminal );

  // parola
  char password[ 100 ];
  printf ( "Your password: " );
  bzero ( password, 100 );
  fflush ( stdout );
  if ( fgets ( password, BUFSIZ, stdin ) == NULL )
    password[ 0 ] = '\0';
  else
    password[ strlen ( password ) - 1 ] = '\0';

  printf ( "\nConfirm password: " );
  char confirmPassword[ 100 ];
  bzero ( confirmPassword, 100 );
  fflush ( stdout );
  if ( fgets ( confirmPassword, BUFSIZ, stdin ) == NULL )
    confirmPassword[ 0 ] = '\0';
  else
    confirmPassword[ strlen ( confirmPassword ) - 1 ] = '\0';

  printf ( "\n In scopul prezentarii proiectului, parolele introduse au fost: "
	   "%s , %s\n",
	   password, confirmPassword );

  tcsetattr ( STDIN_FILENO, TCSANOW, &old_terminal );

  if ( strcmp ( password, confirmPassword ) ) {
    printf ( RED "Parolele introduse nu sunt la fel!\n" NORMAL );
    return 0;
  }

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
    printf ( GREEN "Cont creat cu success\n" NORMAL );
  } else {
    printf ( RED "Exista deja un user cu acelasi username\n" NORMAL );
  }
  return 0;
}

int showAllUsers ( ) {

  if ( sending ( "show all" ) )
    return errno;

  int numberOfUsers;
  if ( read ( sd, &numberOfUsers, sizeof ( numberOfUsers ) ) < 0 ) {
    perror ( RED "[client] Eroare la read de la server" NORMAL );
    return errno;
  }

  printf ( "Sunt inregistrati %d useri:\n", numberOfUsers );
  for ( int i = 0; i < numberOfUsers; i++ ) {

    char *user;
    if ( ! strcmp ( ( user = receiving ( ) ), "error" ) )
      return errno;
    printf ( BLUE "%s\n" NORMAL, user );
  }
  return 0;
}

int showOnlineUsers ( ) {
  if ( sending ( "show online" ) )
    return -1;

  int numberOfUsers;
  if ( read ( sd, &numberOfUsers, sizeof ( numberOfUsers ) ) < 0 ) {
    perror ( RED "[client] Eroare la read de la server" NORMAL );
    return errno;
  }

  printf ( "Sunt online %d useri:\n", numberOfUsers );
  for ( int i = 0; i < numberOfUsers; i++ ) {

    char *user;
    if ( ! strcmp ( ( user = receiving ( ) ), "error" ) )
      return errno;
    printf ( "%s\n", user );
  }
  printf ( "\n" );
  return 0;
}

int requestUnreadMessages ( ) {
  if ( sending ( "unread" ) )
    return errno;
  if ( sending ( myUsername ) )
    return errno;

  int numberOfNewMessages;
  if ( read ( sd, &numberOfNewMessages, sizeof ( numberOfNewMessages ) ) < 0 ) {
    perror ( RED "[client] Eroare la read de la server" NORMAL );
    return errno;
  }

  printf ( YEL "Ai %d mesaje noi.\n" NORMAL, numberOfNewMessages );
  for ( int i = 0; i < numberOfNewMessages; i++ ) {
    char *user;
    if ( ! strcmp ( ( user = receiving ( ) ), "error" ) )
      return errno;
    char *sentAt;
    if ( ! strcmp ( ( sentAt = receiving ( ) ), "error" ) )
      return errno;
    printf ( "Ai un mesaj nou de la " YEL "%s" NORMAL " trimis la " YEL
	     "%s" NORMAL "\n",
	     user, sentAt );
    char *message;
    if ( ! strcmp ( ( message = receiving ( ) ), "error" ) )
      return errno;
    printf ( "Mesajul este:\n" YEL "%s" NORMAL "\n", message );
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
	  printf ( GREEN "Mesaj trimis cu succes\n" NORMAL );
	} else {
	  printf (
	      RED
	      "Eroare la trimiterea mesajului catre utilizatorul %s\n" NORMAL,
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
    printf ( GREEN "Mesaj trimis cu succes\n" NORMAL );
  } else {
    printf ( RED
	     "Eroare la trimiterea mesajului catre utilizatorul %s\n " NORMAL,
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
    perror ( RED "[client] Eroare la read de la server" NORMAL );
    return errno;
  }

  for ( int i = 0; i < numberOfMessages; i++ ) {
    char *sender;
    if ( ! strcmp ( ( sender = receiving ( ) ), "error" ) )
      return errno;
    char *sentAt;
    if ( ! strcmp ( ( sentAt = receiving ( ) ), "error" ) )
      return errno;
    char *chatMessage;
    if ( ! strcmp ( ( chatMessage = receiving ( ) ), "error" ) )
      return errno;
    printf ( YEL "%s:" NORMAL " %s (at: " YEL "%s" NORMAL ")\n", sender,
	     chatMessage, sentAt );
  }
  return 0;
}

int main ( int argc, char *argv[] ) {
  struct sockaddr_in server; // structura folosita pentru conectare
  char command[ 100 ];
  int loggedIn = 0;
  int quitted = 0;

  if ( argc != 3 ) {
    printf ( RED "Sintaxa: %s <adresa_server> <port>\n" NORMAL, argv[ 0 ] );
    return -1;
  }

  port = atoi ( argv[ 2 ] ); /* stabilim portul */

  if ( ( sd = socket ( AF_INET, SOCK_STREAM, 0 ) ) == -1 ) {
    perror ( RED "Eroare la socket().\n" NORMAL );
    return errno;
  }

  server.sin_family = AF_INET;			    /* familia socket-ului */
  server.sin_addr.s_addr = inet_addr ( argv[ 1 ] ); /* adresa IP a serverului */
  server.sin_port = htons ( port );		    /* portul de conectare */

  if ( connect ( sd, ( struct sockaddr * ) &server,
		 sizeof ( struct sockaddr ) ) == -1 ) {
    perror ( RED "[client]Eroare la connect().\n" NORMAL );
    return errno;
  }

  while ( ! quitted ) {
    printf ( "Connected successfully;\n" );
    printf ( "===== Welcome to your Messenger ! =====\n" );
    printf ( "          MENU                \n" );

    while ( ! loggedIn ) {
      printf ( "         Choose your action :            \n" );
      printf ( CYAN "1.Login\n" NORMAL );
      printf ( CYAN "2.Signup\n" NORMAL );
      printf ( CYAN "3.Exit\n" NORMAL );
      printf ( "\n" );

      bzero ( command, 100 );
      printf ( YEL ">" NORMAL );
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
      printf ( "1. " CYAN "All\t\t" NORMAL "See all users\n" );
      printf ( "2. " CYAN "Online\t" NORMAL "See online users\n" );
      printf ( "3. " CYAN "Unread\t" NORMAL "See unread messages\n" );
      printf ( "4. " CYAN "Send\t\t" NORMAL "Send a message\n" );
      printf ( "5. " CYAN "Conv\t\t" NORMAL
	       "Asta e ca sa vezi conv cu un user\n" );
      printf ( "6. " CYAN "Logout\t" NORMAL
	       "Te scote din cont dar nu termina programul\n" );
      printf ( "7. " CYAN "Exit\t\t" NORMAL "Termina programul.\n" );

      bzero ( command, 100 );
      printf ( YEL "%s>" NORMAL, myUsername );
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
	sending ( "exit" );
	loggedIn = 0;
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
