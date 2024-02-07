import numpy as np
import pandas
import statsmodels.api
import statsmodels.stats.weightstats
import matplotlib
import matplotlib.pylab
import matplotlib.pyplot


def create_xticks(ax, df):
    xticks = np.linspace(-0.5, df.size - .5, num=len(df.index) + 1)
    ax.set_xticks(xticks, minor=True)

    xticks = np.linspace(float(len(df.columns)) / 2, df.size - float(len(df.columns)) / 2, num=len(df.index))
    ax.set_xticks(xticks)
    ax.set_xticklabels(df.index)
    ax.tick_params(axis='x', which='major', length=0, labelsize=14, labelrotation=70)

    return ax


def plot_dataframe_errorbar(ax, df, errbar_df):
    ax = create_xticks(ax, df)

    # Plot each codec with error bar.
    for i, codec in enumerate(df.columns):
        indices = range(i, df.size, len(df.columns))
        real_bars = abs(errbar_df[i])
        matplotlib.pylab.errorbar(indices, df[codec].values, xerr=0, yerr=real_bars, fmt='o', capsize=5, label=codec, axes=ax)

    # Add a legend
    ax.grid('on', axis='y')
    ax.grid('on', axis='x', which='minor', color='k', linestyle=':', alpha=.25)
    ax.legend(bbox_to_anchor=(1, 1))

    return ax


def plot_one_stimulus(df_list, output_filename, stimulus):
    fig = matplotlib.pylab.figure()
    ax = fig.add_axes([0.1, 0.15, 0.7, 0.7])
    filtered_df_list = {}
    for df in df_list:
        filtered_df_list[df] = df_list[df].copy().filter(axis=0, items=[stimulus])

    # Concatenate all user dataframes into a multi-index dataframe
    df = pandas.concat(filtered_df_list, keys=filtered_df_list.keys(), names=['Listener', 'Stimulus'])

    # transpose so we see individual subjects in the X-axis
    df.transpose()

    # sort codecs by score, ascending
    df.sort_index(axis=0, inplace=True)
    sorting = df.mean().sort_values().index
    df = df.reindex(sorting, axis=1)

    # compute confidence intervals for mean scores
    conf_per_codec = statsmodels.stats.weightstats.DescrStatsW(df).tconfint_mean(alpha=0.05) - df.mean().values

    # NOTE: below function computes "wrong" confidence level when dataset is too small?
    # confidence = st.norm.interval(0.95, loc=np.mean(df['xheaac_vbr4']), scale=st.sem(df['xheaac_vbr4']))

    # Add a row containing the mean score
    df.loc['mean'] = df.mean()

    bar_size = list(range(len(df.columns)))
    for i, codec in enumerate(df.columns):
        # Error bars fixed to zero for all except the mean values
        b = np.zeros(len(df.index) - 1)
        b = np.append(b, conf_per_codec[0][i])
        bar_size[i] = b

    ax = plot_dataframe_errorbar(ax, df, bar_size)
    ax.autoscale(enable='true')
    ax.set_title(stimulus)
    fig.savefig(output_filename)
    matplotlib.pyplot.close()

    return ax


# plot results and compare listeners
def plot_all_listeners(df_list, output_filename, ref_codec='reference', title='listening_test'):
    fig = matplotlib.pylab.figure()
    ax = fig.add_axes([0.1, 0.15, 0.7, 0.7])

    idx = pandas.IndexSlice  # useful for slicing sections of the dataframe

    # Concatenate all user dataframes into a multi-index dataframe
    df = pandas.concat(df_list, keys=df_list.keys(), names=['Listener', 'Stimulus'])

    if ref_codec != 'reference':
        df = df.sub(df[ref_codec], axis=0)

    # Re-sort dataframes according to mean score
    sorting = df.mean().sort_values().index
    df = df.reindex(sorting, axis=1)

    # sort stimuli by user
    df.sort_index(inplace=True)

    # compute diff amounts to the reference codec
    if ref_codec != 'reference':
        df = df.sub(df[ref_codec], axis=0)

    # Create summary chart containing mean scores across all stimuli
    summary_df = df.groupby(level=0).mean()

    print(summary_df)

    # Compute 95% Confidence intervals for each codec
    conf_per_subject_codec = pandas.DataFrame(columns=summary_df.columns)
    for subject in df.index.levels[0]:
        subject_chart = df.loc[idx[[subject], :], :]
        conf = get_conf_from_dataframe(subject_chart, alpha_value=0.05)
        conf_per_subject_codec.loc[subject] = conf

    # Add Mean row to confidence & summary score dataframes
    conf_per_subject_codec.loc['mean'] = get_conf_from_dataframe(df, alpha_value=0.05)
    summary_df.loc['mean'] = df.mean()  # df.mean() will ignore nan by default

    # Setup a chart with labels for each test stimulus on x-axis
    ax = create_xticks(ax, summary_df)

    # Plot each codec with errorbar.
    for i, codec in enumerate(summary_df.columns):
        # Errorbars fixed to zero for all except the mean values
        real_bars = abs(conf_per_subject_codec[codec])

        indices = range(i, summary_df.size, len(summary_df.columns))
        h = matplotlib.pylab.errorbar(indices, summary_df[codec].values, xerr=0, yerr=real_bars, fmt='o', capsize=5,
                                      label=codec, axes=ax)

    # Add a legend
    ax.grid('on', axis='y')
    ax.grid('on', axis='x', which='minor', color='k', linestyle=':', alpha=.25)
    ax.legend(bbox_to_anchor=(1, 1), fontsize='12')
    ax.set_title(title + ': ' + str(len(df_list)) + ' listeners')
    fig.savefig(output_filename)

    return ax


def get_conf_from_dataframe(chart, alpha_value=0.05):
    """
    This function computes confidence interval for each treatment (or codec) and ignores nan (missing data)
    Parameters
    ----------
    chart: a dataframe object from pandas, typically structured as scores x treatment
    alpha_value: significance level for the confidence interval, coverage is 1-alpha (from tconfint_mean)

    Returns: list of confidence intervals; one value per treatment
    -------
    """
    conf = []

    # special-case code to compute correct error bars when the test results are sparse
    if chart.isnull().values.any():
        for codec, scores in chart.items():  # iteritems was deprecated since pandas 2.0.0
            newScores = scores.dropna()  # newScores can be empty when some of the entries are incomplete
            if len(newScores) == 0:
                newScores = scores.fillna(0)  # this will lead to conf interval = 0
                print("Results contain incomplete entries: ignore missing values for conf interval estimation")
            conf.append(
                (statsmodels.stats.weightstats.DescrStatsW(newScores).tconfint_mean(alpha=alpha_value) - newScores.mean())[1])  # index 1 is to take positive value only
    else:
        conf = (statsmodels.stats.weightstats.DescrStatsW(chart).tconfint_mean(
            alpha=alpha_value) - chart.mean().values)[1]
    return conf


# diff-plot a single user
def plot_one_subject(df, output_filename, subject, ref_codec='reference'):
    fig = matplotlib.pylab.figure()
    ax = fig.add_axes([0.1, 0.15, 0.7, 0.7])

    if ref_codec != 'reference':
        df = df.sub(df[ref_codec], axis=0)

    # sort codecs by mean score, ascending
    df.sort_index(axis=0, inplace=True)
    sorting = df.mean().sort_values().index
    df = df.reindex(sorting, axis=1)

    # compute confidence intervals for mean scores
    conf_per_codec = statsmodels.stats.weightstats.DescrStatsW(df).tconfint_mean(alpha=0.05) - df.mean().values

    # NOTE: below function computes "wrong" confidence level when dataset is too small?
    # confidence = st.norm.interval(0.95, loc=np.mean(df['xheaac_vbr4']), scale=st.sem(df['xheaac_vbr4']))

    # Add a row containing the mean score
    df.loc['mean'] = df.mean()

    bar_size = list(range(len(df.columns)))
    for i, codec in enumerate(df.columns):
        # Error bars fixed to zero for all except the mean values
        b = np.zeros(len(df.index) - 1)
        b = np.append(b, conf_per_codec[0][i])
        bar_size[i] = b

    ax = plot_dataframe_errorbar(ax, df, bar_size)
    ax.autoscale(enable='true')
    ax.set_title(subject)
    fig.savefig(output_filename)
    return ax


# plot all items
def plot_all(df_list, output_filename, ref_codec='reference', title='listening_test'):
    fig = matplotlib.pylab.figure()
    ax = fig.add_axes([0.1, 0.15, 0.8, 0.8])

    idx = pandas.IndexSlice  # useful for slicing sections of the dataframe

    # Concatenate all user dataframes into a multi-index dataframe
    df = pandas.concat(df_list, keys=df_list.keys(), names=['Listener', 'Stimulus'])

    if ref_codec != 'reference':
        df = df.sub(df[ref_codec], axis=0)

    # Re-sort dataframes according to mean score
    sorting = df.mean().sort_values().index
    df = df.reindex(sorting, axis=1)

    # sort stimuli by user
    df.sort_index(inplace=True)

    # compute diff amounts to the reference codec
    if ref_codec != 'reference':
        df = df.sub(df[ref_codec], axis=0)

    # Create summary chart containing mean scores across all listeners
    summary_df = df.groupby(level=1).mean()

    print(summary_df)

    # Compute 95% Confidence intervals for each codec
    conf_per_stimulus_codec = pandas.DataFrame(columns=summary_df.columns)
    for stimulus in df.index.levels[1]:
        stimulus_chart = df.loc[idx[:, [stimulus]], :]
        conf = get_conf_from_dataframe(stimulus_chart, alpha_value=0.05)
        conf_per_stimulus_codec.loc[stimulus] = conf

    # Add Mean row to confidence & summary score dataframes
    conf_per_stimulus_codec.loc['mean'] = get_conf_from_dataframe(df, alpha_value=0.05)
    summary_df.loc['mean'] = df.mean()

    # Setup a chart with labels for each test stimulus on x-axis
    ax = create_xticks(ax, summary_df)

    # Plot each codec with errorbar.
    for i, codec in enumerate(summary_df.columns):
        # Errorbars fixed to zero for all except the mean values
        real_bars = abs(conf_per_stimulus_codec[codec])

        indices = range(i, summary_df.size, len(summary_df.columns))
        h = matplotlib.pylab.errorbar(indices, summary_df[codec].values, xerr=0, yerr=real_bars, fmt='o', capsize=5,
                                      label=codec, axes=ax)

    # Add a legend
    ax.grid('on', axis='y')
    ax.grid('on', axis='x', which='minor', color='k', linestyle=':', alpha=.25)
    ax.legend(bbox_to_anchor=(1, 1), fontsize='12')
    ax.set_title(title + ': ' + str(len(df_list)) + ' listeners')

    fig.savefig(output_filename)

    return ax

