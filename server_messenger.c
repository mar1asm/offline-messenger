
#include <errno.h>
#include <mysql/mysql.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <wait.h>

/* portul folosit */
#define PORT 2025

int sendOnlineUsers ( int client, MYSQL * );
int sendAllUsers ( int client, MYSQL * );
int sendUnreadMessages ( int client, MYSQL * );
int sendMessage ( int client, MYSQL * );
int sendConversation ( int client, MYSQL * );

/* codul de eroare returnat de anumite apeluri */
extern int errno;
char *this_username;

int sending (
    int sd,
    char message[] ) { // ca sa nu scriu de 100 ori aceeasi chestie, am facut o
		       // functie care trimite mesajul la server. returneaza 0
		       // daca totul e ok si altceva in caz de eroare
  int length = strlen ( message );
  if ( write ( sd, &length, sizeof ( length ) ) <= 0 ) {
    perror ( "[client]Eroare la write() catre server.\n" );
    return errno;
  }
  if ( write ( sd, message, length ) <= 0 ) {
    perror ( "[client]Eroare la write() catre server.\n" );
    return errno;
  }
  printf ( "am trimis %s\n", message );
  return 0;
}

char *receiving ( int sd ) {
  int length;
  char *error = "error";
  if ( read ( sd, &length, sizeof ( length ) ) <= 0 ) {
    perror ( "[client]Eroare la write() catre server.\n" );
    return error;
  }

  char *message = ( char * ) malloc ( length + 1 );
  if ( read ( sd, message, length ) <= 0 ) {
    perror ( "[client]Eroare la write() catre server.\n" );
    free ( message );
    return error;
  }

  message[ length ] = '\0';
  return message;
}

MYSQL *connectToDatabase ( ) {
  MYSQL_ROW row;
  MYSQL *conn;
  char *host = "127.0.0.1";
  char *user = "root";
  char *pass = "0Mid@-70";
  char *database = "sys";

  conn = mysql_init ( NULL );

  /* Connect to database */
  if ( ! mysql_real_connect ( conn, host, user, pass, database, 0, NULL, 0 ) ) {
    fprintf ( stderr, "%s\n", mysql_error ( conn ) );
    return NULL;
  }
  return conn;
}

int startServer ( int *sd ) {
  struct sockaddr_in server; // structura folosita de server

  if ( ( *sd = socket ( AF_INET, SOCK_STREAM, 0 ) ) == -1 ) {
    perror ( "[server]Eroare la socket().\n" );
    return errno;
  }

  /* pregatirea structurilor de date */
  bzero ( &server, sizeof ( server ) );

  server.sin_family = AF_INET; /* stabilirea familiei de socket-uri */
  server.sin_addr.s_addr = htonl ( INADDR_ANY ); /* acceptam orice adresa */
  server.sin_port = htons ( PORT ); /* utilizam un port utilizator */

  /* atasam socketul */
  if ( bind ( *sd, ( struct sockaddr * ) &server,
	      sizeof ( struct sockaddr ) ) == -1 ) {
    perror ( "[server]Eroare la bind().\n" );
    return errno;
  }
  /* punem serverul sa asculte daca vin clienti sa se conecteze */
  if ( listen ( *sd, 1 ) == -1 ) {
    perror ( "[server]Eroare la listen().\n" );
    return errno;
  }
  return 0;
}

int processMessage ( int, char[], MYSQL * );

void treat ( int sd ) {
  struct sockaddr_in from;
  bzero ( &from, sizeof ( from ) );

  while ( 1 ) {
    int client;
    unsigned int length = sizeof ( from );

    printf ( "[server]Asteptam la portul %d...\n", PORT );
    fflush ( stdout );

    client = accept ( sd, ( struct sockaddr * ) &from,
		      &length ); /* acceptam un client  */

    /* eroare la acceptarea conexiunii de la un client */
    if ( client < 0 ) {
      perror ( "[server]Eroare la accept().\n" );
      continue;
    }
    printf ( "S-a conectat un client\n" );

    int pid;
    if ( ( pid = fork ( ) ) == -1 ) {
      close ( client );
      continue;
    }
    if ( pid > 0 ) {
      close ( client );
      while ( waitpid ( -1, NULL, WNOHANG ) )
	;
      continue;
    }
    if ( pid == 0 ) {
      MYSQL *conn = connectToDatabase ( );
      if ( conn == NULL ) {
	perror ( "Eroare la baza de date" );
	exit ( 1 );
      }
      while ( true ) {
	printf ( "[server]Asteptam mesajul...\n" ); /* s-a realizat conexiunea,
						       se astepta mesajul */
	fflush ( stdout );
	char *msg;
	msg = receiving ( client );
	if ( ! strcmp ( msg, "error" ) ) {
	  close ( client ); /* inchidem conexiunea cu clientul */
	  mysql_close ( conn );
	  break; /* continuam sa ascultam */
	}

	printf ( "[server]Mesajul a fost receptionat...%s\n", msg );
	if ( processMessage ( client, msg, conn ) == -4 ) {
	  close ( client );
	  mysql_close ( conn );
	  free ( this_username );
	  break;
	}
      }
    }
  }
}

int signup ( int client, MYSQL *conn ) {
  MYSQL_RES *res;
  MYSQL_ROW row;

  char *username;
  if ( ! strcmp ( ( username = receiving ( client ) ), "error" ) ) {
    close ( client ); /* inchidem conexiunea cu clientul */
    return -1;	      /* continuam sa ascultam */
  }

  char *password;
  if ( ! strcmp ( ( password = receiving ( client ) ), "error" ) ) {
    close ( client ); /* inchidem conexiunea cu clientul */
    return -1;	      /* continuam sa ascultam */
  }

  char query[ 100 ];
  strcpy ( query, "SELECT *FROM users where username =\"" );
  strcat ( query, username );
  strcat ( query, "\"" );
  if ( mysql_query ( conn, query ) ) {
    fprintf ( stderr, "%s\n", mysql_error ( conn ) );
    exit ( 1 );
  }

  res = mysql_store_result ( conn );
  if ( ( row = mysql_fetch_row ( res ) ) == 0 ) {
    mysql_free_result ( res );
    strcpy ( query, "INSERT INTO users(username,password) VALUES(\'" );
    strcat ( query, username );
    strcat ( query, "\',\'" );
    strcat ( query, password );
    strcat ( query, "\')" );
    if ( mysql_query ( conn, query ) ) {
      fprintf ( stderr, "%s\n", mysql_error ( conn ) );
      exit ( 1 );
    }
    return 0;
  }
  mysql_free_result ( res );

  return -2;
}

int login ( int client, MYSQL *conn ) { //-1 eroare la comunicare
  MYSQL_RES *res;
  MYSQL_ROW row;
  char *username;
  if ( ! strcmp ( ( username = receiving ( client ) ), "error" ) )
    return errno;

  char *password;
  if ( ! strcmp ( ( password = receiving ( client ) ), "error" ) )
    return errno;

  char query[ 100 ];
  strcpy ( query, "SELECT * FROM users where username =\"" );
  strcat ( query, username );
  strcat ( query, "\" and password=\"" );
  strcat ( query, password );
  strcat ( query, "\"" );
  if ( mysql_query ( conn, query ) ) {
    fprintf ( stderr, "%s\n", mysql_error ( conn ) );
    exit ( 1 );
  }
  // sa fac sa fie online

  res = mysql_store_result ( conn );
  if ( ( row = mysql_fetch_row ( res ) ) == 0 ) {
    mysql_free_result ( res );
    printf ( "Nu exista user cu asa date de conectare\n" );
    return -2;
  }
  printf ( "Userul %s s-a logat\n", username );

  mysql_free_result ( res );
  strcpy ( query, "UPDATE users SET connected = 1 WHERE username  =\"" );
  strcat ( query, username );
  strcat ( query, "\"" );
  this_username = ( char * ) malloc ( strlen ( username ) + 1 );
  strcpy ( this_username, username );
  this_username[ strlen ( username ) ] = '\0';
  if ( mysql_query ( conn, query ) ) {
    fprintf ( stderr, "%s\n", mysql_error ( conn ) );
    exit ( 1 );
  }
  return 0;
}

int processMessage ( int client, char message[], MYSQL *conn ) {

  if ( ! strcmp ( message, "login" ) ) {
    int temp = login ( client, conn );
    if ( temp == -2 ) {
      if ( sending ( client, "userNotFound" ) )
	return -1;
      return 0;
    }
    if ( temp == 0 ) {
      if ( sending ( client, "loggedIn" ) )
	return -1;
      return 0;
    }
    return 0;
  }

  if ( ! strcmp ( message, "signup" ) ) {
    int temp = signup ( client, conn );
    if ( temp == -2 ) {
      if ( sending ( client, "alreadyExists" ) )
	return -1;
      return 0;
    }
    if ( temp == 0 ) {
      if ( sending ( client, "signedup" ) )
	return -1;
      return 0;
    }

    return 0;
  }

  if ( ! strcmp ( message, "show online" ) ) {
    if ( sendOnlineUsers ( client, conn ) == 0 ) {
      return 0;
    } else {
      return -1;
    }
  }
  if ( ! strcmp ( message, "show all" ) ) {
    if ( sendAllUsers ( client, conn ) == 0 )
      return 0;
    else
      return -1;
  }
  if ( ! strcmp ( message, "unread" ) ) {
    sendUnreadMessages ( client, conn );
  }
  if ( ! strcmp ( message, "send" ) ) {
    sendMessage ( client, conn );
  }
  if ( ! strcmp ( message, "view" ) ) {
    sendConversation ( client, conn );
  }

  if ( ! strcmp ( message, "exit" ) ) {
    char query[ 100 ];
    strcpy ( query, "UPDATE users SET connected = 0 WHERE username  =\"" );
    strcat ( query, this_username );
    strcat ( query, "\"" );
    if ( mysql_query ( conn, query ) ) {
      fprintf ( stderr, "%s\n", mysql_error ( conn ) );
      exit ( 1 );
    }
  }
}

int sendOnlineUsers ( int client, MYSQL *conn ) {
  MYSQL_RES *res;
  MYSQL_ROW row;
  char query[ 100 ];
  strcpy ( query, "SELECT *FROM users where connected is not NULL" );
  if ( mysql_query ( conn, query ) ) {
    fprintf ( stderr, "%s\n", mysql_error ( conn ) );
    exit ( 1 );
  }

  res = mysql_store_result ( conn );

  int numberOfUsers = mysql_num_rows ( res );
  if ( write ( client, &numberOfUsers, sizeof ( numberOfUsers ) ) < 0 ) {
    perror ( "[client] Eroare la read de la server" );
    return errno;
  }

  while ( ( row = mysql_fetch_row ( res ) ) != 0 ) {
    int length = strlen ( row[ 1 ] );
    char username[ strlen ( row[ 1 ] ) + 1 ];
    strcpy ( username, row[ 1 ] );
    username[ strlen ( username ) ] = '\0';
    if ( sending ( client, username ) )
      return errno;
  }
  mysql_free_result ( res );
  return 0;
}
int sendAllUsers ( int client, MYSQL *conn ) {
  MYSQL_RES *res;
  MYSQL_ROW row;
  char query[ 100 ];
  strcpy ( query, "SELECT * FROM users" );
  if ( mysql_query ( conn, query ) ) {
    fprintf ( stderr, "%s\n", mysql_error ( conn ) );
    exit ( 1 );
  }

  res = mysql_store_result ( conn );

  int numberOfUsers = mysql_num_rows ( res );
  if ( write ( client, &numberOfUsers, sizeof ( numberOfUsers ) ) < 0 ) {
    perror ( "[client] Eroare la read de la server" );
    return errno;
  }

  while ( ( row = mysql_fetch_row ( res ) ) != 0 ) {
    int length = strlen ( row[ 1 ] );
    char username[ strlen ( row[ 1 ] ) + 1 ];
    strcpy ( username, row[ 1 ] );
    username[ strlen ( username ) ] = '\0';
    if ( sending ( client, username ) )
      return errno;
  }
  mysql_free_result ( res );
  return 0;
}
int sendUnreadMessages ( int client, MYSQL *conn ) {
  MYSQL_RES *res;
  MYSQL_ROW row;
  char *username;
  if ( ! strcmp ( ( username = receiving ( client ) ), "error" ) ) {
    close ( client );
    return errno;
  }

  char query[ 100 ];
  strcpy ( query, "SELECT * FROM messages where `to`=\"" );
  strcat ( query, username );
  strcat ( query, "\" and `read`=0" );
  if ( mysql_query ( conn, query ) ) {
    fprintf ( stderr, "%s\n", mysql_error ( conn ) );
    exit ( 1 );
  }

  res = mysql_store_result ( conn );

  int numberOfMessages = mysql_num_rows ( res );

  if ( write ( client, &numberOfMessages, sizeof ( numberOfMessages ) ) < 0 ) {
    perror ( "[client] Eroare la read de la server" );
    return errno;
  }

  while ( ( row = mysql_fetch_row ( res ) ) != 0 ) {
    char username[ strlen ( row[ 1 ] ) + 1 ];
    strcpy ( username, row[ 1 ] );
    username[ strlen ( username ) ] = '\0';
    if ( sending ( client, username ) )
      return errno;

    char message[ strlen ( row[ 3 ] ) + 1 ];
    strcpy ( message, row[ 3 ] );
    message[ strlen ( message ) ] = '\0';
    if ( sending ( client, message ) )
      return errno;
    char *answer;
    if ( ! strcmp ( ( answer = receiving ( client ) ), "error" ) ) {
      close ( client );
      return errno;
    }

    char query[ 100 ];
    strcpy ( query, " UPDATE messages SET `read` = 1 WHERE `ID` =" );
    strcat ( query, row[ 0 ] );

    if ( mysql_query ( conn, query ) ) {
      fprintf ( stderr, "%s\n", mysql_error ( conn ) );
      exit ( 1 );
    }

    if ( ! strcmp ( answer, "send" ) )
      sendMessage ( client, conn );
  }
  mysql_free_result ( res );
  return 0;
}
int sendMessage ( int client, MYSQL *conn ) {
  MYSQL_RES *res;
  MYSQL_ROW row;
  char *fromUsername;
  if ( ! strcmp ( ( fromUsername = receiving ( client ) ), "error" ) ) {
    close ( client );
    return errno;
  }
  char *toUsername;
  if ( ! strcmp ( ( toUsername = receiving ( client ) ), "error" ) ) {
    close ( client );
    return errno;
  }

  char *message;
  if ( ! strcmp ( ( message = receiving ( client ) ), "error" ) ) {
    close ( client );
    return errno;
  }

  char query2[ 100 ];
  strcpy ( query2, "SELECT * FROM users where username=\"" );
  strcat ( query2, toUsername );
  strcat ( query2, "\"" );

  if ( mysql_query ( conn, query2 ) ) {
    fprintf ( stderr, "%s\n", mysql_error ( conn ) );
    exit ( 1 );
  }

  res = mysql_store_result ( conn );
  int numberOfUsers = mysql_num_rows ( res );
  if ( numberOfUsers == 0 ) {
    if ( sending ( client, "notExisting" ) ) {
      mysql_free_result ( res );
      return perror;
    }
    mysql_free_result ( res );
    return 0;
  }

  char query[ 100 ];
  strcpy ( query, "INSERT INTO messages (`from`, `to`, message) VALUES ('" );
  strcat ( query, fromUsername );
  strcat ( query, "','" );
  strcat ( query, toUsername );
  strcat ( query, "','" );
  strcat ( query, message );
  strcat ( query, "')" );
  if ( mysql_query ( conn, query ) ) {
    printf ( "am ajuns dupa query\n" );
    fprintf ( stderr, "%s\n", mysql_error ( conn ) );
    exit ( 1 );
  }

  if ( sending ( client, "sent" ) ) {
    mysql_free_result ( res );
    return perror;
  }
  mysql_free_result ( res );
}
int sendConversation ( int client, MYSQL *conn ) {
  MYSQL_RES *res;
  MYSQL_ROW row;
  char *fromUsername;
  if ( ! strcmp ( ( fromUsername = receiving ( client ) ), "error" ) ) {
    close ( client );
    return errno;
  }
  char *toUsername;
  if ( ! strcmp ( ( toUsername = receiving ( client ) ), "error" ) ) {
    close ( client );
    return errno;
  }

  char query[ 200 ];
  strcpy ( query, "SELECT * FROM messages where (`to`=\"" );
  strcat ( query, toUsername );
  strcat ( query, "\" and `from`=\"" );
  strcat ( query, fromUsername );
  strcat ( query, "\") OR (`to`=\"" );
  strcat ( query, fromUsername );
  strcat ( query, "\" and `from`=\"" );
  strcat ( query, toUsername );
  strcat ( query, "\")" );

  printf ( "%s", query );

  if ( mysql_query ( conn, query ) ) {
    fprintf ( stderr, "%s\n", mysql_error ( conn ) );
    exit ( 1 );
  }

  res = mysql_store_result ( conn );

  int numberOfMessages = mysql_num_rows ( res );
  if ( write ( client, &numberOfMessages, sizeof ( numberOfMessages ) ) < 0 ) {
    perror ( "[server] Eroare la read de la client\n" );
    return perror;
  }

  while ( ( row = mysql_fetch_row ( res ) ) != 0 ) {
    char sender[ strlen ( row[ 1 ] ) + 1 ];
    strcpy ( sender, row[ 1 ] );

    char message[ strlen ( row[ 3 ] ) + 1 ];
    strcpy ( message, row[ 3 ] );

    char query[ 100 ];
    strcpy ( query, " UPDATE messages SET `read` = 1 WHERE `ID` =" );
    strcat ( query, row[ 0 ] );

    if ( mysql_query ( conn, query ) ) {
      fprintf ( stderr, "%s\n", mysql_error ( conn ) );
      exit ( 1 );
    }
    if ( sending ( client, sender ) )
      return errno;
    if ( sending ( client, message ) )
      return errno;
  }
  mysql_free_result ( res );
}

int main ( ) {

  int sd;

  if ( startServer ( &sd ) ) {
    printf ( "Nu s-a putut porni serverul\n" );
    exit ( 2 );
  } else {
    printf ( "Serverul a pornit\n" );
  }

  treat ( sd );
}