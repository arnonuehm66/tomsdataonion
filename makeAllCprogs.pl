#!/usr/bin/perl
#*******************************************************************************
#** Name: makeAllCprogs.pl
#** Purpose:  Creates all C programms in path '~/bin/Cpp/' with 'gcc' on system.
#** Author: (JE) Jens Elstner <jens.elstner@bka.bund.de>
#*******************************************************************************
#** Date        User  Changelog
#**-----------------------------------------------------------------------------
#** 31.01.2018  JE    Created program.
#** 27.05.2019  JE    Added '-9' for forcing C99 standard compilation.
#** 27.05.2019  JE    Added '-l' for adding libraries for compilation.
#** 20.01.2020  JE    Now strips the programs after compilation.
#** 04.02.2020  JE    Added '-O' for gcc optimzations (-O{0,1,2,3,g,s}).
#** 04.02.2020  JE    Updated usage text accordingly.
#** 11.03.2020  JE    Now '-O s' and '-l pcre2-8' is default.
#** 11.03.2020  JE    Updated usage text accordingly.
#** 02.04.2020  JE    Added '-p' to give path where to save the compiled progs.
#** 14.05.2020  JE    Changed default '-O s' to '-O fast'.
#** 18.06.2020  JE    Added '-lm' for automatic math.h integration.
#** 10.09.2020  JE    Added '-lcrypto' for automatic openssl integration.
#** 28.01.2021  JE    Added getMeBinDir() to set 'bindir' var to default or via
#**                   environment variable 'NAVIFOR_BIN'.
#** 28.01.2021  JE    Changed '-p' to '-b'.
#** 08.03.2021  JE    Added '--dellibs' for deleting default libraries.
#** 07.04.2021  JE    Now all '-llib' are at the end of gcc, 'cause all debians.
#*******************************************************************************


#*******************************************************************************
#* pragmas

use 5.014;
use strict;
use File::Find;
use File::Spec;


#*******************************************************************************
#* infos

my $g_meversion = '0.10.1';
my $g_mename    = getMeName();
my $g_mehome    = getMeHome();
my $g_mebindir  = getMeBinDir(); # Needs $g_mehome


#*******************************************************************************
#* constants

# usage()
use constant ERR_NOERR => 0x00;
use constant ERR_ARGS  => 0x01;
use constant ERR_FILE  => 0x02;
use constant ERR_ELSE  => 0xff;

use constant sERR_ARGS => 'Argument error';
use constant sERR_FILE => 'File error';
use constant sERR_ELSE => 'Unknown error';


#*******************************************************************************
#* global variables

# Arguments
my %g_a    = ();
my @g_args = ();


#*******************************************************************************
#* functions

#*******************************************************************************
#* Name:  version
#* Purpose: Prints version of script and end it.
#*******************************************************************************
sub version() {
  print "$g_mename v$g_meversion\n";
  exit(ERR_NOERR);
}

#*******************************************************************************
#* Name:  getMeName
#* Purpose: Gets calling name of the script.
#*******************************************************************************
sub getMeName() {
  $0 =~ /([^\\\/]+)$/;
  return $1;
}

#*******************************************************************************
#* Name:  getMeHome
#* Purpose: Gets my home directory.
#*******************************************************************************
sub getMeHome() {
  my $user = ($ENV{"SUDO_USER"} ne '') ? $ENV{"SUDO_USER"} : $ENV{"USER"};
  my $home = ($user eq 'root') ? '/root' : "/home/$user";

  $home = $ENV{"USERPROFILE"} if $^O eq 'MSWin32';
  $home = removeSlash($home);
  return $home;
}

#*******************************************************************************
#* Name:  getMeName
#* Purpose: Gets calling name of the script.
#*******************************************************************************
sub getMeBinDir() {
  my $path = ($ENV{'NAVIFOR_BIN'} ne '')
           ? $ENV{'NAVIFOR_BIN'}
           : "$g_mehome/bin";
  $path = removeSlash($path);
  return $path;
}

#*******************************************************************************
#* Name:  usage
#* Purpose: Prints usage of script and end it.
#*******************************************************************************
sub usage($;$) { my ($err, $txt) = @_;
  my $msg = '';

  # Ensure text ends appropriate, if printed.
  chomp($txt);
  $txt = "$txt\n\n" if defined $txt;

  #Summary:************************ 80 chars width ****************************************
  $msg .= "usage: $g_mename [-t] [-9] [-O n] [-b <path>] [--dellibs] [-l <lib1> [-l <lib2> ...]] [path1 path2 ...]\n";
  $msg .= "       $g_mename [-h|--help|-v|--version]\n";
  $msg .= " Creates all C programms in path '~/bin/Cpp/' or path given at cli with the\n";
  $msg .= " 'gcc' compiler on your system and strips all symbols.\n";
  $msg .= " 'path' given points to folder with at least one 'main.c[pp]' file.\n";
  $msg .= " Try to use '-O s' first, which will optimize for speed. '-O 2' should also be\n";
  $msg .= " OK, but use '-O 3' with care, because it could break some code. '-O 0 will\n";
  $msg .= " just switch off optimization. See 'man gcc' for more indepth informations.\n";
  $msg .= "  -t:            don't execute printed commands (default execute)\n";
  $msg .= "  -9:            explicitly compile C99 standard, else with system preferences\n";
  $msg .= "  -O n:          compile with gcc optimzations (n = 0, 1, 2, 3, fast or s)\n";
  $msg .= "                 (default '-O fast' for speed)\n";
  $msg .= "  -b <path>:     path to compiled programs (default '~/bin/')\n";
  $msg .= "  --dellibs:     delete default libs, use prior use of any other '-l'\n";
  $msg .= "  -l <lib>:      add a lib for each '-l' (default -l pcre2-8 -l m -l crypto)\n";
  $msg .= "  -h|--help:     print this help\n";
  $msg .= "  -v|--version:  print version of program\n";
  #Summary:************************ 80 chars width ****************************************

  # Print to appropriate output channel.
  if ($err == ERR_NOERR) {
    print STDOUT "$txt$msg";
  }
  else {
    print STDERR "$txt$msg";
  }

  exit($err);
}

#*******************************************************************************
#* Name:  dispatchError
#* Purpose: Print out specific error message, if any occurres.
#*******************************************************************************
sub dispatchError(@) { my ($rv, $msg) = @_;
  my $txt = ($msg ne '') ? ": $msg" : '';

  usage(ERR_ARGS, sERR_ARGS . $txt) if $rv == ERR_ARGS;
  usage(ERR_FILE, sERR_FILE . $txt) if $rv == ERR_FILE;
  usage(ERR_ELSE, sERR_ELSE . $txt) if $rv == ERR_ELSE;
}

#*******************************************************************************
#* Name:  removeSlash
#* Purpose: Removes slash to path if existing. Do not alter empty or undef paths.
#*******************************************************************************
sub removeSlash($) { my ($path) = @_;
  $path =~ s/[\/\\]$//;
  return $path;
}

#*******************************************************************************
#* Name:  getOptions
#* Purpose: Filters command line.
#*******************************************************************************
sub getOptions(@) { my (@args) = @_;
  my $arg = '';
  my $opt = '';
  my $tmp = '';

  # Set defaults.
  $g_a{'testMode'} = 0;
  $g_a{'c99'}      = '';
  $g_a{'O'}        = ' -Ofast';
  $g_a{'bindir'}   = $g_mebindir;
  $g_a{'l'}        = ' -lpcre2-8 -lm -lcrypto';
  @g_args          = ();

  # Loop all arguments from command line POSIX style.
argument:
  while (defined($arg = shift(@args))) {

    # Long options:
    if ($arg =~ /^--/) {
      if ($arg eq '--help') {
        usage(ERR_NOERR);
      }
      if ($arg eq '--version') {
      version();
      }
      if ($arg eq '--dellibs') {
        $g_a{'l'} = '';
        next;
      }
      return (ERR_ARGS, 'Invalid long option');
    }

    # Short options:
    if ($arg =~ /^-([^-].*)/) {
      $arg = $1;
      while ($arg =~ /(.)/g) {
        $opt = $1;
        if ($opt eq 'h') {
          usage(ERR_NOERR);
        }
        if ($opt eq 'v') {
          version();
        }
        if ($opt eq 't' ) {
          $g_a{'testMode'} = 1;
          next;
        }
        if ($opt eq '9' ) {
          $g_a{'c99'} = ' -std=c99';
          next;
        }
        if ($opt eq 'O' ) {
          $tmp      = shift(@args);
          $g_a{'O'} = " -O$tmp";
          next;
        }
        if ($opt eq 'b' ) {
          $g_a{'bindir'} = removeSlash(shift(@args));
          next;
        }
        if ($opt eq 'l' ) {
          # -l mylib -> -lmykib
          $tmp       =  shift(@args);
          $g_a{'l'} .=  " -l$tmp";
          next;
        }
        return (ERR_ARGS, 'Invalid short option');
      }
      next argument;
    }

    # All else are treated as free arguments.
    push(@g_args, $arg);
  }

  # Sanity check of arguments and flags.
  $g_args[0] = "$g_mebindir/Cpp/" if @g_args == 0;

  # Get absolute path no matter if it was given.
  foreach my$file (@g_args) {
    $file = File::Spec->rel2abs($file);
  }

  # All is set and done.
  return ERR_NOERR;
}

#*******************************************************************************
#* Name:  cmd
#* Purpose: Prints what will be done and do it.
#*******************************************************************************
sub cmd($) { my ($cmd) = @_;
  my $rv = '';

  print "$g_mename: [$cmd]\n";
  $rv = `$cmd` if not $g_a{'testMode'};

  chomp($rv);
  return $rv;
}

#*******************************************************************************
#* Name:  compile
#* Purpose: Compiles found 'main.c[pp]# file.
#*******************************************************************************
sub compile() {
  # $File::Find::dir  = /some/path/
  # $_                = foo.ext
  # $File::Find::name = /some/path/foo.ext

  # Compile main.c and main.cpp only.
  return if $_ !~ /^main\.c(?:pp)?$/;

  # Derive name from directory path.
  my ($name) = $File::Find::dir =~ /([^\/]+)$/;

  # Check, if no name could be derived from path.
  return if $name eq '';

  my $rv = cmd("gcc -Wall$g_a{'O'}$g_a{'c99'} -o $g_a{'bindir'}/$name $File::Find::name$g_a{'l'}");
  print "$rv";
     $rv = cmd("strip $g_a{'bindir'}/$name");
  print "$rv";
}


#*******************************************************************************
#* main

sub main() {
  # Print apropriate error message, if necessary.
  dispatchError(getOptions(@ARGV));

  find(\&compile, @g_args);

  exit(ERR_NOERR);
}


#*******************************************************************************
#* start programm
main()
__END__
