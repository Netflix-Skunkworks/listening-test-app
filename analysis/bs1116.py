import matplotlib.pylab
import matplotlib.pyplot


from plot import plot_one_subject, plot_one_stimulus, plot_all_listeners, plot_all
from utility import parse_xml_results


def plot_bs1116_results(listeners_dict,
                        test_info_dict,
                        output_dir,
                        plot_subjects=True,
                        plot_stimuli=True,
                        show_plots=False):
    """
    :param listeners_dict: dictionary, key = "Subject %d: %s" % (index, subjectName)
                                       val = dataframe of the test results
    :param test_info_dict: key = {tag, test_name, test_status, start_time, stop_time, stimuli}
                           val = the values specified in the header of the xml files
    :param output_dir: str, path to a directory for storing the plots; default = "./output/"
    :param plot_subjects: bool, generate individual plot for each subject
    :param plot_stimuli: bool, generate individual plot for each stimulus
    :param show_plots: bool, show plots immediately
    """
    matplotlib.pylab.rcParams['figure.figsize'] = 20, 12
    stimuli = []
    test_name = ""
    for key in test_info_dict:
        test_info = test_info_dict[key]
        test_name = test_info["test_name"]
        stimuli = test_info_dict[key]["stimuli"]

    if plot_subjects:
        for listener in listeners_dict:
            results_df = listeners_dict[listener]
            listener_filename = output_dir + "%s_%s.png" % (test_name, listener)
            plot_one_subject(results_df, listener_filename, listener)

    if plot_stimuli:
        for stim in stimuli:
            stimulus_filename = output_dir + "%s_%s.png" % (test_name, stim)
            plot_one_stimulus(listeners_dict, stimulus_filename, stim)

    all_listeners_filename = output_dir + "%s_all_listeners.png" % test_name
    plot_all_listeners(listeners_dict, all_listeners_filename, title=test_name)

    if len(listeners_dict) > 1:
        all_items_filename = output_dir + "%s_all_items.png" % test_name
        plot_all(listeners_dict, all_items_filename, title=test_name)

    if show_plots:
        matplotlib.pyplot.show()


if __name__ == '__main__':
    data_dir = "./resources/bs1116_example_results/"
    output_dir = "./output/"
    listeners, test_info = parse_xml_results(data_dir)
    plot_bs1116_results(listeners, test_info, output_dir, plot_subjects=True, plot_stimuli=True)
