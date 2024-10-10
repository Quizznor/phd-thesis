from scipy.signal import argrelextrema
from scipy.optimize import curve_fit
from ...binaries import np, uncertainties
from ...plotting import plt
from ... import CONSTANTS

class Histo:

    def __init__(self, counts: list[np.ndarray]) -> None:

        assert len(counts) == 4, 'Missing a PMT?'

        self.wcds = counts[:-1]
        self.ssd = counts[-1]

        if len(self.ssd) == 150:
            self.is_charge, self.is_peak = False, True
            self.bins = [CONSTANTS.UUB_WCD_PEAK, CONSTANTS.UUB_SSD_PEAK]
        else:
            self.is_charge, self.is_peak = True, False
            self.bins = [CONSTANTS.UUB_WCD_CHARGE, CONSTANTS.UUB_SSD_CHARGE]

        self.popts = []


    def get_peak(self) -> list[uncertainties.ufloat]:
        if not self.is_peak: raise ValueError("getting Peak of a Charge histogram")
        
        peaks = []
        for i, counts in enumerate([*self.wcds, self.ssd]):

            try:
                ADC = self.bins[0 if i<3 else 1]
                __peak = argrelextrema(counts[:99], np.greater, order=99)[0][0]
                x1, x2, y1, y2 = __peak, 99, counts[__peak], counts[99]

                slope, offset = -(np.log(y1)-np.log(y2))/(x2-x1), np.log(y1)
                exp_fit = lambda x: np.exp(slope*(x-x1)+offset)

                guess = np.argmax(counts[__peak:99] - exp_fit(range(__peak, 99)))
                start, stop = guess - 6, guess + 25
        
                (curve, peak, height), pcov = curve_fit(parabola, ADC[start:stop], counts[start:stop] - exp_fit(range(start, stop)),
                                                bounds=([-np.inf, 0, 0],[0, np.inf, np.inf]),
                                                maxfev=10000,
                                                p0=[-1, guess, counts[guess]])

                self.popts.append([slope, x1, offset, curve, peak, height])
                (_, peak, _) = uncertainties.correlated_values([curve, peak, height], pcov)
                peaks.append(peak)

            except Exception as e:
                print(f"PMT {i} fit failed: {e}")
                self.popts.append(None)
                peaks.append(uncertainties.ufloat(np.nan, np.nan))

        return peaks


    def get_charge(self) -> list[float]:
        if not self.is_charge: raise ValueError("getting Charge of Peak histogram")

        charges = []
        for i, counts in enumerate([*self.wcds, self.ssd]):

            try:
                ADC = self.bins[0 if i<3 else 1]
                __peak = argrelextrema(counts[:399], np.greater, order=399)[0][0]
                x1, x2, y1, y2 = __peak, 399, counts[__peak], counts[399]

                slope, offset = -(np.log(y1)-np.log(y2))/(x2-x1), np.log(y1)
                exp_fit = lambda x: np.exp(slope*(x-x1)+offset)

                guess = np.argmax(counts[__peak:399] - exp_fit(range(__peak, 399)))
                start, stop = guess - 24, guess + 100
        
                (curve, charge, height), pcov = curve_fit(parabola, ADC[start:stop], counts[start:stop] - exp_fit(range(start, stop)),
                                                bounds=([-np.inf, 0, 0],[0, np.inf, np.inf]),
                                                maxfev=10000,
                                                p0=[-1, guess, counts[guess]])

                self.popts.append([slope, x1, offset, curve, charge, height])
                (_, charge, _) = uncertainties.correlated_values([curve, charge, height], pcov)
                charges.append(charge)

            except Exception as e:
                print(f"PMT {i} fit failed: {e}")
                self.popts.append(None)
                charges.append(uncertainties.ufloat(np.nan, np.nan))

        return charges


    def plot(self, ax=None, **kwargs):
        
        if ax is None:
            axes = [plt.gca() for _ in range(4)]
        elif isinstance(ax, plt.Axes):
            axes = [ax for _ in range(4)]
        else: axes = ax

        colors = ['red', 'blue', 'mediumturquoise', 'k']
        for i, (ax, c) in enumerate(zip(axes, colors)):

            if i < 3:
                bins = self.bins[0]
                counts = self.wcds[i]
            else:
                bins = self.bins[1]
                counts = self.ssd

            ax.plot(bins, counts, c=c, ls='solid')
            if self.popts[i] is not None:
                slope, x1, offset, curve, peak, height = self.popts[i]
                exp_fit = lambda x: np.exp(slope*(x-x1)+offset)
                X = np.linspace(0.7 * peak, 1.3 * peak, 100)
                ax.plot(X, exp_fit(X) + parabola(X, curve, peak, height), c=c, ls='--')



    

def parabola(x, scale, mip, y0):
    return scale * (x-mip)**2 + y0
