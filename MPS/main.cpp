#include "MPS.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	MPS w;
	w.show();
	return a.exec();
}
