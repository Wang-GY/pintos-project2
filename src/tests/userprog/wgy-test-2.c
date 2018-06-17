/*
free file attack
open a file many times and close them all.
then open them again. we repeat this procegure and hope we can
get same result every time

if the kernel does't free the file descriptor, this program will use up memory soon
*/

#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"
// 31370 on my computer
#define PASS_NUM  50000

void
test_main (void)
{

  int fd =0;
  /*get max numner of files it can open*/
  for(int i=0;true;i++){
  fd= open("sample.txt");
    if(fd == -1){
      msg("open fail");
      return;
    }
    close(fd);
    // sucess
    if(i > PASS_NUM){
      break;
    }
  }
  msg("sucess");
}
