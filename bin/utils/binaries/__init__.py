def setup():
    
    global np, pd

    np = __import__('numpy')
    pd = __import__('pandas')

    print('import numpy as np                                       # The fundamental package for scientific computing with Python')
    print('import pandas as pd                                      # Easy to use open source data analysis and manipulation tool')
    print()