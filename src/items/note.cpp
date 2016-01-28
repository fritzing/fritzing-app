/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2016 Fritzing

Fritzing is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Fritzing is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Fritzing.  If not, see <http://www.gnu.org/licenses/>.

********************************************************************

$Revision: 6980 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-22 01:45:43 +0200 (Mo, 22. Apr 2013) $

********************************************************************/

#include "note.h"
#include "../debugdialog.h"
#include "../sketch/infographicsview.h"
#include "../model/modelpart.h"
#include "../utils/resizehandle.h"
#include "../utils/textutils.h"
#include "../utils/graphicsutils.h"
#include "../fsvgrenderer.h"

#include <QTextFrame>
#include <QTextLayout>
#include <QTextFrameFormat>
#include <QApplication>
#include <QTextDocumentFragment>
#include <QTimer>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QPushButton>

// TODO:
//		** search for ModelPart:: and fix up
//		check which menu items don't apply
//		** selection
//		** delete
//		** move
//		** undo delete + text
//		** resize
//		** undo resize
//		anchor
//		** undo change text
//		** undo selection
//		** undo move
//		** layers and z order
//		** hide and show layer
//		** save and load
//		format: bold, italic, size (small normal large huge), color?,
//		undo format
//		heads-up controls
//		copy/paste
//		** z-order manipulation
//		hover
//		** multiple selection
//		** icon in taskbar (why does it show up as text until you update it?)

const int Note::emptyMinWidth = 40;
const int Note::emptyMinHeight = 25;
const int Note::initialMinWidth = 140;
const int Note::initialMinHeight = 45;
const int borderWidth = 7;
const int TriangleOffset = 7;

const double InactiveOpacity = 0.5;

QString Note::initialTextString;

QRegExp UrlTag("<a.*href=[\"']([^\"]+[.\\s]*)[\"'].*>");    

///////////////////////////////////////

void findA(QDomElement element, QList<QDomElement> & aElements) 
{
	if (element.tagName().compare("a", Qt::CaseInsensitive) == 0) {
		aElements.append(element);
		return;
	}

	QDomElement c = element.firstChildElement();
	while (!c.isNull()) {
		findA(c, aElements);

		c = c.nextSiblingElement();
	}
}


QString addText(const QString & text, bool inUrl) 
{
	if (text.isEmpty()) return "";

	return QString("<tspan fill='%1' >%2</tspan>\n")
				.arg(inUrl ? "#0000ff" : "#000000")
				.arg(TextUtils::convertExtendedChars(TextUtils::escapeAnd(text)))
				;
}

///////////////////////////////////////

class NoteGraphicsTextItem : public QGraphicsTextItem
{
public:
	NoteGraphicsTextItem(QGraphicsItem * parent = NULL);

protected:
	void focusInEvent(QFocusEvent *);
	void focusOutEvent(QFocusEvent *);
};

NoteGraphicsTextItem::NoteGraphicsTextItem(QGraphicsItem * parent) : QGraphicsTextItem(parent)
{
	const QTextFrameFormat format = document()->rootFrame()->frameFormat();
	QTextFrameFormat altFormat(format);
	altFormat.setMargin(0);										// so document never thinks a mouse click is a move event
	document()->rootFrame()->setFrameFormat(altFormat);
}

void NoteGraphicsTextItem::focusInEvent(QFocusEvent * event) {
	InfoGraphicsView * igv = InfoGraphicsView::getInfoGraphicsView(this);
	if (igv != NULL) {
		igv->setNoteFocus(this, true);
	}
	QApplication::instance()->installEventFilter((Note *) this->parentItem());
	QGraphicsTextItem::focusInEvent(event);
	DebugDialog::debug("note focus in");
}

void NoteGraphicsTextItem::focusOutEvent(QFocusEvent * event) {
	InfoGraphicsView * igv = InfoGraphicsView::getInfoGraphicsView(this);
	if (igv != NULL) {
		igv->setNoteFocus(this, false);
	}
	QApplication::instance()->removeEventFilter((Note *) this->parentItem());
	QGraphicsTextItem::focusOutEvent(event);
	DebugDialog::debug("note focus out");
}

//////////////////////////////////////////

LinkDialog::LinkDialog(QWidget *parent) : QDialog(parent) 
{
	this->setWindowTitle(QObject::tr("Edit link"));

	QVBoxLayout * vLayout = new QVBoxLayout(this);

	QGroupBox * formGroupBox = new QGroupBox(this);

	QFormLayout * formLayout = new QFormLayout();

	m_urlEdit = new QLineEdit(this);
	m_urlEdit->setFixedHeight(25);
	m_urlEdit->setFixedWidth(200);
	formLayout->addRow(tr("url:"), m_urlEdit );

	m_textEdit = new QLineEdit(this);
	m_textEdit->setFixedHeight(25);
	m_textEdit->setFixedWidth(200);
	formLayout->addRow(tr("text:"), m_textEdit );

	formGroupBox->setLayout(formLayout);

	vLayout->addWidget(formGroupBox);

    QDialogButtonBox * buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
	buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	vLayout->addWidget(buttonBox);

	this->setLayout(vLayout);
}

LinkDialog::~LinkDialog() {
}

void LinkDialog::setText(const QString & text) {
	m_textEdit->setText(text);
}

void LinkDialog::setUrl(const QString & url) {
	m_urlEdit->setText(url);
}

QString LinkDialog::text() {
	return m_textEdit->text();
}

QString LinkDialog::url() {
	return m_urlEdit->text();
}

/////////////////////////////////////////////

Note::Note( ModelPart * modelPart, ViewLayer::ViewID viewID,  const ViewGeometry & viewGeometry, long id, QMenu* itemMenu)
	: ItemBase(modelPart, viewID, viewGeometry, id, itemMenu)
{
	m_charsAdded = 0;
	if (initialTextString.isEmpty()) {
		initialTextString = tr("[write your note here]");
	}

	m_inResize = NULL;
	this->setCursor(Qt::ArrowCursor);

    setFlag(QGraphicsItem::ItemIsSelectable, true);

	if (viewGeometry.rect().width() == 0 || viewGeometry.rect().height() == 0) {
		m_rect.setRect(0, 0, Note::initialMinWidth, Note::initialMinHeight);
	}
	else {
		m_rect.setRect(0, 0, viewGeometry.rect().width(), viewGeometry.rect().height());
	}
	m_pen.setWidth(borderWidth);
	m_pen.setCosmetic(false);
    m_pen.setBrush(QColor(0xfa, 0xbc, 0x4f));

    m_brush.setColor(QColor(0xff, 0xe9, 0xc8));
	m_brush.setStyle(Qt::SolidPattern);

	setPos(m_viewGeometry.loc());

	QPixmap pixmap(":/resources/images/icons/noteResizeGrip.png");
	m_resizeGrip = new ResizeHandle(pixmap, Qt::SizeFDiagCursor, false, this);
	connect(m_resizeGrip, SIGNAL(mousePressSignal(QGraphicsSceneMouseEvent *, ResizeHandle *)), this, SLOT(handleMousePressSlot(QGraphicsSceneMouseEvent *, ResizeHandle *)));
	connect(m_resizeGrip, SIGNAL(mouseMoveSignal(QGraphicsSceneMouseEvent *, ResizeHandle *)), this, SLOT(handleMouseMoveSlot(QGraphicsSceneMouseEvent *, ResizeHandle *)));
	connect(m_resizeGrip, SIGNAL(mouseReleaseSignal(QGraphicsSceneMouseEvent *, ResizeHandle *)), this, SLOT(handleMouseReleaseSlot(QGraphicsSceneMouseEvent *, ResizeHandle *)));
	//connect(m_resizeGrip, SIGNAL(zoomChangedSignal(double)), this, SLOT(handleZoomChangedSlot(double)));

	m_graphicsTextItem = new NoteGraphicsTextItem();
	QFont font("Droid Sans", 9, QFont::Normal);
	m_graphicsTextItem->setFont(font);
	m_graphicsTextItem->document()->setDefaultFont(font);
	m_graphicsTextItem->setParentItem(this);
	m_graphicsTextItem->setVisible(true);
	m_graphicsTextItem->setPlainText(initialTextString);
	m_graphicsTextItem->setTextInteractionFlags(Qt::TextEditorInteraction | Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
	m_graphicsTextItem->setCursor(Qt::IBeamCursor);
	m_graphicsTextItem->setOpenExternalLinks(true);


	connectSlots();

	positionGrip();

	setAcceptHoverEvents(true);
}

void Note::saveGeometry() {
	m_viewGeometry.setRect(boundingRect());
	m_viewGeometry.setLoc(this->pos());
	m_viewGeometry.setSelected(this->isSelected());
	m_viewGeometry.setZ(this->zValue());
}

bool Note::itemMoved() {
	return (this->pos() != m_viewGeometry.loc());
}

void Note::saveInstanceLocation(QXmlStreamWriter & streamWriter) {
	QRectF rect = m_viewGeometry.rect();
	QPointF loc = m_viewGeometry.loc();
	streamWriter.writeAttribute("x", QString::number(loc.x()));
	streamWriter.writeAttribute("y", QString::number(loc.y()));
	streamWriter.writeAttribute("width", QString::number(rect.width()));
	streamWriter.writeAttribute("height", QString::number(rect.height()));
}

void Note::moveItem(ViewGeometry & viewGeometry) {
	this->setPos(viewGeometry.loc());
}

void Note::findConnectorsUnder() {
}

void Note::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(widget);

	if (m_hidden) return;

	if (m_inactive) {
		painter->save();
		painter->setOpacity(InactiveOpacity);
	}

	painter->setPen(Qt::NoPen);
	painter->setBrush(m_brush);
    QPolygonF poly;
    poly.append(m_rect.topLeft());
    poly.append(m_rect.topRight() + QPointF(-TriangleOffset, 0));
    poly.append(m_rect.topRight() + QPointF(0, TriangleOffset));
    poly.append(m_rect.bottomRight());
    poly.append(m_rect.bottomLeft());
    painter->drawPolygon(poly);

    painter->setBrush(m_pen.brush());
    poly.clear();
    poly.append(m_rect.topRight() + QPointF(-TriangleOffset, 0));
    poly.append(m_rect.topRight() + QPointF(0, TriangleOffset));
    poly.append(m_rect.topRight() + QPointF(-TriangleOffset, TriangleOffset));
    painter->drawPolygon(poly);

    painter->setPen(m_pen);
    poly.clear();
    poly.append(QPointF(0, m_rect.bottom() - borderWidth / 2.0));
    poly.append(QPointF(m_rect.right(), m_rect.bottom() - borderWidth / 2.0));
    painter->drawPolygon(poly);

	if (option->state & QStyle::State_Selected) {
		GraphicsUtils::qt_graphicsItem_highlightSelected(painter, option, boundingRect(), QPainterPath());
    }

	if (m_inactive) {
		painter->restore();
	}
}

QRectF Note::boundingRect() const
{
	return m_rect;
}

QPainterPath Note::shape() const
{
    QPainterPath path;
    path.addRect(boundingRect());
    return path;
}

void Note::positionGrip() {
	QSizeF gripSize = m_resizeGrip->boundingRect().size();
	QSizeF sz = this->boundingRect().size(); 
	QPointF p(sz.width() - gripSize.width(), sz.height() - gripSize.height());
	m_resizeGrip->setPos(p);
	m_graphicsTextItem->setPos(TriangleOffset / 2, TriangleOffset / 2);
	m_graphicsTextItem->setTextWidth(sz.width() - TriangleOffset);
}

void Note::mousePressEvent(QGraphicsSceneMouseEvent * event) {
	InfoGraphicsView *infographics = InfoGraphicsView::getInfoGraphicsView(this);
	if (infographics != NULL && infographics->spaceBarIsPressed()) {
		m_spaceBarWasPressed = true;
		event->ignore();
		return;
	}

	m_spaceBarWasPressed = false;
	m_inResize = NULL;
	ItemBase::mousePressEvent(event);
}

void Note::mouseMoveEvent(QGraphicsSceneMouseEvent * event) {
	if (m_spaceBarWasPressed) {
		event->ignore();
		return;
	}

	ItemBase::mouseMoveEvent(event);
}

void Note::mouseReleaseEvent(QGraphicsSceneMouseEvent * event) {
	if (m_spaceBarWasPressed) {
		event->ignore();
		return;
	}

	ItemBase::mouseReleaseEvent(event);
}

void Note::contentsChangeSlot(int position, int charsRemoved, int charsAdded) {
	Q_UNUSED(charsRemoved);

	m_charsAdded = charsAdded;
	m_charsPosition = position;
}

void Note::forceFormat(int position, int charsAdded) {
	disconnectSlots();
	QTextCursor textCursor = m_graphicsTextItem->textCursor();

	QTextCharFormat f;
	QFont font("Droid Sans", 9, QFont::Normal);

	f.setFont(font);
	f.setFontFamily("Droid Sans");
	f.setFontPointSize(9);

	int cc = m_graphicsTextItem->document()->characterCount();
	textCursor.setPosition(position, QTextCursor::MoveAnchor);
	if (position + charsAdded >= cc) {
		charsAdded--;
	}
	textCursor.setPosition(position + charsAdded, QTextCursor::KeepAnchor);

	//textCursor.setCharFormat(f);
	textCursor.mergeCharFormat(f);
	//DebugDialog::debug(QString("setting font tc:%1,%2 params:%3,%4")
		//.arg(textCursor.anchor()).arg(textCursor.position())
		//.arg(position).arg(position + charsAdded));

	/*
	textCursor = m_graphicsTextItem->textCursor();
	for (int i = 0; i < charsAdded; i++) {
		textCursor.setPosition(position + i, QTextCursor::MoveAnchor);
		f = textCursor.charFormat();
		DebugDialog::debug(QString("1format %1 %2 %3").arg(f.fontPointSize()).arg(f.fontFamily()).arg(f.fontWeight()));
		//f.setFont(font);
		//f.setFontPointSize(9);
		//f.setFontWeight(QFont::Normal);
		//textCursor.setCharFormat(f);
		//QTextCharFormat g = textCursor.charFormat();
		//DebugDialog::debug(QString("2format %1 %2 %3").arg(g.fontPointSize()).arg(g.fontFamily()).arg(g.fontWeight()));
	}
	*/

	connectSlots();
}

void Note::contentsChangedSlot() {

    //DebugDialog::debug(QString("contents changed ") + m_graphicsTextItem->document()->toPlainText());
	if (m_charsAdded > 0) {
		forceFormat(m_charsPosition, m_charsAdded);
	}

	InfoGraphicsView *infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		QString oldText;
		if (m_modelPart) {
			oldText = m_modelPart->instanceText();
		}

		QSizeF oldSize = m_rect.size();
		QSizeF newSize = oldSize;
		checkSize(newSize);

		infoGraphicsView->noteChanged(this, oldText, m_graphicsTextItem->document()->toHtml(), oldSize, newSize);
	}
	if (m_modelPart) {
		m_modelPart->setInstanceText(m_graphicsTextItem->document()->toHtml());
	}
}

void Note::checkSize(QSizeF & newSize) {
	QSizeF gripSize = m_resizeGrip->boundingRect().size();
	QSizeF size = m_graphicsTextItem->document()->size();
	if (size.height() + gripSize.height() + gripSize.height() > m_rect.height()) {
		prepareGeometryChange();
		m_rect.setHeight(size.height() + gripSize.height() + gripSize.height());
		newSize.setHeight(m_rect.height());
		positionGrip();
		this->update();
	}
}

void Note::disconnectSlots() {
	disconnect(m_graphicsTextItem->document(), SIGNAL(contentsChanged()),
			this, SLOT(contentsChangedSlot()));
	disconnect(m_graphicsTextItem->document(), SIGNAL(contentsChange(int, int, int)),
			this, SLOT(contentsChangeSlot(int, int, int)));
}

void Note::connectSlots() {
	connect(m_graphicsTextItem->document(), SIGNAL(contentsChanged()),
		this, SLOT(contentsChangedSlot()), Qt::DirectConnection);
	connect(m_graphicsTextItem->document(), SIGNAL(contentsChange(int, int, int)),
		this, SLOT(contentsChangeSlot(int, int, int)), Qt::DirectConnection);
}

void Note::setText(const QString & text, bool check) {
	// disconnect the signal so it doesn't fire recursively
	disconnectSlots();
	QString oldText = text;
	m_graphicsTextItem->document()->setHtml(text);
	connectSlots();

	if (check) {
		QSizeF newSize;
		checkSize(newSize);
		forceFormat(0, m_graphicsTextItem->document()->characterCount());
	}
}

QString Note::text() {
	return m_graphicsTextItem->document()->toHtml();
}

void Note::setSize(const QSizeF & size)
{
	prepareGeometryChange();
	m_rect.setWidth(size.width());
	m_rect.setHeight(size.height());
	positionGrip();
}

void Note::setHidden(bool hide)
{
	ItemBase::setHidden(hide);
	m_graphicsTextItem->setVisible(!hide);
	m_resizeGrip->setVisible(!hide);
}

void Note::setInactive(bool inactivate)
{
	ItemBase::setInactive(inactivate);
}

bool Note::eventFilter(QObject * object, QEvent * event)
{
	if (event->type() == QEvent::Shortcut || event->type() == QEvent::ShortcutOverride)
	{
		if (!object->inherits("QGraphicsView"))
		{
			event->accept();
			return true;
		}
	}

	if (event->type() == QEvent::KeyPress) {
		QKeyEvent * kevent = static_cast<QKeyEvent *>(event);
		if (kevent->matches(QKeySequence::Bold)) {
			QTextCursor textCursor = m_graphicsTextItem->textCursor();
			QTextCharFormat cf = textCursor.charFormat();
			bool isBold = cf.fontWeight() == QFont::Bold;
			QTextCharFormat textCharFormat;
			textCharFormat.setFontWeight(isBold ? QFont::Normal : QFont::Bold);
			textCursor.mergeCharFormat(textCharFormat);
			event->accept();
			return true;
		}
		if (kevent->matches(QKeySequence::Italic)) {
			QTextCursor textCursor = m_graphicsTextItem->textCursor();
			QTextCharFormat cf = textCursor.charFormat();
			QTextCharFormat textCharFormat;
			textCharFormat.setFontItalic(!cf.fontItalic());
			textCursor.mergeCharFormat(textCharFormat);
			event->accept();
			return true;
		}
		if ((kevent->key() == Qt::Key_L) && (kevent->modifiers() & Qt::ControlModifier)) {
			QTimer::singleShot(75, this, SLOT(linkDialog()));
			event->accept();
			return true;

		}
	}
	return false;
}

void Note::linkDialog() {
	QTextCursor textCursor = m_graphicsTextItem->textCursor();
	bool gotUrl = false;
	if (textCursor.anchor() == textCursor.selectionStart()) {
		// the selection returns empty since we're between characters
		// so select one character forward or one character backward 
		// to see whether we're in a url
		int wasAnchor = textCursor.anchor();
		bool atEnd = textCursor.atEnd();
		bool atStart = textCursor.atStart();
		if (!atStart) {
			textCursor.setPosition(wasAnchor - 1, QTextCursor::KeepAnchor);
			QString html = textCursor.selection().toHtml();
			if (UrlTag.indexIn(html) >= 0) {
				gotUrl = true;
			}
		}
		if (!gotUrl && !atEnd) {
			textCursor.setPosition(wasAnchor + 1, QTextCursor::KeepAnchor);
			QString html = textCursor.selection().toHtml();
			if (UrlTag.indexIn(html) >= 0) {
				gotUrl = true;
			}
		}
		textCursor.setPosition(wasAnchor, QTextCursor::MoveAnchor);
	}
	else {
		QString html = textCursor.selection().toHtml();
		DebugDialog::debug(html);
		if (UrlTag.indexIn(html) >= 0) {
			gotUrl = true;
		}
	}

	LinkDialog ld;
	QString originalText;
	QString originalUrl;
	if (gotUrl) {
		originalUrl = UrlTag.cap(1);
		ld.setUrl(originalUrl);
		QString html = m_graphicsTextItem->toHtml();

		// assumes html is in xml form
		QString errorStr;
		int errorLine;
		int errorColumn;

		QDomDocument domDocument;
		if (!domDocument.setContent(html, &errorStr, &errorLine, &errorColumn)) {
			return;
		}

		QDomElement root = domDocument.documentElement();
		if (root.isNull()) {
			return;
		}

		if (root.tagName() != "html") {
			return;
		}

		DebugDialog::debug(html);
		QList<QDomElement> aElements;
		findA(root, aElements);
		foreach (QDomElement a, aElements) {
			// TODO: if multiple hrefs point to the same url this will only find the first one
			QString href = a.attribute("href");
			if (href.isEmpty()) {
				href = a.attribute("HREF");
			}
			if (href.compare(originalUrl) == 0) {
				QString text;
				if (TextUtils::findText(a, text)) {
					ld.setText(text);
					break;
				}
				else {
					return;
				}
			}
		}
	}
	int result = ld.exec();
	if (result == QDialog::Accepted) {
		if (gotUrl) {
			int from = 0;
			while (true) {
				QTextCursor cursor = m_graphicsTextItem->document()->find(originalText, from);
				if (cursor.isNull()) {
					// TODO: tell the user
					return;
				}

				QString html = cursor.selection().toHtml();
				if (html.contains(originalUrl)) {
					cursor.insertHtml(QString("<a href=\"%1\">%2</a>").arg(ld.url()).arg(ld.text()));
					break;
				}

				from = cursor.selectionEnd();
			}
		}
		else {
			textCursor.insertHtml(QString("<a href=\"%1\">%2</a>").arg(ld.url()).arg(ld.text()));
		}
	}
}

void Note::handleZoomChangedSlot(double scale) {
	Q_UNUSED(scale);
	positionGrip();
}

void Note::handleMousePressSlot(QGraphicsSceneMouseEvent * event, ResizeHandle * resizeHandle) {
	if (m_spaceBarWasPressed) return;

	saveGeometry();

	QSizeF sz = this->boundingRect().size();
	resizeHandle->setResizeOffset(this->pos() + QPointF(sz.width(), sz.height()) - event->scenePos());

	m_inResize = resizeHandle;
}

void Note::handleMouseMoveSlot(QGraphicsSceneMouseEvent * event, ResizeHandle * resizeHandle) {
    Q_UNUSED(resizeHandle);

    if (!m_inResize) return;

	double minWidth = emptyMinWidth;
	double minHeight = emptyMinHeight;
	QSizeF gripSize = m_resizeGrip->boundingRect().size();
	QSizeF minSize = m_graphicsTextItem->document()->size() + gripSize + gripSize;
	if (minSize.height() > minHeight) minHeight = minSize.height();

	QRectF rect = boundingRect();
	rect.moveTopLeft(this->pos());

	double oldX1 = rect.x();
	double oldY1 = rect.y();
	double newX = event->scenePos().x() + m_inResize->resizeOffset().x();
	double newY = event->scenePos().y() + m_inResize->resizeOffset().y();
	QRectF newR;

	if (newX - oldX1 < minWidth) {
		newX = oldX1 + minWidth;
	}
	if (newY - oldY1 < minHeight) {
		newY = oldY1 + minHeight;
	}	
	newR.setRect(0, 0, newX - oldX1, newY - oldY1);

	prepareGeometryChange();
	m_rect = newR;
	positionGrip();	
}

void Note::handleMouseReleaseSlot(QGraphicsSceneMouseEvent * event, ResizeHandle * resizeHandle) {
    Q_UNUSED(resizeHandle);
    Q_UNUSED(event);

	if (!m_inResize) return;

	m_inResize = NULL;
	InfoGraphicsView *infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
        infoGraphicsView->noteSizeChanged(this, m_viewGeometry.rect().size(), m_rect.size());
	}
}

bool Note::hasPartLabel() {
	
	return false;
}

bool Note::stickyEnabled() {
	return false;
}

bool Note::hasPartNumberProperty()
{
	return false;
}

bool Note::rotationAllowed() {
	return false;
}

bool Note::rotation45Allowed() {
	return false;
}

QString Note::retrieveSvg(ViewLayer::ViewLayerID viewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor)
{
	Q_UNUSED(svgHash);
    factor = 1;

	switch (viewLayerID) {
		case ViewLayer::BreadboardNote:
			if (viewID() != ViewLayer::BreadboardView) return "";
			break;
		case ViewLayer::PcbNote:
			if (viewID() != ViewLayer::PCBView) return "";
			break;
		case ViewLayer::SchematicNote:
			if (viewID() != ViewLayer::SchematicView) return "";
			break;
		default:
			return "";
	}

	QString svg = "<g>";

	QString penColor = blackOnly ? "#000000" : m_pen.color().name();
	double penWidth = m_pen.widthF() * dpi / GraphicsUtils::SVGDPI;
	QString brushColor = blackOnly ? "none" : m_brush.color().name();
	svg += QString("<rect x='%1' y='%2' width='%3' height='%4' fill='%5' stroke='%6' stroke-width='%7' />")
		.arg(penWidth / 2)
		.arg(penWidth / 2)
		.arg((m_rect.width() * dpi / GraphicsUtils::SVGDPI) - penWidth)
		.arg((m_rect.height() * dpi / GraphicsUtils::SVGDPI) - penWidth)
		.arg(brushColor)
		.arg(penColor)
		.arg(penWidth)
		;

	QTextCursor textCursor = m_graphicsTextItem->textCursor();
	QSizeF gripSize = m_resizeGrip->boundingRect().size();
	double docLeft = gripSize.width();
	double docTop = gripSize.height();
	for (QTextBlock block = m_graphicsTextItem->document()->begin(); block.isValid(); block = block.next()) {
		QTextLayout * layout = block.layout();
		double left = block.blockFormat().leftMargin() + docLeft + layout->position().x();
		double top = block.blockFormat().topMargin() + docTop + layout->position().y();
		for (int i = 0; i < layout->lineCount(); i++) {
			QTextLine line = layout->lineAt(i);
			QRectF r = line.rect();
			int start = line.textStart();
			int count = line.textLength();

			QString soFar;

			svg += QString("<text  x='%1' y='%2' font-family='%3' stroke='none' fill='#000000' text-anchor='left' font-size='%4' >\n")
				.arg((left + r.left()) * dpi / GraphicsUtils::SVGDPI)
				.arg((top + r.top() + line.ascent()) * dpi / GraphicsUtils::SVGDPI)
				.arg("Droid Sans")
				.arg(line.ascent() * dpi / GraphicsUtils::SVGDPI) 
				;


			bool inUrl = false;
			for (int i = 0; i < count; i++) {
				textCursor.setPosition(i + start + block.position(), QTextCursor::MoveAnchor);
				textCursor.setPosition(i + start + block.position() + 1, QTextCursor::KeepAnchor);
				QString html = textCursor.selection().toHtml();
				if (UrlTag.indexIn(html) >= 0) {
					if (inUrl) {
						soFar += block.text().mid(start + i, 1);
						continue;
					}

					svg += addText(soFar, false);
					soFar = block.text().mid(start + i, 1);
					inUrl = true;
				}
				else {
					if (!inUrl) {
						soFar += block.text().mid(start + i, 1);
						continue;
					}

					svg += addText(soFar, !blackOnly);
					soFar = block.text().mid(start + i, 1);
					inUrl = false;
				}
			}
			svg += addText(soFar, inUrl && !blackOnly);
			svg += "</text>";
		}
	}	


	svg += "</g>";


	return svg;	
}

ViewLayer::ViewID Note::useViewIDForPixmap(ViewLayer::ViewID, bool) 
{
    return ViewLayer::IconView;
}

void Note::addedToScene(bool temporary)
{
	positionGrip();

    return ItemBase::addedToScene(temporary);
}
