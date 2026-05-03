package Pod::Usage;
use strict;
use warnings;
use Exporter 'import';
our @EXPORT_OK = qw(pod2usage);
our @EXPORT = qw(pod2usage);

sub pod2usage {
    my %args = @_ == 1 ? ( -message => $_[0] ) : @_;
    my $message = $args{-message} || $args{message} || '';
    print STDERR "$message\n" if $message ne '';
    exit($args{-exitval}) if defined $args{-exitval};
}

1;
