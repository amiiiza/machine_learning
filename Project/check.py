from matplotlib import pyplot as plt
import numpy as np
import random
import math

source = "amplitude"

inputf = open(source + "_input.csv").readlines()
labels = open(source + "_label.csv").readlines()
inputs = [[float(x) for x in y.split(",")] for y in inputf]
indices = [x for x in range(len(inputs))]

samples = random.sample(indices, k=5)
x = [float(x)*60 for x in range(1, 101)]
X = np.array(x)

for i in samples:

    plt.plot(X, inputs[i])
    plt.title(labels[i])
    plt.show()

