#!/usr/bin/perl
# $Id$ $Name$
#
use strict;
use File::Basename;
use Getopt::Long;

my %args;

GetOptions (\%args,
            'convutildir=s', #Directory which contains dirac conversion utilities
            'hdrutildir=s', #Directory which contains dirac picheader utilities
			'onlyhdrs', #create only headers
			'width=i', #input frame width
			'height=i', #input frame height
			'num_frames=i', #number of input frames to use
			'frame_rate=s', #frames per sec
			'use', #Print usage information
			);

if ($args{use})
{	printUsage(); }

my $num_files = scalar @ARGV;

my @chroma = qw (411 420 422 444);

$args{convutildir} .= "/" if (defined $args{convutildir});
$args{hdrutildir} .= "/" if (defined $args{hdrutildir});

for ( my $i = 0; $i < scalar(@chroma) ; ++$i)
{
	my $conv = $args{convutildir}."RGBtoYUV".$chroma[$i];
	system ("$conv > /dev/null 2>&1") == 0 || die "$conv not found";
}

my $hdrutil =  $args{hdrutildir}."make_header";
system ("$hdrutil > /dev/null 2>&1") == 0 || die "$hdrutil not found";

for (my $i = 0; $i < $num_files; $i++)
{
	next if  $ARGV[$i] !~ /[^\d]*(\d+)x(\d+)x(\d+)\.rgb$/;
	my $width = $1;
	my $height = $2;
	my $num_frames = $3;

	$width = $args{width} if defined $args{width};
	$height = $args{height} if defined $args{height};
	$num_frames = $args{num_frames} if defined $args{num_frames};

    my $frame_rate_num = 25;
    my $frame_rate_denom = 1;

	if ($height == 288)
	{ $frame_rate_num = 13; }
	elsif ($height == 720)
	{ $frame_rate_num = 50; }

	if (defined $args{frame_rate})
	{
		if ($args{frame_rate} =~ /^\d+$/)
		{ $frame_rate_num =  $args{frame_rate}; }
		elsif ($args{frame_rate} =~ /^(\d+)\/(\d+)$/)
		{ $frame_rate_num = $1; $frame_rate_denom = $2; }
		else
		{ 
			print "Incorrect frame rate format ($args{frame_rate}). Ignoring...\n";
		}
	}
	#my $frame_rate = $frame_rate_num . "/" . $frame_rate_denom;
	my $fram_rate = $frame_rate_num;
	my $yuv_file = basename($ARGV[$i]);
	$yuv_file =~ s/\.rgb//g;
	print "\nConverting $ARGV[$i] - ";
	print "width=$width height=$height num_frames=$num_frames\n";

	for (my $j =0; $j < scalar (@chroma); $j++)
	{
		my $yuv = $yuv_file . "_" . $chroma[$j];
		if (!defined $args{onlyhdrs})
		{
			print "\tCreating $chroma[$j] yuv file\n";
			my $conv =$args{convutildir}. "RGBtoYUV" . $chroma[$j];
			system("$conv $width $height $num_frames < $ARGV[$i] > $yuv.yuv 2>/dev/null") == 0 || die "$conv failed: $!";
		}
		print "\tCreating $chroma[$j] hdr file\n";
		my $cformat = "format" . $chroma[$j];
		my $hdr = $args{hdrutildir} . "make_header";
		system("$hdr -xl $width -yl $height -framerate $frame_rate -cformat $cformat $yuv") == 0 || die "make_header failed: $!";
	}

	#now create the yonly file
	my $yuv420 = $yuv_file . "_420.yuv";
	my $yonly = $yuv_file . "_Yonly";
	if (!defined $args{onlyhdrs})
	{
		print "\tCreating Yonly yuv file\n";
		my $inframe_size = $width*$height*1.5;
		my $outframe_size = $width*$height;
		open(IN, $yuv420) || die "$yuv420 $!\n";
		open(OUT, ">", $yonly. ".yuv") || die "$yonly $!\n";
		while (1)
		{
		    my $r = read(IN, $_, $inframe_size);
			last if ($r != $inframe_size);
			syswrite (OUT, $_, $outframe_size);
		}
		close(IN);
		close(OUT);
	}
	print "\tCreating Yonly hdr file\n";
	my $hdr = $args{hdrutildir} . "make_header";
	system("$hdr -xl $width -yl $height -framerate $frame_rate -cformat Yonly $yonly") == 0 || die "make_header failed";
}

sub printUsage
{
	print "\nUsage: create_dirac_testfile.pl -<flag1>[=<flag_val]] ...  <input rgb files>\n";
	print "\nConverts rgb video to input formats required by Dirac. Expects the input files\n" ;
	print "to be in RGB24 format and end with the extension .rgb. The user can either\n" ;
	print "enter the sequence dimensions using the command line arguments, or can supply\n";
	print "the dimensions as part of the input file name in the format - \n" ;
	print "<name>-<width>x<height>x<num_frames>.rgb. E.g. snowboard-jum-720x576x50.rgb\n";


	print "\nThe command line arguments are\n";
	print "-convutildir=dir-name  dir-name is the directory where the Dirac conversion\n";
	print "                       utilities are available. If not specified, it looks\n";
	print "                       for the utilities in the directories in the PATH \n";
	print "                       variable.\n" ;
	print "-hdrutildir=dir-name   dir-name is the directory where the Dirac picheader\n";
	print "                       utilities are available. If not specified, it looks\n";
	print "                       for the utilities in the directories in the PATH \n";
	print "                       variable.\n" ;
	print "-onlyhdrs              only create the image header files\n" ;
	print "-width=w               w is the width of input frame\n" ;
	print "-height=h              h is the height of input frame\n" ;
	print "-num_frames=n          n is the number of input frames to use\n" ;
	print "-frame_rate=r          r is the input frames per sec. It can be specified\n";
	print "                       as an integer (25), or a rational number (30000/1001)\n";
	print "                       If not specified a default is used based on the frame\n";
	print "                       width\n";
	print "-use                   Print usage info and exit\n";
	print "\nExample:\n";
	print "\t create_dirac_testfile.pl -convutildir=/home/guest/dirac-0.5.0/util/conversion -hdrutildir=/home/guest/dirac-0.5.0/picheader -frame_rate=13 /home/guest/rgb/snowboard-jum-352x288x75.rgb /home/guest/rgb/squirrel-352x288x75.rgb\n";
	exit 0;
}
