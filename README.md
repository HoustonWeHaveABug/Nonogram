# Nonogram

Nonogram solver written in C, solution to challenge https://www.reddit.com/r/dailyprogrammer/comments/am1x6o/20190201_challenge_374_hard_nonogram_solver/.

The program is a backtracker that runs in two phases:

- Phase 1 - Lock all constant cells: for each clue, the program generates all combinations, and then locks all cells that have the same value in all the combinations generated. This phase is iterated until no change can be made in the grid.

- Phase 2 - Guess the remaining cells: the program selects the most constrained clue at each step of the recursion, and tries all the possible combinations for the selected clue. When the first solution is found it is immediately printed, and at the end of execution the program will print the size of the search tree (the number of nodes) and the number of solutions found.

The number of combinations generated at every possible position and set index is cached for each clue to avoid the same search happening more than once.

Sample puzzles from the challenge comments and additional ones are available in this repository.

The structure of the puzzle data is the following:

- Number of columns
- Number of rows
- Colored puzzle (0: No, 1: Yes)
- Column clues (sets in the same column are enclosed by double quotes - sets/set groups are separated by a comma)
- Row clues (same structure as for the column clues)

If the graph is colored, the color is represented by an alphanumeric symbol and specified after each set (it means that the program can manage up to 62 colors). The separator between the set and the color is a dash.
