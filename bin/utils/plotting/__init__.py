import matplotlib.pyplot as plt
import seaborn as sns
import scienceplots

plt.style.use(['science', 'ieee'])

plt.rcParams['text.usetex'] = True
plt.rcParams['text.latex.preamble'] = \
    r'\usepackage{lipsum}' \
    + r'\usepackage{amsmath}' \
    + r'\usepackage{upgreek}' \
    + r'\usepackage{siunitx}'
