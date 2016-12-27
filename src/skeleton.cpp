/*
 * Copyright 2016  Christoph Feck <cfeck@kde.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "skeleton.h"

#include <KDecoration2/DecoratedClient>
#include <KDecoration2/DecorationButtonGroup>
#include <KDecoration2/DecorationSettings>
#include <KDecoration2/DecorationShadow>

#include <KPluginFactory>

#include <QPainter>
#include <QPropertyAnimation>

#include <QtWidgets/qdrawutil.h>
#include <QBitmap>

K_PLUGIN_FACTORY_WITH_JSON(SkeletonDecorationFactory,
    "skeleton.json",
    registerPlugin<Skeleton::Decoration>();
    registerPlugin<Skeleton::ThemeLister>(QStringLiteral("themes"));
)

namespace Skeleton
{

static const unsigned char iconify_bits[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0x78, 0x00, 0x78, 0x00,
  0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const unsigned char close_bits[] = {
  0x00, 0x00, 0x84, 0x00, 0xce, 0x01, 0xfc, 0x00, 0x78, 0x00, 0x78, 0x00,
  0xfc, 0x00, 0xce, 0x01, 0x84, 0x00, 0x00, 0x00};

static const unsigned char maximize_bits[] = {
  0x00, 0x00, 0xfe, 0x01, 0xfe, 0x01, 0x86, 0x01, 0x86, 0x01, 0x86, 0x01,
  0x86, 0x01, 0xfe, 0x01, 0xfe, 0x01, 0x00, 0x00};

static const unsigned char minmax_bits[] = {
  0x7f, 0x00, 0x7f, 0x00, 0x63, 0x00, 0xfb, 0x03, 0xfb, 0x03, 0x1f, 0x03,
  0x1f, 0x03, 0x18, 0x03, 0xf8, 0x03, 0xf8, 0x03};

static const unsigned char question_bits[] = {
  0x00, 0x00, 0x78, 0x00, 0xcc, 0x00, 0xc0, 0x00, 0x60, 0x00, 0x30, 0x00,
  0x00, 0x00, 0x30, 0x00, 0x30, 0x00, 0x00, 0x00};

static const unsigned char above_on_bits[] = {
   0x00, 0x00, 0xfe, 0x01, 0xfe, 0x01, 0x30, 0x00, 0xfc, 0x00, 0x78, 0x00,
   0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static const unsigned char above_off_bits[] = {
   0x30, 0x00, 0x78, 0x00, 0xfc, 0x00, 0x30, 0x00, 0xfe, 0x01, 0xfe, 0x01,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static const unsigned char below_on_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x78, 0x00, 0xfc, 0x00,
   0x30, 0x00, 0xfe, 0x01, 0xfe, 0x01, 0x00, 0x00 };

static const unsigned char below_off_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x01, 0xfe, 0x01,
   0x30, 0x00, 0xfc, 0x00, 0x78, 0x00, 0x30, 0x00 };

static const unsigned char shade_on_bits[] = {
   0x00, 0x00, 0xfe, 0x01, 0xfe, 0x01, 0xfe, 0x01, 0x02, 0x01, 0x02, 0x01,
   0x02, 0x01, 0x02, 0x01, 0xfe, 0x01, 0x00, 0x00 };

static const unsigned char shade_off_bits[] = {
   0x00, 0x00, 0xfe, 0x01, 0xfe, 0x01, 0xfe, 0x01, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static const unsigned char pindown_white_bits[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x80, 0x1f, 0xa0, 0x03,
  0xb0, 0x01, 0x30, 0x01, 0xf0, 0x00, 0x70, 0x00, 0x20, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const unsigned char pindown_gray_bits[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c,
  0x00, 0x0e, 0x00, 0x06, 0x00, 0x00, 0x80, 0x07, 0xc0, 0x03, 0xe0, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const unsigned char pindown_dgray_bits[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xc0, 0x10, 0x70, 0x20, 0x50, 0x20,
  0x48, 0x30, 0xc8, 0x38, 0x08, 0x1f, 0x08, 0x18, 0x10, 0x1c, 0x10, 0x0e,
  0xe0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const unsigned char pindown_mask_bits[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xc0, 0x1f, 0xf0, 0x3f, 0xf0, 0x3f,
  0xf8, 0x3f, 0xf8, 0x3f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf0, 0x1f, 0xf0, 0x0f,
  0xe0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const unsigned char pinup_white_bits[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x11,
  0x3f, 0x15, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const unsigned char pinup_gray_bits[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x80, 0x0a, 0xbf, 0x0a, 0x80, 0x15, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const unsigned char pinup_dgray_bits[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x20, 0x40, 0x31, 0x40, 0x2e,
  0x40, 0x20, 0x40, 0x20, 0x7f, 0x2a, 0x40, 0x3f, 0xc0, 0x31, 0xc0, 0x20,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const unsigned char pinup_mask_bits[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x20, 0xc0, 0x31, 0xc0, 0x3f,
  0xff, 0x3f, 0xff, 0x3f, 0xff, 0x3f, 0xc0, 0x3f, 0xc0, 0x31, 0xc0, 0x20,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void drawColorBitmaps(QPainter *p, const QPalette &pal, int x, int y, int w, int h,
                      const uchar *lightColor, const uchar *midColor, const uchar *blackColor)
{
    const uchar *data[]={lightColor, midColor, blackColor};

    QColor colors[]={pal.color(QPalette::Light), pal.color(QPalette::Mid), Qt::black};

    int i;
    QSize s(w,h);
    for(i=0; i < 3; ++i){
        QBitmap b = QBitmap::fromData(s, data[i], QImage::Format_MonoLSB );
        b.setMask(b);
        p->setPen(colors[i]);
        p->drawPixmap(x, y, b);
    }
}
static void gradientFill(QPixmap *pixmap, const QColor &color1, const QColor &color2)
{
    QPainter p(pixmap);
    QLinearGradient gradient(0, 0, 0, pixmap->height());
    gradient.setColorAt(0.0, color1);
    gradient.setColorAt(1.0, color2);
    QBrush brush(gradient);
    p.fillRect(pixmap->rect(), brush);
}
void drawButtonBackground(QPixmap *pix,
        const QPalette &g, bool sunken)
{
    QPainter p;
    int w = pix->width();
    int h = pix->height();
    int x2 = w-1;
    int y2 = h-1;

    bool highcolor = true; //useGradients && (QPixmap::defaultDepth() > 8);
    QColor c = g.color( QPalette::Background );

    // Fill the background with a gradient if possible
    if (highcolor)
        gradientFill(pix, c.light(130), c.dark(130));
    else
        pix->fill(c);

    p.begin(pix);
    // outer frame
    p.setPen(g.color( QPalette::Mid ));
    p.drawLine(0, 0, x2, 0);
    p.drawLine(0, 0, 0, y2);
    p.setPen(g.color( QPalette::Light ));
    p.drawLine(x2, 0, x2, y2);
    p.drawLine(0, x2, y2, x2);
    p.setPen(g.color( QPalette::Dark ));
    p.drawRect(1, 1, w-3, h-3);
    p.setPen(sunken ? g.color( QPalette::Mid ) : g.color( QPalette::Light ));
    p.drawLine(2, 2, x2-2, 2);
    p.drawLine(2, 2, 2, y2-2);
    p.setPen(sunken ? g.color( QPalette::Light ) : g.color( QPalette::Mid ));
    p.drawLine(x2-2, 2, x2-2, y2-2);
    p.drawLine(2, x2-2, y2-2, x2-2);
}
void DecorationButton::setBitmap(const unsigned char *bitmap)
{
    delete deco;
    deco = 0;

    if (bitmap) {
        deco = new QPainterPath;
        deco->addRegion(QRegion( QBitmap::fromData(QSize( 10, 10 ), bitmap) ));
//      deco->setMask( *deco );
    }
}

QVariantMap ThemeLister::themes() const
{
    QVariantMap themes;
    themes.insert(tr("Skeleton"), QStringLiteral("Skeleton"));
    return themes;
}

Decoration::Decoration(QObject *parent, const QVariantList &args)
    : KDecoration2::Decoration(parent, args)
    , m_leftButtons(new KDecoration2::DecorationButtonGroup(this))
    , m_rightButtons(new KDecoration2::DecorationButtonGroup(this))
{
    if (!args.isEmpty()) {
        QVariantMap map = args.at(0).toMap();
        QVariantMap::const_iterator it = map.constFind(QStringLiteral("theme"));
        if (it != map.constEnd()) {
            Q_ASSERT(it.value().toString() == QLatin1String("Skeleton"));
        }
    }
}

Decoration::~Decoration()
{
}

void Decoration::init()
{
    connect(settings().data(), &KDecoration2::DecorationSettings::decorationButtonsLeftChanged, this, &Decoration::recreateButtons);
    connect(settings().data(), &KDecoration2::DecorationSettings::decorationButtonsRightChanged, this, &Decoration::recreateButtons);
    connect(settings().data(), &KDecoration2::DecorationSettings::reconfigured, this, &Decoration::recreateButtons);

    connect(settings().data(), &KDecoration2::DecorationSettings::fontChanged, this, &Decoration::updateButtons);
    connect(settings().data(), &KDecoration2::DecorationSettings::onAllDesktopsAvailableChanged, this, &Decoration::updateButtons);
    connect(client().data(), &KDecoration2::DecoratedClient::shadeableChanged, this, &Decoration::updateButtons);
    connect(client().data(), &KDecoration2::DecoratedClient::providesContextHelpChanged, this, &Decoration::updateButtons);
    connect(client().data(), &KDecoration2::DecoratedClient::minimizeableChanged, this, &Decoration::updateButtons);
    connect(client().data(), &KDecoration2::DecoratedClient::maximizeableChanged, this, &Decoration::updateButtons);
    connect(client().data(), &KDecoration2::DecoratedClient::closeableChanged, this, &Decoration::updateButtons);

    connect(client().data(), &KDecoration2::DecoratedClient::widthChanged, this, &Decoration::updateLayout);
    connect(client().data(), &KDecoration2::DecoratedClient::heightChanged, this, &Decoration::updateLayout);
    connect(client().data(), &KDecoration2::DecoratedClient::maximizedChanged, this, &Decoration::updateLayout);
    connect(client().data(), &KDecoration2::DecoratedClient::shadedChanged, this, &Decoration::updateLayout);

    connect(client().data(), &KDecoration2::DecoratedClient::paletteChanged, this, [this]() { update(); });
    connect(client().data(), &KDecoration2::DecoratedClient::iconChanged, this, [this]() { update(); });
    connect(client().data(), &KDecoration2::DecoratedClient::captionChanged, this, [this]() { update(m_captionRect); });
    connect(client().data(), &KDecoration2::DecoratedClient::activeChanged, this, [this]() { update(); });

    createButtons();
    updateButtons();
}

void Decoration::createButtons()
{
    QVector<KDecoration2::DecorationButtonType> buttons;

    buttons = settings()->decorationButtonsLeft();
    for (int i = 0; i < buttons.size(); ++i) {
        m_leftButtons->addButton(new DecorationButton(buttons.at(i), this, m_leftButtons));
    }
    buttons = settings()->decorationButtonsRight();
    for (int i = 0; i < buttons.size(); ++i) {
        m_rightButtons->addButton(new DecorationButton(buttons.at(i), this, m_rightButtons));
    }
}

void Decoration::deleteButtons()
{
    int size;
    size = m_leftButtons->buttons().size();
    while (size) {
        m_leftButtons->removeButton(m_leftButtons->buttons().at(--size));
    }
    size = m_rightButtons->buttons().size();
    while (size) {
        m_rightButtons->removeButton(m_rightButtons->buttons().at(--size));
    }
}

void Decoration::recreateButtons()
{
    deleteButtons();
    createButtons();
    updateButtons();
}

void Decoration::updateButtons()
{
    QVector<QPointer<KDecoration2::DecorationButton>> buttons;
    buttons.append(m_leftButtons->buttons());
    buttons.append(m_rightButtons->buttons());

    buttonSize = settings()->fontMetrics().height();

    for (int i = 0; i < buttons.size(); ++i) {
        DecorationButton *button = qobject_cast<DecorationButton *>(buttons.at(i));
        switch (button->type()) {
        case KDecoration2::DecorationButtonType::OnAllDesktops:
            button->setVisible(settings()->isOnAllDesktopsAvailable());
            break;
        case KDecoration2::DecorationButtonType::Shade:
            button->setVisible(client().data()->isShadeable());
            break;
        case KDecoration2::DecorationButtonType::ContextHelp:
            button->setVisible(client().data()->providesContextHelp());
            break;
        case KDecoration2::DecorationButtonType::Minimize:
            button->setVisible(client().data()->isMinimizeable());
            break;
        case KDecoration2::DecorationButtonType::Maximize:
            button->setVisible(client().data()->isMaximizeable());
            break;
        case KDecoration2::DecorationButtonType::Close:
            button->setVisible(client().data()->isCloseable());
            break;
        default:
            break;
        }
        button->setGeometry(QRect(0, 0, buttonSize, buttonSize));
    }

    updateLayout();
}

void Decoration::updateLayout()
{
    bool isMaximized = client().data()->isMaximized();
    setOpaque(isMaximized);

    if (!shadow().data()) {
        createShadow();
    }

    int frame = settings()->fontMetrics().height() / 5;
    side = 10; //(isMaximized ? 0 : frame);
    bottom = 10; //(isMaximized ? 0 : frame);
    top = 10; //(isMaximized ? 0 : frame);
    int titleHeight = qRound(1.25 * settings()->fontMetrics().height());
    setBorders(QMargins(side, titleHeight + top, side, (client().data()->isShaded() ? 0 : bottom)));

    m_frameRect = QRect(0, 0, size().width(), size().height());
    setTitleBar(QRect(side, top, size().width() - 2 * side, borderTop()));

    m_leftButtons->setPos(QPointF(side, (titleHeight + top - buttonSize)/2));
    m_rightButtons->setPos(QPointF(size().width() - m_rightButtons->geometry().width() - side, (titleHeight + top - buttonSize)/2));

    int left = m_leftButtons->geometry().x() + m_leftButtons->geometry().width();
    m_captionRect = QRect(left, 0, m_rightButtons->geometry().x() - left, titleHeight + top);
}

void Decoration::createShadow()
{
    QImage image(65, 65, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(0, 0, 0, 0));

    QPainter painter;
    painter.begin(&image);
    painter.setRenderHints(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    QRect rect = image.rect();
    for (int i = 0; i < 32; ++i) {
        painter.setBrush(QColor(0, 0, 0, i / 3));
        painter.drawRoundedRect(rect, 20, 20);
        rect.adjust(1, 1, -1, -1);
    }
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(QRect(24, 20, 17, 17), QColor(0, 0, 0, 0));
    painter.end();

    KDecoration2::DecorationShadow *decorationShadow = new KDecoration2::DecorationShadow();
    decorationShadow->setPadding(QMargins(24, 20, 24, 28));
    decorationShadow->setInnerShadowRect(QRect(32, 32, 1, 1));
    decorationShadow->setShadow(image);
    setShadow(QSharedPointer<KDecoration2::DecorationShadow>(decorationShadow));
}

static void drawShadowRect(QPainter *p, const QRect &rect)
{
    int margin = 2;

    QRect r;
    r = rect; r.setHeight(margin);
    p->fillRect(r, QColor(255, 255, 255, 70));
    r = rect; r.setWidth(margin); r.setTop(r.top() + margin);
    p->fillRect(r, QColor(255, 255, 255, 70));
    r = rect; r.setLeft(r.width() - margin); r.setBottom(r.bottom() - margin);
    p->fillRect(r, QColor(0, 0, 0, 50));
    r = rect; r.setTop(r.height() - margin);
    p->fillRect(r, QColor(0, 0, 0, 50));
}

void Decoration::paint(QPainter *painter, const QRect &repaintArea)
{
    KDecoration2::ColorGroup colorGroup = (client().data()->isActive() ? KDecoration2::ColorGroup::Active : KDecoration2::ColorGroup::Inactive);
//    painter->fillRect(m_frameRect, client().data()->color(colorGroup, KDecoration2::ColorRole::Frame));
    QColor color = client().data()->color(colorGroup, KDecoration2::ColorRole::TitleBar);
    if (!client().data()->isActive()) {
        color = client().data()->color(QPalette::Active, QPalette::Window);
    }
    if (!client().data()->isMaximized()) {
        color.setAlphaF(0.9);
    }
    painter->setRenderHints(QPainter::Antialiasing, false);
    painter->fillRect(m_frameRect, color);

    // Obtain widget bounds.
    QRect r(m_frameRect);
    int w  = r.width();
    int h  = r.height();

    QPalette g = client().data()->palette();
    QColor c2 = client().data()->color(colorGroup, KDecoration2::ColorRole::Frame);
    int leftFrameStart = m_captionRect.height()+26;

    // left side
    painter->setPen(c2);
    QPolygon a;
    QBrush brush( c2, Qt::SolidPattern );
    a.setPoints( 4, 0,            leftFrameStart+top,
                    side, leftFrameStart,
                    side, h,
                    0,            h);
    painter->drawPolygon( a );
    QPainterPath path;
    path.addPolygon(a);
    painter->fillPath(path, brush);
    // Finish drawing the titlebar extension
    painter->setPen(Qt::black);
    painter->drawLine(0, leftFrameStart+top, side, leftFrameStart);
    // right side
    painter->fillRect(w-side, 0,
               side, h,
               c2 );

    // Fill with frame color behind RHS buttons
    painter->fillRect( m_rightButtons->geometry().x(), 0, m_rightButtons->geometry().width(), m_captionRect.height(), c2);
    // Draw titlebar colour separator line
    painter->setPen(g.color( QPalette::Dark ));
    painter->drawLine(m_rightButtons->geometry().x()-1, 0, m_rightButtons->geometry().x()-1, m_captionRect.height());

    // Draw the bottom handle if required
    if (!client().data()->isMaximized())
    {
        int grabWidth = 2*side+12;
            qDrawShadePanel(painter, 0, h-bottom+1, grabWidth, bottom,
                            g, false, 1, &g.brush(QPalette::Mid));
            qDrawShadePanel(painter, grabWidth, h-bottom+1, w-2*grabWidth, bottom,
                            g, false, 1, client().data()->isActive() ?
                            &g.brush(QPalette::Background) :
                            &g.brush(QPalette::Mid));
            qDrawShadePanel(painter, w-grabWidth, h-bottom+1, grabWidth, bottom,
                            g, false, 1, &g.brush(QPalette::Mid));
    } else
        {
            painter->fillRect(0, h-bottom, w, bottom, c2);
        }

    drawShadowRect(painter, m_frameRect);

    // Draw an outer black frame
    painter->setPen(Qt::black);
    painter->drawRect(0,0,w-1,h-1);

    // Draw a frame around the wrapped widget.
    painter->setPen( g.color( QPalette::Dark ) );
    painter->drawRect( side-1,m_captionRect.height()-1,w-2*side+1,h-m_captionRect.height()-bottom+1 );

    QRectF clipRect = painter->clipBoundingRect();
    if (clipRect.isEmpty() || clipRect.intersects(m_captionRect)) {
        QRect captionRect = m_captionRect.adjusted(4, 0, -4, 0);
        QString caption = settings()->fontMetrics().elidedText(client().data()->caption(), Qt::ElideMiddle, captionRect.width());
//        painter->fillRect(m_captionRect, client().data()->color(colorGroup, KDecoration2::ColorRole::TitleBar));
//        painter->fillRect(m_captionRect, color);
        painter->setPen(client().data()->color(colorGroup, KDecoration2::ColorRole::Foreground));
        painter->setFont(settings()->font());
        painter->drawText(captionRect, Qt::AlignVCenter, caption);
    }

    m_leftButtons->paint(painter, repaintArea);
    m_rightButtons->paint(painter, repaintArea);
}

void Decoration::updateHoverAnimation(qreal /*hoverProgress*/, const QRect &updateRect)
{
    update(updateRect);
}

DecorationButton::DecorationButton(KDecoration2::DecorationButtonType type, Decoration *decoration, QObject *parent)
    : KDecoration2::DecorationButton(type, decoration, parent)
    , m_hoverProgress(0.0)
{
    connect(this, &KDecoration2::DecorationButton::hoveredChanged, this, [this](bool hovered) { startHoverAnimation(hovered ? 1.0 : 0.0); });

    deco        = NULL;
}

DecorationButton::~DecorationButton()
{
    if (deco)
        delete deco;
}

qreal DecorationButton::hoverProgress() const
{
    return m_hoverProgress;
}

void DecorationButton::setHoverProgress(qreal hoverProgress)
{
    if (m_hoverProgress != hoverProgress) {
        m_hoverProgress = hoverProgress;
        qobject_cast<Decoration *>(decoration())->updateHoverAnimation(m_hoverProgress, geometry().toRect().adjusted(-1, -1, 1, 1));
    }
}

void DecorationButton::startHoverAnimation(qreal endValue)
{
    QPropertyAnimation *hoverAnimation = m_hoverAnimation;
    const int hoverDuration = 150;

    if (hoverAnimation) {
        if (hoverAnimation->endValue() == endValue) {
            return;
        }
        hoverAnimation->stop();
    } else if (m_hoverProgress != endValue) {
        if (hoverDuration < 10) {
            setHoverProgress(endValue);
            return;
        }
        hoverAnimation = new QPropertyAnimation(this, "hoverProgress");
        m_hoverAnimation = hoverAnimation;
    } else {
        return;
    }
    hoverAnimation->setEasingCurve(QEasingCurve::OutQuad);
    hoverAnimation->setStartValue(m_hoverProgress);
    hoverAnimation->setEndValue(endValue);
    hoverAnimation->setDuration(1 + qRound(hoverDuration * qAbs(m_hoverProgress - endValue)));
    hoverAnimation->start();
}

void DecorationButton::paint(QPainter *painter, const QRect &/*repaintArea*/)
{
//    KDecoration2::ColorGroup colorGroup = decoration()->client().data()->isActive() ? KDecoration2::ColorGroup::Active : KDecoration2::ColorGroup::Inactive;
    if (type() == KDecoration2::DecorationButtonType::Menu) {
        decoration()->client().data()->icon().paint(painter, geometry().toRect());
    } else {
        QColor buttonColor(100, 100, 100);
        if (type() == KDecoration2::DecorationButtonType::Close) {
            buttonColor = QColor(150, 100, 100);
        } else if (isCheckable() && isChecked()) {
            buttonColor = QColor(150, 150, 100);
        }
        painter->fillRect(geometry().adjusted(1, 1, -1, -1), buttonColor);
    }

    if (isPressed()) {
        painter->fillRect(geometry(), QColor(0, 0, 0, 50));
    } else {
        painter->fillRect(geometry().adjusted(-2, -2, 2, 2), QColor(255, 255, 255, 100 * m_hoverProgress));
    }
}

} // namespace Skeleton

#include "skeleton.moc"
