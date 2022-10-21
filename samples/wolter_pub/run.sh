if [ $# -ne 2 ]
then
	echo "Usage: $0 <id min> <id max>"
	exit 1
fi
let PUZZLE_ID=$1
while [ $PUZZLE_ID -le $2 ]
do
	FILENAME=./webpbn_$PUZZLE_ID.txt
	../../src/webpbn_conv.sh $PUZZLE_ID >${FILENAME}
	if [ $? -eq 0 ]
	then
		echo "Solving ${FILENAME}..."
		time timeout 120 ../../bin/nonogram 0 2 <${FILENAME}
	fi
	rm -f ${FILENAME}
	let PUZZLE_ID=$PUZZLE_ID+1
done
