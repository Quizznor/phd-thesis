from ..testing import create_stream_logger
import logging, colorlog

logger = create_stream_logger("utils.plotting")

def set_plt_style(style : str = 'notebook') -> None : 
    """Change the global plotting style based on performance/look"""
    import scienceplots

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

    logger.debug(f'plotting style set to `{style}`.')
    logger.debug('run set_plt_style() to change style')

# this check is useless
try:
    _ = np.__version__
    _ = pd.__version__
except NameError:
    import numpy as np
    logger.info('import numpy as np')
    import pandas as pd
    logger.info('import pandas as pd')

import matplotlib.pyplot as plt
logger.info('import matplotlib.pyplot as plt')
import seaborn as sns
logger.info('import seaborn as sns')

set_plt_style()

del create_stream_logger
from . import tools as plotting
logger.info('import plotting.tools as plotting')
del logger
del tools