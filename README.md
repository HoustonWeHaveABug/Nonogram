# Nonogram

Nonogram solver written in C, solution to challenge https://www.reddit.com/r/dailyprogrammer/comments/am1x6o/20190201_challenge_374_hard_nonogram_solver/.

The program is a backtracker that selects at each step of the recursion the most constrained clue, and tries all the possible combinations for the selected clue. When the first solution is found it is immediately printed, and at the end of execution the program will print the size of the search tree (the number of nodes) and the number of solutions found.

Sample files from the challenge comments are available in this repository, all are solved in less than 50ms on my current computer (Windows 7 Professional, Intel i3-4150 3.50 GHz).
