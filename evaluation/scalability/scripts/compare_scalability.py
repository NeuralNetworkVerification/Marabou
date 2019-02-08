import sys
import os
import csv
from parse_result import *
import numpy as np
import matplotlib.pyplot as plt

def main():

    marabou_path = "Marabou/"
    reluval_path = "ReluVal/"
    cwd = os.getcwd()
    summaries_m = []
    for node in os.listdir(marabou_path):
        for prop in os.listdir("{}{}/".format(marabou_path, node)):
            summaries_m += get_summary_marabou("{}/{}{}/{}/".format(cwd, marabou_path, node, prop))
    summaries_r = []
    for node in os.listdir(reluval_path):
        for prop in os.listdir("{}{}/".format(reluval_path, node)):
            summaries_r += get_summary_reluval("{}/{}{}/{}/".format(cwd, reluval_path, node, prop))

    m_errors = 0
    m_solved = 0
    m_to = 0

    cpu_to_rt_ms = [{}, {}, {}, {}]
    for i in range(7)[1:]:
        for j in range(4):
            cpu_to_rt_ms[j][int(2**i)] = []
    for line in summaries_m:
        prop = int(line[2][-1])
        state = line[4]
        if state == "TO" or state == "Ok":
            cpu_to_rt_ms[prop - 1][int(line[3])].append(float(line[5]))
        if state == "Ok":
            m_solved += 1
        elif state == "TO":
            m_to += 1
        else:
            m_errors += 1
            print (line)

    r_errors = 0
    r_solved = 0
    r_to = 0

    cpu_to_rt_rs = [{}, {}, {}, {}]
    for i in range(7)[1:]:
        for j in range(4):
            cpu_to_rt_rs[j][int(2**i)] = []
    for line in summaries_r:
        prop = int(line[2][-1])
        state = line[4]
        if state == "TO" or state == "Ok":
            cpu_to_rt_rs[prop - 1][int(line[3])].append(float(line[5]))
        if  state == "Ok":
            r_solved += 1
        elif state == "TO":
            r_to += 1
        else:
            r_errors += 1
            print (line)

    print ("Total number of solved: {} {}".format(m_solved, r_solved))
    print ("Total number of TOs: {} {}".format(m_to, r_to))
    print ("Total number of Errors: {} {}".format(m_errors, r_errors))

    color = ["red", "blue", "green", "purple"]

    for i in range(4):
        x_axi_text = []
        m_scale = []
        r_scale = []
        empty = True
        for j in range(7)[1:]:
            x_axi_text.append("{} cpus".format(2**j))
            if len(cpu_to_rt_ms[i][int(2**j)]) > 0:
                empty = False
                m_scale.append(np.mean(cpu_to_rt_ms[i][int(2**j)]))
            if len(cpu_to_rt_rs[i][int(2**j)]) > 0:
                empty = False
                r_scale.append(np.mean(cpu_to_rt_rs[i][int(2**j)]))
        x = np.arange(1, len(x_axi_text) + 1, 1)
        if empty:
            continue
        plt.plot(x, m_scale, c=color[i], label="Property {} Marabou".format(i + 1))
        plt.plot(x, r_scale, c=color[i], ls="--", label="Property {} ReluVal".format(i + 1))
        plt.legend(loc="upper right")
    plt.title("Average runtime (secs) of Marabou and \nReluVal vs. Number of allocated CPUs")
    plt.xticks(x, x_axi_text)
    plt.show()

if __name__ == "__main__":
    main()
