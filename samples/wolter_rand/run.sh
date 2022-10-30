WGET_LOG=./run_wget.log
WGET_OUTPUT=./run_wget.tgz
wget -o $WGET_LOG -O $WGET_OUTPUT https://webpbn.com/survey/rand30.tgz --inet4-only --no-check-certificate
if [ ! -s $WGET_OUTPUT ]
then
	echo "Could not download tarball"
	rm -f $WGET_LOG $WGET_OUTPUT
	exit 1
fi
tar xvf $WGET_OUTPUT
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
cd ..
rm -rf 30x30-2
rm -f $WGET_LOG $WGET_OUTPUT
