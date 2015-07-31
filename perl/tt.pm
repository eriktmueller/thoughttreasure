#
# ThoughtTreasure commonsense platform
# Perl-based client API
#
# Copyright 1999, 2015 Erik Thomas Mueller. All Rights Reserved.
# 

package tt;
use Socket;
use FileHandle;

$HOST="myhostname";
$PORT=1832;

$F_PRESENT_PARTICIPLE = "e";
$F_INFINITIVE         = "f";
$F_FRENCH             = "y";
$F_ENGLISH            = "z";

%isaCache = ();

sub ttopenconn {
  return ttopen($HOST, $PORT);
}

sub ttopen {
  my ($remote, $port) = @_;
  my ($iaddr, $paddr, $proto, $h);
  $iaddr = inet_aton($remote);
  $paddr = sockaddr_in($port, $iaddr);
  $proto = getprotobyname('tcp');
  $h = new FileHandle;
  if (!socket($h, PF_INET, SOCK_STREAM, $proto)) {
    return 0;
  }
  setsockopt($h, SOL_SOCKET, SO_REUSEADDR, 1);
  if (!connect($h, $paddr)) {
    return 0;
  }
  return $h;
}

sub brToSr {
  my $br = $_[0];
  if ($br == 0) {
    return "n";
  }
  return "y";
}

sub srToBr {
  my $sr = $_[0];
  if ($sr eq "y") {
    return 1;
  }
  return 0;
}

sub isa {
  my ($h, $classname, $objname) = @_;
  $classname = whitespaceTrim($classname);
  $objname = whitespaceTrim($objname);
  my $sr = $isaCache{"$classname=$objname"};
  if ($sr) {
    return srToBr($sr);
  }
  my $br = queryBoolean($h, "ISA $classname $objname");
  $isaCache{"$classname=$objname"} = brToSr($br);
  return $br;
}

sub retrieve {
  my ($h, $picki, $anci, $desci, $mode, $ptn) = @_;
  $mode = whitespaceTrim($mode);
  $ptn = whitespaceTrim($ptn);
  return queryAssertionLines($h, "Retrieve $picki $anci $desci $mode $ptn");
}

sub tag {
  my ($h, $feat, $text) = @_;
  $text = whitespaceTrim($text);
  $feat = whitespaceTrim($feat);
  return queryPNodes($h, "Tag $feat $text ");
}

sub ttclose {
  my $h = $_[0];
  close($h);
}

# Internal functions

sub printList {
  foreach $s (@_) {
    print "$s\n";
  }
  return "";
}

# removes leading and trailing white space
sub whitespaceTrim {
  my $s = $_[0];
  $s =~ s/^\s*(.*?)\s*$/$1/;
  return $s;
}

sub mapNewlineToSpace {
  my $s = $_[0];
  $s =~ s/\n/ /g;
  return $s;
}

sub mapNewlineToSemicolon {
  my $s = $_[0];
  $s =~ s/\n/;/g;
  return $s;
}

sub elimNewlines {
  my $s = $_[0];
  $s =~ s/\n//g;
  return $s;
}

sub elimReturns {
  my $s = $_[0];
  $s =~ s/\r//g;
  return $s;
}

sub elimSemicolons {
  my $s = $_[0];
  $s =~ s/;//g;
  return $s;
}

sub queryAssertionLines {
  my ($h, $cmd) = @_;
  if (!$h) {
    print "ERROR: null handle\n";
    return ();
  }
  print $h "$cmd\n";
  flush $h;
  return rdlines($h);
}

sub queryLines {
  my ($h, $cmd) = @_;
  if (!$h) {
    print "ERROR: null handle\n";
    return ();
  }
  print $h "$cmd\n";
  flush $h;
  return rdlines($h);
}

sub queryPNodes {
  my ($h, $cmd) = @_;
  if (!$h) {
    print "ERROR: null handle\n";
    return ();
  }
  print $h "$cmd\n";
  flush $h;
  @lines = rdlines($h);
  return tokenizeMany(":",@lines);
}

sub queryBoolean {
  my ($h, $cmd) = @_;
  if (!$h) {
    print "ERROR: null handle\n";
    return "0";
  }
  print $h "$cmd\n";
  flush $h;
  my $line = rdline($h);
  if (substr($line, 0, length("error:")) eq "error:") {
    print $line."\n";
    return 0;
  }
  return $line;
}

sub tokenize {
  my ($s, $delim) = @_;
  return split(/$delim/, $s);
}

sub tokenizeMany {
  my ($delim, @lines) = @_;
  my @r;
  foreach $s (@lines) {
    @r = (@r, tokenize($s, $delim));
  }
  return @r;
}

sub rdline {
  my $h = $_[0];
  $line = <$h>;
#  print "WAITING FOR LINE...\n";
  $line = elimNewlines($line);
#  print "RECEIVED = <$line>\n";
  return $line;
}

sub rdlines {
  my $h = $_[0];
  my $line = "";
  my @r;
  while (1) {
    $line = rdline($h);
    if (substr($line, 0, length("error:")) eq "error:") {
      print $line."\n";
      return ();
    }
#print "LINE <$line>\n";
    if ($line eq ".") {
      last;
    }
    @r = (@r,$line);
  }
  return @r;
}

sub featIn {
  my ($f, $features) = @_;
  return -1 != index $features, $f
}

sub numberTodGet {
  my $s = $_[0];
  my @x = tokenize($s, ":");
  if ($x[0] eq "NUMBER" &&
      $x[1] eq "tod") {
    return $x[2];
  }
  return 0;
}

# tt::numberSecondGet("NUMBER:second:1800") -> 1800
sub numberSecondGet {
  my $s = $_[0];
  my @x = tokenize($s, ":");
  if ($x[0] eq "NUMBER" &&
      $x[1] eq "second") {
    return $x[2];
  }
  return 0;
}

return 1;

# End of file.
