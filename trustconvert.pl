#!/usr/bin/perl -w

use DBI;
use Net::DNS;

# Konfiguration

sub readconfig {
  # Par 0: filename
  unless (open(IN, '< '.$_[0])) { return 0; }
  my $bla;
  while ($bla=<IN>) {
    if ($bla =~ /^([a-zA-Z0-9]+)=([^\r\n]*)/) {
      if ($1 eq 'sqlport') { $DB_PORT=$2; }
      if ($1 eq 'sqlhost') { $DB_HOST=$2; }
      if ($1 eq 'sqluser') { $DB_USER=$2; }
      if ($1 eq 'sqlpass') { $DB_PASS=$2; }
      if ($1 eq 'sqldb') { $DB_NAME=$2; }
    }
  }
  close(IN);
  return 1;
}

# ------------------

unless (@ARGV>=1) {
  print("\nSyntax: $0 configfilename [-purgedead]\n");
  print("where configfilename is the operservice configfile (the script will take\n");
  print("the database-settings/passwords from there)\n");
  print("if you specify -purgedead, then all 'dead', a.k.a. unresolvable entries\n");
  print("will be killed.\n");
  exit;
}
unless (readconfig($ARGV[0])) {
  print("\nCould not read config file $ARGV[0]\n");
  exit;
}
$purgedead=0;
if ($ARGV[1] eq '-purgedead') { $purgedead=1; }
# Open database
unless ($dbh=DBI->connect("dbi:mysql:$DB_NAME:$DB_HOST:$DB_PORT",$DB_USER,$DB_PASS)) {
  print("\nConnection to database failed.\n");
  exit;
}
unless ($sth = $dbh->prepare('select hostname from trustedhosts')) {
  print("\nCould not read trusted hosts from database(1).\n");
  exit;
}
unless ($sth->execute()) {
  print("\nCould not read trusted hosts from database(2).\n");
  exit;
}
@allips=(); @purgehosts=();
$myres=Net::DNS::Resolver->new;
$myres->tcp_timeout(5);
$myres->udp_timeout(5);
while (($chostname)=$sth->fetchrow_array()) {
  if ($chostname =~ /^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$/ ) { next; }
  unless ($IP=$myres->query($chostname,'A')) {
    print("$chostname could not be resolved\n");
    push(@purgehosts, $chostname);
  } else {
    @answer = $IP->answer;
    for ($j=0;$j<@answer;$j++) {
      if ($answer[$j]->type eq 'A') {
        $niceIP=$answer[$j]->address;
        if ($revdns=$myres->query($niceIP,'PTR')) {
          @revan=$revdns->answer; $foundit=-1;
          for ($k=0;$k<@revan;$k++) {
            if ($revan[$k]->type eq 'PTR') { $foundit=$k; }
          }
          if ($foundit>=0) {
            if (lc($revan[$foundit]->ptrdname) eq lc($chostname)) {
              print("$chostname (".($j+1)."/".int(@answer).") <=> $niceIP\n");
              push(@allips, [ $chostname, $niceIP ] );
            } else {
              print("$chostname (".($j+1)."/".int(@answer).") => $niceIP but reverse (".
                $revan[$foundit]->ptrdname.") does not match.\n");
              push(@purgehosts, $chostname);
            }
          } else {
            print("$chostname (".($j+1)."/".int(@answer).") => $niceIP but reverse has no PTR.\n");
            push(@purgehosts, $chostname);
          }
        } else {
          print("$chostname (".($j+1)."/".int(@answer).") => $niceIP but reverse failed.\n");
          push(@purgehosts, $chostname);
        }
      } else {
        print("$chostname (".($j+1)."/".int(@answer).") did not resolve to an IPv4 IP\n");
        push(@purgehosts, $chostname);
      }
    }
#    if (substr($chostname,0,1) eq 'b') { last; }
  }
}
$sth->finish();
print("Now updating database...\n");
foreach $bla (@allips) {
  $alrin=$dbh->selectrow_array("select count(*) from trustedhosts where hostname=".
               $dbh->quote($bla->[1]));
  if ($alrin==1) {
    print("Host $bla->[1] (converted from $bla->[0]) is already in the DB - deleting $bla->[0]");
    unless($dbh->do('delete from trustedhosts where hostname='.$dbh->quote($bla->[0]))) {
      print(" - FAILED\n");
    } else {
      print(" - Done.\n");
    }
  } else {
    unless($dbh->do('update trustedhosts set hostname='.
        $dbh->quote($bla->[1]).' where hostname='.
        $dbh->quote($bla->[0]))) {
          print("Could not update databaseentry for $bla->[0] to $bla->[1]\n");
    }
  }
}
if ($purgedead==1) {
  print("Deleting unresolvable hosts...\n");
  foreach $bla (@purgehosts) {
    unless ($dbh->do('delete from trustedhosts where hostname='.$dbh->quote($bla))) {
      print("Error deleting $bla\n");
    }
  }
}
$dbh->disconnect(); 
print("All done\n");
# EOF
