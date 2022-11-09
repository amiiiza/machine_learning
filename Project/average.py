from matplotlib import pyplot as plt
import numpy as np
import pandas as pd
import random
import math
from matplotlib.backends.backend_pdf import PdfPages

source = "training_energy"

input_data = pd.read_csv(source + "_input.csv", sep=",")
label_data = pd.read_csv(source + "_label.csv", sep=",")

all_data = pd.concat([input_data, label_data], axis=1)
all_data.columns = ["f"+str(i) for i in range(100)] + ["vowel"]

vowels = ["a", "e", "i", "o", "u"]
data = [all_data[all_data["vowel"] == i] for i in vowels]

x = [float(x)*60 for x in range(1, 101)]

fig = plt.figure()
ax = fig.add_subplot()

for i in range(5):

    [inputs, labels] = np.split(data[i], [100], axis=1)
    
    y = np.mean(inputs.to_numpy(), axis=0)*x*x
    y = np.log10(y.tolist())*10

    ax.plot(x, y)

ax.set_ylabel("dB")
ax.set_xlabel("Hz")
ax.legend(vowels)

# plt.show()

with PdfPages(r"energy_distribution.pdf") as f:
    f.savefig()

