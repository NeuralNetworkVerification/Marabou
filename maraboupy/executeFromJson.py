import json
import subprocess
import os
import argparse

parser = argparse.ArgumentParser(description='Executre list of commands from JSON file.')
parser.add_argument("--file", type=str, default="", help="File containing commands", required=True)
args = parser.parse_args()

with open(os.path.abspath(args.file), 'r') as f:
    commands = json.load(f)

for cmd in commands:
    print("-------------------- Started executing {}".format(cmd))
    subprocess.run(cmd.split(' '))
    print("-------------------- Finished executing {}".format(cmd))


print("-------------------- -------------------- -------------------- --------------------")
print("-------------------- Finished executing all commands --------------------")
