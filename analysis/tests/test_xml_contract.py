# Copyright(C) 2026 Netflix, Inc.
"""Dependency-free checks of the published results contract, used in CI."""
import sys
import os
import tempfile
import unittest
from pathlib import Path
import xml.etree.ElementTree as ET

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from results_xml import read_results, discover_results, get_test_info, get_codec_name
from ab_results import add_system_to_result_xml, add_comparison_to_result_xml

RESOURCES = Path(__file__).resolve().parents[1] / 'resources'


class XmlContractTests(unittest.TestCase):
    @unittest.skipUnless(os.environ.get('LISTENING_TEST_XML_CONTRACT_DIR'), 'Run through CTest to consume C++ results')
    def test_current_cpp_serializer(self):
        directory = Path(os.environ['LISTENING_TEST_XML_CONTRACT_DIR'])
        files = list(directory.glob('*.xml'))
        self.assertEqual(len(files), 3)
        for file in files:
            original_path = next(RESOURCES.glob('*/' + file.name))
            original, saved = read_results(original_path), read_results(file)
            self.assertEqual(saved.tag, original.tag)
            self.assertEqual(saved.find('info').get('subjectName'), original.find('info').get('subjectName'))
            before, after = original.findall('trials/trial'), saved.findall('trials/trial')
            self.assertEqual(len(before), len(after))
            for old_trial, new_trial in zip(before, after):
                self.assertEqual(old_trial.get('trialName'), new_trial.get('trialName'))
                old_files, new_files = old_trial.findall('testFile'), new_trial.findall('testFile')
                self.assertEqual(len(old_files), len(new_files))
                for old, new in zip(old_files, new_files):
                    self.assertEqual(new.get('fileName'), old.get('fileName'))
                    self.assertEqual(new.get('plays'), old.get('plays'))
                    self.assertEqual(new.get('comment', ''), old.get('comment', ''))
                    self.assertAlmostEqual(float(new.get('score')), float(old.get('score')), delta=0.00051)

    def test_published_examples(self):
        tags = {'mushra_example_results': 'MUSHRA', 'bs1116_example_results': 'BS-1116',
                'ab_example_results': 'AudioVideo_AB_Forced_Choice'}
        for directory, tag in tags.items():
            records = discover_results(RESOURCES / directory)
            self.assertEqual(len(records), 3)
            for path, root in records:
                with self.subTest(file=path):
                    self.assertEqual(root.tag, tag)
                    info = get_test_info(root)
                    self.assertEqual(info['test_status'], 'complete')
                    self.assertTrue(info['test_name'])
                    trials = root.findall('trials/trial')
                    self.assertTrue(trials)
                    for trial in trials:
                        stimuli = trial.findall('testFile')
                        self.assertGreaterEqual(len(stimuli), 2)
                        if tag == 'BS-1116':
                            reference = [s for s in stimuli if 'reference' in s.get('fileName').lower()]
                            self.assertEqual(len(reference), 1)
                            self.assertGreaterEqual(float(reference[0].get('score')), 1.0)
                        elif tag == 'AudioVideo_AB_Forced_Choice':
                            self.assertEqual(len(stimuli), 2)
                            self.assertEqual(sum(float(s.get('score')) for s in stimuli), 1.0)
                            self.assertIsNotNone(trial.find('videoFile'))

    def test_ab_filenames_and_current_score_precision(self):
        for score in ('1', '1.0', '1.000'):
            root = ET.fromstring(f'''<AB_Forced_Choice><trials><trial trialName="clip">
                <testFile fileName="clip.system2.wav" score="0"/>
                <testFile fileName="clip_system1.wav" score="{score}"/>
                </trial></trials></AB_Forced_Choice>''')
            add_comparison_to_result_xml(add_system_to_result_xml(root))
            trial = root.find('trials/trial')
            self.assertEqual(trial.get('comparison'), 'system1_system2')
            self.assertTrue(trial.get('win'))

    def test_schema_failures_and_extensions(self):
        with tempfile.TemporaryDirectory() as temporary:
            file = Path(temporary) / 'result.xml'
            for content in ('<unknown/>', '<MUSHRA/>',
                            '<MUSHRA><info/><trials><trial trialName="x"/></trials></MUSHRA>'):
                file.write_text(content)
                with self.assertRaises(ValueError):
                    read_results(file)
            original = ET.parse(next((RESOURCES / 'mushra_example_results').glob('*.xml'))).getroot()
            original.append(ET.fromstring('<surveyResults surveyCompleted="yes"><custom value="kept"/></surveyResults>'))
            ET.ElementTree(original).write(file)
            self.assertEqual(read_results(file).find('surveyResults/custom').get('value'), 'kept')


if __name__ == '__main__':
    unittest.main()
