# Copyright(C) 2026 Netflix, Inc.
"""Compatibility imports and command-line entry point for A/B analysis."""
from ab_results import *
from ab_statistics import *
from ab_plots import *
from ab_report import ab_analysis

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description='Analyze A/B or audio/video A/B result XML')
    parser.add_argument('-i', '--input-data-dir', required=True)
    ab_analysis(parser.parse_args().input_data_dir)
