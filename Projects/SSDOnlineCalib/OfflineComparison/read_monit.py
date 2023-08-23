import uproot

#monitoring = uproot.concatenate("/cr/tempdata01/filip/SSDCalib/Monit/*.root")
monitoring = uproot.open("/cr/tempdata01/filip/SSDCalib/Monit/SD_2022_11_16_adst.root")

print(monitoring.keys())