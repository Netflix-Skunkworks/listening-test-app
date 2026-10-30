# Copyright(C) 2026 Netflix, Inc.
"""A/B score tables and win-rate calculations, independent of plotting."""
import pandas

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
                if float(f.get('score')) == 1.0:
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
                if float(f.get('score')) == 1.0:
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
                    if float(f.get('score')) == 0.0:
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
        systems = {f.get('system') for trial in stimuli[stimulus] for f in trial.findall('testFile')}
        for system in sorted(systems):
            stimulus_results[system] = get_winrate_vs_each_system(stimuli[stimulus], system)

        all_stimuli_results[stimulus] = stimulus_results

    return all_stimuli_results


