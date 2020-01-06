import sys

new_benchmark_lines = []
with open(sys.argv[1]) as in_file:
    for line in in_file.readlines():
        line = line.split()
        new_line = (line[0], line[1], line[2], line[3], line[4] + ".postPreprocessing.{}".format(sys.argv[3]),
                    line[5], "--fixed-relu=" + line[4] + ".fixed", sys.argv[2])
        new_benchmark_lines.append(" ".join(new_line))

with open(sys.argv[3], 'w') as out_file:
    for line in new_benchmark_lines:
        out_file.write(line + "\n")

