# Copyright(C) 2026 Netflix, Inc.
"""Choose methodology-specific reports after parsing and score transformation."""


def generate_report(listeners, test_info, output_dir, plot_subjects=False, plot_stimuli=False):
    types = {info['tag'] for info in test_info.values()}
    if len(types) != 1:
        raise ValueError('Analyze one methodology at a time in a separate input directory')
    test_type = types.pop()
    if test_type.startswith('MUSHRA'):
        from mushra import plot_mushra_results
        renderer = plot_mushra_results
    elif test_type == 'BS-1116':
        from bs1116 import plot_bs1116_results
        renderer = plot_bs1116_results
    else:
        print('A/B plotting is available through the separate ab.py consumer')
        return
    renderer(listeners, test_info, output_dir, plot_subjects=plot_subjects, plot_stimuli=plot_stimuli)
