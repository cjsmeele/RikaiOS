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

use FindBin '$Bin';

# This script is a small hack to produce slide deck images from a plain-text
# file. It relies heavily on ImageMagick and Pango to do the heavy lifting, but
# it's extremely flexible (your slide deck can evaluate arbitrary Perl code!).
#
# Also maybe don't run this on untrusted deck files :-)

# For what it's worth: Among my student peers I am known for sneaking a Perl
# script into any (group) assignment/project that I work on.
# This tool is sort-of intended as a nice conclusion to that tradition.

# -b = also produce RLE encoded RGBA files. Needed for the RikaiOS slideshow presenter tool.

my $make_bin = 0;
if (@ARGV && $ARGV[0] eq '-b') { shift; $make_bin = 1 }

die "usage: $0 [-b] <file>\n" unless @ARGV;

sub slurp {
    open my $fh, '<', shift;
    <$fh>;
}

# Job queue. We parallelize slide processing for speeeed.
my @queue;

# Slides / pages are separated by "---" markers.
my @txts = split /^---\n/m, join("\n", map(s/\n//gr, slurp $ARGV[0]));

# Presentation-wide variables.
# Overridable per slide or globally, by directives in the deck file.
my %DEF = (
    INDENT   => 8, # page indent (columns, from west side)
    DRAWPNR  => 1, # draw page nr
    DRAWHOOK => 1, # draw fancy headings

    # This should be a monospace font.
    font     => 'PxPlus IBM VGA8',
    fontsize => 48,

    # format strings
    fhead => q(<span color="#ffffff">%s</span>), # heading
    fbold => q(<span color="#ffb700">%s</span>), # *emphasis*
    fpunc => q(<span color="#606060">%s</span>), # lines etc.
    fpnr  => q(<span color="#606060"> %02d / %02d </span>), # page nr.

    # default fore/background colors
    bg  => '#000000',
    fg  => '#b7b7b7',

    # bullet list characters
    bul1 => '→',
    bul3 => '*',
    bul2 => '►',
    bul4 => '□',
);

my $i = 0;       # current page
my $t = @txts-1; # total pages
page: for (@txts) {

    # Per-slide variables.
    # Re-cloned from global vars every page.
    my %V = %DEF;

    my $fn  = sprintf "%02d", $i;
    my $png = "$fn.png";
    my $rle = "$fn.rli";

    # imagemagick additional commandline options,
    # settable per slide from the deck file.
    my @opts;

    #my $txt = " \n";
    my $txt;

    s/^\s*//; s/\s*$//;

    for (split /\n/, $_) {
        chomp;
        s/ij/ĳ/g; s/IJ/Ĳ/g; # ;-)

        if (/^;(.*?)\s*$/) {
            # add imagemagick options
            if ($1 =~ /^!\s*(\S.*?)\s*$/) {
                @opts = split /\s+/, $1;
            }
            # evaluate perl-code
            # (used to override per-slide or global settings)
            if ($1 =~ /^\$\s*(\S.*?)\s*$/) {
                eval $1;
                # if "die" is processed, skip to the next slide.
                # needed to set global variables that apply to the first page.
                if ($@) { $t--; next page }
            }
            # comment?
            next;
        }

        my $indent = " "x$V{INDENT};

        if ($V{DRAWHOOK}) {
            # generate fancy headings.

            my $cols = 60; # a wild guess.
            my $fill = ' 'x$cols;
            if (m@^(#+\s*)(.*?)\s*$@) {
                #my $t = sprintf($V{fpunc}, $1)
                # these colors should be configurable instead...
                my $t = sprintf(q(<span color="#00b7ff">%s</span>), $1)
                      . sprintf($V{fhead}, $2);
                my $tl = length($1) + length($2);

                $_ = q(<span background="#0000b7">) . $fill . "</span>\n"
                   . q(<span background="#0000b7">)
                   . $indent . $t
                   . q(<span background="#0000b7">)
                   . ' 'x($cols - $tl)
                   . "</span>\n"
                   . q(<span background="#0000b7" color="#00b7ff">)
                   . '▁'x($cols)
                   . '</span>'
                   . q(</span>) and $indent = "";
            }
        } else {
            # generate basic heading
            s@^(#+\s*)(.*?)\s*$@
                  sprintf($V{fpunc}, $1)
                . sprintf($V{fhead}, $2)@e
        }

        # bullet points.
        s@\*(\S[^*]+\S)\*@sprintf($V{fbold}, $1)@ge;
        s@(?<=^\s{0})-(?=\s)@sprintf($V{fpunc}, $V{bul1})@e;
        s@(?<=^\s{2})-(?=\s)@sprintf($V{fpunc}, $V{bul2})@e;
        s@(?<=^\s{4})-(?=\s)@sprintf($V{fpunc}, $V{bul3})@e;
        $_ = $indent . $_ . $indent;

        $txt .= "$_\n";
    }

    #my $txt = " \n" . (join "\n", @src);
    my $pnr = sprintf $V{fpnr}, $i, $t;

    # run imagemagick... wait, let's create a job queue instead.

    push @queue, { nr => $i, job => sub {
        # run imagemagick with the generated pango markup.
        system "convert"
              # bg layer
              ,-size => "1920x1080"
              ,"-depth" => 8
              ,"canvas:$V{bg}"
              ,"+size"
              # extra options (may render images or center the page)
              ,@opts
              # the actual page text
              ,-background => "none"
              ,-fill       => $V{fg}
              ,-font       => $V{font}
              ,-pointsize  => $V{fontsize}
              ,"pango:$txt"
              ,"-composite"
              ,"+size"
              # the page number
              ,$V{DRAWPNR} ? (-gravity  => "SouthEast"
                             ,"pango:$pnr"
                             ,-geometry => "+32+32"
                             ,"-composite") : ()
              ,$png
                   and die "could not convert\n";

        # using an ImageMagick module / library would probably be cleaner,
        # but hey, this entire script is a hack, so who cares. :^)

        if ($make_bin) {
            # convert PNG -> RGBA -> RLE.
            system "./png2rgba.pl < '$png' | ./rlencode.pl > '$rle'"
                and die "could not convert to RLE\n";
        }
    }};

    ++$i # next slide!
}

# Running ImageMagick and then processing 16MiB of RGBA data per slide is
# apparently a bit heavy on the CPU, so let's parallelize slide generation.

my $MAX_PAR = 4; # add salt to taste.

my %pids;

my $done = 0;

sub progress {
    # printf "\rworking... [%2d -> \e[31m%2d\e[0m -> \e[32m%2d\e[0m]", (scalar @queue), (scalar keys %pids), $done;
    my ($a, $b, $c) = ($done, (scalar keys %pids), (scalar @queue));
    my $t = $a+$b+$c;
    my $s = $t < 10 ? 4 : $t < 20 ? 2 : 1;

    # I spend too much time on aesthetics...
    printf "\rworking... " . '%s'x9 . ' %d/%d  ',
           # "\e[32m", '█'x$a, "\e[0m",
           # "\e[33m", '█'x$b, "\e[0m",
           # "\e[31m", '█'x$c, "\e[0m";
           #"\e[32;41m", '█'x($s*$a), "\e[0m",
           # "\e[32;41m", '█'x($s*$a), "\e[0m",
           # "\e[32;41m", '▒'x($s*$b), "\e[0m",
           # "\e[32;41m", ' 'x($s*$c), "\e[0m",
           "\e[42m", ' 'x($s*$a), "\e[0m",
           "\e[43m", ' 'x($s*$b), "\e[0m",
           "\e[41m", ' 'x($s*$c), "\e[0m",
           # "", '+'x($s*$a), "",
           # "", '-'x($s*$b), "",
           # "", '.'x($s*$c), "",
           $a, $t;
}

$| = 1;

progress;
select undef, undef, undef, 0.1; # don't question it.

while ((keys %pids) or @queue) {
    while ((keys %pids) >= $MAX_PAR or ((keys %pids) and !@queue)) {
        # process finished jobs.
        my $p = wait;
        redo unless exists $pids{$p};
        $done++;
        delete $pids{$p};
        progress;
    }
    while (@queue and (keys %pids) < $MAX_PAR) {
        # enqueue jobs...
        my $item = shift @queue;
        my $p = fork;
        if ($p) { $pids{$p} = $item->{nr} } else { $item->{job}->(); exit }
        progress;
    }
}

# whoopwoop
say "\r".' 'x60 . "\rdone";
