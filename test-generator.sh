#!/bin/sh

# Warning - do not use test-generator in pipeline - it is very slow!
# If you want to measure performance of multispoof component(s)
# redirect test-generator output to file, then use cat and redirect
# its output to tested component.

if [ "$2" == "" ]
then
	echo "Usage:"
	echo -e "\ttest-generator packet_file iteration_count"
	exit 1
fi

COUNT=0;
PACKET_FILE=$1
ITERATION_COUNT=$2

while [ "$COUNT" -lt "$ITERATION_COUNT" ]
do
	cat "$PACKET_FILE"
	COUNT=$[ $COUNT + 1 ]
done
