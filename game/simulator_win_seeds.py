# Exemple per executar:
#   python simulator.py Dummy Dummy Dummy Dummy 10

import subprocess
import random
import sys

from multiprocessing import Pool
from collections import defaultdict

N_THREADS = 6  # Recomanació: 2*(no. nuclis)-1 o 2*(no. nuclis)-2


def execute_single_game(input):
    p1, p2, p3, p4, seed = input

    # Command to execute and execution with subprocess
    command = (
        f"./Game -s {seed} {p1} {p2} {p3} {p4} --input default.cnf --output /dev/null"
    )
    res = subprocess.run(list(command.split(" ")), capture_output=True)

    # Extract results (only winner)
    for line in res.stderr.decode().splitlines():
        if "got top score" in line:
            for p in (p1, p2, p3, p4):
                if p in line.split():
                    return (p, seed)


if __name__ == "__main__":
    assert (
        len(sys.argv) >= 6
    ), "Expected 5 arguments: p1, p2, p3, p4, N_PARTIDES (e.g python simulator.py Dummy Dummy Dummy Dummy 10)"

    p1, p2, p3, p4, N_PARTIDES = sys.argv[1:]

    N_PARTIDES = int(N_PARTIDES)

    with Pool(processes=N_THREADS) as pool:
        result = pool.map(
            execute_single_game,
            (
                (p1, p2, p3, p4, seed)
                for seed in random.choices(range(99999), k=N_PARTIDES)
            ),
        )

        res = defaultdict(list)
        for i, j in result:
            res[i].append(j)

        for p in res:
            print(f"Player {p} won with seeds {res[p]}\n")

        print("\n \nAND:")

        for p in res:
            print(
                f"Player {p:12} got {len(res[p])} wins, i.e. {100.0*len(res[p])/N_PARTIDES:2.2f}%"
            )
