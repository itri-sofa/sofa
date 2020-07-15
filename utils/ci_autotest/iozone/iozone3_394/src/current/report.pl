#!/usr/bin/perl
##
## Copyright (c) 2015-2020 Industrial Technology Research Institute.
##
## Licensed to the Apache Software Foundation (ASF) under one
## or more contributor license agreements.  See the NOTICE file
## distributed with this work for additional information
## regarding copyright ownership.  The ASF licenses this file
## to you under the Apache License, Version 2.0 (the
## "License"); you may not use this file except in compliance
## with the License.  You may obtain a copy of the License at
##
##    http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing,
## software distributed under the License is distributed on an
## "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
## KIND, either express or implied.  See the License for the
## specific language governing permissions and limitations
## under the License.
##
##
##
##
##
##
##  
##


@Reports=@ARGV;

die "usage: $0 <iozone.out> [<iozone2.out>...]\n" if not @Reports or grep (m|^-|, @Reports);

die "report files must be in current directory" if grep (m|/|, @Reports);

%columns=(
         'write'     =>3,
         'read'      =>5,
         'rewrite'   =>4,
         'reread'    =>6,
         'randread'  =>7,
         'randwrite' =>8,
         'bkwdread'  =>9,
         'recrewrite'=>10,
         'strideread'=>11,
         'fwrite'    =>12,
         'frewrite'  =>13,
         'fread'     =>14,
         'freread'   =>15,
         );

#
# create output directory. the name is the concatenation
# of all report file names (minus the file extension, plus
# prefix report_)
#
$outdir="report_".join("_",map{/([^\.]+)(\..*)?/ && $1}(@Reports));

print STDERR "Output directory: $outdir ";

if ( -d $outdir ) 
{
    print STDERR "(removing old directory) "; 
    system "rm -rf $outdir";
}

mkdir $outdir or die "cannot make directory $outdir";

print STDERR "done.\nPreparing data files...";

foreach $report (@Reports)
{
    open(I, $report) or die "cannot open $report for reading";
    $report=~/^([^\.]+)/;
    $datafile="$1.dat";
    push @datafiles, $datafile;
    open(O, ">$outdir/$datafile") or die "cannot open $outdir/$datafile for writing";
    open(O2, ">$outdir/2d-$datafile") or die "cannot open $outdir/$datafile for writing";
    while(<I>)
    {
        next unless ( /^[\s\d]+$/ );
        @split = split();
        next unless ( @split == 15 );
        print O;
        print O2 if $split[1] == 16384 or $split[0] == $split[1];
    }
    close I, O, O2;
}

print STDERR "done.\nGenerating graphs:";

foreach $column (keys %columns)
{
    print STDERR " $column";
    
    open(G, ">$outdir/$column.do") or die "cannot open $outdir/$column.do for writing";
    print G qq{
set title "Iozone performance: $column"
set grid lt 2 lw 1
set surface
set parametric
set xtics
set ytics
set logscale x 2
set logscale y 2
set autoscale z
set xrange [2.**5:2.**24]
set xlabel "File size in KBytes"
set ylabel "Record size in Kbytes"
set zlabel "Kbytes/sec"
set data style lines
set dgrid3d 80,80,3
#set terminal png small picsize 900 700
set terminal png small size 900 700
set output "$column.png"
};

    print G "splot ". join(", ", map{qq{"$_" using 1:2:$columns{$column} title "$_"}}(@datafiles));

    print G "\n";

    close G;

    open(G, ">$outdir/2d-$column.do") or die "cannot open $outdir/$column.do for writing";
    print G qq{
set title "Iozone performance: $column"
#set terminal png small picsize 450 350
set terminal png small size 450 350
set logscale x
set xlabel "File size in KBytes"
set ylabel "Kbytes/sec"
set output "2d-$column.png"
};

    print G "plot ". join(", ", map{qq{"2d-$_" using 1:$columns{$column} title "$_" with lines}}(@datafiles));

    print G "\n";

    close G;

    if ( system("cd $outdir && gnuplot $column.do && gnuplot 2d-$column.do") )
    {
        print STDERR "(failed) ";
    }
    else
    {
        print STDERR "(ok) ";
    }
}

print STDERR "done.\n";

