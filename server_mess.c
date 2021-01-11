
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
#define PORT 2024

/* codul de eroare returnat de anumite apeluri */
extern int errno;

int main ( ) {
  char *err_msg = 0;
  char str[ 100 ];
  char sql[ 100 ];

  MYSQL *conn = mysql_init ( NULL );
  MYSQL_RES *res;
  MYSQL_ROW row;

  char *host = "127.0.0.1";
  char *user = "root";
  char *pass = "0Mid@-70";
  char *database = "sys";

  /* Connect to database */
  if ( ! mysql_real_connect ( conn, host, user, pass, database, 0, NULL, 0 ) ) {
    fprintf ( stderr, "%s\n", mysql_error ( conn ) );
    exit ( 1 );
  }

  /*send SQL query*/
  if ( mysql_query ( conn, "SELECT * FROM users" ) ) {
    fprintf ( stderr, "%s\n", mysql_error ( conn ) );
    exit ( 1 );
  }

  res = mysql_use_result ( conn );

  struct sockaddr_in server; // structura folosita de server
  struct sockaddr_in from;
  char msg[ 100 ];	  // mesajul primit de la client
  char rasp[ 100 ] = " "; // mesaj de raspuns pentru client
  int sd;		  // descriptorul de socket

  char username[ 100 ], password[ 100 ];

  /* crearea unui socket */
  if ( ( sd = socket ( AF_INET, SOCK_STREAM, 0 ) ) == -1 ) {
    perror ( "[server]Eroare la socket().\n" );
    return errno;
  }

  /* pregatirea structurilor de date */
  bzero ( &server, sizeof ( server ) );
  bzero ( &from, sizeof ( from ) );

  server.sin_family = AF_INET; /* stabilirea familiei de socket-uri */
  server.sin_addr.s_addr = htonl ( INADDR_ANY ); /* acceptam orice adresa */
  server.sin_port = htons ( PORT ); /* utilizam un port utilizator */

  /* atasam socketul */
  if ( bind ( sd, ( struct sockaddr * ) &server, sizeof ( struct sockaddr ) ) ==
       -1 ) {
    perror ( "[server]Eroare la bind().\n" );
    return errno;
  }

  /* punem serverul sa asculte daca vin clienti sa se conecteze */
  if ( listen ( sd, 1 ) == -1 ) {
    perror ( "[server]Eroare la listen().\n" );
    return errno;
  }

  /* servim in mod concurent clientii... */
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

    int pid;
    if ( ( pid = fork ( ) ) == -1 ) {
      close ( client );
      continue;
    } else if ( pid > 0 ) {
      // parinte
      close ( client );
      while ( waitpid ( -1, NULL, WNOHANG ) )
	;
      continue;
    } else if ( pid == 0 ) {
      int closing = 0, login_user = 0;
      while ( 1 ) {
	close ( sd );

	bzero ( msg, 100 );
	printf ( "[server]Asteptam mesajul...\n" ); /* s-a realizat conexiunea,
						       se astepta mesajul */
	fflush ( stdout );

	/* citirea mesajului */
	if ( read ( client, msg, 100 ) <= 0 ) {
	  perror ( "[server]Eroare la read() de la client.\n" );
	  close ( client ); /* inchidem conexiunea cu clientul */
	  continue;	    /* continuam sa ascultam */
	}

	printf ( "[server]Mesajul a fost receptionat...%s\n", msg );

	// Logare user
	/*if (strstr(msg, "Login") != NULL)
	{   char buffer[100];
	    int bytes;
	    char user[100];
	    char rasp[100]=" ";

	    bytes = read (client, user, sizeof (buffer));
	    if (bytes < 0)
	    {
		perror ("Eroare la read() de la client.\n");
		return 0;
	    }
	    printf ("[server]Utilizatorul cu numele %s incearca sa se
	conecteze.\n", msg);


	    if (mysql_query(conn, "SELECT * FROM users") !=0 )
	    {
		fprintf(stderr, "Eroare interogare\n");
		exit(1);
	    }

	    MYSQL_RES *rez = mysql_store_result(conn);

	    int num_fields = mysql_num_fields(rez);
	    bool found = false;

	    while( (row = mysql_fetch_row(rez))){
		if(strcmp(row[0],user)){
		    bzero(rasp,100);
		    strcat(rasp,"Hello");
		    strcat(rasp,user);
		    found = true;
		}
	    }
	    if(found == false){
		bzero(rasp,100);
		strcat(rasp,user);
		strcat(rasp," This user does not exist in this DB");

	    }

	    printf("[server]Trimitem mesajul inapoi...%s\n",rasp);

	    if (bytes && write (client, rasp, bytes) < 0)


	    {
		perror ("[server] Eroare la write() catre client.\n");
		return 0;
	    }

	    return bytes;
	}

	mysql_free_result(res);
	mysql_close(conn);*/

	if ( strstr ( msg, "Login_utilizator" ) != NULL ) {
	  str[ 100 ] = "";
	  sql[ 100 ] = "";
	  char credentiale[ 100 ] = "";
	  char x[ 100 ] = "";
	  strncpy ( x, msg, ( strlen ( msg ) ) );
	  strcpy ( credentiale, x + 16 );
	  printf ( "%s\n", credentiale );
	  char a[ 2 ][ 100 ];
	  int i = 1;
	  char *p = strtok ( credentiale, " " );
	  printf ( "%s\n", p );
	  while ( p != NULL ) {
	    strcpy ( a[ i ], p );
	    printf ( "%s\n", a[ i ] );
	    p = strtok ( NULL, " " );
	    i++;
	  }

	  sprintf ( sql, "SELECT * FROM User WHERE Name LIKE '%s';", a[ 1 ] );
	  rc = sqlite3_exec ( db, sql, callback, str, &err_msg );
	  printf ( "%s", str );
	  if ( rc != SQLITE_OK ) {
	    fprintf ( stderr, "Failed to select data\n" );
	    fprintf ( stderr, "SQL error: %s\n", err_msg );
	    bzero ( rasp, 100 );
	    strcpy ( rasp, "Failed to select data" );
	    sqlite3_free ( err_msg );
	    return 1;
	  }
	  if ( strstr ( str, a[ 1 ] ) != NULL ) {
	    sprintf ( sql, "SELECT * FROM User WHERE password LIKE '%s';",
		      a[ 2 ] );
	    rc = sqlite3_exec ( db, sql, callback, str, &err_msg );
	    printf ( "%s", str );
	    if ( rc != SQLITE_OK ) {
	      fprintf ( stderr, "Failed to select data\n" );
	      fprintf ( stderr, "SQL error: %s\n", err_msg );
	      bzero ( rasp, 100 );
	      strcpy ( rasp, "Failed to select data" );
	      sqlite3_free ( err_msg );
	      return 1;
	    }
	    if ( strstr ( str, a[ 2 ] ) != NULL ) {
	      bzero ( rasp, 100 );
	      strcpy ( rasp, "Bun venit!" );
	    } else {
	      printf ( "%s\n", a[ 2 ] );
	      bzero ( rasp, 100 );
	      strcpy ( rasp, "Parola incorecta!" );
	    }

	    /*bzero(msgrasp, 100);
	    strcpy(msgrasp,"Bun venit!");*/
	  } else {
	    printf ( "%s\n", a[ 1 ] );
	    bzero ( rasp, 100 );
	    strcpy ( rasp, "Numele de utilizator inexitent!" );
	  }
	  /* bzero(msgrasp, 100);
	   strcpy(msgrasp,"Logare cu succes\n");*/
	}
      }

      /*pregatim mesajul de raspuns */
      bzero ( rasp, 100 );
      strcat ( rasp, "Hello " );
      strcat ( rasp, msg );

      printf ( "[server]Trimitem mesajul inapoi...%s\n", rasp );

      /* returnam mesajul clientului */
      if ( write ( client, rasp, 100 ) <= 0 ) {
	perror ( "[server]Eroare la write() catre client.\n" );
	continue; /* continuam sa ascultam */
      } else
	printf ( "[server]Mesajul a fost trasmis cu succes.\n" );

      /* am terminat cu acest client, inchidem conexiunea */
      close ( client );
      exit ( 0 );
    }
  }
}
