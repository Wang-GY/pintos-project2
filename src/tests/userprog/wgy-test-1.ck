# -*- perl -*-
use strict;
use warnings;
use tests::tests;

check_expected (IGNORE_EXIT_CODES => 1,IGNORE_USER_FAULTS =>1, [<<'EOF',<<'EOF',<<'EOF']);
(wgy-test-1) begin
(wgy-test-1) success, execute 60 children
(wgy-test-1) end
EOF
(wgy-test-1) begin
(wgy-test-1) success, execute 60 children
(wgy-test-1) end
(child-simple) run
EOF
(wgy-test-1) begin
(wgy-test-1) success, execute 60 children
(wgy-test-1) end
(child-simple) run
(child-simple) run
EOF
pass;
