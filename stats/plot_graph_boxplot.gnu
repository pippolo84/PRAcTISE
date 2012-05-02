set terminal svg size 800,600 dynamic	fname 'Arial'	fsize 12 butt solid
set output 'graph_box.svg'
set style boxplot nooutliers
set style fill empty noborder
set style data boxplot
set boxwidth 0.5
set pointsize 0.5 
unset key
unset label
set title 'KERNEL - set on push global structure'
set xlabel 'CPUs number'
set ylabel 'cycles'
set border 2
set xtics ("2" 1, "4" 2, "8" 3, "16" 4, "24" 5, "32" 6, "40" 7, "48" 8) scale 0.0
set xtics nomirror
set ytics nomirror
set yrange [0:5000]

plot 'results' using (1):1 , '' using(1):2, '' using (2):3, '' using (2):4, '' using (3):5, '' using (3):6, '' using (4):7, '' using (4):8, '' using (5):9, '' using (5):10, '' using (6):11, '' using(6):12, '' using (7):13, '' using (7):14, '' using (8):15, '' using (8):16
