package Locale::Maketext::Simple;
use strict;
use warnings;

sub import {
    my $class = shift;
    my $caller = caller;
    no strict 'refs';
    *{"${caller}::loc"} = \&loc;
}

sub loc {
    my ($message, @args) = @_;
    return '' unless defined $message;
    for (my $i = 0; $i < @args; $i++) {
        my $n = $i + 1;
        my $value = defined $args[$i] ? $args[$i] : '';
        $message =~ s/%$n/$value/g;
    }
    return $message;
}

1;
