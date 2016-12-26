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

K_PLUGIN_FACTORY_WITH_JSON(SkeletonDecorationFactory,
    "skeleton.json",
    registerPlugin<Skeleton::Decoration>();
    registerPlugin<Skeleton::ThemeLister>(QStringLiteral("themes"));
)

namespace Skeleton
{

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
        const int buttonSize = settings()->fontMetrics().height();
        button->setGeometry(QRect(0, 0, 16, 16));
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

    m_leftButtons->setPos(QPointF(side, (titleHeight + top)/2-8));
    m_rightButtons->setPos(QPointF(size().width() - m_rightButtons->geometry().width() - side, (titleHeight + top)/2-8));

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
}

DecorationButton::~DecorationButton()
{
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
