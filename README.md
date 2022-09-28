# Nonogram

Nonogram solver written in C, solution to challenge https://www.reddit.com/r/dailyprogrammer/comments/am1x6o/20190201_challenge_374_hard_nonogram_solver/.

The program expects the following arguments on the command line:

- Verbose mode flag (0: Off, 1: On)
- Maximum number of solutions it will search for. This argument is optional and the default value is the LONG_MAX C constant.

It is a backtracker that runs in two phases at each node of the search tree:

Phase 1 - For each clue that have still unknown cells, the program generates all combinations, and then locks all cells that have the same value in all the combinations generated. This phase is iterated until no change can be made in the grid, or no combination is generated for one clue, which is a contradiction and makes the program backtracks.

Phase 2 - If all cells are locked it means a solution was found and it is immediately printed along with the current running time in seconds and size of the search tree (the number of nodes). Otherwise the program selects the "considered as optimal" set using the following criteria in order by running Phase 1 at the next depth for each set:

- Lowest number of possible placements that do not lead to a solution
- Lowest number of possible placements skipped
- Greatest number of placements that lead to a solution
- Greatest number of locked cells for all placements

Inside a given range [a, b] of possible placements, only placements at position a and b are avaluated, the others are skipped.

Phase 2 is iterated as long as at least one placement fails. Then the search continues at the next depth for each placement of the selected set.
 
At the end of execution the program will print the running time in seconds, some statistics about the search tree (total number of nodes, number of run nodes, number of failures) and the number of solutions found.

The number of combinations generated at every possible position and set index is cached for each clue to avoid the same search happening more than once.

Puzzles from the challenge comments and additional ones are available in the "puzzles" folder.

Sample puzzle sets from Jan Wolter's thorough survey (https://webpbn.com/survey) were tested and the test scripts/results are available in the "samples" folder. This folder also contains scripts to run TAAI competitions (like http://kcwu.csie.org/~kcwu/nonogram/taai11/index.html).

The structure of the puzzle data is the following:

- Number of columns
- Number of rows
- Colored puzzle (0: No, 1: Yes)
- Column clues (sets in the same column are enclosed by double quotes - sets/set groups are separated by a comma)
- Row clues (same structure as for the column clues)

If the puzzle is colored, the color is represented by an alphanumeric symbol and specified after each set (it means that the program can manage up to 62 colors). The separator between the set and the color is a dash.

If one column/row is empty, the clues shall be specified as "0".
