#!/bin/sh

echo "set datafile separator \",\"
set ylabel \"latency (us)\"
set xlabel \"transactions per second\"
set title \"Throughput vs Latency for different data sizes\"
set grid" > /tmp/bigplot.gp.$$
first=1
echo -n "plot " >> /tmp/bigplot.gp.$$
for point in `ls -1 | awk -F_ '{print $(NF)}' | awk -F\. '{print $1}' | sort -n`
do
    if [ $first -eq 1 ]
	then
	first=0
    else
	echo ",\\" >> /tmp/bigplot.gp.$$
    fi
    echo -n "\"rate_vs_latency_${point}.csv\" every ::3::53 using 18:19 title \"data=$point\"" >> /tmp/bigplot.gp.$$
done