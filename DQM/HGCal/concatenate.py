#!/usr/bin/env python2

kappa = 0.000

fp = open("meta_conditions/output_DQMEDAnalyzer_calibration_parameters_pedestalData.txt", "r")
fc = open("meta_conditions/output_DQMEDAnalyzer_calibration_parameters_beamData_ped_subtracted.txt", "r")

channels = []
d_parameters = {"ped":{}, "cm-slope":{}, "cm-offset":{}}
for line in fp.readlines():
    if "Channel" in line: continue
    ch = int(line.strip().split()[0])
    d_parameters["ped"][ch] = float(line.strip().split()[1])
    channels.append(ch)

for line in fc.readlines():
    if "Channel" in line: continue
    ch = int(line.strip().split()[0])
    d_parameters["cm-slope"][ch]  = float(line.strip().split()[2])
    d_parameters["cm-offset"][ch] = float(line.strip().split()[3])

column = "Channel Pedestal CM_slope CM_offset kappa_BXm1"
with open("meta_conditions/calibration_parameters.txt", 'w') as fout:
    fout.write(column+'\n')
    for ch in channels:
        ped    = d_parameters["ped"][ch]
        slope  = d_parameters["cm-slope"][ch] 
        offset = d_parameters["cm-offset"][ch]
        info = "%d %f %f %f %f" % (ch, ped, slope, offset, kappa)
        fout.write(info+'\n')

column = "#Channel,Pedestal,CM_slope,CM_offset,kappa_BXm1"
with open("meta_conditions/calibration_parameters.csv", 'w') as fout:
    fout.write(column+'\n')
    for ch in channels:
        ped    = d_parameters["ped"][ch]
        slope  = d_parameters["cm-slope"][ch] 
        offset = d_parameters["cm-offset"][ch]
        info = "%d,%f,%f,%f,%f" % (ch, ped, slope, offset, kappa)
        fout.write(info+'\n')
