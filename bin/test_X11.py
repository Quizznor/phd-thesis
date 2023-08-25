import matplotlib.pyplot as plt
import numpy as np

X = np.random.normal(0, 1, 1000)

plt.hist(X)
plt.show()