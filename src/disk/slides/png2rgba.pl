#!/usr/bin/env perl

# (this is secretly just a wrapper for imagemagick)

use 5.12.0;
use warnings;

# Feed PNG data on stdin, and receive RGBA data on stdout.

system "convert"
      ,"png:-"
      ,-depth => 8
      ,-alpha => "on"
      ,"-color-matrix" => "0 0 1, 0 1 0, 1 0 0"
      ,"rgba:-";
