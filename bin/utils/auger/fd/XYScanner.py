from ... import CONSTANTS as CONST
from ...binaries import uncertainties
from ... import create_stream_logger
from ...binaries import pd
from ...binaries import np
from ...plotting import plt
from . import AperturePlot, PixelPlot
import logging
import glob

from datetime import datetime
from matplotlib import colors
from matplotlib.gridspec import GridSpec
from matplotlib.colorbar import ColorbarBase
from collections import defaultdict
from tabulate import tabulate, SEPARATING_LINE
import subprocess
import os
from warnings import warn

def read_runlist(runlist_path: str) -> pd.DataFrame:
    return pd.read_csv(runlist_path,
                       names=["id","tel","step","date","source","mA","forDB","jobfile","comment"],
                       dtype=defaultdict(lambda: "str", step="int", mA="float", forDB="bool"),
                       skipinitialspace=True, parse_dates=["date"], index_col=0, comment="#", sep=";")

def get_mirror_name(tel: int) -> str:
    if tel <= 6:
        return f"LL{tel}"
    elif tel <= 12:
        return f"LM{tel - 6}"
    elif tel <= 18:
        return f"LA{tel - 12}"
    elif tel <= 24:
        return f"CO{tel - 18}"
    else:
        return f"HE{tel - 24}"

def get_mirror_number(tel:str) -> int:
    tel = tel.upper()
    if "LL" in tel:
        return int(tel[-1])
    elif "LM" in tel:
        return int(tel[-1]) + 6
    elif "LA" in tel:
        return int(tel[-1]) + 12
    elif "CO" in tel:
        return int(tel[-1]) + 18
    elif "HE" in tel:
        return int(tel[-1]) + 24


class XYRun():

    def __init__(self, runs: pd.DataFrame) -> None:

        self.year_month_day = runs[runs['forDB']].values[0,2].strftime("%Y-%m-%d")
        self.telescope = runs[runs['forDB']].values[0,0].upper()
        self.source = runs[runs['forDB']].values[0,3].upper()
        self.stepsize = runs[runs['forDB']].values[0,1]
        self.run_numbers = self._scan_runs(runs)

        match self.telescope:
            case "LL1":
                mask_file = CONST.SCAN_PATH / "config/pixel_masks/ll1.txt"
                mask = np.loadtxt(mask_file, usecols=1, dtype=bool)
            case "LL6":
                mask_file = CONST.SCAN_PATH / "config/pixel_masks/ll6-xp3062.txt"
                mask = np.loadtxt(mask_file, usecols=1, dtype=bool)
            case _:
                mask = np.ones(440, dtype=bool)

        self.run_data = {}
        for run, run_id in self.run_numbers.items():
            try:
                if run == "XY":
                    file = f"results/outCorr_{run_id}.txt"
                    std, xy = np.loadtxt(CONST.SCAN_PATH / file, usecols=[3,4], unpack=True)
                    std[~mask], xy[~mask] = np.nan, np.nan
                    self.run_data["CalA"] = std
                    self.run_data["XY"] = xy
                    mask[std == 0.] = False
                    std[~mask], xy[~mask] = np.nan, np.nan
                    ratio = xy / std
                    self.run_data["ratio"] = ratio
                else:
                    file = f"results/out_{run_id}.txt"
                    run_data = np.loadtxt(CONST.SCAN_PATH / file, usecols=2)
                    run_data[~mask] = np.nan
                    self.run_data[run] = run_data
                
            except FileNotFoundError:
                warn(f"{self.telescope} @ {self.year_month_day}: {run} ({run_id}) not found")

    
    def __repr__(self) -> str:
        return f"{self.telescope} @ {self.year_month_day} w/ {self.source}: {self.run_numbers}"

    
    def get_id(self, key: str) -> str:
        return self.run_numbers.get(key, "")


    def get_data(self, key: str) -> str:
        return self.run_data.get(key, None)


    @staticmethod
    def _scan_runs(runs: pd.DataFrame) -> dict:
        run_dict, postXY = {}, False
        for _id, info in runs.iterrows():

            if info['forDB']: 
                postXY = True
                key = "XY"
            else:
                try:
                    key = "post" if postXY else "pre"
                    if "open shutter" in info["comment"]:
                        key += "XY"
                    else: 
                        key += "DAQ"
                except TypeError: continue

            if run_dict.get(key, None) is None or not postXY:
                run_dict[key] = _id

        return run_dict
    

    def run_calib(self, offline_version=None, rerun=True) -> None:

        if offline_version is None:
            offline_version = "../bd0e9e"
        
        ids = " ".join(self.runs.values())
        r = "-r" if rerun else ""

        move = f"cd {CONST.SCAN_PATH}"
        run_calib = f"./run_Calib.py -i {ids} {r} -cfg {'_'.join(self.year_month_day[:-3])}"
        source_offline = f"source {CONST.OFLN_PATH / offline_version / 'set_offline_env.sh'}"

        subprocess.call("; ".join([move, source_offline, run_calib]), shell=True, executable='/bin/bash')


class XYRunContainer():

    def __init__(self, pd_runlist: pd.DataFrame) -> None:
        self.runs = self.split_runlist(pd_runlist)
        self.data = pd_runlist

    def __getitem__(self, idx) -> XYRun:
        if isinstance(idx, int):
            if idx > len(self.runs): return StopIteration
            idx = list(self.runs.keys())[idx]
        elif isinstance(idx, slice):
            return list(self.runs.values())[idx]

        return self.runs[idx.upper()]
        

    def __repr__(self) -> str:
        
        table_data = []
        for key, run in self.runs.items():
            
            table_data.append(
                [key,
                 run.get_id('preDAQ'),
                 run.get_id('preXY'),
                 run.get_id('XY'),
                 run.get_id('postXY'),
                 run.get_id('postDAQ')
                 ]
            )

        return tabulate(table_data, disable_numparse=True,
                        headers=["", 'preDAQ', 'preXY', 'XY', 'postXY', 'postDAQ'])


    @staticmethod
    def split_runlist(pd_runlist: pd.DataFrame) -> dict:
        raise NotImplementedError() from None


    def plot_summary(self) -> plt.Figure:
        raise NotImplementedError


    def get_data(self, key: str) -> dict:

        data_dict = {}
        for run in self:
            data_dict[str(run)] = run.get_data(key)

        return data_dict


    def get_id(self, key: str) -> dict:

        data_dict = {}
        for run in self:
            data_dict[str(run)] = run.get_id(key)

        return data_dict


    def pack_files(self, out: str = None, extension: str = "") -> None:

        candidate_files = set()
        base_dir = str(CONST.SCAN_PATH / "results")
        if out is None: out = f"archive{extension}.tgz"

        for _id in self.data.index:
            files = glob.glob(f"{base_dir}/{extension}*{_id}.txt")
            for file in files:
                candidate_files.add(file.split("/")[-1])

        subprocess.call(f"tar -caf {out} -C {base_dir} {' '.join(candidate_files)}", shell=True, executable='/bin/bash')


class Campaign(XYRunContainer):

    mirrors = [
        "LL1", "LL2", "LL3", "LL4", "LL5", "LL6", 
        "LM1", "LM2", "LM3", "LM4", "LM5", "LM6", 
        "LA1", "LA2", "LA3", "LA4", "LA5", "LA6", 
        "CO1", "CO2", "CO3", "CO4", "CO5", "CO6", 
        "HE1", "HE2", "HE3"
    ]

    def __init__(self, year: int, month: int) -> None:
        self.year, self.month = year, month

        runlist = CONST.SCAN_PATH / f"config/calib_runlists/calib_runs_{self.year}-{self.month}.list"
        
        if not os.path.isfile(runlist):
            raise FileNotFoundError(f"{runlist} not found!\nAvailable campaigns are:\n" + 
                                    "\n".join(os.listdir(CONST.SCAN_PATH / "config/calib_runlists")))

        super().__init__(read_runlist(runlist))


    def split_runlist(self, runlist: pd.DataFrame) -> dict:

        run_dict = {}
        for _, info in runlist[runlist['forDB']].iterrows():
            
            run_dict[info['tel'].upper()] = XYRun(runlist[
                (runlist['date'] == info['date'])
                & (runlist['tel'] == info['tel'])])

        telescopes_ordered = sorted([get_mirror_number(t) for t in run_dict.keys()])
        telescopes_ordered = [get_mirror_name(t) for t in telescopes_ordered]
        run_dict = {t : run_dict[t] for t in telescopes_ordered}

        return run_dict

    # def _sort_mirrors(self, tel1: str, tel2: str) -> bool:
    #     raise NotImplementedError("let's see if this is used anywhere")
    #     mirror1 = self._get_mirror_number(tel1)
    #     mirror2 = self._get_mirror_number(tel2)
    #     return mirror1 < mirror2 


    # def run_calib(self, /, offline_version=None, *, rerun=False) -> None:
    #     for run in self:
    #         run.run_calib(offline_version, rerun)

    
    def plot_summary(self, normalize='none', **kwargs) -> plt.Figure:

        fig, ax = plt.subplots()

        if normalize == 'none':
            ax.set_ylabel(r'calib. const. / $\left( \gamma\,/\,\mathrm{ADC} \right)$')
        elif normalize.lower() == 'cala':
            cala_drift = kwargs.get('cala_drift', 0)
            drift_label = fr"($+\frac{{ {1e2*cala_drift}\% }}{{ \mathrm{{year}} }}$)" if cala_drift else ""
            ax.set_ylabel(f'XY calib. const. / std. calib. {drift_label}')
            time_difference_years = (datetime(self.year, self.month) - datetime(2010,6, 1)).days / 365
            ax.fill_between([0,32], 1-0.099, 1+0.099, color='k', alpha=0.1, edgecolor='none')
        else:
            ax.set_ylabel(f'XY calib. const. / {kwargs.get("label", "custom norm")}')

        ax.text(
            0, 1,
            f"{self.year}-{self.month} XY campaign summary",
            transform=ax.transAxes,
            ha="left",
            va="bottom",
            fontsize="large",
            fontweight="bold",
        )
        
        ax.set_xticklabels([], fontsize=8, rotation=90)
        ax.set_xticks([], minor=True)
        ax.grid(axis="y")
        ax.grid(axis="y", which="minor", alpha=0.3)
        ax.set_xlim(0, 32)

        data, positions, labels = [], [], []
        for pos, tel in enumerate(self.mirrors, 1):
            try:


                if normalize == 'none':
                    d = self[tel].get_data("XY")
                    norm = np.ones(440)
                elif normalize.lower() == 'cala':
                    d = self[tel].get_data("ratio")
                    norm = (1 + cala_drift*time_difference_years)
                else:
                    d = self[tel].get_data("XY")
                    norm = normalize

                if d is None: continue
                ratio = d / norm
                data.append(ratio[~np.isnan(ratio)])
                labels.append(tel)

                match tel:
                    case _ if "LL" in tel: increment = 0
                    case _ if "LM" in tel: increment = 1
                    case _ if "LA" in tel: increment = 2
                    case _ if "CO" in tel: increment = 3
                    case _ if "HE" in tel: increment = 4
                positions.append(pos + increment)

            except KeyError:
                continue

        ax.boxplot(data, positions=positions, labels=labels, widths=0.5,
                   flierprops={'markersize': 2, 'alpha': 0.2}, notch=True,
                   bootstrap=5000)

        ymin, ymax = ax.get_ylim()
        ax.set_ylim(kwargs.get('ymin', ymin), kwargs.get('ymax', ymax))

        return fig


class Telescope(XYRunContainer):

    def __init__(self, telescope: str) -> None:

        self.telescope = telescope

        data = pd.DataFrame()
        for runlist in os.listdir(CONST.SCAN_PATH / f"config/calib_runlists"):
            if "testing" in runlist or "va" in runlist: continue

            this_run = read_runlist(CONST.SCAN_PATH / f"config/calib_runlists/{runlist}")
            dropped_indices = this_run[this_run['tel'] != telescope.lower()].index
            data = pd.concat([data, this_run.drop(dropped_indices)])

        super().__init__(data)


    def split_runlist(self, runlist: pd.DataFrame) -> dict:

        run_dict = {}
        for _, info in runlist[runlist['forDB']].iterrows():
            run_dict[info['date'].strftime("%Y-%m-%d")] = XYRun(runlist[runlist['date'] == info['date']])

        runs_ordered = sorted([d for d in run_dict.keys()])
        run_dict = {d : run_dict[d] for d in runs_ordered}

        return run_dict
    

    def plot_summary(self, normalize='none', **kwargs) -> None:

        if len(self.runs) == 1:
            ax = PixelPlot(list(self.runs.values())[0].xy)
            return

        cut = lambda x: x[np.abs(x - np.mean(x)) < np.std(x)]

        fig = plt.figure()
        gs = GridSpec(
            2,
            len(self.runs) + 1,
            fig,
            height_ratios=[0.6, 1],
            width_ratios=[0.1] + [1 for _ in range(len(self.runs))]
        )
        gs.update(left=0.05, right=0.95, wspace=0.05, hspace=0.1)

        pixel_plots = [fig.add_subplot(gs[0, i], aspect='equal') for i in range(1, len(self.runs) + 1)]
        history_plot = fig.add_subplot(gs[1, :])
        colorbar = fig.add_subplot(gs[0, 0])

        vmin, vmax = np.inf, -np.inf
        data, positions, labels = [], [], []
        fit_y = []
        for run, ax in zip(self.runs.values(), pixel_plots):
            xy = run.get_data('XY')
            vmin, vmax = np.min([vmin, *cut(xy)]), np.max([vmax, *cut(xy)])
            
            if normalize == "none":
                norm = np.ones(440)
                colorbar_label = r"$\gamma\,/\,\mathrm{ADC}$"
            elif normalize == "cala":
                norm = run.cala
                colorbar_label = "XY / std."
                vmin, vmax = np.min(vmin/norm), np.max(vmax/norm)

            PixelPlot(xy / norm, ax, title=run.year_month_day, vmin=vmin, vmax=vmax, lw=0.4)
            data.append(xy / norm)
            positions.append(datetime.strptime(run.year_month_day, "%Y-%m-%d").timestamp())
            fit_y.append(np.median(xy / norm))
            labels.append(run.year_month_day)

        history_plot.boxplot(data, positions=positions, labels=labels, widths=3600*24*10,
                            whiskerprops={'lw': 0.3}, notch=True,
                            bootstrap=5000, showfliers=False)
        
        # drift 
        # TODO

        history_plot.set_ylabel(f"{self.telescope}: XY calib")
        
        ColorbarBase(
                colorbar,
                cmap=plt.cm.viridis,
                norm=colors.Normalize(vmin, vmax),
                orientation="vertical",
                label=colorbar_label
            )
        
        colorbar.yaxis.set_ticks_position('left')
        colorbar.yaxis.set_label_position('left')


# class Grid:

#     # data_path: str = f"{CONST.AUGER_FD_ROOT}/xy_measurements.pkl"
#     n_bays: dict = {t: 6 if t != "HE" else 3 for t in ["LL", "LM", "LA", "CO", "HE"]}

#     def __init__(self, *args, compare="none") -> None:

#         raise DeprecationWarning("change this")

#         import pickle

#         with open(f"{CONST.AUGER_FD_ROOT}/xy_measurements.pkl", "rb") as f:
#             measurements = pickle.load(f)

#         telescopes = [eye.upper() for eye in args]
#         self.n_rows, self.n_cols, self.dates = self.get_rows_and_cols(
#             measurements, telescopes
#         )
#         self.xlabels = [
#             [f"{t}{n}" for n in range(1, self.n_bays[t] + 1)] for t in telescopes
#         ]
#         self.xlabels = [bay for site in self.xlabels for bay in site]
#         self.ylabels = sorted(self.dates)
#         self.compare = compare

#         print("columns (sites):", self.xlabels)
#         print("rows    (dates):", self.ylabels)

#         self.grid_data = {
#             bay: {date: [] for date in self.ylabels} for bay in self.xlabels
#         }
#         for bay in self.xlabels:

#             try:
#                 first_measurement = measurements[bay][0]
#                 date = self.get_month(first_measurement["date"])

#                 self.grid_data[bay][date] = first_measurement

#                 if len(measurements[bay]) == 1:
#                     continue

#                 last_date_entry = None
#                 for measurement in measurements[bay][1:]:
#                     tmp = self.get_month(measurement["date"])
#                     self.grid_data[bay][tmp] = [measurement, first_measurement]
#                     if tmp == last_date_entry:
#                         raise ValueError(f"Multiple runs found for {tmp}")
#                     last_date_entry = tmp
#             except KeyError:
#                 continue

#     def plot(self) -> plt.Figure:

#         from matplotlib.gridspec import GridSpec
#         from matplotlib.colorbar import ColorbarBase
#         from itertools import product

#         fig = plt.figure(figsize=(2 * self.n_cols + 0.2, 2 * self.n_rows))
#         gs = GridSpec(
#             self.n_rows,
#             self.n_cols + 1,
#             figure=fig,
#             width_ratios=[1 for _ in range(self.n_cols)] + [0.2],
#             height_ratios=[1 for _ in range(self.n_rows)],
#         )

#         gs.update(left=0.05, right=0.95, wspace=0.02, hspace=0.02)

#         data = [
#             self.grid_data[self.xlabels[j]][self.ylabels[i]]
#             for i, j in product(range(self.n_rows), range(self.n_cols))
#         ]
#         axes = iter(
#             [
#                 fig.add_subplot(gs[i, j])
#                 for i, j in product(range(self.n_rows), range(self.n_cols))
#             ]
#         )
#         data, types, _range = self.compute_data(data)

#         self.norm = (
#             colors.Normalize(*_range)
#             if self.compare == "none"
#             else colors.CenteredNorm(1, _range)
#         )

#         for i, j in product(range(self.n_rows), range(self.n_cols)):
#             self.draw_axis(i, j, next(axes), next(data), next(types))

#         cax = fig.add_subplot(gs[:, -1])
#         ColorbarBase(
#             cax,
#             cmap=plt.cm.viridis if self.compare == "none" else plt.cm.coolwarm,
#             norm=self.norm,
#             orientation="vertical",
#         )

#         return fig

#     def draw_axis(self, row, col, ax, data, type) -> None:

#         if col == 0:
#             ax.set_ylabel(self.ylabels[row])
#         if row == 0:
#             ax.set_title(self.xlabels[col])

#         if len(data) != 0:
#             kwargs = {
#                 "norm": self.norm,
#                 "cmap": plt.cm.coolwarm if type and self.compare != "none" else None,
#             }
#             PixelPlot(data, ax=ax, axis_off=False, **kwargs)

#         self.hide_axis(ax)

#     def compute_data(self, data) -> tuple[list, list, float, float]:

#         standard_deviations = []
#         types, computed_data = [], []
#         data_min, data_max = [np.inf, -np.inf]
#         for runs in data:
#             match len(runs):
#                 case 0:
#                     computed_data.append([])
#                     types.append(0)

#                 case 2:
#                     xy_pixels1 = self.read_data_and_normalize(runs[0])
#                     xy_pixels2 = self.read_data_and_normalize(runs[1])
#                     ratio = (
#                         xy_pixels1 / xy_pixels2
#                         if self.compare == "first"
#                         else xy_pixels2
#                     )

#                     mean, std = ratio.mean(), ratio.std()

#                     if mean + 3 * std > data_max:
#                         data_max = np.max([mean + 3 * std, data_max])
#                     if mean - 3 * std < data_min:
#                         data_min = np.min([mean - 3 * std, data_min])

#                     computed_data.append(ratio)
#                     types.append(1)

#                     standard_deviations.append(std)

#                 case 4:
#                     xy_pixels = self.read_data_and_normalize(runs)

#                     mean, std = xy_pixels.mean(), xy_pixels.std()

#                     if mean + 3 * std > data_max and self.compare == "none":
#                         data_max = np.max([mean + 3 * std, data_max])
#                     if mean - 3 * std < data_min and self.compare == "none":
#                         data_min = np.min([mean - 3 * std, data_min])

#                     computed_data.append(xy_pixels)
#                     types.append(0)

#         return (
#             iter(computed_data),
#             iter(types),
#             (
#                 (np.max([0, data_min]), data_max)
#                 if self.compare == "none"
#                 else 3 * np.mean(standard_deviations)
#             ),
#         )

#     @staticmethod
#     def hide_axis(ax: plt.Axes) -> None:
#         ax.spines[["right", "top", "left", "bottom"]].set_visible(False)
#         ax.set_xticks([])
#         ax.set_yticks([])

#     @staticmethod
#     def get_month(date_str) -> str:
#         return "-".join(date_str.split("-")[:-1])

#     def read_data_and_normalize(self, run) -> tuple[np.ndarray, np.ndarray]:

#         result_dir = "/cr/data01/filip/xy-calibration/results"
#         xy, CalAs = run["XY"], run["CalA_open_shutter"]
#         try:
#             CalA_signal = np.zeros(440)

#             n_CalA = 0
#             for CalA in CalAs:
#                 if CalA is None:
#                     continue

#                 CalA_signal += np.loadtxt(f"{result_dir}/out_{CalA}.txt", usecols=[1])
#                 n_CalA += 1

#             CalA_signal /= n_CalA

#         except (IndexError, AssertionError):
#             print(
#                 "Malformed CalA data received, please make sure you pass \
#                 in two CalAs (pre/post-XY), which have 440 pixels of data"
#             )

#         pixels = np.loadtxt(f"{result_dir}/out_{xy}.txt", usecols=[1])

#         if self.compare != "none":
#             pixels /= CalA_signal / 50
#         return np.array(pixels)

#     @staticmethod
#     def get_rows_and_cols(measurements, telescopes) -> tuple[int, int, list[str]]:

#         n_cols = sum([6 if t != "HE" else 3 for t in telescopes])

#         times, n_rows = [], 0
#         for telescope in measurements.values():
#             for runs in telescope:
#                 times.append("-".join(runs["date"].split("-")[:-1]))

#         times = np.unique(times)
#         n_rows += len(times)

#         return n_rows, n_cols, list(times)

#     def __getitem__(self, keys) -> list:
#         if len(keys) != 2:
#             raise ValueError(f"Please provide ['telescope', 'date']")

#         return self.grid_data[keys[0]][keys[1]]


# def XYComparisonPlot(
#     *runs: list[dict],
#     cmap=plt.cm.coolwarm,
#     hist_bins=50,
#     vmin=0.6,
#     vmax=1.4,
#     contrast_boost=False,
# ) -> plt.figure:
#     """Compare the results of two XY runs, normalized to their respective Cal A signal"""

#     from matplotlib.colors import Normalize
#     from matplotlib.gridspec import GridSpec
#     from matplotlib.colorbar import ColorbarBase

#     assert len(runs) == 2, "please only compare two runs at a time"

#     fig = plt.figure()
#     gs = GridSpec(
#         2,
#         4,
#         figure=fig,
#         width_ratios=[1, 1, 0.8, 0.05],
#         height_ratios=[1, 1],
#     )
#     gs.update(left=0.05, right=0.95, wspace=0.05, hspace=0.02)

#     positions_normalized, pixels_normalized = [], []
#     result_dir = "/cr/data01/filip/xy-calibration/results"
#     for run in runs:
#         xy, CalAs = run["XY"], run["CalA_open_shutter"]
#         try:
#             CalA_signal = np.zeros(440)
#             for CalA in CalAs:
#                 CalA_signal += np.loadtxt(f"{result_dir}/out_{CalA}.txt", usecols=[2])

#         except (IndexError, AssertionError):
#             print(
#                 "Malformed CalA data received, please make sure you pass \
#                 in two CalAs (pre/post-XY), which have 440 pixels of data"
#             )
#             return fig

#         pixels = np.loadtxt(f"{result_dir}/out_{xy}.txt", usecols=[2])
#         positions = pd.read_csv(
#             f"{result_dir}/outPositionsComb_{xy}.txt", usecols=["x", "y", "FDeventSum"]
#         )

#         # divide by 50 due to 50 flashes
#         pixels /= CalA_signal / 50  # normalize pixels to calA pixels
#         positions["FDeventSum"] /= (
#             np.sum(CalA_signal) / 50
#         )  # normalize positions to calA sum

#         positions_normalized.append(positions)
#         pixels_normalized.append(pixels)

#     pixel_ratio = pixels_normalized[0] / pixels_normalized[1]
#     positions_ratio = build_xy_ratio(positions_normalized[0], positions_normalized[1])
#     mean_positions = uncertainties.ufloat(
#         np.mean(positions_ratio["ratio"]), np.std(positions_ratio["ratio"])
#     )
#     mean_pixels = uncertainties.ufloat(np.mean(pixel_ratio), np.std(pixel_ratio))

#     ax1 = fig.add_subplot(gs[:, 0])
#     ax2 = fig.add_subplot(gs[:, 1])
#     ax3 = fig.add_subplot(gs[0, 2])
#     ax4 = fig.add_subplot(gs[1, 2], sharex=ax3)
#     ax5 = fig.add_subplot(gs[:, 3])

#     # set up colorbar for position and pixel comparison
#     norm = Normalize(vmin=vmin, vmax=vmax)
#     date1, date2 = runs[0]["date"], runs[1]["date"]
#     ColorbarBase(
#         ax5,
#         cmap=cmap,
#         norm=norm,
#         orientation="vertical",
#         label=rf"$\tilde{{\mathrm{{XY}}}}_\mathrm{{{date1}}}\,/\,\tilde{{\mathrm{{XY}}}}_\mathrm{{{date2}}}$",
#     )

#     # set up aperture position comparison
#     AperturePlot(ax1)
#     ax1.scatter(
#         positions_ratio.x,
#         positions_ratio.y,
#         c=positions_ratio["ratio"],
#         norm=norm,
#         marker="o",
#         cmap=cmap,
#         s=4,
#     )
#     ax1.axis("off")
#     ax1.set_title("Aperture view")
#     ax1.plot([-650, 650], [1350, 1350], c="k", ls="--", clip_on=False)

#     # set up camera pixel comparison
#     if contrast_boost:
#         _min, _max, mean, std = (
#             pixel_ratio.min(),
#             pixel_ratio.max(),
#             pixel_ratio.mean(),
#             pixel_ratio.std(),
#         )
#         pixel_plot_ratios = np.interp(
#             pixel_ratio, (mean - 2 * std, mean + 2 * std), (vmin, vmax)
#         )
#     else:
#         pixel_plot_ratios = pixel_ratio

#     PixelPlot(
#         pixel_plot_ratios,
#         ax=ax2,
#         cmap=cmap,
#         vmin=vmin,
#         vmax=vmax,
#         norm=norm,
#         title="Camera view" + (" (contrast boost)" if contrast_boost else ""),
#     )
#     ax2.plot([-7, 7], [-18, -18], c="r", clip_on=False)

#     # histograms for comparison
#     bins = np.linspace(vmin, vmax, hist_bins)
#     bin_centers = 0.5 * (bins[1:] + bins[:-1])
#     ax3.hist(
#         np.clip(pixel_ratio, vmin, vmax),
#         bins=bins,
#         density=True,
#         histtype="step",
#         color="r",
#         ls="solid",
#     )
#     n1, _, patches = ax3.hist(np.clip(pixel_ratio, vmin, vmax), bins=bins, density=True)
#     ax3.text(
#         0.02,
#         0.98,
#         rf"$\langle\tilde{{\mathrm{{XY}}}}^\mathrm{{pix.}}_\mathrm{{{date1}}}\,/\,\tilde{{\mathrm{{XY}}}}^\mathrm{{pix.}}_\mathrm{{{date2}}}\rangle = {mean_pixels.format('S')}$",
#         horizontalalignment="left",
#         verticalalignment="top",
#         transform=ax3.transAxes,
#         fontdict={"fontsize": 7},
#     )

#     for x, p in zip(bin_centers, patches):
#         plt.setp(p, "facecolor", cmap(norm(x)))

#     ax4.hist(
#         np.clip(positions_ratio["ratio"], vmin, vmax),
#         bins=bins,
#         density=True,
#         histtype="step",
#         color="k",
#         ls="--",
#     )
#     n2, _, patches = ax4.hist(
#         np.clip(positions_ratio["ratio"], vmin, vmax), bins=bins, density=True
#     )
#     ax4.text(
#         0.02,
#         0.98,
#         rf"$\langle\tilde{{\mathrm{{XY}}}}^\mathrm{{pos.}}_\mathrm{{{date1}}}\,/\,\tilde{{\mathrm{{XY}}}}^\mathrm{{pos.}}_\mathrm{{{date2}}}\rangle = {mean_positions.format('S')}$",
#         horizontalalignment="left",
#         verticalalignment="top",
#         transform=ax4.transAxes,
#         fontdict={"fontsize": 7},
#     )
#     for x, p in zip(bin_centers, patches):
#         plt.setp(p, "facecolor", cmap(norm(x)))

#     ax3.set_ylim(1e-2, 1.2 * max(n1))
#     ax4.set_ylim(1e-2, 1.2 * max(n2))
#     # ax3.legend(fontsize=7, loc='upper left')
#     # ax4.legend(fontsize=7, loc='upper left')
#     ax3.set_yticks([])
#     ax4.set_yticks([])

#     return fig


# def build_xy_ratio(data1, data2):

#     pd.options.mode.chained_assignment = None

#     # multiple flashes at position 0
#     d1_zeros = data1.loc[data1["x"] + data1["y"] == 0]
#     d2_zeros = data2.loc[data2["x"] + data2["y"] == 0]

#     data1 = pd.concat([data1, d1_zeros, d1_zeros]).drop_duplicates(keep=False)
#     data2 = pd.concat([data2, d2_zeros, d2_zeros]).drop_duplicates(keep=False)

#     ratios = pd.DataFrame(
#         {
#             "x": 0,
#             "y": 0,
#             "ratio": d1_zeros["FDeventSum"].mean() / d2_zeros["FDeventSum"].mean(),
#         },
#         index=[0],
#     )

#     # all other flashes
#     smaller_dataset = data1 if len(data1) < len(data2) else data2
#     larger_dataset = data2 if len(data1) < len(data2) else data1
#     factor = lambda x: x if len(data1) < len(data2) else 1 / x

#     for i, (_, row) in enumerate(smaller_dataset.iterrows(), 1):

#         try:

#             y_distances = (larger_dataset["y"] - row.loc["y"]).abs()
#             closest_y = larger_dataset.iloc[y_distances.argmin()]["y"]

#             same_row = larger_dataset[larger_dataset["y"] == closest_y]
#             same_row["x_distances"] = (same_row["x"] - row.loc["x"]).abs()

#             closest_xy = same_row.iloc[same_row["x_distances"].argmin()]

#             closest_distance = np.sqrt(
#                 (closest_xy["y"] - row["y"]) ** 2 + (closest_xy["x"] - row["x"]) ** 2
#             )
#             if closest_distance > 10:
#                 continue

#             ratio = factor(row["FDeventSum"] / closest_xy["FDeventSum"])
#             this_ratio = pd.DataFrame(
#                 {"x": row["x"], "y": row["y"], "ratio": ratio}, index=[i]
#             )
#             ratios = pd.concat([ratios, this_ratio], ignore_index=True)
#         except ValueError:
#             pass

#     return ratios


# def get_run_numbers(telescope: str, date: str) -> dict:

#     import pickle

#     with open("/cr/users/filip/bin/utils/Auger/FD/xy_measurements.pkl", "rb") as f:
#         data = pickle.load(f)

#     try:
#         requested_telescope = data[telescope]
#         for measurement in requested_telescope:
#             if measurement["date"] == date:
#                 return measurement
#         else:
#             raise KeyError

#     except KeyError:

#         logger = create_stream_logger("XY-logger", logging.ERROR)
#         logger.error(f"requested dataset does not exist! {telescope = }, {date = }")
#         return {}


# def update_runlist_files() -> None:

#     import pickle

#     # add new runlists here!
#     runlists_after_oct_2022 = ["2023-10", "2023-11"]

#     runlist = pd.read_csv(
#         f"/cr/data01/filip/xy-calibration/config/calib_runlists/calib_runs_2022-10.list",
#         sep=";",
#     )
#     runlist["comment"] = [comment.strip() for comment in runlist["comment"]]
#     runlist["source"] = [source.strip() for source in runlist["source"]]

#     for month in runlists_after_oct_2022:
#         this_runlist = pd.read_csv(
#             f"/cr/data01/filip/xy-calibration/config/calib_runlists/calib_runs_{month}.list",
#             sep=";",
#         )
#         this_runlist["comment"] = [
#             comment.strip() for comment in this_runlist["comment"]
#         ]
#         this_runlist["source"] = [source.strip() for source in this_runlist["source"]]

#         runlist = pd.concat([runlist, this_runlist], ignore_index=True)

#     only_xy_runs = runlist
#     only_xy_runs = runlist.drop(runlist[runlist["comment"] != '""'].index)
#     only_xy_runs = only_xy_runs.drop(
#         only_xy_runs[only_xy_runs["source"] != '"OLO"'].index
#     )
#     only_xy_runs = only_xy_runs.drop(only_xy_runs[only_xy_runs["forDB"] == 0].index)

#     XY_measurements, n_runs = {}, 0
#     for index, measurement in only_xy_runs.iterrows():

#         try:
#             _ = XY_measurements[measurement["telescope"].strip().upper()]
#         except KeyError:
#             XY_measurements[measurement["telescope"].strip().upper()] = []

#         (runid, telescope), date = measurement[:2], measurement["date"].strip()
#         telescope = telescope.strip().upper()

#         same_day = runlist[[d.strip() == date for d in runlist["date"]]]
#         same_telescope = same_day[
#             [t.strip().upper() == telescope for t in same_day["telescope"]]
#         ]

#         year, month, day = date.split("-")

#         XY_measurement = {
#             "XY": measurement["#runid"],
#             "CalA_closed_shutter": [None, None],
#             "CalA_open_shutter": [None, None],
#             "date": f"{year}-{month}-{day}",
#         }

#         # find Cal A closed shutter
#         for candidate_index, candidate in same_telescope.iterrows():
#             if candidate["comment"] == '"Cal A"':
#                 if candidate_index < index:
#                     XY_measurement["CalA_closed_shutter"][0] = candidate["#runid"]
#                 if candidate_index > index:
#                     XY_measurement["CalA_closed_shutter"][1] = candidate["#runid"]
#             if candidate["comment"] == '"Cal A open shutter"':
#                 if candidate_index < index:
#                     XY_measurement["CalA_open_shutter"][0] = candidate["#runid"]
#                 if candidate_index > index:
#                     XY_measurement["CalA_open_shutter"][1] = candidate["#runid"]

#         XY_measurements[telescope].append(XY_measurement)
#         n_runs += 1

#     with open("/cr/users/filip/bin/utils/Auger/FD/xy_measurements.pkl", "wb") as f:
#         pickle.dump(XY_measurements, f, pickle.HIGHEST_PROTOCOL)

#     print(
#         f"{len(XY_measurements.keys())} bays: {n_runs} written to xy_measurements.pkl"
#     )
