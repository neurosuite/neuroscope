#ifndef PREFDEFAULTSLAYOUT_H
#define PREFDEFAULTSLAYOUT_H

#include "ui_prefdefaultslayout.h"


class PrefDefaultsLayout : public QWidget, public Ui_PrefDefaultsLayout
{
    Q_OBJECT

public:
    explicit PrefDefaultsLayout( QWidget* parent = 0 );

};

#endif
