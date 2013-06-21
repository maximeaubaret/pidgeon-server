#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>

#include "Client.h"

Client *
client_new () 
{
  return calloc (1, sizeof (Client));
}

void 
client_free (
    Client *c) 
{
  free (c);
}

