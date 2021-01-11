
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
MYSQL_RES *res;
MYSQL_ROW row;
MYSQL *conn;

int sendOnlineUsers ( int client );
int sendAllUsers ( int client );
int sendUnreadMessages ( int client );
int sendMessage ( int client );
int sendConversation ( int client );

/* codul de eroare returnat de anumite apeluri */
extern int errno;

bool connectToDatabase ( ) {
  char *host = "127.0.0.1";
  char *user = "root";
  char *pass = "0Mid@-70";
  char *database = "sys";

  conn = mysql_init ( NULL );

  /* Connect to database */
  if ( ! mysql_real_connect ( conn, host, user, pass, database, 0, NULL, 0 ) ) {
    fprintf ( stderr, "%s\n", mysql_error ( conn ) );
    return false;
  }
  return true;
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

int processMessage ( int, char[] );

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
      while ( true ) {
	printf ( "[server]Asteptam mesajul...\n" ); /* s-a realizat conexiunea,
						       se astepta mesajul */
	fflush ( stdout );
	int size;
	if ( read ( client, &size, sizeof ( size ) ) <= 0 ) {
	  perror ( "[server]Eroare la read() de la client.\n" );
	  close ( client ); /* inchidem conexiunea cu clientul */
	  break;	    /* continuam sa ascultam */
	}

	char msg[ size + 1 ];
	bzero ( msg, size );

	/* citirea mesajului */
	if ( read ( client, msg, size ) <= 0 ) {
	  perror ( "[server]Eroare la read() de la client.\n" );
	  close ( client ); /* inchidem conexiunea cu clientul */
	  break;	    /* continuam sa ascultam */
	}
	msg[ size ] = '\0';
	printf ( "[server]Mesajul a fost receptionat...%s\n", msg );
	processMessage ( client, msg );
      }
    }
  }
}

int signup ( int client ) {
  int size;
  if ( read ( client, &size, sizeof ( size ) ) <= 0 ) {
    perror ( "[server]Eroare la read() de la client.\n" );
    close ( client ); /* inchidem conexiunea cu clientul */
    return -1;	      /* continuam sa ascultam */
  }

  char username[ size + 1 ];
  bzero ( username, size );

  /* citirea mesajului */
  if ( read ( client, username, size ) <= 0 ) {
    perror ( "[server]Eroare la read() de la client.\n" );
    close ( client ); /* inchidem conexiunea cu clientul */
    return -1;	      /* continuam sa ascultam */
  }
  username[ size ] = '\0';

  if ( read ( client, &size, sizeof ( size ) ) <= 0 ) {
    perror ( "[server]Eroare la read() de la client.\n" );
    close ( client ); /* inchidem conexiunea cu clientul */
    return -1;	      /* continuam sa ascultam */
  }

  char password[ size + 1 ];
  bzero ( password, size );

  /* citirea mesajului */
  if ( read ( client, password, size ) <= 0 ) {
    perror ( "[server]Eroare la read() de la client.\n" );
    close ( client );
    return -1;
  }
  password[ size ] = '\0';
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

int login ( int client ) { //-1 eroare la comunicare
  int size;
  if ( read ( client, &size, sizeof ( size ) ) <= 0 ) {
    perror ( "[server]Eroare la read() de la client.\n" );
    close ( client );
    return -1;
  }

  char username[ size + 1 ];
  bzero ( username, size );

  /* citirea mesajului */
  if ( read ( client, username, size ) <= 0 ) {
    perror ( "[server]Eroare la read() de la client.\n" );
    close ( client ); /* inchidem conexiunea cu clientul */
    return -1;	      /* continuam sa ascultam */
  }
  username[ size ] = '\0';

  if ( read ( client, &size, sizeof ( size ) ) <= 0 ) {
    perror ( "[server]Eroare la read() de la client.\n" );
    close ( client ); /* inchidem conexiunea cu clientul */
    return -1;	      /* continuam sa ascultam */
  }

  char password[ size + 1 ];
  bzero ( password, size );

  /* citirea mesajului */
  if ( read ( client, password, size ) <= 0 ) {
    perror ( "[server]Eroare la read() de la client.\n" );
    close ( client ); /* inchidem conexiunea cu clientul */
    return -1;	      /* continuam sa ascultam */
  }
  password[ size ] = '\0';
  char query[ 100 ];
  strcpy ( query, "SELECT *FROM users where username =\"" );
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
  if ( mysql_query ( conn, query ) ) {
    fprintf ( stderr, "%s\n", mysql_error ( conn ) );
    exit ( 1 );
  }
  return 0;
}

int processMessage ( int client, char message[] ) {
  if ( ! strcmp ( message, "login" ) ) {
    int temp = login ( client );
    if ( temp == -2 ) {
      char answer[] = "userNotFound";
      int size = strlen ( answer );
      if ( write ( client, &size, sizeof ( size ) ) <= 0 ) {
	perror ( "[server]Eroare la write() la client.\n" );
	return -1;
      }
      if ( write ( client, answer, size ) <= 0 ) {
	perror ( "[server]Eroare la write() la client.\n" );
	return -1;
      }
      return -2;
    }
    if ( temp == 0 ) {
      char answer[] = "loggedIn";
      int size = strlen ( answer );
      if ( write ( client, &size, sizeof ( size ) ) <= 0 ) {
	perror ( "[server]Eroare la write() la client.\n" );
	return -1;
      }
      if ( write ( client, answer, size ) <= 0 ) {
	perror ( "[server]Eroare la write() la client.\n" );
	return -1;
      }
    }

    return -1;
  }

  if ( ! strcmp ( message, "signup" ) ) {
    int temp = signup ( client );
    if ( temp == -2 ) {
      char answer[] = "alreadyExists";
      int size = strlen ( answer );
      if ( write ( client, &size, sizeof ( size ) ) <= 0 ) {
	perror ( "[server]Eroare la write() la client.\n" );
	return -1;
      }
      if ( write ( client, answer, size ) <= 0 ) {
	perror ( "[server]Eroare la write() la client.\n" );
	return -1;
      }
      return -2;
    }
    if ( temp == 0 ) {
      char answer[] = "signedup";
      int size = strlen ( answer );
      if ( write ( client, &size, sizeof ( size ) ) <= 0 ) {
	perror ( "[server]Eroare la write() la client.\n" );
	return -1;
      }
      if ( write ( client, answer, size ) <= 0 ) {
	perror ( "[server]Eroare la write() la client.\n" );
	return -1;
      }
    }

    return -1;
  }

  if ( ! strcmp ( message, "show online" ) ) {
    if ( sendOnlineUsers ( client ) == 0 ) {
      return 0;
    } else {
      return -1;
    }
  }
  if ( ! strcmp ( message, "show all" ) ) {
    if ( sendAllUsers ( client ) == 0 )
      return 0;
    else
      return -1;
  }
  if ( ! strcmp ( message, "unread" ) ) {
    sendUnreadMessages ( client );
  }
  if ( ! strcmp ( message, "send" ) ) {
    sendMessage ( client );
  }
  if ( ! strcmp ( message, "view" ) ) {
    sendConversation ( client );
  }
}

int sendOnlineUsers ( int client ) {
  char query[ 100 ];
  strcpy ( query, "SELECT *FROM users where connected is not NULL" );
  if ( mysql_query ( conn, query ) ) {
    fprintf ( stderr, "%s\n", mysql_error ( conn ) );
    exit ( 1 );
  }

  res = mysql_store_result ( conn );

  int numberOfUsers = mysql_num_rows ( res );

  if ( write ( client, &numberOfUsers, sizeof ( numberOfUsers ) ) <= 0 ) {
    perror ( "[server]Eroare la write() la client.\n" );
    return -1;
  }

  while ( ( row = mysql_fetch_row ( res ) ) != 0 ) {
    int size = strlen ( row[ 1 ] );
    char username[ strlen ( row[ 1 ] ) + 1 ];
    strcpy ( username, row[ 1 ] );
    username[ strlen ( username ) ] = '\0';
    if ( write ( client, &size, sizeof ( size ) ) <= 0 ) {
      perror ( "[server]Eroare la write() la client.\n" );
      return -1;
    }
    if ( write ( client, username, size ) <= 0 ) {
      perror ( "[server]Eroare la write() la client.\n" );
      return -1;
    }
  }
  mysql_free_result ( res );
  return 0;
}
int sendAllUsers ( int client ) {
  char query[ 100 ];
  strcpy ( query, "SELECT * FROM users" );
  if ( mysql_query ( conn, query ) ) {
    fprintf ( stderr, "%s\n", mysql_error ( conn ) );
    exit ( 1 );
  }

  res = mysql_store_result ( conn );

  int numberOfUsers = mysql_num_rows ( res );

  if ( write ( client, &numberOfUsers, sizeof ( numberOfUsers ) ) <= 0 ) {
    perror ( "[server]Eroare la write() la client.\n" );
    return -1;
  }

  while ( ( row = mysql_fetch_row ( res ) ) != 0 ) {
    int size = strlen ( row[ 1 ] );
    char username[ strlen ( row[ 1 ] ) + 1 ];
    strcpy ( username, row[ 1 ] );
    username[ strlen ( username ) ] = '\0';
    if ( write ( client, &size, sizeof ( size ) ) <= 0 ) {
      perror ( "[server]Eroare la write() la client.\n" );
      return -1;
    }
    if ( write ( client, username, size ) <= 0 ) {
      perror ( "[server]Eroare la write() la client.\n" );
      return -1;
    }
  }
  mysql_free_result ( res );
  return 0;
}
int sendUnreadMessages ( int client ) {
  int size;
  if ( read ( client, &size, sizeof ( size ) ) <= 0 ) {
    perror ( "[server]Eroare la read() de la client.\n" );
    close ( client );
    return -1;
  }

  char username[ size + 1 ];
  bzero ( username, size );

  /* citirea mesajului */
  if ( read ( client, username, size ) <= 0 ) {
    perror ( "[server]Eroare la read() de la client.\n" );
    close ( client );
    return -1;
  }
  username[ size ] = '\0';

  char query[ 100 ];
  strcpy ( query, "SELECT * FROM messages where `to`=\"" );
  strcat ( query, username );
  strcat ( query, "\" and `read`=0" );
  if ( mysql_query ( conn, query ) ) {
    fprintf ( stderr, "%s\n", mysql_error ( conn ) );
    exit ( 1 );
  }

  res = mysql_store_result ( conn );

  int numberOfUsers = mysql_num_rows ( res );

  if ( write ( client, &numberOfUsers, sizeof ( numberOfUsers ) ) <= 0 ) {
    perror ( "[server]Eroare la write() la client.\n" );
    return -1;
  }

  while ( ( row = mysql_fetch_row ( res ) ) != 0 ) {
    int size = strlen ( row[ 1 ] );
    char username[ strlen ( row[ 1 ] ) + 1 ];
    strcpy ( username, row[ 1 ] );
    username[ strlen ( username ) ] = '\0';
    if ( write ( client, &size, sizeof ( size ) ) <= 0 ) {
      perror ( "[server]Eroare la write() la client.\n" );
      return -1;
    }
    if ( write ( client, username, size ) <= 0 ) {
      perror ( "[server]Eroare la write() la client.\n" );
      return -1;
    }

    size = strlen ( row[ 3 ] );
    char message[ strlen ( row[ 1 ] ) + 1 ];
    strcpy ( message, row[ 1 ] );
    message[ strlen ( message ) ] = '\0';
    if ( write ( client, &size, sizeof ( size ) ) <= 0 ) {
      perror ( "[server]Eroare la write() la client.\n" );
      return -1;
    }
    if ( write ( client, message, size ) <= 0 ) {
      perror ( "[server]Eroare la write() la client.\n" );
      return -1;
    }
  }
  mysql_free_result ( res );
  return 0;
}
int sendMessage ( int client ) {
  int size;
  if ( read ( client, &size, sizeof ( size ) ) <= 0 ) {
    perror ( "[server]Eroare la read() de la client.\n" );
    close ( client );
    return -1;
  }

  char fromUsername[ size + 1 ];
  bzero ( fromUsername, size );

  /* citirea mesajului */
  if ( read ( client, fromUsername, size ) <= 0 ) {
    perror ( "[server]Eroare la read() de la client.\n" );
    close ( client );
    return -1;
  }
  fromUsername[ size ] = '\0';

  if ( read ( client, &size, sizeof ( size ) ) <= 0 ) {
    perror ( "[server]Eroare la read() de la client.\n" );
    close ( client );
    return -1;
  }

  char toUsername[ size + 1 ];
  bzero ( toUsername, size );

  /* citirea mesajului */
  if ( read ( client, toUsername, size ) <= 0 ) {
    perror ( "[server]Eroare la read() de la client.\n" );
    close ( client );
    return -1;
  }
  toUsername[ size ] = '\0';

  if ( read ( client, &size, sizeof ( size ) ) <= 0 ) {
    perror ( "[server]Eroare la read() de la client.\n" );
    close ( client );
    return -1;
  }

  char message[ size + 1 ];
  bzero ( message, size );

  /* citirea mesajului */
  if ( read ( client, message, size ) <= 0 ) {
    perror ( "[server]Eroare la read() de la client.\n" );
    close ( client );
    return -1;
  }
  message[ size ] = '\0';

  char query[ 100 ];
  strcpy ( query, "INSERT INTO messages (`from`, `to`, message) VALUES ('" );
  strcat ( query, fromUsername );
  strcat ( query, "','" );
  strcat ( query, toUsername );
  strcat ( query, "','" );
  strcat ( query, message );
  strcat ( query, "')" );
  if ( mysql_query ( conn, query ) ) {
    fprintf ( stderr, "%s\n", mysql_error ( conn ) );
    exit ( 1 );
  }
}
int sendConversation ( int client ) {
  int size;
  if ( read ( client, &size, sizeof ( size ) ) <= 0 ) {
    perror ( "[server]Eroare la read() de la client.\n" );
    close ( client );
    return -1;
  }

  char fromUsername[ size + 1 ];
  bzero ( fromUsername, size );

  /* citirea mesajului */
  if ( read ( client, fromUsername, size ) <= 0 ) {
    perror ( "[server]Eroare la read() de la client.\n" );
    close ( client );
    return -1;
  }
  fromUsername[ size ] = '\0';

  if ( read ( client, &size, sizeof ( size ) ) <= 0 ) {
    perror ( "[server]Eroare la read() de la client.\n" );
    close ( client );
    return -1;
  }

  char toUsername[ size + 1 ];
  bzero ( toUsername, size );

  /* citirea mesajului */
  if ( read ( client, toUsername, size ) <= 0 ) {
    perror ( "[server]Eroare la read() de la client.\n" );
    close ( client );
    return -1;
  }
  toUsername[ size ] = '\0';

  if ( read ( client, &size, sizeof ( size ) ) <= 0 ) {
    perror ( "[server]Eroare la read() de la client.\n" );
    close ( client );
    return -1;
  }
}

int main ( ) {
  char *err_msg = 0;
  char str[ 100 ];
  char sql[ 100 ];

  int sd;

  if ( ! connectToDatabase ( ) )
    exit ( 1 );
  if ( startServer ( &sd ) ) {
    exit ( 2 );
  }

  treat ( sd );

  // printf ( "Am ajuns aici\n" ); so far so good

  /*send SQL query*/
  /*  if ( mysql_query ( conn, "SELECT * FROM users" ) ) {
     fprintf ( stderr, "%s\n", mysql_error ( conn ) );
     exit ( 1 );
   }

   res = mysql_store_result ( conn );

   while ( ( row = mysql_fetch_row ( res ) ) != 0 ) {
     char *entry;
     entry = row[ 1 ];
     printf ( "%s\n", entry );
   }

   /* Free results when done
   mysql_free_result ( res ); */
}