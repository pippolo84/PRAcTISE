#! /bin/bash
# Author: Fabio Falzoi
# Semplice script per il calcolo della media di N numeri
# è possibile specificare come primo parametro il numero di decimali (saranno settati con l'opzione scale)
# il ciclo di input termina quando si legge EOF (CTRL+D da terminale)

# Uscita in caso di errore
set -e

# Numero di decimali da riga di comando (0 per default)
scale_value=0
if [ -n "$1" ] 
then
	scale_value=$1
fi

n=0
sum=0
while read x
do
	sum=`expr $sum + $x`
	n=`expr $n + 1`
done
avg=`echo "scale=$scale_value;$sum/$n" | bc`
echo $avg
