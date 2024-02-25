import matplotlib.pyplot as plt
import glob
import pandas as pd
import numpy as np
import itertools

from cycler import cycler
import matplotlib.colors as mcolors

MATPLOTLIB_COLOURS = list(mcolors.TABLEAU_COLORS)
MATPLOTLIB_LINE_STYLES = ['-', '--', '-.']


def plot_seqno_update_rate_on_x_axis():
    f = open("avg_seqno_delta.csv", "r")
    d = f.readlines()
    f.close()

    #node_cnt,separation,beaconperiod,repetitions,summaries,avgdelta,stddev,cnt
    data = [line.strip().split(",") for line in d[1:]]

    separation = [255]
    beacon_period = [100, 50]
    num_repetitions = [1, 2, 3]
    num_summaries = [3]
    update_periods = [200, 300, 400, 500, 1000, 2000]
    grid_sizes = [5, 7, 9, 11, 13, 15, 17, 19, 21]

    beacon_period_tl  = {100: "10 Hz beaconing", 50: "20 Hz beaconing"}
    summaries_tl = {0: "No summaries", 3: "3 Summaries"}
    sep_tl = {250: "5% PER", 255: "10% PER", 260: "22% PER"}
    marker_assign = {100: "o", 50: "."}

    for sep in separation:
        fig, ax = plt.subplots()
        ax.set_title(f"Grid Network, {sep_tl[sep]}")
        max_av = 0
        for rep in num_repetitions:
            for gs in grid_sizes:
                x = [int(up) for (n, s, r, up, avg, std, cnt) in data if n == str(gs) and s == str(sep) and r == str(rep)]
                y = [float(avg) for (n, s, r, up, avg, std, cnt) in data if n == str(gs) and s == str(sep) and r == str(rep)]
                ci =[1.960 * float(std) / np.sqrt(float(cnt)) for (n, s, r, up, avg, std, cnt) in data if n == str(gs) and s == str(sep) and r == str(rep)]

                if len(x) > 0:
                    ax.errorbar(x, y, yerr=ci, label=f"{gs} x {gs} grid, {rep:.0f} repetitions", marker='o')
                    max_av = max(max(y), max_av)
        ax.legend()
        ax.set_xlim([190, 2010])
        #ax.set_ylim([0.9, 8])
        ax.set_xlabel("Average Update Period (\N{GREEK SMALL LETTER LAMDA}, ms)")
        ax.set_ylabel("Sequence No Difference")

def plot_delay_update_rate_on_x_axis():
    f = open("avg_update_delay.csv", "r")
    d = f.readlines()
    f.close()

    #node_cnt,separation,beaconperiod,repetitions,summaries,avgdelta,stddev,cnt
    data = [line.strip().split(",") for line in d[1:]]

    separation = [255]
    beacon_period = [100, 50]
    num_repetitions = [1, 2, 3]
    num_summaries = [3]
    update_periods = [200, 300, 400, 500, 1000, 2000]
    grid_sizes = [5, 7, 9, 11, 13, 15, 17, 19, 21]

    beacon_period_tl  = {100: "10 Hz beaconing", 50: "20 Hz beaconing"}
    summaries_tl = {0: "No summaries", 3: "3 Summaries"}
    sep_tl = {250: "5% PER", 255: "10% PER", 260: "22% PER"}
    marker_assign = {100: "o", 50: "."}

    for sep in separation:
        fig, ax = plt.subplots()
        ax.set_title(f"Grid Network, {sep_tl[sep]}")
        max_av = 0
        for rep in num_repetitions:
            for gs in grid_sizes:
                x = [int(up) for (n, s, r, up, avg, std, cnt) in data if n == str(gs) and s == str(sep) and r == str(rep)]
                y = [float(avg) for (n, s, r, up, avg, std, cnt) in data if n == str(gs) and s == str(sep) and r == str(rep)]
                ci =[1.960 * float(std) / np.sqrt(float(cnt)) for (n, s, r, up, avg, std, cnt) in data if n == str(gs) and s == str(sep) and r == str(rep)]

                if len(x) > 0:
                    ax.errorbar(x, y, yerr=ci, label=f"{gs} x {gs} grid, {rep:.0f} repetitions", marker='o')
                    max_av = max(max(y), max_av)
        ax.legend()
        ax.set_xlim([190, 2010])
        #ax.set_ylim([0, 4500])
        ax.set_xlabel("Average Update Period (\N{GREEK SMALL LETTER LAMDA}, ms)")
        ax.set_ylabel("Update Delay (ms)")


def plot_seqno_pos_update_rate_on_x_axis():
    f = open("avg_seqno_delta_pos.csv", "r")
    d = f.readlines()
    f.close()

    #grid_size,separation,beaconperiod,repetitions,summaries,update_period,node_pos,avgdelta,stddev,cnt
    data = [line.strip().split(",") for line in d[1:]]

    separation = [255]
    beacon_period = [100, 50]
    num_repetitions = [1, 2, 3]
    num_summaries = [3]
    update_periods = [200, 300, 400, 500, 1000, 2000]
    grid_sizes = [5, 7, 9, 11, 13, 15, 17, 19, 21]
    node_poss = ["edge", "intermediate", "center"]

    beacon_period_tl  = {100: "10 Hz beaconing", 50: "20 Hz beaconing"}
    summaries_tl = {0: "No summaries", 3: "3 Summaries"}
    sep_tl = {250: "5% PER", 255: "10% PER", 260: "22% PER"}
    marker_assign = {100: "o", 50: "."}

    for sep in separation:
        fig, ax = plt.subplots()
        ax.set_title(f"Grid Network, {sep_tl[sep]}, By Position")
        max_av = 0
        for rep in num_repetitions:
            for gs in grid_sizes:
                for pos in node_poss:
                    x = [int(up) for (n, s, r, up, node_pos, avg, std, cnt) in data if n == str(gs) and s == str(sep) and r == str(rep) and node_pos == pos]
                    y = [float(avg)for (n, s, r, up, node_pos, avg, std, cnt) in data if n == str(gs) and s == str(sep) and r == str(rep) and node_pos == pos]
                    ci =[1.960 * float(std) / np.sqrt(float(cnt)) for (n, s, r, up, node_pos, avg, std, cnt) in data if n == str(gs) and s == str(sep) and r == str(rep) and node_pos == pos]

                    if len(x) > 0:
                        ax.errorbar(x, y, yerr=ci, label=f"{gs} x {gs} grid, {rep:.0f} repetitions, {pos}", marker='o')
                        max_av = max(max(y), max_av)
        ax.legend()
        ax.set_xlim([190, 2010])
        #ax.set_ylim([0.9, 8])
        ax.set_xlabel("Average Update Period (\N{GREEK SMALL LETTER LAMDA}, ms)")
        ax.set_ylabel("Sequence No Difference")


def plot_delay_pos_update_rate_on_x_axis():
    f = open("avg_update_delay_pos.csv", "r")
    d = f.readlines()
    f.close()

    #grid_size,separation,beaconperiod,repetitions,summaries,update_period,node_pos,avgdelta,stddev,cnt
    data = [line.strip().split(",") for line in d[1:]]

    separation = [255]
    beacon_period = [100, 50]
    num_repetitions = [1, 2, 3]
    num_summaries = [3]
    update_periods = [200, 300, 400, 500, 1000, 2000]
    grid_sizes = [5, 7, 9, 11, 13, 15, 17, 19, 21]
    node_poss = ["edge", "intermediate", "center"]

    beacon_period_tl  = {100: "10 Hz beaconing", 50: "20 Hz beaconing"}
    summaries_tl = {0: "No summaries", 3: "3 Summaries"}
    sep_tl = {250: "5% PER", 255: "10% PER", 260: "22% PER"}
    marker_assign = {100: "o", 50: "."}

    for sep in separation:
        fig, ax = plt.subplots()
        ax.set_title(f"Grid Network, {sep_tl[sep]}, By Position")
        max_av = 0
        for rep in num_repetitions:
            for gs in grid_sizes:
                for pos in node_poss:
                    x = [int(up) for (n, s, r, up, node_pos, avg, std, cnt) in data if n == str(gs) and s == str(sep) and r == str(rep) and node_pos == pos]
                    y = [float(avg)for (n, s, r, up, node_pos, avg, std, cnt) in data if n == str(gs) and s == str(sep) and r == str(rep) and node_pos == pos]
                    ci =[1.960 * float(std) / np.sqrt(float(cnt)) for (n, s, r, up, node_pos, avg, std, cnt) in data if n == str(gs) and s == str(sep) and r == str(rep) and node_pos == pos]

                    if len(x) > 0:
                        ax.errorbar(x, y, yerr=ci, label=f"{gs} x {gs} grid, {rep:.0f} repetitions, {pos}", marker='o')
                        max_av = max(max(y), max_av)
            ax.legend()
            ax.set_xlim([190, 2010])
            #ax.set_ylim([0, 4500])
            ax.set_xlabel("Average Update Period (\N{GREEK SMALL LETTER LAMDA}, ms)")
            ax.set_ylabel("Update Delay (ms)")


def plot_seqno_gridsize_on_x_axis():
    f = open("avg_seqno_delta.csv", "r")
    d = f.readlines()
    f.close()

    #node_cnt,separation,beaconperiod,repetitions,summaries,avgdelta,stddev,cnt
    data = [line.strip().split(",") for line in d[1:]]

    separation = [255]
    beacon_period = [100, 50]
    num_repetitions = [1, 2, 3]
    num_summaries = [3]
    update_periods = [200, 300, 400, 500, 1000, 2000]
    grid_sizes = [5, 7, 9, 11, 13, 15, 17, 19, 21]

    beacon_period_tl  = {100: "10 Hz beaconing", 50: "20 Hz beaconing"}
    summaries_tl = {0: "No summaries", 3: "3 Summaries"}
    sep_tl = {250: "5% PER", 255: "10% PER", 260: "22% PER"}
    marker_assign = {100: "o", 50: "."}

    for sep in separation:
        fig, ax = plt.subplots()
        ax.set_title(f"Grid Network, {sep_tl[sep]}")
        max_av = 0
        for rep in num_repetitions:
            for update_period in update_periods:
                x = [int(n) for (n, s, r, up, avg, std, cnt) in data if up == str(update_period) and s == str(sep) and r == str(rep)]
                y = [float(avg) for (n, s, r, up, avg, std, cnt) in data if up == str(update_period) and s == str(sep) and r == str(rep)]
                ci =[1.960 * float(std) / np.sqrt(float(cnt)) for (n, s, r, up, avg, std, cnt) in data if up == str(update_period) and s == str(sep) and r == str(rep)]

                if len(x) > 0:
                    ax.errorbar(x, y, yerr=ci, label=f"\N{GREEK SMALL LETTER LAMDA}={update_period}ms, {rep:.0f} repetitions", marker='o')
                    max_av = max(max(y), max_av)
        ax.legend()
        ax.set_xlim([5, 15])
        #ax.set_ylim([0.9, 8])
        ax.set_xlabel("Grid Size (K)")
        ax.set_ylabel("Sequence No Difference")


def plot_delay_gridsize_on_x_axis():
    f = open("avg_update_delay.csv", "r")
    d = f.readlines()
    f.close()

    #node_cnt,separation,beaconperiod,repetitions,summaries,avgdelta,stddev,cnt
    data = [line.strip().split(",") for line in d[1:]]

    separation = [255]
    beacon_period = [100, 50]
    num_repetitions = [1, 2, 3]
    num_summaries = [3]
    update_periods = [200, 300, 400, 500, 1000, 2000]
    grid_sizes = [5, 7, 9, 11, 13, 15, 17, 19, 21]

    beacon_period_tl  = {100: "10 Hz beaconing", 50: "20 Hz beaconing"}
    summaries_tl = {0: "No summaries", 3: "3 Summaries"}
    sep_tl = {250: "5% PER", 255: "10% PER", 260: "22% PER"}
    marker_assign = {100: "o", 50: "."}

    for sep in separation:
        fig, ax = plt.subplots()
        ax.set_title(f"Grid Network, {sep_tl[sep]}")
        max_av = 0
        for rep in num_repetitions:
            for update_period in update_periods:
                x = [int(n) for (n, s, r, up, avg, std, cnt) in data if up == str(update_period) and s == str(sep) and r == str(rep)]
                y = [float(avg) for (n, s, r, up, avg, std, cnt) in data if up == str(update_period) and s == str(sep) and r == str(rep)]
                ci =[1.960 * float(std) / np.sqrt(float(cnt)) for (n, s, r, up, avg, std, cnt) in data if up == str(update_period) and s == str(sep) and r == str(rep)]


                if len(x) > 0:
                    ax.errorbar(x, y, yerr=ci, label=f"\N{GREEK SMALL LETTER LAMDA}={update_period}ms, {rep:.0f} repetitions", marker='o')
                    max_av = max(max(y), max_av)
        ax.legend()
        ax.set_xlim([5, 15])
        #ax.set_ylim([0, 4500])
        ax.set_xlabel("Grid Size (K)")
        ax.set_ylabel("Update Delay (ms)")


def plot_seqno_pos_gridsize_on_x_axis():
    f = open("avg_seqno_delta_pos.csv", "r")
    d = f.readlines()
    f.close()

    #grid_size,separation,beaconperiod,repetitions,summaries,update_period,node_pos,avgdelta,stddev,cnt
    data = [line.strip().split(",") for line in d[1:]]

    separation = [255]
    beacon_period = [100, 50]
    num_repetitions = [1, 2, 3]
    num_summaries = [3]
    update_periods = [200, 300, 400, 500, 1000, 2000]
    grid_sizes = [5, 7, 9, 11, 13, 15, 17, 19, 21]
    node_poss = ["edge", "intermediate", "center"]

    beacon_period_tl  = {100: "10 Hz beaconing", 50: "20 Hz beaconing"}
    summaries_tl = {0: "No summaries", 3: "3 Summaries"}
    sep_tl = {250: "5% PER", 255: "10% PER", 260: "22% PER"}
    marker_assign = {100: "o", 50: "."}

    for sep in separation:
        for update_period in update_periods:
            fig, ax = plt.subplots()
            ax.set_title(f"Grid Network, {sep_tl[sep]}, By Position, \N{GREEK SMALL LETTER LAMDA}={update_period}ms")
            max_av = 0
            for rep in num_repetitions:
                for pos in node_poss:
                    x = [int(n) for (n, s, r, up, node_pos, avg, std, cnt) in data if up == str(update_period) and s == str(sep) and r == str(rep) and node_pos == pos]
                    y = [float(avg) for (n, s, r, up, node_pos, avg, std, cnt) in data if up == str(update_period) and s == str(sep) and r == str(rep) and node_pos == pos]
                    ci =[1.960 * float(std) / np.sqrt(float(cnt)) for (n, s, r, up, node_pos, avg, std, cnt) in data if up == str(update_period) and s == str(sep) and r == str(rep) and node_pos == pos]

                    if len(x) > 0:
                        ax.errorbar(x, y, yerr=ci, label=f"{rep:.0f} repetitions, {pos}", marker='o')
                        max_av = max(max(y), max_av)
            ax.legend()
            ax.set_xlim([5, 15])
            #ax.set_ylim([0.9, 8])
            ax.set_xlabel("Grid Size (K)")
            ax.set_ylabel("Sequence No Difference")


def plot_delay_pos_gridsize_on_x_axis():
    f = open("avg_update_delay_pos.csv", "r")
    d = f.readlines()
    f.close()

    #grid_size,separation,beaconperiod,repetitions,summaries,update_period,node_pos,avgdelta,stddev,cnt
    data = [line.strip().split(",") for line in d[1:]]

    separation = [255]
    beacon_period = [100, 50]
    num_repetitions = [1, 2, 3]
    num_summaries = [3]
    update_periods = [200, 300, 400, 500, 1000, 2000]
    grid_sizes = [5, 7, 9, 11, 13, 15, 17, 19, 21]
    node_poss = ["edge", "intermediate", "center"]

    beacon_period_tl  = {100: "10 Hz beaconing", 50: "20 Hz beaconing"}
    summaries_tl = {0: "No summaries", 3: "3 Summaries"}
    sep_tl = {250: "5% PER", 255: "10% PER", 260: "22% PER"}
    marker_assign = {100: "o", 50: "."}

    for sep in separation:
        for update_period in update_periods:
            fig, ax = plt.subplots()
            ax.set_title(f"Grid Network, {sep_tl[sep]}, By Position, \N{GREEK SMALL LETTER LAMDA}={update_period}ms")
            max_av = 0
            for rep in num_repetitions:
                for pos in node_poss:
                    x = [int(n) for (n, s, r, up, node_pos, avg, std, cnt) in data if up == str(update_period) and s == str(sep) and r == str(rep) and node_pos == pos]
                    y = [float(avg) for (n, s, r, up, node_pos, avg, std, cnt) in data if up == str(update_period) and s == str(sep) and r == str(rep) and node_pos == pos]
                    ci =[1.960 * float(std) / np.sqrt(float(cnt)) for (n, s, r, up, node_pos, avg, std, cnt) in data if up == str(update_period) and s == str(sep) and r == str(rep) and node_pos == pos]

                    if len(x) > 0:
                        ax.errorbar(x, y, yerr=ci, label=f"{rep:.0f} repetitions, {pos}", marker='o')
                        max_av = max(max(y), max_av)
            ax.legend()
            ax.set_xlim([5, 15])
            #ax.set_ylim([0, min(np.ceil(max_av / 500) * 500, 4500)])
            ax.set_xlabel("Grid Size (K)")
            ax.set_ylabel("Update Delay (ms)")


if __name__ == "__main__":
    default_cycler = cycler(linestyle=MATPLOTLIB_LINE_STYLES) * cycler(color=MATPLOTLIB_COLOURS)
    plt.rc("axes", prop_cycle=default_cycler)
    plot_seqno_update_rate_on_x_axis()
    #plot_seqno_gridsize_on_x_axis()
    plot_delay_update_rate_on_x_axis()
    #plot_delay_gridsize_on_x_axis()

    plot_seqno_pos_update_rate_on_x_axis()
    #plot_seqno_pos_gridsize_on_x_axis()
    plot_delay_pos_update_rate_on_x_axis()
    #plot_delay_pos_gridsize_on_x_axis()
    plt.show()
