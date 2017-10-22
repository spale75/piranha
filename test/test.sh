#!/bin/sh -e

for proto in ipv4 ipv6
do
	for mode in H m j
	do
		printf "Testing ${proto} in mode ${mode}: "
		../bin/ptoa -${mode} ${proto}.input > test.out
		diff test.out ${proto}.output.${mode} > /dev/null && \
		{
			printf "OK\n";
		} || { \
			printf "ERROR\n";
			printf "NOTE: ouput of ../bin/ptoa -${mode} ${proto}.input differs from ${proto}.output.${mode}\n";
			diff test.out ${proto}.output.${mode} | sed "s/^/DIFF: /";
			printf "\n";
		}
		rm -f test.out
	done
done
