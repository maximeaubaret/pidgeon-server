#ifndef UTILS_H_
#define UTILS_H_

#define CNRM  "\x1B[0m"
#define CRED  "\x1B[31m"
#define CGRN  "\x1B[32m"
#define CYEL  "\x1B[33m"
#define CBLU  "\x1B[34m"
#define CMAG  "\x1B[35m"
#define CCYN  "\x1B[36m"
#define CWHT  "\x1B[37m"

#define CBNRM  "\x1B[1;0m"
#define CBRED  "\x1B[1;31m"
#define CBGRN  "\x1B[1;32m"
#define CBYEL  "\x1B[1;33m"
#define CBBLU  "\x1B[1;34m"
#define CBMAG  "\x1B[1;35m"
#define CBCYN  "\x1B[1;36m"
#define CBWHT  "\x1B[1;37m"

#define LINE_CLEAR "\033[F\033[1G\033[K"

void die (char *msg);

#endif
