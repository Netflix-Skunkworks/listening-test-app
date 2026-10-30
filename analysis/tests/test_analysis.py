# Copyright(C) 2026 Netflix, Inc.
"""Score transformation and statistical checks using the existing analysis dependencies."""
import sys
import unittest
from pathlib import Path
import xml.etree.ElementTree as ET
import pandas

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from dataframes import parse_xml_results
from summary_statistics import confidence_offsets
from ab_results import add_system_to_result_xml
from ab_statistics import get_system_winrate

RESOURCES = Path(__file__).resolve().parents[1] / 'resources'


class AnalysisTests(unittest.TestCase):
    def test_mushra_scores_and_bs1116_differences(self):
        listeners, info = parse_xml_results(str(RESOURCES / 'mushra_example_results'))
        self.assertEqual(len(listeners), 3)
        for scores in listeners.values():
            self.assertTrue(((scores.stack() >= 0) & (scores.stack() <= 100)).all())
        listeners, info = parse_xml_results(str(RESOURCES / 'bs1116_example_results'))
        self.assertEqual(len(listeners), 3)
        for path in sorted((RESOURCES / 'bs1116_example_results').glob('*.xml')):
            root = ET.parse(path).getroot()
            key = next(k for k in listeners if k.endswith(': ' + root.find('info').get('subjectName')))
            for trial in root.findall('trials/trial'):
                reference = next(s for s in trial.findall('testFile') if 'reference' in s.get('fileName'))
                for stimulus in trial.findall('testFile'):
                    codec = stimulus.get('fileName')[len(trial.get('trialName')) + 1:-4]
                    # The published dataset repeats each folder once per coded source.
                    if codec != 'reference':
                        expected = float(stimulus.get('score')) - float(reference.get('score'))
                        self.assertAlmostEqual(listeners[key].loc[trial.get('trialName'), codec], expected)

    def test_ab_win_rates_accept_integer_and_decimal_scores(self):
        for path in (RESOURCES / 'ab_example_results').glob('*.xml'):
            root = add_system_to_result_xml(ET.parse(path).getroot())
            for system in ('system1', 'system2'):
                expected = sum(float(s.get('score')) for s in root.findall('trials/trial/testFile') if s.get('system') == system)
                self.assertEqual(get_system_winrate(root, system)['wins'], expected)
            for stimulus in root.findall('trials/trial/testFile'):
                stimulus.set('score', str(int(float(stimulus.get('score')))))
            self.assertEqual(sum(get_system_winrate(root, s)['wins'] for s in ('system1', 'system2')),
                             len(root.findall('trials/trial')))

    def test_confidence_intervals(self):
        offsets = confidence_offsets(pandas.Series([1.0, 2.0, 3.0]))
        self.assertAlmostEqual(offsets[0], -2.4841377117, places=8)
        self.assertAlmostEqual(offsets[1], 2.4841377117, places=8)


if __name__ == '__main__':
    unittest.main()
