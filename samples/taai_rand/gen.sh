if [ $# -ne 3 ]
then
	echo "Usage: %0 <prefix> <width> <height>"
	exit 1
fi
sed s/"\\$"/$1/g | sed s/"	"/","/g | (
	read PUZZLEID
	while [ "${PUZZLEID}" ]
	do
		FILENAME=nonogram_$PUZZLEID.txt
		echo $2 >$FILENAME
		echo $3 >>$FILENAME
		read CLUE
		echo -n "\"${CLUE}\"" >>$FILENAME
		let I=1
		while [ $I -lt $2 ]
		do
			read CLUE
			echo -n ",\"${CLUE}\"" >>$FILENAME
			let I=$I+1
		done
		echo >>$FILENAME
		read CLUE
		echo -n "\"${CLUE}\"" >>$FILENAME
		let I=1
		while [ $I -lt $3 ]
		do
			read CLUE
			echo -n ",\"${CLUE}\"" >>$FILENAME
			let I=$I+1
		done
		echo >>$FILENAME
		read PUZZLEID
	done
)
