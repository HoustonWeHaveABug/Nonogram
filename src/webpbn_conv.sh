if [ $# -ne 1 ]
then
	echo "Usage: $0 <puzzle number>"
	exit 1
fi
WGET_LOG=./webpbn_conv_wget.log
WGET_OUTPUT=./webpbn_conv_wget.txt
wget -o $WGET_LOG -O $WGET_OUTPUT https://webpbn.com/export.cgi --inet4-only --no-check-certificate --post-data "id=$1&fmt=olsak&go=1"
if [ ! -s $WGET_OUTPUT ]
then
	echo "Could not download puzzle $1"
	rm -f $WGET_LOG $WGET_OUTPUT
	exit 1
fi
grep -q ": rows" $WGET_OUTPUT
if [ $? -ne 0 ]
then
	cat $WGET_OUTPUT
	rm -f $WGET_LOG $WGET_OUTPUT
	exit 1
fi
grep -q ": columns" $WGET_OUTPUT
if [ $? -ne 0 ]
then
	cat $WGET_OUTPUT
	rm -f $WGET_LOG $WGET_OUTPUT
	exit 1
fi
echo ": eof" >>$WGET_OUTPUT
cat $WGET_OUTPUT | (
	read LINE
	while [ "$LINE" != ": rows" ]
	do
		read LINE
	done
	PUZZLE_ROWS_TMP=./webpbn_$1_rows.tmp
	>$PUZZLE_ROWS_TMP
	let ROWS=0
	read LINE
	while [ "$LINE" != ": columns" ]
	do
		if [ "$LINE" != "" ]
		then
			echo -n $LINE | sed s/^/\"/g | sed s/$/\"/g | sed "s/\([0-9][0-9]*\)/\\1-/g" | sed s/" "/,/g >>$PUZZLE_ROWS_TMP
		else
			echo -n \"0\" >>$PUZZLE_ROWS_TMP
		fi
		let ROWS=$ROWS+1
		read LINE
		if [ "$LINE" != ": columns" ]
		then
			echo -n "," >>$PUZZLE_ROWS_TMP
		fi
	done
	echo >>$PUZZLE_ROWS_TMP
	PUZZLE_COLS_TMP=./webpbn_$1_cols.tmp
	>$PUZZLE_COLS_TMP
	let COLS=0
	read LINE
	while [ "$LINE" != ": eof" ]
	do
		if [ "$LINE" != "" ]
		then
			echo -n $LINE | sed s/^/\"/g | sed s/$/\"/g | sed "s/\([0-9][0-9]*\)/\\1-/g" | sed s/" "/,/g >>$PUZZLE_COLS_TMP
		else
			echo -n \"0\" >>$PUZZLE_COLS_TMP
		fi
		let COLS=$COLS+1
		read LINE
		if [ "$LINE" != ": eof" ]
		then
			echo -n "," >>$PUZZLE_COLS_TMP
		fi
	done
	echo >>$PUZZLE_COLS_TMP
	echo $COLS
	echo $ROWS
	cat $PUZZLE_COLS_TMP
	cat $PUZZLE_ROWS_TMP
	rm -f $PUZZLE_ROWS_TMP $PUZZLE_COLS_TMP
)
rm -f $WGET_LOG $WGET_OUTPUT
exit 0
