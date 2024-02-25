import matplotlib.pyplot as plt
import glob
import pandas as pd
import numpy as np
import itertools


def plot_seqno():
    f = open("avg_seqno_delta.csv", "r")
    d = f.readlines()
    f.close()

    #node_cnt,beaconperiod,repetitions,summaries,avgdelta,stddev,cnt
    data = [line.strip().split(",") for line in d[1:]]

    beacon_period = [100, 50]
    num_repetitions = [1, 2, 3]
    num_summaries = [0, 3]

    beacon_period_tl  = {100: "10 Hz beaconing", 50: "20 Hz beaconing"}
    summaries_tl = {0: "No summaries", 3: "3 Summaries"}
    marker_assign = {100: "o", 50: "D"}

    for summ in num_summaries:
        fig, ax = plt.subplots()
        ax.set_title(f"Experiment 2: {summaries_tl[int(summ)]}")
        for bec in beacon_period:
            max_av = 0
            for (rep, _) in itertools.product(num_repetitions, [1]):
                x = [int(n) for (n, bp, r, summaries, avg, std, cnt) in data if bp == str(bec) and r == str(rep) and summaries == str(summ)]
                y = [float(avg) for (n, bp, r, summaries, avg, std, cnt) in data if bp == str(bec) and r == str(rep) and summaries == str(summ)]
                ci =[1.960 * float(std) / np.sqrt(float(cnt)) for (n, bp, r, summaries, avg, std, cnt) in data if bp == str(bec) and r == str(rep) and summaries == str(summ)]

                if len(x) > 0:
                    ax.errorbar(x, y, yerr=ci, label=f"{beacon_period_tl[bec]}, {rep:.0f} repetitions", marker=marker_assign[bec])
                    max_av = max(max(y), max_av)
        ax.legend()
        ax.set_xlim([5, 18])
        #ax.set_ylim([0.95, np.ceil(max_av)])
        ax.set_xlabel("\# Nodes, $K$")
        ax.set_ylabel("Sequence No Difference")

def plot_delay():
    f = open("avg_update_delay.csv", "r")
    d = f.readlines()
    f.close()

    #node_cnt,beaconperiod,repetitions,summaries,avgdelta,stddev,cnt
    data = [line.strip().split(",") for line in d[1:]]

    beacon_period = [100, 50]
    num_repetitions = [1, 2, 3]
    num_summaries = [0, 3]

    beacon_period_tl  = {100: "$\\beta=10$ Hz", 50: "$\\beta=20$ Hz"}
    summaries_tl = {0: "No summaries", 3: "3 Summaries"}
    marker_assign = {100: "o", 50: "D"}

    for summ in num_summaries:
        fig, ax = plt.subplots()
        ax.set_title(f"Experiment 2: {summaries_tl[int(summ)]}")
        max_av = 0
        for bec in beacon_period:
            for (rep, _) in itertools.product(num_repetitions, [1]):
                x = [int(n) for (n, bp, r, summaries, avg, std, cnt) in data if bp == str(bec) and r == str(rep) and summaries == str(summ)]
                y = [float(avg) for (n, bp, r, summaries, avg, std, cnt) in data if bp == str(bec) and r == str(rep) and summaries == str(summ)]
                ci =[1.960 * float(std) / np.sqrt(float(cnt)) for (n, bp, r, summaries, avg, std, cnt) in data if bp == str(bec) and r == str(rep) and summaries == str(summ)]

                if len(x) > 0:
                    ax.errorbar(x, y, yerr=ci, label=f"{beacon_period_tl[bec]}, $\mathtt{{repCnt}}={rep:.0f}$", marker=marker_assign[bec])
                    max_av = max(max(y), max_av)
        ax.legend()
        ax.set_xlim([6, 18])
        ax.set_ylim([0, 200])
        ax.set_xlabel("\# Nodes, $K$")
        ax.set_ylabel("Update Delay (ms)")


if __name__ == "__main__":
    plot_seqno()
    plot_delay()
    plt.show()
