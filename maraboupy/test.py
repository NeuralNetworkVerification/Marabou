import sys, os
sys.path.append(os.path.join(os.path.dirname('.')))
import maraboupy.Marabou as Marabou
import maraboupy.MarabouCore as Core
network = Marabou.read_onnx("/home/matthew/Code/AISEC/Marabou/resources/onnx/oneInput_twoBranches.onnx")

