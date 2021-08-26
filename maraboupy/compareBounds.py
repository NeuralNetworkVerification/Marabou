import argparse
import matplotlib.pyplot as plt
import json
import os

parser = argparse.ArgumentParser(description='Run MNIST based verification scheme using abstraction')
parser.add_argument("--fx", type=str, help="X axis bound file")
parser.add_argument("--fy", type=str, help="Y axis bound file")
args = parser.parse_args()

with open(args.fx, "r") as f:
    fx = json.load(f)
with open(args.fy, "r") as f:
    fy = json.load(f)

xInt = {var['variable'] : var['upper'] - var['lower'] for var in fx}
yInt = {var['variable'] : var['upper'] - var['lower'] for var in fy}
mutual = [x for x in xInt.keys() if x in yInt]

xIntMutual = [xInt[k] for k in mutual]
yIntMutual = [yInt[k] for k in mutual]

ratio = [yInt[k] / xInt[k] if xInt[k] != 0 else (100 if yInt[k] != 0 else 1) for k in mutual]

graphDir = 'fx={}_fy={}'.format(args.fx.replace('.json','').replace('/','___'), args.fy.replace('.json','').replace('/','___'))
if not os.path.exists(graphDir):
    os.mkdir(graphDir)
os.chdir(graphDir)

plt.scatter(xIntMutual, yIntMutual)
figure = plt.gcf()
figure.set_size_inches(12,9)
plt.savefig("boundCompare.png", dpi=100)

plt.figure()
plt.scatter(list(range(len(ratio))), ratio)
figure = plt.gcf()
figure.set_size_inches(12,9)
plt.savefig("boundRatio.png", dpi=100)
plt.close()
