/*
MULTIPLE  CHECP CHILDREN ATTACK
create many simple children and let them exit notmally.
ususally,
if kernel implenment wait by block the child brefore exit. then
the children will consume some resources and become zoombie, the maximum number
of the cheap children will be limited

we expect at least 60 process can be execute.
*/
#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"
#define CHILD_CNT 60

void test_main(void){
  for(int i=0;i < CHILD_CNT; i++){
    exec("child-simple");
  }
  msg("success, execute %d children",CHILD_CNT);
}
