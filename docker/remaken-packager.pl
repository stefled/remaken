#!/usr/bin/perl
# author Loic Touraine <loic.touraine@b-com.com>
# copyright (C) Copyright b<>com\n
#            This software is the confidential and proprietary
#            information of b<>com. You shall not disclose such
#            confidential information and shall use it only in
#            accordance with the terms of the license agreement
#            you entered into with b<>com.


#strategies :
#lib debug and release built with suffix
#only debug or release build with suffix
#only debug or release build without suffix
#usage samples :
# -s . -d ~/tmp/packages -p intel-tbb -v 2017.5 -l build/macos_intel64_clang_cc8.0.0_os10.11.6_release -m release -i include
# -s . -d ~/tmp/packages -p intel-tbb -v 2017.5 -l build/macos_intel64_clang_cc8.0.0_os10.11.6_debug -m debug -i include -w _debug
# -s . -d ~/tmp/packages -p intel-ipp -v 8.2 -l ipp/ -n -i ipp/include/ -r Documentation/en_US/ipp/redist.txt
#

use warnings ;
use strict ;
use Term::ReadKey;
use Getopt::Long qw(GetOptions);
use File::Basename;
use File::Spec;
use File::Find;
use File::Copy;
use File::Path qw(make_path remove_tree);

my $sourcedir;
my $includedir;
my $libdir;
my $redistFile;
my $destinationdir;
my $ignoreMode;
my $mode;
my $cflagsDepth;
my $arch = "x86_64";
my $debugLibSuffix;
my $pkgname;
my $pkgversion;
my $pkgOSbase;
my $useOriginalPCfiles;
my $percomponentPCfile;
my $help;
my $command;
my $osname;

my $pkgConfigPfx = "remaken-";
my $libext="a";
my $dynlibext="so";
my $headerExtensions="(\.h|\.hpp)";
my $libpfx="lib";
my $pkgLinkModeOverride="static";

if ($^O =~ /Win/) {
    $osname="win";
}
if ($^O =~ /darwin/) {
    $osname="mac";
}
if ($^O =~ /linux/) {
    $osname="linux";
}

Getopt::Long::Configure qw(gnu_getopt);

# commands:

my $debugEnabled = 1;


sub printDebug($) {
    my $message=shift;
    if ($debugEnabled) {
	print $message."\n";
    }
}

sub printTrace($) {
    my $message=shift;
    print $message."\n";
}

sub usage {
    print "OVERVIEW: b<>com packager tool to generate package and pkgconfig files from thirdparties build output\n";
    print "\n";
    print "USAGE: ".$0." -s [ path_to_root_productdir ] -d [ path_to_destination_dir ] -p [ package_name ] -v [ package_version ] <options>\n";
    print "\n";
    print "OPTIONS:\n";
    print "    -s, --sourcedir                  => product root directory (where libs and includes are located)\n";
    print "    -o, --osname                     => specify the operating system targeted by the product build. It is one of [win|mac|linux]. (defaults to the current OS environment)\n";
    print "    -i, --includedir                 => relative path to include folder to export (defaults to the sourcedir provided with -s)\n";
    print "    -l, --libdir                     => relative path to the library folder to export (defaults to the sourcedir provided with -s)\n";
    print "    -r, --redistfile                 => relative path and filename of a redistribution file to use (such as redist.txt intel ipp's file). Only listed libraries in this file will be packaged\n";
    print "    -d, --destinationdir             => package directory root destination (where the resulting packaging will be stored)\n";
    print "    -p, --packagename                => package name\n";
    print "    -v, --packageversion             => package version\n";
    print "    -n, --ignore-mode                => forces the pkg-config generated file to ignore the mode when providing -L flags\n";
    print "    -m, --mode [debug|release]       => specify the current product build mode. Binaries will be packaged in the appropriate [mode] folder\n";
    print "    -a, --architecture [x86_64|i386] => specify the current product build architecture. Binaries will be packaged in the appropriate [architecture] folder\n";
    print "    -w, --withsuffix suffix          => specify the suffix used by the thirdparty when building with mode mode\n";
    print "    -u, --useOriginalPCfiles         => specify to search and use original pkgconfig files from the thirdparty, instead of generating them\n";
    print "    -h, --help                       => display this help\n";
    print "\n";
    print "EXAMPLES:\n";
    print "    -s {path_to_root_sourcedir} -d {path_to_destination_dir} -p intel-tbb -v 2017.5 -l build/macos_intel64_clang_cc8.0.0_os10.11.6_release -m release -i include\n";
    print "    -s {path_to_root_sourcedir} -d {path_to_destination_dir} -p intel-tbb -v 2017.5 -l build/macos_intel64_clang_cc8.0.0_os10.11.6_debug -m debug -i include -w _debug\n";
    print "    -s {path_to_root_sourcedir} -d {path_to_destination_dir} -p intel-ipp -v 8.2 -l ipp/ -n -i ipp/include/ -r Documentation/en_US/ipp/redist.txt\n";
    print "\n";
}


my @originalPkgconfigFiles;

sub originalPkgconfigParser {
    my($filename, $directory, $suffix) = fileparse($File::Find::name,qr/\.[^.]*/);
    if ($suffix eq ".pc") {
	push(@originalPkgconfigFiles,$File::Find::name);
    }
}

GetOptions ("sourcedir|s=s" => \$sourcedir,
	    "includedir|i=s" => \$includedir,
	    "libdir|l=s" => \$libdir,
	    "redistfile|r=s" => \$redistFile,
	    "destination|d=s" => \$destinationdir,
	    "packagename|p=s" => \$pkgname,
	    "packageversion|v=s" => \$pkgversion,
	    "ignore-mode|n" => \$ignoreMode,
	    "mode|m=s" => \$mode,
	    "architecture|a=s" => \$arch,
	    "withsuffix|w=s" => \$debugLibSuffix,
	    "useOriginalPCfiles|u" =>  \$useOriginalPCfiles,
	    "cflagsdepth|h=i" => \$cflagsDepth,
	    "osname|o=s" => \$pkgOSbase,
	    "percomponentPCfile|c" => \$percomponentPCfile,
	    'help|?|h' => \$help)
    or die("Error in command line arguments\n");
if (not $help) {
    if  (not $sourcedir or not $pkgname or not $pkgversion or not $destinationdir or (not $ignoreMode and not $mode) ) {
        usage();
        die "Check your options : error in command options provided";
    }
    else {
    if (!$pkgOSbase) {
        $pkgOSbase = $osname;
    }
    if ($pkgOSbase !~ /^(win|mac|linux)$/) {
        usage();
        die "Check your options : bad os name provided";
    }
    if ($pkgOSbase =~ /win/) {
        $libpfx="''";
        $libext="lib";
        $dynlibext="dll";
    }
    if ($pkgOSbase =~ /mac/) {
        $dynlibext="dylib";
    }
	if (! -w $destinationdir) {
	    die "Check your options : you haven't write permissions on [".$destinationdir."] folder";
	}
	if ($ignoreMode and not $mode) {
	    $mode = "release";
	}
	elsif ($mode ne "debug" and $mode ne "release") {
	    usage();
	    die "Check your options : error in command options provided";
	}
	if ($arch ne "x86_64" and $arch ne "i386") {
	    usage();
	    die "Check your options : error in command options provided";
	}
	if (not $includedir) {
	    $includedir = $sourcedir;
	}
	if (not $libdir) {
	    $libdir = $sourcedir;;
	}
	if ($useOriginalPCfiles) {
	    find(\&originalPkgconfigParser, $sourcedir);
	    if (!@originalPkgconfigFiles) {
		die "Error : no original pkgconfig file found in ".$sourcedir;
	    }
	}
    }
} else {
    usage();
    die;
}

my $separator = ';';

my ($ldflags,$cflags,$libflags);

sub createPkgconfigContent($$$$$) {
    my ($libname,$pkgversion,$ldflags,$cflags,$libflags) = @_;
    my $pkgConfigFileContent = "libname=".$libname."\n";
    $pkgConfigFileContent .= "prefix=/usr/local\n";
    $pkgConfigFileContent .= "exec_prefix=\${prefix}\n";
    $pkgConfigFileContent .= "libdir=\${exec_prefix}/lib\n";
    $pkgConfigFileContent .= "includedir=\${prefix}/interfaces\n";
    $pkgConfigFileContent .= "\n";
    $pkgConfigFileContent .= "Name: ".$libname."\n";
    $pkgConfigFileContent .= "Description: \n";
    $pkgConfigFileContent .= "Version: ".$pkgversion."\n";
    $pkgConfigFileContent .= "Requires:\n";
    $pkgConfigFileContent .= "Libs: ".$ldflags."\n";
    $pkgConfigFileContent .= "Libs.private: ".$libflags."\n";
    $pkgConfigFileContent .= "Cflags:".$cflags."\n";
    return $pkgConfigFileContent;
}

my %redistLibraries;
my @sharedLibraries;
my @staticLibraries;
my %includeFiles;
my @includeFolders;

sub addLibraryInfos($) {
    my $file = shift;
    my($filename, $directory, $suffix) = fileparse($file,qr/\.[^.]*/);
    my $ldDirectory;
    $filename =~ s/^$libpfx//g;

    if (!$ignoreMode) {
	$ldDirectory = "\${libdir}";
    }
    else {
	$ldDirectory = "\${prefix}/lib/".$arch;
    }

    if($suffix eq ".".$dynlibext) {
	if (! -l $_) {
	    $ldflags .= " -l".$filename;
	}
	if ($ignoreMode) {
	    $ldDirectory = " -L".$ldDirectory."/shared/".$mode;
	}
	else {
	    $ldDirectory = " -L".$ldDirectory;
	}
	if ($ldflags !~ /\Q$ldDirectory/) {
	    $ldflags .= $ldDirectory;
	}
	push(@sharedLibraries, $file);
    }

    if($suffix eq ".".$libext) {
	if ($ignoreMode) {
	    $ldDirectory = $ldDirectory."/static/".$mode;
	}
	my $destlibpath = File::Spec->catfile($ldDirectory,"\${pfx}".$filename.".\${lext}");
	$libflags .= " ".$destlibpath;
	push(@staticLibraries, $file);
    }
}


sub addLibraries($) {
    my $file = shift;
    my($filename, $directory, $suffix) = fileparse($file,qr/\.[^.]*/);
    if ($directory =~ /$libdir/) {
	if ($redistFile)  {
	    if (/\Q$filename$suffix/ ~~ %redistLibraries) {
		addLibraryInfos($file);
	    }
	}
	else {
	    addLibraryInfos($file);
	}
    }
}

sub addIncludes($) {
    my $file = shift;
    my($filename, $directory, $suffix) = fileparse($file,qr/\.[^.]*/);
    if ($includedir !~ /\Q$sourcedir/) {
	$includedir = File::Spec->catfile($sourcedir,$includedir);
    }
    if ($directory =~ /\Q$includedir/) {
	if ($suffix eq ".h" or $suffix eq ".hpp" or $suffix eq ".ipp") {
	    my $targetIncludeFolder = $directory;
	    $targetIncludeFolder =~ s!$includedir[/]*!!;
	    if ($cflags !~ /$targetIncludeFolder/) {#need to handle flat/non flat include path when dependent includes are relative in inclusion
		$cflags .= " -I\${includedir}/".$targetIncludeFolder;
		push(@includeFolders, $targetIncludeFolder);
	    }
	    $includeFiles{$File::Find::name} = $targetIncludeFolder;
	}
    }
}

sub fileParser {
    my($filename, $directory, $suffix) = fileparse($File::Find::name,qr/\.[^.]*/);
    if($suffix eq ".".$dynlibext or $suffix eq ".".$libext) {
	if ($debugLibSuffix) {
	    if ($filename =~ /\Q$debugLibSuffix/) {
		addLibraries($File::Find::name);
	    }
	}
	else {
	    addLibraries($File::Find::name);
	}
    }
    addIncludes($File::Find::name);
}

sub packageIncludes ($$) {
    my ($rootPkgDir,$includeFolders) = @_;

    foreach my $folder (@{$includeFolders}) {
	my $includepath = File::Spec->catfile($rootPkgDir,"interfaces",$folder);
	printDebug("Creating ".$includepath);
	if (!-e $includepath) {
	    make_path($includepath);
	}
    }

    foreach my $file (keys(%includeFiles)) {
	my $destDir = File::Spec->catfile($rootPkgDir,"interfaces",$includeFiles{$file});
	printDebug("Copying ".$file." to ".$destDir);
	copy($file,$destDir) or die "Copy failed: $!";
    }
}

# handle symlinks properly
sub copyEx ($$) {
    my ($source,$destination) = @_;
    printDebug("Copying ".$source." to ".$destination);
    if ($osname eq "mac") {
	system("cp","-pPR",$source,$destination);
    }
    else {
	copy($source,$destination) or die "Copy failed: $!";
    }
}

sub packageLibraries($$) {
    my ($libpath,$libraries) = @_;
    if (! -e $libpath) {
	make_path($libpath);
    }
    foreach my $lib (@{$libraries}) {
	copyEx($lib,$libpath);
    }
}

sub packagePkgconfigFiles($) {
    my $rootPkgDir = shift;
    my $pkgConfigFilename = $pkgConfigPfx.$pkgname.".pc";
    if ($debugLibSuffix) {
	    $pkgConfigFilename = $pkgConfigPfx.$mode."-".$pkgname.".pc";
    }

    my $remakenPkgConfigFilename=File::Spec->catfile($rootPkgDir,$pkgConfigFilename);

    if (!$useOriginalPCfiles) {
        open(my $fileOutHandler,">".$remakenPkgConfigFilename) or die("ERROR: can't open ".$remakenPkgConfigFilename."\n");;
        print $fileOutHandler createPkgconfigContent($pkgname,$pkgversion,$ldflags,$cflags,$libflags);
        close($fileOutHandler);
    }
    else {
        foreach my $srcFile (@originalPkgconfigFiles) {
            my($filename, $directory, $suffix) = fileparse($srcFile,qr/\.[^.]*/);
            my $pkgConfigPfxIndex = index($filename,$pkgConfigPfx);
            if ($pkgConfigPfxIndex == 0) { # found a dedicated remaken pkgconfig
                $remakenPkgConfigFilename=File::Spec->catfile($rootPkgDir,$pkgConfigPfx.$filename.$suffix);
            }
            else {
                $remakenPkgConfigFilename=File::Spec->catfile($rootPkgDir,$filename.$suffix);
            }
            copy($srcFile,$remakenPkgConfigFilename) or die "Copy failed: $!";
        }
    }
}

sub parseRedist {
    if ($redistFile) {
	open(my $fileInHandler,"<".$redistFile) or die("ERROR: can't open ".$redistFile."\n");
	while (defined (my $line = <$fileInHandler>)) {
	    if ($line =~ /<installdir>.*lib.*\.($libext|$dynlibext)$/) {
		$line =~ s!<installdir>\/!!g;
		chomp($line);
		$redistLibraries{$line} = 1;
		#print "$_\n" for keys %redistLibraries;
	    }
	}
	close($fileInHandler);
    }
}

sub preparePackage($$$$$$) {
    my ($srcdir,$destinationdir,$pkgname,$pkgversion,$arch,$mode) = @_;
    my $rootPkgDir = File::Spec->catfile($destinationdir,$pkgname,$pkgversion);
    print  $rootPkgDir;
    if (! -e $rootPkgDir) {
	make_path($rootPkgDir);
    }

    parseRedist();

    find(\&fileParser, $srcdir);
    if (@includeFolders) {
	packageIncludes($rootPkgDir,\@includeFolders);
    }


    if (@sharedLibraries) {
	my $sharedlibpath = File::Spec->catfile($rootPkgDir,"lib",$arch,"shared",$mode);
	packageLibraries($sharedlibpath,\@sharedLibraries);
    }

    if (@staticLibraries) {
	my $staticlibpath = File::Spec->catfile($rootPkgDir,"lib",$arch,"static",$mode);
	packageLibraries($staticlibpath,\@staticLibraries);
    }

    packagePkgconfigFiles($rootPkgDir);
}

preparePackage($sourcedir,$destinationdir,$pkgname,$pkgversion,$arch,$mode);
