import sys
import os
import subprocess

# $1 = input net
# $2 = input property
# $3 = script dir
# $4 = output netname
# $5 = output property

def main():
    input_nnet_name = sys.argv[1].split("/")[-1][:-5]
    subprocess.call(["cp", sys.argv[3] + "/networks/{}.nnet".format(input_nnet_name),
                    sys.argv[4] + ".nnet"])
    subprocess.call(["cp", sys.argv[3] + "/networks/{}.pb".format(input_nnet_name),
                    sys.argv[4] + ".pb"])
    subprocess.call(["cp", sys.argv[2], sys.argv[5]])

if __name__ == "__main__":
    main()
