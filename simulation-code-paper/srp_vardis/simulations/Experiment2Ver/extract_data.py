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
node_cnt = list(range(5, 19))
beacon_period = [100, 50]
num_repetitions = [1, 2, 3]
num_summaries = [10]

seqno_res_file = open(f"{RES_DIR}/../avg_seqno_delta.csv", "w")
seqno_res_file.write("node_cnt,beaconperiod,repetitions,summaries,avgdelta,stddev,cnt\n")
delay_res_file = open(f"{RES_DIR}/../avg_update_delay.csv", "w")
delay_res_file.write("node_cnt,beaconperiod,repetitions,summaries,avgdelay,stddev,cnt\n")

for (bec_per, rep, num_sum) in itertools.product(beacon_period, num_repetitions, num_summaries):
    overall_res_update_delay = [None for i in range(len(node_cnt))]
    overall_res_seqno_delta = [None for i in range(len(node_cnt))]
    for n in range(len(node_cnt)):
        num_nodes = node_cnt[n]

        seqno_delta_average_data = []
        delay_average_data = []

        #General-endNodeX=1120,nodeCnt=6,separation=1120#2f(6_-_1),beaconPeriod=50ms,numRepetitions=1,numSummaries=0-#0.sca
        #General-endNodeX=1120,nodeCnt=5,separation=1120#2f(5_-_1),beaconPeriod=50ms,numRepetitions=1,numSummaries=10-#0

        filelist = list(glob.iglob(f"{RES_DIR}/**/General-endNodeX=1120,nodeCnt={num_nodes},*,beaconPeriod={bec_per}ms,numRepetitions={rep},numSummaries={num_sum}-#*.sca", recursive=True))
        print(len(filelist))

        for filename in filelist:
            data = results.read_result_files(filename, filter_expression=f'module =~ "**nodes[{num_nodes-1}].application" AND (name =~ "updateDelay:stats" OR name =~ "seqnoDelta:stats")', include_fields_as_scalars=True)
            to_print = data[data.type=="statistic"][["name", "mean", "count", "stddev"]]

            for name in list(to_print["name"]):
                if name == "updateDelay:stats":
                    avg = list(to_print[to_print.name == name]["mean"])[0]
                    stddev = list(to_print[to_print.name == name]["stddev"])[0]
                    count = list(to_print[to_print.name == name]["count"])[0]
                    if count > 0:
                        delay_average_data.append((avg, stddev, count))
                else:
                    avg = list(to_print[to_print.name == name]["mean"])[0]
                    stddev = list(to_print[to_print.name == name]["stddev"])[0]
                    count = list(to_print[to_print.name == name]["count"])[0]
                    if count > 0:
                        seqno_delta_average_data.append((avg, stddev, count))

        if len(seqno_delta_average_data) > 0:
            avg, std, cnt = seqno_delta_average_data[0]
            if len(seqno_delta_average_data) > 1:
                for (avg2, std2, cnt2) in seqno_delta_average_data[1:]:
                    avg, std, cnt = combine_samples(avg, std, cnt, avg2, std2, cnt2)
            print(len(filelist), cnt, np.average([c for (_, _, c) in seqno_delta_average_data]))
            seqno_res_file.write(f"{num_nodes},{bec_per},{rep},{num_sum},{avg},{std},{cnt}\n")
            seqno_res_file.flush()

        if len(delay_average_data) > 0:
            avg, std, cnt = delay_average_data[0]
            if len(delay_average_data) > 1:
                for (avg2, std2, cnt2) in delay_average_data[1:]:
                    avg, std, cnt = combine_samples(avg, std, cnt, avg2, std2, cnt2)
            delay_res_file.write(f"{num_nodes},{bec_per},{rep},{num_sum},{avg},{std},{cnt}\n")
            delay_res_file.flush()

seqno_res_file.close()
delay_res_file.close()

