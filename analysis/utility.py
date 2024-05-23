import glob
import xml.etree.ElementTree as elementTree
import pandas


def longest_common_substring(s1, s2):
    """Finds the longest common substring between s1 and s2."""
    s1 = s1.replace('-', '_')
    s2 = s2.replace('-', '_')
    m = len(s1)
    n = len(s2)
    result = ""
    length = 0
    dp = [[0] * (n + 1) for _ in range(m + 1)]

    for i in range(m + 1):
        for j in range(n + 1):
            if i == 0 or j == 0:
                dp[i][j] = 0
            elif s1[i - 1] == s2[j - 1]:
                dp[i][j] = dp[i - 1][j - 1] + 1
                if dp[i][j] > length:
                    length = dp[i][j]
                    result = s1[i - length:i]
            else:
                dp[i][j] = 0
    return result


def get_codec_name(file_name, trial_name):
    # check if the file name is valid
    if not file_name.endswith('.wav'):
        raise TypeError("File = %s: not a wav file" % file_name)
    if not file_name.startswith(trial_name):
        print("--> File = %s: file name prefix is inconsistent with trial name %s" % (file_name, trial_name))
        # also support: <codec_or_system_name>.wav, but notify user of the inconsistency

        # remove the potential artifact prefix, e.g., LP_file_name
        artifact_prefix = trial_name.split("_")[0] + "_"
        new_trial_name = trial_name.replace(artifact_prefix, "")
        # normalize strings from "-" to "_"
        normalized_file_name = file_name.replace("-", "_")
        normalized_trial_name = new_trial_name.replace("-", "_")

        if normalized_file_name.startswith(normalized_trial_name):
            codec_name = file_name.split(".wav")[0][len(normalized_trial_name)+1:]
        else:
            codec_name = file_name.split(".wav")[0]  # if trial and filename have nothing in common, take the filename
    else:
        # naming convention: <trial_name>_<codec_or_system_name>.wav
        codec_name = file_name.split(".wav")[0][len(trial_name)+1:]
    if len(codec_name) == 0:
        raise ValueError("Extracted codec/system name is empty")
    return codec_name


def add_codec_to_result_xml(results, codec_map=None):
    # clean up data: get codec name from filenames
    trials = results.find('trials')
    for trial in trials.findall('trial'):
        trial_name = trial.get('trialName')
        for f in trial.findall('testFile'):
            file_name = f.get('fileName')
            codec_name = get_codec_name(file_name, trial_name)
            if codec_map is not None:
                codec_name = codec_map[codec_name]
            f.set('codec', codec_name)
    return results


def get_test_info(root):
    """
    :param root: tree structured data parsed from one xml file
    :return:
        test_info_dict: key = {tag, test_name, test_status, start_time, stop_time, stimuli}
                        val = the values specified in the header of the xml files
        * note that stimuli is a list of "trial name"
    """
    test_info = dict()
    test_info["tag"] = root.tag
    test_info["test_name"] = root.find('info').get('testName')
    test_info["start_time"] = root.find('info').get('startTime')
    test_info["stop_time"] = root.find('info').get('stopTime')
    test_info["test_status"] = root.find('info').get('testStatus')

    stimuli = []
    trials = root.find('trials')
    for trial in trials.findall('trial'):
        stimuli.append(trial.get('trialName'))
    test_info["stimuli"] = stimuli
    return test_info


def create_dataframe(test_results, test_type):
    # Create dataframes of listeners -> trials -> codecs: scores
    # Create one dataframe per listener.
    codecs = dict()
    trials = test_results.find('trials')

    if not test_type in ["MUSHRA",
                         "MUSHRA_Strict",
                         "MUSHRA_Demo",
                         "BS-1116",
                         "AB_Forced_Choice",
                         "AudioVideo_AB_Forced_Choice"]:
        raise ValueError("Unsupported test type %s" % test_type)

    for trial in trials.findall('trial'):
        codecs[trial.get('trialName')] = dict()

    for trial in trials.findall('trial'):
        for i, f in enumerate(trial.findall('testFile')):
            if 'reference' in f.get('codec'):  # 1116 and MUSHRA treat reference differently
                ref_score = float(f.get('score'))  # within the trial, hold the reference score for 1116 calculation
                if "BS-1116" in test_type:
                    codecs[trial.get('trialName')][f.get('codec')] = float(f.get('score')) - ref_score
                else:
                    codecs[trial.get('trialName')][f.get('codec')] = float(f.get('score'))

        for i, f in enumerate(trial.findall('testFile')):
            if 'reference' not in f.get('codec'):
                if "BS-1116" in test_type:
                    codecs[trial.get('trialName')][f.get('codec')] = float(f.get('score')) - ref_score
                else:
                    codecs[trial.get('trialName')][f.get('codec')] = float(f.get('score'))
    return pandas.DataFrame.from_dict(codecs, orient='index')


def parse_xml_results(input_data_dir, codec_map=None):
    """
    given a directory of xml files, parse the listening test results
    :param input_data_dir: str, a directory of xml files
    :return:
        listeners_dict: dictionary, key = "Subject %d: %s" % (index, subjectName)
                                    val = dataframe of the test results
        test_info_dict: dictionary, key = {tag, test_name, test_status, start_time, stop_time}
                                    val = the values specified in the header of the xml files
    """
    print("searching for results in " + input_data_dir)
    if not input_data_dir.endswith("/"):
        input_data_dir += "/"
    temp_file_list = glob.glob(input_data_dir + 'temp*.xml')
    if len(temp_file_list) > 0:
        print("Temp files exist: the analysis results may be incomplete/incorrect")
    file_list = glob.glob(input_data_dir + '*.xml')
    listeners_dict = dict()
    test_info_dict = dict()

    for i in range(0, len(file_list)):
        file = file_list[i]
        print("handling " + str(file))
        tree = elementTree.parse(file)
        root = tree.getroot()
        test_info = get_test_info(root)
        listener = "Subject " + str(i+1) + ": " + root.find('info').get('subjectName')
        results_with_scores = add_codec_to_result_xml(root, codec_map)
        results_df = create_dataframe(results_with_scores, test_info["tag"])
        listeners_dict[listener] = results_df
        test_info_dict[listener] = test_info
    return listeners_dict, test_info_dict
