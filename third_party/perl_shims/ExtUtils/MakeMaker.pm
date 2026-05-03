package ExtUtils::MakeMaker;
use strict;
use warnings;

package MM;
use strict;
use warnings;

sub maybe_command {
    my ($path) = @_;
    return undef unless defined $path && -f $path;
    return $path;
}

1;
