#!/usr/bin/perl
while ($line = <>) {
  push @lines, $line;
}

while ($line = pop @lines) {
  print $line;
}
