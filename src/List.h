typedef void* pointer;
typedef void            (*Func)                (pointer       data,
                                                pointer       user_data);
typedef struct _List List;

struct _List {
  pointer data;
  List *next;
  List *prev;
};

List *    list_append (List *list, pointer data);
List *    list_remove (List *list, pointer data);
List *    list_last (List *list);
void      list_foreach (List *list, Func func, pointer user_data);
void      list_free (List *list);
