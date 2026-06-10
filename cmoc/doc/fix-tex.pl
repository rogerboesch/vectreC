#!/usr/bin/perl -w

use strict;

my $previousLine;
my $eraseNextNonEmptyLine;
my $titleAccumulator;
my $captureAuthor;

while (<>)
{
    if (index($_, '\begin{document}') != -1)
    {
        $_ = "";
        $eraseNextNonEmptyLine = 1;
    }
    elsif (defined $eraseNextNonEmptyLine && /\S/)
    {
        $_ = "";
        $eraseNextNonEmptyLine = undef;
    }
    elsif (/^\\section\{(.*?)$/s)  # if 1st line of lone \section{} command
    {
        $titleAccumulator = $1;
        $_ = "";
    }
    elsif (defined $titleAccumulator)
    {
        die unless /^(.*?)\}/;
        $titleAccumulator .= " $1";
        $_ = "\\title{$titleAccumulator}\n";
        $titleAccumulator = undef;
        $captureAuthor = 1;
    }
    elsif (defined $captureAuthor)
    {
        print STDERR "# captureAuthor: $_";
        if (/\{By (.*?)\}/)
        {
            print STDERR "#   GOT: [$1]\n";
            $_ = "\\author\{$1\}\n";
            $captureAuthor = undef;
        }
    }
    elsif (/Date of this manual: (\S+)/)
    {
        $_ = "\\date{$1}\n\\begin{document}\n\\maketitle\n";
    }
    elsif (index($_, '\subsection{Introduction}\label{introduction}') != -1)
    {
        print '\tableofcontents', "\n";
    }
    print;

    $previousLine = $_;
}