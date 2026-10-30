# Copyright(C) 2026 Netflix, Inc.
"""Legacy A/B plotting; receives analyzed results rather than opening files."""
import pandas
import matplotlib.pylab
import numpy as np
import statsmodels.stats.api as sms
from ab_statistics import create_df_one_subject_system_wins_per_trial, create_df_one_subject_comparison_scores_per_trial

def create_xticks(ax, df):
    xticks = np.linspace(-0.5, df.size - .5, num=len(df.index) + 1)
    ax.set_xticks(xticks, minor=True)

    xticks = np.linspace(float(len(df.columns)) / 2, df.size - float(len(df.columns)) / 2, num=len(df.index))
    ax.set_xticks(xticks)
    ax.set_xticklabels(df.index)
    ax.tick_params(axis='x', which='major', length=0, labelsize=12)
    ax.tick_params(axis='x', rotation=75)

    return ax



def plot_dataframe_errorbar(ax, df, errorbar_df):
    ax = create_xticks(ax, df)

    # Plot each system with errorbar.
    for i, system in enumerate(df.columns):
        indices = range(i, df.size, len(df.columns))
        real_bars = errorbar_df[i]
        matplotlib.pylab.errorbar(indices, df[system].values, xerr=0, yerr=real_bars, fmt='o', capsize=5,
                                  label=system,
                                  Axes=ax)

    return ax


# This cell to plot all users/trials in one graph.  averages the scores of reference for all trials, which may be misleading!
# noinspection PyUnusedLocal

def plot_one_system(dict_of_dfs, ax, system, ref_system='reference'):
    idx = pandas.IndexSlice  # useful for slicing sections of the dataframe

    # Concatenate all user dataframes into a multi-index dataframe
    df = pandas.concat(dict_of_dfs, keys=dict_of_dfs.keys(), names=['Listener', 'Item'])

    # Re-sort dataframes according to mean score
    sorting = df.mean().sort_values().index
    df = df.reindex(sorting, axis=1)

    # sort items by user
    df.sort_index(inplace=True)

    # compute diff amounts to the reference system
    if ref_system != 'reference':
        df = df.sub(df[ref_system], axis=0)

    # Create summary chart containing mean scores across all listeners
    summary_df = df.mean(axis=0, level=1)

    # Compute 95% Confidence intervals for each system
    conf_per_item_system = pandas.DataFrame(columns=summary_df.columns)
    for item in df.index.levels[1]:
        itemchart = df.loc[idx[:, [item]], :]
        conf = sms.DescrStatsW(itemchart).tconfint_mean(alpha=0.05) - itemchart.mean().values
        conf_per_item_system.loc[item] = conf[1]

    # Add Mean row to confidence & summary score dataframes
    meanconf = sms.DescrStatsW(df).tconfint_mean(alpha=0.1) - df.mean().values
    conf_per_item_system.loc['mean'] = meanconf[1]
    summary_df.loc['mean'] = df.mean()

    # Setup a chart with labels for each test item on x-axis
    ax = create_xticks(ax, summary_df)

    # Plot each system with errorbar.
    for i, system in enumerate(summary_df.columns):
        indices = range(i, summary_df.size, len(summary_df.columns))
        realbars = conf_per_item_system[system]
        matplotlib.pylab.errorbar(indices, summary_df[system].values, xerr=0, yerr=realbars, fmt='o', capsize=5,
                                  label=system, Axes=ax)

    # Add a legend
    ax.grid(True, axis='y')
    ax.grid(True, axis='x', which='minor', color='k', linestyle=':', alpha=.25)
    ax.legend(bbox_to_anchor=(1, 1), fontsize=14)

    return ax



def plot_all(dict_of_dfs, ax, ref_system='reference', title='AB Preference listening test'):
    idx = pandas.IndexSlice  # useful for slicing sections of the dataframe

    # Concatenate all user dataframes into a multi-index dataframe
    df = pandas.concat(dict_of_dfs, keys=dict_of_dfs.keys(), names=['Listener', 'Item'])

    # Re-sort dataframes according to mean score
    sorting = df.mean().sort_values().index
    df = df.reindex(sorting, axis=1)

    # sort items by user
    df.sort_index(inplace=True)

    # compute diff amounts to the reference system
    if ref_system != 'reference':
        df = df.sub(df[ref_system], axis=0)

    # Create summary chart containing mean scores across all listeners
    summary_df = df.mean(axis=0, level=1)

    # Compute 95% Confidence intervals for each system
    conf_per_item_system = pandas.DataFrame(columns=summary_df.columns)
    for item in df.index.levels[1]:
        itemchart = df.loc[idx[:, [item]], :]
        conf = sms.DescrStatsW(itemchart).tconfint_mean(alpha=0.05) - itemchart.mean().values
        conf_per_item_system.loc[item] = conf[1]

    # Add Mean row to confidence & summary score dataframes
    meanconf = sms.DescrStatsW(df).tconfint_mean(alpha=0.1) - df.mean().values
    conf_per_item_system.loc['mean'] = meanconf[1]
    summary_df.loc['mean'] = df.mean()

    # Setup a chart with labels for each test item on x-axis
    ax = create_xticks(ax, summary_df)

    # Plot each system with errorbar.
    for i, system in enumerate(summary_df.columns):
        indices = range(i, summary_df.size, len(summary_df.columns))
        realbars = conf_per_item_system[system]
        matplotlib.pylab.errorbar(indices, summary_df[system].values, xerr=0, yerr=realbars, fmt='o', capsize=5,
                                  label=system, Axes=ax)

    # Add a legend
    ax.grid(True, axis='y')
    ax.grid(True, axis='x', which='minor', color='k', linestyle=':', alpha=.25)
    ax.set_title(title + ': ' + str(len(dict_of_dfs)) + ' listeners')
    ax.legend(bbox_to_anchor=(1, 1), fontsize=14)

    return ax



def plot_sys_wpt(sys_wpt_dict, ax):
    idx = pandas.IndexSlice  # useful for slicing sections of the dataframe

    # Concatenate all user dataframes into a multi-index dataframe
    df = pandas.concat(sys_wpt_dict, keys=sys_wpt_dict.keys(), names=['Subject', 'Stimuli'])

    # Re-sort dataframes according to mean score
    sorting = df.mean().sort_values().index
    df = df.reindex(sorting, axis=1)

    # sort items by user
    df.sort_index(inplace=True)


