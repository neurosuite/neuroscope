/***************************************************************************
                          streamdialog.cpp  -  description
                             -------------------
    purpose              : Choice of the stream to open from a file read by
                           a plugin
    copyright            : (C) 2026 by Joscha Schmiedt
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "streamdialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

StreamDialog::StreamDialog(const QList<PluginFile::Stream>& streams, int defaultStream, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Open Stream"));

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("The file contains several streams. Choose the one to show in this window:"), this));

    list = new QListWidget(this);
    for (const PluginFile::Stream& stream : streams)
    {
        auto* item = new QListWidgetItem(stream.label, list);
        item->setToolTip(tr("%1 channels at %2 Hz (stream %3)").arg(stream.channelCount).arg(stream.samplingRate).arg(stream.id));
    }
    list->setCurrentRow(defaultStream);
    layout->addWidget(list);

    others = new QCheckBox(tr("Open the other streams in new windows"), this);
    layout->addWidget(others);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(list, &QListWidget::itemDoubleClicked, this, &QDialog::accept);
}

int StreamDialog::selectedStream() const
{
    return list->currentRow();
}

bool StreamDialog::openOthers() const
{
    return others->isChecked();
}
