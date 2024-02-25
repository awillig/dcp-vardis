from omnetpp.scave import results
import numpy as np
import glob
import matplotlib.pyplot as plt
import itertools


def combine_samples(avg1, stddev1, count1, avg2, stddev2, count2):
    #Method taken from: https://math.stackexchange.com/a/2971563
    if count1 <= 1:
        if count2 > 1:
            return avg2, stddev2, count2
        else:
            return 0, 0, 0
    elif count2 <= 1:
        if count1 > 1:
            return avg1, stddev1, count1
        else:
            return 0, 0, 0

    new_avg = ((count1 * avg1) + (count2 * avg2)) / (count1 + count2)
    new_var = (((count1 - 1) * np.power(stddev1, 2)) + ((count2 - 1) * np.power(stddev2, 2))) / (count1 + count2 - 1 )
    new_var += ((count1 * count2) * np.power(avg1 + avg2, 2)) / ((count1 + count2) * (count1 + count2 - 1 ))
    return new_avg, np.sqrt(new_var), count1 + count2



RES_DIR = "results"
node_cnt = list(range(3, 19))
separation = [280, 263, 273] #250, 255, 260,
beacon_period = [100, 50]
num_repetitions = [1, 2, 3]
num_summaries = [0, 3, 10]
update_delay_bin_edges = list(range(0, 10001, 20))
seqno_delta_bin_edges = list(range(1000))

seqno_res_file = open(f"{RES_DIR}/../experiment1_avg_seqno_delta.csv", "w")
seqno_res_file.write("node_cnt,separation,beaconperiod,repetitions,summaries,avgdelta,stddev,cnt\n")
delay_res_file = open(f"{RES_DIR}/../experiment1_avg_update_delay.csv", "w")
delay_res_file.write("node_cnt,separation,beaconperiod,repetitions,summaries,avgdelay,stddev,cnt\n")

for (sep, bec_per, rep, num_sum) in itertools.product(separation, beacon_period, num_repetitions, num_summaries):
    overall_res_update_delay = [None for i in range(len(node_cnt))]
    overall_res_seqno_delta = [None for i in range(len(node_cnt))]
    for n in range(len(node_cnt)):
        num_nodes = node_cnt[n]

        update_delay_bin_values = [0 for i in range(len(update_delay_bin_edges) - 1)]
        seqno_delta_bin_values = [0 for i in range(len(seqno_delta_bin_edges) - 1)]

        seqno_delta_average_data = []
        delay_average_data = []

        filelist = list(glob.iglob(f"{RES_DIR}/General-nodeCnt={num_nodes},separation={sep}m,beaconPeriod={bec_per}ms,numRepetitions={rep},numSummaries={num_sum}-#*.sca"))
        print(len(filelist))

        for filename in filelist:
            data = results.read_result_files(filename, filter_expression=f'module =~ "**nodes[{num_nodes-1}].application" AND (name =~ "updateDelay*" OR name =~ "seqnoDelta*")', include_fields_as_scalars=True)
            #to_print = data[data.type=="histogram"][["name", "binedges", "binvalues", "mean", "count", "stddev"]]
            to_print = data[data.type=="histogram"][["name", "mean", "count", "stddev"]]

            for name in list(to_print["name"]):
                if name == "updateDelay:histogram":
                    #bins = list(to_print[to_print.name == name]["binedges"])[0]
                    #cnts = list(to_print[to_print.name == name]["binvalues"])[0]

                    avg = list(to_print[to_print.name == name]["mean"])[0]
                    stddev = list(to_print[to_print.name == name]["stddev"])[0]
                    count = list(to_print[to_print.name == name]["count"])[0]
                    if count < 1:
                        #print(filename)
                        pass
                    else:
                        delay_average_data.append((avg, stddev, count))

                    ##Assume bins in sorted order
                    #i = 0
                    #for j in range(len(bins) - 1):
                    #    while i < len(update_delay_bin_edges) and update_delay_bin_edges[i] < bins[j]:
                    #        i += 1
                    #    if i < len(update_delay_bin_edges):
                    #        update_delay_bin_values[i - 1] += cnts[j]
                    #    else:
                    #        raise Exception(f"Error: bin time coverage too small ({bins[j]})")
                else:
                    #bins = list(to_print[to_print.name == name]["binedges"])[0]
                    #cnts = list(to_print[to_print.name == name]["binvalues"])[0]

                    avg = list(to_print[to_print.name == name]["mean"])[0]
                    stddev = list(to_print[to_print.name == name]["stddev"])[0]
                    count = list(to_print[to_print.name == name]["count"])[0]
                    if count < 1:
                        #print(filename)
                        pass
                    else:
                        seqno_delta_average_data.append((avg, stddev, count))

                    #Assume bins in sorted order
                    #i = 0
                    #for j in range(len(bins) - 1):
                    #    while i < len(seqno_delta_bin_edges) and seqno_delta_bin_edges[i] < bins[j]:
                    #        i += 1
                    #    if i < len(seqno_delta_bin_edges):
                    #        seqno_delta_bin_values[i - 1] += cnts[j]
                    #    else:
                    #        raise Exception(f"Error: bin time coverage too small ({bins[j]})")

        if len(seqno_delta_average_data) > 2:
            avg, std, cnt = seqno_delta_average_data[0]
            for (avg2, std2, cnt2) in seqno_delta_average_data[1:]:
                avg, std, cnt = combine_samples(avg, std, cnt, avg2, std2, cnt2)
            seqno_res_file.write(f"{num_nodes},{sep},{bec_per},{rep},{num_sum},{avg},{std},{cnt}\n")
            seqno_res_file.flush()

        if len(delay_average_data) > 2:
            avg, std, cnt = delay_average_data[0]
            for (avg2, std2, cnt2) in delay_average_data[1:]:
                avg, std, cnt = combine_samples(avg, std, cnt, avg2, std2, cnt2)
            delay_res_file.write(f"{num_nodes},{sep},{bec_per},{rep},{num_sum},{avg},{std},{cnt}\n")
            delay_res_file.flush()

        #overall_res_update_delay[n] = update_delay_bin_values
        #overall_res_seqno_delta[n] = seqno_delta_bin_values

    #f = open(f"{RES_DIR}/../delay_summary/summ-separation={sep}m,beaconPeriod={bec_per}ms,numRepetitions={rep},numSummaries={num_sum}.csv", "w")
    #f.write("bin_edge," + ",".join([str(n) for n in node_cnt]) + "\n")
    #for i in range(len(update_delay_bin_edges) - 1):
    #    f.write(f"{update_delay_bin_edges[i]}," + ",".join([str(c[i]) for c in overall_res_update_delay]) + "\n")
    #f.close()

    #f = open(f"{RES_DIR}/../delta_summary/summ-separation={sep}m,beaconPeriod={bec_per}ms,numRepetitions={rep},numSummaries={num_sum}.csv", "w")
    #f.write("bin_edge," + ",".join([str(n) for n in node_cnt]) + "\n")
    #for i in range(len(seqno_delta_bin_edges) - 1):
    #    f.write(f"{seqno_delta_bin_edges[i]}," + ",".join([str(c[i]) for c in overall_res_seqno_delta]) + "\n")
    #f.close()

seqno_res_file.close()
delay_res_file.close()

#plt.legend()
#plt.show()

