#ifndef PROPERTIESLAYOUT_H
#define PROPERTIESLAYOUT_H

#include "ui_propertieslayout.h"


class PropertiesLayout : public QWidget, public Ui_PropertiesLayout
{
    Q_OBJECT

public:
    explicit PropertiesLayout( QWidget* parent = 0 );
};

#endif
