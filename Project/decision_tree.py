import numpy as np
import pandas as pd
import pydotplus
from IPython.display import display
from matplotlib import pyplot as plt
import seaborn as sns
from sklearn.tree import export_text, plot_tree, DecisionTreeClassifier, export_graphviz
from sklearn.model_selection import train_test_split
from sklearn.metrics import mean_squared_error, confusion_matrix, accuracy_score
from sklearn.preprocessing import LabelEncoder
from matplotlib.backends.backend_pdf import PdfPages

source = "amplitude"
depth = np.array(range(1, 21))

features = pd.read_csv(source + "_input.csv", sep=",").to_numpy()
labels = pd.read_csv(source + "_label.csv", sep=",").to_numpy()

X_train, X_val, y_train, y_val = train_test_split(features, labels, test_size=0.2, random_state=2)
train_errors = []
val_errors = []

# Encoding labels to compute error and choose best depth
y_val_enc = y_val.ravel()
y_train_enc = y_train.ravel()
le = LabelEncoder()
y_val_enc = le.fit_transform(y_val_enc)
y_train_enc = le.fit_transform(y_train_enc)

# iteration decision tree for different depth and computing training and validation error
for i in depth:
    clf = DecisionTreeClassifier(random_state=0, max_depth=i)
    clf.fit(X_train, y_train_enc)
    y_pred_train = clf.predict(X_train)
    dec_train_error = mean_squared_error(y_train_enc, y_pred_train)
    y_pred_val = clf.predict(X_val)
    dec_val_error = mean_squared_error(y_val_enc, y_pred_val)
    train_errors.append(dec_train_error)
    val_errors.append(dec_val_error)

# create a table to compare the training and validation errors
errors = {"depth": depth,
          "train_errors": train_errors,
          "val_errors": val_errors,
          }
display(pd.DataFrame(errors))

# choosing the depth with minimum validation error
index_min_val_error = np.argmin(val_errors)
print("Choosing depth ", depth[index_min_val_error], " gives us minimum validation error with error = ",
      np.amin(val_errors),
      end='\n')

best_depth = depth[index_min_val_error]
# Fitting a classifier to the training set using a decision tree with depth = best_depth
clf = DecisionTreeClassifier(random_state=0, max_depth=best_depth)
clf.fit(X_train, y_train)
y_pred = clf.predict(X_val)
print("The accuracy of the predicted classes", clf.score(X_val, y_pred), end='\n')

# Compute the test accuracy
acc = accuracy_score(y_val, y_pred)
print("Test Accuracy:", acc, end='\n')

# plot the confusion matrix
confmat = confusion_matrix(y_val, y_pred)
ax = plt.subplot()
sns.heatmap(confmat, annot=True, fmt='g', ax=ax)
ax.set_xlabel('prediction')
ax.set_ylabel('actual')
# plt.show()
with PdfPages(r"decision_tree_confmat.pdf") as f:
    f.savefig()

# save as pdf for a high res image:
d_tree = export_graphviz(clf, feature_names=range(60, 6001, 60), filled=True, class_names=["a", "e", "i", "o", "u"])
pydot_graph = pydotplus.graph_from_dot_data(d_tree)
pydot_graph.write_pdf('vowels_tree.pdf')
