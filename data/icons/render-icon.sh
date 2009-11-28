#!/bin/bash

if [ -z "$1" ]; then
	echo "Usage: $0 <source.svg>"
	exit 1
fi

source="${1%.svg}"

for s in 16 22 24 32 36 48 64; do
	cmd="inkscape -e $source.$s.png -w $s -h $s $source.svg"
	echo $cmd
	$cmd
done
