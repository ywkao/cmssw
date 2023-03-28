# PlaygroundEDProducer

### Usage of the package
```
# Need to manually create a directory (or softlink) for output files
$ mkdir ./eos

# Setup environment and compile
$ cmsenv
$ time scram b -j10

# Execution
$ time cmsRun python/ExampleConfig_cfg.py
```

### Features
- Level-0 calibrations (pedestal & CM subtractions) are applied
- Pedestal and CM parameters are evaluated using running statistics
- Parameters are loaded using an independent user-defined C++ class (in csv format)

### Caveats
- The current input is ntuple from test beam data taken in 2022 October.
```
/eos/cms/store/group/dpg_hgcal/tb_hgcal/2022/sps_oct2022/pion_beam_150_320fC/beam_run/run_20221007_191926/beam_run0.root
/eos/cms/store/group/dpg_hgcal/tb_hgcal/2022/sps_oct2022/pedestals/pedestal_320fC/pedestal_run/run_20221008_192720/pedestal_run0.root
```
- The output file from the DQM module contains only one entry because of the line in python/ExampleConfig_cfg.py
```
process.maxEvents.input = 1
```
- RecHit format follows a simpler version -> need to take the standard one in CMSSW.
