import numpy as np
from scipy.optimize import curve_fit
import scipy.stats as stats

def offline_MIP(x_min, x_max, rates, station, ax = None):

    rates_normalized = rates / np.sum(rates)
    rates_normalized_error = rates_normalized[x_min:x_max] * 0.012
    mu = -np.log(rates_normalized[x_max] / rates_normalized[x_min]) / (x_max - x_min)

    background = lambda x: rates_normalized[x_min] * np.exp(-mu * (x-x_min))
    landau = lambda x, A, loc, scale: A * stats.moyal(loc, scale).pdf(x)
    residuals = rates_normalized[x_min:x_max] - background(np.arange(x_min, x_max,1))

    popt, pcov = curve_fit(landau, range(x_min, x_max), residuals, sigma=rates_normalized_error)

    # Calculate uncertainty in mean from MC analysis
    upper, lower = [landau(range(x_min, x_max), *popt) for _ in range(2)]
    means = []

    for i in range(1000):
        mean = np.random.normal(popt[1], np.sqrt(np.diag(pcov)[1]))
        std = np.random.normal(popt[2], np.sqrt(np.diag(pcov)[2]))
        means.append(stats.moyal(mean, std).mean())

        if ax is not None:
            this_pdf = landau(range(x_min, x_max), popt[0], mean, std)
            upper, lower = np.maximum(upper, this_pdf), np.minimum(lower, this_pdf)

    mean, sigma = np.mean(means), np.std(means)

    if ax is not None:
        kwargs = {'ls' : None, 'capsize' : 1, 'fmt' : 'o', 'markersize' : 0.6,  'elinewidth' : 0.2, 'capthick' : 0.2}
        c = {'NuriaJrFilteredDownsampled' : 'green', 'NadiaLateFilteredDownsampled' : 'tomato', 'NadiaEarlyFilteredDownsampled' : 'rebeccapurple'}
        ax.errorbar(range(x_min, x_max), residuals, yerr=rates_normalized_error, lw=2, c=c[station], label = station, **kwargs)
        ax.plot(range(x_min, x_max), landau(range(x_min, x_max), *popt), ls='--', c =c[station])
        ax.fill_between(range(x_min, x_max), lower, upper, color=c[station], alpha=0.1)

    return mean, sigma

def online_MIP(target, rates, station_time):

    differences = np.abs(rates - target)
    freq_error = np.sqrt(rates * station_time) / station_time

    best_rate = np.argmin(differences)
    this_error = freq_error[best_rate]
    propagated = np.argmin(np.abs(target - this_error - rates))
    propagated -= np.argmin(np.abs(target + this_error - rates))

    return np.argmin(differences), propagated