cat puzzles.txt | (
	read FILENAME
	while [ "${FILENAME}" ]
	do
		echo "Solving ${FILENAME}..."
		time timeout 1800 ../../bin/nonogram 2 <../../puzzles/${FILENAME}
		read FILENAME
	done
)
