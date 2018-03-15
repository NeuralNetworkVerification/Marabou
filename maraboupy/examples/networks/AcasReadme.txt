This folder contains the 45 networks that define the most recent version of the ACAS Xu neural networks. These networks were trained to represent different discrete combinations of state variables tau and previous RA (pra). Tau has 9 discrete values while pra has 5:

Tau: [0, 1, 5, 10, 20, 40, 60, 80, 100]
Pra: [1, 2, 3, 4, 5]

In previous versions of the ACAS Xu networks, the network names were marked with "1_1" .... "5_9" to reprent the different pra and tau combination indices. Now, the networks names are more descriptive and specify the actual tau and pra combination used to train that network.



These networks have five inputs and produce five outputs, but the training data was normalized by subtracting by the mean and dividing by the range. Therefore, when passing inputs, use the following normalization constants:

        Input_1  Input_2        Input_3        Input_4     Input_5
Means:  21770.2, 0.0,           0.0,           468.777778, 420.0  
Ranges: 66287.1, 6.28318530718, 6.28318530718, 1100.0,     1200.0



In addition, the target data was normalized for all networks using a single normalization constant for all outputs. To get the unnormalized network outputs, multiply by the output range and then add the output mean:

       Output
Mean:  8.2091785953
Range: 399.086622997



The inputs to the network should never go outside the bounds of the training data. Unnormalized, those bounds are:

       Input_1   Input_2    Input_3   Input_4  Input_5
Mins:  548.9,   -3.141593, -3.141593, 100.0,   0.0
Maxes: 66836.0,  3.141593,  3.141593, 1200.0,  1200.0

For convenience, the normalized bounds are:

       Input_1     Input_2  Input_3  Input_4     Input_5
Mins:  -0.320142, -0.5,    -0.5,    -0.3352525, -0.35
Maxes:  0.679858,  0.5,     0.5,     0.6647475,  0.65



Lastly, an additional cost of 15.0 (unnormalized) is added to the first network output (Clear-of-Conflict) when the previous RA is not COC (networks with pra > 1), and the distance between the two aircraft at closest point of approach (CPA) is less than 4000 ft. The distance at CPA (dCPA) can be calculated as a function of the unnormalized input variables:

r     = input_1
theta = input_2
psi   = input_3
v_own = input_4
v_int = input_5

dx   = v_int*cos(psi) - v_own 
dy   = v_int*sin(psi) 
tCPA = (r*cos(theta)*dx - r*sin(theta)*dy)/(dx^2 + dy^2) 
xCPA = r*cos(theta) + dx*tCPA 
yCPA = r*sin(theta) + dy*tCPA 
dCPA = sqrt(xCPA^2 + yCPA^2)


This additional cost was present in the training data, but represents a large discontinuity that the neural network regressed poorly. The solution was to remove the cost from the training data and then re-apply the cost to the network outputs. This resulted in neural networks that trained more accurately, but now an additional cost must be added to the network outputs in some cases.