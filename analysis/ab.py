import os
import pandas
import matplotlib.pylab
import xml.etree.ElementTree as et
import glob
import numpy as np
import statsmodels.stats.api as sms


system_names = ['system1', 'system2']

# The minimum number of seconds that a subject should spend listening to any trial
# ideally this would be double the length of the shortest file in the test
minimum_test_duration = 15


# extract system identifier from filename
# depends on several assumptions about the filename conventions
def add_system_to_result_xml(results):
    # clean up data: get system name from filenames
    trials = results.find('trials')
    for trial in trials.findall('trial'):
        trialName = trial.get('trialName')
        for f in trial.findall('testFile'):
            system = f.get('fileName')
            if system.endswith('.wav'):
                system = system[:-9]  # removes all of '_24.0.wav'
            if system.startswith(trialName + '_'):
                system = system[len(trialName) + 1:]
            else:
                i = system.rindex(".")
                system = system[i + 1:]
            f.set('system', system)

    return results


def add_comparison_to_result_xml(results):
    trials = results.find('trials')
    for trial in trials.findall('trial'):
        comparison = []
        for f in trial.findall('testFile'):
            comparison.append(f.get('system'))

        comparison.sort()

        first_sys = trial.findall('testFile')[0].get('system')
        first_score = trial.findall('testFile')[0].get('score')

        win = False
        if comparison[0] == first_sys:
            win = first_score == '1.0'
        else:
            win = first_score != '1.0'

        trial.set('comparison', '_'.join(map(str, comparison)))  # remove trailing underscore
        trial.set('win', win)

    return results


# crate dataframe for analyzing specific comparisons
# trial -> comparison -> win & total trial count
# For a comparison notated as 'a_b', that is system a vs system b
# 'win' == True referenced system A.  So if 'win' == False, system B won.
def create_df_one_subject_comparison_scores_per_trial(results_xml_with_comparisons):
    # create top dimension of dictionary: stimuli
    comparisons = {}
    trials = results_xml_with_comparisons.find('trials')
    for trial in trials.findall('trial'):
        stimuli_name = trial.get('trialName')
        if not (stimuli_name in comparisons):
            comparisons[stimuli_name] = {}
        comparisons[stimuli_name][trial.get('comparison')] = trial.get('win')

    df = pandas.DataFrame.from_dict(comparisons, orient='index')
    df = df.reindex(sorted(df.columns), axis=1)  # sort by system name

    return df


# create dataframe for this subject: trial -> system -> score
# This function populates each [trial][system] with the total number of wins that system tallied
# to find overall preference for a system per trial.
# not useful for analyzing specific system comparisons
# Also tallies total trials so winrate can be calculated with ease
def create_df_one_subject_system_wins_per_trial(results_xml_with_systems):
    # create top dimension of dictionary: stimuli
    systems = {}
    trials = results_xml_with_systems.find('trials')
    for trial in trials.findall('trial'):
        stimuli_name = trial.get('trialName')
        if not (stimuli_name in systems):
            systems[stimuli_name] = {}
        for f in trial.findall('testFile'):
            system = f.get('system')
            if not (system in systems[stimuli_name]):
                systems[stimuli_name][system] = {}
            systems[stimuli_name][system]['wins'] = 0
            systems[stimuli_name][system]['trials'] = 0
    #            systems[stimuli_name][f.get('system')] = 0

    # now populate trials with system scores
    for trial in trials.findall('trial'):
        for i, f in enumerate(trial.findall('testFile')):
            systems[trial.get("trialName")][f.get('system')]['wins'] += float(f.get('score'))
            systems[trial.get("trialName")][f.get('system')]['trials'] += 1

    df = pandas.DataFrame.from_dict(systems, orient='index')
    df = df.reindex(sorted(df.columns), axis=1)  # sort by system name

    return df


# def plot_overall_wins(ax, df_wins_per_trial):


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


def print_full_df(df):
    pandas.set_option('display.max_rows', None)
    pandas.set_option('display.max_columns', None)
    pandas.set_option('display.width', None)
    pandas.set_option('display.max_colwidth', None)
    print(df)


# This cell to plot all users/trials in one graph.  averages the scores of reference for all trials, which may be misleading!
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


def check_trial_validity(results_xml):
    trials = results_xml.find('trials')
    badDurationTrials = []
    skippedSystemTrials = []
    for trial in trials.findall('trial'):
        removeTrial = False
        duration = float(trial.get('trialSeconds'))
        if duration < minimum_test_duration:
            badDurationTrials.append(trial)
            removeTrial = True

        for f in trial.findall('testFile'):
            if int(f.get('plays')) < 1:
                skippedSystemTrials.append(trial)
                removeTrial = True

        if removeTrial:
            trials.remove(trial)

    return {'badDurationTrialsList': badDurationTrials,
            'skippedSystemTrialsList': skippedSystemTrials,
            'filteredResultsXml': results_xml}


def print_validity_check_results(subject, validity_check_results):
    print('Validity check for subject ' + subject + ':')
    if len(validity_check_results['skippedSystemTrialsList']) > 0:
        print('\t Did not listen to both stimuli:')
        fileList = []
        for trial in validity_check_results['skippedSystemTrialsList']:
            stimuli = trial.get('trialName')
            systems = []
            for f in trial.findall('testFile'):
                systems.append(f.get('system'))
            systems.sort()
            fileList.append('\t\t' + stimuli + ': ' + systems[0] + ' vs. ' + systems[1] + ' (' + trial.get(
                'trialSeconds') + ' sec.)')

        fileList.sort()
        for f in fileList:
            print(f)

    if len(validity_check_results['badDurationTrialsList']):
        print('\t Not enough time spent listening to trial (minimum ' + str(minimum_test_duration) + ' seconds):')
        fileList = []
        for trial in validity_check_results['badDurationTrialsList']:
            stimuli = trial.get('trialName')
            systems = []
            for f in trial.findall('testFile'):
                systems.append(f.get('system'))
            systems.sort()
            fileList.append('\t\t' + stimuli + ': ' + systems[0] + ' vs. ' + systems[1] + ' (' + trial.get(
                'trialSeconds') + ' sec.)')

        fileList.sort()
        for f in fileList:
            print(f)


def get_system_overall_winrate(all_results, system):
    system_wins = 0
    system_comps = 0
    for subject in all_results:
        subject_winrate = get_system_winrate(all_results[subject], system)
        system_wins += subject_winrate['wins']
        system_comps += subject_winrate['comps']

    return {'wins': system_wins,
            'comps': system_comps,
            'winrate': system_wins / system_comps}


def get_system_winrate(subject_results, system):
    wins = 0
    comps = 0
    trials = subject_results.find('trials')
    for trial in trials.findall('trial'):
        for f in trial.findall('testFile'):
            if f.get('system') == system:
                comps += 1
                if f.get('score') == '1.0':
                    wins += 1

    return {'wins': wins,
            'comps': comps,
            'winrate': wins / comps}


def get_overall_winrate_vs_each_system(all_results, system):
    opposing_systems = {}

    for subject in all_results:
        trials = all_results[subject].find('trials')
        for trial in trials.findall('trial'):
            for f in trial.findall('testFile'):
                if (f.get('system') != system) & (f.get('system') not in opposing_systems):
                    opposing_systems[f.get('system')] = {'wins': 0, 'comps': 0}

    opposing_systems = dict(sorted(opposing_systems.items(), key=lambda item: item[0]))

    for subject in all_results:
        subject_oppo_systems = get_subject_winrate_vs_each_system(all_results[subject], system)

        for oppo_system in opposing_systems:
            opposing_systems[oppo_system]['wins'] += subject_oppo_systems[oppo_system]['wins']
            opposing_systems[oppo_system]['comps'] += subject_oppo_systems[oppo_system]['comps']

    return opposing_systems


def get_subject_overall_winrate(subject_results, system):
    system_wins = 0
    system_comps = 0

    trials = subject_results.find('trials')
    for trial in trials.findall('trial'):
        for f in trial.findall('testFile'):
            if f.get('system') == system:
                system_comps += 1
                if f.get('score') == '1.0':
                    system_wins += 1

    return {'wins': system_wins,
            'comps': system_comps,
            'winrate': system_wins / system_comps}


def get_subject_winrate_vs_each_system(subject_results, system):
    return get_winrate_vs_each_system(subject_results.find('trials'), system)


def get_winrate_vs_each_system(trials, system):
    opposing_systems = {}

    for trial in trials:
        for f in trial.findall('testFile'):
            if (f.get('system') != system) & (f.get('system') not in opposing_systems):
                opposing_systems[f.get('system')] = {'wins': 0, 'comps': 0}

    opposing_systems = dict(sorted(opposing_systems.items(), key=lambda item: item[0]))

    for trial in trials:
        foundSystem = False
        for f in trial.findall('testFile'):
            if f.get('system') == system:
                foundSystem = True
        if foundSystem:
            for f in trial.findall('testFile'):
                if f.get('system') != system:
                    opposing_systems[f.get('system')]['comps'] += 1
                    if f.get('score') == '0.0':
                        opposing_systems[f.get('system')]['wins'] += 1

    total_wins = 0
    total_comps = 0
    for system in opposing_systems:
        total_wins += opposing_systems[system]['wins']
        total_comps += opposing_systems[system]['comps']

    opposing_systems['all'] = {'wins': total_wins, 'comps': total_comps, 'winrate': total_wins/total_comps}

    return opposing_systems


def get_all_stimuli_winrates(all_results):
    stimuli = {}
    for subject in all_results:
        trials = all_results[subject].find('trials')
        for trial in trials.findall('trial'):
            trialName = trial.get('trialName')
            if trialName not in stimuli:
                stimuli[trialName] = []
            stimuli[trialName].append(trial)

    all_stimuli_results = {}
    for stimulus in stimuli:
        stimulus_results = {}
        for system in {'I', 'II', 'III', 'IV', 'V'}:
            stimulus_results[system] = get_winrate_vs_each_system(stimuli[stimulus], system)

        all_stimuli_results[stimulus] = stimulus_results

    return all_stimuli_results


def print_winrate_vs_each_system(opposing_systems, system_name):
    for system in opposing_systems:
        wins = opposing_systems[system]['wins']
        comps = opposing_systems[system]['comps']
        if comps > 0:
            print(system_name + ' win rate vs. System ' + system + ': ' + str(round(wins / comps, 2)) + ' (' + str(
                wins) + ' out of ' + str(comps) + ')')


# noinspection PyUnusedLocal
def add_info_to_xml():
    data_dir = '/Users/phill/Projects/aa-listening-tests/bsu_binaural_2020_final_data/AV/'
    file_list = glob.glob(data_dir + '**/*.xml', recursive=True)

    #    print('files found: ' + str(file_list))
    #    matplotlib.pylab.rcParams['figure.figsize'] = 16, 12

    all_results = {}

    # Collect results into a dictionary and filter out bad trials
    for file in file_list:
        # print("handling " + str(file))

        tree = et.parse(os.path.join(data_dir, file))
        root = tree.getroot()

        subject = root.find('info').get('subjectName')
        results_with_systems = add_system_to_result_xml(root)
        results_with_systems_and_comparisons = add_comparison_to_result_xml(results_with_systems)

        my_data = et.tostring(results_with_systems_and_comparisons)
        my_file = open("items2.xml", "w")
        my_file.write(my_data)


print_overall = True
print_subjects = True
print_stimuli = True


# noinspection PyUnusedLocal
def ab_analysis():
    data_dir = './ab_example_results/'
    file_list = glob.glob(data_dir + '**/*.xml', recursive=True)

    all_results = {}

    # Collect results into a dictionary and filter out bad trials
    for file in file_list:
        tree = et.parse(file)
        root = tree.getroot()

        subject = root.find('info').get('subjectName')
        results_with_systems = add_system_to_result_xml(root)

        validity_check_results = check_trial_validity(results_with_systems)
        # print_validity_check_results(subject, validity_check_results)

        # this is just to check that I post-screened as expected
        validity_check_results = check_trial_validity(validity_check_results['filteredResultsXml'])
        assert (len(validity_check_results['badDurationTrialsList']) == 0)
        assert (len(validity_check_results['skippedSystemTrialsList']) == 0)

        all_results[subject] = validity_check_results['filteredResultsXml']

    # Get overall win rate of each system vs each other system
    for sys in system_names:
        sys_overall_win_rate = get_system_overall_winrate(all_results, sys)

        print(sys + ' win rate: ' + str(round(sys_overall_win_rate['winrate'], 2)) +
              ' (' + str(sys_overall_win_rate['wins']) + ' out of ' + str(sys_overall_win_rate['comps']) + ')')

        sys_win_rate_vs_each_system = get_overall_winrate_vs_each_system(all_results, sys)
        print_winrate_vs_each_system(sys_win_rate_vs_each_system, sys)
        print('\n')

    # Do the same as above, except broken out by subject
    if print_subjects:
        for subject in all_results:
            print('Results for Subject ' + subject)
            for sys in system_names:
                sys_overall_win_rate = get_system_winrate(all_results[subject], sys)

                print(sys + ' win rate: ' + str(round(sys_overall_win_rate['winrate'], 2)) +
                      ' (' + str(sys_overall_win_rate['wins']) + ' out of ' + str(sys_overall_win_rate['comps']) + ')')

                sys_win_rate_vs_each_system = get_subject_winrate_vs_each_system(all_results[subject], sys)
                print_winrate_vs_each_system(sys_win_rate_vs_each_system, sys)
                print('\n')

    if print_stimuli:
        stimuli_winrates = get_all_stimuli_winrates(all_results)

        for stimuli in stimuli_winrates:
            print("Win rates for " + stimuli + ':')
            overall_wins = 0
            overall_comps = 0
            for sys in system_names:
                print_winrate_vs_each_system(stimuli_winrates[stimuli][sys], sys)
                print('\n')

if __name__ == '__main__':
    add_info_to_xml()
    ab_analysis()
