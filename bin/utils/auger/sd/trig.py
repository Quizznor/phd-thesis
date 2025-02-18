from ...binaries import np

def filter_and_downsample(*traces: np.ndarray, random_phase: int = 1) -> list:
    """convert UUB trace to UB equivalent, for compatibility mode"""

    filtered_and_downsampled_traces = []

    # see Framework/SDetector/UUBDownsampleFilter.h in Offline main branch for more information
    kFirCoefficients = np.array(
        [
            5,
            0,
            12,
            22,
            0,
            -61,
            -96,
            0,
            256,
            551,
            681,
            551,
            256,
            0,
            -96,
            -61,
            0,
            22,
            12,
            0,
            5,
        ]
    )
    buffer_length = int(0.5 * len(kFirCoefficients))
    kFirNormalizationBitShift = 11
    kADCSaturation = 4095
    kFirLen = len(kFirCoefficients)

    for pmt in traces:
        temp = np.zeros(len(pmt) + len(kFirCoefficients), dtype=int)
        temp[0:buffer_length] = pmt[::-1][-buffer_length - 1 : -1]
        temp[-buffer_length - 1 : -1] = pmt[::-1][0:buffer_length]
        temp[buffer_length : -buffer_length - 1] = pmt

        temp_shifted = np.array([temp[k : k + len(pmt)] for k in range(kFirLen)])
        outer_product = temp_shifted * kFirCoefficients[:, np.newaxis]

        trace = np.sum(outer_product, axis=0)
        trace = np.clip(
            np.right_shift(trace, kFirNormalizationBitShift), 0, kADCSaturation
        )

        trace = trace[random_phase::3]
        filtered_and_downsampled_traces.append(np.array(trace, dtype="u2"))

    return filtered_and_downsampled_traces


def threshold_trigger(
    traces: np.ndarray, threshold: float, latch_bin: bool = False
) -> bool:
    pmt1, pmt2, pmt3 = traces

    assert (
        (trace_length := len(pmt1)) == len(pmt2) == len(pmt3)
    ), "Unrealistic time trace"

    for _b in range(trace_length):
        if pmt1[_b] >= threshold:
            if pmt2[_b] >= threshold:
                if pmt3[_b] >= threshold:
                    return _b if latch_bin else True

    return -1 if latch_bin else False


def wcd_t1_trigger(traces: np.ndarray, latch_bin: bool = False) -> bool:
    return threshold_trigger(traces, threshold=1.75, latch_bin=latch_bin)


def wcd_t2_trigger(traces: np.ndarray, latch_bin: bool = False) -> bool:
    return threshold_trigger(traces, threshold=3.2, latch_bin=latch_bin)
