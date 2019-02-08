import sys
import os

arguments = []

for filename in os.listdir(sys.argv[1]):
    tmp_address= "/home/cav/marabou/evaluation/scalability/sample_experiment/tmp/ReluVal/{}_nodes/property4/".format(sys.argv[3])
    if not os.path.exists(tmp_address):
        os.makedirs(tmp_address)
    if ".nnet" in filename:
        arg = sys.argv[3] + " " + sys.argv[2][-5] + " "
        arg += sys.argv[1] + filename + " "
        arg += tmp_address +  filename + "-" +  sys.argv[2].split("/")[-1].split(".")[0] + "-summary-{}.txt\n".format(sys.argv[3])

        arguments.append(arg)
with open(sys.argv[4], 'w') as out_file:
    for line in arguments:
        out_file.write(line)
