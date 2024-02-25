import matplotlib.pyplot as plt
import matplotlib
import glob
import pandas as pd
import numpy as np
import itertools

RES_FILE_SEQNO = "experiment1_avg_seqno_delta.csv"
RES_FILE_DELAY = "experiment1_avg_update_delay.csv"


def plot_seqno(extension="jpg"):
    f = open(RES_FILE_SEQNO, "r")
    d = f.readlines()
    f.close()

    #node_cnt,separation,beaconperiod,repetitions,summaries,avgdelta,stddev,cnt
    data = [line.strip().split(",") for line in d[1:]]

    separation = [280, 263, 273] #[250, 255, 260]
    beacon_period = [100, 50]
    num_repetitions = [1, 2, 3]
    num_summaries = [0, 3]

    #beacon_period_tl  = {100: "10 Hz beaconing", 50: "20 Hz beaconing"}
    beacon_period_tl  = {100: "$\\beta=10$ Hz", 50: "$\\beta=20$ Hz"}
    summaries_tl = {0: "No summaries", 3: "3 Summaries"}
    summaries_to_filename = {0: "nosummaries", 3: "3summaries"}
    sep_tl = {250: "5% PER", 255: "10% PER", 260: "18% PER", 280: "80% PER", 263: "20% PER", 273: "50% PER"}
    per_to_filename = {250: "5percper", 255: "10percper", 260: "18percper", 280: "80percper", 263: "20percper", 273: "50percper"}
    marker_assign = {100: "o", 50: "D"}

    for sep in separation:
        for summ in num_summaries:
            fig, ax = plt.subplots()
            #ax.set_title(f"{sep_tl[sep]},  {summaries_tl[int(summ)]}")
            for bec in beacon_period:
                max_av = 0
                for (rep, _) in itertools.product(num_repetitions, [1]):
                    x = [int(n) for (n, s, bp, r, summaries, avg, std, cnt) in data if s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ)]
                    y = [float(avg) for (n, s, bp, r, summaries, avg, std, cnt) in data if s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ)]
                    ci =[1.960 * float(std) / np.sqrt(float(cnt)) for (n, s, bp, r, summaries, avg, std, cnt) in data if s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ)]

                    if len(x) > 0:
                        ax.errorbar(x, y, yerr=ci, label=f"{beacon_period_tl[bec]}, $\\mathtt{{repCnt}}=${rep:.0f}", marker=marker_assign[bec])
                        max_av = max(max(y), max_av)
            ax.legend()
            ax.set_xlim([3, 17])
            #ax.set_ylim([0.67, np.ceil(max_av)])
            if extension == "pgf":
                ax.set_xlabel("\# Nodes, $K$")
            else:
                ax.set_xlabel("# Nodes, $K$")
            ax.set_ylabel("Sequence No Difference")

            if (summ == 0) and (sep == 263):
                ax.set_ylim([0, 30])

            fig.savefig(f"experiment1_seqnodelta_{per_to_filename[sep]}_{summaries_to_filename[summ]}.{extension}")


def plot_delay(extension="png"):
    f = open(RES_FILE_DELAY, "r")
    d = f.readlines()
    f.close()

    #node_cnt,separation,beaconperiod,repetitions,summaries,avgdelta,stddev,cnt
    data = [line.strip().split(",") for line in d[1:]]

    separation = [280, 263, 273] #[250, 255, 260]
    beacon_period = [100, 50]
    num_repetitions = [1, 2, 3]
    num_summaries = [0, 3]

    #beacon_period_tl  = {100: "10 Hz beaconing", 50: "20 Hz beaconing"}
    beacon_period_tl  = {100: "$\\beta=10$ Hz", 50: "$\\beta=20$ Hz"}
    summaries_tl = {0: "No summaries", 3: "3 Summaries"}
    summaries_to_filename = {0: "nosummaries", 3: "3summaries"}
    sep_tl = {250: "5% PER", 255: "10% PER", 260: "18% PER", 280: "80% PER", 263: "20% PER", 273: "50% PER"}
    per_to_filename = {250: "5percper", 255: "10percper", 260: "18percper", 280: "80percper", 263: "20percper", 273: "50percper"}
    marker_assign = {100: "o", 50: "D"}

    for sep in separation:
        for summ in num_summaries:
            fig, ax = plt.subplots()
            #ax.set_title(f"{sep_tl[sep]},  {summaries_tl[int(summ)]}")
            for bec in beacon_period:
                max_av = 0
                for (rep, _) in itertools.product(num_repetitions, [1]):
                    x = [int(n) for (n, s, bp, r, summaries, avg, std, cnt) in data if s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ)]
                    y = [float(avg) for (n, s, bp, r, summaries, avg, std, cnt) in data if s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ)]
                    ci =[1.960 * float(std) / np.sqrt(float(cnt)) for (n, s, bp, r, summaries, avg, std, cnt) in data if s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ)]

                    if len(x) > 0:
                        ax.errorbar(x, y, yerr=ci, label=f"{beacon_period_tl[bec]}, $\\mathtt{{repCnt}}=${rep:.0f}", marker=marker_assign[bec])
                        max_av = max(max(y), max_av)
            ax.legend()
            ax.set_xlim([3, 17])
            #ax.set_ylim([0.67, np.ceil(max_av)])
            if extension == "pgf":
                ax.set_xlabel("\# Nodes, $K$")
            else:
                ax.set_xlabel("# Nodes, $K$")
            ax.set_ylabel("Update Delay (ms)")
            fig.savefig(f"experiment1_updatedelay_{per_to_filename[sep]}_{summaries_to_filename[summ]}.{extension}")


def set_up_latex():
    matplotlib.use("pgf")
    plt.rcParams.update({
        "font.family": "serif",  # use serif/main font for text elements
        "text.usetex": True,  # use inline math for ticks
        "pgf.rcfonts": False,  # don't setup fonts from rc parameters
        "pgf.preamble": [
            "\\usepackage{units}",  # load additional packages
            "\\usepackage{metalogo}",
            "\\usepackage{unicode-math}",  # unicode math setup
            "\\usepackage{gensymb}",
        ]
    })


if __name__ == "__main__":
    plot_seqno(extension="pdf")
    plot_delay(extension="pdf")
    #plt.show()

    #set_up_latex()
    #plot_seqno(extension="pgf")
    #plot_delay(extension="pgf")

