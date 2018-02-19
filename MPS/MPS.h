#pragma once

#include <QtWidgets/QWidget>
#include "ui_MPS.h"

class MPS : public QWidget
{
	Q_OBJECT

public:
	MPS(QWidget *parent = Q_NULLPTR);

private:
	Ui::MPSClass ui;
};
