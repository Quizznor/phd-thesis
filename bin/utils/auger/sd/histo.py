__all__ = ['SdHisto', 'read_histos']

from ...binaries import uncertainties, np
from scipy.signal import argrelextrema
from scipy.optimize import curve_fit
from ...plotting import plt
from ... import CONSTANTS

class SdHisto:
    def __init__(
        self,
        *,
        peak: list[np.ndarray] = None,
        charge: list[np.ndarray] = None,
        pmt_mask: list = [1, 1, 1, 1],
    ) -> None:

        assert peak is None or len(peak) == 4, "Missing a PMT?"
        assert charge is None or len(charge) == 4, "Missing a PMT?"

        self.mask = pmt_mask
        self.histos = {"peak": peak, "charge": charge}

        self.fill_value = uncertainties.ufloat(np.nan, np.nan)
        self.popts = {
            "peak": [[self.fill_value for _ in range(3)] for _ in range(4)],
            "charge": [[self.fill_value for _ in range(3)] for _ in range(4)],
        }

        self.fit_was_run = False


    def __call__(self) -> list[uncertainties.ufloat]:
        if not self.fit_was_run:
            _ = self.fit()

        peak_charge = {
            "peak": [],
            "charge": [],
        }
        for i in range(4):
            peak_charge["peak"].append(self.popts["peak"][i][1])
            peak_charge["charge"].append(self.popts["charge"][i][1])

        return peak_charge


    def __getitem__(self, pmt: int) -> np.ndarray:
        if pmt < 10:
            return self.histos["peak"][pmt]
        else:
            return self.histos["charge"][pmt-10]


    def fit(self) -> dict:
        if self.histos["peak"] is not None:
            self.popts["peak"] = self.get_peak("peak")
        if self.histos["charge"] is not None:
            self.popts["charge"] = self.get_peak("charge")

        self.fit_was_run = True

        return self.popts


    def get_peak(self, mode: str) -> list[list[uncertainties.ufloat]]:

        peaks = []
        for i, counts in enumerate(self.histos[mode]):
            if i < 3:
                peak = (
                    [self.fill_value for _ in range(3)]
                    if not self.mask[i]
                    else self.fit_wcd(counts[: (99 if mode == "peak" else 399)])
                )
                peaks.append(peak)
            else:
                peak = (
                    [self.fill_value for _ in range(3)]
                    if not self.mask[i]
                    else self.fit_ssd(counts[: (99 if mode == "peak" else 399)])
                )
                peaks.append(peak)

        return peaks


    @staticmethod
    def fit_wcd(counts: np.ndarray) -> list[uncertainties.ufloat]:

        try:
            match len(counts):
                case 99:
                    increment = 5
                    bins = CONSTANTS.UUB.WCD_PEAK
                    initial_start = 99 - increment
                case 399:
                    increment = 20
                    bins = CONSTANTS.UUB.WCD_CHARGE
                    initial_start = 399 - increment
                case _:
                    raise IndexError(f"received histogram with length {len(counts)}")

            old_peak, guess = np.argmax(counts[initial_start:]), 0
            while old_peak != guess:
                old_peak = guess
                initial_start -= increment
                guess = np.argmax(counts[initial_start:]) + initial_start

            start, stop = guess - increment, guess + increment
            x1, x2, y1, y2 = (
                start,
                len(counts) - 1,
                counts[stop],
                counts[len(counts) - 1],
            )
            background_slope = lambda x: (y2 - y1) / (x2 - x1) * (x - x1) + y1

            popts, pcov = curve_fit(
                SdHisto.parabola,
                bins[start:stop],
                np.log(counts[start:stop])
                - background_slope(np.arange(start, stop, 1)),
                bounds=([-np.inf, 0, 0], [0, np.inf, np.inf]),
                maxfev=100000,
                p0=[-0.01, bins[guess], counts[guess]],
                nan_policy="omit",
            )

            popts = uncertainties.correlated_values(popts, pcov)
            # if len(counts) == 99 and popts[1].n < 100 or 300 < popts[1].n: raise ValueError(f"calculated {popts[1]:i} ADC for WCD peak")
            # if len(counts) == 399 and popts[1].n < 1000 or 2000 < popts[1].n: raise ValueError(f"calculated {popts[1]:i} ADC for WCD charge")
            if (r := popts[1].std_dev / popts[1].n) > 0.2:
                raise ValueError(f"large fit error for WCD: {r*100:.0f}%")
            return popts

        except Exception as e:
            # print(f'WCD SdHisto fit failed: {e}')
            return [uncertainties.ufloat(np.nan, np.nan) for _ in range(3)]


    @staticmethod
    def fit_ssd(counts: np.ndarray) -> list[uncertainties.ufloat]:

        try:
            match len(counts):
                case 99:
                    bins = CONSTANTS.UUB.SSD_PEAK
                    increment = 5
                    start = np.argmax(counts)

                    while not np.argmax(counts[start:]):
                        start += 1
                case 399:
                    bins = CONSTANTS.UUB.SSD_CHARGE
                    increment = 20
                    order = 10

                    while (
                        len(
                            (dips := argrelextrema(counts[1:], np.less, order=order)[0])
                        )
                        > 1
                    ):
                        order += 1
                    start = dips[0] + 1

                case _:
                    raise IndexError(f"received histogram with length {len(counts)}")

            guess = start + np.argmax(counts[start:])
            start, stop = guess - increment, guess + increment

            popts, pcov = curve_fit(
                SdHisto.parabola,
                bins[start:stop],
                counts[start:stop],
                bounds=([-np.inf, 0, 0], [0, np.inf, np.inf]),
                maxfev=100000,
                p0=[-0.01, bins[guess], counts[guess]],
                nan_policy="omit",
            )

            popts = uncertainties.correlated_values(popts, pcov)
            # if len(counts) == 99 and popts[1].n < 20 or 100 < popts[1].n: raise ValueError(f"calculated {popts[1]:i} ADC for SSD peak")
            # if len(counts) == 399 and popts[1].n < 20 or 100 < popts[1].n: raise ValueError(f"calculated {popts[1]:i} ADC for SSD charge")
            if (r := popts[1].std_dev / popts[1].n) > 0.2:
                raise ValueError(f"large fit error for SSD: {r*100:.0f}%")
            return popts

        except Exception as e:
            print(f'SSD SdHisto fit failed: {e}')
            return [uncertainties.ufloat(np.nan, np.nan) for _ in range(3)]


    def plot(self) -> plt.Figure:

        if self.histos["peak"] is not None and self.histos["charge"] is not None:
            fig, (ax1, ax2) = plt.subplots(1, 2)
        else:
            fig, ax = plt.subplots()
            ax1 = ax2 = ax

        f = 4
        c = ["red", "blue", "mediumturquoise", "k"]
        l = ["WCD1", "WCD2", "WCD3", rf"SSD $\times$ {f}"]

        if self.histos["peak"] is not None:
            ax1.set_xlabel("max. pulse height / ADC")
            for i, counts in enumerate(self.histos["peak"]):

                linestyle = "-" if self.mask[i] else ":"
                label = l[i] if self.mask[i] else l[i] + ", (masked)"
                lw = 1 if self.mask[i] else 0.5
                factor = 1 if i < 3 else f

                ax1.plot(
                    self.get_bins("peak", i) * factor,
                    counts,
                    c=c[i],
                    ls=linestyle,
                    label=label,
                    marker="none",
                    lw=lw,
                )
                ax1.axvline(
                    self.popts["peak"][i][1].n * factor, lw=0.4, ls="--", c=c[i]
                )
                err = (
                    self.popts["peak"][i][1].std_dev * np.array([-1, 1])
                    + self.popts["peak"][i][1].n
                )
                ax1.axvspan(*(err * factor), color=c[i], alpha=0.1)

            ax1.set_xlim(0, 400)
            ax1.legend(title="Peak")

        if self.histos["charge"] is not None:
            ax2.set_xlabel("integral / ADC")
            for i, counts in enumerate(self.histos["charge"]):

                linestyle = "-" if self.mask[i] else ":"
                label = l[i] if self.mask[i] else l[i] + ", (masked)"
                lw = 1 if self.mask[i] else 0.5
                factor = 1 if i < 3 else f

                ax2.plot(
                    self.get_bins("charge", i) * factor,
                    counts,
                    c=c[i],
                    ls=linestyle,
                    label=label,
                    marker="none",
                    lw=lw,
                )
                ax2.axvline(
                    self.popts["charge"][i][1].n * factor, lw=0.4, ls="--", c=c[i]
                )
                err = (
                    self.popts["charge"][i][1].std_dev * np.array([-1, 1])
                    + self.popts["charge"][i][1].n
                )
                ax2.axvspan(*(err * factor), color=c[i], alpha=0.1)

            ax2.set_xlim(0, 3200)
            ax2.legend(title="Charge")

        ax1.set_ylabel("Counts")
        return fig


    @staticmethod
    def parabola(x, scale, mip, y0):
        return scale * (x - mip) ** 2 + y0

    @staticmethod
    def get_bins(mode: str, pmt: int) -> np.ndarray:

        if mode == "peak" and pmt < 3:
            return CONSTANTS.UUB.WCD_PEAK
        elif mode == "peak" and pmt == 3:
            return CONSTANTS.UUB.SSD_PEAK
        elif mode == "charge" and pmt < 3:
            return CONSTANTS.UUB.WCD_CHARGE
        elif mode == "charge" and pmt == 3:
            return CONSTANTS.UUB.SSD_CHARGE


def read_histos(path_to_file: str) -> list[dict]:

    return_list = []
    mask = [
        [0, 0, 0, 1],
        [1, 0, 0, 1],
        [0, 1, 0, 1],
        [1, 1, 0, 1],
        [0, 0, 1, 1],
        [1, 0, 1, 1],
        [0, 1, 1, 1],
        [1, 1, 1, 1],
    ]

    original_data = np.loadtxt(path_to_file, dtype=int)

    for event in np.split(original_data, len(original_data) // 4):

        assert (
            len(id := np.unique(event[:, 0])) == 1
        ), "Missing data =( (Station Id looks wrong)"
        assert (
            len(daq_time := np.unique(event[:, 1])) == 1
        ), "Missing data =( (DAQ time looks wrong)"
        assert (
            len(timestamp := np.unique(event[:, 2])) == 1
        ), "Missing data =( (timestamp looks wrong)"
        assert (
            len(pmt_mask := np.unique(event[:, 3])) == 1
        ), "Missing data =( (PMT mask looks wrong)"

        return_list.append(
            {
                "id": id[0],
                "daq_time": daq_time[0],
                "timestamp": timestamp[0],
                "pmt_mask": mask[pmt_mask[0]],
                "data": event[:, 5:],
            }
        )

    return return_list
