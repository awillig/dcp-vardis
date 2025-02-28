#!/usr/bin/perl

if (($#ARGV+1) ne 5)
{
    print "Five parameters expected: ifname outputfilename stnidshort bpperiodms vdperiodms.\nExiting.\n";
    exit;
}


$ifname        = $ARGV[0]; shift;
$outputfile    = $ARGV[0]; shift;
$srpaverage    = $ARGV[0]; shift;
$srpaverage    = 1000 * $srpaverage;
$bpperiodms    = $ARGV[0]; shift;
$vdperiodms    = $ARGV[0]; shift;

$bpcommandsocketfile     = "/tmp/dcp-bp-command-socket";
$bpcommandsockettimeout  = 400;

$console_logging = 0;

# ===============================================================
# Create required directories if needed
# ===============================================================

mkdir ("cfg")         unless (-d "cfg");
mkdir ("cfg/bp")      unless (-d "cfg/bp");
mkdir ("cfg/vardis")  unless (-d "cfg/vardis");
mkdir ("cfg/srp")     unless (-d "cfg/srp");

# ===============================================================
# BP configuration
# ===============================================================

my $bpcfg = <<END;

[logging]
loggingToConsole     =  $console_logging 
filenamePrefix       =  log-dcp-bp
severityLevel        =  trace

[BPCommandSocket]
commandSocketFile       =  $bpcommandsocketfile
commandSocketTimeoutMS  =  $bpcommandsockettimeout

[BP]
interface_name       =  $ifname
interface_mtuSize    =  1386
interface_etherType  =  24687
maxBeaconSize        =  880
avgBeaconPeriodMS    =  $bpperiodms
jitterFactor         =  0.1

END


open (FH, '>', "cfg/bp/$outputfile") or die $!;
print FH $bpcfg;
close (FH);


# ===============================================================
# Vardis configuration
# ===============================================================

my $vardiscfg = <<END;

[BPCommandSocket]
commandSocketFile       =  $bpcommandsocketfile
commandSocketTimeoutMS  =  $bpcommandsockettimeout

[BPSharedMem]
areaName  =   shm-area-bpclient-vardis

[logging]
loggingToConsole     =  $console_logging
filenamePrefix       =  log-dcp-vardis
severityLevel        =  trace

[Vardis]
maxValueLength                 =   64
maxDescriptionLength           =   64
maxRepetitions                 =   2
maxPayloadSize                 =   600
maxSummaries                   =   20
payloadGenerationIntervalMS    =   $vdperiodms
pollRTDBServiceIntervalMS      =   50
queueMaxEntries                =   2

[VardisCommandSocket]
commandSocketFile        =  /tmp/dcp-vardis-command-socket
commandSocketTimeoutMS   =  $bpcommandsockettimeout


END

open (FH, '>', "cfg/vardis/$outputfile") or die $!;
print FH $vardiscfg;
close (FH);

# ===============================================================
# SRP configuration
# ===============================================================

my $srpcfg = <<END;

[BPCommandSocket]
commandSocketFile       =  $bpcommandsocketfile
commandSocketTimeoutMS  =  $bpcommandsockettimeout

[BPSharedMem]
areaName  =   shm-area-bpclient-vardis

[SRP]
averageX            =  $srpaverage
averageY            =  $srpaverage
averageZ            =  $srpaverage
stdDev              =  25
generationPeriodMS  =  200

END

open (FH, '>', "cfg/srp/$outputfile") or die $!;
print FH $srpcfg;
close (FH);

