#include "nwblocations.h"

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

// Here are some example locations that were used during development and testing.
//<nwb_dataset_name>/processing/ecephys/LFP/lfp/data</nwb_dataset_name>
//<nwb_sampling_name>/processing/ecephys/LFP/lfp/starting_time</nwb_sampling_name>
//<nwb_voltage_electrodes>/processing/ecephys/LFP/lfp/electrodes</nwb_voltage_electrodes>
//<nwb_voltage_electrodes_shanks>general/extracellular_ephys/electrodes/shank_electrode_number</nwb_voltage_electrodes_shanks>
//<nwb_spike_times_values>units/spike_times</nwb_spike_times_values>
//<nwb_spike_times_indices>units/spike_times_index</nwb_spike_times_indices>
//<nwb_spike_times_shanks>units/electrode_group</nwb_spike_times_shanks>
//<nwb_event_times>/stimulus/presentation/PulseStim_0V_10001ms_LD0/timestamps</nwb_event_times>



NWBLocations::NWBLocations(std::string hsFileName)
{
    strLoc =  getLocationName(hsFileName, "_loc.xml");
}


QString NWBLocations::getLocationName(std::string& s, const std::string& newExt) {

   std::string new_filename = s;
   std::string::size_type i = s.rfind('.', s.length());

   if (i != std::string::npos)
      new_filename.replace(i, s.length()-i, newExt);
   else
      new_filename.append(newExt);

   QString QNewName = QString::fromUtf8(new_filename.c_str());
   return QNewName;
}

std::string NWBLocations::getVoltageDataSetName()
{
    QString strDSN = "/processing/ecephys/LFP/lfp/data";

    NeuroscopeXmlReader reader = NeuroscopeXmlReader();
    if(reader.parseFile(strLoc,NeuroscopeXmlReader::PARAMETER)){
        strDSN = reader.getNWBDataSetName();
    }

    std::string utf8_text = strDSN.toUtf8().constData();
    return utf8_text;
}

std::string NWBLocations::getSamplingName()
{
    QString strSN = "/processing/ecephys/LFP/lfp/starting_time";

    NeuroscopeXmlReader reader = NeuroscopeXmlReader();
    if(reader.parseFile(strLoc,NeuroscopeXmlReader::PARAMETER)){
        strSN = reader.getNWBSamplingName();
    }

    std::string utf8_text = strSN.toUtf8().constData();
    return utf8_text;
}

std::string NWBLocations::getGenericText(std::string strLabel, std::string strDefault)
{
    NeuroscopeXmlReader reader = NeuroscopeXmlReader();
    if(reader.parseFile(strLoc,NeuroscopeXmlReader::PARAMETER)){
        QString qsLabel = QString::fromUtf8(strLabel.c_str());
        QString qsDefault = QString::fromUtf8(strDefault.c_str());

        QString DSN = reader.getGenericText(qsLabel, qsDefault);

        std::string utf8_text = DSN.toUtf8().constData();
        return utf8_text;
    }
    return "";
}

QList<std::string> NWBLocations::getListGenericTexts(std::string strLabel, std::string strDefault)
{
    NeuroscopeXmlReader reader = NeuroscopeXmlReader();
    if(reader.parseFile(strLoc,NeuroscopeXmlReader::PARAMETER)){
        QString qsLabel = QString::fromUtf8(strLabel.c_str());
        QString qsDefault = QString::fromUtf8(strDefault.c_str());

        QList<QString> DSNs = reader.getListGenericTexts(qsLabel, qsDefault);

        QList<std::string> lstDSNs = QList<std::string>();
        for(QString ss : DSNs)
        {
            std::string utf8_text = ss.toUtf8().constData();
            lstDSNs.append(utf8_text);
        }
        return lstDSNs;
    }
    return QList<std::string>();
}

