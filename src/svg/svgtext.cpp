#include "svgtext.h"

#include <QString>
#include <QDomElement>
#include <QMatrix>
#include <QRect>
#include <QSvgRenderer>
#include <QPainter>

#define MINMAX(mx, my)          \
	if (mx < minX) minX = mx;   \
	if (mx > maxX) maxX = mx;   \
	if (my < minY) minY = my;   \
	if (my > maxY) maxY = my;

static QString IDString("-_-_-text-_-_-%1");

void SvgText::renderText(QImage & image, QDomElement & text, int & minX, int & minY, int & maxX, int & maxY, QMatrix & matrix, QRectF & viewBox)
{
	QString oldid = text.attribute("id");
	text.setAttribute("id", IDString);

	// TODO: handle inherited fill/stroke values
	QString oldFill = text.attribute("fill");
	text.setAttribute("fill", "black");
	QString oldStroke = text.attribute("stroke");
	text.setAttribute("stroke", "black");
	text.setTagName("text");

	image.fill(0xffffffff);
	QByteArray byteArray = text.ownerDocument().toByteArray();
	QSvgRenderer renderer(byteArray);
	QPainter painter;
	painter.begin(&image);
	painter.setRenderHint(QPainter::Antialiasing, false);
	renderer.render(&painter  /*, sourceRes */);
	painter.end();

	viewBox = renderer.viewBoxF();
	double x = text.attribute("x").toDouble();
	double y = text.attribute("y").toDouble();
	QPointF xy(x, y);
	matrix = renderer.matrixForElement(IDString);
	QPointF mxy = matrix.map(xy);

	QPointF p(image.width() * mxy.x() / viewBox.width(), image.height() * mxy.y() / viewBox.height());
	QPoint iq((int) p.x(), (int) p.y());

	minX = p.x();
	maxX = p.x();
	minY = p.y();
	maxY = p.y();

	// spiral around q
	int limit = qMax(image.width(), image.height());
	for (int lim = 0; lim < limit; lim++) {
		int t = qMax(0, iq.y() - lim);
		int b = qMin(iq.y() + lim, image.height() - 1);
		int l = qMax(0, iq.x() - lim);
		int r = qMin(iq.x() + lim, image.width() - 1);

		for (int iy = t; iy <= b; iy++) {
			if (image.pixel(l, iy) == 0xff000000) {
				MINMAX(l, iy);
			}
			if (image.pixel(r, iy) == 0xff000000) {
				MINMAX(r, iy);
			}
		}

		for (int ix = l + 1; ix < r; ix++) {
			if (image.pixel(ix, t) == 0xff000000) {
				MINMAX(ix, t);
			}
			if (image.pixel(ix, b) == 0xff000000) {
				MINMAX(ix, b);
			}
		}
	}

	text.setTagName("g");
	if (oldid.isEmpty()) text.removeAttribute("id");
	else text.setAttribute("id", oldid);
	if (oldFill.isEmpty()) text.removeAttribute("fill");
	else text.setAttribute("fill", oldFill);
	if (oldStroke.isEmpty()) text.removeAttribute("stroke");
	else text.setAttribute("stroke", oldStroke);
}
