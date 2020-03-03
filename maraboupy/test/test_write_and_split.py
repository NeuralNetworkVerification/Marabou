
from MarabouNetworkNNetExtended import *
from  MarabouNetworkNNet import *

from MarabouNetworkNNetExtentions import *

#import filecmp
from subprocess import call



property_filename = "../resources/properties/acas_property_4.txt"
network_filename = "../resources/nnet/acasxu/ACASXU_experimental_v2a_1_9.nnet"

layer = 2

nnet_object = MarabouNetworkNNetExtended(filename=network_filename,property_filename=property_filename)

# Debug
# print("1:")
# print(nnet_object.inputRanges)
# print(nnet_object.inputMeans)


nnet_object1, nnet_object2 = splitNNet(marabou_nnet=nnet_object,layer=layer)

# Debug
# print("2:")
# print(nnet_object.inputRanges)
# print(nnet_object.inputMeans)



#TESTING THE SPLIT FUNCTIONALITY AND DIFFERENT METHODS OF EVALUATION. WORKS!

N = 10

for i in range(N):
        inputs = createRandomInputsForNetwork(nnet_object)

        layer_output = nnet_object.evaluateNetworkToLayer(inputs, last_layer=layer, normalize_inputs=False, normalize_outputs=False, activate_output_layer=True)
        output1 = nnet_object1.evaluateNetworkToLayer(inputs,last_layer=0, normalize_inputs=False, normalize_outputs=False, activate_output_layer=True
                                                    )

        if not (layer_output == output1).all():
               print("Failed1")

        true_output = nnet_object.evaluateNetworkToLayer(inputs, last_layer=0, normalize_inputs=False, normalize_outputs=False)
        output2 = nnet_object2.evaluateNetworkToLayer(layer_output,last_layer=0, normalize_inputs=False, normalize_outputs=False)
        output2b = nnet_object.evaluateNetworkFromLayer(layer_output,first_layer=layer)
        true_outputb = nnet_object.evaluateNetworkFromLayer(inputs)
        true_outputc = nnet_object.evaluateNetwork(inputs,normalize_inputs=False,normalize_outputs=False)
        # true_outputd = nnet_object.evaluateWithMarabou(inputs) #Failed, requires a different type of input!

        # Debug
        # print(i, "   ", inputs, "   ", "\n", true_output, "\n", output2, "\n", true_output == output2)
        if not (true_outputb == output2b).all():
               print("Failed2")

        if not (true_outputc == output2b).all():
               print("Failed2")



# TESTING WRITE TO FILE


output_filename = "test/ACASXU_experimental_v2a_1_9_output.nnet"
output_filename1 = "test/ACASXU_experimental_v2a_1_9_output1.nnet"
output_filename2 = "test/ACASXU_experimental_v2a_1_9_output2.nnet"

nnet_object.writeNNet(output_filename)

#print(filecmp.cmp(output_filename,network_filename))

call(['diff',output_filename,network_filename])

nnet_object1.writeNNet(output_filename1)
nnet_object2.writeNNet(output_filename2)

nnet_object_a = MarabouNetworkNNetExtended(filename=output_filename,property_filename=property_filename)
nnet_object1_a = MarabouNetworkNNetExtended(filename=output_filename1)
nnet_object2_a = MarabouNetworkNNetExtended(filename=output_filename2)



# COMPARING RESULTS OF THE NETWORKS CREATED FROM NEW FILES TO THE ORIGINALS ONES. WORKS!

for i in range(N):
        inputs = createRandomInputsForNetwork(nnet_object_a)

        layer_output = nnet_object_a.evaluateNetworkToLayer(inputs, last_layer=layer, normalize_inputs=False, normalize_outputs=False, activate_output_layer=True)
        output1 = nnet_object1_a.evaluateNetworkToLayer(inputs,last_layer=0, normalize_inputs=False, normalize_outputs=False, activate_output_layer=True
                                                    )

        if not (layer_output == output1).all():
               print("Failed1")

        true_output = nnet_object_a.evaluateNetworkToLayer(inputs, last_layer=0, normalize_inputs=False, normalize_outputs=False)
        output2 = nnet_object2_a.evaluateNetworkToLayer(layer_output,last_layer=0, normalize_inputs=False, normalize_outputs=False)
        output2b = nnet_object_a.evaluateNetworkFromLayer(layer_output,first_layer=layer)
        true_outputb = nnet_object_a.evaluateNetworkFromLayer(inputs)
        true_outputc = nnet_object_a.evaluateNetwork(inputs,normalize_inputs=False,normalize_outputs=False)
        true_outputd = nnet_object.evaluateNetwork(inputs,normalize_inputs=False,normalize_outputs=False)
        # true_outputd = nnet_object.evaluateWithMarabou(inputs) #Failed, expects a different type of input!

        # Debug
        # print(i, "   ", inputs, "   ", "\n", true_output, "\n", output2, "\n", true_output == output2)
        if not (true_outputb == output2b).all():
               print("Failed2")

        if not (true_outputd == true_outputc).all():
               print("Failed2")

