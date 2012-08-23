/***************************************************************************
                          neuroscope.cpp  -  description
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

// include files for QT
#include <QDir>
#include <QPainter>
#include <QCursor>
#include <qinputdialog.h>
//Added by qt3to4:
#include <QPixmap>
#include <Q3ValueList>
#include <QEvent>
#include <QCustomEvent>
#include <Q3PopupMenu>
#include <QInputDialog>
#include <QFileDialog>

// include files for KDE


#include <QStatusBar>


#include <QProcess>

// application specific includes
#include "neuroscope.h"
#include "neuroscopedoc.h"
#include "channelpalette.h"
#include "prefdialog.h"
#include "configuration.h"  // class Configuration
#include "propertiesdialog.h"
#include "traceview.h"
#include "itempalette.h"
#include "eventsprovider.h"

//General C++ include files
#include <iostream>
#include <exception>
using namespace std;


NeuroscopeApp::NeuroscopeApp():KDockMainWindow(0, "NeuroScope"),prefDialog(0L),displayCount(0),mainDock(0),
    displayPanel(0),displayChannelPalette(0),spikeChannelPalette(0),tabsParent(0L),paletteTabsParent(0L),
    palettePanel(0L),isInit(true),groupsModified(false),colorModified(false),eventsModified(false),initialOffsetDefault(0),propertiesDialog(0L),
    select(false),filePath(""),initialTimeWindow(0),eventIndex(0),buttonEventIndex(0),eventLabelToCreate(""),
    eventProvider(""),undoRedoInprocess(false),isPositionFileLoaded(false){

    printer = new QPrinter();

    //Apply the user settings.
    initializePreferences();

    ///////////////////////////////////////////////////////////////////
    // call inits to invoke all other construction parts
    initStatusBar();
    initItemPanel();

    //Create a NeuroscopeDoc which will hold the document manipulated by the application.
    doc = new NeuroscopeDoc(this,*displayChannelPalette,*spikeChannelPalette,channelNbDefault,datSamplingRateDefault,eegSamplingRateDefault,
                            initialOffsetDefault,voltageRangeDefault,amplificationDefault,screenGainDefault,resolutionDefault,
                            eventPosition,clusterPosition,nbSamplesDefault,peakIndexDefault,videoSamplingRateDefault,videoWidthDefault,videoHeightDefault,backgroundImageDefault,traceBackgroundImageDefault,
                            rotationDefault,flipDefault,drawPositionsOnBackgroundDefault);

    //Prepare the actions
    initActions();

    // Apply the saved mainwindow settings, if any, and ask the mainwindow
    // to automatically save settings if changed: window size, toolbar
    // position, icon size, etc.
    setAutoSaveSettings();

    // initialize the recent file list
    ///KDAB_PENDING fileOpenRecent->loadEntries(config);

    //Disable some actions at startup (see the neuroscope.rc file)
    slotStateChanged("initState");
}

NeuroscopeApp::~NeuroscopeApp()
{
    //Clear the memory by deleting all the pointers
    delete doc;
    delete printer;
}

void NeuroscopeApp::initActions()
{
    KStdAction::open(this, SLOT(slotFileOpen()), actionCollection());
    fileOpenRecent = KStdAction::openRecent(this, SLOT(slotFileOpenRecent(const QString&)), actionCollection());
    VARIABLE = MENU->addAction(tr("&Close"), "fileclose");
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotFileClose()));

    KStdAction::save(this, SLOT(saveSession()), actionCollection());
    KStdAction::saveAs(this, SLOT(slotSessionSaveAs()), actionCollection());

    KStdAction::print(this, SLOT(slotFilePrint()), actionCollection());
    KStdAction::quit(this, SLOT(close()), actionCollection());
    viewMainToolBar = KStdAction::showToolbar(this, SLOT(slotViewMainToolBar()), actionCollection());
    viewStatusBar = KStdAction::showStatusbar(this, SLOT(slotViewStatusBar()), actionCollection());
    KStdAction::undo(this, SLOT(slotUndo()), actionCollection());
    KStdAction::redo(this, SLOT(slotRedo()), actionCollection());
    KStdAction::preferences(this,SLOT(executePreferencesDlg()), actionCollection());

    //Custom actions and menus

    //File Menu
    VARIABLE = MENU->addAction(tr("&Properties"));
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotFileProperties()));

    VARIABLE = MENU->addAction(tr("Load Cl&uster File(s)..."));
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotLoadClusterFiles()));

    VARIABLE = MENU->addAction(tr("Load &Event File(s)..."));
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotLoadEventFiles()));

    VARIABLE = MENU->addAction(tr("Load Posi&tion File..."));
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotLoadPositionFile()));

    VARIABLE = MENU->addAction(tr("Create Event &File..."));
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotCreateEventFile()));

    VARIABLE = MENU->addAction(tr("Close C&luster File"));
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotCloseClusterFile()));

    VARIABLE = MENU->addAction(tr("Close E&vent File"));
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotCloseEventFile()));

    VARIABLE = MENU->addAction(tr("Close Position File"));
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotClosePositionFile()));




    //Edit menu
    VARIABLE = MENU->addAction(tr("Select &All"));
    VARIABLE->setShortcuts(Qt::CTRL + Qt::Key_A);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotSelectAll()));

    new QAction(tr("Select All e&xcept 0 and 1"), Qt::CTRL + Qt::SHIFT + Qt::Key_A, this,
                SLOT(slotSelectAllWO01()),actionCollection(),"edit_select_all_except01");
    VARIABLE = MENU->addAction(tr("Deselect All"));
    VARIABLE->setShortcuts(Qt::CTRL + Qt::Key_U);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotDeselectAll()));

    editMode = MENU->addAction(tr("&Edit Mode"),QIcon(":/icons/edit"));
    editMode->setShortcuts(Qt::CTRL + Qt::Key_E);
    editMode->setCheckable(true);
    connect(editMode,SIGNAL(triggered()), this,SLOT(slotEditMode()));

    editMode->setChecked(true);

    //Tools menu
    VARIABLE = MENU->addAction(tr("Zoom"),QIcon(":/icons/zoom_tool"));
    VARIABLE->setShortcuts(Qt::Key_Z);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotZoom()));

    VARIABLE = MENU->addAction(tr("Select Channels"),QIcon(":/icons/select_tool"));
    VARIABLE->setShortcuts(Qt::Key_C);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotSelect()));

    VARIABLE = MENU->addAction(tr("Measure"),QIcon(":/icons/measure_tool"));
    VARIABLE->setShortcuts(Qt::Key_V);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotMeasure()));

    VARIABLE = MENU->addAction(tr("Select Time"),QIcon(":/icons/time_tool"));
    VARIABLE->setShortcuts(Qt::Key_T);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotSelectTime()));

    VARIABLE = MENU->addAction(tr("Select Event"),QIcon(":/icons/event_tool"));
    VARIABLE->setShortcuts(Qt::Key_E);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotSelectEvent()));

    addEventMenu = new KSelectAction(tr("Add Event"),QIcon(":/icons/add_event_tool"),Qt::Key_N,this, SLOT(addEvent()),actionCollection(), "add_event");
    connect(addEventMenu->popupMenu(), SIGNAL(aboutToShow()), this, SLOT(slotAddEventAboutToShow()));
    connect(addEventMenu->popupMenu(), SIGNAL(activated(int)), this, SLOT(slotAddEventActivated(int)));

    addEventToolBarAction = new QAction(tr("Add Event"),QICon(":icons/add_event_tool"));
    connect(addEventToolBarAction,SIGNAL(triggered()), this,SLOT(addEvent()));

    addEventPopup = new QMenu;
    addEventToolBarAction->setMenu(addEventPopup);
    addEventPopup->setCheckable(true);
    connect(addEventPopup, SIGNAL(aboutToShow()), this, SLOT(slotAddEventAboutToShow()));
    connect(addEventPopup, SIGNAL(triggered(QAction *)), this, SLOT(slotAddEventButtonActivated(QAction *)));

    VARIABLE = MENU->addAction(tr("Draw Time Line"),QIcon(":/icons/time_line_tool"));
    VARIABLE->setShortcuts(Qt::Key_L);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotDrawTimeLine()));


    //Traces menu
    displayMode = MENU->addAction(tr("&Multiple Columns"));
    displayMode->setCheckable(true);
    connect(displayMode,SIGNAL(triggered()), this,SLOT(slotDisplayMode()));

    displayMode->setChecked(false);
    greyScale = MENU->addAction(tr("&Grey-Scale"));
    greyScale->setCheckable(true);
    connect(greyScale,SIGNAL(triggered()), this,SLOT(slotSetGreyScale()));

    greyScale->setChecked(false);
    VARIABLE = MENU->addAction(tr("&Increase All Channel Amplitudes"));
    VARIABLE->setShortcuts(Qt::CTRL + Qt::Key_I);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotIncreaseAllChannelsAmplitude()));

    VARIABLE = MENU->addAction(tr("&Decrease All Channel Amplitudes"));
    VARIABLE->setShortcuts(Qt::CTRL + Qt::Key_D);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotDecreaseAllChannelsAmplitude()));

    VARIABLE = MENU->addAction(tr("I&ncrease Selected Channel Amplitudes"));
    VARIABLE->setShortcuts(Qt::CTRL + Qt::SHIFT + Qt::Key_I);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotIncreaseSelectedChannelsAmplitude()));

    VARIABLE = MENU->addAction(tr("D&ecrease Selected Channel Amplitudes"));
    VARIABLE->setShortcuts(Qt::CTRL + Qt::SHIFT + Qt::Key_D);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotDecreaseSelectedChannelsAmplitude()));

    VARIABLE = MENU->addAction(tr("Reset Selected Channel &Offsets"));
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotResetOffsets()));

    VARIABLE = MENU->addAction(tr("Reset Selected Channel &Amplitudes"));
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotResetGains()));

    VARIABLE = MENU->addAction(tr("&Set Current Offsets as Defaults"));
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotSetDefaultOffsets()));

    VARIABLE = MENU->addAction(tr("Set Default Offsets to &Zero"));
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotResetDefaultOffsets()));


    /// Added by M.Zugaro to enable automatic forward paging
    VARIABLE = MENU->addAction(tr("Page"));
    VARIABLE->setShortcuts(Qt::CTRL + Qt::SHIFT + Qt::Key_Space);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(page()));

    VARIABLE = MENU->addAction(tr("Accelerate"));
    VARIABLE->setShortcuts(Qt::CTRL + Qt::Key_Up);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(accelerate()));

    VARIABLE = MENU->addAction(tr("Decelerate"));
    VARIABLE->setShortcuts(Qt::CTRL + Qt::Key_Down);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(decelerate()));


    //Displays Menu
    VARIABLE = MENU->addAction(tr("&New Display"));
    VARIABLE->setShortcuts(Qt::CTRL + Qt::Key_N);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotNewDisplay()));

    VARIABLE = MENU->addAction(tr("&Rename Active Display"));
    VARIABLE->setShortcuts(Qt::CTRL + Qt::Key_R);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotRenameActiveDisplay()));

    VARIABLE = MENU->addAction(tr("&Close Active Display"));
    VARIABLE->setShortcuts(Qt::CTRL + Qt::Key_W);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotDisplayClose()));


    //Channels Menu
    VARIABLE = MENU->addAction(tr("Show &Channels"),QIcon(":/icons/eye"));
    VARIABLE->setShortcuts(Qt::CTRL + Qt::Key_C);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotShowChannels()));

    VARIABLE = MENU->addAction(tr("&Hide Channels"),QIcon(":/icons/eye_close"));
    VARIABLE->setShortcuts(Qt::CTRL + Qt::Key_H);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotHideChannels()));


    VARIABLE = MENU->addAction(tr("&Move Channels to New Group"),QIcon(":/icons/new_group"));
    VARIABLE->setShortcuts(Qt::CTRL + Qt::Key_G);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotCreateGroup()));

    VARIABLE = MENU->addAction(tr("&Remove Channels from Group"),QIcon(":/icons/remove"));
    VARIABLE->setShortcuts(Qt::SHIFT + Qt::Key_Delete);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotDiscardSpikeChannels()));

    VARIABLE = MENU->addAction(tr("&Discard Channels"),QIcon(":/icons/discard"));
    VARIABLE->setShortcuts(Qt::Key_Delete);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotDiscardChannels()));

    VARIABLE = MENU->addAction(tr("&Keep Channels"),QIcon(":/icons/keep"));
    VARIABLE->setShortcuts(Qt::CTRL + Qt::SHIFT + Qt::Key_K);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotKeepChannels()));

    VARIABLE = MENU->addAction(tr("&Skip Channels"),QIcon(":/icons/skip"));
    VARIABLE->setShortcuts(Qt::CTRL + Qt::SHIFT + Qt::Key_S);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotSkipChannels()));


    VARIABLE = MENU->addAction(tr("&Synchronize Groups"));
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotSynchronize()));

    showHideLabels = MENU->addAction(tr("Show &Labels"),0);
    showHideLabels->setShortcuts(Qt::CTRL + Qt::Key_L);
    showHideLabels->setCheckable(true);
    connect(showHideLabels,SIGNAL(triggered()), this,SLOT(slotShowLabels()));

    showHideLabels->setChecked(false);

    //Color section
    VARIABLE = MENU->addAction(tr("Color by &Anatomical Groups"));
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotApplyDisplayColor()));

    VARIABLE = MENU->addAction(tr("Color by S&pike Groups"));
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotApplySpikeColor()));


    //Units Menu
    clusterVerticalLines = MENU->addAction(tr("&Vertical Lines"));
    clusterVerticalLines->setCheckable(true);
    connect(clusterVerticalLines,SIGNAL(triggered()), this,SLOT(slotClustersVerticalLines()));

    clusterVerticalLines->setChecked(false);
    clusterRaster = MENU->addAction(tr("&Raster"));
    clusterRaster->setCheckable(true);
    connect(clusterRaster,SIGNAL(triggered()), this,SLOT(slotClustersRaster()));

    clusterRaster->setChecked(true);
    clusterWaveforms = MENU->addAction(tr("&Waveforms"));
    clusterWaveforms->setCheckable(true);
    connect(clusterWaveforms,SIGNAL(triggered()), this,SLOT(slotClustersWaveforms()));

    clusterWaveforms->setChecked(false);
    VARIABLE = MENU->addAction(tr("&Increase Height"));
    VARIABLE->setShortcuts(Qt::CTRL + Qt::Key_Plus);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotIncreaseRasterHeight()));

    VARIABLE = MENU->addAction(tr("&Decrease Height"));
    VARIABLE->setShortcuts(Qt::CTRL + Qt::Key_Minus);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotDecreaseRasterHeight()));

    VARIABLE = MENU->addAction(tr("&Next Spike"),QIcon(":/icons/forwardCluster"));
    VARIABLE->setShortcuts(Qt::CTRL + Qt::SHIFT + Qt::Key_F);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotShowNextCluster()));

    VARIABLE = MENU->addAction(tr("&Previous Spike"),QIcon(":/icons/backCluster"));
    VARIABLE->setShortcuts(Qt::CTRL + Qt::SHIFT + Qt::Key_B);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotShowPreviousCluster()));


    //Events Menu
    VARIABLE = MENU->addAction(tr("&Next Event"),QIcon(":/icons/forwardEvent"));
    VARIABLE->setShortcuts(Qt::CTRL + Qt::Key_F);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotShowNextEvent()));

    VARIABLE = MENU->addAction(tr("&Previous Event"),QIcon(":/icons/backEvent"));
    VARIABLE->setShortcuts(Qt::CTRL + Qt::Key_B);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(slotShowPreviousEvent()));

    VARIABLE = MENU->addAction(tr("&Remove Event"),0);
    VARIABLE->setShortcuts(Qt::CTRL + Qt::Key_K);
    connect(VARIABLE,SIGNAL(triggered()), this,SLOT(removeEvent()));


    //Positions Menu
    positionViewToggle = MENU->addAction(tr("&Show Position View"));
    positionViewToggle->setCheckable(true);
    connect(positionViewToggle,SIGNAL(triggered()), this,SLOT(slotShowPositionView()));

    positionViewToggle->setChecked(false);
    showEventsInPositionView = MENU->addAction(tr("Show &Events"));
    showEventsInPositionView->setCheckable(true);
    connect(showEventsInPositionView,SIGNAL(triggered()), this,SLOT(slotShowEventsInPositionView()));

    showEventsInPositionView->setChecked(false);

    //Settings menu
    viewToolBar = MENU->addAction(tr("Show T&ools"));
    viewToolBar->setCheckable(true);
    connect(viewToolBar,SIGNAL(triggered()), this,SLOT(slotViewToolBar()));

    viewToolBar->setChecked(true);
    viewParameterBar = MENU->addAction(tr("Show &Parameters"));
    viewParameterBar->setCheckable(true);
    connect(viewParameterBar,SIGNAL(triggered()), this,SLOT(slotViewParameterBar()));

    viewParameterBar->setChecked(true);
    calibrationBar = MENU->addAction(tr("&Display Calibration"));
    calibrationBar->setCheckable(true);
    connect(calibrationBar,SIGNAL(triggered()), this,SLOT(slotShowCalibration()));

    calibrationBar->setChecked(false);

    //Custom connections
    connect(doc, SIGNAL(noSession(QMap<int,int>&,QMap<int,bool>&)),this, SLOT(slotDefaultSetUp(QMap<int,int>&,QMap<int,bool>&)));
    connect(doc, SIGNAL(loadFirstDisplay(Q3ValueList<int>*,bool,bool,bool,bool,bool,bool,Q3ValueList<int>,Q3ValueList<int>,Q3ValueList<int>,QMap<int,bool>&,long,long,QString,bool,int,bool)),this,
            SLOT(slotSetUp(Q3ValueList<int>*,bool,bool,bool,bool,bool,bool,Q3ValueList<int>,Q3ValueList<int>,Q3ValueList<int>,QMap<int,bool>&,long,long,QString,bool,int,bool)));

    connect(displayChannelPalette, SIGNAL(singleChangeColor(int)),this, SLOT(slotSingleChannelColorUpdate(int)));
    connect(spikeChannelPalette, SIGNAL(singleChangeColor(int)),this, SLOT(slotSingleChannelColorUpdate(int)));
    connect(displayChannelPalette, SIGNAL(singleChangeColor(int)),spikeChannelPalette, SLOT(updateColor(int)));
    connect(spikeChannelPalette, SIGNAL(singleChangeColor(int)),displayChannelPalette, SLOT(updateColor(int)));

    connect(displayChannelPalette, SIGNAL(groupChangeColor(int)),this, SLOT(slotChannelGroupColorUpdate(int)));
    connect(spikeChannelPalette, SIGNAL(groupChangeColor(int)),this, SLOT(slotChannelGroupColorUpdate(int)));
    connect(displayChannelPalette, SIGNAL(groupChangeColor(int)),spikeChannelPalette, SLOT(applyCustomColor()));
    connect(spikeChannelPalette, SIGNAL(groupChangeColor(int)),displayChannelPalette, SLOT(applyCustomColor()));

    /*connect(displayChannelPalette, SIGNAL(channelsChangeColor(QValueList<int>)),this, SLOT(slotChannelsColorUpdate(QValueList<int>)));
  connect(spikeChannelPalette, SIGNAL(channelsChangeColor(QValueList<int>)),this, SLOT(slotChannelsColorUpdate(QValueList<int>)));
  connect(displayChannelPalette, SIGNAL(channelsChangeColor(QValueList<int>)),spikeChannelPalette, SLOT(updateColor(QValueList<int>)));
  connect(spikeChannelPalette, SIGNAL(channelsChangeColor(QValueList<int>)),displayChannelPalette, SLOT(updateColor(QValueList<int>)));
  */
    connect(displayChannelPalette, SIGNAL(updateShownChannels(const Q3ValueList<int>&)),this, SLOT(slotUpdateShownChannels(const Q3ValueList<int>&)));
    connect(spikeChannelPalette, SIGNAL(updateShownChannels(const Q3ValueList<int>&)),this, SLOT(slotUpdateShownChannels(const Q3ValueList<int>&)));

    connect(displayChannelPalette, SIGNAL(updateHideChannels(const Q3ValueList<int>&)),this, SLOT(slotUpdateHiddenChannels(const Q3ValueList<int>&)));
    connect(spikeChannelPalette, SIGNAL(updateHideChannels(const Q3ValueList<int>&)),this, SLOT(slotUpdateHiddenChannels(const Q3ValueList<int>&)));

    connect(displayChannelPalette, SIGNAL(channelsDiscarded(const Q3ValueList<int>&)),this, SLOT(slotChannelsDiscarded(const Q3ValueList<int>&)));
    connect(spikeChannelPalette, SIGNAL(channelsDiscarded(const Q3ValueList<int>&)),this, SLOT(slotChannelsDiscarded(const Q3ValueList<int>&)));
    connect(displayChannelPalette, SIGNAL(channelsMovedToTrash(const Q3ValueList<int>&,QString,bool)),spikeChannelPalette, SLOT(discardChannels(const Q3ValueList<int>&,QString,bool)));
    connect(spikeChannelPalette, SIGNAL(channelsMovedToTrash(const Q3ValueList<int>&,QString,bool)),displayChannelPalette, SLOT(discardChannels(const Q3ValueList<int>&,QString,bool)));
    connect(displayChannelPalette, SIGNAL(channelsMovedAroundInTrash(const Q3ValueList<int>&,QString,bool)),spikeChannelPalette, SLOT(trashChannelsMovedAround(const Q3ValueList<int>&,QString,bool)));
    connect(spikeChannelPalette, SIGNAL(channelsMovedAroundInTrash(const Q3ValueList<int>&,QString,bool)),displayChannelPalette, SLOT(trashChannelsMovedAround(const Q3ValueList<int>&,QString,bool)));

    connect(displayChannelPalette, SIGNAL(channelsRemovedFromTrash(const Q3ValueList<int>&)),spikeChannelPalette, SLOT(removeChannelsFromTrash(const Q3ValueList<int>&)));
    connect(spikeChannelPalette, SIGNAL(channelsRemovedFromTrash(const Q3ValueList<int>&)),displayChannelPalette, SLOT(removeChannelsFromTrash(const Q3ValueList<int>&)));

    connect(displayChannelPalette, SIGNAL(groupModified()),this, SLOT(slotGroupsModified()));
    connect(spikeChannelPalette, SIGNAL(groupModified()),this, SLOT(slotGroupsModified()));


    connect(displayChannelPalette, SIGNAL(channelsSelected(const Q3ValueList<int>&)),this, SLOT(slotChannelsSelected(const Q3ValueList<int>&)));
    connect(spikeChannelPalette, SIGNAL(channelsSelected(const Q3ValueList<int>&)),this, SLOT(slotChannelsSelected(const Q3ValueList<int>&)));

    //Actually create the menus and toolbars
    createGUI();
}


void NeuroscopeApp::initStatusBar()
{
    ///////////////////////////////////////////////////////////////////
    // STATUSBAR
    // TODO: add your own items you need for displaying current application status.
    statusBar()->insertItem(tr("Ready."),1);
}

void NeuroscopeApp::initItemPanel(){


    //Creation of the left panel containing the channels.
    if(displayPaletteHeaders){
        displayPanel = createDockWidget("displayPanel",QPixmap(":/icons/anatomy", 0L, tr("Anatomy"), tr("Anatomy"));
                spikePanel = createDockWidget("spikePanel",QPixmap(":/icons/spikes", 0L, tr("Spikes"), tr("Spikes"));
    }
    else{
        displayPanel = createDockWidget("displayPanel",QPixmap(":/icons/anatomy", 0L,"");
                spikePanel = createDockWidget("spikePanel",QPixmap(":/icons/spikes", 0L,"");
    }
    //Initialisation of the channel palettes containing the channel list
    displayChannelPalette = new ChannelPalette(ChannelPalette::DISPLAY,backgroundColor,true,displayPanel,"DisplaylPalette");
    spikeChannelPalette = new ChannelPalette(ChannelPalette::SPIKE,backgroundColor,true,spikePanel,"SpikePalette");
    //Place the displayChannelPalette and  spikeChannelPalette in the KDockWidgets (the view)
    displayPanel->setWidget(displayChannelPalette);
    spikePanel->setWidget(spikeChannelPalette);

    //Create the paletteArea which will contain all the palettes.
    KDockArea* paletteArea = new KDockArea(mainDock,"PanelArea");
    paletteArea->setMainDockWidget(displayPanel);

    //Create the KDockWidget which will contain the paletteArea and be dock to the mainDock.
    palettePanel = createDockWidget("Palettes", QPixmap(), 0L, tr("Palettes"), tr("Palettes"));
    palettePanel->setWidget(paletteArea);
}

void NeuroscopeApp::executePreferencesDlg(){
    if(prefDialog == 0L){
        prefDialog = new PrefDialog(this);
        // connect to the "settingsChanged" signal
        connect(prefDialog,SIGNAL(settingsChanged()),this,SLOT(applyPreferences()));
    }

    // update the dialog widgets.
    prefDialog->updateDialog();

    if(prefDialog->exec() == QDialog::Accepted){  // execute the dialog
        //if the user did not click the applyButton, save the new settings.
        if(prefDialog->isApplyEnable()){
            prefDialog->updateConfiguration();        // store settings in config object
            applyPreferences();                      // let settings take effect
        }
    }
}

void NeuroscopeApp::applyPreferences() {
    configuration().write();

    if(backgroundColor != configuration().getBackgroundColor()){
        backgroundColor = configuration().getBackgroundColor();
        if(mainDock){
            //loop on the palettes (if their are any skipped channels, their color are going to be updated to the new background color)
            for(int i = 0; i < paletteTabsParent->count();++i){
                KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->page(i));
                if((current->getWidget())->isA("ChannelPalette"))
                    static_cast<ChannelPalette*>(current->getWidget())->changeBackgroundColor(backgroundColor);
                else if((current->getWidget())->isA("ItemPalette"))
                    static_cast<ItemPalette*>(current->getWidget())->changeBackgroundColor(backgroundColor);
            }
            doc->setBackgroundColor(backgroundColor); //will take care of displays
        }
    }

    if(displayPaletteHeaders != configuration().isPaletteHeadersDisplayed()){
        displayPaletteHeaders = configuration().isPaletteHeadersDisplayed();
        if(mainDock){
            if(displayPaletteHeaders){
                for(int i = 0; i < paletteTabsParent->count();++i){
                    KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->page(i));
                    QString name = current->name();
                    QString label;
                    if(name.contains("displayPanel")) label = "Anatomy";
                    else if(name.contains("spikePanel")) label = "Spikes";
                    else if(name.contains("clusterPanel")) label = "Units";
                    else if(name.contains("eventPanel")) label = "Events";

                    paletteTabsParent->setTabLabel(current,label);
                    current->setTabPageLabel(label);
                }
            }
            else{
                for(int i = 0; i< paletteTabsParent->count();i++){
                    KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->page(i));
                    paletteTabsParent->setTabLabel(current,"");
                    current->setTabPageLabel("");
                }
            }
        }
        else{
            if(displayPaletteHeaders){
                displayPanel->setTabPageLabel("Anatomy");
                spikePanel->setTabPageLabel("Spikes");
            }
            else{
                displayPanel->setTabPageLabel("");
                spikePanel->setTabPageLabel("");
            }
        }
    }

    if(eventPosition != configuration().getEventPosition()){
        eventPosition = configuration().getEventPosition();
        doc->setEventPosition(eventPosition);
    }

    if(clusterPosition != configuration().getClusterPosition()){
        clusterPosition = configuration().getClusterPosition();
        doc->setClusterPosition(clusterPosition);
    }

    if(nbSamplesDefault != configuration().getNbSamples() || peakIndexDefault != configuration().getPeakIndex()){
        nbSamplesDefault = configuration().getNbSamples();
        peakIndexDefault = configuration().getPeakIndex();
        doc->setDefaultWaveformInformation(nbSamplesDefault,peakIndexDefault);
    }

    if(traceBackgroundImageDefault != configuration().getTraceBackgroundImage()){
        traceBackgroundImageDefault = configuration().getTraceBackgroundImage();
        doc->setDefaultTraceBackgroundImage(traceBackgroundImageDefault);
        if(!mainDock) doc->setTraceBackgroundImage(traceBackgroundImageDefault);
    }

    if(videoSamplingRateDefault != configuration().getVideoSamplingRate() || videoWidthDefault != configuration().getWidth() ||
            videoHeightDefault != configuration().getHeight() || backgroundImageDefault != configuration().getBackgroundImage() ||
            rotationDefault != configuration().getRotation() || flipDefault != configuration().getFlip() || drawPositionsOnBackgroundDefault != configuration().getPositionsBackground()){

        videoSamplingRateDefault = configuration().getVideoSamplingRate();
        videoWidthDefault = configuration().getWidth();
        videoHeightDefault = configuration().getHeight();
        backgroundImageDefault = configuration().getBackgroundImage();
        rotationDefault = configuration().getRotation();
        flipDefault = configuration().getFlip();
        drawPositionsOnBackgroundDefault = configuration().getPositionsBackground();
        doc->setDefaultPositionInformation(videoSamplingRateDefault,videoWidthDefault,videoHeightDefault,
                                           backgroundImageDefault,rotationDefault,flipDefault,drawPositionsOnBackgroundDefault);
    }

    if(initialOffsetDefault != configuration().getOffset()){
        initialOffsetDefault = configuration().getOffset();
        doc->setDefaultInitialOffset(initialOffsetDefault);
        if(!mainDock)doc->setInitialOffset(initialOffsetDefault);
    }

    if(channelNbDefault != configuration().getNbChannels()){
        channelNbDefault = configuration().getNbChannels();
        doc->setDefaultChannelNb(channelNbDefault);
        if(!mainDock) doc->setChannelNb(channelNbDefault);
    }

    if(datSamplingRateDefault != configuration().getDatSamplingRate()){
        datSamplingRateDefault = configuration().getDatSamplingRate();
        doc->setDefaultDatSamplingRate(datSamplingRateDefault);
    }

    if(eegSamplingRateDefault != configuration().getEegSamplingRate()){
        eegSamplingRateDefault = configuration().getEegSamplingRate();
        doc->setDefaultEegSamplingRate(eegSamplingRateDefault);
    }

    if(resolutionDefault != configuration().getResolution()){
        resolutionDefault = configuration().getResolution();
        doc->setDefaultResolution(resolutionDefault);
        if(!mainDock)doc->setResolution(resolutionDefault);
    }

    if(voltageRangeDefault != configuration().getVoltageRangeDefault()){
        voltageRangeDefault = configuration().getVoltageRangeDefault();
        if(amplificationDefault != configuration().getAmplificationDefault())
            amplificationDefault = configuration().getAmplificationDefault();
        if(screenGainDefault != configuration().getScreenGainDefault())
            doc->setDefaultGains(voltageRangeDefault,amplificationDefault,screenGainDefault);
        if(!mainDock)doc->setGains(voltageRangeDefault,amplificationDefault,screenGainDefault);
    }

    if(amplificationDefault != configuration().getAmplificationDefault()){
        amplificationDefault = configuration().getAmplificationDefault();
        if(screenGainDefault != configuration().getScreenGainDefault())
            screenGainDefault = configuration().getScreenGainDefault();
        doc->setDefaultGains(voltageRangeDefault,amplificationDefault,screenGainDefault);
        if(!mainDock)doc->setGains(voltageRangeDefault,amplificationDefault,screenGainDefault);
    }

    if(screenGainDefault != configuration().getScreenGainDefault()){
        screenGainDefault = configuration().getScreenGainDefault();
        doc->setDefaultGains(voltageRangeDefault,amplificationDefault,screenGainDefault);
        if(!mainDock)doc->setGains(voltageRangeDefault,amplificationDefault,screenGainDefault);
    }
}

void NeuroscopeApp::initializePreferences(){
    backgroundColor =  configuration().getBackgroundColor();
    displayPaletteHeaders = configuration().isPaletteHeadersDisplayed();
    screenGainDefault = configuration().getScreenGain();
    amplificationDefault = configuration().getAmplification();
    voltageRangeDefault = configuration().getVoltageRange();
    channelNbDefault = configuration().getNbChannels();
    datSamplingRateDefault = configuration().getDatSamplingRate();
    eegSamplingRateDefault = configuration().getEegSamplingRate();
    initialOffsetDefault = configuration().getOffset();
    resolutionDefault = configuration().getResolution();
    eventPosition = configuration().getEventPosition();
    clusterPosition = configuration().getClusterPosition();
    nbSamplesDefault = configuration().getNbSamples();
    peakIndexDefault = configuration().getPeakIndex();
    videoSamplingRateDefault = configuration().getVideoSamplingRate();
    videoWidthDefault = configuration().getWidth();
    videoHeightDefault = configuration().getHeight();
    backgroundImageDefault = configuration().getBackgroundImage();
    traceBackgroundImageDefault = configuration().getTraceBackgroundImage();
    rotationDefault = configuration().getRotation();
    flipDefault = configuration().getFlip();
    drawPositionsOnBackgroundDefault = configuration().getPositionsBackground();
}

void NeuroscopeApp::initDisplay(Q3ValueList<int>* channelsToDisplay,Q3ValueList<int> offsets,Q3ValueList<int> channelGains,
                                Q3ValueList<int> selectedChannels,QMap<int,bool>& skipStatus,int rasterHeight,long duration,long startTime,QString tabLabel)
{ 
    isInit = true; //prevent the spine boxes or the lineedit and the editline to trigger during initialisation
    //Initialize the spinboxe and scrollbar

    //Create the mainDock (first view)
    if(tabLabel == "") tabLabel = "Field Potentials Display";
    mainDock = createDockWidget( "1", QPixmap(), 0L, tr(doc->url().path()),tabLabel);
    mainDock->setDockWindowTransient(this,true);

    isInit = false; //now a change in a spine box or the lineedit will trigger an update of the display

    NeuroscopeView* view = new NeuroscopeView(*this,tabLabel,startTime,duration,backgroundColor,WDestructiveClose,statusBar(),channelsToDisplay,greyScale->isChecked(),
                                              doc->tracesDataProvider(),displayMode->isChecked(),clusterVerticalLines->isChecked(),
                                              clusterRaster->isChecked(),clusterWaveforms->isChecked(),showHideLabels->isChecked(),doc->getGain(),doc->getAcquisitionGain(),
                                              doc->channelColors(),doc->getDisplayGroupsChannels(),doc->getDisplayChannelsGroups(),
                                              offsets,channelGains,selectedChannels,skipStatus,rasterHeight,doc->getTraceBackgroundImage(),mainDock,"TracesDisplay");

    view->installEventFilter(this);

    connect(view,SIGNAL(channelsSelected(const Q3ValueList<int>&)),this, SLOT(slotSelectChannelsInPalette(const Q3ValueList<int>&)));
    connect(view,SIGNAL(eventModified(QString,int,double,double)),this, SLOT(slotEventModified(QString,int,double,double)));
    connect(view,SIGNAL(eventRemoved(QString,int,double)),this, SLOT(slotEventRemoved(QString,int,double)));
    connect(view,SIGNAL(eventAdded(QString,QString,double)),this, SLOT(slotEventAdded(QString,QString,double)));
    connect(view,SIGNAL(positionViewClosed()),this, SLOT(positionViewClosed()));

    //Keep track of the number of displays
    displayCount ++;

    //Update the document's list of view
    doc->addView(view);
    mainDock->setWidget(view);

    //allow dock on the left side only
    mainDock->setDockSite(KDockWidget::DockLeft);

    setView(mainDock); // central widget in a KDE mainwindow <=> setMainWidget
    setMainDockWidget(mainDock);

    //disable docking abilities of mainDock itself
    mainDock->setEnableDocking(KDockWidget::DockNone);

    //Initialize and dock the displayPanel
    //Create the channel lists and select the channels which will be drawn
    displayChannelPalette->createChannelLists(doc->channelColors(),doc->getDisplayGroupsChannels(),doc->getDisplayChannelsGroups());
    displayChannelPalette->updateShowHideStatus(*channelsToDisplay,true);
    spikeChannelPalette->createChannelLists(doc->channelColors(),doc->getSpikeGroupsChannels(),doc->getChannelsSpikeGroups());
    spikeChannelPalette->updateShowHideStatus(*channelsToDisplay,true);
    displayChannelPalette->setGreyScale(greyScale->isChecked());
    spikeChannelPalette->setGreyScale(greyScale->isChecked());

    //Update the skip status of the channels
    displayChannelPalette->updateSkipStatus(skipStatus);

    //update the channel palettes selection
    if(selectedChannels.size() > 0){
        spikeChannelPalette->selectChannels(selectedChannels);
        displayChannelPalette->selectChannels(selectedChannels);
    }

    displayPanel->setEnableDocking(KDockWidget::DockFullSite);
    spikePanel->setEnableDocking(KDockWidget::DockFullSite);
    displayPanel->setDockSite(KDockWidget::DockFullSite);
    spikePanel->setDockSite(KDockWidget::DockFullSite);

    //Add spikeChannelPalette as a tab and get a new DockWidget, grandParent of the target (displayPanel)
    //and spikeChannelPalette.
    KDockWidget* grandParent = spikePanel->manualDock(displayPanel,KDockWidget::DockCenter);

    displayPanel->setEnableDocking(KDockWidget::DockNone);
    spikePanel->setEnableDocking(KDockWidget::DockNone);
    displayPanel->setDockSite(KDockWidget::DockNone);
    spikePanel->setDockSite(KDockWidget::DockNone);

    //The grandParent's widget is the KDockTabGroup regrouping all the tabs
    paletteTabsParent = static_cast<KDockTabGroup*>(grandParent->getWidget());

    //Connect the change tab signal to slotPaletteTabChange(QWidget* widget) to trigger updates when
    //the active palette changes.
    connect(paletteTabsParent, SIGNAL(currentChanged(QWidget*)), this, SLOT(slotPaletteTabChange(QWidget*)));

    //Disable the possibility to dock the palette or to dock into it.
    grandParent->setEnableDocking(KDockWidget::DockNone);
    grandParent->setDockSite(KDockWidget::DockNone);

    //allow dock on the right side only (the displays will be on the rigth side)
    palettePanel->setDockSite(KDockWidget::DockRight);

    //Dock the palettePanel on the left
    palettePanel->setEnableDocking(KDockWidget::DockFullSite);
    palettePanel->manualDock(mainDock,KDockWidget::DockLeft,20);  // relation target/this (in percent)

    //forbit docking abilities of palettePanel itself
    palettePanel->setEnableDocking(KDockWidget::DockNone);

    //Enable some actions now that a document is open (see the klustersui.rc file)
    slotStateChanged("documentState");
    slotStateChanged("displayChannelState");
    if(clusterRaster->isChecked()) slotStateChanged("clusterRasterState");
    else slotStateChanged("noClusterRasterState");
}

void NeuroscopeApp::openDocumentFile(const QString& url)
{
    slotStatusMsg(tr("Opening file..."));

    filePath = url.path();
    QFileInfo file(filePath);

    if(url.protocol() == "file"){
        if((fileOpenRecent->items().contains(url.prettyURL())) && !file.exists()){
            QString title = "File not found: ";
            title.append(filePath);
            int answer = KMessageBox::questionYesNo(this,tr("The selected file no longer exists. Do you want to remove it from the list?"), tr(title));
            if(answer == KMessageBox::Yes) fileOpenRecent->removeURL(url);
            else  fileOpenRecent->addURL(url); //hack, unselect the item
            filePath = "";
            return;
        }
    }
    //Do not handle remote files
    else{
        KMessageBox::sorry(this,tr("Sorry, NeuroScope does not handle remote files."),tr("Remote file handling"));
        fileOpenRecent->removeURL(url);
        return;
    }

    //Check if the file exists
    if(!file.exists()){
        KMessageBox::error (this,tr("The selected file does not exist."), tr("Error!"));
        fileOpenRecent->removeURL(url);
        return;
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    //If no document is open already, open the document asked.
    if(!mainDock){
        displayCount = 0;
        fileOpenRecent->addURL(url);

        // Open the file (that will also initialize the document)
        int returnStatus = doc->openDocument(url);
        if(returnStatus == NeuroscopeDoc::INCORRECT_FILE){
            QApplication::restoreOverrideCursor();
            KMessageBox::error (this,tr("The selected file is invalid, it has to be of the form baseName.nrs, baseName.xml or baseName.*"), tr("Error!"));
            //close the document
            doc->closeDocument();
            resetState();
            return;
        }
        if(returnStatus == NeuroscopeDoc::DOWNLOAD_ERROR){
            QApplication::restoreOverrideCursor();
            KMessageBox::error (this,tr("Could not get the data file."), tr("Error!"));
            //close the document
            doc->closeDocument();
            resetState();
            return;
        }
        if(returnStatus == NeuroscopeDoc::OPEN_ERROR){
            QApplication::restoreOverrideCursor();
            KMessageBox::error (this,tr("Could not open the files."), tr("Error!"));
            //close the document
            doc->closeDocument();
            resetState();
            return;
        }
        if(returnStatus == NeuroscopeDoc::PARSE_ERROR){
            QApplication::restoreOverrideCursor();
            KMessageBox::error(this,tr("Either the parameter file or the session file could not be parsed correctly."), tr("IO Error!"));
            //close the document
            doc->closeDocument();
            resetState();
            return;
        }
        if(returnStatus == NeuroscopeDoc::MISSING_FILE){
            QApplication::restoreOverrideCursor();
            KMessageBox::error(this,tr("The parameter file is missing."), tr("IO Error!"));
            //close the document
            doc->closeDocument();
            resetState();
            return;
        }
        

        //Save the recent file list
        fileOpenRecent->saveEntries(config);

        //update the spike and event browsing status
        updateBrowsingStatus();

        //Update the menu related to the display of events in the PositionView
        if(isPositionFileLoaded && !eventFileList.isEmpty()) slotStateChanged("eventsInPositionViewEnableState");

        setCaption(url.path());
        QApplication::restoreOverrideCursor();
    }
    // check, if this document is already open. If yes, do not do anything
    else{
        QString path = doc->url().path();

        if(path == url.path()){
            fileOpenRecent->addURL(url); //hack, unselect the item
            QApplication::restoreOverrideCursor();
            return;
        }
        //If the document asked is not the already open. Open a new instance of the application with it.
        else{
            fileOpenRecent->addURL(url);
            //Save the recent file list
            fileOpenRecent->saveEntries(config);
            filePath = path;

            QProcess::startDetached("neuroscope", QStringList()<<url.path());
            QApplication::restoreOverrideCursor();
        }
    }

    slotStatusMsg(tr("Ready."));
}


NeuroscopeDoc* NeuroscopeApp::getDocument() const
{
    return doc;
}

void NeuroscopeApp::updateBrowsingStatus(){
    if(!clusterFileList.isEmpty()){
        ItemPalette* palette;
        for(int i = 0; i<paletteTabsParent->count();i++){
            KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->page(i));
            QString name = current->name();
            if((current->getWidget())->isA("ItemPalette") && name.contains("clusterPanel")){
                palette = static_cast<ItemPalette*>(current->getWidget());
                break;
            }
        }

        NeuroscopeView* view = activeView();
        Q3ValueList<QString>::iterator iterator;
        for(iterator = clusterFileList.begin(); iterator != clusterFileList.end(); ++iterator){
            const Q3ValueList<int>* selectedClusters = view->getSelectedClusters(*iterator);
            const Q3ValueList<int>* skippedClusterIds = view->getClustersNotUsedForBrowsing(*iterator);
            palette->selectItems(*iterator,*selectedClusters,*skippedClusterIds);
        }

        if(!palette->isBrowsingEnable()) slotStateChanged("noClusterBrowsingState");
        else slotStateChanged("clusterBrowsingState");
    }
    if(!eventFileList.isEmpty()){
        ItemPalette* palette;
        for(int i = 0; i<paletteTabsParent->count();i++){
            KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->page(i));
            QString name = current->name();
            if((current->getWidget())->isA("ItemPalette") && name.contains("eventPanel")){
                palette = static_cast<ItemPalette*>(current->getWidget());
                break;
            }
        }

        NeuroscopeView* view = activeView();
        Q3ValueList<QString>::iterator iterator;
        for(iterator = eventFileList.begin(); iterator != eventFileList.end(); ++iterator){
            const Q3ValueList<int>* selectedEvents = view->getSelectedEvents(*iterator);
            const Q3ValueList<int>* skippedEventIds = view->getEventsNotUsedForBrowsing(*iterator);
            palette->selectItems(*iterator,*selectedEvents,*skippedEventIds);
        }

        if(!palette->isBrowsingEnable()) slotStateChanged("noEventBrowsingState");
        else slotStateChanged("eventBrowsingState");
    }
}

void NeuroscopeApp::saveProperties()
{
#if KDAB_PENDING
    // the 'config' object points to the session managed
    // config file.  anything you write here will be available
    // later when this app is restored

    //Save the recent file list
    fileOpenRecent->saveEntries(config);
    config->writePathEntry("openFile",filePath);
#endif
}


void NeuroscopeApp::readProperties()
{
#if KDAB_PENDING
    // the 'config' object points to the session managed
    // config file.  this function is automatically called whenever
    // the app is being restored.  read in here whatever you wrote
    // in 'saveProperties'

    // initialize the recent file list
    fileOpenRecent->loadEntries(config);
    filePath = config->readPathEntry("openFile");
    QString url;
    url.setPath(filePath);
    openDocumentFile(url);
#endif
}


void NeuroscopeApp::slotGroupsModified(){
    groupsModified = true;

    NeuroscopeView* view = activeView();
    doc->groupsModified(view);
}


bool NeuroscopeApp::queryClose()
{
    //Save the recent file list
    fileOpenRecent->saveEntries(config);

    //call when the kDockMainWindow will be close
    if(doc == 0 || !doc->isADocumentToClose()) return true;
    else{
        QApplication::restoreOverrideCursor();
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        //try to close the document
        if(doc->canCloseDocument(this,"queryClose")){
            //If either groups, colors or events have been modified, ask if the user wants to save them
            int saveStatus;
            int eventSaveStatus = 0;
            if(groupsModified || colorModified || eventsModified){
                QApplication::restoreOverrideCursor();
                QString message;
                QString title;
                if((groupsModified || colorModified) && !eventsModified){
                    message = "Your configuration has changed, do you want to save it?";
                    title = "Configuration modification";
                }
                else if(eventsModified && !groupsModified && !colorModified){
                    message = "Some events have changed, do you want to save the event file(s)?";
                    title = "Event modification";
                }
                else if((groupsModified || colorModified) && eventsModified){
                    message = "Your configuration and some events have changed, do you want to save the configuration and the event file(s)?";
                    title = "Modification";
                }
                switch(KMessageBox::warningYesNoCancel(0,tr(message),tr(title),KGuiItem("Save"),KGuiItem("Discard"))){
                case KMessageBox::Yes://<=> Save
                    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                    if(eventsModified){
                        eventSaveStatus = doc->saveEventFiles();
                        QApplication::restoreOverrideCursor();
                        if(eventSaveStatus != IO_Ok){
                            switch(KMessageBox::warningYesNo(0,tr("The event file(s) could not be saved possibly because of insufficient file access permissions."
                                                                  "Close anyway ?"), tr("I/O Error !"),
                                                             KGuiItem("Close"),KGuiItem("Discard"))){
                            case KMessageBox::Yes:
                                break;
                            case KMessageBox::No:
                                return false;
                            }
                        }
                    }//eventsModified
                    //save session
                    saveStatus = doc->saveSession();
                    QApplication::restoreOverrideCursor();
                    if(saveStatus != NeuroscopeDoc::OK){
                        if(saveStatus == NeuroscopeDoc::CREATION_ERROR){
                            message = "The current session could not be saved possibly because of insufficient file access permissions."
                                    " You may consider saving your session file to another location using the Save As entry in the File menu.\n"
                                    "Close anyway ?";
                            title = "I/O Error !";
                        }
                        else if(saveStatus == NeuroscopeDoc::PARSE_ERROR){
                            message = "The current session could not be saved because the parameter file is incorrect.\nClose anyway ?";
                            title = "Parsing error !";
                        }
                        else if(saveStatus == NeuroscopeDoc::NOT_WRITABLE){
                            message = "The current session could not be saved because the parameter file is not writable.\nClose anyway ?";
                            title = "Writing error !";
                        }
                        switch(KMessageBox::warningYesNo(0,tr(message), tr(title),KGuiItem("Close"),KGuiItem("Discard"))){
                        case KMessageBox::Yes:
                            break;
                        case KMessageBox::No:
                            return false;
                        }
                    }
                    break;
                case KMessageBox::No://<=> Discard
                    break;
                case KMessageBox::Cancel:
                    return false;
                }
            }
            else{
                //If neither groups, colors or events have been modified, save the session without asking
                saveStatus = doc->saveSession();
                QString message;
                QString title;
                QApplication::restoreOverrideCursor();
                if(saveStatus != NeuroscopeDoc::OK){
                    if(saveStatus == NeuroscopeDoc::CREATION_ERROR){
                        message = "The current session could not be saved possibly because of insufficient file access permissions."
                                " You may consider saving your session file to another location using the Save As entry in the File menu.\n"
                                "Close anyway ?";
                        title = "I/O Error !";
                    }
                    else if(saveStatus == NeuroscopeDoc::PARSE_ERROR){
                        message = "The current session could not be saved because the parameter file is incorrect.\nClose anyway ?";
                        title = "Parsing error !";
                    }
                    else if(saveStatus == NeuroscopeDoc::NOT_WRITABLE){
                        message = "The current session could not be saved because the parameter file is not writable.\nClose anyway ?";
                        title = "Writing error !";
                    }
                    switch(KMessageBox::warningYesNo(0,tr(message), tr(title),KGuiItem("Close"),KGuiItem("Discard"))){
                    case KMessageBox::Yes:
                        break;
                    case KMessageBox::No:
                        return false;
                    }
                }
            }
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            doc->closeDocument();
            QApplication::restoreOverrideCursor();
            return true;
        }
        else return false;
    }
}

bool NeuroscopeApp::queryExit()
{
    return true;
}

/////////////////////////////////////////////////////////////////////
// SLOT IMPLEMENTATION
/////////////////////////////////////////////////////////////////////


void NeuroscopeApp::slotFileOpen()
{
    slotStatusMsg(tr("Opening file..."));

    QString url=QFileDialog::getOpenFileName(this, tr("Open File..."),QString(),
                                             tr("*.dat *.eeg *.fil|Data File (*.dat), EEG File (*.eeg), Filter File (*.fil)\n*.dat|Data File (*.dat)\n*.eeg|EEG File (*.eeg)\n*.fil|Filter File (*.fil)\n*|All files") );
    if(!url.isEmpty())
    {
        openDocumentFile(url);
    }

    slotStatusMsg(tr("Ready."));
}

void NeuroscopeApp::slotLoadClusterFiles(){
    slotStatusMsg(tr("Loading cluster file(s)..."));

    QStringList urls=QFileDialog::getOpenFileNames(this, tr("Open Cluster Files..."),QString(),
                                                   tr("*.clu.*|Cluster File (*.clu.n)\n*.clu|Cluster File (*.clu)"));
    if(urls.size() != 0)
    {
        loadClusterFiles(urls);
    }

    slotStatusMsg(tr("Ready."));
}


void NeuroscopeApp::slotLoadEventFiles(){
    slotStatusMsg(tr("Loading event file(s)..."));

    QStringList urls=QFileDialog::getOpenFileNames(this, tr("Open Event Files..."),QString(),
                                                   tr("*.evt *.evt.*|Event File (*.evt, *.evt.*)"));
    if(urls.size() != 0)
    {
        loadEventFiles(urls);
    }

    slotStatusMsg(tr("Ready."));
}

void NeuroscopeApp::slotLoadPositionFile(){
    slotStatusMsg(tr("Loading position file..."));

    QString url=QFileDialog::getOpenFileName(this, tr("Open position File..."),QString(),
                                             tr("*|All Files") );
    if(!url.isEmpty())
    {
        loadPositionFile(url);
    }

    slotStatusMsg(tr("Ready."));

}

void NeuroscopeApp::slotCreateEventFile(){
    slotStatusMsg(tr("Creating an event file..."));

    const QString& docUrl = doc->url();
    QString eventUrl(docUrl);
    QString baseName = doc->documentBaseName();
    eventUrl.setFileName(baseName);

    QFileDialog dialog(this,tr("CreateEvent"),eventUrl.path(),tr("*.evt *.evt.*|Event file (*.evt, *.evt.*)"));
    //KDAB: can't change button label in qfiledialog
    //KPushButton* ok = dialog.okButton();
    ok->setText(tr("Create"));
    dialog.setCaption(tr("Create Event File as..."));
    if(!dialog.exec())
        return;
    QString url = dialog.selectedFiles().first();

    if(!url.isEmpty()){
        //Check if the file already exist
        QFileInfo fileInfo = QFileInfo(url.path());
        if(fileInfo.exists()){
            KMessageBox::error(this,tr("The selected file already exist."), tr("Error!"));
            return;
        }

        NeuroscopeView* view = activeView();
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        int returnStatus = doc->createEventFile(url,view);

        if(returnStatus == NeuroscopeDoc::INCORRECT_FILE){
            QApplication::restoreOverrideCursor();
            KMessageBox::error (this,tr("The selected file name is invalid, it has to be of the form baseName.id.evt or baseName.evt.id (with id a 3 character identifier)."), tr("Error!"));
        }
        else if(returnStatus == NeuroscopeDoc::ALREADY_OPENED){
            QApplication::restoreOverrideCursor();
            KMessageBox::error (this,tr("The selected file name is already opened."), tr("Error!"));
        }
        else{
            QString eventFileId = doc->lastLoadedProviderName();
            if(eventFileList.isEmpty()) createEventPalette(eventFileId);
            else addEventFile(eventFileId);
            eventsModified = true;
            QApplication::restoreOverrideCursor();
        }
    }

    slotStatusMsg(tr("Ready."));
}

void NeuroscopeApp::slotFileOpenRecent(const QString& url){
    slotStatusMsg(tr("Opening file..."));

    openDocumentFile(url);

    slotStatusMsg(tr("Ready."));
}

void NeuroscopeApp::slotFileClose(){ 
    if(doc != 0){
        QApplication::restoreOverrideCursor();
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        //try to close the document
        if(doc->canCloseDocument(this,"fileClose")){
            //If either groups, colors or events have been modified, ask if the user wants to save them
            int saveStatus;
            int eventSaveStatus = 0;
            if(groupsModified || colorModified || eventsModified){
                QApplication::restoreOverrideCursor();
                QString message;
                QString title;
                if((groupsModified || colorModified) && !eventsModified){
                    message = "Your configuration has changed, do you want to save it?";
                    title = "Configuration modification";
                }
                else if(eventsModified && !groupsModified && !colorModified){
                    message = "Some events have changed, do you want to save the event file(s)?";
                    title = "Event modification";
                }
                else if((groupsModified || colorModified) && eventsModified){
                    message = "Your configuration and some events have changed, do you want to save the configuration and the event file(s)?";
                    title = "Modification";
                }
                switch(KMessageBox::warningYesNoCancel(0,tr(message),tr(title),KGuiItem("Save"),KGuiItem("Discard"))){
                case KMessageBox::Yes://<=> Save
                    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                    if(eventsModified){
                        eventSaveStatus = doc->saveEventFiles();
                        QApplication::restoreOverrideCursor();
                        if(eventSaveStatus != IO_Ok){
                            switch(KMessageBox::warningYesNo(0,tr("The event file(s) could not be saved possibly because of insufficient file access permissions."
                                                                  "Close anyway ?"), tr("I/O Error !"),
                                                             KGuiItem("Close"),KGuiItem("Discard"))){
                            case KMessageBox::Yes:
                                break;
                            case KMessageBox::No:
                                return;
                            }
                        }
                    }//eventsModified
                    //Save session
                    saveStatus = doc->saveSession();
                    QApplication::restoreOverrideCursor();
                    if(saveStatus != NeuroscopeDoc::OK){
                        if(saveStatus == NeuroscopeDoc::CREATION_ERROR){
                            message = "The current session could not be saved possibly because of insufficient file access permissions."
                                    " You may consider saving your session file to another location using the Save As entry in the File menu.\n"
                                    "Close anyway ?";
                            title = "I/O Error !";
                        }
                        else if(saveStatus == NeuroscopeDoc::PARSE_ERROR){
                            message = "The current session could not be saved because the parameter file is incorrect.\nClose anyway ?";
                            title = "Parsing error !";
                        }
                        else if(saveStatus == NeuroscopeDoc::NOT_WRITABLE){
                            message = "The current session could not be saved because the parameter file is not writable.\nClose anyway ?";
                            title = "Writing error !";
                        }
                        switch(KMessageBox::warningYesNo(0,tr(message), tr(title),KGuiItem("Close"),KGuiItem("Discard"))){
                        case KMessageBox::Yes:
                            break;
                        case KMessageBox::No:
                            return;
                        }
                    }
                    break;
                case KMessageBox::No://<=> Discard
                    break;
                case KMessageBox::Cancel:
                    return;
                }
            }
            else{
                //If neither groups, colors or events have been modified, save the session without asking
                saveStatus = doc->saveSession();
                QString message;
                QString title;
                QApplication::restoreOverrideCursor();
                if(saveStatus != NeuroscopeDoc::OK){
                    if(saveStatus == NeuroscopeDoc::CREATION_ERROR){
                        message = "The current session could not be saved possibly because of insufficient file access permissions."
                                " You may consider saving your session file to another location using the Save As entry in the File menu.\n"
                                "Close anyway ?";
                        title = "I/O Error !";
                    }
                    else if(saveStatus == NeuroscopeDoc::PARSE_ERROR){
                        message = "The current session could not be saved because the parameter file is incorrect.\nClose anyway ?";
                        title = "Parsing error !";
                    }
                    else if(saveStatus == NeuroscopeDoc::NOT_WRITABLE){
                        message = "The current session could not be saved because the parameter file is not writable.\nClose anyway ?";
                        title = "Writing error !";
                    }
                    switch(KMessageBox::warningYesNo(0,tr(message), tr(title),KGuiItem("Close"),KGuiItem("Discard"))){
                    case KMessageBox::Yes:
                        break;
                    case KMessageBox::No:
                        return;
                    }
                }
            }
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            //remove all the displays
            if(tabsParent){
                //Remove the display from the group of tabs
                while(true){
                    int nbOfTabs = tabsParent->count();
                    KDockWidget* current = static_cast<KDockWidget*>(tabsParent->page(0));
                    if(current == mainDock){
                        current = static_cast<KDockWidget*>(tabsParent->page(1));
                    }
                    tabsParent->removePage(current);
                    delete current;
                    if(nbOfTabs == 2) break; //the reminding one is the mainDock
                }
                tabsParent = 0L;
            }

            //remove the cluster and event palettes if any
            while(paletteTabsParent->count() > 2){
                KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->page(0));
                if((current->getWidget())->isA("ChannelPalette")){
                    current = static_cast<KDockWidget*>(paletteTabsParent->page(1));
                    if((current->getWidget())->isA("ChannelPalette"))
                        current = static_cast<KDockWidget*>(paletteTabsParent->page(2));
                }
                paletteTabsParent->removePage(current);
                delete current;
            }

            //reset the channel palettes and hide the channel panels
            spikeChannelPalette->reset();
            displayChannelPalette->reset();
            palettePanel->hide();
            palettePanel->undock();

            //Delete the view
            delete mainDock;
            mainDock = 0L;
            doc->closeDocument();
            resetState();
            QApplication::restoreOverrideCursor();
        }
    }
}

void NeuroscopeApp::slotFilePrint()
{
    slotStatusMsg(tr("Printing..."));

    printer->setOrientation(QPrinter::Landscape);
    printer->setColorMode(QPrinter::Color);
    //KDAB remove it
    //printer->addDialogPage(new printDialogPage(this));

    if (printer->setup(this))
    {
        //retrieve the backgroundColor setting from KPrinter object, //1 <=> white background
        int whiteBackground = printer->option("kde-neuroscope-backgroundColor").toInt();
        NeuroscopeView* view = activeView();
        if(whiteBackground == 1){
            //update the color of the skipped channels to white
            doc->updateSkippedChannelColors(true,backgroundColor);
            view->print(printer,filePath,true);
            //update the color of the skipped channels to the background color
            doc->updateSkippedChannelColors(false,backgroundColor);
        }
        else view->print(printer,filePath,false);
    }

    slotStatusMsg(tr("Ready."));
}

void NeuroscopeApp::slotFileQuit()
{
    slotStatusMsg(tr("Exiting..."));
    close();

    slotStatusMsg(tr("Ready."));
}

void NeuroscopeApp::slotViewMainToolBar()
{
    slotStatusMsg(tr("Toggle the main toolbar..."));

    // turn Toolbar on or off
    if(!viewMainToolBar->isChecked())
    {
        toolBar("mainToolBar")->hide();
    }
    else
    {
        toolBar("mainToolBar")->show();
    }

    slotStatusMsg(tr("Ready."));
}

void NeuroscopeApp::slotViewToolBar()
{
    slotStatusMsg(tr("Toggle the tools..."));

    // turn Toolbar on or off
    if(!viewToolBar->isChecked())
    {
        toolBar("toolBar")->hide();
    }
    else
    {
        toolBar("toolBar")->show();
    }

    slotStatusMsg(tr("Ready."));
}

void NeuroscopeApp::slotViewStatusBar()
{
    slotStatusMsg(tr("Toggle the statusbar..."));
    ///////////////////////////////////////////////////////////////////
    //turn Statusbar on or off
    if(!viewStatusBar->isChecked())
    {
        statusBar()->hide();
    }
    else
    {
        statusBar()->show();
    }

    slotStatusMsg(tr("Ready."));
}

void NeuroscopeApp::slotViewParameterBar(){
    slotStatusMsg(tr("Toggle the parameters..."));

    // turn Toolbar on or off
    if(!viewParameterBar->isChecked())
    {
        toolBar("parameterBar")->hide();
    }
    else
    {
        toolBar("parameterBar")->show();
    }
    slotStatusMsg(tr("Ready."));
}

void NeuroscopeApp::slotShowCalibration(){
    NeuroscopeView* view = activeView();
    doc->showCalibration(calibrationBar->isChecked(),view);
}


void NeuroscopeApp::slotShowPositionView(){
    if(!positionViewToggle->isChecked())
    {
        NeuroscopeView* view = activeView();
        view->removePositionView();
    }
    else
    {
        NeuroscopeView* view = activeView();
        doc->addPositionView(view,backgroundColor);
    }
}

void NeuroscopeApp::slotStatusMsg(const QString &text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message permanently
    statusBar()->clear();
    statusBar()->changeItem(text,1);
}


void NeuroscopeApp::slotZoom(){
    slotStatusMsg(tr("Zooming..."));

    NeuroscopeView* view = activeView();
    view->setMode(BaseFrame::ZOOM,true);

    select = false;

    slotStatusMsg(tr("Ready."));
}

void NeuroscopeApp::slotSelect(){
    slotStatusMsg(tr("Selecting..."));

    select = true;
    NeuroscopeView* view = activeView();
    view->setMode(TraceView::SELECT,true);

    KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->currentPage());

    if((current->getWidget())->isA("ChannelPalette")){
        //the 2 palettes have the same selected channels
        displayChannelPalette->selectionTool();
    }

    slotStatusMsg(tr("Ready."));
}

void NeuroscopeApp::slotMeasure(){
    slotStatusMsg(tr("Measuring..."));

    NeuroscopeView* view = activeView();
    view->setMode(TraceView::MEASURE,true);

    select = false;

    slotStatusMsg(tr("Ready."));
}

void NeuroscopeApp::slotSelectTime(){
    slotStatusMsg(tr("Selecting time..."));

    NeuroscopeView* view = activeView();
    view->setMode(TraceView::SELECT_TIME,true);

    select = false;

    slotStatusMsg(tr("Ready."));
}

void NeuroscopeApp::slotSelectEvent(){
    slotStatusMsg(tr("Selecting event..."));

    NeuroscopeView* view = activeView();
    view->setMode(TraceView::SELECT_EVENT,true);

    select = false;

    slotStatusMsg(tr("Ready."));
}

void NeuroscopeApp::slotDrawTimeLine(){
    slotStatusMsg(tr("Drawing a line..."));
    NeuroscopeView* view = activeView();
    view->setMode(TraceView::DRAW_LINE,true);

    select = false;

    slotStatusMsg(tr("Ready."));
}

NeuroscopeView* NeuroscopeApp::activeView(){
    KDockWidget* current;

    //Get the active tab
    if(tabsParent) current = static_cast<KDockWidget*>(tabsParent->currentPage());
    //or the active window if there is only one display (which can only be the mainDock)
    else current = mainDock;

    return static_cast<NeuroscopeView*>(current->getWidget());
}

void NeuroscopeApp::slotSingleChannelColorUpdate(int channelId){
    colorModified = true;
    NeuroscopeView* view = activeView();
    doc->singleChannelColorUpdate(channelId,view);
}

void NeuroscopeApp::slotChannelGroupColorUpdate(int groupId){
    colorModified = true;
    NeuroscopeView* view = activeView();
    doc->channelGroupColorUpdate(groupId,view);
}

void NeuroscopeApp::slotUpdateShownChannels(const Q3ValueList<int>& shownChannels){  
    KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->currentPage());
    ChannelPalette* channelPalette = static_cast<ChannelPalette*>(current->getWidget());

    NeuroscopeView* view = activeView();
    view->shownChannelsUpdate(shownChannels);

    //if the view is in selection mode, highlight the selected channels
    if(view->isSelectionTool()) view->selectChannels(channelPalette->selectedChannels());
    else view->setSelectedChannels(channelPalette->selectedChannels());

    //Update the show/hide status of the inactive palette.
    if(channelPalette == displayChannelPalette) spikeChannelPalette->updateShowHideStatus(shownChannels,true);
    else displayChannelPalette->updateShowHideStatus(shownChannels,true);
}

void NeuroscopeApp::slotUpdateHiddenChannels(const Q3ValueList<int>& hiddenChannels){
    //Update the show/hide status of the inactive palette
    KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->currentPage());
    ChannelPalette* channelPalette = static_cast<ChannelPalette*>(current->getWidget());

    if(channelPalette == displayChannelPalette) spikeChannelPalette->updateShowHideStatus(hiddenChannels,false);
    else displayChannelPalette->updateShowHideStatus(hiddenChannels,false);
}

void NeuroscopeApp::slotFileProperties(){
    if(propertiesDialog == 0L) propertiesDialog = new PropertiesDialog(this);

    //enable the tabs for cluster and positions if a corresponding file is open, otherwise disable them.
    if(isPositionFileLoaded) propertiesDialog->setEnabledPosition(true);
    else propertiesDialog->setEnabledPosition(false);

    if(clusterFileList.isEmpty() || !doc->isCurrentFileAdatFile()) propertiesDialog->setEnabledCluster(false);
    else propertiesDialog->setEnabledCluster(true);

    int channelNb = doc->getChannelNb();
    double SR = doc->getSamplingRate();
    double acquisitionSystemSamplingRate = doc->getAcquisitionSystemSamplingRate();
    int resolution = doc->getResolution();
    int offset = doc->getInitialOffset();
    int voltageRange = doc->getVoltageRange();
    int amplification = doc->getAmplification();
    float screenGain = doc->getScreenGain();
    int  nbSamples = doc->getNbSamples();
    int peakIndex = doc->getPeakIndex();
    double videoSamplingRate = doc->getVideoSamplingRate();

    int  width = doc->getWidth();
    int height = doc->getHeight();
    QString backgroundImage = doc->getBackgroundImage();
    QString traceBackgroundImage = doc->getTraceBackgroundImage();

    int rotation = doc->getRotation();
    int flip = doc->getFlip();
    bool positionsBackground = doc->getPositionsBackground();

    //If the current opened file is a dat file, the sampling rate in provided in the Acquisition System section
    //and the line edit for the current file sampling rate is not necessary.
    if(doc->isCurrentFileAdatFile()) propertiesDialog->setEnabledCurrentSamplingRate(false);
    else propertiesDialog->setEnabledCurrentSamplingRate(true);

    //update the dialog widget.
    propertiesDialog->updateDialog(channelNb,SR,resolution,offset,screenGain,voltageRange,amplification,
                                   nbSamples,peakIndex,videoSamplingRate,width,height,backgroundImage,rotation,flip,acquisitionSystemSamplingRate,positionsBackground,traceBackgroundImage);

    if(propertiesDialog->exec() == QDialog::Accepted){  // execute the dialog
        if(propertiesDialog->isModified()){
            if(channelNb != propertiesDialog->getNbChannels()) doc->setChannelNb(propertiesDialog->getNbChannels());
            if(SR != propertiesDialog->getSamplingRate()) doc->setSamplingRate(propertiesDialog->getSamplingRate());
            if(acquisitionSystemSamplingRate != propertiesDialog->getAcquisitionSystemSamplingRate()){
                if(doc->isCurrentFileAdatFile()) doc->setSamplingRate(propertiesDialog->getAcquisitionSystemSamplingRate());
                else doc->setAcquisitionSystemSamplingRate(propertiesDialog->getAcquisitionSystemSamplingRate());
            }
            if(resolution != propertiesDialog->getResolution()) doc->setResolution(propertiesDialog->getResolution());
            if(offset != propertiesDialog->getOffset()) doc->setInitialOffset(propertiesDialog->getOffset());
            if(voltageRange != propertiesDialog->getVoltageRange() || amplification != propertiesDialog->getAmplification() ||
                    screenGain!= propertiesDialog->getScreenGain()){
                doc->setGains(propertiesDialog->getVoltageRange(),propertiesDialog->getAmplification(),propertiesDialog->getScreenGain());
            }
            if(nbSamples != propertiesDialog->getNbSamples() || peakIndex != propertiesDialog->getPeakIndex())
                doc->setWaveformInformation(propertiesDialog->getNbSamples(),propertiesDialog->getPeakIndex(),activeView());


            if(traceBackgroundImage != propertiesDialog->getTraceBackgroundImage()){
                doc->setTraceBackgroundImage(propertiesDialog->getTraceBackgroundImage());
            }

            if(videoSamplingRate != propertiesDialog->getVideoSamplingRate() || width != propertiesDialog->getWidth() ||
                    height != propertiesDialog->getHeight() || backgroundImage != propertiesDialog->getBackgroundImage()||
                    rotation != propertiesDialog->getRotation() || flip != propertiesDialog->getFlip() || positionsBackground != propertiesDialog->getPositionsBackground())
                doc->setPositionInformation(propertiesDialog->getVideoSamplingRate(),propertiesDialog->getWidth(),propertiesDialog->getHeight(),
                                            propertiesDialog->getBackgroundImage(),propertiesDialog->getRotation(),propertiesDialog->getFlip(),propertiesDialog->getPositionsBackground(),activeView());
        }
    }
}

void NeuroscopeApp::displayFileProperties(int channelNb,double SR,int resolution,int offset,int voltageRange,int amplification,
                                          float screenGain,int currentNbSamples,int currentPeakIndex,double videoSamplingRate,int width,
                                          int height, QString backgroundImage,int rotation,int flip,double acquisitionSystemSamplingRate,bool isaDatFile,bool positionsBackground,QString traceBackgroundImage){
    QApplication::restoreOverrideCursor();

    if(propertiesDialog == 0L) propertiesDialog = new PropertiesDialog(this);

    //enable the tabs for cluster and positions if a corresponding file is open, otherwise disable them.
    if(isPositionFileLoaded) propertiesDialog->setEnabledPosition(true);
    else propertiesDialog->setEnabledPosition(false);

    if(clusterFileList.isEmpty() || !doc->isCurrentFileAdatFile()) propertiesDialog->setEnabledCluster(false);
    else propertiesDialog->setEnabledCluster(true);

    //If the current opened file is a dat file, the sampling rate in provided in the Acquisition System section
    //and the line edit for the current file sampling rate is not necessary.
    if(isaDatFile) propertiesDialog->setEnabledCurrentSamplingRate(false);
    else propertiesDialog->setEnabledCurrentSamplingRate(true);

    //update the dialog widget.
    propertiesDialog->updateDialog(channelNb,SR,resolution,offset,screenGain,voltageRange,amplification,currentNbSamples,
                                   currentPeakIndex,videoSamplingRate,width,height,backgroundImage,rotation,flip,acquisitionSystemSamplingRate,positionsBackground,traceBackgroundImage);
    propertiesDialog->openState(true);

    if(propertiesDialog->exec() == QDialog::Accepted){  // execute the dialog
        if(propertiesDialog->isModified()){
            doc->updateFileProperties(propertiesDialog->getNbChannels(),propertiesDialog->getSamplingRate(),
                                      propertiesDialog->getResolution(),propertiesDialog->getOffset(),propertiesDialog->getVoltageRange(),
                                      propertiesDialog->getAmplification(),propertiesDialog->getScreenGain(),propertiesDialog->getNbSamples(),
                                      propertiesDialog->getPeakIndex(),propertiesDialog->getVideoSamplingRate(),propertiesDialog->getWidth(),
                                      propertiesDialog->getHeight(),propertiesDialog->getBackgroundImage(),propertiesDialog->getTraceBackgroundImage(),propertiesDialog->getRotation(),
                                      propertiesDialog->getFlip(),propertiesDialog->getAcquisitionSystemSamplingRate(),propertiesDialog->getPositionsBackground());
        }
    }

    propertiesDialog->openState(false);

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
}

void NeuroscopeApp::setFileProperties(QString channelNb,QString SR,QString resolution,QString offset,QString voltageRange,
                                      QString amplification,QString screenGain,QString timeWindow){
    //If the the number of channels has not been provided, the parameter equals 0.
    if(channelNb.toInt() != 0){
        doc->setChannelNb(channelNb.toInt());
        //Inform the document that the information comes from the command line.
        doc->propertiesFromCommandLine();
    }

    //If the sampling rate has not been provided, the parameter equals 0.
    if(SR.toDouble() != 0){
        doc->setSamplingRate(SR.toDouble());
        //Inform the document that the information comes from the command line.
        doc->propertiesFromCommandLine();
    }

    //If the resolution has not been provided, the parameter equals 0.
    if(resolution.toInt() != 0){
        doc->setResolution(resolution.toInt());
        //Inform the document that the information comes from the command line.
        doc->propertiesFromCommandLine();
    }

    //If the offset has not been provided, the parameter equals 0.
    if(offset.toInt() != 0){
        doc->setInitialOffset(offset.toInt());
        //Inform the document that the information comes from the command line.
        doc->propertiesFromCommandLine();
    }

    //If the voltageRange has not been provided, the parameter equals 0.
    if(voltageRange.toInt() != 0){
        doc->setVoltageRange(voltageRange.toInt());
        //Inform the document that the information comes from the command line.
        doc->propertiesFromCommandLine();
    }

    //If the voltageRange has not been provided, the parameter equals 0.
    if(amplification.toInt() != 0){
        doc->setAmplification(amplification.toInt());
        //Inform the document that the information comes from the command line.
        doc->propertiesFromCommandLine();
    }

    //If the voltageRange has not been provided, the parameter equals 0.
    if(screenGain.toFloat() != 0){
        doc->setScreenGain(screenGain.toFloat());
        //Inform the document that the information comes from the command line.
        doc->propertiesFromCommandLine();
    }

    //If the time window has not been provided, the parameter equals 0.
    if(timeWindow.toInt() != 0){
        initialTimeWindow = timeWindow.toInt();
    }
}

void NeuroscopeApp::resetState(){
    isInit = true; //prevent the spine boxes or the lineedit and the editline to trigger during initialisation

    clusterWaveforms->setChecked(false);
    clusterRaster->setChecked(true);
    slotStateChanged("clusterRasterState");
    clusterVerticalLines->setChecked(false);
    greyScale->setChecked(false);
    displayMode->setChecked(false);
    editMode->setChecked(true);
    showHideLabels->setChecked(false);
    positionViewToggle->setChecked(false);
    showEventsInPositionView->setChecked(false);
    isPositionFileLoaded = false;

    displayChannelPalette->reset();
    spikeChannelPalette->reset();
    select = false;

    isInit = false; //now a change in a spine box  or the lineedit
    //will trigger an update of the view contain in the display.
    displayCount = 0;
    groupsModified = false;
    colorModified = false;
    eventsModified = false;
    filePath = "";
    clusterFileList.clear();
    eventFileList.clear();
    eventLabelToCreate = "";
    eventProvider = "";
    currentNbUndo = 0;
    currentNbRedo = 0;


    //Disable some actions when no document is open (see the klustersui.rc file)
    slotStateChanged("initState");
    setCaption("");
}

void NeuroscopeApp::slotDefaultSetUp(QMap<int,int>& channelDefaultOffsets,QMap<int,bool>& skipStatus){
    //All the channels will be selected and none skipped
    Q3ValueList<int>* channelsToDisplay = new Q3ValueList<int>();
    int channelNb = doc->getChannelNb();
    for(int i = 0 ; i < channelNb; ++i){
        channelsToDisplay->append(i);
    }

    Q3ValueList<int> offsets = channelDefaultOffsets.values();
    Q3ValueList<int> channelGains;
    Q3ValueList<int> selectedChannels;
    if(initialTimeWindow != 0) initDisplay(channelsToDisplay,offsets,channelGains,selectedChannels,skipStatus,initialTimeWindow);
    else initDisplay(channelsToDisplay,offsets,channelGains,selectedChannels,skipStatus);

}

void NeuroscopeApp::slotSetUp(Q3ValueList<int>* channelsToDisplay,bool verticalLines,bool raster,bool waveforms,bool showLabels,bool multipleColumns,
                              bool greyMode,Q3ValueList<int> offsets,Q3ValueList<int> channelGains,Q3ValueList<int> selectedChannels,
                              QMap<int,bool>& skipStatus,long startTime,long duration,QString tabLabel,bool positionView,int rasterHeight,bool showEventsInPositionView){

    isInit = true; //prevent the KToggleAction to trigger during initialisation

    clusterVerticalLines->setChecked(verticalLines);
    clusterRaster->setChecked(raster);
    clusterWaveforms->setChecked(waveforms);
    greyScale->setChecked(greyMode);
    displayMode->setChecked(multipleColumns);
    showHideLabels->setChecked(showLabels);
    positionViewToggle->setChecked(positionView);
    this->showEventsInPositionView->setChecked(showEventsInPositionView);

    isInit = false; //now a change in a KToggleAction will trigger an update of the display

    initDisplay(channelsToDisplay,offsets,channelGains,selectedChannels,skipStatus,rasterHeight,duration,startTime,tabLabel);
}


void NeuroscopeApp::slotSetGreyScale(){
    NeuroscopeView* view = activeView();
    view->setGreyScale(greyScale->isChecked());
    displayChannelPalette->setGreyScale(greyScale->isChecked());
    spikeChannelPalette->setGreyScale(greyScale->isChecked());
}


void NeuroscopeApp::slotCreateGroup(){
    //Get the active palette
    KDockWidget* current = dynamic_cast<KDockWidget*>(paletteTabsParent->currentPage());
    ChannelPalette* channelPalette = dynamic_cast<ChannelPalette*>(current->getWidget());
    channelPalette->createGroup();
}

void NeuroscopeApp::slotResetOffsets(){
    NeuroscopeView* view = activeView();
    //The 2 palettes have the same selected channels
    view->resetOffsets(displayChannelPalette->selectedChannels());
}

void NeuroscopeApp::slotResetGains(){
    NeuroscopeView* view = activeView();
    //The 2 palettes have the same selected channels
    view->resetGains(displayChannelPalette->selectedChannels());
}

void NeuroscopeApp::slotSelectAll(){
    KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->currentPage());

    if((current->getWidget())->isA("ChannelPalette")){
        //update the 2 palettes
        spikeChannelPalette->selectAllChannels();
        displayChannelPalette->selectAllChannels();
        NeuroscopeView* view = activeView();
        if(editMode->isChecked()) doc->selectAllChannels(*view,true);
        else doc->selectAllChannels(*view,false);
    }
    else{
        //Update the selected items of the current palette
        if((current->getWidget())->isA("ItemPalette")){
            QString name = current->name();
            //update the cluster palette
            if(name.contains("clusterPanel")){
                ItemPalette* clusterPalette = static_cast<ItemPalette*>(current->getWidget());
                NeuroscopeView* view = activeView();
                Q3ValueList<int> clustersToHide;
                doc->showAllClustersExcept(clusterPalette,view,clustersToHide);
            }
            //update the event palette
            if(name.contains("eventPanel")){
                ItemPalette* eventPalette = static_cast<ItemPalette*>(current->getWidget());
                NeuroscopeView* view = activeView();
                doc->showAllEvents(eventPalette,view);
            }
            //update the spike palette

        }
    }
}

void NeuroscopeApp::slotDeselectAll(){
    KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->currentPage());

    if((current->getWidget())->isA("ChannelPalette")){
        //update the 2 palettes
        spikeChannelPalette->deselectAllChannels();
        displayChannelPalette->deselectAllChannels();
        NeuroscopeView* view = activeView();
        if(editMode->isChecked()) doc->deselectAllChannels(*view,true);
        else doc->deselectAllChannels(*view,false);
    }
    else{
        //Update the selected items of the current palette
        if((current->getWidget())->isA("ItemPalette")){
            QString name = current->name();
            //update the cluster palette
            if(name.contains("clusterPanel")){
                ItemPalette* clusterPalette = static_cast<ItemPalette*>(current->getWidget());
                NeuroscopeView* view = activeView();
                doc->deselectAllClusters(clusterPalette,view);
                slotStateChanged("noClusterBrowsingState");
            }
            //update the event palette
            if(name.contains("eventPanel")){
                ItemPalette* eventPalette = static_cast<ItemPalette*>(current->getWidget());
                NeuroscopeView* view = activeView();
                doc->deselectAllEvents(eventPalette,view);
                slotStateChanged("noEventBrowsingState");
            }
            //update the spike palette

        }
    }
}

void NeuroscopeApp::slotSelectAllWO01(){
    KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->currentPage());

    //Update the selected items of the current palette
    if((current->getWidget())->isA("ItemPalette")){
        QString name = current->name();
        //update the cluster palette
        if(name.contains("clusterPanel")){
            ItemPalette* clusterPalette = static_cast<ItemPalette*>(current->getWidget());
            NeuroscopeView* view = activeView();
            Q3ValueList<int> clustersToHide;
            clustersToHide.append(0);
            clustersToHide.append(1);
            doc->showAllClustersExcept(clusterPalette,view,clustersToHide);
        }
    }
}

void NeuroscopeApp::slotDisplayMode(){
    NeuroscopeView* view = activeView();
    view->setMultiColumns(displayMode->isChecked());
}

void NeuroscopeApp::slotShowLabels(){
    NeuroscopeView* view = activeView();
    view->showLabelsUpdate(showHideLabels->isChecked());
}

void NeuroscopeApp::slotClustersVerticalLines(){
    NeuroscopeView* view = activeView();
    view->setClusterVerticalLines(clusterVerticalLines->isChecked());
}

void NeuroscopeApp::slotClustersRaster(){
    if(clusterRaster->isChecked()) slotStateChanged("clusterRasterState");
    else slotStateChanged("noClusterRasterState");
    NeuroscopeView* view = activeView();
    view->setClusterRaster(clusterRaster->isChecked());
}
void NeuroscopeApp::slotClustersWaveforms(){
    NeuroscopeView* view = activeView();
    view->setClusterWaveforms(clusterWaveforms->isChecked());
}

void NeuroscopeApp::slotDiscardChannels(){
    //Get the active palette.
    KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->currentPage());
    ChannelPalette* channelPalette = static_cast<ChannelPalette*>(current->getWidget());
    channelPalette->discardChannels();
}

void NeuroscopeApp::slotDiscardSpikeChannels(){
    spikeChannelPalette->discardSpikeChannels();
    groupsModified = true;
}

void NeuroscopeApp::slotKeepChannels(){
    colorModified = true;
    const Q3ValueList<int> selectedChannels = displayChannelPalette->selectedChannels();

    //The order in which the palettes are updated matters. The first one will give the new color for the
    //channels which change status
    KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->currentPage());
    if((current->getWidget()) == displayChannelPalette){
        displayChannelPalette->updateSkipStatus(selectedChannels,false);
        spikeChannelPalette->updateSkipStatus(selectedChannels,false);
    }
    else{
        spikeChannelPalette->updateSkipStatus(selectedChannels,false);
        displayChannelPalette->updateSkipStatus(selectedChannels,false);
    }

    //Update the skipped channel list in the views
    doc->updateSkipStatus();

    //if the view is in selection mode, highlight the selected channels
    NeuroscopeView* view = activeView();
    if(view->isSelectionTool()) view->selectChannels(selectedChannels);
    else view->setSelectedChannels(selectedChannels);


    //Update the content of the view contains in active display.
    activeView()->updateViewContents();
}

void NeuroscopeApp::slotSkipChannels(){
    colorModified = true;
    const Q3ValueList<int> selectedChannels = displayChannelPalette->selectedChannels();
    displayChannelPalette->updateSkipStatus(selectedChannels,true);
    spikeChannelPalette->updateSkipStatus(selectedChannels,true);

    //Update the skipped channel list in the views
    doc->updateSkipStatus();

    //Update the content of the view contains in active display.
    activeView()->updateViewContents();
}


void NeuroscopeApp::slotChannelsDiscarded(const Q3ValueList<int>& discarded){
    groupsModified = true;

    //Update the inactive palette
    KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->currentPage());
    ChannelPalette* channelPalette = static_cast<ChannelPalette*>(current->getWidget());

    if(channelPalette == displayChannelPalette) spikeChannelPalette->discardChannels(discarded);
    else displayChannelPalette->discardChannels(discarded);

    NeuroscopeView* view = activeView();
    doc->groupsModified(view);
}

void NeuroscopeApp::slotShowChannels(){
    //Get the active palette if there are 2 or the displayChannelPalette otherwise.
    KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->currentPage());
    ChannelPalette* channelPalette = static_cast<ChannelPalette*>(current->getWidget());
    channelPalette->showChannels();
}

void NeuroscopeApp::slotHideChannels(){
    //Get the active palette if there are 2 or the displayChannelPalette otherwise.
    KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->currentPage());
    ChannelPalette* channelPalette = static_cast<ChannelPalette*>(current->getWidget());
    channelPalette->hideChannels();
}

void NeuroscopeApp::slotTabChange(QWidget* widget){
    KDockWidget* display = dynamic_cast<KDockWidget*>(widget);
    NeuroscopeView* activeView = dynamic_cast<NeuroscopeView*>(display->getWidget());

    isInit = true; //prevent the KToggleAction to trigger during initialisation

    clusterVerticalLines->setChecked(activeView->getClusterVerticalLines());
    clusterRaster->setChecked(activeView->getClusterRaster());
    if(clusterRaster->isChecked()) slotStateChanged("clusterRasterState");
    else slotStateChanged("noClusterRasterState");
    clusterWaveforms->setChecked(activeView->getClusterWaveforms());
    showHideLabels->setChecked(activeView->getLabelStatus());
    positionViewToggle->setChecked(activeView->isPositionView());
    showEventsInPositionView->setChecked(activeView->isEventsInPositionView());

    bool greyMode = activeView->getGreyScale();
    greyScale->setChecked(greyMode);
    displayChannelPalette->setGreyScale(greyMode);
    spikeChannelPalette->setGreyScale(greyMode);

    displayMode->setChecked(activeView->getMultiColumns());
    select = activeView->isSelectionTool();
    const Q3ValueList<int> selectedChannels = activeView->getSelectedChannels();

    KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->currentPage());

    if((current->getWidget())->isA("ChannelPalette")){
        //update the channel palettes
        displayChannelPalette->hideUnselectAllChannels();
        spikeChannelPalette->hideUnselectAllChannels();
        displayChannelPalette->updateShowHideStatus(activeView->channels(),true);
        spikeChannelPalette->updateShowHideStatus(activeView->channels(),true);
        displayChannelPalette->selectChannels(selectedChannels);
        spikeChannelPalette->selectChannels(selectedChannels);
    }

    //update the spike and event browsing status
    updateBrowsingStatus();

    //Update the content of the view contains in active display.
    activeView->updateViewContents();

    isInit = false; //now a change in a KToggleAction will trigger an update of the display

}


void NeuroscopeApp::slotPaletteTabChange(QWidget* widget){
    //Update the show/hide status of the inactive palette
    KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->currentPage());

    //Disable some actions when no document is open (see the klustersui.rc file)
    if((current->getWidget())->isA("ChannelPalette")){
        if(editMode->isChecked()){
            if((current->getWidget()) == displayChannelPalette) slotStateChanged("displayChannelState");
            else slotStateChanged("spikeChannelState");
        }
        else slotStateChanged("enableEditState");

        //update the channel palettes
        NeuroscopeView* view = activeView();
        const Q3ValueList<int> selectedChannels = view->getSelectedChannels();
        displayChannelPalette->hideUnselectAllChannels();
        spikeChannelPalette->hideUnselectAllChannels();
        displayChannelPalette->updateShowHideStatus(view->channels(),true);
        spikeChannelPalette->updateShowHideStatus(view->channels(),true);
        displayChannelPalette->selectChannels(selectedChannels);
        spikeChannelPalette->selectChannels(selectedChannels);
    }
    else{
        slotStateChanged("noChannelState");
        //Update the selected items of the current palette
        if((current->getWidget())->isA("ItemPalette")){
            QString name = current->name();
            if(name.contains("clusterPanel")){
                ItemPalette* clusterPalette = static_cast<ItemPalette*>(current->getWidget());
                NeuroscopeView* view = activeView();
                Q3ValueList<QString>::iterator iterator;
                for(iterator = clusterFileList.begin(); iterator != clusterFileList.end(); ++iterator){
                    const Q3ValueList<int>* selectedClusters = view->getSelectedClusters(*iterator);
                    const Q3ValueList<int>* skippedClusterIds = view->getClustersNotUsedForBrowsing(*iterator);
                    clusterPalette->selectItems(*iterator,*selectedClusters,*skippedClusterIds);
                }
                slotStateChanged("clusterTabState");

                if(!clusterPalette->isBrowsingEnable()) slotStateChanged("noClusterBrowsingState");
                else slotStateChanged("clusterBrowsingState");
            }
            //update the event palettes
            if(name.contains("eventPanel")){
                ItemPalette* eventPalette = static_cast<ItemPalette*>(current->getWidget());
                NeuroscopeView* view = activeView();
                Q3ValueList<QString>::iterator iterator;
                for(iterator = eventFileList.begin(); iterator != eventFileList.end(); ++iterator){
                    const Q3ValueList<int>* selectedEvents = view->getSelectedEvents(*iterator);
                    const Q3ValueList<int>* skippedEventIds = view->getEventsNotUsedForBrowsing(*iterator);
                    eventPalette->selectItems(*iterator,*selectedEvents,*skippedEventIds);
                }
                slotStateChanged("eventTabState");
                if(!eventPalette->isBrowsingEnable()) slotStateChanged("noEventBrowsingState");
                else slotStateChanged("eventBrowsingState");
            }

            //update the spike palettes

        }
    }
}

void NeuroscopeApp::slotApplyDisplayColor(){
    spikeChannelPalette->applyGroupColor(ChannelPalette::DISPLAY);
    displayChannelPalette->applyGroupColor(ChannelPalette::DISPLAY);

    //Update the content of the view contains in active display.
    activeView()->updateViewContents();
}

void NeuroscopeApp::slotApplySpikeColor(){
    spikeChannelPalette->applyGroupColor(ChannelPalette::SPIKE);
    displayChannelPalette->applyGroupColor(ChannelPalette::SPIKE);

    //Update the content of the view contains in active display.
    activeView()->updateViewContents();
}


void NeuroscopeApp::slotDisplayClose(){   
    KDockWidget* current;

    slotStatusMsg(tr("Closing display..."));

    //Get the active tab
    if(tabsParent){
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        int nbOfTabs = tabsParent->count();
        current = static_cast<KDockWidget*>(tabsParent->currentPage());
        //If the active display is the mainDock, assign the mainDock status to an other display (take the first one available)
        if(current == mainDock){
            if(tabsParent->currentPageIndex() == 0){
                mainDock = static_cast<KDockWidget*>(tabsParent->page(1));
                setMainDockWidget(mainDock);
            }
            else setMainDockWidget(static_cast<KDockWidget*>(tabsParent->page(0)));
        }
        //Remove the display from the group of tabs
        tabsParent->removePage(current);
        displayCount --;
        //If there is only one display left, the group of tabs will be deleted so we set tabsParent to null
        if(nbOfTabs == 2){
            slotStateChanged("noTabState");
            tabsParent = 0L;
        }
        //Remove the view from the document list
        NeuroscopeView* view = dynamic_cast<NeuroscopeView*>(current->getWidget());
        doc->removeView(view);

        //Delete the view
        delete current;

        QApplication::restoreOverrideCursor();
    }
    //or the active window if there is only one display (which can only be the mainDock)
    else{
        QApplication::restoreOverrideCursor();
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        //try to close the document
        if(doc->canCloseDocument(this,"displayClose")){
            //If either groups, colors or events have been modified, ask if the user wants to save them
            int saveStatus;
            int eventSaveStatus = 0;
            if(groupsModified || colorModified || eventsModified){
                QApplication::restoreOverrideCursor();
                QString message;
                QString title;
                if((groupsModified || colorModified) && !eventsModified){
                    message = "Your configuration has changed, do you want to save it?";
                    title = "Configuration modification";
                }
                else if(eventsModified && !groupsModified && !colorModified){
                    message = "Some events have changed, do you want to save the event file(s)?";
                    title = "Event modification";
                }
                else if((groupsModified || colorModified) && eventsModified){
                    message = "Your configuration and some events have changed, do you want to save the configuration and the event file(s)?";
                    title = "Modification";
                }
                switch(KMessageBox::warningYesNoCancel(0,tr(message),tr(title),KGuiItem("Save"),KGuiItem("Discard"))){
                case KMessageBox::Yes://<=> Save
                    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                    if(eventsModified){
                        eventSaveStatus = doc->saveEventFiles();
                        QApplication::restoreOverrideCursor();
                        if(eventSaveStatus != IO_Ok){
                            switch(KMessageBox::warningYesNo(0,tr("The event file(s) could not be saved possibly because of insufficient file access permissions."	"Close anyway ?"), tr("I/O Error !")
                                                             ,KGuiItem("Close"),KGuiItem("Discard"))){
                            case KMessageBox::Yes:
                                break;
                            case KMessageBox::No:
                                return;
                            }
                        }
                    }//eventsModified
                    //Save session
                    saveStatus = doc->saveSession();
                    QApplication::restoreOverrideCursor();
                    if(saveStatus != NeuroscopeDoc::OK){
                        if(saveStatus == NeuroscopeDoc::CREATION_ERROR){
                            message = "The current session could not be saved possibly because of insufficient file access permissions."
                                    " You may consider saving your session file to another location using the Save As entry in the File menu.\n"
                                    "Close anyway ?";
                            title = "I/O Error !";
                        }
                        else if(saveStatus == NeuroscopeDoc::PARSE_ERROR){
                            message = "The current session could not be saved because the parameter file is incorrect.\nClose anyway ?";
                            title = "Parsing error !";
                        }
                        else if(saveStatus == NeuroscopeDoc::NOT_WRITABLE){
                            message = "The current session could not be saved because the parameter file is not writable.\nClose anyway ?";
                            title = "Writing error !";
                        }
                        switch(KMessageBox::warningYesNo(0,tr(message), tr(title),KGuiItem("Close"),KGuiItem("Discard"))){
                        case KMessageBox::Yes:
                            break;
                        case KMessageBox::No:
                            return;
                        }
                    }
                    break;
                case KMessageBox::No://<=> Discard
                    break;
                case KMessageBox::Cancel:
                    return;
                }
            }
            else{
                //If neither groups, colors or events have been modified, save the session without asking
                saveStatus = doc->saveSession();
                QString message;
                QString title;
                QApplication::restoreOverrideCursor();
                if(saveStatus != NeuroscopeDoc::OK){
                    if(saveStatus == NeuroscopeDoc::CREATION_ERROR){
                        message = "The current session could not be saved possibly because of insufficient file access permissions."
                                " You may consider saving your session file to another location using the Save As entry in the File menu.\n"
                                "Close anyway ?";
                        title = "I/O Error !";
                    }
                    else if(saveStatus == NeuroscopeDoc::PARSE_ERROR){
                        message = "The current session could not be saved because the parameter file is incorrect.\nClose anyway ?";
                        title = "Parsing error !";
                    }
                    else if(saveStatus == NeuroscopeDoc::NOT_WRITABLE){
                        message = "The current session could not be saved because the parameter file is not writable.\nClose anyway ?";
                        title = "Writing error !";
                    }
                    switch(KMessageBox::warningYesNo(0,tr(message), tr(title),KGuiItem("Close"),KGuiItem("Discard"))){
                    case KMessageBox::Yes:
                        break;
                    case KMessageBox::No:
                        return;
                    }
                }
            }
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

            //remove the cluster and event palettes if any
            while(paletteTabsParent->count() > 2){
                KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->page(0));
                if((current->getWidget())->isA("ChannelPalette")){
                    current = static_cast<KDockWidget*>(paletteTabsParent->page(1));
                    if((current->getWidget())->isA("ChannelPalette"))
                        current = static_cast<KDockWidget*>(paletteTabsParent->page(2));
                }
                paletteTabsParent->removePage(current);
                delete current;
            }

            //reset the channel palettes and hide the channel panels
            spikeChannelPalette->reset();
            displayChannelPalette->reset();
            palettePanel->hide();
            palettePanel->undock();

            doc->closeDocument();
            //Delete the view
            delete mainDock;
            mainDock = 0L;
            resetState();
            QApplication::restoreOverrideCursor();
        }
    }
    slotStatusMsg(tr("Ready."));
}

void NeuroscopeApp::slotRenameActiveDisplay(){
    if(tabsParent){
        KDockWidget* current;

        //Get the active tab
        current = static_cast<KDockWidget*>(tabsParent->currentPage());

        bool ok;
        QString newLabel = QInputDialog::getText(tr("New Display label"),tr("Type in the new display label"),QLineEdit::Normal,
                                                 current->tabPageLabel(),&ok,this);
        if(ok&& !newLabel.isEmpty()){
            tabsParent->setTabLabel(current,newLabel);
            current->setTabPageLabel(newLabel);
            activeView()->setTabName(newLabel);
        }
    }
}

void NeuroscopeApp::slotNewDisplay(){
    //Present the channels of the current display in the new display.
    Q3ValueList<int>* channelList = new Q3ValueList<int>();
    const Q3ValueList<int>& currentChannels = activeView()->channels();
    Q3ValueList<int>::const_iterator iterator;
    for(iterator = currentChannels.begin(); iterator != currentChannels.end(); ++iterator)
        channelList->append(*iterator);

    Q3ValueList<int> offsets = activeView()->getChannelOffset();
    Q3ValueList<int> channelGains = activeView()->getGains();
    Q3ValueList<int> selectedChannels = activeView()->getSelectedChannels();
    long startTime = activeView()->getStartTime();
    long duration = activeView()->getTimeWindow();
    int rasterHeight = activeView()->getRasterHeight();

    createDisplay(channelList,clusterVerticalLines->isChecked(),clusterRaster->isChecked(),clusterWaveforms->isChecked(),
                  showHideLabels->isChecked(),displayMode->isChecked(),greyScale->isChecked(),offsets,channelGains,selectedChannels,startTime,
                  duration,rasterHeight);

    //informs the new view of the list of providers
    NeuroscopeView* view = activeView();
    doc->setProviders(view);
}

void NeuroscopeApp::createDisplay(Q3ValueList<int>* channelsToDisplay,bool verticalLines,bool raster,bool waveforms,bool showLabels,bool multipleColumns,bool greyMode,
                                  Q3ValueList<int> offsets,Q3ValueList<int> channelGains,Q3ValueList<int> selectedChannels,long startTime,long duration,int rasterHeight, QString tabLabel){
    if(mainDock){
        if(tabLabel == "") tabLabel = "Field Potentials Display";
        KDockWidget* display = createDockWidget( "1", QPixmap(), 0L, tr(doc->url().path()),tabLabel);

        NeuroscopeView* view = new NeuroscopeView(*this,tabLabel,startTime,duration,backgroundColor,WDestructiveClose,statusBar(),channelsToDisplay,
                                                  greyMode,doc->tracesDataProvider(),multipleColumns,verticalLines,raster,waveforms,showLabels,
                                                  doc->getGain(),doc->getAcquisitionGain(),doc->channelColors(),doc->getDisplayGroupsChannels(),doc->getDisplayChannelsGroups(),
                                                  offsets,channelGains,selectedChannels,displayChannelPalette->getSkipStatus(),rasterHeight,doc->getTraceBackgroundImage(),mainDock,"TracesDisplay");

        view->installEventFilter(this);

        connect(view,SIGNAL(channelsSelected(const Q3ValueList<int>&)),this, SLOT(slotSelectChannelsInPalette(const Q3ValueList<int>&)));
        connect(view,SIGNAL(eventModified(QString,int,double,double)),this, SLOT(slotEventModified(QString,int,double,double)));
        connect(view,SIGNAL(eventRemoved(QString,int,double)),this, SLOT(slotEventRemoved(QString,int,double)));
        connect(view,SIGNAL(eventAdded(QString,QString,double)),this, SLOT(slotEventAdded(QString,QString,double)));
        connect(view,SIGNAL(positionViewClosed()),this, SLOT(positionViewClosed()));

        view->installEventFilter(this);

        //Update the document's list of view
        doc->addView(view);
        //install the new view in the display so it can be see in the future tab.
        display->setWidget(view);

        //Temporarily allow addition of a new dockWidget in the center
        mainDock->setDockSite(KDockWidget::DockCenter);
        //Add the new display as a tab and get a new DockWidget, grandParent of the target (mainDock)
        //and the new display.
        KDockWidget* grandParent = display->manualDock(mainDock,KDockWidget::DockCenter);

        //Disconnect the previous connection
        if(tabsParent != NULL) disconnect(tabsParent,0,0,0);

        //The grandParent's widget is the KDockTabGroup regrouping all the tabs
        tabsParent = static_cast<KDockTabGroup*>(grandParent->getWidget());

        //Connect the change tab signal to slotTabChange(QWidget* widget) to trigger updates when
        //the active display change.
        connect(tabsParent, SIGNAL(currentChanged(QWidget*)), this, SLOT(slotTabChange(QWidget*)));

        slotStateChanged("tabState");

        //Back to enable dock to the left side only
        mainDock->setDockSite(KDockWidget::DockLeft);

        // forbit docking abilities of display itself
        display->setEnableDocking(KDockWidget::DockNone);
        // allow others to dock to the left side only
        display->setDockSite(KDockWidget::DockLeft);

        //Keep track of the number of displays
        displayCount ++;

        //show all the encapsulated widgets of all controlled dockwidgets
        dockManager->activate();

        //Show the calibration bars if need it
        view->showCalibration(calibrationBar->isChecked(),false);
    }
}

void NeuroscopeApp::slotEditMode(){
    spikeChannelPalette->setEditMode(editMode->isChecked());
    displayChannelPalette->setEditMode(editMode->isChecked());

    KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->currentPage());

    if(editMode->isChecked()){
        slotStateChanged("editState");
        if((current->getWidget()) == displayChannelPalette) slotStateChanged("displayChannelState");
        else slotStateChanged("spikeChannelState");
    }
    else{
        slotStateChanged("noEditState");
        NeuroscopeView* view = activeView();
        doc->setNoneEditMode(view);
        select = false;

        if((current->getWidget())->isA("ChannelPalette")){
            //the 2 palettes have the same selected channels
            displayChannelPalette->selectionTool();
        }
    }
}

void NeuroscopeApp::slotSynchronize(){
    if(KMessageBox::warningContinueCancel(this,tr("This will overwrite all your spike groups by your anatomical groups. Do you wish to continue?"),
                                          tr("Synchronize anatomic and spike groups")) == KMessageBox::Cancel)
        return;

    groupsModified = true;
    doc->synchronize();
    spikeChannelPalette->reset();

    //Update and show the spike Palette.
    spikeChannelPalette->createChannelLists(doc->channelColors(),doc->getSpikeGroupsChannels(),doc->getChannelsSpikeGroups());
    spikeChannelPalette->updateShowHideStatus(displayChannelPalette->getShowHideChannels(true),true);
    spikeChannelPalette->update();

    //update the display of spikes
    slotGroupsModified();

    //show all the encapsulated widgets of all controlled dockwidgets
    dockManager->activate();

    slotStateChanged("displayChannelState");

}

void NeuroscopeApp::resizePalettePanel(){
    static_cast<QWidget*>(displayPanel)->resize(static_cast<QWidget*>(displayPanel->parent())->size());
    displayChannelPalette->update();
}

void NeuroscopeApp::slotSelectChannelsInPalette(const Q3ValueList<int>& selectedIds){
    spikeChannelPalette->selectChannels(selectedIds);
    displayChannelPalette->selectChannels(selectedIds);
}


void NeuroscopeApp::slotChannelsSelected(const Q3ValueList<int>& selectedIds){
    //if the selection tool is selected  warn the active display
    if(select) activeView()->selectChannels(selectedIds);
    else activeView()->setSelectedChannels(selectedIds);

    KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->currentPage());
    ChannelPalette* channelPalette = static_cast<ChannelPalette*>(current->getWidget());

    //Update the selection of the inactive palette.
    if(channelPalette == displayChannelPalette) spikeChannelPalette->selectChannels(selectedIds);
    else displayChannelPalette->selectChannels(selectedIds);
}

void NeuroscopeApp::slotIncreaseSelectedChannelsAmplitude(){
    //Get the active palette if any.
    KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->currentPage());
    if(current->getWidget()->isA("ChannelPalette")){
        ChannelPalette* channelPalette = static_cast<ChannelPalette*>(current->getWidget());
        activeView()->increaseSelectedChannelsAmplitude(channelPalette->selectedChannels());
    }
    else
        //The 2 palettes have the same selection (selected from the view)
        activeView()->increaseSelectedChannelsAmplitude(displayChannelPalette->selectedChannels());
}


void NeuroscopeApp::slotDecreaseSelectedChannelsAmplitude(){
    //Get the active palette if any.
    KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->currentPage());
    if(current->getWidget()->isA("ChannelPalette")){
        ChannelPalette* channelPalette = static_cast<ChannelPalette*>(current->getWidget());
        activeView()->decreaseSelectedChannelsAmplitude(channelPalette->selectedChannels());
    }
    else
        //The 2 palettes have the same selection (selected from the view)
        activeView()->decreaseSelectedChannelsAmplitude(displayChannelPalette->selectedChannels());
}

void NeuroscopeApp::saveSession(){  
    //Save the event files if need it
    if(eventsModified){
        int eventSaveStatus = doc->saveEventFiles();
        if(eventSaveStatus != IO_Ok)
            KMessageBox::error(0,tr("The event file(s) could not be saved possibly because of insufficient file access permissions."),
                               tr("I/O Error !"));
        else eventsModified = false;
    }
    //Save the session
    int saveStatus = doc->saveSession();
    if(saveStatus == NeuroscopeDoc::CREATION_ERROR){
        KMessageBox::error(0,tr("The current session could not be saved possibly because of insufficient file access permissions."
                                " You may consider saving your session file to another location using the Save As entry in the File menu."), tr("I/O Error !"));
    }
    else if(saveStatus == NeuroscopeDoc::PARSE_ERROR){
        KMessageBox::error(0,tr("The current session could not be saved because the parameter file is incorrect."), tr("Parsing error !"));
    }
    else if(saveStatus == NeuroscopeDoc::NOT_WRITABLE){
        KMessageBox::error(0,tr("The current session could not be saved because the parameter file is not writable.\nClose anyway ?"), tr("Writing error !"));
    }
    else{
        groupsModified = false;
        colorModified = false;
    }
}

void NeuroscopeApp::slotSessionSaveAs(){
    //Save the event files if need it
    if(eventsModified){
        int eventSaveStatus = doc->saveEventFiles();
        if(eventSaveStatus != IO_Ok)
            KMessageBox::error(0,tr("The event file(s) could not be saved possibly because of insufficient file access permissions."),
                               tr("I/O Error !"));
        else eventsModified = false;
    }
    //Save the session
    QString url=QFileDialog::getSaveFileName(this, tr("Save as..."),doc->sessionPath(),
                                             tr("*|All files") );
    if(!url.isEmpty()){
        int saveStatus = doc->saveSession(url);
        if(saveStatus == NeuroscopeDoc::CREATION_ERROR){
            KMessageBox::error(0,tr("The current session could not be saved possibly because of insufficient file access permissions."), tr("I/O Error !"));
        }
        else if(saveStatus == NeuroscopeDoc::PARSE_ERROR){
            KMessageBox::error(0,tr("The current session could not be saved because the parameter file is incorrect."), tr("Parsing error !"));
        }
        else if(saveStatus == NeuroscopeDoc::NOT_WRITABLE){
            KMessageBox::error(0,tr("The current session could not be saved because the parameter file is not writable.\nClose anyway ?"), tr("Writing error !"));
        }
        else{
            groupsModified = false;
            colorModified = false;
        }
    }
}

void NeuroscopeApp::customEvent (QCustomEvent* event){
    //Event sent by NeuroscopeDoc to advice that there is some threads still running.
    if(event->type() == QEvent::User + 200){
        NeuroscopeDoc::CloseDocumentEvent* closeEvent = (NeuroscopeDoc::CloseDocumentEvent*) event;
        QString origin = closeEvent->methodOfOrigin();

        //Try to close the document again
        if(doc->canCloseDocument(this,origin)){
            doc->closeDocument();

            //Execute what is need it after the close depending on the callingMethod
            if(origin == "queryClose"){
                QApplication::restoreOverrideCursor();
                close();
            }
            else if(origin == "fileClose" || origin == "displayClose"){
                slotFileClose();
                QApplication::restoreOverrideCursor();
                slotStatusMsg(tr("Ready."));
            }
        }
    }
}

void NeuroscopeApp::loadClusterFiles(QStringList urls){

    NeuroscopeView* view = activeView();
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    //Loop on the files
    int counter = 0;
    Q3ValueList<QString>::iterator iterator;
    for(iterator = urls.begin();iterator != urls.end();++iterator){
        //Create the provider
        int returnStatus = doc->loadClusterFile(*iterator,view);
        if(returnStatus == NeuroscopeDoc::OPEN_ERROR){
            QApplication::restoreOverrideCursor();
            KMessageBox::error (this,tr("Could not load the file " + static_cast<QString>(*iterator).path()), tr("Error!"));
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            continue;
        }
        else if(returnStatus == NeuroscopeDoc::INCORRECT_FILE){
            QApplication::restoreOverrideCursor();
            KMessageBox::error (this,tr("Incorrect file name (" + static_cast<QString>(*iterator).path() + "): the name has to be of the form baseName.n.clu or baseName.clu.n (with n a number identifier)."), tr("Error!"));
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            continue;
        }
        else if(returnStatus == NeuroscopeDoc::MISSING_FILE){
            QApplication::restoreOverrideCursor();
            KMessageBox::error (this,tr("There is no time file (.res) corresponding to the requested file " + static_cast<QString>(*iterator).path()), tr("Error!"));
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            continue;
        }
        else if(returnStatus == NeuroscopeDoc::INCORRECT_CONTENT){
            QApplication::restoreOverrideCursor();
            KMessageBox::error (this,tr("The number of spikes read in the requested file (" + static_cast<QString>(*iterator).path() + ") or the corresponding time file (.res) does not correspond to number of spikes computed."), tr("Error!"));
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            continue;
        }
        else if(returnStatus == NeuroscopeDoc::CREATION_ERROR){
            QApplication::restoreOverrideCursor();
            KMessageBox::error (this,tr("The number of spikes of the requested file (" + static_cast<QString>(*iterator).path() + ") could not be determined."), tr("Error!"));
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            continue;
        }
        else if(returnStatus == NeuroscopeDoc::ALREADY_OPENED){
            QApplication::restoreOverrideCursor();
            KMessageBox::error (this,tr("The requested file (" + static_cast<QString>(*iterator).path() + ") is already loaded."), tr("Error!"));
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            continue;
        }

        //Create the cluster palette if need it
        counter++;
        QString clusterFileId = doc->lastLoadedProviderName();
        if(clusterFileList.isEmpty() && counter == 1) createClusterPalette(clusterFileId);
        else addClusterFile(clusterFileId);
    }
    QApplication::restoreOverrideCursor();
}

void NeuroscopeApp::createClusterPalette(QString clusterFileId){  

    KDockWidget* clusterDock;
    if(displayPaletteHeaders) clusterDock = createDockWidget("clusterPanel",QPixmap(":/icons/clusters", 0L,tr("Units"), tr("Units"));
            else clusterDock = createDockWidget("clusterPanel",QPixmap(":/icons/clusters", 0L,"");
            ItemPalette* clusterPalette = new ItemPalette(ItemPalette::CLUSTER,backgroundColor,clusterDock,"units");
    clusterDock->setWidget(clusterPalette);
    clusterFileList.append(clusterFileId);

    //Create the list
    clusterPalette->createItemList(doc->providerColorList(clusterFileId),clusterFileId,0);

    clusterDock->setEnableDocking(KDockWidget::DockFullSite);

    //Temporarily allow addition of a new dockWidget in the center
    displayPanel->setDockSite(KDockWidget::DockCenter);
    
    //Add the new palette as a tab and get a new DockWidget, grandParent of the target (displayPanel)
    //and the new palette.
    KDockWidget* grandParent = clusterDock->manualDock(displayPanel,KDockWidget::DockCenter,50,QPoint(0, 0),false,paletteTabsParent->count()-1);

    //Disconnect the previous connection
    if(paletteTabsParent != NULL) disconnect(paletteTabsParent,0,0,0);

    //The grandParent's widget is the KDockTabGroup regrouping all the tabs
    paletteTabsParent = static_cast<KDockTabGroup*>(grandParent->getWidget());

    //Connect the change tab signal to slotPaletteTabChange(QWidget* widget) to trigger updates when
    //the active palette changes.
    connect(paletteTabsParent, SIGNAL(currentChanged(QWidget*)), this, SLOT(slotPaletteTabChange(QWidget*)));

    //Disable the possibility to dock the palette or to dock into it.
    grandParent->setEnableDocking(KDockWidget::DockNone);
    grandParent->setDockSite(KDockWidget::DockNone);

    //allow dock on the right side only (the displays will be on the rigth side)
    palettePanel->setDockSite(KDockWidget::DockRight);

    // forbit docking abilities of the clusterDock itself
    clusterDock->setEnableDocking(KDockWidget::DockNone);
    // allow others to dock to the left side only
    clusterDock->setDockSite(KDockWidget::DockRight);

    //Palette connections
    connect(clusterPalette, SIGNAL(colorChanged(int,QString)), this, SLOT(slotClusterColorUpdate(int,QString)));
    connect(clusterPalette, SIGNAL(updateShownItems(const QMap<QString,Q3ValueList<int> >&)), this, SLOT(slotUpdateShownClusters(const QMap<QString,Q3ValueList<int> >&)));
    connect(clusterPalette, SIGNAL(updateItemsToSkip(QString,const Q3ValueList<int>&)), this, SLOT(slotUpdateClustersToSkip(QString,const Q3ValueList<int>&)));
    connect(clusterPalette,SIGNAL(noClustersToBrowse()),this, SLOT(slotNoClustersToBrowse()));
    connect(clusterPalette,SIGNAL(clustersToBrowse()),this, SLOT(slotClustersToBrowse()));

    slotStateChanged("clusterState");
    //Waveforms are allowed only for dat and fil files.
    if(filePath.contains(".dat")||filePath.contains(".fil")) slotStateChanged("datState");
    else slotStateChanged("noDatState");
}

void NeuroscopeApp::addClusterFile(QString clusterFileId){
    clusterFileList.append(clusterFileId);

    for(int i = 0; i<paletteTabsParent->count();i++){
        KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->page(i));
        QString name = current->name();
        if((current->getWidget())->isA("ItemPalette") && name.contains("clusterPanel")){
            ItemPalette* clusterPalette = static_cast<ItemPalette*>(current->getWidget());
            //Create the list
            clusterPalette->createItemList(doc->providerColorList(clusterFileId),clusterFileId,0);
            break;
        }
    }
}


void NeuroscopeApp::slotClusterColorUpdate(int clusterId,QString providerName){  
    KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->currentPage());
    QString name = current->name();
    if((current->getWidget())->isA("ItemPalette") && name.contains("clusterPanel")){
        NeuroscopeView* view = activeView();
        doc->clusterColorUpdate(providerName,clusterId,view);
    }

}

void NeuroscopeApp::slotUpdateShownClusters(const QMap<QString,Q3ValueList<int> >& selection){ 
    KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->currentPage());
    QString name = current->name();
    if((current->getWidget())->isA("ItemPalette") && name.contains("clusterPanel")){
        QMap<QString,Q3ValueList<int> >::ConstIterator groupIterator;
        for(groupIterator = selection.begin(); groupIterator != selection.end(); ++groupIterator){
            QString providerName = groupIterator.key();
            Q3ValueList<int> clusterIds = groupIterator.data();
            NeuroscopeView* view = activeView();
            view->shownClustersUpdate(providerName,clusterIds);
        }
    }
}

void NeuroscopeApp::slotCloseClusterFile(){
    KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->currentPage());
    QString name = current->name();
    if((current->getWidget())->isA("ItemPalette") && name.contains("clusterPanel")){
        ItemPalette* clusterPalette = static_cast<ItemPalette*>(current->getWidget());
        QString providerName = clusterPalette->selectedGroup();

        if(providerName != ""){
            NeuroscopeView* view = activeView();
            doc->removeClusterFile(providerName,view);

            clusterPalette->removeGroup(providerName);

            clusterFileList.remove(providerName);
            if(clusterFileList.isEmpty()){
                paletteTabsParent->removePage(current);
                delete current;
                slotStateChanged("noClusterState");
            }
            else{
                slotStateChanged("clusterState");
                if(!clusterPalette->isBrowsingEnable()) slotStateChanged("noClusterBrowsingState");
                else slotStateChanged("clusterBrowsingState");
            }
        }
    }
}

void NeuroscopeApp::slotShowNextCluster(){
    NeuroscopeView* view = activeView();
    view->showNextCluster();
}

void NeuroscopeApp::slotShowPreviousCluster(){
    NeuroscopeView* view = activeView();
    view->showPreviousCluster();
}


void NeuroscopeApp::loadPositionFile(QString url){
    NeuroscopeView* view = activeView();
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    //Create the provider
    int returnStatus = doc->loadPositionFile(url,view);
    if(returnStatus == NeuroscopeDoc::OPEN_ERROR){
        QApplication::restoreOverrideCursor();
        KMessageBox::error (this,tr("Could not load the file " + url.path()), tr("Error !"));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    }
    if(returnStatus == NeuroscopeDoc::INCORRECT_FILE){
        QApplication::restoreOverrideCursor();
        KMessageBox::error (0,tr("Incorrect file name (" + url.path() + "): extension missing."), tr("Error!"));

        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    }
    else{
        isPositionFileLoaded = true;
        positionViewToggle->setChecked(true);

        slotStateChanged("positionState");
        if(!eventFileList.isEmpty()) slotStateChanged("eventsInPositionViewEnableState");
    }

    QApplication::restoreOverrideCursor();
}

void NeuroscopeApp::loadEventFiles(QStringList urls){

    NeuroscopeView* view = activeView();
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    //Loop on the files
    int counter = 0;
    QStringList::iterator iterator;
    for(iterator = urls.begin();iterator != urls.end();++iterator){
        //Create the provider
        int returnStatus = doc->loadEventFile(*iterator,view);
        if(returnStatus == NeuroscopeDoc::OPEN_ERROR){
            QApplication::restoreOverrideCursor();
            KMessageBox::error (this,tr("Could not load the file " + static_cast<QString>(*iterator).path()), tr("Error!"));
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            continue;
        }
        else if(returnStatus == NeuroscopeDoc::INCORRECT_FILE){
            QApplication::restoreOverrideCursor();
            KMessageBox::error (0,tr("Incorrect file name (" + static_cast<QString>(*iterator).path() + "): the name has to be of the form baseName.id.evt or baseName.evt.id (with id a 3 character identifier)."), tr("Error!"));
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            continue;
        }
        else if(returnStatus == NeuroscopeDoc::CREATION_ERROR){
            QApplication::restoreOverrideCursor();
            KMessageBox::error (this,tr("The number of events of the requested file (" + static_cast<QString>(*iterator).path() + ") could not be determined."), tr("Error!"));
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            continue;
        }
        else if(returnStatus == NeuroscopeDoc::INCORRECT_CONTENT){
            QApplication::restoreOverrideCursor();
            KMessageBox::error (this,tr("The content of the requested file (" + static_cast<QString>(*iterator).path() + ") is incorrect (see file format information)."), tr("Error!"));
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            continue;
        }
        else if(returnStatus == NeuroscopeDoc::ALREADY_OPENED){
            QApplication::restoreOverrideCursor();
            KMessageBox::error (this,tr("The requested file (" + static_cast<QString>(*iterator).path() + ") is already loaded."), tr("Error!"));
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            continue;
        }

        //Create the event palette if need it
        counter++;
        QString eventFileId = doc->lastLoadedProviderName();
        if(eventFileList.isEmpty() && counter == 1) createEventPalette(eventFileId);
        else addEventFile(eventFileId);
    }
    QApplication::restoreOverrideCursor();
}


void NeuroscopeApp::createEventPalette(QString eventFileId){

    KDockWidget* eventDock;
    if(displayPaletteHeaders) eventDock = createDockWidget("eventPanel",QPixmap(":/icons/events", 0L,tr("Events"), tr("Events"));
            else eventDock = createDockWidget("eventPanel",QPixmap(":/icons/events", 0L,"");
            ItemPalette* eventPalette = new ItemPalette(ItemPalette::EVENT,backgroundColor,eventDock,"events");
    eventDock->setWidget(eventPalette);
    eventFileList.append(eventFileId);

    //Palette connections
    connect(eventPalette, SIGNAL(colorChanged(int,QString)), this, SLOT(slotEventColorUpdate(int,QString)));
    connect(eventPalette, SIGNAL(updateShownItems(const QMap<QString,Q3ValueList<int> >&)), this, SLOT(slotUpdateShownEvents(const QMap<QString,Q3ValueList<int> >&)));
    connect(eventPalette, SIGNAL(selectedGroupChanged(QString)), this, SLOT(slotEventGroupSelected(QString)));
    connect(eventPalette, SIGNAL(updateItemsToSkip(QString,const Q3ValueList<int>&)), this, SLOT(slotUpdateEventsToSkip(QString,const Q3ValueList<int>&)));
    connect(eventPalette,SIGNAL(noEventsToBrowse()),this, SLOT(slotNoEventsToBrowse()));
    connect(eventPalette,SIGNAL(eventsToBrowse()),this, SLOT(slotEventsToBrowse()));

    //Create the list
    eventPalette->createItemList(doc->providerColorList(eventFileId),eventFileId,doc->getLastEventProviderGridX());

    eventDock->setEnableDocking(KDockWidget::DockFullSite);

    //Temporarily allow addition of a new dockWidget in the center
    displayPanel->setDockSite(KDockWidget::DockCenter);
    //Add the new palette as a tab and get a new DockWidget, grandParent of the target (displayPanel)
    //and the new palette.
    KDockWidget* grandParent = eventDock->manualDock(displayPanel,KDockWidget::DockCenter,50,QPoint(0, 0),false,paletteTabsParent->count()-1);

    //Disconnect the previous connection
    if(paletteTabsParent != NULL) disconnect(paletteTabsParent,0,0,0);

    //The grandParent's widget is the KDockTabGroup regrouping all the tabs
    paletteTabsParent = static_cast<KDockTabGroup*>(grandParent->getWidget());

    //Connect the change tab signal to slotPaletteTabChange(QWidget* widget) to trigger updates when
    //the active palette changes.
    connect(paletteTabsParent, SIGNAL(currentChanged(QWidget*)), this, SLOT(slotPaletteTabChange(QWidget*)));

    //Disable the possibility to dock the palette or to dock into it.
    grandParent->setEnableDocking(KDockWidget::DockNone);
    grandParent->setDockSite(KDockWidget::DockNone);

    //allow dock on the right side only (the displays will be on the rigth side)
    palettePanel->setDockSite(KDockWidget::DockRight);

    // forbit docking abilities of the clusterDock itself
    eventDock->setEnableDocking(KDockWidget::DockNone);
    // allow others to dock to the left side only
    eventDock->setDockSite(KDockWidget::DockRight);

    slotStateChanged("eventState");
    if(isPositionFileLoaded) slotStateChanged("eventsInPositionViewEnableState");
}

void NeuroscopeApp::addEventFile(QString eventFileId){
    eventFileList.append(eventFileId);

    for(int i = 0; i<paletteTabsParent->count();i++){
        KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->page(i));
        QString name = current->name();
        if((current->getWidget())->isA("ItemPalette") && name.contains("eventPanel")){
            ItemPalette* eventPalette = static_cast<ItemPalette*>(current->getWidget());
            //Create the list
            eventPalette->createItemList(doc->providerColorList(eventFileId),eventFileId,doc->getLastEventProviderGridX());
            break;
            if(isPositionFileLoaded) slotStateChanged("eventsInPositionViewEnableState");
        }
    }
}

void NeuroscopeApp::slotEventColorUpdate(int eventId,QString providerName){  
    KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->currentPage());
    QString name = current->name();
    if((current->getWidget())->isA("ItemPalette") && name.contains("eventPanel")){
        NeuroscopeView* view = activeView();
        doc->eventColorUpdate(providerName,eventId,view);
    }
}

void NeuroscopeApp::slotUpdateShownEvents(const QMap<QString,Q3ValueList<int> >& selection){  
    KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->currentPage());
    QString name = current->name();
    if((current->getWidget())->isA("ItemPalette") && name.contains("eventPanel")){
        QMap<QString,Q3ValueList<int> >::ConstIterator groupIterator;
        for(groupIterator = selection.begin(); groupIterator != selection.end(); ++groupIterator){
            QString providerName = groupIterator.key();
            Q3ValueList<int> eventIds = groupIterator.data();
            NeuroscopeView* view = activeView();
            view->shownEventsUpdate(providerName,eventIds);
        }
    }
}

void NeuroscopeApp::slotCloseEventFile(){
    KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->currentPage());
    QString name = current->name();
    if((current->getWidget())->isA("ItemPalette") && name.contains("eventPanel")){
        ItemPalette* eventPalette = static_cast<ItemPalette*>(current->getWidget());

        NeuroscopeView* view = activeView();
        if(eventFileList.count() == 1) doc->removeEventFile(eventProvider,view,true);
        else doc->removeEventFile(eventProvider,view,false);
        eventFileList.remove(eventProvider);
        eventPalette->removeGroup(eventProvider);

        if(eventFileList.isEmpty()){
            paletteTabsParent->removePage(current);
            eventProvider = "";
            currentNbUndo = 0;
            currentNbRedo = 0;
            delete current;
            slotStateChanged("noEventState");
            showEventsInPositionView->setChecked(false);
        }
        else{
            slotStateChanged("eventState");
            if(!eventPalette->isBrowsingEnable()) slotStateChanged("noEventBrowsingState");
            else slotStateChanged("eventBrowsingState");
        }
    }
}



void NeuroscopeApp::slotShowNextEvent(){
    NeuroscopeView* view = activeView();
    view->showNextEvent();
}

void NeuroscopeApp::slotShowPreviousEvent(){
    NeuroscopeView* view = activeView();
    view->showPreviousEvent();
}

void NeuroscopeApp::slotEventModified(QString providerName,int selectedEventId,double time,double newTime){
    eventsModified = true;
    currentNbUndo = 1;
    currentNbRedo = 0;
    slotStateChanged("undoState");
    slotStateChanged("emptyRedoState");
    doc->eventModified(providerName,selectedEventId,time,newTime,activeView());
}

void NeuroscopeApp::slotUndo()
{
    slotStatusMsg(tr("Reverting last action..."));

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    currentNbUndo = 0;
    slotStateChanged("emptyUndoState");
    currentNbRedo = 1;
    eventsModified = true;//in case the user saved and then undo, this will allowed to save again
    undoRedoInprocess = true;
    doc->undo(activeView());
    undoRedoInprocess = false;
    QApplication::restoreOverrideCursor();

    slotStatusMsg(tr("Ready."));
}

void NeuroscopeApp::slotRedo()
{
    slotStatusMsg(tr("Reverting last undo action..."));
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    currentNbUndo = 1;
    currentNbRedo = 0;
    slotStateChanged("undoState");
    slotStateChanged("emptyRedoState");
    eventsModified = true;//in case the user saved and then undo, this will allowed to save again
    undoRedoInprocess = true;
    doc->redo(activeView());
    undoRedoInprocess = false;
    QApplication::restoreOverrideCursor();

    slotStatusMsg(tr("Ready."));
}

void NeuroscopeApp::removeEvent(){
    NeuroscopeView* view = activeView();
    view->removeEvent();
}

void NeuroscopeApp::slotEventRemoved(QString providerName,int selectedEventId,double time){
    eventsModified = true;
    currentNbUndo = 1;
    currentNbRedo = 0;
    slotStateChanged("undoState");
    slotStateChanged("emptyRedoState");
    doc->eventRemoved(providerName,selectedEventId,time,activeView());
}

void NeuroscopeApp::addEvent(){
    NeuroscopeView* view = activeView();
    view->setMode(TraceView::ADD_EVENT,true);
    view->eventToAddProperties(eventProvider,eventLabelToCreate);
}


void NeuroscopeApp::slotAddEventAboutToShow(){
#if KDAB_PENDING
    addEventPopup->clear();
    addEventPopup->resize(0,0);
    addEventPopup->adjustSize();
    addEventMenu->menu()->clear();

    eventIndex = -1;
    QStringList list;
    Q3ValueList<EventDescription> eventList = doc->eventIds(eventProvider);

    Q3ValueList<EventDescription>::iterator it;
    for(it = eventList.begin(); it != eventList.end(); ++it){
        QString label = static_cast<QString>(*it);
        int index = addEventPopup->insertItem(label);
        list.append(label);
        if(eventLabelToCreate == label){
            buttonEventIndex = index;
            eventIndex = list.size() - 1;
        }
    }

    //Add at the bottom an entry to create a new event description
    addEventPopup->insertItem("New Event ...");
    list.append("New Event ...");

    addEventMenu->setItems(list);
    //If eventLabelToCreate exists in this event file, select it
    if(eventIndex != -1){
        addEventMenu->setCurrentItem(eventIndex);
        addEventPopup->setItemChecked(buttonEventIndex,true);
    }
    else eventLabelToCreate = "";
#endif
}

void NeuroscopeApp::slotAddEventButtonActivated(QAction *act){
    if(!act)
        return;
#if KDAB_PENDING
    buttonEventIndex = index;

    QString description = act->text();
    if(index == (addEventPopup->idAt(addEventPopup->count() - 1))) {
        bool ok;
        QString result = QInputDialog::getText(this,tr("New Event Description"),tr("Type in the new event description"),QLineEdit::Normal,QString::Null,&ok);
        if(ok) {
            eventLabelToCreate = result;
        }
    }
    else eventLabelToCreate = description;
#endif
    addEvent();
}

void NeuroscopeApp::slotAddEventActivated(int index){
    if(eventProvider.isEmpty()) return;
    QMenu* menu = addEventMenu->menu();
    eventIndex = index;

    QString description = menu->text(index);
    if(index == static_cast<int>(menu->count() - 1)) {
        bool ok;
        QString result = QInputDialog::getText(this,tr("New Event Description"),tr("Type in the new event description"),QLineEdit::Normal,QString::Null,&ok);
        if(ok) {
            eventLabelToCreate = result;
        }
    }
    else eventLabelToCreate = description;

    addEvent();
}

ItemPalette* NeuroscopeApp::getEventPalette(){
    ItemPalette* eventPalette = 0L;

    for(int i = 0; i< paletteTabsParent->count();++i){
        KDockWidget* current = static_cast<KDockWidget*>(paletteTabsParent->page(i));
        QString name = current->name();
        if((current->getWidget())->isA("ItemPalette") && name.contains("eventPanel")){
            eventPalette = static_cast<ItemPalette*>(current->getWidget());
            break;
        }
    }

    return eventPalette;
}

void NeuroscopeApp::slotEventGroupSelected(QString eventGroupName){
    //This function can be called because an undo or redo action,
    //this should not be taken into account.
    if(undoRedoInprocess) return;
    eventProvider = eventGroupName;
    eventLabelToCreate = "";
    NeuroscopeView* view = activeView();
    view->eventToAddProperties(eventProvider,eventLabelToCreate);
}

void NeuroscopeApp::slotEventAdded(QString providerName,QString addEventDescription,double time){  
    eventsModified = true;
    currentNbUndo = 1;
    currentNbRedo = 0;
    slotStateChanged("undoState");
    slotStateChanged("emptyRedoState");
    doc->eventAdded(providerName,addEventDescription,time,activeView());
}

void NeuroscopeApp::slotClosePositionFile(){
    NeuroscopeView* view = activeView();
    doc->removePositionFile(view);
    isPositionFileLoaded = false;
    positionViewToggle->setChecked(false);
    showEventsInPositionView->setChecked(false);
    slotStateChanged("noPositionState");
}

void NeuroscopeApp::slotUpdateEventsToSkip(QString groupName,const Q3ValueList<int>& eventsToSkip){
    activeView()->updateNoneBrowsingEventList(groupName,eventsToSkip);
}

void NeuroscopeApp::slotUpdateClustersToSkip(QString groupName,const Q3ValueList<int>& clustersToSkip){
    activeView()->updateNoneBrowsingClusterList(groupName,clustersToSkip);
}

void NeuroscopeApp::slotSetDefaultOffsets(){
    doc->setDefaultOffsets(activeView());
}

void NeuroscopeApp::slotResetDefaultOffsets(){
    doc->resetDefaultOffsets();
}

void NeuroscopeApp::slotIncreaseRasterHeight(){
    activeView()->increaseRasterHeight();
}

void NeuroscopeApp::slotDecreaseRasterHeight(){
    activeView()->decreaseRasterHeight();
}

void NeuroscopeApp::slotShowEventsInPositionView(){
    NeuroscopeView* view = activeView();
    view->setEventsInPositionView(showEventsInPositionView->isChecked());
}

#include "neuroscope.moc"
