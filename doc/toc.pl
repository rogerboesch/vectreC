#!/usr/bin/perl -w

use strict;
use Getopt::Long;
use File::Basename;


my $program = basename($0);
my $version = "0.0.0";
my $numErrors = 0;
my $numWarnings = 0;
#my $dryRun = 0;
#my $verbose = 0;


sub usage
{
    my ($exitStatus) = @_;

    print <<__EOF__;
$program $version - Adds a table of contents to a Markdown-generated HTML file.
Copyright (C) 2015 Pierre Sarrazin <http://sarrazip.com/>
This script is distributed under the GNU General Public License version 3 or later.

Usage: $program [OPTIONS]

Options:
--help          Print this help page.
--version       Print this program's name and version number.

__EOF__

    exit($exitStatus);
}


sub errmsg
{
    print "$program: ERROR: ";
    printf @_;
    print "\n";

    ++$numErrors;
}


sub warnmsg
{
    print "$program: Warning: ";
    printf @_;
    print "\n";

    ++$numWarnings;
}


my $tagCounter = 0;
my @headings;  # list of hashes


# May increment $tagCounter and append objects to @headings.
# Returns an HTML string.
#
sub fixHeading($$)
{
    my ($headingLevel, $title) = @_;

    my $anchor = "";
    if ($headingLevel >= 2)
    {
        my $tag = ++$tagCounter;
        push @headings, { level => $headingLevel, title => $title, tag => $tag };
        $anchor = "<a name='t$tag'></a>";
    }

    # Put a space in the <Hx> tag to mark it as modified.
    #
    return "<H$headingLevel >$anchor$title</H$headingLevel>";
}


sub work
{
    # Read the whole HTML file in memory.
    #
    my $temp = $/;
    $/ = undef;
    my $html = <>;
    $/ = $temp;

    # Insert <A NAME="xxx"></A> in all <Hn>...</Hn>, for n >= 2.
    #
    while ($html =~ s!<H(\d+)>(.*?)</H\d+>!fixHeading($1, $2)!ieg)
    {
    }

    my $htmlTOC = "";
    my $fh;
    if (!open($fh, '>', \$htmlTOC))
    {
        errmsg("failed to open a file to write into a string: $!");
        return;
    }
    print $fh "<H2>Table of Contents</H2>\n";
    print $fh "<small>\n";
    print $fh "<ul>\n";
    my $curLevel = 2;
    for my $h (@headings)
    {
        if ($h->{level} > $curLevel)
        {
            print $fh "<ul>\n";  #" <!-- ", $h->{level}, " $curLevel -->\n";
            $curLevel = $h->{level};
        }
        else
        {
            while ($h->{level} < $curLevel)
            {
                print $fh "</ul>\n"; #<!-- ", $h->{level}, " $curLevel -->\n";
                $curLevel--;
            }
        }

        printf $fh "<li><a href='#t%s'>%s</a></li> <!--%d-->\n", $h->{tag}, $h->{title}, $h->{level};
    }
    print $fh "</ul>\n";
    print $fh "</small>\n";

    # Put the TOC before the first modified <H2> tag, i.e., the first one
    # with a space in the <H2> tag.
    #
    $html =~ s/(<H2 >)/$htmlTOC$1/;

    print $html;
}


###############################################################################


my $showUsage = 0;

if (!GetOptions(
    "help" => \$showUsage,
    "version" => \$showUsage,
    #"not-really" => \$dryRun,
    #"verbose" => \$verbose,
    ))
{
    exit 1;
}

usage(0) if $showUsage;

$| = 1;  # no buffering on STDOUT

work();

#print "$program: $numErrors error(s), $numWarnings warnings(s).\n";
exit($numErrors > 0);
