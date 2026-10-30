# Copyright(C) 2026 Netflix, Inc.
"""A/B report orchestration. XML normalization and win calculations stay separate."""
from results_xml import discover_results
from ab_results import add_system_to_result_xml, check_trial_validity
from ab_statistics import get_system_overall_winrate, get_subject_winrate_vs_each_system


def ab_analysis(data_dir='./resources/ab_example_results/'):
    all_results = {}
    for path, root in discover_results(data_dir):
        if root.tag not in {'AB_Forced_Choice', 'AudioVideo_AB_Forced_Choice'}:
            raise ValueError(f'Expected A/B results in {path}')
        normalized = add_system_to_result_xml(root)
        all_results[path.name] = check_trial_validity(normalized)['filteredResultsXml']
    systems = sorted({stimulus.get('system') for root in all_results.values()
                      for stimulus in root.findall('trials/trial/testFile')})
    if not systems:
        raise ValueError('No valid A/B trials found')
    for system in systems:
        result = get_system_overall_winrate(all_results, system)
        print(f"{system} win rate: {result['winrate']:.2f} ({result['wins']} out of {result['comps']})")
    return all_results
