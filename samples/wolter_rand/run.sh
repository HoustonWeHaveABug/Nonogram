if [ ! -d 30x30-2 ]
then
	echo "1 - Download tarball https://webpbn.com/survey/rand30.tgz"
	echo "2 - Extract the content in this directory"
	echo "3 - Run this script"
	exit 1
fi
cd 30x30-2
ls rand* | (
	read FILENAME
	while [ "${FILENAME}" ]
	do
		echo "Solving ${FILENAME}..."
		time ../../../bin/convert_bitmap 30 30 <$FILENAME | timeout 120 ../../../bin/nonogram 0 2
		read FILENAME
	done
)
