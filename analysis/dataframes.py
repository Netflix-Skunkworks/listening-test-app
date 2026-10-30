# Copyright(C) 2026 Netflix, Inc.
"""Convert result XML into listener score tables; no plotting imports."""
from results_xml import SUPPORTED_TEST_TYPES, add_codec_to_result_xml, discover_results, get_test_info


def create_dataframe(test_results, test_type):
    import pandas
    if test_type not in SUPPORTED_TEST_TYPES:
        raise ValueError(f'Unsupported test type {test_type}')
    scores = {}
    for trial in test_results.findall('trials/trial'):
        row = scores.setdefault(trial.get('trialName'), {})
        files = trial.findall('testFile')
        reference_score = 0.0
        if test_type == 'BS-1116':
            reference = next((s for s in files if 'reference' in s.get('codec').lower()), None)
            if reference is None:
                raise ValueError(f"Missing reference in {trial.get('trialName')}")
            reference_score = float(reference.get('score'))
        for stimulus in files:
            row[stimulus.get('codec')] = float(stimulus.get('score')) - reference_score
    return pandas.DataFrame.from_dict(scores, orient='index')


def parse_xml_results(input_data_dir, codec_map=None):
    listeners, test_info = {}, {}
    for index, (path, root) in enumerate(discover_results(input_data_dir), 1):
        info = get_test_info(root)
        listener = f"Subject {index}: {root.find('info').get('subjectName')}"
        listeners[listener] = create_dataframe(add_codec_to_result_xml(root, codec_map), info['tag'])
        test_info[listener] = info
    return listeners, test_info
