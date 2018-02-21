#include "MPS.h"

MPS::MPS(QWidget *parent)
	: QWidget(parent)
{
	initial();

	/* set widget*/
	setWindowIcon(QIcon(":MPS/Resources/icon.ico"));			// set program's icon
	setMinimumSize(300, 300);											// set widget's minimum size
	setCursor(Qt::CrossCursor);											// set cursor to cross type
	setMouseTracking(true);												// tracking mouse location
	setAcceptDrops(true);												// set widget can be drop
																		//showMaximized();													// set widget to show max
}

void MPS::initial()
{
	/* set times */
	times = 0;

	/* clear imagePoints */
	imagePointsS.clear();
	imagePointsL.clear();

	/* set maximum scale (maximum scale is 1/5 screen width) */
	maxScale = QApplication::desktop()->screenGeometry().width() / 5;	// set maximum scale

	/* set window's title*/
	if (!fileName.isEmpty()) {
		int pos1 = fileName.lastIndexOf('/');
		filePath = fileName.left(pos1 + 1);							//檔案路徑
		fileName = fileName.right(fileName.size() - pos1 - 1);		//檔案名稱
	}
	QString title = QString(tr("MPS")) + QString(tr(" - ")) + fileName;
	setWindowTitle(title);

	/*read MPS points set*/
	QFile fileS(filePath + "MPS_S(PT).txt");
	if (fileS.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QTextStream in(&fileS);
		while (!in.atEnd()) {
			QString line = in.readLine();
			QStringList lineSplit = line.split("\t");
			imagePointsS.push_back(QPointF(lineSplit[0].toFloat(), lineSplit[1].toFloat()));
		}
	}
	fileS.close();

	QFile fileL(filePath + "MPS_L(PT).txt");
	if (fileL.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QTextStream in(&fileL);
		while (!in.atEnd()) {
			QString line = in.readLine();
			QStringList lineSplit = line.split("\t");
			imagePointsL.push_back(QPointF(lineSplit[0].toFloat(), lineSplit[1].toFloat()));
		}
	}
	fileL.close();

	if ((imagePointsS.size() + imagePointsL.size()) % 4 == 0) {
		times = 0;
	} else if ((imagePointsS.size() + imagePointsL.size()) % 4 == 1) {
		times = 1;
	} else if ((imagePointsS.size() + imagePointsL.size()) % 4 == 2) {
		times = 2;
	} else {
		times = 3;
	}

	/* calculate scale to show image */
	float scaleW = (float)winW / (float)(imgW - 1);
	float scaleH = (float)winH / (float)(imgH - 1);
	// If the scale is to zoom in, keep scale to 1:1.
	if (scaleW >= 1 && scaleH >= 1) {
		scale = 1;
		minScale = 1;
		// If the scale is to zoom out and have been zoom in, adjudge below.
	} else {
		minScale = scaleW < scaleH ? scaleW : scaleH;
		scale = minScale;
	}
}

void MPS::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasFormat("text/uri-list")) {
		event->acceptProposedAction();
	}
}

void MPS::dropEvent(QDropEvent *event)
{
	QList<QUrl> urls = event->mimeData()->urls();
	if (urls.isEmpty()) {
		return;
	}

	fileName = urls.first().toLocalFile();
	if (fileName.isEmpty()) {
		return;
	}

	img.load(fileName);		// load image
	initial();				// reset parameter
	update();
}

void MPS::resizeEvent(QResizeEvent *event)
{
	/* calculate scale to show image*/
	float scaleW = (float)winW / (float)(imgW - 1);
	float scaleH = (float)winH / (float)(imgH - 1);
	// If the scale is to zoom in, keep scale to 1:1.
	if (scaleW >= 1 && scaleH >= 1) {
		scale = 1;
		minScale = 1;
		// If the scale is to zoom out and have been zoom in, adjudge below.
	} else {
		// If scale is lower than minimum scale, set scale = minScale.
		// else keep the scale.
		minScale = scaleW < scaleH ? scaleW : scaleH;
		scale = scale < minScale ? minScale : scale;
	}
	update();
}

void MPS::wheelEvent(QWheelEvent *event)
{
	QPointF pixelPos = QPointF(event->posF());			// pixel coordinate
	QPointF imagePos = (pixelPos - newDelta) / scale;	// image coordinate

														// zoom in or ot
	if (event->angleDelta().y() > 0) {
		scale = scale * 1.2 > maxScale ? maxScale : scale * 1.2;
	} else {
		scale = scale / 1.2 < minScale ? minScale : scale / 1.2;
	}
	newDelta = pixelPos - scale * imagePos;
	oldDelta = newDelta;

	update();
}

void MPS::mousePressEvent(QMouseEvent *event)
{
	// drag image
	if (event->buttons() == Qt::LeftButton) {
		pos1 = QPointF(event->pos());
		// set point
	} else if (event->buttons() == Qt::RightButton) {
		// change to image's pixel coordinate
		QPointF imagePos = (QPointF(event->pos()) - newDelta) / scale;
		// limit the point in the image
		if (imagePos.x() >= 0 && imagePos.x() <= (imgW - 1) && imagePos.y() >= 0 && imagePos.y() <= (imgH - 1)) {
			// record file.
			if (times < 2) {
				imagePointsL.push_back(imagePos);
			} else {
				imagePointsS.push_back(imagePos);
			}
			times = (times + 1) % 4;
			outBorder = false;
			update();
		} else {
			outBorder = true;
		}
	}
}

void MPS::mouseMoveEvent(QMouseEvent *event)
{
	// drag image
	if (event->buttons() == Qt::LeftButton) {
		pos2 = QPointF(event->pos());
		/* new displacement add last displacement */
		newDelta = pos2 - pos1 + oldDelta;
		update();
		// set point
	} else if (event->buttons() == Qt::RightButton) {
		// change to image's pixel coordinate
		QPointF imagePos = (QPointF(event->pos()) - newDelta) / scale;
		// limit the point in the image
		if (imagePos.x() >= 0 && imagePos.x() <= (imgW - 1) && imagePos.y() >= 0 && imagePos.y() <= (imgH - 1)) {
			// record file.
			// mousePress points is out of border 
			if (outBorder) {
				if (times < 2 && times > 0) {
					imagePointsL.push_back(imagePos);
				} else {
					imagePointsS.push_back(imagePos);
				}
				outBorder = false;
				// mousePress points isn't out of border 
			} else {
				if (times < 2 && times > 0) {
					imagePointsL[imagePointsL.size() - 1] = imagePos;
				} else {
					imagePointsS[imagePointsS.size() - 1] = imagePos;
				}
			}
			update();
		}
	}
}

void MPS::mouseReleaseEvent(QMouseEvent *event)
{
	// record the distance of drag image
	oldDelta = newDelta;
	update();
}

void MPS::keyPressEvent(QKeyEvent *event)
{
	// press keyboard Enter to output points file
	if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {

		QVector<float> outAxisS;
		for (int i = 0; i < imagePointsS.size(); i = i + 2) {
			QPointF p1 = imagePointsS[i];
			if (imagePointsS.size() != i + 1) {
				QPointF p2 = imagePointsS[i + 1];
				outAxisS.push_back(std::sqrt(std::pow(p1.x() - p2.x(), 2) + std::pow(p1.y() - p2.y(), 2)));
			}
		}
		qSort(outAxisS);

		QFile fileS(filePath + "MPS_S(PSD).txt");
		if (fileS.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
			QTextStream out(&fileS);
			out << dec << fixed;
			out << fileName << ":\t";
			for (size_t i = 0; i < outAxisS.size(); ++i) {
				out << outAxisS[i];
				if (i != outAxisS.size() - 1) {
					out << "\t";
				}
			}
			out << endl;
			fileS.close();
		}

		QVector<float> outAxisL;
		for (int i = 0; i < imagePointsL.size(); i = i + 2) {
			QPointF p1 = imagePointsL[i];
			if (imagePointsL.size() != i + 1) {
				QPointF p2 = imagePointsL[i + 1];
				outAxisL.push_back(std::sqrt(std::pow(p1.x() - p2.x(), 2) + std::pow(p1.y() - p2.y(), 2)));
			}
		}
		qSort(outAxisL);

		QFile fileL(filePath + "MPS_L(PSD).txt");
		if (fileL.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
			QTextStream out(&fileL);
			out << dec << fixed;
			out << fileName << ":\t";
			for (size_t i = 0; i < outAxisL.size(); ++i) {
				out << outAxisL[i];
				if (i != outAxisL.size() - 1) {
					out << "\t";
				}
			}
			out << endl;
			fileL.close();
		}

// press keyboard Esc to give up setting point
	} else if (event->key() == Qt::Key_Escape) {
		imagePointsS.clear();	// clear points
		imagePointsL.clear();
		times = 0;
		update();
	// press keyboard Backspace to delete last point
	} else if (event->key() == Qt::Key_Backspace) {
		if (times == 0) {
			times = 3;
			imagePointsL.pop_back();	// clear points
		} else if (times == 1) {
			times = 0;
			imagePointsL.pop_back();
		} else if (times == 2) {
			times = 1;
			imagePointsS.pop_back();
		} else if (times == 3) {
			times = 2;
			imagePointsS.pop_back();
		}
		update();
	// press keyboard S to save points file
	} else if (event->key() == Qt::Key_S) {
		QFile fileS(filePath + "MPS_S(PT).txt");
		if (fileS.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
			QTextStream out(&fileS);
			out << dec << fixed;
			for (int i = 0; i < imagePointsS.size(); ++i) {
				out << imagePointsS[i].x() << "\t" << imagePointsS[i].y() << endl;
			}
			fileS.close();
		}

		QFile fileL(filePath + "MPS_L(PT).txt");
		if (fileL.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
			QTextStream out(&fileL);
			out << dec << fixed;
			for (int i = 0; i < imagePointsL.size(); ++i) {
				out << imagePointsL[i].x() << "\t" << imagePointsL[i].y() << endl;
			}
			fileL.close();
		}
	}
}

void MPS::paintEvent(QPaintEvent *event)
{
	/* modify xDelta */
	// If image's horizontal size to show is longer than winW, set it to center.
	if ((imgW - 1) * scale < winW) {
		newDelta.rx() = winW / 2 - scale * (imgW - 1) / 2;
		// If image's horizontal delta is less than winW, set it to edge.
	} else if (newDelta.x() > 0) {
		newDelta.rx() = 0;
		// If image's horizontal delta is bigger than 0, set it to edge.
	} else if ((imgW - 1) * scale + newDelta.x() < winW) {
		newDelta.rx() = winW - (imgW - 1) * scale;
	}

	/* modify yDelta */
	// If image's vertical size to show is longer than winW, set it to center.
	if ((imgH - 1) * scale < winH) {
		newDelta.ry() = winH / 2 - scale * (imgH - 1) / 2;
		// If image's vertical delta is less than winW, set it to edge.
	} else if (newDelta.ry() > 0) {
		newDelta.ry() = 0;
		// If image's vertical delta is bigger than 0, set it to edge.
	} else if ((imgH - 1) * scale + newDelta.y() < winH) {
		newDelta.ry() = winH - (imgH - 1) * scale;
	}

	QPainter painter(this);

	/* draw image */
	QRectF rect(newDelta.x() - 0.5 * scale, newDelta.y() - 0.5 * scale, imgW * scale, imgH * scale);	// draw range
	painter.drawImage(rect, img);	// draw image

	/* draw points and lines */
	if (imagePointsL.size() > 0) {
		for (int i = 0; i < imagePointsL.size(); i = i + 2) {
			QPointF p1 = imagePointsL[i] * scale + newDelta;
			painter.setPen(QPen(Qt::red, 5));
			painter.drawPoint(p1);
			if (imagePointsL.size() != i + 1) {
				QPointF p2 = imagePointsL[i + 1] * scale + newDelta;
				/* draw line to lower level*/
				painter.setPen(QPen(Qt::green, 3));
				painter.drawLine(p1, p2);
				/* draw point to upper level */
				painter.setPen(QPen(Qt::red, 4));
				painter.drawPoint(p1);
				painter.drawPoint(p2);
			}
		}
	}

	if (imagePointsS.size() > 0) {
		for (int i = 0; i < imagePointsS.size(); i = i + 2) {
			QPointF p1 = imagePointsS[i] * scale + newDelta;
			painter.setPen(QPen(Qt::red, 5));
			painter.drawPoint(p1);
			if (imagePointsS.size() != i + 1) {
				QPointF p2 = imagePointsS[i + 1] * scale + newDelta;
				/* draw line to lower level*/
				painter.setPen(QPen(Qt::blue, 3));
				painter.drawLine(p1, p2);
				/* draw point to upper level */
				painter.setPen(QPen(Qt::red, 4));
				painter.drawPoint(p1);
				painter.drawPoint(p2);
			}
		}
	}
}