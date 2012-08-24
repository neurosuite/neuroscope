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
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

// include files for KDE
#include "config-neuroscope.h"

// include files for QT
#include <QDir>
#include <QString>
#include <QApplication>

//Application specific include files
#include "neuroscope.h"
#if KDAB_PENDING

static const char *description =
        I18N_NOOP("NeuroScope - Viewer for Local Field Potentials, spikes, events and positional data");
// INSERT A DESCRIPTION FOR YOUR APPLICATION HERE


static KCmdLineOptions options[] =
{
    { "r",0, 0 },
    { "resolution ", I18N_NOOP("Resolution of the acquisition system."), 0 },
    { "c",0, 0 },
    { "nbChannels ", I18N_NOOP("Number of channels."), 0 },
    { "o",0, 0 },
    { "offset ", I18N_NOOP("Initial offset."), 0 },
    { "m",0, 0 },
    { "voltageRange ", I18N_NOOP("Voltage range."), 0 },
    { "a",0, 0 },
    { "amplification ", I18N_NOOP("Amplification."), 0 },
    { "g",0, 0 },
    { "screenGain ", I18N_NOOP("Screen gain."), 0 },
    { "s",0, 0 },
    { "samplingRate ", I18N_NOOP("Sampling rate."), 0 },
    { "t",0, 0 },
    { "timeWindow ", I18N_NOOP("Initial time window (in miliseconds)."), 0 },
    { "+file", I18N_NOOP("Document to open."), 0 },
    { 0, 0, 0 }
    // INSERT YOUR COMMANDLINE OPTIONS HERE
};
#endif
int main(int argc, char *argv[])
{
    QApplication::setOrganizationName("sourceforge");
    QApplication::setOrganizationDomain("sourceforge.net");
    QApplication::setApplicationName("neuroscope");

#if KDAB_PENDING
    KAboutData aboutData( "neuroscope", I18N_NOOP("NeuroScope"),
                          VERSION, description, KAboutData::License_GPL,
                          "(c) 2004-2006, Lynn Hazan", 0, 0, "lynn.hazan.myrealbox.com");
    aboutData.addAuthor("Lynn Hazan",I18N_NOOP("Designer and developer"), "lynn.hazan.myrealbox.com");
    aboutData.addAuthor("Michael Zugaro",I18N_NOOP("Provided technical assistance and co-designed the interface. Tested the application."), "michael.zugaro@college-de-france.fr");
    aboutData.addCredit("CMBN Members",I18N_NOOP("Helped define the set of features and tested the application."),0, "http://osiris.rutgers.edu");
    KCmdLineArgs::init(argc,argv,&aboutData );
    KCmdLineArgs::addCmdLineOptions(options);

    version = VERSION;

    KApplication app;
    // see if we are starting with session management
    if(app.isRestored())
    {
        RESTORE(NeuroscopeApp);
    }
    // no session.. just start up normally
    else
    {
        NeuroscopeApp* neuroscope = new NeuroscopeApp();
        KCmdLineArgs* args = KCmdLineArgs::parsedArgs();
        neuroscope->show();

        //Set the specific features for the future open document.
        neuroscope->setFileProperties(args->getOption("nbChannels"),args->getOption("samplingRate"),args->getOption("resolution"),
                                      args->getOption("offset"),args->getOption("voltageRange"),args->getOption("amplification"),
                                      args->getOption("screenGain"),args->getOption("timeWindow"));

        //If there is an argument provided it is the name of the file.
        if(args->count()){
            QString file = args->arg(0);
            if(file.left(1) != "/"){
                QString url = QString();
                url.setPath((QDir::currentPath()).append("/"));
                url.setFileName(file);
                neuroscope->openDocumentFile(url);
            }
            else  neuroscope->openDocumentFile(file);
        }
        args->clear();
    }
#endif
    //KDAB_TODO
    QApplication app(argc, argv);

    NeuroscopeApp* neuroscope = new NeuroscopeApp();
    neuroscope->show();
    return app.exec();
}  
