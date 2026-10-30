# Copyright(C) 2026 Netflix, Inc.
"""XML contract and discovery. Standard library only; no plotting or dataframes."""
from pathlib import Path
import math
import xml.etree.ElementTree as ET

SUPPORTED_TEST_TYPES = {
    'MUSHRA', 'MUSHRA_Strict', 'MUSHRA_Demo', 'BS-1116',
    'AB_Forced_Choice', 'AudioVideo_AB_Forced_Choice',
}


def get_codec_name(file_name, trial_name):
    """Support underscore/dot separators and unprefixed stimulus filenames."""
    if not file_name.lower().endswith('.wav'):
        raise TypeError(f'File = {file_name}: not a wav file')
    stem = file_name[:-4]
    if stem.startswith(trial_name + '_') or stem.startswith(trial_name + '.'):
        codec_name = stem[len(trial_name):].lstrip('_.')
    else:
        codec_name = stem
    if not codec_name:
        raise ValueError('Extracted codec/system name is empty')
    return codec_name


def add_codec_to_result_xml(results, codec_map=None):
    for trial in results.findall('trials/trial'):
        for stimulus in trial.findall('testFile'):
            codec = get_codec_name(stimulus.get('fileName'), trial.get('trialName'))
            stimulus.set('codec', codec_map[codec] if codec_map is not None else codec)
    return results


def get_test_info(root):
    info = root.find('info')
    return {
        'tag': root.tag,
        'test_name': info.get('testName'),
        'start_time': info.get('startTime'),
        'stop_time': info.get('stopTime'),
        'test_status': info.get('testStatus'),
        'stimuli': [trial.get('trialName') for trial in root.findall('trials/trial')],
    }


def read_results(path):
    """Read stored results without accessing media or interpreting surveys."""
    root = ET.parse(path).getroot()
    if root.tag not in SUPPORTED_TEST_TYPES:
        raise ValueError(f'Unsupported test type {root.tag} in {path}')
    trials = root.findall('trials/trial')
    if root.find('info') is None or not trials:
        raise ValueError(f'Missing info or trials in {path}')
    for trial in trials:
        files = trial.findall('testFile')
        if not trial.get('trialName') or len(files) < 2:
            raise ValueError(f'Missing trial name or stimuli in {path}')
        for stimulus in files:
            score = float(stimulus.get('score', 'nan'))
            if not stimulus.get('fileName') or not math.isfinite(score):
                raise ValueError(f'Invalid stimulus or score in {path}')
    return root


def discover_results(directory):
    """Stable file identities; retain historical temporary-result inclusion."""
    directory = Path(directory)
    if not directory.is_dir():
        raise ValueError(f'Input directory does not exist: {directory}')
    files = sorted(directory.glob('*.xml'))
    if any(file.name.startswith('temp') for file in files):
        print('Temp files exist: the analysis results may be incomplete/incorrect')
    return [(file, read_results(file)) for file in files]
