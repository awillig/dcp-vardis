#!/usr/bin/perl

$parspec     = $ARGV[0]; shift;
$inputfile   = $ARGV[0]; shift;

($nodecntspec, $periodspec, $separationspec, $numrepspec) = split (/,/, $parspec);

open my $fhandle, $inputfile or die "Could not open file $inputfile: $!";

while (my $line = <$fhandle>) {
    if ($line =~ /#/)
    {
	print ($line);
    }
    if (!($line =~ /#/))
    {
	($nodecnt,$period,$separation,$numrep,$count,$seqnoCnt,$seqnoMean,$seqnoMin,$seqnoMax,$delayCnt,$delayMean,$delayMin,$delayMax) = split(/,/, $line);
	if (    (($nodecnt eq $nodecntspec)         || ($nodecntspec eq "*"))
	     && (($period eq $periodspec)           || ($periodspec eq "*"))
	     && (($separation eq $separationspec)   || ($separationspec eq "*"))
	     && (($numrep eq $numrepspec)           || ($numrepspec eq "*")))
	{
	    print ($line);
	}
    }
}

close $fhandle;
