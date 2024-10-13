from scipy.signal import argrelextrema
from scipy.optimize import curve_fit
from utils.binaries import np, uncertainties
from utils.plotting import plt
from utils import CONSTANTS


class Histo:

    def __init__(self, *, peak: list[np.ndarray]=None, charge: list[np.ndarray]=None) -> None:

        assert len(peak) == len(charge) == 4, 'Missing a PMT?'
        
        self.histos = {
            'peak' : peak,
            'charge' : charge
        }
        self.popts = {
            'peak': [[np.nan, np.nan] for _ in range(4)], 
            'charge': [[np.nan, np.nan] for _ in range(4)]
        }


    def fit(self) -> dict:

        if self.histos['peak'] is not None: 
            self.popts['peak'] = self.get_peak('peak')
        if self.histos['charge'] is not None:
            self.popts['charge'] = self.get_peak('charge')

        return self.popts


    def get_peak(self, mode: str) -> list[uncertainties.ufloat]:

        peaks = []
        for i, counts in enumerate(self.histos[mode]):

            bin_change = 99 if len(counts) == 150 else 399
            start, stop, peak = self.range_finder(counts[:bin_change], i, mode)
            bins = self.get_bins(mode, i)

            try:
                
                x1, x2, y1, y2 = start, bin_change, counts[stop], counts[bin_change]
                slope, offset = -(np.log(y1)-np.log(y2))/(x2-x1), np.log(y1)
                exp_fit = lambda x: np.exp(slope*(x-x1)+offset)
                exp_background = exp_fit(range(start, stop))
        
                popts, pcov = curve_fit(self.parabola, bins[start:stop], 
                                        counts[start:stop] - exp_background,
                                        bounds=([-np.inf, 0, 0],[0, np.inf, np.inf]),
                                        maxfev=10000, p0=[-0.01, bins[peak], counts[peak]],
                                        nan_policy='omit')
                
                # popts, pcov = curve_fit(self.exp_hump, bins[start:stop], counts[start:stop],
                #                         bounds=([0, 0, 0, 0, 0], [np.inf, np.inf, np.inf, np.inf, np.inf]),
                #                         maxfev=10000, p0=[5e3, 2e2, 0.1, 200 if mode=='peak' else 2000, 0.1]
                # )

                popts = uncertainties.correlated_values(popts, pcov)
                peaks.append(popts)

            except Exception as e:
                print(f"PMT {i} fit failed: {e}")
                peaks.append([uncertainties.ufloat(np.nan, np.nan) for _ in range(3)])
                # raise e

            self.popts[mode][i] = peaks
        
        return peaks
    

    def plot(self) -> plt.Figure:
        
        if self.histos['peak'] is not None and self.histos['charge'] is not None:
            fig, (ax1, ax2) = plt.subplots(1, 2)
        else:
            fig, ax = plt.subplots()
            ax1 = ax2 = ax

        f = 4
        c = ['red', 'blue', 'mediumturquoise', 'k']
        l = ['WCD1', 'WCD2', 'WCD3', rf'SSD $\times$ {f}']
        
        if self.histos['peak'] is not None:
            ax1.set_xlabel('max. pulse height / ADC')
            for i, counts in enumerate(self.histos['peak']):
                factor = 1 if i<3 else f
                ax1.plot(self.get_bins('peak', i) * factor, counts, c=c[i], ls='-', label=l[i])
                ax1.axvline(self.popts['peak'][i][1].n * factor, ymin=0.95, c=c[i])
            
            ax1.set_xlim(0, 400)
            ax1.legend(title='Peak')

        if self.histos['charge'] is not None:
            ax2.set_xlabel('integral / ADC')
            for i, counts in enumerate(self.histos['charge']):
                factor = 1 if i<3 else f
                ax2.plot(self.get_bins('charge', i) * factor, counts, c=c[i], ls='-', label=l[i])
                ax2.axvline(self.popts['charge'][i][1].n * factor, ymin=0.95, c=c[i])
            
            ax2.set_xlim(0, 3200)
            ax2.legend(title='Charge')

        return fig
    

    @staticmethod
    def range_finder(counts: np.ndarray, pmt: int, mode: str) -> tuple[int]:
        
        if pmt<3:
            peak = np.argmax(counts)
            x1, x2, y1, y2 = peak, len(counts), counts[peak], counts[len(counts)-1]

            slope, offset = -(np.log(y1)-np.log(y2))/(x2-x1), np.log(y1)
            exp_fit = lambda x: np.exp(slope*(x-x1)+offset)

            guess = np.argmax(counts[peak:] - exp_fit(range(peak, len(counts))))
            start = guess - (6 if mode=='peak' else 18)
            stop = guess + (25 if mode=='peak' else 100)

        else:
            peak = np.argmax(counts)
            skip = 7 if mode=='peak' else 18
            guess = peak + skip + np.argmax(counts[peak + skip:])
            start = guess - (5 if mode=='peak' else 18)
            stop = guess + (18 if mode=='peak' else 40)

        return start, stop, guess


    @staticmethod
    def parabola(x, scale, mip, y0):
        return scale * (x-mip)**2 + y0

    @staticmethod
    def exp_hump(x, A, B, tau, hump, scale) -> np.ndarray:
        return A * np.exp(-tau*x) + B * np.exp(-(x-hump)**2/(2*scale**2))

    @staticmethod
    def get_bins(mode: str, pmt: int) -> np.ndarray:

        if mode == 'peak' and pmt < 3:
            return CONSTANTS.UUB_WCD_PEAK
        elif mode == 'peak' and pmt == 3:
            return CONSTANTS.UUB_SSD_PEAK
        elif mode == 'charge' and pmt < 3:
            return CONSTANTS.UUB_WCD_CHARGE
        elif mode == 'charge' and pmt == 3:
            return CONSTANTS.UUB_SSD_CHARGE
