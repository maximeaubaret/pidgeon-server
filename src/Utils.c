#include <stdio.h>
#include <stdlib.h>

#include "Utils.h"

void
die (
    char *msg)
{
  perror (msg);
  exit (EXIT_FAILURE);
}
