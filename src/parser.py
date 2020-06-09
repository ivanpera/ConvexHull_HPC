#matr is (12,5)
#open file
#while read line
#matr[row, col] = parsed line
#row = row + 1 % 12
#col += row == 0

#mean = matr * eye / 5
import numpy as np
import sys

# def main(argv):
COLS = 5
ROWS = 4
MAXPROC = 4
print(sys.argv)
argv = sys.argv
matr = np.empty((ROWS, COLS), dtype=float)
row = col = 0
f = open(argv[1], "r")
for line in f:
    if "Elapsed time" in line:
        matr[row, col] = float(line[14:-1])
        row = (row + 1) % MAXPROC
        col += row == 0
m = np.sort(matr)[:, 1:-1]
res = m.mean(1)
print(res)
print(f"Speedup = {res[0]/res[:]}")
print(f"Efficiency = {res[0]/np.array([res[nproc - 1] * nproc for nproc in range(1, MAXPROC + 1)])}")
f.close()
