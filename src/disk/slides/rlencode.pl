#!/usr/bin/env perl

# Copyright 2019 Chris Smeele
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

use 5.12.0;
use warnings;

# This accepts RGBA image data on stdin, and produces RLI data on stdout.

binmode STDIN;
binmode STDOUT;

sub min { my ($a, $b) = @_; $a <= $b ? $a : $b }
sub max { my ($a, $b) = @_; $a >= $b ? $a : $b }

my $last = 0; # currently repeating 32-bit value
my $i    = 0; # counter

sub emitmaybe  {
    if (defined $last and $i > 0) {
        while ($i) {
            # Count max is 0xffff, we may need multiple entries for one value.
            my $x = min $i, 0xffff;
            print pack("SV", $x, $last);
            $i -= $x;
        }
    }
};

while (read STDIN, my $x, 4) {
    $x = unpack "V", $x;
    # Keep going until we encounter a different 32-bit value.
    if (defined $last and $x == $last) { ++$i; next }
    emitmaybe;
    $last = $x; $i = 1
}

emitmaybe;
