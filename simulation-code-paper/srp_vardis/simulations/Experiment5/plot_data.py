import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import matplotlib.lines as mlines
import glob
import pandas as pd
import numpy as np
import itertools

from cycler import cycler
import matplotlib.colors as mcolors

MATPLOTLIB_COLOURS = list(mcolors.TABLEAU_COLORS)
MATPLOTLIB_LINE_STYLES = ['-', '--', '-.']

separation = [255, 263]
beacon_period = [100, 50]
num_repetitions = [1, 2, 3]
num_summaries = [10, 20]
update_periods = [200, 300, 400, 500, 750, 1000, 1500, 2000]
grid_sizes = [5, 7, 9, 11, 13]
node_poss = ["edge", "intermediate", "center"]

beacon_period_tl  = {100: "10 Hz beaconing", 50: "20 Hz beaconing"}
summaries_tl = {0: "No summaries", 3: "$\\mathtt{maxSumCnt}=3$", 10: "$\\mathtt{maxSumCnt}=10$", 20: "$\\mathtt{maxSumCnt}=20$"}
sep_tl = {250: "5% PER", 255: "10% PER", 263: "20% PER"}
marker_assign = {100: "o", 50: "D", "edge": "o", "center": "x", "intermediate": "D"}

summaries_to_filename = {0: "nosummaries", 10: "10summaries", 20: "20summaries"}
per_to_filename = {250: "5percper", 255: "10percper", 260: "18percper", 280: "80percper", 263: "20percper", 273: "50percper"}
beacon_period_to_filename  = {100: "10hzbeaconing", 50: "20hzbeaconing"}

colours = {5: MATPLOTLIB_COLOURS[0], 7: MATPLOTLIB_COLOURS[1], 9: MATPLOTLIB_COLOURS[2], 11: MATPLOTLIB_COLOURS[3], 13: MATPLOTLIB_COLOURS[4], 15: MATPLOTLIB_COLOURS[5]}


def plot_seqno_update_rate_on_x_axis():
    f = open("rawresults/experiment3_avg_seqno_delta.csv", "r")
    d = f.readlines()
    f.close()

    #node_cnt,separation,beaconperiod,repetitions,summaries,avgdelta,stddev,cnt
    data = [line.strip().split(",") for line in d[1:]]


    for sep in separation:
        for summ in num_summaries:
            for bec in beacon_period:
                for rep in num_repetitions:
                    fig, ax = plt.subplots()
                    ax.set_title(f"Grid Network, {sep_tl[sep]},  {summaries_tl[int(summ)]},  {beacon_period_tl[bec]}, {rep:.0f} repetitions")
                    max_av = 0
                    for gs in grid_sizes:
                        x = [int(up) for (n, s, bp, r, summaries, up, avg, std, cnt) in data if n == str(gs) and s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ)]
                        y = [float(avg) for (n, s, bp, r, summaries, up, avg, std, cnt) in data if n == str(gs) and s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ)]
                        ci =[1.960 * float(std) / np.sqrt(float(cnt)) for (n, s, bp, r, summaries, up, avg, std, cnt) in data if n == str(gs) and s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ)]

                        if len(x) > 0:
                            ax.errorbar(x, y, yerr=ci, label=f"{gs} x {gs} grid", marker="o")
                            max_av = max(max(y), max_av)
                    ax.plot([190, 2010], [1.5, 1.5])
                    ax.legend()
                    ax.set_xlim([190, 2010])
                    ax.set_ylim([1, 2])
                    ax.set_xlabel("Average Update Period (\N{GREEK SMALL LETTER LAMDA}, ms)")
                    ax.set_ylabel("Sequence No Difference")

def plot_delay_update_rate_on_x_axis():
    f = open("rawresults/experiment3_avg_update_delay.csv", "r")
    d = f.readlines()
    f.close()

    #node_cnt,separation,beaconperiod,repetitions,summaries,avgdelta,stddev,cnt
    data = [line.strip().split(",") for line in d[1:]]

    for sep in separation:
        for summ in num_summaries:
            for bec in beacon_period:
                for rep in num_repetitions:
                    fig, ax = plt.subplots()
                    ax.set_title(f"Grid Network, {sep_tl[sep]},  {summaries_tl[int(summ)]},  {beacon_period_tl[bec]}, {rep:.0f} repetitions")
                    max_av = 0
                    for gs in grid_sizes:
                        x = [int(up) for (n, s, bp, r, summaries, up, avg, std, cnt) in data if n == str(gs) and s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ)]
                        y = [float(avg) for (n, s, bp, r, summaries, up, avg, std, cnt) in data if n == str(gs) and s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ)]
                        ci =[1.960 * float(std) / np.sqrt(float(cnt)) for (n, s, bp, r, summaries, up, avg, std, cnt) in data if n == str(gs) and s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ)]

                        if len(x) > 0:
                            ax.errorbar(x, y, yerr=ci, label=f"{gs} x {gs} grid", marker=marker_assign[bec])
                            max_av = max(max(y), max_av)
                    ax.legend()
                    ax.set_xlim([190, 2010])
                    ax.set_ylim([0, 4500])
                    ax.set_xlabel("Average Update Period (\N{GREEK SMALL LETTER LAMDA}, ms)")
                    ax.set_ylabel("Update Delay (ms)")


def plot_seqno_pos_update_rate_on_x_axis(extension="pdf"):
    f = open("rawresults/experiment3_avg_seqno_delta_pos.csv", "r")
    d = f.readlines()
    f.close()

    #grid_size,separation,beaconperiod,repetitions,summaries,update_period,node_pos,avgdelta,stddev,cnt
    data = [line.strip().split(",") for line in d[1:]]

    for sep in separation:
        for summ in num_summaries:
            for bec in beacon_period:
                for rep in num_repetitions:
                    fig, ax = plt.subplots()
                    ax.set_title(f"Grid Network, {sep_tl[sep]},  {summaries_tl[int(summ)]}, {beacon_period_tl[bec]}, {rep:.0f} repetitions")
                    max_av = 0
                    for pos in node_poss:
                        for gs in grid_sizes:
                            x = [int(up) for (n, s, bp, r, summaries, up, node_pos, avg, std, cnt) in data if n == str(gs) and s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ) and node_pos == pos]
                            y = [float(avg) for (n, s, bp, r, summaries, up, node_pos, avg, std, cnt) in data if n == str(gs) and s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ) and node_pos == pos]
                            ci =[1.960 * float(std) / np.sqrt(float(cnt)) for (n, s, bp, r, summaries, up, node_pos, avg, std, cnt) in data if n == str(gs) and s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ) and node_pos == pos]

                            if len(x) > 0:
                                ax.errorbar(x, y, yerr=ci, label=f"{gs} x {gs} grid, {pos}", marker=marker_assign[pos], color=colours[gs])
                                max_av = max(max(y), max_av)

                    legend_values_sizes = [mpatches.Patch(color=colours[gs], label=f"{gs} x {gs}") for gs in grid_sizes]
                    legend_values_pos = [mlines.Line2D([],[], color='k', marker=marker_assign[pos], label=pos) for pos in node_poss]
                    fig.add_artist(fig.legend(handles=legend_values_sizes, loc='upper center', ncol=5))
                    ax.add_artist(ax.legend(handles=legend_values_pos, loc='upper right'))
                    ax.set_xlim([190, 2010])
                    #ax.set_ylim([0.9, 8])
                    ax.set_xlabel("Average Update Period (\N{GREEK SMALL LETTER LAMDA}, ms)")
                    ax.set_ylabel("Sequence No Difference")
                    fig.savefig(f"experiment3_seqnodelta_{per_to_filename[sep]}_{summaries_to_filename[summ]}_{beacon_period_to_filename[bec]}_{rep:.0f}repetitions.{extension}")


def plot_delay_pos_update_rate_on_x_axis(extension="pdf"):
    f = open("rawresults/experiment3_avg_update_delay_pos.csv", "r")
    d = f.readlines()
    f.close()

    #grid_size,separation,beaconperiod,repetitions,summaries,update_period,node_pos,avgdelta,stddev,cnt
    data = [line.strip().split(",") for line in d[1:]]

    for sep in separation:
        for summ in num_summaries:
            for bec in beacon_period:
                for rep in num_repetitions:
                    fig, ax = plt.subplots()
                    ax.set_title(f"Grid Network, {sep_tl[sep]}, {summaries_tl[int(summ)]}, {beacon_period_tl[bec]}, {rep:.0f} repetitions")
                    max_av = 0
                    for pos in node_poss:
                        for gs in grid_sizes:
                            x = [int(up) for (n, s, bp, r, summaries, up, node_pos, avg, std, cnt) in data if n == str(gs) and s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ) and node_pos == pos]
                            y = [float(avg) for (n, s, bp, r, summaries, up, node_pos, avg, std, cnt) in data if n == str(gs) and s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ) and node_pos == pos]
                            ci =[1.960 * float(std) / np.sqrt(float(cnt)) for (n, s, bp, r, summaries, up, node_pos, avg, std, cnt) in data if n == str(gs) and s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ) and node_pos == pos]

                            if len(x) > 0:
                                ax.errorbar(x, y, yerr=ci, label=f"{gs} x {gs} grid, {pos}", marker=marker_assign[pos], color=colours[gs])
                                max_av = max(max(y), max_av)

                    legend_values_sizes = [mpatches.Patch(color=colours[gs], label=f"{gs} x {gs}") for gs in grid_sizes]
                    legend_values_pos = [mlines.Line2D([],[], color='k', marker=marker_assign[pos], label=pos) for pos in node_poss]
                    fig.add_artist(fig.legend(handles=legend_values_sizes, loc='upper center', ncol=5))
                    ax.add_artist(ax.legend(handles=legend_values_pos, loc='upper right'))
                    ax.set_xlim([190, 2010])
                    #ax.set_ylim([0, 4500])
                    ax.set_xlabel("Average Update Period (\N{GREEK SMALL LETTER LAMDA}, ms)")
                    ax.set_ylabel("Update Delay (ms)")
                    fig.savefig(f"experiment3_updatedelay_{per_to_filename[sep]}_{summaries_to_filename[summ]}_{beacon_period_to_filename[bec]}_{rep:.0f}repetitions.{extension}")
                    #experiment1_seqnodelta_{per_to_filename[sep]}_{summaries_to_filename[summ]}.{extension}


def extract_reliability_capacity_data():
    f = open("rawresults/experiment3_avg_seqno_delta_pos.csv", "r")
    d = f.readlines()
    f.close()

    #grid_size,separation,beaconperiod,repetitions,summaries,update_period,node_pos,avgdelta,stddev,cnt
    data = [line.strip().split(",") for line in d[1:]]

    f = open("rawresults/experiment3_reliability_update_capacity.csv", "w")
    f.write(d[0])
    for i in range(len(data)):
        if float(data[i][-3]) < 1.5:
            f.write(d[i + 1])
    f.close()

    df = pd.read_csv("rawresults/experiment3_reliability_update_capacity.csv")
    df = df.groupby(["grid_size", "separation", "beaconperiod", "repetitions", "summaries", "node_pos"])
    df.min(df.update_period).to_csv("rawresults/experiment3_reliability_update_capacity.csv")


def plot_reliability_capacity_data(extension="pdf"):
    f = open("rawresults/experiment3_reliability_update_capacity.csv", "r")
    d = f.readlines()
    f.close()

    marker_assign = {1: "o", 2: "x", 3: "D"}
    colours = {100: MATPLOTLIB_COLOURS[0], 50: MATPLOTLIB_COLOURS[1]}
    line_styles = {10: "--", 20: "-"}

    #grid_size,separation,beaconperiod,repetitions,summaries,update_period,node_pos,avgdelta,stddev,cnt
    data = [line.strip().split(",") for line in d[1:]]
    for sep in separation:
        for pos in node_poss:
            fig, ax = plt.subplots()
            #ax.set_title(f"Reliability Update Capacity, {sep_tl[sep]}, {pos}")
            max_av = 0
            for bec in beacon_period:
                for summ in num_summaries:
                    for rep in num_repetitions:
                        x = [int(n) for (n, s, bp, r, summaries, node_pos, up, avg, std, cnt) in data if s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ) and node_pos == pos]
                        y = [float(up) for (n, s, bp, r, summaries,node_pos, up, avg, std, cnt) in data if s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ) and node_pos == pos]
                        plt.plot(x, y, label=f"{summaries_tl[int(summ)]}, {rep:.0f} repetitions", marker=marker_assign[rep], color=colours[bec], linestyle=line_styles[summ], fillstyle='none')
                        max_av = max(max(y), max_av)
            ax.set_xlabel("$K$")
            ax.set_ylabel("Average Update Period (\N{GREEK SMALL LETTER LAMDA}, ms)")
            ax.legend()
            ax.set_ylim([190, max_av + 10])
            ax.set_xlim([4.5, 13.5])

            legend_values_sizes = [mlines.Line2D([],[], color='k', linestyle=line_styles[summ], label=summaries_tl[summ]) for summ in num_summaries]
            legend_values_sizes += [mlines.Line2D([],[], color=colours[bec], label=f"$\\beta={1000/bec:.0f}\\,$Hz") for bec in beacon_period]
            legend_values_pos = [mlines.Line2D([],[], color='k', marker=marker_assign[rep], label=f"$\\mathtt{{repCnt}}={rep}$", fillstyle='none') for rep in num_repetitions]
            ax.add_artist(ax.legend(handles=legend_values_sizes, loc='upper center', ncol=4, borderaxespad=-2.5, columnspacing=1))
            ax.add_artist(ax.legend(handles=legend_values_pos, loc='upper left'))
            fig.savefig(f"experiment3_reliability_update_cap_{per_to_filename[sep]}_{pos}.{extension}")


def extract_delay_capacity_data():
    f = open("rawresults/experiment3_avg_update_delay_pos.csv", "r")
    d = f.readlines()
    f.close()

    #grid_size,separation,beaconperiod,repetitions,summaries,update_period,node_pos,avgdelta,stddev,cnt
    data = [line.strip().split(",") for line in d[1:]]

    f = open("rawresults/experiment3_delay_update_capacity.csv", "w")
    f.write(d[0])
    for i in range(len(data)):
        if float(data[i][-3]) < 250: #float(data[i][-5]):
            f.write(d[i + 1])
    f.close()

    df = pd.read_csv("rawresults/experiment3_delay_update_capacity.csv")
    df = df.groupby(["grid_size", "separation", "beaconperiod", "repetitions", "summaries", "node_pos"])
    df.min(df.update_period).to_csv("rawresults/experiment3_delay_update_capacity.csv")


def plot_delay_capacity_data(extension="pdf"):
    f = open("rawresults/experiment3_delay_update_capacity.csv", "r")
    d = f.readlines()
    f.close()

    marker_assign = {1: "o", 2: "x", 3: "D"}
    colours = {100: MATPLOTLIB_COLOURS[0], 50: MATPLOTLIB_COLOURS[1]}
    line_styles = {10: "--", 20: "-"}

    #grid_size,separation,beaconperiod,repetitions,summaries,update_period,node_pos,avgdelta,stddev,cnt
    data = [line.strip().split(",") for line in d[1:]]
    for sep in separation:
        for pos in node_poss:
            fig, ax = plt.subplots()
            #ax.set_title(f"Delay Update Capacity, {sep_tl[sep]}, {pos}")
            max_av = 0
            for bec in beacon_period:
                for summ in num_summaries:
                    for rep in num_repetitions:
                        x = [int(n) for (n, s, bp, r, summaries, node_pos, up, avg, std, cnt) in data if s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ) and node_pos == pos]
                        y = [float(up) for (n, s, bp, r, summaries,node_pos, up, avg, std, cnt) in data if s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ) and node_pos == pos]
                        #plt.plot(x, y, label=f"{summaries_tl[int(summ)]}, {rep:.0f} repetitions", marker=marker_assign[bec])
                        plt.plot(x, y, label=f"{summaries_tl[int(summ)]}, {rep:.0f} repetitions", marker=marker_assign[rep], color=colours[bec], linestyle=line_styles[summ], fillstyle='none')
                        if len(y) > 0:
                            max_av = max(max(y), max_av)
            ax.set_xlabel("$K$")
            ax.set_ylabel("Average Update Period (\N{GREEK SMALL LETTER LAMDA}, ms)")
            ax.legend()
            ax.set_ylim([190, max_av + 10])
            ax.set_xlim([4.5, 13.5])

            legend_values_sizes = [mlines.Line2D([],[], color='k', linestyle=line_styles[summ], label=summaries_tl[summ]) for summ in num_summaries]
            legend_values_sizes += [mlines.Line2D([],[], color=colours[bec], label=f"$\\beta={1000/bec:.0f}\\,$Hz") for bec in beacon_period]
            legend_values_pos = [mlines.Line2D([],[], color='k', marker=marker_assign[rep], label=f"$\\mathtt{{repCnt}}={rep}$", fillstyle='none') for rep in num_repetitions]
            ax.add_artist(ax.legend(handles=legend_values_sizes, loc='upper center', ncol=4, borderaxespad=-2.5, columnspacing=1))
            ax.add_artist(ax.legend(handles=legend_values_pos, loc='upper left'))

            fig.savefig(f"experiment3_delay_update_cap_{per_to_filename[sep]}_{pos}.{extension}")


def plot_summ_effect_update_delay(extension="pdf"):
    f = open("rawresults/experiment3_avg_update_delay_pos.csv", "r")
    d = f.readlines()
    f.close()

    #grid_size,separation,beaconperiod,repetitions,summaries,update_period,node_pos,avgdelta,stddev,cnt
    data = [line.strip().split(",") for line in d[1:]]

    lines_tl = {10: "-", 20: "--"}
    ylims = {"1,100": [0, 4500], "1,50": [0, 1700], "2,100": [0, 12000], "2,50": [0, 5500], "3,100": [0, 16100], "3,50": [0, 8000]}

    for sep in separation:
        for bec in beacon_period:
            for rep in [1, 2, 3]:
                fig, ax = plt.subplots()
                #ax.set_title(f"Grid Network, {sep_tl[sep]}, {beacon_period_tl[bec]}, {rep}")
                max_av = 0
                for summ in num_summaries:
                    for pos in node_poss:
                        for gs in grid_sizes:
                            x = [int(up) for (n, s, bp, r, summaries, up, node_pos, avg, std, cnt) in data if n == str(gs) and s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ) and node_pos == pos]
                            y = [float(avg) for (n, s, bp, r, summaries, up, node_pos, avg, std, cnt) in data if n == str(gs) and s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ) and node_pos == pos]
                            ci =[1.960 * float(std) / np.sqrt(float(cnt)) for (n, s, bp, r, summaries, up, node_pos, avg, std, cnt) in data if n == str(gs) and s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ) and node_pos == pos]

                            if len(x) > 0:
                                ax.errorbar(x, y, yerr=ci, label=f"{gs} x {gs} grid, {pos}, {summaries_tl[int(summ)]}", marker=marker_assign[pos], color=colours[gs], linestyle=lines_tl[int(summ)])
                                max_av = max(max(y), max_av)

                legend_values_sizes = [mpatches.Patch(color=colours[gs], label=f"{gs} x {gs}") for gs in grid_sizes]
                legend_values_pos = [mlines.Line2D([],[], color='k', marker=marker_assign[pos], label=pos) for pos in node_poss]
                legend_values_pos += [mlines.Line2D([],[], color='k', linestyle=lines_tl[int(summ)], label=summaries_tl[int(summ)]) for summ in num_summaries]
                ax.add_artist(ax.legend(handles=legend_values_sizes, loc='upper center', ncol=5, borderaxespad=-2, columnspacing=1.5))
                ax.add_artist(ax.legend(handles=legend_values_pos, loc='upper right'))
                #ax.legend()
                ax.set_xlim([190, 2010])
                ax.set_ylim(ylims[str(rep) + "," + str(bec)])

                ax.set_xlabel("Average Update Period (\N{GREEK SMALL LETTER LAMDA}, ms)")
                ax.set_ylabel("Update Delay (ms)")
                fig.savefig(f"experiment3_updatedelay_numsumcomp_{per_to_filename[sep]}_{beacon_period_to_filename[bec]}_{rep:.0f}repetitions.{extension}")


def plot_summ_effect_seqno_delta(extension="pdf"):
    f = open("rawresults/experiment3_avg_seqno_delta_pos.csv", "r")
    d = f.readlines()
    f.close()

    #grid_size,separation,beaconperiod,repetitions,summaries,update_period,node_pos,avgdelta,stddev,cnt
    data = [line.strip().split(",") for line in d[1:]]

    lines_tl = {10: "-", 20: "--"}
    ylims = {"1,100": [1, 6.5], "1,50": [1, 4.5], "2,100": [1, 7.5], "2,50": [1, 4.5], "3,100": [1, 11], "3,50": [1, 6.5]}

    for sep in separation:
        for bec in beacon_period:
            for rep in [1, 2, 3]:
                fig, ax = plt.subplots()
                #ax.set_title(f"Grid Network, {sep_tl[sep]}, {beacon_period_tl[bec]}, {rep}")
                max_av = 0
                for summ in num_summaries:
                    for pos in node_poss:
                        for gs in grid_sizes:
                            x = [int(up) for (n, s, bp, r, summaries, up, node_pos, avg, std, cnt) in data if n == str(gs) and s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ) and node_pos == pos]
                            y = [float(avg) for (n, s, bp, r, summaries, up, node_pos, avg, std, cnt) in data if n == str(gs) and s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ) and node_pos == pos]
                            ci =[1.960 * float(std) / np.sqrt(float(cnt)) for (n, s, bp, r, summaries, up, node_pos, avg, std, cnt) in data if n == str(gs) and s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ) and node_pos == pos]

                            if len(x) > 0:
                                ax.errorbar(x, y, yerr=ci, label=f"{gs} x {gs} grid, {pos}, {summaries_tl[int(summ)]}", marker=marker_assign[pos], color=colours[gs], linestyle=lines_tl[int(summ)])
                                max_av = max(max(y), max_av)

                legend_values_sizes = [mpatches.Patch(color=colours[gs], label=f"{gs} x {gs}") for gs in grid_sizes]
                legend_values_pos = [mlines.Line2D([],[], color='k', marker=marker_assign[pos], label=pos) for pos in node_poss]
                legend_values_pos += [mlines.Line2D([],[], color='k', linestyle=lines_tl[int(summ)], label=summaries_tl[int(summ)]) for summ in num_summaries]
                ax.add_artist(ax.legend(handles=legend_values_sizes, loc='upper center', ncol=5, borderaxespad=-2, columnspacing=1.5))
                ax.add_artist(ax.legend(handles=legend_values_pos, loc='upper right'))
                #ax.legend()
                ax.set_xlim([190, 2010])
                ax.set_ylim(ylims[str(rep) + "," + str(bec)])

                ax.set_xlabel("Average Update Period (\N{GREEK SMALL LETTER LAMDA}, ms)")
                ax.set_ylabel("Sequence No Difference")
                fig.savefig(f"experiment3_seqnodelta_numsumcomp_{per_to_filename[sep]}_{beacon_period_to_filename[bec]}_{rep:.0f}repetitions.{extension}")



def plot_num_rep_effect_seqno_delta(extension="pdf"):
    f = open("rawresults/experiment3_avg_seqno_delta_pos.csv", "r")
    d = f.readlines()
    f.close()

    #grid_size,separation,beaconperiod,repetitions,summaries,update_period,node_pos,avgdelta,stddev,cnt
    data = [line.strip().split(",") for line in d[1:]]

    lines_tl = {1: "-", 2: "--", 3:"-."}
    #ylims = {"1,100": [1, 6.5], "1,50": [1, 4.5], "2,100": [1, 7.5], "2,50": [1, 4.5], "3,100": [1, 11], "3,50": [1, 6.5]}

    for sep in separation:
        for bec in beacon_period:
            for summ in num_summaries:
                for gs in grid_sizes:
                    fig, ax = plt.subplots()
                    #ax.set_title(f"Grid Network, {sep_tl[sep]}, {beacon_period_tl[bec]}, {summaries_tl[int(summ)]}")
                    max_av = 0
                    for rep in [1, 2, 3]:
                        for pos in node_poss:
                            x = [int(up) for (n, s, bp, r, summaries, up, node_pos, avg, std, cnt) in data if n == str(gs) and s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ) and node_pos == pos]
                            y = [float(avg) for (n, s, bp, r, summaries, up, node_pos, avg, std, cnt) in data if n == str(gs) and s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ) and node_pos == pos]
                            ci =[1.960 * float(std) / np.sqrt(float(cnt)) for (n, s, bp, r, summaries, up, node_pos, avg, std, cnt) in data if n == str(gs) and s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ) and node_pos == pos]

                            if len(x) > 0:
                                ax.errorbar(x, y, yerr=ci, label=f"{gs} x {gs} grid, {pos}, {summaries_tl[int(summ)]}", marker=marker_assign[pos], color=MATPLOTLIB_COLOURS[0], linestyle=lines_tl[int(rep)])
                                max_av = max(max(y), max_av)

                    legend_values_sizes = [mpatches.Patch(color=colours[gs], label=f"{gs} x {gs}") for gs in grid_sizes]
                    legend_values_pos = [mlines.Line2D([],[], color='k', marker=marker_assign[pos], label=pos) for pos in node_poss]
                    legend_values_pos += [mlines.Line2D([],[], color='k', linestyle=lines_tl[int(rep)], label=f"$\\mathtt{{repCnt}}=${rep:.0f}") for rep in [1,2,3]]
                    #ax.add_artist(ax.legend(handles=legend_values_sizes, loc='upper center', ncol=5, borderaxespad=-2, columnspacing=1.5))
                    ax.add_artist(ax.legend(handles=legend_values_pos, loc='upper right'))
                    ax.set_xlim([190, 2010])
                    if gs != 5:
                        ax.set_ylim([1, int(np.ceil(max_av))])
                    else:
                        if bec == 100:
                            ax.set_ylim([1, 2.25])
                        else:
                            ax.set_ylim([1, 1.4])
                    ax.set_xlabel("Average Update Period (\N{GREEK SMALL LETTER LAMDA}, ms)")
                    ax.set_ylabel("Sequence No Difference")
                    fig.savefig(f"experiment3_seqnodelta_numrepcomp_{gs}by{gs}_{per_to_filename[sep]}_{beacon_period_to_filename[bec]}_{summ}summaries.{extension}")



def plot_num_rep_effect_update_delay_both_bp(extension="pdf"):
    f = open("rawresults/experiment3_avg_update_delay_pos.csv", "r")
    d = f.readlines()
    f.close()

    #grid_size,separation,beaconperiod,repetitions,summaries,update_period,node_pos,avgdelta,stddev,cnt
    data = [line.strip().split(",") for line in d[1:]]

    lines_tl = {1: "-", 2: "--", 3:"-."}

    ylims = {"5,100":[0, 650], "7,100":[0,2500], "9,100":[0,5300], "11,100":[0,10000], "13,100":[0, 16400],
             "5,50":[0, 250], "7,50":[0,1100], "9,50":[0, 2600], "11,50":[0,5000], "13,50":[0, 8000]}

    colours = {100: MATPLOTLIB_COLOURS[0], 50: MATPLOTLIB_COLOURS[1]}

    for sep in separation:
        for summ in num_summaries:
            for gs in grid_sizes:
                fig, ax = plt.subplots()
                #ax.set_title(f"Grid Network, {gs}, {sep_tl[sep]}, {beacon_period_tl[bec]}, {summaries_tl[int(summ)]}")
                max_av = 0
                for bec in beacon_period:
                    for rep in [1, 2, 3]:
                        for pos in node_poss:
                            x = [int(up) for (n, s, bp, r, summaries, up, node_pos, avg, std, cnt) in data if n == str(gs) and s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ) and node_pos == pos]
                            y = [float(avg) for (n, s, bp, r, summaries, up, node_pos, avg, std, cnt) in data if n == str(gs) and s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ) and node_pos == pos]
                            ci =[1.960 * float(std) / np.sqrt(float(cnt)) for (n, s, bp, r, summaries, up, node_pos, avg, std, cnt) in data if n == str(gs) and s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ) and node_pos == pos]

                            if len(x) > 0:
                                ax.errorbar(x, y, yerr=ci, label=f"{gs} x {gs} grid, {pos}, {summaries_tl[int(summ)]}", marker=marker_assign[pos], color=colours[bec], linestyle=lines_tl[int(rep)])
                                max_av = max(max(y), max_av)

                legend_values_pos = [mlines.Line2D([],[], color='k', marker=marker_assign[pos], label=pos) for pos in node_poss]
                legend_values_pos += [mlines.Line2D([],[], color='k', linestyle=lines_tl[int(rep)], label=f"$\\mathtt{{repCnt}}=${rep:.0f}") for rep in [1,2,3]]
                legend_values_sizes = [mpatches.Patch(color=colours[bec], label=f"$\\beta{{}}={1000/bec:.0f}\\,$Hz") for bec in beacon_period]
                if gs == 5:
                    ax.add_artist(ax.legend(handles=legend_values_sizes, loc='upper center', ncol=2, borderaxespad=-2.5))
                ax.add_artist(ax.legend(handles=legend_values_pos, loc='upper right'))
                ax.set_xlim([190, 2010])
                ax.set_ylim(ylims[f"{gs},{100}"])

                ax.set_xlabel("Average Update Period (\N{GREEK SMALL LETTER LAMDA}, ms)")
                ax.set_ylabel("Update Delay (ms)")
                fig.savefig(f"experiment3_updatedelay_numrepcomp_{gs}by{gs}_{per_to_filename[sep]}_{summ}summaries.{extension}")


def plot_num_rep_effect_update_delay(extension="pdf"):
    f = open("rawresults/experiment3_avg_update_delay_pos.csv", "r")
    d = f.readlines()
    f.close()

    #grid_size,separation,beaconperiod,repetitions,summaries,update_period,node_pos,avgdelta,stddev,cnt
    data = [line.strip().split(",") for line in d[1:]]

    lines_tl = {1: "-", 2: "--", 3:"-."}

    ylims = {"5,100":[0, 650], "7,100":[0,2500], "9,100":[0,5300], "11,100":[0,10000], "13,100":[0, 16400],
             "5,50":[0, 250], "7,50":[0,1100], "9,50":[0, 2600], "11,50":[0,5000], "13,50":[0, 8000]}

    colours = {100: MATPLOTLIB_COLOURS[0], 50: MATPLOTLIB_COLOURS[0]}

    for sep in separation:
        for summ in num_summaries:
            for gs in grid_sizes:
                for bec in beacon_period:
                    fig, ax = plt.subplots()
                    #ax.set_title(f"Grid Network, {gs}, {sep_tl[sep]}, {beacon_period_tl[bec]}, {summaries_tl[int(summ)]}")
                    max_av = 0
                    for rep in [1, 2, 3]:
                        for pos in node_poss:
                            x = [int(up) for (n, s, bp, r, summaries, up, node_pos, avg, std, cnt) in data if n == str(gs) and s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ) and node_pos == pos]
                            y = [float(avg) for (n, s, bp, r, summaries, up, node_pos, avg, std, cnt) in data if n == str(gs) and s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ) and node_pos == pos]
                            ci =[1.960 * float(std) / np.sqrt(float(cnt)) for (n, s, bp, r, summaries, up, node_pos, avg, std, cnt) in data if n == str(gs) and s == str(sep) and bp == str(bec) and r == str(rep) and summaries == str(summ) and node_pos == pos]

                            if len(x) > 0:
                                ax.errorbar(x, y, yerr=ci, label=f"{gs} x {gs} grid, {pos}, {summaries_tl[int(summ)]}", marker=marker_assign[pos], color=colours[bec], linestyle=lines_tl[int(rep)])
                                max_av = max(max(y), max_av)

                    legend_values_pos = [mlines.Line2D([],[], color='k', marker=marker_assign[pos], label=pos) for pos in node_poss]
                    legend_values_pos += [mlines.Line2D([],[], color='k', linestyle=lines_tl[int(rep)], label=f"$\\mathtt{{repCnt}}=${rep:.0f}") for rep in [1,2,3]]
                    legend_values_sizes = [mpatches.Patch(color=colours[bec], label=f"$\\beta{{}}={1000/bec:.0f}\\,$Hz") for bec in beacon_period]
                    #if gs == 5:
                    #    ax.add_artist(ax.legend(handles=legend_values_sizes, loc='upper center', ncol=2, borderaxespad=-2.5))
                    ax.add_artist(ax.legend(handles=legend_values_pos, loc='upper right'))
                    ax.set_xlim([190, 2010])
                    ax.set_ylim(ylims[f"{gs},{bec}"])

                    ax.set_xlabel("Average Update Period (\N{GREEK SMALL LETTER LAMDA}, ms)")
                    ax.set_ylabel("Update Delay (ms)")
                    fig.savefig(f"experiment3_updatedelay_numrepcomp_{gs}by{gs}_{per_to_filename[sep]}_{beacon_period_to_filename[bec]}_{summ}summaries.{extension}")



if __name__ == "__main__":
    #default_cycler = cycler(linestyle=MATPLOTLIB_LINE_STYLES) * cycler(color=MATPLOTLIB_COLOURS)
    #plt.rc("axes", prop_cycle=default_cycler)

    extract_reliability_capacity_data()
    plot_reliability_capacity_data()

    extract_delay_capacity_data()
    plot_delay_capacity_data()


    #plot_seqno_update_rate_on_x_axis()
    #plot_seqno_gridsize_on_x_axis()
    #plot_delay_update_rate_on_x_axis()
    #plot_delay_gridsize_on_x_axis()

    #plot_seqno_pos_update_rate_on_x_axis()
    #plot_seqno_pos_gridsize_on_x_axis()
    #plot_delay_pos_update_rate_on_x_axis()
    #plot_delay_pos_gridsize_on_x_axis()

    plot_summ_effect_update_delay()
    plot_summ_effect_seqno_delta()
    plot_num_rep_effect_seqno_delta()
    plot_num_rep_effect_update_delay()
    #plt.show()
