#include <stdio.h>
#include <stdlib.h>
#include "../src/List.h"

void
print_list (
    List *list)
{
  while (list) {
    printf ("%s\n", list->data);
    list = list->next;
  }
}

int
main (
    int argc,
    char **argv)
{
  char s1[] = "String1";
  char s2[] = "String2";
  char s3[] = "String3";

  List *l = NULL;
  l = list_append (l, s1);
  l = list_append (l, s2);
  l = list_append (l, s3);

  print_list (l);

  l = list_remove (l, s2);
  l = list_remove (l, s1);

  print_list (l);

  l = list_remove (l, s3);

  print_list (l);

  return EXIT_SUCCESS;
}
