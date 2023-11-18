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

def setup() -> None:
    
    global plt, sns

    mpl = __import__('matplotlib.pyplot')
    sns = __import__('seaborn')

    plt = mpl.pyplot

    print('import matplotlib.pyplot as plt                          # Comprehensive library for creating beautiful visualizations')
    print('import seaborn as sns                                    # Statistical data visualization library based on matplotlib')
    print()