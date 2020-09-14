#! /usr/bin/gnuplot
#
# NAME: Cindi Dong
# EMAIL: cindidong@gmail.com
# ID: 405126747
#
# purpose:
#	 generate data reduction graphs for the multi-threaded list project
#
# input: lab2_list.csv
#	1. test name
#	2. # threads
#	3. # iterations per thread
#	4. # lists
#	5. # operations performed (threads x iterations x (ins + lookup + delete))
#	6. run time (ns)
#	7. run time per operation (ns)
#	8. average lock wait time (ns)
#
# output:
#	lab2b_1.png ... throughput vs. number of threads for mutex and spin-lock synchronized list operations
#	lab2b_2.png ... mean time per mutex wait and mean time per operation for mutex-synchronized list operations
#	lab2b_3.png ... successful iterations vs. threads for each synchronization method
#	lab2b_4.png ... throughput vs. number of threads for mutex synchronized partitioned lists
#	lab2b_5.png ... throughput vs. number of threads for spin-lock-synchronized partitioned lists
#
# Note:
#	Managing data is simplified by keeping all of the results in a single
#	file.  But this means that the individual graphing commands have to
#	grep to select only the data they want.
#
#	Early in your implementation, you will not have data for all of the
#	tests, and the later sections may generate errors for missing data.
#

# general plot parameters
set terminal png
set datafile separator ","

# throughput vs. number of threads for mutex and spin-lock synchronized list operations
set title "List-1: Throughput vs. Number of Threads"
set xlabel "Threads"
set logscale x 2
set ylabel "Throughput (ops per sec)"
set logscale y 10
set output 'lab2b_1.png'

# grep out only single threaded, un-protected, non-yield results
plot \
     "< grep -e 'list-none-m,[[:digit:]]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'mutex' with linespoints lc rgb 'red', \
     "< grep -e 'list-none-s,[[:digit:]]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'spin-lock' with linespoints lc rgb 'green'



# mean time per mutex wait and mean time per operation for mutex-synchronized list operations
set title "List-2: Wait for lock time"
set xlabel "Threads"
set logscale x 2
set ylabel "Time (ns)"
set logscale y 10
set output 'lab2b_2.png'
set key left top

# grep out only single threaded, un-protected, non-yield results
plot \
     "< grep -e 'list-none-m,[[:digit:]]*,1000,1,' lab2b_list.csv" using ($2):($7) \
	title 'mean time per operation' with linespoints lc rgb 'red', \
     "< grep -e 'list-none-m,[[:digit:]]*,1000,1,' lab2b_list.csv" using ($2):($8) \
	title 'wait for lock time' with linespoints lc rgb 'green'



set title "List-3: Unprotected and Protected Threads and Iterations that run without failure"
set xlabel "Threads"
set logscale x 2
set xrange [0.5:32]
set ylabel "Successful Iterations"
set logscale y 10
set output 'lab2b_3.png'
set key right top
plot \
     "< grep -e 'list-id-none' lab2b_list.csv" using ($2):($3) \
	title 'unprotected' with points lc rgb 'green', \
     "< grep 'list-id-s' lab2b_list.csv" using ($2):($3) \
	title 'spin-locked' with points lc rgb 'red', \
     "< grep 'list-id-m' lab2b_list.csv" using ($2):($3) \
	title 'mutex-locked' with points lc rgb 'violet'


set title "List-4: Mutex lock Throughput vs. Number of Threads (for --lists)"
set logscale x 2
unset xrange
set xrange [0.75:]
set xlabel "Threads"
set ylabel "Throughput (ops per sec)"
set logscale y 10
set output 'lab2b_4.png'
set key right top
plot \
    "< grep -e 'list-none-m,[[:digit:]]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title "lists=1" with linespoints lc rgb 'green', \
    "< grep -e 'list-none-m,[[:digit:]]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title "lists=4" with linespoints lc rgb 'red', \
    "< grep -e 'list-none-m,[[:digit:]]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title "lists=8" with linespoints lc rgb 'violet', \
    "< grep -e 'list-none-m,[[:digit:]]*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title "lists=16" with linespoints lc rgb 'blue'

#
# "no valid points" is possible if even a single iteration can't run
#


set title "List-5: Spin lock Throughput vs. Number of Threads (for --lists)"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput (ops per sec)"
set logscale y
set output 'lab2b_5.png'
set key right top
plot \
    "< grep -e 'list-none-s,[[:digit:]]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title "lists=1" with linespoints lc rgb 'green', \
    "< grep -e 'list-none-s,[[:digit:]]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title "lists=4" with linespoints lc rgb 'red', \
    "< grep -e 'list-none-s,[[:digit:]]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title "lists=8" with linespoints lc rgb 'violet', \
    "< grep -e 'list-none-s,[[:digit:]]*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title "lists=16" with linespoints lc rgb 'blue'