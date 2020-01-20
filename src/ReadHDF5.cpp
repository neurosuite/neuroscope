


#ifdef OLD_HEADER_FILENAME
#include <iostream.h>
#else
#include <iostream>
#endif
using std::cout;
using std::endl;
#include <string>
#include "H5Cpp.h"
#include "hdf5.h"
#include "H5File.h"
using namespace H5;
#include "neuroscopexmlreader.h"
#include <QString>

#include <array.h>





#ifdef TRASH
def get3RawSpikeTimes(nwb_spike_times, nwb_spike_times_index, nwb_units_electrode_group, myfile):
    spikeTimes     = myfile[nwb_spike_times]
    spikeTimeIndex = myfile[nwb_spike_times_index]
    ElectrodeGroup = myfile[nwb_units_electrode_group]
    return(spikeTimes, spikeTimeIndex, ElectrodeGroup)

def mapSpikeUnits(spikeTimes, spikeTimeIndex, ElectrodeGroup, myfile):
    units = []
    for idx, ndxUpper in enumerate(spikeTimeIndex):
        ndxLower = 0 if idx == 0 else spikeTimeIndex[idx-1]
        units.append((myfile[ElectrodeGroup[idx]].name, spikeTimes[ndxLower:ndxUpper]))
    return units

def getSpikeUnits(nwb_spike_times, nwb_spike_times_index, nwb_units_electrode_group, myfile):
    (spikeTimes, spikeTimeIndex, ElectrodeGroup) = get3RawSpikeTimes(nwb_spike_times, nwb_spike_times_index, nwb_units_electrode_group, myfile)
    units = mapSpikeUnits(spikeTimes, spikeTimeIndex, ElectrodeGroup, myfile)
    return units

def stripShankName(strIn):
    strSplit = strIn.split('/')
    return (strSplit[-1] if len(strSplit)>0 else strIn)


nwb_spike_times = 'units/spike_times'
nwb_spike_times_index = 'units/spike_times_index'
nwb_units_electrode_group = 'units/electrode_group'

units = getSpikeUnits(nwb_spike_times, nwb_spike_times_index, nwb_units_electrode_group, myfile)
for oneUnit in units:
    print(stripShankName(oneUnit[0]), oneUnit[1][:8])
#endif


