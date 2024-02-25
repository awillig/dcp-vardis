import matplotlib.pyplot as plt
import glob
import numpy as np
import scipy.stats as st

END_TIME = 9950
START_TIME = 50

def plot_ecdf(fig, data, label=""):
    fig.gca().plot(np.sort(data), np.linspace(0, 1, len(data)), label=label)


def plot_ecdf_axes(ax, data, label=""):
    ax.plot(np.sort(data), np.linspace(0, 1, len(data)), label=label)


def get_res_from_file_old(filename):
    creation_times = dict() #[var][seq_no] = times
    delays = dict()         #[var][seq_no] = (node, time)
    f = open(filename, "r")

    #res_file format
    #  address,variable_id,is_producer,seq_no,timestamp

    valid = False
    line = f.readline().strip()
    while line != "":
        node, var_id, prod, seq_no, ts = line.split(',')
        if int(prod) == 1:
            var = creation_times.get(var_id, dict())
            t = var.get(seq_no, [])
            t.append(float(ts))
            var[seq_no] = t
            creation_times[var_id] = var
            if float(ts) > END_TIME:
                valid = True
        else:
            var = delays.get(var_id, dict())
            t = var.get(seq_no, [])
            t.append((node, float(ts)))
            var[seq_no] = t
            delays[var_id] = var
        line = f.readline().strip()

    results = dict()
    for var in list(creation_times.keys()):
        v = dict()
        for seq_no in list(creation_times[var].keys()):
            creates = sorted(creation_times[var][seq_no])
            create_strings = [str(c) for c in creates]
            for (node, time) in delays[var].get(seq_no, []):
                found = False
                i = 0
                while i < len(creates) - 1 and not found:
                    if creates[i + 1] > time:
                        found = True
                    else:
                        i += 1
                s = v.get(f"{seq_no}:{create_strings[i]}",dict())
                s["created"] = creates[i]
                r = s.get("delays", [])
                r.append((node, (time - creates[i]) * 1000))
                s["delays"] = r
                v[f"{seq_no}:{create_strings[i]}"] = s
        results[var] = v

    if valid:
        return results
    else:
        return dict()

def get_res_from_file(filename):
    creation_times = dict() #[var][seq_no] = times
    delays = dict()         #[var][seq_no] = (node, time)
    f = open(filename, "r")

    #res_file format
    #  address,variable_id,is_producer,seq_no,timestamp

    valid = False
    line = f.readline().strip()
    while line != "":
        node, var_id, prod, seq_no, ts = line.split(',')
        if int(prod) == 1:
            var = creation_times.get(var_id, dict())
            var["creator"] = node
            t = var.get(seq_no, [])
            t.append(float(ts))
            var[seq_no] = t
            creation_times[var_id] = var
            if float(ts) > END_TIME:
                valid = True
        else:
            var = delays.get(var_id, dict())
            t = var.get(seq_no, [])
            t.append((node, float(ts)))
            var[seq_no] = t
            delays[var_id] = var
        line = f.readline().strip()

    results = dict()
    for var in list(creation_times.keys()):
        v = dict()
        creator = creation_times[var]["creator"]
        creation_times[var].pop("creator")
        for seq_no in list(creation_times[var].keys()):
            creates = sorted(creation_times[var][seq_no])
            create_strings = [str(c) for c in creates]
            for (node, time) in delays[var].get(seq_no, []):
                if node != creator:
                    found = False
                    i = 0
                    while i < len(creates) - 1 and not found:
                        if creates[i + 1] > time:
                            found = True
                        else:
                            i += 1
                    if creates[i] > time:
                        print(f"Something has gone wrong: {filename}, {seq_no}, {create_strings[i]}, {node}: {time}")
                    s = v.get(f"{seq_no}:{create_strings[i]}",dict())
                    s["created"] = creates[i]
                    r = s.get("delays", [])
                    r.append((node, (time - creates[i]) * 1000))
                    s["delays"] = r
                    v[f"{seq_no}:{create_strings[i]}"] = s
        results[var] = v

    if valid:
        return results
    else:
        return dict()


def main():
    PER = 5
    files = list(glob.iglob("/local/spe107/results/GridNetworkExperiment/complete2_250/**/*.csv", recursive=True))

    fig = plt.figure()

    overall_delays = dict()

    for filename in files:
        name = filename.split('/')
        density = name[-3]
        packet_size = name[-2]
        data = get_res_from_file(filename)

        if len(list(data.keys())) == 0:
            print(f"Ignoring data from incomplete replication... {density} nodes, {packet_size} B packets: {filename}")
        else:
            incomplete_delivery = []
            if int(density) > 20:
                target_node = 11725260718100
            else:
                target_node = 11725260718090

            for v in data.keys():
                delays_per_var = []

                for seq_no in data[v].keys():
                    if START_TIME < data[v][seq_no]["created"] < END_TIME:
                        delays = data[v][seq_no]["delays"]
                        if len(delays) > 0:
                            if len(delays) < (int(density) - 1):
                                incomplete_delivery.append((v, seq_no))
                            else:
                                r = [d for (i, d) in delays if int(i) == target_node]
                                if len(r) > 0:
                                    delays_per_var.append(r[0])
                                else:
                                    print(f"{seq_no} not delived to node of interest: {filename}")
                print(f"{density} nodes, {packet_size} B packets, {len(incomplete_delivery)} updates not completely delivered...")
                #plot_ecdf(fig, delays_per_var, label=f"{density}")
                d_set = overall_delays.get(density, [])
                d_set += delays_per_var
                overall_delays[int(density)] = d_set

    for density in list(overall_delays.keys()):
        plot_ecdf(fig, overall_delays[density], label=f"{density}")

    plt.xlabel("Update delay (ms)")
    plt.ylabel("ECDF")
    plt.plot([0, 5000], [0.95, 0.95], '-k')
    plt.xlim([0, 3100])
    plt.ylim([0, 1.01])
    plt.legend()
    plt.title(f"PER = {PER}%")

    fig = plt.figure()
    data = []
    #labels = []
    for density in sorted(list(overall_delays.keys())):
        data.append(overall_delays[density])
        f = open(f"summary_results/{density}.csv", "w")
        for i in overall_delays[density]:
            f.write(f"{i}\n")
        f.close()
        #labels.append(density)
    labels = ["Linear", "10 x 2 Grid", "10 x 3 Grid"][:len(list(overall_delays.keys()))]
    fig.gca().boxplot(data, labels=labels, notch=True)
    plt.ylim([0, 3100])
    plt.xlabel("Density")
    plt.title(f"PER = {PER}%")


    x_axis = np.arange(0, 5000, 0.01)
    for density in list(overall_delays.keys()):
        plt.figure()
        avg = np.average(overall_delays[density])
        ci = 1.960 * np.std(overall_delays[density]) / np.sqrt(len(overall_delays[density]))
        print(f"{density}: {avg:.3f} +/- {ci:.3f}")
        plt.hist(overall_delays[density], density=True, bins=np.arange(0, 5000, 10))
        plt.plot(x_axis, st.norm.pdf(x_axis, avg, np.std(overall_delays[density])))
        plt.title(f"{density} - PER = {PER}%")

    x_axis = np.arange(0, 5000, 0.01)

    ds = sorted(list(overall_delays.keys()))
    fig = plt.figure()
    dim_x = int(np.ceil(np.sqrt(len(ds))))
    dim_y = int(np.ceil(len(ds) / dim_x))
    subfigs = fig.subplots(dim_x, dim_y).flatten()

    for i in range(len(ds)):
        density = ds[i]
        ax = subfigs[i]
        avg = np.average(overall_delays[density])
        ci = 1.960 * np.std(overall_delays[density]) / np.sqrt(len(overall_delays[density]))
        print(f"{density}: {avg:.3f} +/- {ci:.3f}")
        ax.hist(overall_delays[density], density=True, bins=np.arange(0, 5000, 10))
        ax.plot(x_axis, st.norm.pdf(x_axis, avg, np.std(overall_delays[density])))
        grid_size = int(density / 10)
        ax.set_title(f"{grid_size}x{10}")
        ax.set_xlim([0, 3100])
        ax.set_ylim([0, 0.0045])
        ax.set_xlabel("Update Delay (ms)")
    plt.subplots_adjust(hspace=0.5, wspace=0.275)
    fig.suptitle(f"Partial Grid - PER = {PER}%")

    plt.show()


if __name__ == "__main__":
    main()

