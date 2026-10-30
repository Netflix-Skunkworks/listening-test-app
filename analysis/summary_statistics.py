# Copyright(C) 2026 Netflix, Inc.
"""Statistical calculations shared by rating plots, independent of rendering."""
from statsmodels.stats.weightstats import DescrStatsW


def confidence_offsets(scores, alpha=0.05):
    """Two-sided Student t intervals around the observed mean, as offsets."""
    mean = scores.mean()
    if hasattr(mean, 'values'):
        mean = mean.values
    return DescrStatsW(scores).tconfint_mean(alpha=alpha) - mean
