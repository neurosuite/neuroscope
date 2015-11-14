/***************************************************************************
                          main.cpp  -  description
                             -------------------
    begin                : Wed Feb 25 19:05:25 EST 2004
    copyright            : (C) 2004 by Lynn Hazan
    email                : lynn.hazan.myrealbox.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "config-neuroscope.h"

// include C++ include file
#include <iostream>

// include files for QT
#include <QDir>
#include <QString>
#include <QApplication>

//Application specific include files
#include "neuroscope.h"
int main(int argc, char *argv[])
{    
    // QApplication::setGraphicsSystem() was removed from Qt5
    #if QT_VERSION < 0x050000 
      QApplication::setGraphicsSystem("raster");
    #endif
    QApplication::setOrganizationName("neurosuite");
    QApplication::setOrganizationDomain("neurosuite.github.io");
    QApplication::setApplicationName("neuroscope");

    QApplication app(argc, argv);
    QString file;
    QStringList args = QApplication::arguments();
    QString channelNb;
    QString SR;
    QString resolution;
    QString offset;
    QString voltageRange;
    QString amplification;
    QString screenGain;
    QString timeWindow;
    bool streamMode = false;
    //TODO Qt5.2 use QCommandLineParser
    for (int i = 1, n = args.size(); i < n; ++i) {
        const QString arg = args.at(i);
        if (arg == "-h" || arg == "--help" || arg == "-help") {
            std::cerr  << "Usage: " << qPrintable(args.at(0)) << " [file]\n"
                       << "\n"
                       << "Optional settings:\n"
                       << "  -r, --resolution        Resolution of the acquisition system.\n"
                       << "  -c, --nbChannels        Number of channels.\n"
                       << "  -o, --offset            Initial offset.\n"
                       << "  -m, --voltageRange      Voltage range.\n"
                       << "  -a, --amplification     Amplification.\n"
                       << "  -g, --screenGain        Screen gain.\n"
                       << "  -s, --samplingRate      Sampling rate.\n"
                       << "  -t, --timeWindow        Initial time window (in miliseconds).\n"
                       << "\n"
                       << "Optional flags:\n"
            #if WITH_CEREBUS
                       << "  -n, --stream            Open network stream instead of file.\n"
            #endif
                       << "  -h, --help              print this help\n"
                       << "  -v, --version           print version info\n";
            return 1;
        } else if (arg == "-v" || arg == "--version"  || arg == "-version") {
            std::cout << "NeuroScope " << NEUROSCOPE_VERSION << std::endl;
            return 0;
        }

        bool handled = true;
         if (i < n - 1) { // Parameter value flags
             if (arg == "-r" || arg == "--resolution" || arg == "-resolution") {
                 resolution = args.at(++i);
             } else if (arg == "-c" || arg == "--nbChannels" || arg == "-nbChannels") {
                 channelNb = args.at(++i);
             } else if (arg == "-o" || arg == "--offset" || arg == "-offset") {
                  offset = args.at(++i);
             } else if (arg == "-m" || arg == "--voltageRange" || arg == "-voltageRange") {
                  voltageRange = args.at(++i);
             } else if (arg == "-a" || arg == "--amplification" || arg == "-amplification") {
                  amplification = args.at(++i);
             } else if (arg == "-g" || arg == "--screenGain" || arg == "-screenGain") {
                  screenGain = args.at(++i);
             } else if (arg == "-s" || arg == "--samplingRate" || arg == "-samplingRate") {
                  SR = args.at(++i);
             } else if (arg == "-t" || arg == "--timeWindow" || arg == "-timeWindow") {
                  timeWindow = args.at(++i);
#ifdef WITH_CEREBUS
             } else if (arg == "-n" || arg == "--stream" || arg == "-stream") {
                 streamMode = true;
#endif
             } else {
                 handled = false;
             }
         } else {
             handled = false;
         }
         // Nothing know. Treat it as path.
         if (!handled)
             file = args.at(i);
    }

    if (file.startsWith(QLatin1String("-")) ) {
        std::cerr << "The flag '" << file.toStdString() << "' is unknown or missing a parameter." << std::endl;
        return 1;
    }

    NeuroscopeApp* neuroscope = new NeuroscopeApp();
    neuroscope->setFileProperties(channelNb,SR,resolution,
                                  offset,voltageRange,amplification,
                                  screenGain,timeWindow);

    neuroscope->show();
#ifdef WITH_CEREBUS
    if (streamMode) {
        if(!file.isEmpty()) {
            int group = file.toInt();
            if(group > 0 && group < 6) {
    		    neuroscope->openNetworkStream(static_cast<CerebusTracesProvider::SamplingGroup>(group));
            } else {
                std::cerr << "Sampling group must be between 1 (500 samp/sec) and 5 (30k samp/sec)." << std::endl;
            }
        } else {
            std::cerr << "Network stream mode expects a sampling group as file argument." << std::endl;
        }
    } else {
#endif
        if (!file.isEmpty()) {
            QFileInfo fInfo(file);
            if(fInfo.isRelative()) {
                const QString url = QDir::currentPath().append("/") + file;
                neuroscope->openDocumentFile(url);
            } else {
                neuroscope->openDocumentFile(file);
            }
        }
#ifdef WITH_CEREBUS
    }
#endif

    const int ret = app.exec();
    delete neuroscope;
    return ret;
}
