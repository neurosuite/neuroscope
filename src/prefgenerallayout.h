#ifndef PREFGENERALLAYOUT_H
#define PREFGENERALLAYOUT_H

#include "ui_prefgenerallayout.h"


class PrefGeneralLayout : public QWidget, public Ui_PrefGeneralLayout
{
    Q_OBJECT

public:
    explicit PrefGeneralLayout( QWidget* parent = 0 );
   
};

#endif
