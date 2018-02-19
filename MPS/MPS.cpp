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
	/* clear imagePoints */
	imagePoints.clear();

	/* set maximum scale (maximum scale is 1/5 screen width) */
	maxScale = QApplication::desktop()->screenGeometry().width() / 5;	// set maximum scale

	/* set window's title*/
	if (!fileName.isEmpty()) {
		int pos1 = fileName.lastIndexOf('/');
		filePath = fileName.left(pos1 + 1);							//檔案路徑
		fileName = fileName.right(fileName.size() - pos1 - 1);		//檔案名稱
		fileName = QString(tr(" - ")) + fileName;
	}
	QString title = QString(tr("MPS")) + fileName;
	setWindowTitle(title);

	/*read MPS points set*/
	QFile file(filePath + "MPS(PT).txt");
	if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QTextStream in(&file);
		while (!in.atEnd()) {
			QString line = in.readLine();
			QStringList lineSplit = line.split("\t");
			imagePoints.push_back(QPointF(lineSplit[0].toFloat(), lineSplit[1].toFloat()));
		}
	}
	file.close();

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
			imagePoints.push_back(imagePos);
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
				imagePoints.push_back(imagePos);
				outBorder = false;
				// mousePress points isn't out of border 
			} else {
				imagePoints[imagePoints.size() - 1] = imagePos;
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
	if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return && imagePoints.size() > 4) {
		QFile file(filePath + "MPS(PT).txt");
		if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
			QTextStream out(&file);
			out << dec << fixed;
			for (int i = 0; i < imagePoints.size(); ++i) {
				out << imagePoints[i].x() << "\t" << imagePoints[i].y() << endl;
			}
			file.close();
		}
	// press keyboard Esc to give up setting point
	} else if (event->key() == Qt::Key_Escape) {
		imagePoints.clear();	// clear points
		update();
	// press keyboard Backspace to delete last point
	} else if (event->key() == Qt::Key_Backspace) {
		imagePoints.pop_back();	// clear points
		update();
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
	if (imagePoints.size() > 0) {
		for (int i = 0; i < imagePoints.size(); i = i + 2) {
			QPointF p1 = imagePoints[i] * scale + newDelta;
			painter.setPen(QPen(Qt::red, 5));
			painter.drawPoint(p1);
			if (imagePoints.size() != i + 1) {
				QPointF p2 = imagePoints[i + 1] * scale + newDelta;
				/* draw line to lower level*/
				painter.setPen(QPen(Qt::green, 3));
				painter.drawLine(p1, p2);
				/* draw point to upper level */
				painter.setPen(QPen(Qt::red, 5));
				painter.drawPoint(p1);
				painter.drawPoint(p2);
			}
		}
	}
}