import matplotlib.pyplot as plt
import numpy as np

file=f"/cr/tempdata01/filip/SSDCalib/RawNoCut/randoms0800_SSD.dat"
SSD = np.loadtxt(file)
print(SSD.shape)


for trace in SSD:
    trace = trace[1:] - trace[0]
    break

plt.plot(range(2048), trace)
plt.show()