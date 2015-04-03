#!/bin/sh

[ $# -lt 1 ] && exit 1

nb_instances=$1
shift

./maze -f -s "$@" 2>&1 > /dev/null &

for i in $(seq $nb_instances)
do
  ./maze -f 127.0.0.1 $3 2>&1 > /dev/null &
done

./maze 127.0.0.1 $3

read c

killall -9 maze
