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

// include files for QT
#include <QDir>
#include <QString>
#include <QApplication>
#include <QDebug>
//Application specific include files
#include "neuroscope.h"
int main(int argc, char *argv[])
{
    QApplication::setGraphicsSystem("raster");
    QApplication::setOrganizationName("sourceforge");
    QApplication::setOrganizationDomain("sourceforge.net");
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
    //TODO Qt5.2 use QCommandLineParser
    for (int i = 1, n = args.size(); i < n; ++i) {
        const QString arg = args.at(i);
        if (arg == "-h" || arg == "--help" || arg == "-help") {
            qWarning() << "Usage: " << qPrintable(args.at(0))
                       << " [file]"
                       << "\n\n"
                       << "Arguments:\n"
                       << "  -r, --resolution        Resolution of the acquisition system.\n"
                       << "  -c, --nbChannels        Number of channels.\n"
                       << "  -o, --offset            Initial offset.\n"
                       << "  -m, --voltageRange      Voltage range.\n"
                       << "  -a, --amplification     Amplification.\n"
                       << "  -g, --screenGain        Screen gain.\n"
                       << "  -s, --samplingRate      Sampling rate.\n"
                       << "  -t, --timeWindow        Initial time window (in miliseconds).\n"
                       << "  -h, --help              print this help\n";
            return 1;
        }

        bool handled = true;
         if (i < n - 1) {
             if (arg == "-r" || arg == "--resolution" || arg == "-resolution")
                 resolution = args.at(++i);
             else if (arg == "-c" || arg == "--nbChannels" || arg == "-nbChannels") {
                 channelNb = args.at(++i);
             }
             else if (arg == "o-" || arg == "--offset" || arg == "-offset")
                  offset = args.at(++i);
             else if (arg == "-m" || arg == "--voltageRange" || arg == "-voltageRange")
                  voltageRange = args.at(++i);
             else if (arg == "-a" || arg == "--amplification" || arg == "-amplification")
                  amplification = args.at(++i);
             else if (arg == "-g" || arg == "--screenGain" || arg == "-screenGain")
                  screenGain = args.at(++i);
             else if (arg == "-s" || arg == "--samplingRate" || arg == "-samplingRate")
                  SR = args.at(++i);
             else if (arg == "-t" || arg == "--timeWindow" || arg == "-timeWindow")
                  timeWindow = args.at(++i);
             else
                 handled = false;

         } else {
                 handled = false;
         }
         // Nothing know. Treat it as path.
         if (!handled || (n == 2) )
             file = args.at(i);
    }
    NeuroscopeApp* neuroscope = new NeuroscopeApp();
    neuroscope->setFileProperties(channelNb,SR,resolution,
                                  offset,voltageRange,amplification,
                                  screenGain,timeWindow);

    neuroscope->show();
    if (!file.isEmpty()) {
        QFileInfo fInfo(file);
        if (file.startsWith(QLatin1String("-")) ) {
            qWarning() << "it's not a filename :"<<file;
        } else if(fInfo.isRelative()) {
            const QString url = QDir::currentPath().append("/") + file;
            neuroscope->openDocumentFile(url);
        } else {
            neuroscope->openDocumentFile(file);
        } 
    }


    const int ret = app.exec();
    delete neuroscope;
    return ret;
}  

