#include <stdio.h>

/*
	"ccurrent" (spelled "concurrent") is a word play between "C" and "Concurrency"
	This aims to be a portable, minimal and lightweight header-only library to
	abstract away Threads and Processes creation, management and destruction on
	POSIX and Windows system. (Write once, compile everywhere)

    - https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread.h.html
    - https://www.intel.sg/content/dam/www/public/apac/xa/en/pdfs/ssg/Programming_with_Windows_Threads.pdf

    TODO:   Write test functions for "cc_th_attr_get/setinheritsched" and "cc_th_attr_get/setscope".
*/

#include "src/ccurrent.h"

CC_TH_FUNC_RET hello_world(void *a);

CC_TH_FUNC_RET hello_world(void *a) {
    (void)a;
    printf("Hello World!\n");
    CC_TH_RETURN(123);
}


int main(void) {
    cc_th th1;
    void *retval;

    cc_th_create(&th1, NULL, hello_world, NULL);
    printf("%ld\n", cc_th_self());
	cc_th_join(th1, &retval);
    
    return 0;
}
