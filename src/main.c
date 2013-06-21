#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Server.h"

#define PORT 5000

  void
usage (
    char *pname)
{
  printf ("Usage:\n");
  printf ("\t%s [--port <port>]\n", pname);
}

int
main (
    int argc,
    char **argv)
{
  int port = PORT;

  if (argc == 3) {
    if (strcmp (argv[1], "--port") == 0) {
      port = atoi (argv[2]);
    }
    else {
      usage (argv[0]);
      return EXIT_FAILURE;
    }
  }

  Server *s = server_new ();
  s->port = port;
  server_listen (s);
  server_free (s);

  return EXIT_SUCCESS;
}

