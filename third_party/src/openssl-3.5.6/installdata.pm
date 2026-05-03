package OpenSSL::safe::installdata;

use strict;
use warnings;
use Exporter;
our @ISA = qw(Exporter);
our @EXPORT = qw(
    @PREFIX
    @libdir
    @BINDIR @BINDIR_REL_PREFIX
    @LIBDIR @LIBDIR_REL_PREFIX
    @INCLUDEDIR @INCLUDEDIR_REL_PREFIX
    @APPLINKDIR @APPLINKDIR_REL_PREFIX
    @ENGINESDIR @ENGINESDIR_REL_LIBDIR
    @MODULESDIR @MODULESDIR_REL_LIBDIR
    @PKGCONFIGDIR @PKGCONFIGDIR_REL_LIBDIR
    @CMAKECONFIGDIR @CMAKECONFIGDIR_REL_LIBDIR
    $COMMENT $VERSION @LDLIBS
);

our $COMMENT                    = '';
our @PREFIX                     = ( 'C:/Users/matis/Documents/Hesia-Firmware/open_source/hesia-core/third_party/install/windows-x86_64/openssl' );
our @libdir                     = ( 'C:/Users/matis/Documents/Hesia-Firmware/open_source/hesia-core/third_party/install/windows-x86_64/openssl/lib64' );
our @BINDIR                     = ( 'C:/Users/matis/Documents/Hesia-Firmware/open_source/hesia-core/third_party/install/windows-x86_64/openssl/bin' );
our @BINDIR_REL_PREFIX          = ( 'bin' );
our @LIBDIR                     = ( 'C:/Users/matis/Documents/Hesia-Firmware/open_source/hesia-core/third_party/install/windows-x86_64/openssl/lib64' );
our @LIBDIR_REL_PREFIX          = ( 'lib64' );
our @INCLUDEDIR                 = ( 'C:/Users/matis/Documents/Hesia-Firmware/open_source/hesia-core/third_party/install/windows-x86_64/openssl/include' );
our @INCLUDEDIR_REL_PREFIX      = ( 'include' );
our @APPLINKDIR                 = ( 'C:/Users/matis/Documents/Hesia-Firmware/open_source/hesia-core/third_party/install/windows-x86_64/openssl/include/openssl' );
our @APPLINKDIR_REL_PREFIX      = ( 'include/openssl' );
our @ENGINESDIR                 = ( 'C:/Users/matis/Documents/Hesia-Firmware/open_source/hesia-core/third_party/install/windows-x86_64/openssl/lib64/engines-3' );
our @ENGINESDIR_REL_LIBDIR      = ( 'engines-3' );
our @MODULESDIR                 = ( 'C:/Users/matis/Documents/Hesia-Firmware/open_source/hesia-core/third_party/install/windows-x86_64/openssl/lib64/ossl-modules' );
our @MODULESDIR_REL_LIBDIR      = ( 'ossl-modules' );
our @PKGCONFIGDIR               = ( 'C:/Users/matis/Documents/Hesia-Firmware/open_source/hesia-core/third_party/install/windows-x86_64/openssl/lib64/pkgconfig' );
our @PKGCONFIGDIR_REL_LIBDIR    = ( 'pkgconfig' );
our @CMAKECONFIGDIR             = ( 'C:/Users/matis/Documents/Hesia-Firmware/open_source/hesia-core/third_party/install/windows-x86_64/openssl/lib64/cmake/OpenSSL' );
our @CMAKECONFIGDIR_REL_LIBDIR  = ( 'cmake/OpenSSL' );
our $VERSION                    = '3.5.6';
our @LDLIBS                     =
    # Unix and Windows use space separation, VMS uses comma separation
    $^O eq 'VMS'
    ? split(/ *, */, '-lws2_32 -lgdi32 -lcrypt32 ')
    : split(/ +/, '-lws2_32 -lgdi32 -lcrypt32 ');

1;
