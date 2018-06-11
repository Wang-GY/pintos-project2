#!/bin/bash
make clean
make

pintos -v -k -T 60 --bochs  --filesys-size=2 -p build/tests/userprog/sc-bad-sp -a sc-bad-sp -- -q  -f run sc-bad-sp

# pintos-mkdisk filesys.dsk --filesys-size=2
# pintos -f -q
# pintos -p build/tests/userprog/args-multiple -a args-multiple -- -q
# pintos -q run 'args-multiple some arguments for you!'

