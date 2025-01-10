from utils.binaries import *
from utils.Auger.SD import SdHisto

from datetime import date, timedelta
from pathlib import Path

start_dt = date(2025, 1, 1)
end_dt = date(2025, 1, 5)

def daterange(date1, date2):
    for n in range(int ((date2 - date1).days)+1):
        yield date1 + timedelta(n)

for d in daterange(start_dt, end_dt):
    year, month, day = d.year, d.month, d.day

    # TODO: get moni mip peak
    # TODO: get histo mip peak
    # get diff
    # save to file