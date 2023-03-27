# PlaygroundDQMEDAnalyzer

```
# Need to manually create a directory (or softlink) for output files
$ mkdir ./eos

# Setup environment and compile
$ cmsenv
$ time scram b -j10

# Execution
$ time cmsRun python/ExampleConfig_cfg.py

# Make plots from DQM_V0001_*.root
$ ./getPlots.py
```
