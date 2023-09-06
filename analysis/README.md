# Basic analysis scripts for listening tests

## Usage

Virtual environment is recommended. The scripts were tested in the following environments:
- Python 3.8.16 (2021 Macbook Pro - M1 Max)
- Python 3.11.4 (2021 Macbook Pro - M1 Max)

```
>> virtualenv -p python3 .venv
>> source .venv/bin/activate
>> pip install -r requirements.txt
>> python analyze_cli.py --help
```

For example:
```
>> python analyze_cli.py -i "./resources/mushra_example_results/" -o "./output/"
```

If you would like to have individual plot for each subject and stimulus:
```
>> python analyze_cli.py -i "./resources/mushra_example_results/" -o "./output/" -psub -psti
```