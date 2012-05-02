#! /bin/bash

# Uscita in caso di errore
set -e

sum=0
while read x
do
	sum=`expr $sum + $x`
done
echo $sum
