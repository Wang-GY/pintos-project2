#!/bin/bash


pintos-mkdisk filesys.dsk --filesys-size=2
pintos -f -q
pintos -p build/tests/userprog/args-multiple -a args-multiple -- -q
pintos -q run 'args-multiple some arguments for you!'

