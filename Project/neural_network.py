from matplotlib import pyplot as plt
import numpy as np
import pandas as pd
from sklearn.preprocessing import LabelEncoder
import seaborn as sns
from sklearn.neural_network import MLPRegressor
from sklearn.metrics import accuracy_score, confusion_matrix, mean_squared_error
from sklearn.model_selection import train_test_split
from matplotlib.backends.backend_pdf import PdfPages

source = "amplitude"
le = LabelEncoder()
features = pd.read_csv(source + "_input.csv", sep=",").to_numpy()
labels = pd.read_csv(source + "_label.csv", sep=",").to_numpy()
labels = pd.get_dummies(labels.ravel())

X_train, X_validate, y_train, y_validate = train_test_split(features, labels, \
        test_size=0.2, random_state=1337)

reg = MLPRegressor(hidden_layer_sizes=[20]*5, max_iter=100, random_state=42)
reg.fit(X_train, y_train)

y_prediction = reg.predict(X_validate)

vowels = ["a", "e", "i", "o", "u"]
vomap = {i : vowels[i] for i in range(5)}

y_prediction = pd.DataFrame(y_prediction).idxmax(axis="columns").map(vomap).ravel()
y_validate = pd.DataFrame(y_validate).idxmax(axis="columns").ravel()

ax = plt.subplot()
cmatrix = confusion_matrix(y_validate, y_prediction)
sns.heatmap(cmatrix, annot=True, fmt='g', ax=ax)

vowels = ["a", "e", "i", "o", "u"]

ax.set_xlabel('prediction')
ax.set_ylabel('actual')
ax.xaxis.set_ticklabels(vowels)
ax.yaxis.set_ticklabels(vowels)

accuracy = accuracy_score(y_validate, y_prediction)
print(f"Prediction accuracy: {100*accuracy:.2f}%")

# plt.show()

with PdfPages(r"NN_confusion.pdf") as f:
    f.savefig()

# Output the test error
print("The test error is ", mean_squared_error(le.fit_transform(y_validate.ravel()), le.fit_transform(y_prediction.ravel())))