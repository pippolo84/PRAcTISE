set terminal svg size 800,600 dynamic  fname 'Arial'  fsize 12 butt solid
set output 'graph_avg.svg'
set style data linespoints
set size 1.0, 1.0
set origin 0.0, 0.0
unset key
set grid
unset label
set title 'Heap - dequeue_cycle'
set xrange [0:50]
set autoscale y
set xlabel 'CPUs number'
set ylabel 'Cycles'
set xtics border in scale 1,0.5 mirror norotate  offset character 0, 0, 0
set xtics autofreq  norangelimit
set ytics border in scale 1,0.5 mirror norotate  offset character 0, 0, 0
set ytics autofreq norangelimit
plot './dequeue_cycle_avg'
