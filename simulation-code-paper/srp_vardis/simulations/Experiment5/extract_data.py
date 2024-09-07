from omnetpp.scave import results
import numpy as np
import glob
import matplotlib.pyplot as plt
import itertools


def combine_samples(avg1, stddev1, count1, avg2, stddev2, count2):
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

    #Method taken from: https://math.stackexchange.com/a/2971563
    new_avg = ((count1 * avg1) + (count2 * avg2)) / (count1 + count2)
    new_var = (((count1 - 1) * np.power(stddev1, 2)) + ((count2 - 1) * np.power(stddev2, 2))) / (count1 + count2 -1 )
    new_var += ((count1 * count2) * np.power(avg1 + avg2, 2)) / ((count1 + count2) * (count1 + count2 -1 ))
    return new_avg, np.sqrt(new_var), count1 + count2


def combine_sample_list(avgs, stddevs, counts):
    avg = avgs[0]
    stddev = stddevs[0]
    count = counts[0]

    for i in range(1, len(avgs)):
        avg, stddev, count = combine_samples(avg, stddev, count, avgs[i], stddevs[i], counts[i])

    return avg, stddev, count


RES_DIR = "/local/spe107/BulkRuns_Nov22/Experiment5ResBulk"
node_cnt_x = list(range(7, 15))
beacon_period = [50]
num_repetitions = [1]
num_summaries = [10]
update_periods = [150, 175, 200, 225, 250, 275, 300, 325, 350, 375, 400, 425, 450, 475, 500, 750, 1000, 1500, 2000]

seqno_res_file = open(f"{RES_DIR}/../avg_seqno_delta.csv", "w")
seqno_res_file.write("grid_size,beaconperiod,repetitions,summaries,update_period,avgdelta,stddev,cnt\n")
delay_res_file = open(f"{RES_DIR}/../avg_update_delay.csv", "w")
delay_res_file.write("grid_size,beaconperiod,repetitions,summaries,update_period,avgdelay,stddev,cnt\n")

seqno_res_file_select_pos = open(f"{RES_DIR}/../avg_seqno_delta_pos.csv", "w")
seqno_res_file_select_pos.write("grid_size,beaconperiod,repetitions,summaries,update_period,node_pos,avgdelta,stddev,cnt\n")
delay_res_file_select_pos = open(f"{RES_DIR}/../avg_update_delay_pos.csv", "w")
delay_res_file_select_pos.write("grid_size,beaconperiod,repetitions,summaries,update_period,node_pos,avgdelay,stddev,cnt\n")

for (bec_per, rep, num_sum, update_period) in itertools.product(beacon_period, num_repetitions, num_summaries, update_periods):
    overall_res_update_delay = [None for i in range(len(node_cnt_x))]
    overall_res_seqno_delta = [None for i in range(len(node_cnt_x))]
    for n in range(len(node_cnt_x)):
        num_nodes = node_cnt_x[n]
        seqno_delta_average_data = []
        delay_average_data = []

        seqno_delta_average_data_center = []
        delay_average_data_center = []

        seqno_delta_average_data_intermediate = []
        delay_average_data_intermediate = []

        seqno_delta_average_data_edge = []
        delay_average_data_edge = []

        center_node = (n ** 2) // 2
        edge_node = 0
        intermediate_node = (n + 1) * (n // 4)

        #General-nodeCntX=15,nodeCnt=15_#5e_2,separation=255m,beaconPeriod=50ms,numRepetitions=1,numSummaries=3,updatePeriod=poisson(500),runTime=1042s-#0
        #SmallGrids-LargeSeparations-nodeCntX=5,nodeCnt=5_#5e_2,separation=255,beaconPeriod=100ms,numRepetitions=2,numSummaries=3,updatePeriod=poisson(2000),runTime=4167s-#0.sca
        #LargeGrids-LabRun-nodeCntX=15,nodeCnt=15_#5e_2,separation=255,beaconPeriod=50ms,numRepetitions=1,numSummaries=20,updatePeriod=exponential(2000),runTime=338s-#306
        #SmallGrids-LabRun-nodeCntX=9,separation=1120#2f(9_-_1),nodeCnt=9_#5e_2,beaconPeriod=50ms,numRepetitions=1,numSummaries=10,updatePeriod=exponential(400),runTime=112s-#569
        #SmallGrids-LabRunExtra-updatePeriod=exponential(150),nodeCntX=7,separation=1120#2f(7_-_1),nodeCnt=7_#5e_2,runTime=56s,beaconPeriod=50ms,numRepetitions=1,numSummaries=10-#7
        filelist = list(glob.iglob(f"{RES_DIR}/SmallGrids-LabRun-nodeCntX={num_nodes},separation=1120#2f({num_nodes}_-_1),nodeCnt={num_nodes}_#5e_2,beaconPeriod={bec_per}ms,numRepetitions={rep},numSummaries={num_sum},updatePeriod=exponential({update_period}),*.sca"))
        filelist += list(glob.iglob(f"{RES_DIR}/SmallGrids-LabRunExtra-updatePeriod=exponential({update_period}),nodeCntX={num_nodes},separation=1120#2f({num_nodes}_-_1),nodeCnt={num_nodes}_#5e_2,*,beaconPeriod={bec_per}ms,numRepetitions={rep},numSummaries={num_sum}*.sca"))

        for filename in filelist:
            data = results.read_result_files(filename, filter_expression=f'module =~ "**.application" AND (name =~ "updateDelay:stats" OR name =~ "seqnoDelta:stats")', include_fields_as_scalars=True)
            try:
                to_print = data[data.type=="statistic"][["name", "mean", "count", "stddev"]]

                for name in list(to_print["name"]):
                    if name == "updateDelay:stats":
                        avgs = list(to_print[to_print.name == name]["mean"])
                        stddevs = list(to_print[to_print.name == name]["stddev"])
                        counts = list(to_print[to_print.name == name]["count"])

                        avg, stddev, count = combine_sample_list(avgs, stddevs, counts)

                        delay_average_data_edge.append((avgs[edge_node], stddevs[edge_node], counts[edge_node]))
                        delay_average_data_center.append((avgs[center_node], stddevs[center_node], counts[center_node]))
                        delay_average_data_intermediate.append((avgs[intermediate_node], stddevs[intermediate_node], counts[intermediate_node]))

                        if count > 0:
                            delay_average_data.append((avg, stddev, count))

                    elif name == "seqnoDelta:stats":
                        avgs = list(to_print[to_print.name == name]["mean"])
                        stddevs = list(to_print[to_print.name == name]["stddev"])
                        counts = list(to_print[to_print.name == name]["count"])

                        avg, stddev, count = combine_sample_list(avgs, stddevs, counts)

                        seqno_delta_average_data_edge.append((avgs[edge_node], stddevs[edge_node], counts[edge_node]))
                        seqno_delta_average_data_center.append((avgs[center_node], stddevs[center_node], counts[center_node]))
                        seqno_delta_average_data_intermediate.append((avgs[intermediate_node], stddevs[intermediate_node], counts[intermediate_node]))

                        if count > 0:
                            seqno_delta_average_data.append((avg, stddev, count))
            except KeyError:
                print(f"Missing data from: {filename}")

        if len(seqno_delta_average_data) > 2:
            avg, std, cnt = seqno_delta_average_data[0]
            for (avg2, std2, cnt2) in seqno_delta_average_data[1:]:
                avg, std, cnt = combine_samples(avg, std, cnt, avg2, std2, cnt2)
            seqno_res_file.write(f"{num_nodes},{bec_per},{rep},{num_sum},{update_period},{avg},{std},{cnt}\n")
            seqno_res_file.flush()

        if len(delay_average_data) > 2:
            avg, std, cnt = delay_average_data[0]
            for (avg2, std2, cnt2) in delay_average_data[1:]:
                avg, std, cnt = combine_samples(avg, std, cnt, avg2, std2, cnt2)
            delay_res_file.write(f"{num_nodes},{bec_per},{rep},{num_sum},{update_period},{avg},{std},{cnt}\n")
            delay_res_file.flush()

        if len(seqno_delta_average_data_edge) > 2:
            avg, std, cnt = seqno_delta_average_data_edge[0]
            for (avg2, std2, cnt2) in seqno_delta_average_data_edge[1:]:
                avg, std, cnt = combine_samples(avg, std, cnt, avg2, std2, cnt2)
            seqno_res_file_select_pos.write(f"{num_nodes},{bec_per},{rep},{num_sum},{update_period},edge,{avg},{std},{cnt}\n")
            seqno_res_file_select_pos.flush()

        if len(delay_average_data_edge) > 2:
            avg, std, cnt = delay_average_data_edge[0]
            for (avg2, std2, cnt2) in delay_average_data_edge[1:]:
                avg, std, cnt = combine_samples(avg, std, cnt, avg2, std2, cnt2)
            delay_res_file_select_pos.write(f"{num_nodes},{bec_per},{rep},{num_sum},{update_period},edge,{avg},{std},{cnt}\n")
            delay_res_file_select_pos.flush()

        if len(seqno_delta_average_data_center) > 2:
            avg, std, cnt = seqno_delta_average_data_center[0]
            for (avg2, std2, cnt2) in seqno_delta_average_data_center[1:]:
                avg, std, cnt = combine_samples(avg, std, cnt, avg2, std2, cnt2)
            seqno_res_file_select_pos.write(f"{num_nodes},{bec_per},{rep},{num_sum},{update_period},center,{avg},{std},{cnt}\n")
            seqno_res_file_select_pos.flush()

        if len(delay_average_data_center) > 2:
            avg, std, cnt = delay_average_data_center[0]
            for (avg2, std2, cnt2) in delay_average_data_center[1:]:
                avg, std, cnt = combine_samples(avg, std, cnt, avg2, std2, cnt2)
            delay_res_file_select_pos.write(f"{num_nodes},{bec_per},{rep},{num_sum},{update_period},center,{avg},{std},{cnt}\n")
            delay_res_file_select_pos.flush()

        if len(seqno_delta_average_data_intermediate) > 2:
            avg, std, cnt = seqno_delta_average_data_intermediate[0]
            for (avg2, std2, cnt2) in seqno_delta_average_data_intermediate[1:]:
                avg, std, cnt = combine_samples(avg, std, cnt, avg2, std2, cnt2)
            seqno_res_file_select_pos.write(f"{num_nodes},{bec_per},{rep},{num_sum},{update_period},intermediate,{avg},{std},{cnt}\n")
            seqno_res_file_select_pos.flush()

        if len(delay_average_data_intermediate) > 2:
            avg, std, cnt = delay_average_data_intermediate[0]
            for (avg2, std2, cnt2) in delay_average_data_intermediate[1:]:
                avg, std, cnt = combine_samples(avg, std, cnt, avg2, std2, cnt2)
            delay_res_file_select_pos.write(f"{num_nodes},{bec_per},{rep},{num_sum},{update_period},intermediate,{avg},{std},{cnt}\n")
            delay_res_file_select_pos.flush()

seqno_res_file.close()
delay_res_file.close()
seqno_res_file_select_pos.close()
delay_res_file_select_pos.close()

#plt.legend()
#plt.show()
