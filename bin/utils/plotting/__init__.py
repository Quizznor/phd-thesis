import matplotlib.pyplot as plt
import seaborn as sns
import scienceplots
import pandas as pd
import numpy as np

def set_plt_style(style : str = 'notebook') -> None : 
    
    if style == 'script':
        plt.style.use(['science', 'ieee'])
        plt.rcParams['text.usetex'] = True
        plt.rcParams['text.latex.preamble'] = \
            r'\usepackage{lipsum}' \
            + r'\usepackage{amsmath}' \
            + r'\usepackage{upgreek}' \
            + r'\usepackage{siunitx}'
    elif style == 'notebook':
        plt.style.use(['science', 'ieee', 'no-latex', 'high-vis'])

    print(f'style has been set to {style}-style. You can execute `set_plt_style(<style>)` to change this.')

print('from utils.plotting import matplotlib.pyplot as plt      # Comprehensive library for creating beautiful visualizations')
print('from utils.plotting import import seaborn as sns         # Statistical data visualization library based on matplotlib')
print('from utils.plotting import import numpy as np            # The fundamental package for scientific computing with Python')
print('from utils.plotting import pandas as pd                  # Easy to use open source data analysis and manipulation tool')

set_plt_style()

from . import tools as plotting