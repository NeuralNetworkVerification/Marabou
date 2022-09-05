To run the robustness check on the runner-up class with eps 0.01 of the 0th
test image on the given network:
   
   ./resources/runMarabou.py resources/onnx/attention-networks/self-attention-small-sim.onnx --marabou build_grb/Marabou -e 0.01 --dataset=mnist -i 0 -s=result_ind0_eps0.01.txt

The result is either unsat (i.e., hold) or UNKNOWN

The script will print out the output of the network on the original image
as well as the output bounds derived by DeepPoly on the given epsilon ball.
It will write the result to result_ind0_eps0.01.txt

