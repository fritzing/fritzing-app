#ifndef SVGTEXT_H
#define SVGTEXT_H

class QImage;
class QDomElement;
class QMatrix;
class QRectF;

class SvgText {
public:
    // Works around QTBUG-32405
    // TODO: Rename, for example textBounds. "renderText" is misleading, as we are not interested in the image, just the
    // size and transforms
    // TODO: Refactor this method so it looks and behaves more similar to qsvg (pull reqeust to QSvgText::bounds ?)
	static void renderText(QImage & image, QDomElement & text, int & minX, int & minY, int & maxX, int & maxY, QMatrix & matrix, QRectF & viewBox);
};

#endif // SVGTEXT_H
