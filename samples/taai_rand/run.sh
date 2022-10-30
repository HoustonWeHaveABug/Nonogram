if [ $# -ne 2 ]
then
	echo "Usage: %0 <timeout> <solutions max>"
	exit 1
fi
ls nonogram_*.txt | (
	read FILENAME
	while [ "${FILENAME}" ]
	do
		echo "Solving ${FILENAME}..."
		time timeout $1 ../../bin/nonogram 0 $2 <$FILENAME
		read FILENAME
	done
)
