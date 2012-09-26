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
#include <QList>
#include <QEvent>
#include <qextendtabwidget.h>

#include <QInputDialog>
#include <QFileDialog>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QMessageBox>
#include <qrecentfileaction.h>


#include <QStatusBar>
#include <QToolBar>


#include <QProcess>
#include <QSplitter>

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



NeuroscopeApp::NeuroscopeApp()
    :QMainWindow(0, "NeuroScope")
    ,prefDialog(0L),displayCount(0),mainDock(0),
      spikeChannelPalette(0),tabsParent(0L),paletteTabsParent(0L),
      isInit(true),groupsModified(false),colorModified(false),eventsModified(false),initialOffsetDefault(0),propertiesDialog(0L),
      select(false),filePath(""),initialTimeWindow(0),eventIndex(0),buttonEventIndex(0),eventLabelToCreate(""),
      eventProvider(""),undoRedoInprocess(false),isPositionFileLoaded(false)
{
    initView();
    //Prepare the actions
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


    initActions();


    //Disable some actions at startup (see the neuroscope.rc file)
    slotStateChanged("initState");
}

NeuroscopeApp::~NeuroscopeApp()
{
    //Clear the memory by deleting all the pointers
    delete doc;
    delete printer;
}


void NeuroscopeApp::initView()
{
    QSplitter *splitter = new QSplitter;
    paletteTabsParent = new QTabWidget();
    splitter->addWidget(paletteTabsParent);
    tabsParent = new QExtendTabWidget(this);
    splitter->addWidget(tabsParent);
    setCentralWidget(splitter);
    QList<int> size;
    size <<10<<90;
    splitter->setSizes(size);
}


void NeuroscopeApp::initActions()
{
    //Custom actions and menus

    //File Menu
    QMenu *fileMenu = menuBar()->addMenu(tr("File"));
    mOpenAction = fileMenu->addAction(tr("&Open..."));
    mOpenAction->setShortcut(QKeySequence::Open);
    mOpenAction->setIcon(QPixmap(":/shared-icons/document-open"));
    connect(mOpenAction, SIGNAL(triggered()), this, SLOT(slotFileOpen()));

    mFileOpenRecent = new QRecentFileAction(this);
    fileMenu->addAction(mFileOpenRecent);
    connect(mFileOpenRecent, SIGNAL(recentFileSelected(QString)), this, SLOT(slotFileOpenRecent(QString)));

    mLoadClusterFiles = fileMenu->addAction(tr("Load Cl&uster File(s)..."));
    connect(mLoadClusterFiles,SIGNAL(triggered()), this,SLOT(slotLoadClusterFiles()));

    mLoadEventFiles = fileMenu->addAction(tr("Load &Event File(s)..."));
    connect(mLoadEventFiles,SIGNAL(triggered()), this,SLOT(slotLoadEventFiles()));

    mCreateEventFile = fileMenu->addAction(tr("Create Event &File..."));
    connect(mCreateEventFile,SIGNAL(triggered()), this,SLOT(slotCreateEventFile()));

    mLoadPositionFile = fileMenu->addAction(tr("Load Posi&tion File..."));
    connect(mLoadPositionFile,SIGNAL(triggered()), this,SLOT(slotLoadPositionFile()));

    fileMenu->addSeparator();


    mSaveAction = fileMenu->addAction(tr("Save..."));
    mSaveAction->setIcon(QPixmap(":/shared-icons/document-save"));
    mSaveAction->setShortcut(QKeySequence::Save);
    connect(mSaveAction, SIGNAL(triggered()), this, SLOT(saveSession()));

    mSaveAsAction = fileMenu->addAction(tr("&Save As..."));
    mSaveAsAction->setIcon(QPixmap(":/shared-icons/document-save-as"));
    mSaveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(mSaveAsAction, SIGNAL(triggered()), this, SLOT(slotSessionSaveAs()));



    fileMenu->addSeparator();


    mPrintAction = fileMenu->addAction(tr("Print"));
    mPrintAction->setIcon(QPixmap(":/shared-icons/document-print"));
    mPrintAction->setShortcut(QKeySequence::Print);
    connect(mPrintAction, SIGNAL(triggered()), this, SLOT(slotFilePrint()));

    fileMenu->addSeparator();

    mProperties = fileMenu->addAction(tr("&Properties"));
    connect(mProperties,SIGNAL(triggered()), this,SLOT(slotFileProperties()));


    mCloseAction = fileMenu->addAction(tr("Close"));
    mCloseAction->setIcon(QPixmap(":/shared-icons/document-close"));
    mCloseAction->setShortcut(QKeySequence::Close);
    connect(mCloseAction, SIGNAL(triggered()), this, SLOT(slotFileClose()));


    mCloseCluster = fileMenu->addAction(tr("Close C&luster File"));
    connect(mCloseCluster,SIGNAL(triggered()), this,SLOT(slotCloseClusterFile()));

    mCloseEvent = fileMenu->addAction(tr("Close E&vent File"));
    connect(mCloseEvent,SIGNAL(triggered()), this,SLOT(slotCloseEventFile()));

    mClosePositionFile = fileMenu->addAction(tr("Close Position File"));
    connect(mClosePositionFile,SIGNAL(triggered()), this,SLOT(slotClosePositionFile()));


    fileMenu->addSeparator();

    mQuitAction = fileMenu->addAction(tr("Quit"));
    mQuitAction->setShortcut(QKeySequence::Print);
    connect(mQuitAction, SIGNAL(triggered()), this, SLOT(close()));



    //Edit menu
    QMenu *editMenu = menuBar()->addMenu(tr("Edit"));

    mUndo = editMenu->addAction(tr("Undo"));
    mUndo->setIcon(QPixmap(":/shared-icons/edit-undo"));
    mUndo->setShortcut(QKeySequence::Undo);
    connect(mUndo, SIGNAL(triggered()), this, SLOT(slotUndo()));

    mRedo = editMenu->addAction(tr("Redo"));
    mRedo->setIcon(QPixmap(":/shared-icons/edit-redo"));
    mRedo->setShortcut(QKeySequence::Redo);
    connect(mRedo, SIGNAL(triggered()), this, SLOT(slotRedo()));

    editMenu->addSeparator();

    mSelectAll = editMenu->addAction(tr("Select &All"));
    mSelectAll->setShortcut(Qt::CTRL + Qt::Key_A);
    connect(mSelectAll,SIGNAL(triggered()), this,SLOT(slotSelectAll()));

    editMenu->addSeparator();

    mSelectAllExcept0And1 = editMenu->addAction(tr("Select All e&xcept 0 and 1"));
    mSelectAllExcept0And1->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_A);
    connect(mSelectAllExcept0And1,SIGNAL(triggered()), this,SLOT(slotSelectAllWO01()));

    mDeselectAll = editMenu->addAction(tr("Deselect All"));
    mDeselectAll->setShortcut(Qt::CTRL + Qt::Key_U);
    connect(mDeselectAll,SIGNAL(triggered()), this,SLOT(slotDeselectAll()));

    editMode = editMenu->addAction(tr("&Edit Mode"));
    editMode->setIcon(QIcon(":/icons/edit"));
    editMode->setShortcut(Qt::CTRL + Qt::Key_E);
    editMode->setCheckable(true);
    connect(editMode,SIGNAL(triggered()), this,SLOT(slotEditMode()));

    editMode->setChecked(true);

    //Tools menu
    QMenu *toolMenu = menuBar()->addMenu(tr("Tool"));
    mZoomTool = toolMenu->addAction(tr("Zoom"));
    mZoomTool->setIcon(QIcon(":/icons/zoom_tool"));
    mZoomTool->setShortcut(Qt::Key_Z);
    connect(mZoomTool,SIGNAL(triggered()), this,SLOT(slotZoom()));


    mDrawTimeLine = toolMenu->addAction(tr("Draw Time Line"));
    mDrawTimeLine->setIcon(QIcon(":/icons/time_line_tool"));
    mDrawTimeLine->setShortcut(Qt::Key_L);
    connect(mDrawTimeLine,SIGNAL(triggered()), this,SLOT(slotDrawTimeLine()));


    mSelectTool = toolMenu->addAction(tr("Select Channels"));
    mSelectTool->setIcon(QIcon(":/icons/select_tool"));
    mSelectTool->setShortcut(Qt::Key_C);
    connect(mSelectTool,SIGNAL(triggered()), this,SLOT(slotSelect()));

    mMeasureTool = toolMenu->addAction(tr("Measure"));
    mMeasureTool->setIcon(QIcon(":/icons/measure_tool"));
    mMeasureTool->setShortcut(Qt::Key_V);
    connect(mMeasureTool,SIGNAL(triggered()), this,SLOT(slotMeasure()));

    mTimeTool = toolMenu->addAction(tr("Select Time"));
    mTimeTool->setIcon(QIcon(":/icons/time_tool"));
    mTimeTool->setShortcut(Qt::Key_T);
    connect(mTimeTool,SIGNAL(triggered()), this,SLOT(slotSelectTime()));

    mEventTool = toolMenu->addAction(tr("Select Event"));
    mEventTool->setIcon(QIcon(":/icons/event_tool"));
    mEventTool->setShortcut(Qt::Key_E);
    connect(mEventTool,SIGNAL(triggered()), this,SLOT(slotSelectEvent()));
    //KDAB_PORTING!
    addEventMenu = 0;
#if KDAB_PENDING
    addEventMenu = new KSelectAction(tr("Add Event"),QIcon(":/icons/add_event_tool"),Qt::Key_N,this, SLOT(addEvent()),actionCollection(), "add_event");
    connect(addEventMenu->popupMenu(), SIGNAL(aboutToShow()), this, SLOT(slotAddEventAboutToShow()));
    connect(addEventMenu->popupMenu(), SIGNAL(activated(int)), this, SLOT(slotAddEventActivated(int)));
#endif
    addEventToolBarAction = toolMenu->addAction(tr("Add Event"));
    addEventToolBarAction->setIcon(QIcon(":icons/add_event_tool"));
    connect(addEventToolBarAction,SIGNAL(triggered()), this,SLOT(addEvent()));

    addEventPopup = new QMenu;
    addEventToolBarAction->setMenu(addEventPopup);
    addEventPopup->setCheckable(true);
    connect(addEventPopup, SIGNAL(aboutToShow()), this, SLOT(slotAddEventAboutToShow()));
    connect(addEventPopup, SIGNAL(triggered(QAction*)), this, SLOT(slotAddEventButtonActivated(QAction*)));

    //Channels Menu
    QMenu *channelsMenu = menuBar()->addMenu(tr("&Channels"));
    mShowChannel = channelsMenu->addAction(tr("Show &Channels"));
    mShowChannel->setIcon(QIcon(":/icons/eye"));
    mShowChannel->setShortcut(Qt::CTRL + Qt::Key_C);
    connect(mShowChannel,SIGNAL(triggered()), this,SLOT(slotShowChannels()));

    mHideChannel = channelsMenu->addAction(tr("&Hide Channels"));
    mHideChannel->setIcon(QIcon(":/icons/eye_close"));
    mHideChannel->setShortcut(Qt::CTRL + Qt::Key_H);
    connect(mHideChannel,SIGNAL(triggered()), this,SLOT(slotHideChannels()));

    channelsMenu->addSeparator();

    mMoveToNewGroup = channelsMenu->addAction(tr("&Move Channels to New Group"));
    mMoveToNewGroup->setIcon(QIcon(":/icons/new_group"));
    mMoveToNewGroup->setShortcut(Qt::CTRL + Qt::Key_G);
    connect(mMoveToNewGroup,SIGNAL(triggered()), this,SLOT(slotCreateGroup()));

    mRemoveChannelFromGroup = channelsMenu->addAction(tr("&Remove Channels from Group"));
    mRemoveChannelFromGroup->setIcon(QIcon(":/icons/remove"));
    mRemoveChannelFromGroup->setShortcut(Qt::SHIFT + Qt::Key_Delete);
    connect(mRemoveChannelFromGroup,SIGNAL(triggered()), this,SLOT(slotDiscardSpikeChannels()));

    mDiscardChannels = channelsMenu->addAction(tr("&Discard Channels"));
    mDiscardChannels->setIcon(QIcon(":/icons/discard"));
    mDiscardChannels->setShortcut(Qt::Key_Delete);
    connect(mDiscardChannels,SIGNAL(triggered()), this,SLOT(slotDiscardChannels()));

    channelsMenu->addSeparator();

    mKeepChannels = channelsMenu->addAction(tr("&Keep Channels"));
    mKeepChannels->setIcon(QIcon(":/icons/keep"));
    mKeepChannels->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_K);
    connect(mKeepChannels,SIGNAL(triggered()), this,SLOT(slotKeepChannels()));

    mSkipChannels = channelsMenu->addAction(tr("&Skip Channels"));
    mSkipChannels->setIcon(QIcon(":/icons/skip"));
    mSkipChannels->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_S);
    connect(mSkipChannels,SIGNAL(triggered()), this,SLOT(slotSkipChannels()));
    channelsMenu->addSeparator();

    mSynchronizeGroups =channelsMenu->addAction(tr("&Synchronize Groups"));
    connect(mSynchronizeGroups,SIGNAL(triggered()), this,SLOT(slotSynchronize()));

    channelsMenu->addSeparator();
    showHideLabels = channelsMenu->addAction(tr("Show &Labels"));
    showHideLabels->setShortcut(Qt::CTRL + Qt::Key_L);
    showHideLabels->setCheckable(true);
    connect(showHideLabels,SIGNAL(triggered()), this,SLOT(slotShowLabels()));

    showHideLabels->setChecked(false);

    //Color section
    mColorAnatomicalGroups = channelsMenu->addAction(tr("Color by &Anatomical Groups"));
    connect(mColorAnatomicalGroups,SIGNAL(triggered()), this,SLOT(slotApplyDisplayColor()));

    mColorSpikeGroups = channelsMenu->addAction(tr("Color by S&pike Groups"));
    connect(mColorSpikeGroups,SIGNAL(triggered()), this,SLOT(slotApplySpikeColor()));


    //Units Menu
    QMenu *unitsMenu = menuBar()->addMenu(tr("&Units"));
    clusterVerticalLines = unitsMenu->addAction(tr("&Vertical Lines"));
    clusterVerticalLines->setCheckable(true);
    connect(clusterVerticalLines,SIGNAL(triggered()), this,SLOT(slotClustersVerticalLines()));

    clusterVerticalLines->setChecked(false);
    clusterRaster = unitsMenu->addAction(tr("&Raster"));
    clusterRaster->setCheckable(true);
    connect(clusterRaster,SIGNAL(triggered()), this,SLOT(slotClustersRaster()));

    clusterRaster->setChecked(true);
    clusterWaveforms = unitsMenu->addAction(tr("&Waveforms"));
    clusterWaveforms->setCheckable(true);
    connect(clusterWaveforms,SIGNAL(triggered()), this,SLOT(slotClustersWaveforms()));
    clusterWaveforms->setChecked(false);

    unitsMenu->addSeparator();

    mIncreaseHeight = unitsMenu->addAction(tr("&Increase Height"));
    mIncreaseHeight->setShortcut(Qt::CTRL + Qt::Key_Plus);
    connect(mIncreaseHeight,SIGNAL(triggered()), this,SLOT(slotIncreaseRasterHeight()));

    mDecreaseHeight = unitsMenu->addAction(tr("&Decrease Height"));
    mDecreaseHeight->setShortcut(Qt::CTRL + Qt::Key_Minus);
    connect(mDecreaseHeight,SIGNAL(triggered()), this,SLOT(slotDecreaseRasterHeight()));

    unitsMenu->addSeparator();

    mNextSpike = unitsMenu->addAction(tr("&Next Spike"));
    mNextSpike->setIcon(QIcon(":/icons/forwardCluster"));
    mNextSpike->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_F);
    connect(mNextSpike,SIGNAL(triggered()), this,SLOT(slotShowNextCluster()));

    mPreviousSpike = unitsMenu->addAction(tr("&Previous Spike"));
    mPreviousSpike->setIcon(QIcon(":/icons/backCluster"));
    mPreviousSpike->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_B);
    connect(mPreviousSpike,SIGNAL(triggered()), this,SLOT(slotShowPreviousCluster()));


    //Events Menu
    QMenu *eventMenu = menuBar()->addMenu(tr("E&vents"));
    mNextEvent = eventMenu->addAction(tr("&Next Event"));
    mNextEvent->setIcon(QIcon(":/icons/forwardEvent"));
    mNextEvent->setShortcut(Qt::CTRL + Qt::Key_F);
    connect(mNextEvent,SIGNAL(triggered()), this,SLOT(slotShowNextEvent()));

    mPreviousEvent = eventMenu->addAction(tr("&Previous Event"));
    mPreviousEvent->setIcon(QIcon(":/icons/backEvent"));
    mPreviousEvent->setShortcut(Qt::CTRL + Qt::Key_B);
    connect(mPreviousEvent,SIGNAL(triggered()), this,SLOT(slotShowPreviousEvent()));

    mRemoveEvent = eventMenu->addAction(tr("&Remove Event"));
    mRemoveEvent->setShortcut(Qt::CTRL + Qt::Key_K);
    connect(mRemoveEvent,SIGNAL(triggered()), this,SLOT(removeEvent()));


    //Positions Menu
    QMenu *positionsMenu = menuBar()->addMenu(tr("&Positions"));
    positionViewToggle = positionsMenu->addAction(tr("&Show Position View"));
    positionViewToggle->setCheckable(true);
    connect(positionViewToggle,SIGNAL(triggered()), this,SLOT(slotShowPositionView()));

    positionViewToggle->setChecked(false);
    showEventsInPositionView = positionsMenu->addAction(tr("Show &Events"));
    showEventsInPositionView->setCheckable(true);
    connect(showEventsInPositionView,SIGNAL(triggered()), this,SLOT(slotShowEventsInPositionView()));

    showEventsInPositionView->setChecked(false);



    //Traces menu
    QMenu *traceMenu = menuBar()->addMenu(tr("&Traces"));
    displayMode = traceMenu->addAction(tr("&Multiple Columns"));
    displayMode->setCheckable(true);
    connect(displayMode,SIGNAL(triggered()), this,SLOT(slotDisplayMode()));

    traceMenu->addSeparator();

    displayMode->setChecked(false);
    greyScale = traceMenu->addAction(tr("&Grey-Scale"));
    greyScale->setCheckable(true);
    connect(greyScale,SIGNAL(triggered()), this,SLOT(slotSetGreyScale()));

    traceMenu->addSeparator();

    greyScale->setChecked(false);
    mIncreaseAllChannelAmplitudes = traceMenu->addAction(tr("&Increase All Channel Amplitudes"));
    mIncreaseAllChannelAmplitudes->setShortcut(Qt::CTRL + Qt::Key_I);
    connect(mIncreaseAllChannelAmplitudes,SIGNAL(triggered()), this,SLOT(slotIncreaseAllChannelsAmplitude()));


    mDecreaseAllChannelAmplitudes = traceMenu->addAction(tr("&Decrease All Channel Amplitudes"));
    mDecreaseAllChannelAmplitudes->setShortcut(Qt::CTRL + Qt::Key_D);
    connect(mDecreaseAllChannelAmplitudes,SIGNAL(triggered()), this,SLOT(slotDecreaseAllChannelsAmplitude()));

    mIncreaseSelectedChannelAmplitude = traceMenu->addAction(tr("I&ncrease Selected Channel Amplitudes"));
    mIncreaseSelectedChannelAmplitude->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_I);
    connect(mIncreaseSelectedChannelAmplitude,SIGNAL(triggered()), this,SLOT(slotIncreaseSelectedChannelsAmplitude()));

    mDecreaseSelectedChannelAmplitude = traceMenu->addAction(tr("D&ecrease Selected Channel Amplitudes"));
    mDecreaseSelectedChannelAmplitude->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_D);
    connect(mDecreaseSelectedChannelAmplitude,SIGNAL(triggered()), this,SLOT(slotDecreaseSelectedChannelsAmplitude()));

    mResetSelectedChannel = traceMenu->addAction(tr("Reset Selected Channel &Offsets"));
    connect(mResetSelectedChannel,SIGNAL(triggered()), this,SLOT(slotResetOffsets()));


    mResetSelectedChannelAmplitudes = traceMenu->addAction(tr("Reset Selected Channel &Amplitudes"));
    connect(mResetSelectedChannelAmplitudes,SIGNAL(triggered()), this,SLOT(slotResetGains()));

    traceMenu->addSeparator();

    mSetCurrentOffsetsAsDefault = traceMenu->addAction(tr("&Set Current Offsets as Defaults"));
    connect(mSetCurrentOffsetsAsDefault,SIGNAL(triggered()), this,SLOT(slotSetDefaultOffsets()));

    mSetDefaultOffsetToZero = traceMenu->addAction(tr("Set Default Offsets to &Zero"));
    connect(mSetDefaultOffsetToZero,SIGNAL(triggered()), this,SLOT(slotResetDefaultOffsets()));


    /// Added by M.Zugaro to enable automatic forward paging
    mPage = traceMenu->addAction(tr("Page"));
    mPage->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_Space);
    connect(mPage,SIGNAL(triggered()), this,SLOT(page()));

    mAccelerate = traceMenu->addAction(tr("Accelerate"));
    mAccelerate->setShortcut(Qt::CTRL + Qt::Key_Up);
    connect(mAccelerate,SIGNAL(triggered()), this,SLOT(accelerate()));

    mDecelerate = traceMenu->addAction(tr("Decelerate"));
    mDecelerate->setShortcut(Qt::CTRL + Qt::Key_Down);
    connect(mDecelerate,SIGNAL(triggered()), this,SLOT(decelerate()));


    //Displays Menu
    QMenu *displaysMenu = menuBar()->addMenu(tr("&Displays"));

    mNewDisplay = displaysMenu->addAction(tr("&New Display"));
    mNewDisplay->setShortcut(Qt::CTRL + Qt::Key_N);
    connect(mNewDisplay,SIGNAL(triggered()), this,SLOT(slotNewDisplay()));

    mRenameActiveDisplay = displaysMenu->addAction(tr("&Rename Active Display"));
    mRenameActiveDisplay->setShortcut(Qt::CTRL + Qt::Key_R);
    connect(mRenameActiveDisplay,SIGNAL(triggered()), this,SLOT(slotRenameActiveDisplay()));

    mCloseActiveDisplay = displaysMenu->addAction(tr("&Close Active Display"));
    mCloseActiveDisplay->setShortcut(Qt::CTRL + Qt::Key_W);
    connect(mCloseActiveDisplay,SIGNAL(triggered()), this,SLOT(slotDisplayClose()));



    //Settings menu
    QMenu *settingsMenu = menuBar()->addMenu(tr("&Settings"));

    viewMainToolBar = settingsMenu->addAction(tr("Show Main Toolbar"));

    viewMainToolBar->setCheckable(true);
    viewMainToolBar->setChecked(true);
    connect(viewMainToolBar,SIGNAL(triggered()), this,SLOT(slotViewMainToolBar()));


    viewToolBar = settingsMenu->addAction(tr("Show T&ools"));
    viewToolBar->setCheckable(true);
    connect(viewToolBar,SIGNAL(triggered()), this,SLOT(slotViewToolBar()));

    viewToolBar->setChecked(true);

    mViewStatusBar = settingsMenu->addAction(tr("Show StatusBar"));
    mViewStatusBar->setCheckable(true);
    mViewStatusBar->setChecked(true);
    connect(mViewStatusBar,SIGNAL(triggered()), this,SLOT(slotViewStatusBar()));

    settingsMenu->addSeparator();

    calibrationBar = settingsMenu->addAction(tr("&Display Calibration"));
    calibrationBar->setCheckable(true);
    connect(calibrationBar,SIGNAL(triggered()), this,SLOT(slotShowCalibration()));

    calibrationBar->setChecked(false);


    settingsMenu->addSeparator();
    mPreferenceAction = settingsMenu->addAction(tr("Preferences"));
    connect(mPreferenceAction,SIGNAL(triggered()), this,SLOT(executePreferencesDlg()));

    //Help menu
    QMenu *helpMenu = menuBar()->addMenu(tr("Help"));
    QAction *about = helpMenu->addAction(tr("About"));
    connect(about,SIGNAL(triggered()), this,SLOT(slotAbout()));


    //Custom connections
    connect(doc, SIGNAL(noSession(QMap<int,int>&,QMap<int,bool>&)),this, SLOT(slotDefaultSetUp(QMap<int,int>&,QMap<int,bool>&)));
    connect(doc, SIGNAL(loadFirstDisplay(QList<int>*,bool,bool,bool,bool,bool,bool,QList<int>,QList<int>,QList<int>,QMap<int,bool>&,long,long,QString,bool,int,bool)),this,
            SLOT(slotSetUp(QList<int>*,bool,bool,bool,bool,bool,bool,QList<int>,QList<int>,QList<int>,QMap<int,bool>&,long,long,QString,bool,int,bool)));

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
    connect(displayChannelPalette, SIGNAL(updateShownChannels(QList<int>)),this, SLOT(slotUpdateShownChannels(QList<int>)));
    connect(spikeChannelPalette, SIGNAL(updateShownChannels(QList<int>)),this, SLOT(slotUpdateShownChannels(QList<int>)));

    connect(displayChannelPalette, SIGNAL(updateHideChannels(QList<int>)),this, SLOT(slotUpdateHiddenChannels(QList<int>)));
    connect(spikeChannelPalette, SIGNAL(updateHideChannels(QList<int>)),this, SLOT(slotUpdateHiddenChannels(QList<int>)));

    connect(displayChannelPalette, SIGNAL(channelsDiscarded(QList<int>)),this, SLOT(slotChannelsDiscarded(QList<int>)));
    connect(spikeChannelPalette, SIGNAL(channelsDiscarded(QList<int>)),this, SLOT(slotChannelsDiscarded(QList<int>)));
    connect(displayChannelPalette, SIGNAL(channelsMovedToTrash(QList<int>,QString,bool)),spikeChannelPalette, SLOT(discardChannels(QList<int>,QString,bool)));
    connect(spikeChannelPalette, SIGNAL(channelsMovedToTrash(QList<int>,QString,bool)),displayChannelPalette, SLOT(discardChannels(QList<int>,QString,bool)));
    connect(displayChannelPalette, SIGNAL(channelsMovedAroundInTrash(QList<int>,QString,bool)),spikeChannelPalette, SLOT(trashChannelsMovedAround(QList<int>,QString,bool)));
    connect(spikeChannelPalette, SIGNAL(channelsMovedAroundInTrash(QList<int>,QString,bool)),displayChannelPalette, SLOT(trashChannelsMovedAround(QList<int>,QString,bool)));

    connect(displayChannelPalette, SIGNAL(channelsRemovedFromTrash(QList<int>)),spikeChannelPalette, SLOT(removeChannelsFromTrash(QList<int>)));
    connect(spikeChannelPalette, SIGNAL(channelsRemovedFromTrash(QList<int>)),displayChannelPalette, SLOT(removeChannelsFromTrash(QList<int>)));

    connect(displayChannelPalette, SIGNAL(groupModified()),this, SLOT(slotGroupsModified()));
    connect(spikeChannelPalette, SIGNAL(groupModified()),this, SLOT(slotGroupsModified()));


    connect(displayChannelPalette, SIGNAL(channelsSelected(QList<int>)),this, SLOT(slotChannelsSelected(QList<int>)));
    connect(spikeChannelPalette, SIGNAL(channelsSelected(QList<int>)),this, SLOT(slotChannelsSelected(QList<int>)));


    mMainToolBar = new QToolBar;
    mMainToolBar->addAction(mOpenAction);
    mMainToolBar->addAction(mSaveAction);
    mMainToolBar->addAction(mPrintAction);
    mMainToolBar->addSeparator();
    mMainToolBar->addAction(mUndo);
    mMainToolBar->addAction(mRedo);
    mMainToolBar->addSeparator();
    mMainToolBar->addAction(editMode);
    addToolBar(mMainToolBar);

    mToolBar = new QToolBar(tr("Tools"));
    mToolBar->addAction(mZoomTool);
    mToolBar->addAction(mDrawTimeLine);
    mToolBar->addAction(mSelectTool);
    mToolBar->addAction(mEventTool);
    //mToolBar->addAction(/*add_event_toolbarAction*/)
    mToolBar->addAction(mMeasureTool);
    mToolBar->addAction(mTimeTool);
    addToolBar(Qt::LeftToolBarArea, mToolBar);

    mChannelToolBar = new QToolBar(tr("Channels Actions"));
    //mChannelToolBar->addAction(mCreateEventFile);
    mChannelToolBar->addAction(mRemoveChannelFromGroup);
    mChannelToolBar->addAction(mDiscardChannels);
    mChannelToolBar->addAction(mKeepChannels);
    mChannelToolBar->addAction(mSkipChannels);
    mChannelToolBar->addAction(mShowChannel);
    mChannelToolBar->addAction(mHideChannel);

    addToolBar(Qt::LeftToolBarArea, mChannelToolBar);

    mEventToolBar = new QToolBar(tr("Events Actions"));
    mEventToolBar->addAction(mPreviousEvent);
    mEventToolBar->addAction(mNextEvent);
    addToolBar(Qt::LeftToolBarArea, mEventToolBar);

    mClusterToolBar = new QToolBar(tr("Clusters Actions"));
    mClusterToolBar->addAction(mPreviousSpike);
    mClusterToolBar->addAction(mNextSpike);
    addToolBar(Qt::LeftToolBarArea, mClusterToolBar);

}


void NeuroscopeApp::initStatusBar()
{
    ///////////////////////////////////////////////////////////////////
    // STATUSBAR
    // TODO: add your own items you need for displaying current application status.
    statusBar()->showMessage(tr("Ready."));
}

void NeuroscopeApp::initItemPanel(){

    displayChannelPalette = new ChannelPalette(ChannelPalette::DISPLAY,backgroundColor,true,this,"DisplaylPalette");
    spikeChannelPalette = new ChannelPalette(ChannelPalette::SPIKE,backgroundColor,true,this,"SpikePalette");

    if(displayPaletteHeaders) {
        paletteTabsParent->addTab(displayChannelPalette,QIcon(":/icons/anatomy"),tr("Anatomy"));
        paletteTabsParent->addTab(spikeChannelPalette,QIcon(":/icons/spikes"),tr("Spikes"));
    } else {
        int index = paletteTabsParent->addTab(displayChannelPalette,QString());
        paletteTabsParent->setTabIcon(index,QIcon(":/icons/anatomy"));
        index = paletteTabsParent->addTab(spikeChannelPalette,QString());
        paletteTabsParent->setTabIcon(index,QIcon(":/icons/spikes"));
    }
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
                QWidget* current = paletteTabsParent->page(i);
                if(qobject_cast<ChannelPalette*>(current))
                    static_cast<ChannelPalette*>(current)->changeBackgroundColor(backgroundColor);
                else if(qobject_cast<ItemPalette*>(current))
                    static_cast<ItemPalette*>(current)->changeBackgroundColor(backgroundColor);
            }
            doc->setBackgroundColor(backgroundColor); //will take care of displays
        }
    }

    if(displayPaletteHeaders != configuration().isPaletteHeadersDisplayed()){
        displayPaletteHeaders = configuration().isPaletteHeadersDisplayed();
        if(mainDock){
            for(int i = 0; i < paletteTabsParent->count();++i){
                QWidget *current = paletteTabsParent->widget(i);
                if(displayPaletteHeaders) {
                    QString name = current->name();
                    QString label;
                    if(name.contains("displayPanel"))
                        label = tr("Anatomy");
                    else if(name.contains("spikePanel"))
                        label = tr("Spikes");
                    else if(name.contains("clusterPanel"))
                        label = tr("Units");
                    else if(name.contains("eventPanel"))
                        label = tr("Events");
                    paletteTabsParent->setTabLabel(current,label);
                } else {
                    paletteTabsParent->setTabLabel(current,"");
                }
            }
        } else {
#if KDAB_PENDING
            if(displayPaletteHeaders){
                displayPanel->setWindowTitle("Anatomy");
                spikePanel->setWindowTitle("Spikes");
            }
            else{
                displayPanel->setWindowTitle("");
                spikePanel->setWindowTitle("");
            }
#endif
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

    useWhiteColorDuringPrinting = configuration().getUseWhiteColorDuringPrinting();

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
    useWhiteColorDuringPrinting = configuration().getUseWhiteColorDuringPrinting();
}

void NeuroscopeApp::initDisplay(QList<int>* channelsToDisplay,QList<int> offsets,QList<int> channelGains,
                                QList<int> selectedChannels,QMap<int,bool>& skipStatus,int rasterHeight,long duration,long startTime,QString tabLabel)
{ 
    qDebug()<<" void NeuroscopeApp::initDisplay(QList<int>* channelsToDisplay,QList<int> offsets,QList<int> channelGains,";
    isInit = true; //prevent the spine boxes or the lineedit and the editline to trigger during initialisation
    //Initialize the spinboxe and scrollbar

    //Create the mainDock (first view)
    if(tabLabel.isEmpty())
        tabLabel = tr("Field Potentials Display");

    isInit = false; //now a change in a spine box or the lineedit will trigger an update of the display

    NeuroscopeView* view = new NeuroscopeView(*this,tabLabel,startTime,duration,backgroundColor,Qt::WA_DeleteOnClose,statusBar(),channelsToDisplay,greyScale->isChecked(),
                                              doc->tracesDataProvider(),displayMode->isChecked(),clusterVerticalLines->isChecked(),
                                              clusterRaster->isChecked(),clusterWaveforms->isChecked(),showHideLabels->isChecked(),doc->getGain(),doc->getAcquisitionGain(),
                                              doc->channelColors(),doc->getDisplayGroupsChannels(),doc->getDisplayChannelsGroups(),
                                              offsets,channelGains,selectedChannels,skipStatus,rasterHeight,doc->getTraceBackgroundImage(),mainDock,"TracesDisplay");

    view->installEventFilter(this);

    connect(view,SIGNAL(channelsSelected(QList<int>)),this, SLOT(slotSelectChannelsInPalette(QList<int>)));
    connect(view,SIGNAL(eventModified(QString,int,double,double)),this, SLOT(slotEventModified(QString,int,double,double)));
    connect(view,SIGNAL(eventRemoved(QString,int,double)),this, SLOT(slotEventRemoved(QString,int,double)));
    connect(view,SIGNAL(eventAdded(QString,QString,double)),this, SLOT(slotEventAdded(QString,QString,double)));
    connect(view,SIGNAL(positionViewClosed()),this, SLOT(positionViewClosed()));

    //Keep track of the number of displays
    displayCount ++;

    //Update the document's list of view
    doc->addView(view);
    mainDock = view;


    tabsParent->addDockArea(view,tabLabel);


    //Initialize and dock the displayPanel
    //Create the channel lists and select the channels which will be drawn
    qDebug()<<"displayChannelPalette "<<displayChannelPalette;
    displayChannelPalette->createChannelLists(doc->channelColors(),doc->getDisplayGroupsChannels(),doc->getDisplayChannelsGroups());
    displayChannelPalette->updateShowHideStatus(*channelsToDisplay,true);
    spikeChannelPalette->createChannelLists(doc->channelColors(),doc->getSpikeGroupsChannels(),doc->getChannelsSpikeGroups());
    spikeChannelPalette->updateShowHideStatus(*channelsToDisplay,true);
    displayChannelPalette->setGreyScale(greyScale->isChecked());
    spikeChannelPalette->setGreyScale(greyScale->isChecked());

    //Update the skip status of the channels
    displayChannelPalette->updateSkipStatus(skipStatus);

    //update the channel palettes selection
    if(!selectedChannels.isEmpty()){
        spikeChannelPalette->selectChannels(selectedChannels);
        displayChannelPalette->selectChannels(selectedChannels);
    }



    //Connect the change tab signal to slotPaletteTabChange(QWidget* widget) to trigger updates when
    //the active palette changes.
    connect(paletteTabsParent, SIGNAL(currentChanged(QWidget*)), this, SLOT(slotPaletteTabChange(QWidget*)));


    //Enable some actions now that a document is open (see the klustersui.rc file)
    slotStateChanged("documentState");
    slotStateChanged("displayChannelState");
    if(clusterRaster->isChecked()) {
        slotStateChanged("clusterRasterState");
    }
    else{
        slotStateChanged("noClusterRasterState");
    }
}

void NeuroscopeApp::openDocumentFile(const QString& url)
{
    slotStatusMsg(tr("Opening file..."));
    filePath = url;
    QFileInfo file(filePath);

    if(!file.exists()){
        QString title = tr("File not found: ");
        title.append(filePath);
        int answer = QMessageBox::question(this,title, tr("The selected file no longer exists. Do you want to remove it from the list?"));
        if(answer == QMessageBox::Yes) {
            mFileOpenRecent->removeRecentFile(url);
        }
        else  {
            mFileOpenRecent->addRecentFile(url); //hack, unselect the item
        }
        filePath.clear();
        return;
    }

    //Check if the file exists
    if(!file.exists()){
        QMessageBox::critical (this, tr("Error!"),tr("The selected file does not exist."));
        mFileOpenRecent->removeRecentFile(url);
        return;
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    //If no document is open already, open the document asked.
    if(!mainDock){
        displayCount = 0;
        mFileOpenRecent->addRecentFile(url);

        // Open the file (that will also initialize the document)
        int returnStatus = doc->openDocument(url);
        if(returnStatus == NeuroscopeDoc::INCORRECT_FILE){
            QApplication::restoreOverrideCursor();
            QMessageBox::critical (this, tr("Error!"),tr("The selected file is invalid, it has to be of the form baseName.nrs, baseName.xml or baseName.*"));
            //close the document
            doc->closeDocument();
            resetState();
            return;
        }
        if(returnStatus == NeuroscopeDoc::DOWNLOAD_ERROR){
            QApplication::restoreOverrideCursor();
            QMessageBox::critical (this, tr("Error!"),tr("Could not get the data file."));
            //close the document
            doc->closeDocument();
            resetState();
            return;
        }
        if(returnStatus == NeuroscopeDoc::OPEN_ERROR){
            QApplication::restoreOverrideCursor();
            QMessageBox::critical (this, tr("Error!"),tr("Could not open the files."));
            //close the document
            doc->closeDocument();
            resetState();
            return;
        }
        if(returnStatus == NeuroscopeDoc::PARSE_ERROR){
            QApplication::restoreOverrideCursor();
            QMessageBox::critical(this, tr("IO Error!"),tr("Either the parameter file or the session file could not be parsed correctly."));
            //close the document
            doc->closeDocument();
            resetState();
            return;
        }
        if(returnStatus == NeuroscopeDoc::MISSING_FILE){
            QApplication::restoreOverrideCursor();
            QMessageBox::critical(this, tr("IO Error!"),tr("The parameter file is missing."));
            //close the document
            doc->closeDocument();
            resetState();
            return;
        }
        


        //update the spike and event browsing status
        updateBrowsingStatus();

        //Update the menu related to the display of events in the PositionView
        if(isPositionFileLoaded && !eventFileList.isEmpty()){
            slotStateChanged("eventsInPositionViewEnableState");
        }

        setWindowTitle(url);
        QApplication::restoreOverrideCursor();
    }
    // check, if this document is already open. If yes, do not do anything
    else{
        QString path = doc->url();

        if(path == url){
            mFileOpenRecent->addRecentFile(url); //hack, unselect the item
            QApplication::restoreOverrideCursor();
            return;
        }
        //If the document asked is not the already open. Open a new instance of the application with it.
        else{
            mFileOpenRecent->addRecentFile(url);
            filePath = path;

            QProcess::startDetached("neuroscope", QStringList()<<url);
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
            QWidget* current = paletteTabsParent->page(i);
            QString name = current->name();
            if(qobject_cast<ItemPalette*>(current) && name.contains("clusterPanel")){
                palette = static_cast<ItemPalette*>(current);
                break;
            }
        }

        NeuroscopeView* view = activeView();
        QStringList::iterator iterator;
        for(iterator = clusterFileList.begin(); iterator != clusterFileList.end(); ++iterator){
            const QList<int>* selectedClusters = view->getSelectedClusters(*iterator);
            const QList<int>* skippedClusterIds = view->getClustersNotUsedForBrowsing(*iterator);
            palette->selectItems(*iterator,*selectedClusters,*skippedClusterIds);
        }

        if(!palette->isBrowsingEnable()) {
            slotStateChanged("noClusterBrowsingState");
        }
        else {
            slotStateChanged("clusterBrowsingState");
        }
    }
    if(!eventFileList.isEmpty()){
        ItemPalette* palette;
        for(int i = 0; i<paletteTabsParent->count();i++){
            QWidget* current = paletteTabsParent->page(i);
            QString name = current->name();
            if(qobject_cast<ItemPalette*>(current) && name.contains("eventPanel")){
                palette = static_cast<ItemPalette*>(current);
                break;
            }
        }

        NeuroscopeView* view = activeView();
        QStringList::iterator iterator;
        for(iterator = eventFileList.begin(); iterator != eventFileList.end(); ++iterator){
            const QList<int>* selectedEvents = view->getSelectedEvents(*iterator);
            const QList<int>* skippedEventIds = view->getEventsNotUsedForBrowsing(*iterator);
            palette->selectItems(*iterator,*selectedEvents,*skippedEventIds);
        }

        if(!palette->isBrowsingEnable()){
            slotStateChanged("noEventBrowsingState");
        }
        else{
            slotStateChanged("eventBrowsingState");
        }
    }
}



void NeuroscopeApp::slotGroupsModified(){
    groupsModified = true;

    NeuroscopeView* view = activeView();
    doc->groupsModified(view);
}


bool NeuroscopeApp::queryClose()
{

    //call when the kDockMainWindow will be close
    if(doc == 0 || !doc->isADocumentToClose())  {
        return true;
    } else {
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
                    message = tr("Your configuration has changed, do you want to save it?");
                    title = tr("Configuration modification");
                }
                else if(eventsModified && !groupsModified && !colorModified){
                    message = tr("Some events have changed, do you want to save the event file(s)?");
                    title = tr("Event modification");
                }
                else if((groupsModified || colorModified) && eventsModified){
                    message = tr("Your configuration and some events have changed, do you want to save the configuration and the event file(s)?");
                    title = tr("Modification");
                }
                switch(QMessageBox::question(0,title,message,QMessageBox::Cancel|QMessageBox::Discard|QMessageBox::Save)){
                case QMessageBox::Save://<=> Save
                    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                    if(eventsModified){
                        eventSaveStatus = doc->saveEventFiles();
                        QApplication::restoreOverrideCursor();
                        if(eventSaveStatus != IO_Ok){
                            switch(QMessageBox::warning(0, tr("I/O Error !"),tr("The event file(s) could not be saved possibly because of insufficient file access permissions."
                                                                                "Close anyway ?"),QMessageBox::Close|QMessageBox::Discard)){
                            case QMessageBox::Close:
                                break;
                            case QMessageBox::Discard:
                                return false;
                            }
                        }
                    }//eventsModified
                    //save session
                    saveStatus = doc->saveSession();
                    QApplication::restoreOverrideCursor();
                    if(saveStatus != NeuroscopeDoc::OK){
                        if(saveStatus == NeuroscopeDoc::CREATION_ERROR){
                            message = tr("The current session could not be saved possibly because of insufficient file access permissions."
                                         " You may consider saving your session file to another location using the Save As entry in the File menu.\n"
                                         "Close anyway ?");
                            title = tr("I/O Error !");
                        }
                        else if(saveStatus == NeuroscopeDoc::PARSE_ERROR){
                            message = tr("The current session could not be saved because the parameter file is incorrect.\nClose anyway ?");
                            title = tr("Parsing error !");
                        }
                        else if(saveStatus == NeuroscopeDoc::NOT_WRITABLE){
                            message = tr("The current session could not be saved because the parameter file is not writable.\nClose anyway ?");
                            title = tr("Writing error !");
                        }
                        switch(QMessageBox::question(0,title,message,QMessageBox::Close|QMessageBox::Discard)){
                        case QMessageBox::Close:
                            break;
                        case QMessageBox::Discard:
                            return false;
                        }
                    }
                    break;
                case QMessageBox::Discard://<=> Discard
                    break;
                case QMessageBox::Cancel:
                    return false;
                }
            } else {
                //If neither groups, colors or events have been modified, save the session without asking
                saveStatus = doc->saveSession();
                QString message;
                QString title;
                QApplication::restoreOverrideCursor();
                if(saveStatus != NeuroscopeDoc::OK){
                    if(saveStatus == NeuroscopeDoc::CREATION_ERROR){
                        message = tr("The current session could not be saved possibly because of insufficient file access permissions."
                                     " You may consider saving your session file to another location using the Save As entry in the File menu.\n"
                                     "Close anyway ?");
                        title = tr("I/O Error !");
                    }
                    else if(saveStatus == NeuroscopeDoc::PARSE_ERROR){
                        message = tr("The current session could not be saved because the parameter file is incorrect.\nClose anyway ?");
                        title = tr("Parsing error !");
                    }
                    else if(saveStatus == NeuroscopeDoc::NOT_WRITABLE){
                        message = tr("The current session could not be saved because the parameter file is not writable.\nClose anyway ?");
                        title = tr("Writing error !");
                    }
                    switch(QMessageBox::question(0, title,message,QMessageBox::Close|QMessageBox::Discard)){
                    case QMessageBox::Close:
                        break;
                    case QMessageBox::Discard:
                        return false;
                    }
                }
            }
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            doc->closeDocument();
            QApplication::restoreOverrideCursor();
            return true;
        } else {
            return false;
        }
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

    const QString url=QFileDialog::getOpenFileName(this, tr("Open File..."),QString(),
                                             tr("*.dat *.eeg *.fil|Data File (*.dat), EEG File (*.eeg), Filter File (*.fil)\n*.dat|Data File (*.dat)\n*.eeg|EEG File (*.eeg)\n*.fil|Filter File (*.fil)\n*|All files") );
    if(!url.isEmpty())
    {
        openDocumentFile(url);
    }

    slotStatusMsg(tr("Ready."));
}

void NeuroscopeApp::slotLoadClusterFiles(){
    slotStatusMsg(tr("Loading cluster file(s)..."));

    const QStringList urls=QFileDialog::getOpenFileNames(this, tr("Open Cluster Files..."),QString(),
                                                   tr("*.clu.*|Cluster File (*.clu.n)\n*.clu|Cluster File (*.clu)"));
    if(!urls.isEmpty())
    {
        loadClusterFiles(urls);
    }

    slotStatusMsg(tr("Ready."));
}


void NeuroscopeApp::slotLoadEventFiles(){
    slotStatusMsg(tr("Loading event file(s)..."));

    const QStringList urls=QFileDialog::getOpenFileNames(this, tr("Open Event Files..."),QString(),
                                                   tr("*.evt *.evt.*|Event File (*.evt, *.evt.*)"));
    if(!urls.isEmpty())
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
    QString baseName = doc->documentBaseName();
    QString eventUrl = docUrl  +QDir::separator() + baseName;

    QFileDialog dialog(this,tr("CreateEvent"),eventUrl,tr("*.evt *.evt.*|Event file (*.evt, *.evt.*)"));
    dialog.setWindowTitle(tr("Create Event File as..."));
    if(!dialog.exec())
        return;

    const QString url = dialog.selectedFiles().first();

    if(!url.isEmpty()){
        //Check if the file already exist
        QFileInfo fileInfo(url);
        if(fileInfo.exists()){
            QMessageBox::critical(this, tr("Error!"),tr("The selected file already exist."));
            return;
        }

        NeuroscopeView* view = activeView();
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        int returnStatus = doc->createEventFile(url,view);

        if(returnStatus == NeuroscopeDoc::INCORRECT_FILE){
            QApplication::restoreOverrideCursor();
            QMessageBox::critical (this, tr("Error!"),tr("The selected file name is invalid, it has to be of the form baseName.id.evt or baseName.evt.id (with id a 3 character identifier)."));
        }
        else if(returnStatus == NeuroscopeDoc::ALREADY_OPENED){
            QApplication::restoreOverrideCursor();
            QMessageBox::critical (this, tr("Error!"),tr("The selected file name is already opened."));
        }
        else{
            QString eventFileId = doc->lastLoadedProviderName();
            if(eventFileList.isEmpty())
                createEventPalette(eventFileId);
            else
                addEventFile(eventFileId);
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
                    message = tr("Your configuration has changed, do you want to save it?");
                    title = tr("Configuration modification");
                }
                else if(eventsModified && !groupsModified && !colorModified){
                    message = tr("Some events have changed, do you want to save the event file(s)?");
                    title = tr("Event modification");
                }
                else if((groupsModified || colorModified) && eventsModified){
                    message = tr("Your configuration and some events have changed, do you want to save the configuration and the event file(s)?");
                    title = tr("Modification");
                }
                switch(QMessageBox::question(0,title,message,QMessageBox::Save|QMessageBox::Cancel|QMessageBox::Discard)){
                case QMessageBox::Save://<=> Save
                    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                    if(eventsModified){
                        eventSaveStatus = doc->saveEventFiles();
                        QApplication::restoreOverrideCursor();
                        if(eventSaveStatus != IO_Ok){
                            switch(QMessageBox::question(this, tr("I/O Error !"),tr("The event file(s) could not be saved possibly because of insufficient file access permissions."
                                                                                    "Close anyway ?"),QMessageBox::Yes|QMessageBox::No)){
                            case QMessageBox::Yes:
                                break;
                            case QMessageBox::No:
                                return;
                            }
                        }
                    }//eventsModified
                    //Save session
                    saveStatus = doc->saveSession();
                    QApplication::restoreOverrideCursor();
                    if(saveStatus != NeuroscopeDoc::OK){
                        if(saveStatus == NeuroscopeDoc::CREATION_ERROR){
                            message = tr("The current session could not be saved possibly because of insufficient file access permissions."
                                    " You may consider saving your session file to another location using the Save As entry in the File menu.\n"
                                    "Close anyway ?");
                            title = tr("I/O Error !");
                        }
                        else if(saveStatus == NeuroscopeDoc::PARSE_ERROR){
                            message = tr("The current session could not be saved because the parameter file is incorrect.\nClose anyway ?");
                            title = tr("Parsing error !");
                        }
                        else if(saveStatus == NeuroscopeDoc::NOT_WRITABLE){
                            message = tr("The current session could not be saved because the parameter file is not writable.\nClose anyway ?");
                            title = tr("Writing error !");
                        }
                        switch(QMessageBox::question(0,title,message,QMessageBox::Close|QMessageBox::Discard)){
                        case QMessageBox::Close:
                            break;
                        case QMessageBox::Discard:
                            return;
                        }
                    }
                    break;
                case QMessageBox::Discard://<=> Discard
                    break;
                case QMessageBox::Cancel:
                    return;
                }
            } else {
                //If neither groups, colors or events have been modified, save the session without asking
                saveStatus = doc->saveSession();
                QString message;
                QString title;
                QApplication::restoreOverrideCursor();
                if(saveStatus != NeuroscopeDoc::OK){
                    if(saveStatus == NeuroscopeDoc::CREATION_ERROR){
                        message = tr("The current session could not be saved possibly because of insufficient file access permissions."
                                     " You may consider saving your session file to another location using the Save As entry in the File menu.\n"
                                     "Close anyway ?");
                        title = tr("I/O Error !");
                    }
                    else if(saveStatus == NeuroscopeDoc::PARSE_ERROR){
                        message = tr("The current session could not be saved because the parameter file is incorrect.\nClose anyway ?");
                        title = tr("Parsing error !");
                    }
                    else if(saveStatus == NeuroscopeDoc::NOT_WRITABLE){
                        message = tr("The current session could not be saved because the parameter file is not writable.\nClose anyway ?");
                        title = tr("Writing error !");
                    }
                    switch(QMessageBox::question(this, title,message,QMessageBox::Close|QMessageBox::Discard)){
                    case QMessageBox::Close:
                        break;
                    case QMessageBox::Discard:
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
                    DockArea* current = static_cast<DockArea*>(tabsParent->page(0));
                    if(current == mainDock){
                        current = static_cast<DockArea*>(tabsParent->page(1));
                    }
                    tabsParent->removePage(current);
                    delete current;
                    if(nbOfTabs == 2)
                        break; //the reminding one is the mainDock
                }
            }

            //remove the cluster and event palettes if any
            while(paletteTabsParent->count() > 2){
                QWidget* current = paletteTabsParent->page(0);
                if(qobject_cast<ChannelPalette*>(current)){
                    current = paletteTabsParent->page(1);
                    if(qobject_cast<ChannelPalette*>(current))
                        current = paletteTabsParent->page(2);
                }
                paletteTabsParent->removePage(current);
                delete current;
            }

            //reset the channel palettes and hide the channel panels
            spikeChannelPalette->reset();
            displayChannelPalette->reset();
            //palettePanel->hide();
            //KDAD_PENDING palettePanel->undock();

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

    if (printer->setup(this))
    {
        //retrieve the backgroundColor setting from KPrinter object, //1 <=> white background
        NeuroscopeView* view = activeView();
        doc->updateSkippedChannelColors(true,backgroundColor);
        if(useWhiteColorDuringPrinting) {
            //update the color of the skipped channels to white
            doc->updateSkippedChannelColors(true,backgroundColor);
            view->print(printer,filePath,true);
            //update the color of the skipped channels to the background color
            doc->updateSkippedChannelColors(false,backgroundColor);
        } else {
            view->print(printer,filePath,false);
        }
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

    mMainToolBar->setVisible(viewMainToolBar->isChecked());

    slotStatusMsg(tr("Ready."));
}

void NeuroscopeApp::slotViewToolBar()
{
    slotStatusMsg(tr("Toggle the tools..."));

    mToolBar->setVisible(viewToolBar->isChecked());
    slotStatusMsg(tr("Ready."));
}

void NeuroscopeApp::slotViewStatusBar()
{
    slotStatusMsg(tr("Toggle the statusbar..."));
    ///////////////////////////////////////////////////////////////////
    //turn Statusbar on or off
    statusBar()->setVisible(mViewStatusBar->isChecked());
    slotStatusMsg(tr("Ready."));
}

void NeuroscopeApp::slotShowCalibration(){
    NeuroscopeView* view = activeView();
    doc->showCalibration(calibrationBar->isChecked(),view);
}


void NeuroscopeApp::slotShowPositionView(){
    NeuroscopeView* view = activeView();
    if(!positionViewToggle->isChecked())
    {
        view->removePositionView();
    }
    else
    {
        doc->addPositionView(view,backgroundColor);
    }
}

void NeuroscopeApp::slotStatusMsg(const QString &text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message permanently
    statusBar()->clear();
    statusBar()->showMessage(text);
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


    QWidget *current = paletteTabsParent->currentWidget();

    if(qobject_cast<ChannelPalette*>(current)){
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

    DockArea* area = tabsParent->currentDockArea();
    NeuroscopeView *view = static_cast<NeuroscopeView*>(area);
    return view;
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

void NeuroscopeApp::slotUpdateShownChannels(const QList<int>& shownChannels){

    QWidget *current = paletteTabsParent->currentWidget();
    ChannelPalette* channelPalette = static_cast<ChannelPalette*>(current);


    NeuroscopeView* view = activeView();
    view->shownChannelsUpdate(shownChannels);

    //if the view is in selection mode, highlight the selected channels
    if(view->isSelectionTool())
        view->selectChannels(channelPalette->selectedChannels());
    else
        view->setSelectedChannels(channelPalette->selectedChannels());

    //Update the show/hide status of the inactive palette.
    if(channelPalette == displayChannelPalette)
        spikeChannelPalette->updateShowHideStatus(shownChannels,true);
    else
        displayChannelPalette->updateShowHideStatus(shownChannels,true);
}

void NeuroscopeApp::slotUpdateHiddenChannels(const QList<int>& hiddenChannels){
    //Update the show/hide status of the inactive palette
    QWidget *current = paletteTabsParent->currentWidget();
    ChannelPalette* channelPalette = static_cast<ChannelPalette*>(current);

    if(channelPalette == displayChannelPalette)
        spikeChannelPalette->updateShowHideStatus(hiddenChannels,false);
    else
        displayChannelPalette->updateShowHideStatus(hiddenChannels,false);
}

void NeuroscopeApp::slotFileProperties(){
    if(propertiesDialog == 0L)
        propertiesDialog = new PropertiesDialog(this);

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
    if(propertiesDialog == 0L)
        propertiesDialog = new PropertiesDialog(this);

    //enable the tabs for cluster and positions if a corresponding file is open, otherwise disable them.
    if(isPositionFileLoaded)
        propertiesDialog->setEnabledPosition(true);
    else
        propertiesDialog->setEnabledPosition(false);

    if(clusterFileList.isEmpty() || !doc->isCurrentFileAdatFile())
        propertiesDialog->setEnabledCluster(false);
    else
        propertiesDialog->setEnabledCluster(true);

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
    filePath.clear();
    clusterFileList.clear();
    eventFileList.clear();
    eventLabelToCreate.clear();
    eventProvider.clear();
    currentNbUndo = 0;
    currentNbRedo = 0;


    //Disable some actions when no document is open (see the klustersui.rc file)
    slotStateChanged("initState");
    setWindowTitle(QString());
}

void NeuroscopeApp::slotDefaultSetUp(QMap<int,int>& channelDefaultOffsets,QMap<int,bool>& skipStatus){
    //All the channels will be selected and none skipped
    QList<int>* channelsToDisplay = new QList<int>();
    int channelNb = doc->getChannelNb();
    for(int i = 0 ; i < channelNb; ++i){
        channelsToDisplay->append(i);
    }

    QList<int> offsets = channelDefaultOffsets.values();
    QList<int> channelGains;
    QList<int> selectedChannels;
    if(initialTimeWindow != 0)
        initDisplay(channelsToDisplay,offsets,channelGains,selectedChannels,skipStatus,initialTimeWindow);
    else
        initDisplay(channelsToDisplay,offsets,channelGains,selectedChannels,skipStatus);

}

void NeuroscopeApp::slotSetUp(QList<int>* channelsToDisplay,bool verticalLines,bool raster,bool waveforms,bool showLabels,bool multipleColumns,
                              bool greyMode,QList<int> offsets,QList<int> channelGains,QList<int> selectedChannels,
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
    QWidget* current = paletteTabsParent->currentPage();
    ChannelPalette* channelPalette = static_cast<ChannelPalette*>(current);
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
    QWidget *current = paletteTabsParent->currentWidget();
    if(qobject_cast<ChannelPalette*>(current)){
        //update the 2 palettes
        spikeChannelPalette->selectAllChannels();
        displayChannelPalette->selectAllChannels();
        NeuroscopeView* view = activeView();
        if(editMode->isChecked()) doc->selectAllChannels(*view,true);
        else doc->selectAllChannels(*view,false);
    }
    else{
        //Update the selected items of the current palette
        if(qobject_cast<ItemPalette*>(current)){
            QString name = current->name();
            //update the cluster palette
            if(name.contains("clusterPanel")){
                ItemPalette* clusterPalette = static_cast<ItemPalette*>(current);
                NeuroscopeView* view = activeView();
                QList<int> clustersToHide;
                doc->showAllClustersExcept(clusterPalette,view,clustersToHide);
            }
            //update the event palette
            if(name.contains("eventPanel")){
                ItemPalette* eventPalette = static_cast<ItemPalette*>(current);
                NeuroscopeView* view = activeView();
                doc->showAllEvents(eventPalette,view);
            }
            //update the spike palette

        }
    }
}

void NeuroscopeApp::slotDeselectAll(){

    QWidget *current = paletteTabsParent->currentWidget();

    if(qobject_cast<ChannelPalette*>(current)){
        //update the 2 palettes
        spikeChannelPalette->deselectAllChannels();
        displayChannelPalette->deselectAllChannels();
        NeuroscopeView* view = activeView();
        if(editMode->isChecked()) doc->deselectAllChannels(*view,true);
        else doc->deselectAllChannels(*view,false);
    }
    else{
        //Update the selected items of the current palette
        if(qobject_cast<ItemPalette*>(current)){
            QString name = current->name();
            //update the cluster palette
            if(name.contains("clusterPanel")){
                ItemPalette* clusterPalette = static_cast<ItemPalette*>(current);
                NeuroscopeView* view = activeView();
                doc->deselectAllClusters(clusterPalette,view);
                slotStateChanged("noClusterBrowsingState");
            }
            //update the event palette
            if(name.contains("eventPanel")){
                ItemPalette* eventPalette = static_cast<ItemPalette*>(current);
                NeuroscopeView* view = activeView();
                doc->deselectAllEvents(eventPalette,view);
                slotStateChanged("noEventBrowsingState");
            }
            //update the spike palette

        }
    }
}

void NeuroscopeApp::slotSelectAllWO01(){
    QWidget *current = paletteTabsParent->currentWidget();
    if(qobject_cast<ItemPalette*>(current)){
        QString name = current->name();
        //update the cluster palette
        if(name.contains("clusterPanel")){
            ItemPalette* clusterPalette = static_cast<ItemPalette*>(current);
            NeuroscopeView* view = activeView();
            QList<int> clustersToHide;
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
    if(clusterRaster->isChecked()) {
        slotStateChanged("clusterRasterState");
    }
    else{

        slotStateChanged("noClusterRasterState");
    }
    NeuroscopeView* view = activeView();
    view->setClusterRaster(clusterRaster->isChecked());
}
void NeuroscopeApp::slotClustersWaveforms(){
    NeuroscopeView* view = activeView();
    view->setClusterWaveforms(clusterWaveforms->isChecked());
}

void NeuroscopeApp::slotDiscardChannels(){
    //Get the active palette.
    QWidget *current = paletteTabsParent->currentWidget();
    ChannelPalette* channelPalette = static_cast<ChannelPalette*>(current);
    channelPalette->discardChannels();
}

void NeuroscopeApp::slotDiscardSpikeChannels(){
    spikeChannelPalette->discardSpikeChannels();
    groupsModified = true;
}

void NeuroscopeApp::slotKeepChannels(){
    colorModified = true;
    const QList<int> selectedChannels = displayChannelPalette->selectedChannels();

    //The order in which the palettes are updated matters. The first one will give the new color for the
    //channels which change status
    QWidget *current = paletteTabsParent->currentWidget();
    if(current == displayChannelPalette){
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
    const QList<int> selectedChannels = displayChannelPalette->selectedChannels();
    displayChannelPalette->updateSkipStatus(selectedChannels,true);
    spikeChannelPalette->updateSkipStatus(selectedChannels,true);

    //Update the skipped channel list in the views
    doc->updateSkipStatus();

    //Update the content of the view contains in active display.
    activeView()->updateViewContents();
}


void NeuroscopeApp::slotChannelsDiscarded(const QList<int>& discarded){
    groupsModified = true;

    //Update the inactive palette
    QWidget *current = paletteTabsParent->currentWidget();

    ChannelPalette* channelPalette = static_cast<ChannelPalette*>(current);

    if(channelPalette == displayChannelPalette)
        spikeChannelPalette->discardChannels(discarded);
    else
        displayChannelPalette->discardChannels(discarded);

    NeuroscopeView* view = activeView();
    doc->groupsModified(view);
}

void NeuroscopeApp::slotShowChannels(){
    //Get the active palette if there are 2 or the displayChannelPalette otherwise.
    QWidget *current = paletteTabsParent->currentWidget();
    ChannelPalette* channelPalette = static_cast<ChannelPalette*>(current);
    channelPalette->showChannels();
}

void NeuroscopeApp::slotHideChannels(){
    //Get the active palette if there are 2 or the displayChannelPalette otherwise.

    QWidget *current = paletteTabsParent->currentWidget();

    ChannelPalette* channelPalette = static_cast<ChannelPalette*>(current);
    channelPalette->hideChannels();
}

void NeuroscopeApp::slotTabChange(QWidget* widget){
    NeuroscopeView* activeView = dynamic_cast<NeuroscopeView*>(widget);

    isInit = true; //prevent the KToggleAction to trigger during initialisation

    clusterVerticalLines->setChecked(activeView->getClusterVerticalLines());
    clusterRaster->setChecked(activeView->getClusterRaster());
    if(clusterRaster->isChecked()) {
        slotStateChanged("clusterRasterState");
    }
    else{
        slotStateChanged("noClusterRasterState");
    }
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
    const QList<int> selectedChannels = activeView->getSelectedChannels();

    QWidget *channelPalette = paletteTabsParent->currentWidget();

    if(qobject_cast<ChannelPalette*>(channelPalette)){
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
    QWidget* current = paletteTabsParent->currentPage();

    //Disable some actions when no document is open (see the klustersui.rc file)
    if(current->metaObject()->className() == ("ChannelPalette")){
        if(editMode->isChecked()){
            if((current) == displayChannelPalette){
                slotStateChanged("displayChannelState");
            }
            else{

                slotStateChanged("spikeChannelState");
            }
        }
        else{

            slotStateChanged("enableEditState");
        }

        //update the channel palettes
        NeuroscopeView* view = activeView();
        const QList<int> selectedChannels = view->getSelectedChannels();
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
        if(current->metaObject()->className() == ("ItemPalette")){
            QString name = current->name();
            if(name.contains("clusterPanel")){
                ItemPalette* clusterPalette = static_cast<ItemPalette*>(current);
                NeuroscopeView* view = activeView();
                QStringList::iterator iterator;
                for(iterator = clusterFileList.begin(); iterator != clusterFileList.end(); ++iterator){
                    const QList<int>* selectedClusters = view->getSelectedClusters(*iterator);
                    const QList<int>* skippedClusterIds = view->getClustersNotUsedForBrowsing(*iterator);
                    clusterPalette->selectItems(*iterator,*selectedClusters,*skippedClusterIds);
                }
                slotStateChanged("clusterTabState");

                if(!clusterPalette->isBrowsingEnable()) {
                    slotStateChanged("noClusterBrowsingState");
                }
                else{
                    slotStateChanged("clusterBrowsingState");
                }
            }
            //update the event palettes
            if(name.contains("eventPanel")){
                ItemPalette* eventPalette = static_cast<ItemPalette*>(current);
                NeuroscopeView* view = activeView();
                QStringList::iterator iterator;
                for(iterator = eventFileList.begin(); iterator != eventFileList.end(); ++iterator){
                    const QList<int>* selectedEvents = view->getSelectedEvents(*iterator);
                    const QList<int>* skippedEventIds = view->getEventsNotUsedForBrowsing(*iterator);
                    eventPalette->selectItems(*iterator,*selectedEvents,*skippedEventIds);
                }
                slotStateChanged("eventTabState");
                if(!eventPalette->isBrowsingEnable()){
                    slotStateChanged("noEventBrowsingState");
                }
                else {
                    slotStateChanged("eventBrowsingState");
                }
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
    QDockWidget* current;

    slotStatusMsg(tr("Closing display..."));
#if KDAB_PENDING
    //Get the active tab
    if(tabsParent){
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        int nbOfTabs = tabsParent->count();
        current = static_cast<QDockWidget*>(tabsParent->currentPage());
        //If the active display is the mainDock, assign the mainDock status to an other display (take the first one available)
        if(current == mainDock){
            if(tabsParent->currentPageIndex() == 0){
                mainDock = static_cast<QDockWidget*>(tabsParent->page(1));
                setCentralWidget(mainDock);
            }
            else {
                setCentralWidget(static_cast<QDockWidget*>(tabsParent->page(0)));
            }
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
        NeuroscopeView* view = dynamic_cast<NeuroscopeView*>(current->widget());
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
                    message = tr("Your configuration has changed, do you want to save it?");
                    title = tr("Configuration modification");
                }
                else if(eventsModified && !groupsModified && !colorModified){
                    message = tr("Some events have changed, do you want to save the event file(s)?");
                    title = tr("Event modification");
                }
                else if((groupsModified || colorModified) && eventsModified){
                    message = tr("Your configuration and some events have changed, do you want to save the configuration and the event file(s)?");
                    title = tr("Modification");
                }
                switch(QMessageBox::warning(0,title,message,QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel)){
                case QMessageBox::Save://<=> Save
                    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                    if(eventsModified){
                        eventSaveStatus = doc->saveEventFiles();
                        QApplication::restoreOverrideCursor();
                        if(eventSaveStatus != IO_Ok){
                            switch(QMessageBox::question(0, tr("I/O Error !"),tr("The event file(s) could not be saved possibly because of insufficient file access permissions."	"Close anyway ?"),QMessageBox::Close|QMessageBox::Discard)){
                            case QMessageBox::Close:
                                break;
                            case QMessageBox::Discard:
                                return;
                            }
                        }
                    }//eventsModified
                    //Save session
                    saveStatus = doc->saveSession();
                    QApplication::restoreOverrideCursor();
                    if(saveStatus != NeuroscopeDoc::OK){
                        if(saveStatus == NeuroscopeDoc::CREATION_ERROR){
                            message = tr("The current session could not be saved possibly because of insufficient file access permissions."
                                         " You may consider saving your session file to another location using the Save As entry in the File menu.\n"
                                         "Close anyway ?");
                            title = tr("I/O Error !");
                        }
                        else if(saveStatus == NeuroscopeDoc::PARSE_ERROR){
                            message = tr("The current session could not be saved because the parameter file is incorrect.\nClose anyway ?");
                            title = tr("Parsing error !");
                        }
                        else if(saveStatus == NeuroscopeDoc::NOT_WRITABLE){
                            message = tr("The current session could not be saved because the parameter file is not writable.\nClose anyway ?");
                            title = tr("Writing error !");
                        }
                        switch(QMessageBox::question(this,message, title,QMessageBox::Close|QMessageBox::Discard)){
                        case QMessageBox::Close:
                            break;
                        case QMessageBox::Discard:
                            return;
                        }
                    }
                    break;
                case QMessageBox::Discard://<=> Discard
                    break;
                case QMessageBox::Cancel:
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
                        message = tr("The current session could not be saved possibly because of insufficient file access permissions."
                                     " You may consider saving your session file to another location using the Save As entry in the File menu.\n"
                                     "Close anyway ?");
                        title = tr("I/O Error !");
                    }
                    else if(saveStatus == NeuroscopeDoc::PARSE_ERROR){
                        message = tr("The current session could not be saved because the parameter file is incorrect.\nClose anyway ?");
                        title = tr("Parsing error !");
                    }
                    else if(saveStatus == NeuroscopeDoc::NOT_WRITABLE){
                        message = tr("The current session could not be saved because the parameter file is not writable.\nClose anyway ?");
                        title = tr("Writing error !");
                    }
                    switch(QMessageBox::question(0,title,message,QMessageBox::Close|QMessageBox::Discard)){
                    case QMessageBox::Close:
                        break;
                    case QMessageBox::Discard:
                        return;
                    }
                }
            }
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

            //remove the cluster and event palettes if any
            while(paletteTabsParent->count() > 2){
                QDockWidget* current = static_cast<QDockWidget*>(paletteTabsParent->page(0));
                if((current->widget())->metaObject()->className() == ("ChannelPalette")){
                    current = static_cast<QDockWidget*>(paletteTabsParent->page(1));
                    if((current->widget())->metaObject()->className() == ("ChannelPalette"))
                        current = static_cast<QDockWidget*>(paletteTabsParent->page(2));
                }
                paletteTabsParent->removePage(current);
                delete current;
            }

            //reset the channel palettes and hide the channel panels
            spikeChannelPalette->reset();
            displayChannelPalette->reset();
            //palettePanel->hide();

            doc->closeDocument();
            //Delete the view
            delete mainDock;
            mainDock = 0L;
            resetState();
            QApplication::restoreOverrideCursor();
        }
    }
#endif
    slotStatusMsg(tr("Ready."));
}

void NeuroscopeApp::slotRenameActiveDisplay(){
    if(tabsParent){
        const int index = tabsParent->currentIndex();

        bool ok;
        const QString newLabel = QInputDialog::getText(tr("New Display label"),tr("Type in the new display label"),QLineEdit::Normal,
                                                 tabsParent->tabText(index),&ok,this);
        if(ok&& !newLabel.isEmpty()){
            tabsParent->setTabText(index,newLabel);
            activeView()->setTabName(newLabel);
        }
    }
}

void NeuroscopeApp::slotNewDisplay(){
    //Present the channels of the current display in the new display.
    QList<int>* channelList = new QList<int>();
    const QList<int>& currentChannels = activeView()->channels();
    QList<int>::const_iterator iterator;
    for(iterator = currentChannels.begin(); iterator != currentChannels.end(); ++iterator)
        channelList->append(*iterator);

    QList<int> offsets = activeView()->getChannelOffset();
    QList<int> channelGains = activeView()->getGains();
    QList<int> selectedChannels = activeView()->getSelectedChannels();
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

void NeuroscopeApp::createDisplay(QList<int>* channelsToDisplay,bool verticalLines,bool raster,bool waveforms,bool showLabels,bool multipleColumns,bool greyMode,
                                  QList<int> offsets,QList<int> channelGains,QList<int> selectedChannels,long startTime,long duration,int rasterHeight, QString tabLabel){
    if(mainDock){
        if(tabLabel.isEmpty())
            tabLabel = tr("Field Potentials Display");

        NeuroscopeView* view = new NeuroscopeView(*this,tabLabel,startTime,duration,backgroundColor,Qt::WA_DeleteOnClose,statusBar(),channelsToDisplay,
                                                  greyMode,doc->tracesDataProvider(),multipleColumns,verticalLines,raster,waveforms,showLabels,
                                                  doc->getGain(),doc->getAcquisitionGain(),doc->channelColors(),doc->getDisplayGroupsChannels(),doc->getDisplayChannelsGroups(),
                                                  offsets,channelGains,selectedChannels,displayChannelPalette->getSkipStatus(),rasterHeight,doc->getTraceBackgroundImage(),mainDock,
                                                  "TracesDisplay");

        tabsParent->addDockArea(view,tabLabel);
        view->installEventFilter(this);

        connect(view,SIGNAL(channelsSelected(QList<int>)),this, SLOT(slotSelectChannelsInPalette(QList<int>)));
        connect(view,SIGNAL(eventModified(QString,int,double,double)),this, SLOT(slotEventModified(QString,int,double,double)));
        connect(view,SIGNAL(eventRemoved(QString,int,double)),this, SLOT(slotEventRemoved(QString,int,double)));
        connect(view,SIGNAL(eventAdded(QString,QString,double)),this, SLOT(slotEventAdded(QString,QString,double)));
        connect(view,SIGNAL(positionViewClosed()),this, SLOT(positionViewClosed()));

        view->installEventFilter(this);

        //Update the document's list of view
        doc->addView(view);

        disconnect(tabsParent,0,0,0);

        //Connect the change tab signal to slotTabChange(QWidget* widget) to trigger updates when
        //the active display change.
        connect(tabsParent, SIGNAL(currentChanged(QWidget*)), this, SLOT(slotTabChange(QWidget*)));

        slotStateChanged("tabState");

        //Keep track of the number of displays
        displayCount ++;


        //Show the calibration bars if need it
        view->showCalibration(calibrationBar->isChecked(),false);
    }
}

void NeuroscopeApp::slotEditMode(){
    spikeChannelPalette->setEditMode(editMode->isChecked());
    displayChannelPalette->setEditMode(editMode->isChecked());

    QWidget *channelPalette = paletteTabsParent->currentWidget();

    if(editMode->isChecked()){
        slotStateChanged("editState");
        if(channelPalette == displayChannelPalette){
            slotStateChanged("displayChannelState");
        }
        else{
            slotStateChanged("spikeChannelState");
        }
    }
    else{
        slotStateChanged("noEditState");
        NeuroscopeView* view = activeView();
        doc->setNoneEditMode(view);
        select = false;

        if(qobject_cast<ChannelPalette*>(channelPalette)){
            //the 2 palettes have the same selected channels
            displayChannelPalette->selectionTool();
        }
    }
}

void NeuroscopeApp::slotSynchronize(){
    if(QMessageBox::warning(this,tr("Synchronize anatomic and spike groups"),tr("This will overwrite all your spike groups by your anatomical groups. Do you wish to continue?"), QMessageBox::Ok|QMessageBox::Cancel
                            ) == QMessageBox::Cancel)
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


    slotStateChanged("displayChannelState");

}

void NeuroscopeApp::resizePalettePanel(){
#if KDAB_PENDING
    static_cast<QWidget*>(displayPanel)->resize(static_cast<QWidget*>(displayPanel->parent())->size());
#endif
    displayChannelPalette->update();
}

void NeuroscopeApp::slotSelectChannelsInPalette(const QList<int>& selectedIds){
    spikeChannelPalette->selectChannels(selectedIds);
    displayChannelPalette->selectChannels(selectedIds);
}


void NeuroscopeApp::slotChannelsSelected(const QList<int>& selectedIds){
    //if the selection tool is selected  warn the active display
    if(select)
        activeView()->selectChannels(selectedIds);
    else
        activeView()->setSelectedChannels(selectedIds);

    QWidget *channelPalette = paletteTabsParent->currentWidget();

    //Update the selection of the inactive palette.
    if(channelPalette == displayChannelPalette)
        spikeChannelPalette->selectChannels(selectedIds);
    else
        displayChannelPalette->selectChannels(selectedIds);
}

void NeuroscopeApp::slotIncreaseSelectedChannelsAmplitude(){
    //Get the active palette if any.
    QWidget *current = paletteTabsParent->currentWidget();

    ChannelPalette* channelPalette = 0;
    if( channelPalette = qobject_cast<ChannelPalette*>(current)){
        activeView()->increaseSelectedChannelsAmplitude(channelPalette->selectedChannels());
    }
    else
        //The 2 palettes have the same selection (selected from the view)
        activeView()->increaseSelectedChannelsAmplitude(displayChannelPalette->selectedChannels());
}


void NeuroscopeApp::slotDecreaseSelectedChannelsAmplitude(){
    //Get the active palette if any.
    QWidget *current = paletteTabsParent->currentWidget();

    ChannelPalette* channelPalette = 0;
    if( channelPalette = qobject_cast<ChannelPalette*>(current)){
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
            QMessageBox::critical(0,tr("I/O Error !"),tr("The event file(s) could not be saved possibly because of insufficient file access permissions."));
        else eventsModified = false;
    }
    //Save the session
    int saveStatus = doc->saveSession();
    if(saveStatus == NeuroscopeDoc::CREATION_ERROR){
        QMessageBox::critical(0, tr("I/O Error !"),tr("The current session could not be saved possibly because of insufficient file access permissions."
                                                      " You may consider saving your session file to another location using the Save As entry in the File menu."));
    }
    else if(saveStatus == NeuroscopeDoc::PARSE_ERROR){
        QMessageBox::critical(0, tr("Parsing error !"),tr("The current session could not be saved because the parameter file is incorrect."));
    }
    else if(saveStatus == NeuroscopeDoc::NOT_WRITABLE){
        QMessageBox::critical(0, tr("Writing error !"),tr("The current session could not be saved because the parameter file is not writable.\nClose anyway ?"));
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
            QMessageBox::critical(0,tr("I/O Error !"),tr("The event file(s) could not be saved possibly because of insufficient file access permissions.")
                                  );
        else eventsModified = false;
    }
    //Save the session
    QString url=QFileDialog::getSaveFileName(this, tr("Save as..."),doc->sessionPath(),
                                             tr("*|All files") );
    if(!url.isEmpty()){
        int saveStatus = doc->saveSession(url);
        if(saveStatus == NeuroscopeDoc::CREATION_ERROR){
            QMessageBox::critical(0, tr("I/O Error !"),tr("The current session could not be saved possibly because of insufficient file access permissions."));
        }
        else if(saveStatus == NeuroscopeDoc::PARSE_ERROR){
            QMessageBox::critical(0, tr("Parsing error !"),tr("The current session could not be saved because the parameter file is incorrect."));
        }
        else if(saveStatus == NeuroscopeDoc::NOT_WRITABLE){
            QMessageBox::critical(0, tr("Writing error !"),tr("The current session could not be saved because the parameter file is not writable.\nClose anyway ?"));
        }
        else{
            groupsModified = false;
            colorModified = false;
        }
    }
}

void NeuroscopeApp::customEvent (QEvent* event){
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

void NeuroscopeApp::loadClusterFiles(const QStringList &urls){

    NeuroscopeView* view = activeView();
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    //Loop on the files
    int counter = 0;
    QStringList::const_iterator iterator;
    for(iterator = urls.constBegin();iterator != urls.constEnd();++iterator){
        //Create the provider
        int returnStatus = doc->loadClusterFile(*iterator,view);
        if(returnStatus == NeuroscopeDoc::OPEN_ERROR){
            QApplication::restoreOverrideCursor();
            QMessageBox::critical (this, tr("Error!"),tr("Could not load the file %1").arg(*iterator));
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            continue;
        }
        else if(returnStatus == NeuroscopeDoc::INCORRECT_FILE){
            QApplication::restoreOverrideCursor();
            QMessageBox::critical (this, tr("Error!"),
                                   tr("Incorrect file name (%1): the name has to be of the form baseName.n.clu or baseName.clu.n (with n a number identifier).").arg(*iterator));
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            continue;
        }
        else if(returnStatus == NeuroscopeDoc::MISSING_FILE){
            QApplication::restoreOverrideCursor();
            QMessageBox::critical (this, tr("Error!"),tr("There is no time file (.res) corresponding to the requested file %1").arg(*iterator));
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            continue;
        }
        else if(returnStatus == NeuroscopeDoc::INCORRECT_CONTENT){
            QApplication::restoreOverrideCursor();
            QMessageBox::critical (this, tr("Error!"),tr("The number of spikes read in the requested file (%1) or the corresponding time file (.res) does not correspond to number of spikes computed.").arg(*iterator));
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            continue;
        }
        else if(returnStatus == NeuroscopeDoc::CREATION_ERROR){
            QApplication::restoreOverrideCursor();
            QMessageBox::critical (this, tr("Error!"),tr("The number of spikes of the requested file (%1) could not be determined.").arg(*iterator));
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            continue;
        }
        else if(returnStatus == NeuroscopeDoc::ALREADY_OPENED){
            QApplication::restoreOverrideCursor();
            QMessageBox::critical (this, tr("Error!"),tr("The requested file (%1) is already loaded.").arg(*iterator));
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

void NeuroscopeApp::createClusterPalette(const QString& clusterFileId){

    ItemPalette* clusterPalette = new ItemPalette(ItemPalette::CLUSTER,backgroundColor,this,"units");
    if(displayPaletteHeaders) {
        int index = paletteTabsParent->addTab(clusterPalette,tr("Units"));
        paletteTabsParent->setTabIcon(index,QIcon(":/icons/clusters"));
    } else {
        int index = paletteTabsParent->addTab(clusterPalette,QString());
        paletteTabsParent->setTabIcon(index,QIcon(":/icons/clusters"));

    }
    clusterFileList.append(clusterFileId);

    //Create the list
    clusterPalette->createItemList(doc->providerColorList(clusterFileId),clusterFileId,0);

    //Disconnect the previous connection
    if(paletteTabsParent != NULL)
        disconnect(paletteTabsParent,0,0,0);
    //Connect the change tab signal to slotPaletteTabChange(QWidget* widget) to trigger updates when
    //the active palette changes.
    connect(paletteTabsParent, SIGNAL(currentChanged(QWidget*)), this, SLOT(slotPaletteTabChange(QWidget*)));

    //Palette connections
    connect(clusterPalette, SIGNAL(colorChanged(int,QString)), this, SLOT(slotClusterColorUpdate(int,QString)));
    connect(clusterPalette, SIGNAL(updateShownItems(QMap<QString,QList<int> >)), this, SLOT(slotUpdateShownClusters(QMap<QString,QList<int> >)));
    connect(clusterPalette, SIGNAL(updateItemsToSkip(QString,QList<int>)), this, SLOT(slotUpdateClustersToSkip(QString,QList<int>)));
    connect(clusterPalette,SIGNAL(noClustersToBrowse()),this, SLOT(slotNoClustersToBrowse()));
    connect(clusterPalette,SIGNAL(clustersToBrowse()),this, SLOT(slotClustersToBrowse()));

    slotStateChanged("clusterState");
    //Waveforms are allowed only for dat and fil files.
    if(filePath.contains(".dat")||filePath.contains(".fil")) {
        slotStateChanged("datState");
    }
    else{

        slotStateChanged("noDatState");
    }
}

void NeuroscopeApp::addClusterFile(const QString& clusterFileId){
    clusterFileList.append(clusterFileId);

    for(int i = 0; i<paletteTabsParent->count();i++){
        QWidget* current = paletteTabsParent->page(i);
        QString name = current->name();
        if(qobject_cast<ItemPalette*>(current) && name.contains("clusterPanel")){
            ItemPalette* clusterPalette = static_cast<ItemPalette*>(current);
            //Create the list
            clusterPalette->createItemList(doc->providerColorList(clusterFileId),clusterFileId,0);
            break;
        }
    }
}


void NeuroscopeApp::slotClusterColorUpdate(int clusterId,QString providerName){  
    QWidget* current = paletteTabsParent->currentPage();
    QString name = current->name();
    if(qobject_cast<ItemPalette*>(current) && name.contains("clusterPanel")){
        NeuroscopeView* view = activeView();
        doc->clusterColorUpdate(providerName,clusterId,view);
    }

}

void NeuroscopeApp::slotUpdateShownClusters(const QMap<QString,QList<int> >& selection){
    QWidget* current = paletteTabsParent->currentPage();
    QString name = current->name();
    if(qobject_cast<ItemPalette*>(current) && name.contains("clusterPanel")){
        QMap<QString,QList<int> >::ConstIterator groupIterator;
        for(groupIterator = selection.begin(); groupIterator != selection.end(); ++groupIterator){
            QString providerName = groupIterator.key();
            QList<int> clusterIds = groupIterator.data();
            NeuroscopeView* view = activeView();
            view->shownClustersUpdate(providerName,clusterIds);
        }
    }
}

void NeuroscopeApp::slotCloseClusterFile(){
    QWidget* current = paletteTabsParent->currentPage();
    QString name = current->name();
    if(qobject_cast<ItemPalette*>(current) && name.contains("clusterPanel")){
        ItemPalette* clusterPalette = static_cast<ItemPalette*>(current);
        QString providerName = clusterPalette->selectedGroup();

        if(!providerName.isEmpty()){
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
                if(!clusterPalette->isBrowsingEnable()) {
                    slotStateChanged("noClusterBrowsingState");
                }
                else{

                    slotStateChanged("clusterBrowsingState");
                }
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


void NeuroscopeApp::loadPositionFile(const QString& url){
    NeuroscopeView* view = activeView();
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    //Create the provider
    int returnStatus = doc->loadPositionFile(url,view);
    if(returnStatus == NeuroscopeDoc::OPEN_ERROR){
        QApplication::restoreOverrideCursor();
        QMessageBox::critical (this, tr("Error !"),tr("Could not load the file %1").arg(url));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    }
    if(returnStatus == NeuroscopeDoc::INCORRECT_FILE){
        QApplication::restoreOverrideCursor();
        QMessageBox::critical (this, tr("Error!"),tr("Incorrect file name (%1): extension missing.").arg(url));

        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    }
    else{
        isPositionFileLoaded = true;
        positionViewToggle->setChecked(true);

        slotStateChanged("positionState");
        if(!eventFileList.isEmpty()){
            slotStateChanged("eventsInPositionViewEnableState");
        }
    }

    QApplication::restoreOverrideCursor();
}

void NeuroscopeApp::loadEventFiles(const QStringList& urls){

    NeuroscopeView* view = activeView();
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    //Loop on the files
    int counter = 0;
    QStringList::const_iterator iterator;
    for(iterator = urls.constBegin();iterator != urls.constEnd();++iterator){
        //Create the provider
        int returnStatus = doc->loadEventFile(*iterator,view);
        if(returnStatus == NeuroscopeDoc::OPEN_ERROR){
            QApplication::restoreOverrideCursor();
            QMessageBox::critical (this, tr("Error!"),tr("Could not load the file %1").arg(*iterator));
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            continue;
        }
        else if(returnStatus == NeuroscopeDoc::INCORRECT_FILE){
            QApplication::restoreOverrideCursor();
            QMessageBox::critical (this, tr("Error!"),tr("Incorrect file name (%1): the name has to be of the form baseName.id.evt or baseName.evt.id (with id a 3 character identifier).").arg(*iterator));
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            continue;
        }
        else if(returnStatus == NeuroscopeDoc::CREATION_ERROR){
            QApplication::restoreOverrideCursor();
            QMessageBox::critical (this, tr("Error!"),tr("The number of events of the requested file (%1) could not be determined.").arg(*iterator));
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            continue;
        }
        else if(returnStatus == NeuroscopeDoc::INCORRECT_CONTENT){
            QApplication::restoreOverrideCursor();
            QMessageBox::critical (this, tr("Error!"),tr("The content of the requested file (%1) is incorrect (see file format information).").arg(*iterator));
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            continue;
        }
        else if(returnStatus == NeuroscopeDoc::ALREADY_OPENED){
            QApplication::restoreOverrideCursor();
            QMessageBox::critical (this, tr("Error!"),tr("The requested file (%1) is already loaded.").arg(*iterator));
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            continue;
        }

        //Create the event palette if need it
        counter++;
        QString eventFileId = doc->lastLoadedProviderName();
        if(eventFileList.isEmpty() && counter == 1)
            createEventPalette(eventFileId);
        else
            addEventFile(eventFileId);
    }
    QApplication::restoreOverrideCursor();
}


void NeuroscopeApp::createEventPalette(const QString& eventFileId){

    ItemPalette* eventPalette = new ItemPalette(ItemPalette::EVENT,backgroundColor,this,"events");
    eventFileList.append(eventFileId);

    if(displayPaletteHeaders) {
        paletteTabsParent->addTab(eventPalette,QIcon(":/icons/events"),tr("Events"));
    } else {
        int index = paletteTabsParent->addTab(eventPalette,QString());
        paletteTabsParent->setTabIcon(index,QIcon(":/icons/events"));
    }



    //Palette connections
    connect(eventPalette, SIGNAL(colorChanged(int,QString)), this, SLOT(slotEventColorUpdate(int,QString)));
    connect(eventPalette, SIGNAL(updateShownItems(QMap<QString,QList<int> >)), this, SLOT(slotUpdateShownEvents(QMap<QString,QList<int> >)));
    connect(eventPalette, SIGNAL(selectedGroupChanged(QString)), this, SLOT(slotEventGroupSelected(QString)));
    connect(eventPalette, SIGNAL(updateItemsToSkip(QString,QList<int>)), this, SLOT(slotUpdateEventsToSkip(QString,QList<int>)));
    connect(eventPalette,SIGNAL(noEventsToBrowse()),this, SLOT(slotNoEventsToBrowse()));
    connect(eventPalette,SIGNAL(eventsToBrowse()),this, SLOT(slotEventsToBrowse()));

    //Create the list
    eventPalette->createItemList(doc->providerColorList(eventFileId),eventFileId,doc->getLastEventProviderGridX());

    //Disconnect the previous connection
    if(paletteTabsParent != NULL)
        disconnect(paletteTabsParent,0,0,0);

    connect(paletteTabsParent, SIGNAL(currentChanged(QWidget*)), this, SLOT(slotPaletteTabChange(QWidget*)));

    slotStateChanged("eventState");
    if(isPositionFileLoaded) {
        slotStateChanged("eventsInPositionViewEnableState");
    }
}

void NeuroscopeApp::addEventFile(const QString& eventFileId){
    eventFileList.append(eventFileId);

    for(int i = 0; i<paletteTabsParent->count();i++){
        QWidget* current = paletteTabsParent->page(i);
        QString name = current->name();
        if(current->metaObject()->className() == ("ItemPalette") && name.contains("eventPanel")){
            ItemPalette* eventPalette = static_cast<ItemPalette*>(current);
            //Create the list
            eventPalette->createItemList(doc->providerColorList(eventFileId),eventFileId,doc->getLastEventProviderGridX());
            break; // ??????????????

            if(isPositionFileLoaded) {
                slotStateChanged("eventsInPositionViewEnableState");
            }
        }
    }
}

void NeuroscopeApp::slotEventColorUpdate(int eventId, const QString& providerName){
    QWidget* current = paletteTabsParent->currentPage();
    QString name = current->name();
    if(current->metaObject()->className() == ("ItemPalette") && name.contains("eventPanel")){
        NeuroscopeView* view = activeView();
        doc->eventColorUpdate(providerName,eventId,view);
    }
}

void NeuroscopeApp::slotUpdateShownEvents(const QMap<QString,QList<int> >& selection){
    QWidget* current = paletteTabsParent->currentPage();
    QString name = current->name();
    if(current->metaObject()->className() == ("ItemPalette") && name.contains("eventPanel")){
        QMap<QString,QList<int> >::ConstIterator groupIterator;
        for(groupIterator = selection.begin(); groupIterator != selection.end(); ++groupIterator){
            QString providerName = groupIterator.key();
            QList<int> eventIds = groupIterator.data();
            NeuroscopeView* view = activeView();
            view->shownEventsUpdate(providerName,eventIds);
        }
    }
}

void NeuroscopeApp::slotCloseEventFile(){
    QWidget* current = paletteTabsParent->currentPage();
    QString name = current->name();
    if(current->metaObject()->className() == ("ItemPalette") && name.contains("eventPanel")){
        ItemPalette* eventPalette = static_cast<ItemPalette*>(current);

        NeuroscopeView* view = activeView();
        if(eventFileList.count() == 1)
            doc->removeEventFile(eventProvider,view,true);
        else
            doc->removeEventFile(eventProvider,view,false);
        eventFileList.remove(eventProvider);
        eventPalette->removeGroup(eventProvider);

        if(eventFileList.isEmpty()){
            paletteTabsParent->removePage(current);
            eventProvider.clear();
            currentNbUndo = 0;
            currentNbRedo = 0;
            delete current;
            slotStateChanged("noEventState");
            showEventsInPositionView->setChecked(false);
        }
        else{
            slotStateChanged("eventState");
            if(!eventPalette->isBrowsingEnable()) {
                slotStateChanged("noEventBrowsingState");
            }
            else{

                slotStateChanged("eventBrowsingState");
            }
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
    addEventPopup->clear();
    addEventPopup->resize(0,0);
    addEventPopup->adjustSize();
    addEventMenu->menu()->clear();
#if KDAB_PENDING
    eventIndex = -1;
    QStringList list;
    QList<EventDescription> eventList = doc->eventIds(eventProvider);

    QList<EventDescription>::iterator it;
    for(it = eventList.begin(); it != eventList.end(); ++it){
        QString label = *it;
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
    else
        eventLabelToCreate.clear();
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
        QString result = QInputDialog::getText(this,tr("New Event Description"),tr("Type in the new event description"),QLineEdit::Normal,QString(),&ok);
        if(ok) {
            eventLabelToCreate = result;
        }
    }
    else
        eventLabelToCreate = description;
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
        QString result = QInputDialog::getText(0,tr("New Event Description"),tr("Type in the new event description"),QLineEdit::Normal,QString(),&ok);
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
        QWidget* current = paletteTabsParent->page(i);
        QString name = current->name();
        if(qobject_cast<ItemPalette*>(current) && name.contains("eventPanel")){
            eventPalette = static_cast<ItemPalette*>(current);
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
    eventLabelToCreate.clear();
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

void NeuroscopeApp::slotUpdateEventsToSkip(QString groupName,const QList<int>& eventsToSkip){
    activeView()->updateNoneBrowsingEventList(groupName,eventsToSkip);
}

void NeuroscopeApp::slotUpdateClustersToSkip(QString groupName,const QList<int>& clustersToSkip){
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

void NeuroscopeApp::slotStateChanged(const QString& state)
{
    if(state == QLatin1String("initState")) {
    } else if(state == QLatin1String("documentState")) {
    } else if(state == QLatin1String("noChannelState")) {
    } else if(state == QLatin1String("displayChannelState")) {
    } else if(state == QLatin1String("spikeChannelState")) {
    } else if(state == QLatin1String("editState")) {
    } else if(state == QLatin1String("noEditState")) {
    } else if(state == QLatin1String("enableEditState")) {
        editMode->setEnabled(true);
    } else if(state == QLatin1String("tabState")) {
        mRenameActiveDisplay->setEnabled(true);
    } else if(state == QLatin1String("noTabState")) {
        mRenameActiveDisplay->setEnabled(false);
    } else if(state == QLatin1String("datState")) {
    } else if(state == QLatin1String("noDatState")) {
    } else if(state == QLatin1String("clusterTabState")) {
    } else if(state == QLatin1String("clusterState")) {
    } else if(state == QLatin1String("noClusterState")) {
    } else if(state == QLatin1String("noClusterBrowsingState")) {
    } else if(state == QLatin1String("clusterBrowsingState")) {
    } else if(state == QLatin1String("noClusterRasterState")) {
    } else if(state == QLatin1String("clusterRasterState")) {
    } else if(state == QLatin1String("eventState")) {
    } else if(state == QLatin1String("noEventState")) {
    } else if(state == QLatin1String("noEventBrowsingState")) {
    } else if(state == QLatin1String("eventBrowsingState")) {
    } else if(state == QLatin1String("positionState")) {
        /*
  <Enable>
    <Action name="close_position_file" />
    <Action name="position_view"/>
  </Enable>
  <Disable>
   <Action name="load_position_file" />
  </Disable>

  */
    } else if(state == QLatin1String("noPositionState")) {
        /*
  <Enable>
    <Action name="load_position_file" />
  </Enable>
  <Disable>
   <Action name="close_position_file" />
   <Action name="position_view"/>
  </Disable>
*/
        showEventsInPositionView->setEnabled(false);
    } else if(state == QLatin1String("eventsInPositionViewEnableState")) {
        showEventsInPositionView->setEnabled(true);
    } else if(state == QLatin1String("eventTabState")) {
        /*
  <Enable>
   <Action name="close_event_file" />
   <Action name="add_event" />
   <Action name="add_event_toolbarAction" />
  </Enable>
  <Disable>
   <Action name="edit_select_all_except01"/>
  </Disable>
 </State>
*/
        mCloseCluster->setEnabled(false);
    } else if(state == QLatin1String("undoState")) {
        mRedo->setEnabled(true);
        mUndo->setEnabled(true);
    } else if(state == QLatin1String("emptyRedoState")) {
        mRedo->setEnabled(false);
    } else if(state == QLatin1String("emptyUndoState")) {
        mUndo->setEnabled(false);
        mRedo->setEnabled(true);

    } else {
        qDebug()<<" unknown state "<<state;
    }
}

void NeuroscopeApp::slotAbout()
{
    QMessageBox::about(this,tr("Neuroscope"),tr("Viewer for Local Field Potentials, spikes, events and positional data"));

}
#include "neuroscope.moc"
