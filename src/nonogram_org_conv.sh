if [ $# -ne 1 ]
then
	echo "Usage: $0 <puzzle number>"
	exit 1
fi
WGET_LOG=./nonogram_org_conv_wget.log
WGET_OUTPUT=./nonogram_org_conv_wget.html
wget -o $WGET_LOG -O $WGET_OUTPUT https://www.nonograms.org/nonograms/i/$1 --inet4-only --no-check-certificate
if [ ! -s $WGET_OUTPUT ]
then
	wget -o $WGET_LOG -O $WGET_OUTPUT https://www.nonograms.org/nonograms2/i/$1 --inet4-only --no-check-certificate
	if [ ! -s $WGET_OUTPUT ]
	then
		echo "Could not download puzzle $1"
		rm -f $WGET_LOG $WGET_OUTPUT
		exit 1
	fi
fi
PUZZLE_JS=./nonogram_org_conv.js
grep "^var d=" $WGET_OUTPUT >$PUZZLE_JS
cat ./nonogram_org_conv_functions.js >>$PUZZLE_JS
rm -f $WGET_LOG $WGET_OUTPUT
