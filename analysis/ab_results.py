# Copyright(C) 2026 Netflix, Inc.
"""A/B XML normalization and listener screening policies."""
minimum_test_duration = 15

from results_xml import get_codec_name


def add_system_to_result_xml(results):
    for trial in results.findall('trials/trial'):
        for stimulus in trial.findall('testFile'):
            name = get_codec_name(stimulus.get('fileName'), trial.get('trialName'))
            # Retain the older suffix used by some A/B datasets.
            if name.endswith('_24.0'):
                name = name[:-5]
            stimulus.set('system', name)
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
            win = float(first_score) == 1.0
        else:
            win = float(first_score) != 1.0

        trial.set('comparison', '_'.join(map(str, comparison)))  # remove trailing underscore
        trial.set('win', win)

    return results


# crate dataframe for analyzing specific comparisons
# trial -> comparison -> win & total trial count
# For a comparison notated as 'a_b', that is system a vs system b
# 'win' == True referenced system A.  So if 'win' == False, system B won.

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


