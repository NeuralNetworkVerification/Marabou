import os
import sys

summary_string = []

summary_folder_name = sys.argv[1]
summary_file_dir = sys.argv[2]
timeout = float(sys.argv[3])

for filename in os.listdir(summary_folder_name):
    with open(sys.argv[1] + filename, 'r') as in_file:
        for line in in_file.readlines():
            if line.split()[0] == "UNKNOWN" or float(line.split()[1]) >= timeout :
                continue
            summary_string.append("{} {} {}".format(filename.split('.')[0], line.split()[0], int(float(line.split()[1]))))

with open(summary_file_dir + "/".join(summary_folder_name.split('/')[:-1]) + ".txt", 'w') as out_file:
    for line in summary_string:
        out_file.write(line + "\n")
