"""
A Command Line Interface (CLI) for analyzing results from a listening test
"""
import json
import logging
import os

import click

from utility import parse_xml_results
from mushra import plot_mushra_results
from bs1116 import plot_bs1116_results


@click.command()
@click.version_option(version="1.1.0", prog_name="analyze_cli")
@click.option("-i", "--input_data_dir", help="directory of the xml files from a listening test", required=True)
@click.option("-o", "--output_plots_dir", help="directory for saving all the plots", required=False)
@click.option("-psub", "--plot_subjects", help="generate separate plots for all subjects", required=False, is_flag=True)
@click.option("-psti", "--plot_stimuli", help="generate separate plots for all stimuli", required=False,  is_flag=True)
@click.option("-codecs", "--codec_json_file", help="json file relating filenames to codecs for plotting", required=False, is_flag=False)
@click.option("--debug", is_flag=True, help="Show debugging information.")
@click.option("--log", "log_file", help="Optional path to a log file.", required=False)
def main(input_data_dir, output_plots_dir, plot_subjects, plot_stimuli, codec_json_file, debug, log_file):
    """
    Description of command (shows up in --help)
    """
    handlers = [logging.StreamHandler()] + ([logging.FileHandler(log_file)] if log_file else [])
    logging.basicConfig(level=logging.DEBUG if debug else logging.INFO,
                        format="%(asctime)s %(levelname)s %(message)s", handlers=handlers)

    # ==== check input and output dir
    if not os.path.isdir(input_data_dir):
        raise ValueError("input directory is invalid")
    if not output_plots_dir:  # not provided -> using default value
        print("output directory not provided, using default value './output' instead")
        output_plots_dir = os.path.join(".", "output")
    if not os.path.isdir(output_plots_dir):
        print("%s does not exist, os.makedirs() will be called" % output_plots_dir)
        os.makedirs(output_plots_dir)

    # ==== get codec map if specified
    codec_map = None
    if codec_json_file is not None and os.path.isfile(codec_json_file):
        with open(codec_json_file) as f:
            codec_map = json.load(f)

    # ==== parse xml files
    listeners, test_info = parse_xml_results(input_data_dir, codec_map)
    if len(listeners) == 0:
        raise ValueError("No listening test results were found in %s!!" % input_data_dir)

    # ==== plot results depending on the test type
    # supported test types: ["MUSHRA", "BS-1116", "AudioVideo_AB_Forced_Choice"]
    test_type = list(test_info.values())[-1]["tag"]
    print("test type = %s" % test_type)
    if "MUSHRA" in test_type:
        plot_mushra_results(listeners,
                            test_info,
                            output_plots_dir,
                            plot_subjects=plot_subjects,
                            plot_stimuli=plot_stimuli)
    elif "BS-1116" in test_type:
        plot_bs1116_results(listeners,
                            test_info,
                            output_plots_dir,
                            plot_subjects=plot_subjects,
                            plot_stimuli=plot_stimuli)
    elif "AB_Forced_Choice" in test_type:
        print("AB forced choice to be supported soon")


if __name__ == "__main__":
    main()
