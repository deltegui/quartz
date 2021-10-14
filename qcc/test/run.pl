#!/usr/bin/perl
use strict;
use warnings;
use Term::ANSIColor;

sub file_is_valid {
    return (($_[0] ne "") and ($_[0] ne ".") and ($_[0] ne ".."));
}

sub read_files {
    opendir my $dir, $_[0] or die "Cannot open $_[0] dir $!";
    my @files = readdir $dir;
    closedir $dir;
    return @files;
}

sub read_files_as_hash {
    my %hash;
    my $base_path = $_[0];
    foreach(read_files($base_path)) {
        if (file_is_valid($_)) {
            my $full_route = "$base_path/$_";
            if (-d $full_route) {
                my %raw = read_files_as_hash($full_route);
                my %final;
                foreach my $key (keys %raw) {
                    $final{"$_/$key"} = $raw{$key};
                }
                %hash = (%hash, %final);
            } else {
                $hash{$_} = $full_route;
            }
        }
    }
    return %hash;
}

sub read_text {
	open my $fh, '<', $_[0] or die "Can't open file $!";
	my $file_content = do { local $/; <$fh> };
	return $file_content
}

sub clean_entry {
	$_[0] =~ s/^\s+|\s+$//g;
	return $_[0];
}

my $prog_dir = "../programs";
my $test_dir = "./cases";
my $clox_bin = "../quartz";

my %prog = read_files_as_hash($prog_dir);
my %tests = read_files_as_hash($test_dir);
if($ARGV[0]) {
	print "\n======================\n";
	print "Loaded tests:\n";
	print map { "$_ => $tests{$_}\n" } keys %tests;
	print "\n";
	print "Loaded programs:\n";
	print map { "$_ => $prog{$_}\n" } keys %prog;
	print "======================\n\n";
}

my $have_err = 0;
my $test_number = 0;

while(my ($key, $value) = each(%tests)) {
	if($prog{$key}) {
		print("RUNNING TEST: $key... ");
		my $result = `$clox_bin $prog{$key} 2>&1`;
		if(!$result) {
			$result = '';
		}
		$result = clean_entry($result);
		my $expected = clean_entry(read_text($value));
		if($expected eq $result) {
			print color('bold green');
			print "OK\n";
			print color('reset');
		} else {
			print color('bold red');
			print "FAIL\n";
			print "EXPECTED:\n$expected\nBUT HAVE:\n$result\n";
			print color('reset');
			$have_err = 1;
		}
		$test_number++;
	}
}

print("\nExecuted $test_number tests\n");

exit($have_err);
