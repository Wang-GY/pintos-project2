# -*- perl -*-
use strict;
use warnings;
use tests::tests;

check_expected ([<<'EOF']);
(wgy-test-2) begin
(wgy-test-2) sucess
(wgy-test-2) end
wgy-test-2: exit(0)
EOF
pass;
