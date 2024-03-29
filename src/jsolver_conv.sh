if [ $# -ne 1 ]
then
	echo "Usage: $0 <puzzle>"
	exit 1
fi
cat $1 | (
	PUZZLE_TMP=./`basename $1`.tmp
	>$PUZZLE_TMP
	let COLS=0
	read LINE
	while [ "$LINE" != "#" ]
	do
		if [ "$LINE" != " 0" ]
		then
			echo -n $LINE | sed s/" 0$"//g | sed s/^/\"/g | sed s/$/\"/g | sed s/" "/,/g >>$PUZZLE_TMP
		else
			echo -n \"0\" >>$PUZZLE_TMP
		fi
		let COLS=$COLS+1
		read LINE
		if [ "$LINE" != "#" ]
		then
			echo -n "," >>$PUZZLE_TMP
		fi
	done
	echo >>$PUZZLE_TMP
	let ROWS=0
	read LINE
	while [ "$LINE" != "#" ]
	do
		if [ "$LINE" != " 0" ]
		then
			echo -n $LINE | sed s/" 0$"//g | sed s/^/\"/g | sed s/$/\"/g | sed s/" "/,/g >>$PUZZLE_TMP
		else
			echo -n \"0\" >>$PUZZLE_TMP
		fi
		let ROWS=$ROWS+1
		read LINE
		if [ "$LINE" != "#" ]
		then
			echo -n "," >>$PUZZLE_TMP
		fi
	done
	echo >>$PUZZLE_TMP
	echo $COLS
	echo $ROWS
	cat $PUZZLE_TMP
	rm -f $PUZZLE_TMP
)
