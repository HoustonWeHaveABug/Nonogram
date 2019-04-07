# 1 - Download tarball https://webpbn.com/survey/rand30.tgz
# 2 - Extract the content in this directory
# 3 - Run this script
ls rand* | (
	read FILENAME
	while [ "${FILENAME}" ]
	do
		echo "Solving ${FILENAME}..."
		time ../../bin/convert_bitmap 30 30 <$FILENAME | timeout 120 ../../bin/nonogram 2
		read FILENAME
	done
)
