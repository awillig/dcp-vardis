#!/usr/bin/perl

@nodecounts   = (5, 7, 9, 11);
@periods      = (200, 1000);
@separations  = (255, 263);
@numreps      = (1, 2, 3);

$targetdir  = $ARGV[0]; shift;
$outpformat = $ARGV[0]; shift;

(($outpformat eq "verbose") or ($outpformat eq "csv")) || die "invalid output format";

opendir my $dir, $targetdir or die "Cannot open directory";
my @files = readdir $dir;
closedir $dir;

if ($outpformat eq "csv")
{
    print ("#nodecnt,period,separation,numrep,replcount,avgseqnogapcount,avgseqnogapmean,avgseqnogapmin,avgseqnogapmax,avgdelaycount,avgdelaymean,avgdelaymin,avgdelaymax\n");
}

for my $nodecount (@nodecounts) {
    for my $period (@periods) {
	for my $separation (@separations) {
	    for my $numrep (@numreps) {
		$matchpat = "Vardis-numRepetitions=$numrep,beaconPeriod=50ms,numSummaries=10,updatePeriod=exponential\\($period\\),separation=$separation,nodeCntX=$nodecount,.*\\.sca";
		#print("$matchpat\n");
		my $count         = 0;
		
		my $seqnoCntAcc   = 0;
		my $seqnoMeanAcc  = 0;
		my $seqnoMinAcc   = 0;
		my $seqnoMaxAcc   = 0;

		my $delayCntAcc   = 0;
		my $delayMeanAcc  = 0;
		my $delayMinAcc   = 0;
		my $delayMaxAcc   = 0;
		
		for my $file (@files) {
		    if ($file =~ /$matchpat/)
		    {
			#print ("     $file\n");
			$count = $count + 1;
			
			$file =~ s/\(/\\\(/g;
			$file =~ s/\)/\\\)/g;
			#print ("   matching filename: $file\n");

			# -------------------------------------------------------------
			
			$seqnoStr = `opp_scavetool -l $targetdir$file | grep "nodes.0..application.*seqnoDelta" | grep -v histogram`;
			chop($seqnoStr);

			$seqnoCnt = $seqnoStr;
			$seqnoCnt =~ s/.*count=//g;
			($seqnoCnt, $dontcare) = split /[\s]+/, $seqnoCnt, 2;
			$seqnoCntAcc = $seqnoCntAcc + $seqnoCnt;

			$seqnoMean = $seqnoStr;
			$seqnoMean =~ s/.*mean=//g;
			($seqnoMean, $dontcare) = split /[\s]+/, $seqnoMean, 2;
			$seqnoMeanAcc = $seqnoMeanAcc + $seqnoMean;

			$seqnoMin = $seqnoStr;
			$seqnoMin =~ s/.*min=//g;
			($seqnoMin, $dontcare) = split /[\s]+/, $seqnoMin, 2;
			$seqnoMinAcc = $seqnoMinAcc + $seqnoMin;

			$seqnoMax = $seqnoStr;
			$seqnoMax =~ s/.*max=//g;
			($seqnoMax, $dontcare) = split /[\s]+/, $seqnoMax, 2;
			$seqnoMaxAcc = $seqnoMaxAcc + $seqnoMax;
			
			#print ("   outp1: $seqnoStr\n");
			#print ("   outp2: count = $seqnoCnt, mean = $seqnoMean, min = $seqnoMin, max = $seqnoMax\n");
			#print ("   outp3: countAcc = $seqnoCntAcc, meanAcc = $seqnoMeanAcc, minAcc = $seqnoMinAcc, maxAcc = $seqnoMaxAcc\n");


			# -------------------------------------------------------------
			
			$delayStr = `opp_scavetool -l $targetdir$file | grep "nodes.0..application.*updateDelay" | grep -v histogram`;
			chop($delayStr);

			$delayCnt = $delayStr;
			$delayCnt =~ s/.*count=//g;
			($delayCnt, $dontcare) = split /[\s]+/, $delayCnt, 2;
			$delayCntAcc = $delayCntAcc + $delayCnt;

			$delayMean = $delayStr;
			$delayMean =~ s/.*mean=//g;
			($delayMean, $dontcare) = split /[\s]+/, $delayMean, 2;
			$delayMeanAcc = $delayMeanAcc + $delayMean;

			$delayMin = $delayStr;
			$delayMin =~ s/.*min=//g;
			($delayMin, $dontcare) = split /[\s]+/, $delayMin, 2;
			$delayMinAcc = $delayMinAcc + $delayMin;

			$delayMax = $delayStr;
			$delayMax =~ s/.*max=//g;
			($delayMax, $dontcare) = split /[\s]+/, $delayMax, 2;
			$delayMaxAcc = $delayMaxAcc + $delayMax;

		    }
		}

		if ($count > 0)
		{
		    if ($outpformat eq "verbose")
		    {
			print ("Averages: nodecnt = $nodecount , period = $period , separation = $separation , numrep = $numrep");
			print (" , count = $count :");
			print (" seqnoCnt = ");    printf("%.2f", $seqnoCntAcc / $count);
			print (" , seqnoMean = "); printf("%.2f", $seqnoMeanAcc / $count);
			print (" , seqnoMin = ");  printf("%.2f", $seqnoMinAcc / $count);
			print (" , seqnoMax = ");  printf("%.2f", $seqnoMaxAcc / $count);

			print (" , delayCnt = ");  printf("%.2f", $delayCntAcc / $count);
			print (" , delayMean = "); printf("%.2f", $delayMeanAcc / $count);
			print (" , delayMin = ");  printf("%.2f", $delayMinAcc / $count);
			print (" , delayMax = ");  printf("%.2f", $delayMaxAcc / $count);
		    }
		    if ($outpformat eq "csv")
		    {
			print ("$nodecount,$period,$separation,$numrep");
			print (",$count");
			print (","); printf("%.2f", $seqnoCntAcc / $count);
			print (","); printf("%.2f", $seqnoMeanAcc / $count);
			print (","); printf("%.2f", $seqnoMinAcc / $count);
			print (","); printf("%.2f", $seqnoMaxAcc / $count);

			print (","); printf("%.2f", $delayCntAcc / $count);
			print (","); printf("%.2f", $delayMeanAcc / $count);
			print (","); printf("%.2f", $delayMinAcc / $count);
			print (","); printf("%.2f", $delayMaxAcc / $count);			
		    }
		    print ("\n");
		}
	    }
	}
    }
}
