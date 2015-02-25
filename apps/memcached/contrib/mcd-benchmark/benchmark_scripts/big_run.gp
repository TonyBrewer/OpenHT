set datafile separator ","
set ylabel "latency (us)"
set xlabel "transactions per second"
set title "Throughput vs Latency for different data sizes"
set grid
plot "rate_vs_latency_2048.csv" every ::3 using 18:19 title "data=2048", "rate_vs_latency_4096.csv" every ::3 using 18:19 title "data=4096", "rate_vs_latency_6144.csv" every ::3 using 18:19 title "data=6144", "rate_vs_latency_8192.csv" every ::3 using 18:19 title "data=8192", "rate_vs_latency_10240.csv" every ::3 using 18:19 title "data=10240", "rate_vs_latency_12288.csv" every ::3 using 18:19 title "data=12288"
