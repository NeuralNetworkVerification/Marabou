import sys
import os
import math

#nnets/acas/ACASXU_run2a_2_9_batch_2000.nneline-split=1 --to-factor=1.5 --to-sub=5 --initial-splits=0 -w 12"

arguments = []

for filename in os.listdir(sys.argv[1]):
    tmp_address= "/home/haozewu/Projects/NN/Marabou/evaluation/scalability/mini-example/tmp/Marabou/{}_nodes/property4/".format(sys.argv[3])
    if not os.path.exists(tmp_address):
        os.makedirs(tmp_address)
    if ".nnet" in filename:
        arg = sys.argv[1] + filename.split(".")[0] + " "
        arg += sys.argv[2] + " "
        arg += tmp_address +  filename + "-" +  sys.argv[2].split("/")[-1].split(".")[0] + "-summary-{}.txt ".format(sys.argv[3])

        nw = int(sys.argv[3])
        to = 5
        num_splits = int(math.log2(nw))
        if num_splits % 2 == 0:
            for i in range(int(num_splits / 2)):
                to = int(to * 1.5)
        else:
            to = 6
            for i in range(int((num_splits - 1)/2)):
                to = int(to * 1.5)
        arg += '"-w {} --initial-splits={} --to-sub={}"\n'.format(nw, int(math.log2(nw)), to)
        arguments.append(arg)
with open(sys.argv[4], 'w') as out_file:
    for line in arguments:
        out_file.write(line)
