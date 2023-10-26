import numpy as np
from scipy.optimize import curve_fit
import scipy.stats as stats

def offline_MIP(x_min, x_max, rates):

    rates_normalized = rates / np.sum(rates)

    landau = lambda x, A, loc, scale: A * stats.moyal(loc, scale).pdf(x)

    mu = -np.log(rates_normalized[x_max] / rates_normalized[x_min]) / (x_max - x_min)

    background = lambda x: rates_normalized[x_min] * np.exp(-mu * (x-x_min))
    landau = lambda x, A, loc, scale: A * stats.moyal(loc, scale).pdf(x)
    residuals = rates_normalized[x_min:x_max] - background(np.arange(x_min, x_max,1))

    popt, pcov = curve_fit(landau, range(x_min, x_max), residuals)

    # Calculate uncertainty in mean from MC analysis
    means = []

    for i in range(1000):
        mean = np.random.normal(popt[1], np.sqrt(np.diag(pcov)[1]))
        std = np.random.normal(popt[2], np.sqrt(np.diag(pcov)[2]))
        means.append(stats.moyal(mean, std).mean())

    mean, sigma = np.mean(means), np.std(means)

    return mean, sigma

def online_MIP(target, rates, station_time):

    differences = np.abs(rates - target)
    freq_error = np.sqrt(rates * station_time) / station_time

    best_rate = np.argmin(differences)
    this_error = freq_error[best_rate]
    propagated = np.argmin(np.abs(target - this_error - rates))
    propagated -= np.argmin(np.abs(target + this_error - rates))

    return np.argmin(differences), propagated