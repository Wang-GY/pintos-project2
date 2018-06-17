/*
zoombie attack

*/

#include <syscall.h>
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <syscall.h>
#include <random.h>
#include "tests/lib.h"
#include "tests/main.h"

// void test_main(void){
//
//   const int attack_num = 500;
//
//   for (int i=0; i<attack_num; i++){
//     exec("child-simple");
//   }
//
// }

void
test_main (void)
{
  msg ("wait(exec()) = %d", wait (exec ("child-simple")));
}
