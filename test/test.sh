#!/bin/sh -e

for proto in ipv4 ipv6
do
	for mode in H m j
	do
		IN="input_${proto}.bin"
		OUT="test.out"
		REF="output_${proto}_${mode}.bin"

		printf "Testing ${proto} in mode ${mode}: "
		../bin/ptoa -${mode} ${IN} > ${OUT}
		diff ${OUT} ${REF} > /dev/null && \
		{
			printf "OK\n";
		} || { \
			printf "ERROR\n";
			printf "NOTE: ouput of ../bin/ptoa -%s %s differs from %s\n" ${mode} ${IN} ${REF};
			diff ${OUT} ${REF} | sed "s/^/DIFF: /";
			printf "\n";
		}
		rm -f ${OUT}
	done
done
