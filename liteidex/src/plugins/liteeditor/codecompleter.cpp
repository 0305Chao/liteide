#include "codecompleter.h"
#include "faketooltip.h"
#include <QListView>
#include <QStandardItemModel>
#include <QKeyEvent>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QSortFilterProxyModel>
#include <QDesktopWidget>
#include <QItemDelegate>
#include <QDebug>

CodeCompleter::CodeCompleter(QObject *parent) :
    QCompleter(parent)
{
    this->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    this->setWrapAround(true);
    m_popup = new QListView;
    m_popup->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_popup->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_popup->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_popup->setSelectionMode(QAbstractItemView::SingleSelection);
    m_popup->setModelColumn(0);
    this->setPopup(m_popup);
    m_proxy = new QSortFilterProxyModel(this);
}

CodeCompleter::~CodeCompleter()
{
}

void CodeCompleter::setModel(QAbstractItemModel *c)
{
    m_proxy->setSourceModel(c);
    QCompleter::setModel(m_proxy);
}

void CodeCompleter::setSeparator(const QString &separator)
{
    m_seperator = separator;
}

void CodeCompleter::setCompletionPrefix(const QString &prefix)
{
    QCompleter::setCompletionPrefix(prefix);
}

QString CodeCompleter::completionPrefix() const
{
    return QCompleter::completionPrefix();
}

void CodeCompleter::updateFilter()
{

}

bool CodeCompleter::eventFilter(QObject *o, QEvent *e)
{
    switch (e->type()) {
    case QEvent::KeyPress: {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        switch (ke->key()) {
        case Qt::Key_Up:
            if ( this->popup() && this->popup()->isVisible()) {
                QModelIndex index = this->popup()->currentIndex();
                if (index.isValid() && (index.row() == 0)) {
                    this->popup()->setCurrentIndex(this->popup()->model()->index(this->popup()->model()->rowCount()-1,0));
                    return true;
                }
            }
            break;
        case Qt::Key_Down:
            if (this->popup() && this->popup()->isVisible()) {
                QModelIndex index = this->popup()->currentIndex();
                if (index.isValid() && (index.row() == this->popup()->model()->rowCount()-1)) {
                    this->popup()->setCurrentIndex(this->popup()->model()->index(0,0));
                    return true;
                }
            }
            break;
        default:
            break;
        }
        break;
    }
    default:
        break;
    }
    return QCompleter::eventFilter(o,e);
}

QString CodeCompleter::separator() const
{
    return m_seperator;
}

QStringList CodeCompleter::splitPath(const QString &path) const
{
    if (m_seperator.isNull()) {
        return QCompleter::splitPath(path);
    }
    return path.split(m_seperator);
}

QString CodeCompleter::pathFromIndex(const QModelIndex &index) const
{
    if (m_seperator.isNull()) {
        return QCompleter::pathFromIndex(index);
    }

    // navigate up and accumulate data
    QStringList dataList;
    for (QModelIndex i = index; i.isValid(); i = i.parent()) {
        dataList.prepend(model()->data(i, completionRole()).toString());
    }
    return dataList.join(m_seperator);
}

class CodeCompleterListView : public QListView
{
public:
    CodeCompleterListView(QWidget *parent = 0);

    QSize calculateSize() const;
    QPoint infoFramePos() const;
};

CodeCompleterListView::CodeCompleterListView(QWidget *parent)
    : QListView(parent)
{
    setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
}

QSize CodeCompleterListView::calculateSize() const
{
    static const int maxVisibleItems = 10;

    // Determine size by calculating the space of the visible items
    const int visibleItems = qMin(model()->rowCount(), maxVisibleItems);
    const int firstVisibleRow = verticalScrollBar()->value();

    const QStyleOptionViewItem &option = viewOptions();
    QSize shint;
    for (int i = 0; i < visibleItems; ++i) {
        QSize tmp = itemDelegate()->sizeHint(option, model()->index(i + firstVisibleRow, 0));
        if (shint.width() < tmp.width())
            shint = tmp;
    }
    shint.rheight() *= visibleItems;
    return shint;
}

QPoint CodeCompleterListView::infoFramePos() const
{
    const QRect &r = rectForIndex(currentIndex());
    QPoint p((parentWidget()->mapToGlobal(
                    parentWidget()->rect().topRight())).x() + 3,
            mapToGlobal(r.topRight()).y() - verticalOffset()
            );
    return p;
}

class CodeCompleterItemDelegate : public QItemDelegate
{
public:
    CodeCompleterItemDelegate(QAbstractItemView *view)
        : QItemDelegate(view), view(view) { }
    void paint(QPainter *p, const QStyleOptionViewItem& opt, const QModelIndex& idx) const {
        QStyleOptionViewItem optCopy = opt;
        optCopy.showDecorationSelected = true;
        if (view->currentIndex() == idx)
            optCopy.state |= QStyle::State_HasFocus;
        QItemDelegate::paint(p, optCopy, idx);
    }

private:
    QAbstractItemView *view;
};

CodeCompleterProxyModel::CodeCompleterProxyModel(QObject *parent)
    : QAbstractListModel(parent),m_model(0)
{
    m_seperator = ".";
}

int CodeCompleterProxyModel::rowCount(const QModelIndex &) const
{
    return m_items.size();
}

QVariant CodeCompleterProxyModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= m_items.size())
        return QVariant();
    QStandardItem *item = m_items[index.row()];
    return item->data(role);
}

void CodeCompleterProxyModel::setSourceModel(QStandardItemModel *model)
{
    m_model = model;
}

QStandardItemModel *CodeCompleterProxyModel::sourceModel() const
{
    return m_model;
}

QStandardItem *CodeCompleterProxyModel::item(const QModelIndex &index) const
{
    if (index.row() >= m_items.size())
        return 0;
    return m_items[index.row()];
}

bool CodeCompleterProxyModel::splitFilter(const QString &filter, QModelIndex &parent, QString &prefix)
{
    QStringList filterList = filter.split(".");
    if (filterList.size() == 1) {
        parent = QModelIndex();
        prefix = filter;
        return true;
    }
    prefix = filterList.last();
    filterList.removeLast();
    QStandardItem *root = 0;
    QStandardItem *item = 0;
    foreach (QString word, filterList) {
        item = 0;
        QModelIndex parent = m_model->indexFromItem(root);
        for (int i = 0; i < m_model->rowCount(parent); i++) {
            QModelIndex index = m_model->index(i,0,parent);
            item = m_model->itemFromIndex(index);
            if (item->text() == word) {
                break;
            }
        }
        if (item == 0) {
            break;
        }
        root = item;
    }
    if (!item) {
        return false;
    }
    parent = m_model->indexFromItem(item);
    return true;
}

int CodeCompleterProxyModel::filter(const QString &filter, int cs)
{
    if (!m_model) {
        return 0;
    }
    QModelIndex parentIndex;
    QString prefix;
    if (!splitFilter(filter,parentIndex,prefix)) {
        return 0;
    }
    if (prefix.isEmpty()) {
        m_items.clear();
        int count = m_model->rowCount(parentIndex);
        for (int i = 0; i < count; i++) {
            QModelIndex index = m_model->index(i,0,parentIndex);
            QStandardItem *item = m_model->itemFromIndex(index);
            m_items.append(item);
        }
        return m_items.size();
    }
    QString keyRegExp;
    keyRegExp += QLatin1Char('^');
    bool first = true;
    const QLatin1String uppercaseWordContinuation("[a-z0-9_]*");
    const QLatin1String lowercaseWordContinuation("(?:[a-zA-Z0-9]*_)?");
    foreach (const QChar &c, prefix) {
        if (cs == LiteApi::CaseInsensitive ||
            (cs == LiteApi::FirstLetterCaseSensitive && !first)) {

            keyRegExp += QLatin1String("(?:");
            if (!first)
                keyRegExp += uppercaseWordContinuation;
            keyRegExp += QRegExp::escape(c.toUpper());
            keyRegExp += QLatin1Char('|');
            if (!first)
                keyRegExp += lowercaseWordContinuation;
            keyRegExp += QRegExp::escape(c.toLower());
            keyRegExp += QLatin1Char(')');
        } else {
            if (!first) {
                if (c.isUpper())
                    keyRegExp += uppercaseWordContinuation;
                else
                    keyRegExp += lowercaseWordContinuation;
            }
            keyRegExp += QRegExp::escape(c);
        }

        first = false;
    }
    QRegExp regExp(keyRegExp);

    m_items.clear();
    int count = m_model->rowCount(parentIndex);
    for (int i = 0; i < count; i++) {
        QModelIndex index = m_model->index(i,0,parentIndex);
        QStandardItem *item = m_model->itemFromIndex(index);
        if (regExp.indexIn(item->text()) == 0) {
            m_items.append(item);
        }
    }
    return m_items.size();
}

void CodeCompleterProxyModel::setSeparator(const QString &separator)
{
    m_seperator = separator;
}

QString CodeCompleterProxyModel::separator() const
{
    return m_seperator;
}

CodeCompleterEx::CodeCompleterEx(QObject *parent)
    : QObject(parent), m_widget(0)
{
    maxVisibleItems = 10;
    m_eatFocusOut = true;
    m_hiddenBecauseNoMatch = false;
    m_cs = Qt::CaseInsensitive;
    m_wrap = true;
    m_popup = new CodeCompleterListView;
    m_popup->setUniformItemSizes(true);
    m_popup->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_popup->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_popup->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_popup->setSelectionMode(QAbstractItemView::SingleSelection);
    m_popup->setItemDelegate(new CodeCompleterItemDelegate(m_popup));
    m_popup->setModelColumn(0);

    m_popup->setParent(0, Qt::Popup);
    m_popup->setFocusPolicy(Qt::NoFocus);

    m_popup->installEventFilter(this);

    m_proxy = new CodeCompleterProxyModel(this);
    m_popup->setModel(m_proxy);

    QObject::connect(m_popup, SIGNAL(clicked(QModelIndex)),
                     this, SLOT(completerActivated(QModelIndex)));
    QObject::connect(m_popup, SIGNAL(activated(QModelIndex)),
                     m_popup, SLOT(hide()));

}

void CodeCompleterEx::setModel(QStandardItemModel *c)
{
    m_proxy->setSourceModel(c);
}

QAbstractItemModel *CodeCompleterEx::model() const
{
    return m_proxy->sourceModel();
}

void CodeCompleterEx::setSeparator(const QString &separator)
{
    m_proxy->setSeparator(separator);
}

QString CodeCompleterEx::separator() const
{
    return m_proxy->separator();
}

void CodeCompleterEx::setCompletionPrefix(const QString &prefix)
{
    if (m_prefix == prefix) {
        return;
    }
    m_prefix = prefix;
    if (m_proxy->filter(prefix,m_cs) <= 0) {
        if (m_popup->isVisible()) {
            m_popup->close();
        }
        return;
    }
    m_popup->reset();
}

QString CodeCompleterEx::completionPrefix() const
{
    return m_prefix;
}

void CodeCompleterEx::updateFilter()
{
    if (m_proxy->filter(m_prefix,m_cs) <= 0) {
        if (m_popup->isVisible()) {
            m_popup->close();
        }
        return;
    }
    m_popup->reset();
}

void CodeCompleterEx::complete(const QRect &rect)
{
    if (m_proxy->rowCount() == 0) {
        return;
    }

    const QRect screen = QApplication::desktop()->availableGeometry(m_widget);
    Qt::LayoutDirection dir = m_widget->layoutDirection();
    QPoint pos;
    int rh, w;
    int h = (m_popup->sizeHintForRow(0) * qMin(maxVisibleItems, m_popup->model()->rowCount()) + 3)+3;
    QScrollBar *hsb = m_popup->horizontalScrollBar();
    if (hsb && hsb->isVisible())
        h += m_popup->horizontalScrollBar()->sizeHint().height();

    if (rect.isValid()) {
        rh = rect.height();
        w = rect.width();
        pos = m_widget->mapToGlobal(dir == Qt::RightToLeft ? rect.bottomRight() : rect.bottomLeft());
    } else {
        rh = m_widget->height();
        pos = m_widget->mapToGlobal(QPoint(0, m_widget->height() - 2));
        w = m_widget->width();
    }

    if (w > screen.width())
        w = screen.width();
    if ((pos.x() + w) > (screen.x() + screen.width()))
        pos.setX(screen.x() + screen.width() - w);
    if (pos.x() < screen.x())
        pos.setX(screen.x());

    int top = pos.y() - rh - screen.top() + 2;
    int bottom = screen.bottom() - pos.y();
    h = qMax(h, m_popup->minimumHeight());
    if (h > bottom) {
        h = qMin(qMax(top, bottom), h);

        if (top > bottom)
            pos.setY(pos.y() - h - rh + 2);
    }

    m_popup->setGeometry(pos.x(), pos.y(), w, h);

    if (!m_popup->isVisible())
        m_popup->show();
}

QWidget *CodeCompleterEx::widget() const
{
    return m_widget;
}

void CodeCompleterEx::setWidget(QWidget *widget)
{
    if (m_widget == widget) {
        return;
    }
    if (m_widget) {
        m_widget->removeEventFilter(this);
    }
    m_widget = widget;
//    Qt::FocusPolicy origPolicy = Qt::NoFocus;
//    if (widget)
//        origPolicy = widget->focusPolicy();
//    m_popup->setParent(0, Qt::Popup);
//    m_popup->setFocusPolicy(Qt::NoFocus);
//    if (widget)
//        widget->setFocusPolicy(origPolicy);
    if (m_widget) {
        m_widget->installEventFilter(this);
        m_popup->setFocusProxy(m_widget);
    }
}

QModelIndex CodeCompleterEx::currentIndex() const
{
    return m_popup->currentIndex();
}

QString CodeCompleterEx::currentCompletion() const
{
    QModelIndex index = m_popup->currentIndex();
    if (index.isValid()) {
        QStandardItem *item = m_proxy->item(index);
        if (item) {
            return item->text();
        }
    }
    return QString();
}

void CodeCompleterEx::setCaseSensitivity(Qt::CaseSensitivity cs)
{
    m_cs = cs;
}

Qt::CaseSensitivity CodeCompleterEx::caseSensitivity() const
{
    return m_cs;
}

QAbstractItemView *CodeCompleterEx::popup() const
{
    return m_popup;
}

QAbstractItemModel *CodeCompleterEx::completionModel() const
{
    return m_proxy;
}

void CodeCompleterEx::setWrapAround(bool wrap)
{
    m_wrap = wrap;
}

bool CodeCompleterEx::wrapAround() const
{
    return m_wrap;
}

void CodeCompleterEx::completerActivated(QModelIndex index)
{
    if (m_popup->isVisible()) {
        m_popup->close();
    }
    emit activated(index);
}

bool CodeCompleterEx::eventFilter(QObject *o, QEvent *e)
{
    if (m_eatFocusOut && o == m_widget && e->type() == QEvent::FocusOut) {
        m_hiddenBecauseNoMatch = false;
        if (m_popup && m_popup->isVisible())
            return true;
    }

    if (o != m_popup)
        return QObject::eventFilter(o, e);

    switch (e->type()) {
    case QEvent::KeyPress: {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);

        QModelIndex curIndex = m_popup->currentIndex();
        //QModelIndexList selList = m_popup->selectionModel()->selectedIndexes();

        const int key = ke->key();
        // In UnFilteredPopup mode, select the current item
//        if ((key == Qt::Key_Up || key == Qt::Key_Down) && selList.isEmpty() && curIndex.isValid()
//            && m_mode == QCompleter::UnfilteredPopupCompletion) {
//            m_popup->setCurrentIndex(curIndex);
//              return true;
//        }

        // Handle popup navigation keys. These are hardcoded because up/down might make the
        // widget do something else (lineedit cursor moves to home/end on mac, for instance)
        switch (key) {
        case Qt::Key_End:
        case Qt::Key_Home:
            if (ke->modifiers() & Qt::ControlModifier)
                return false;
            break;

        case Qt::Key_Up:
            if (curIndex.row() == 0) {
                if (m_wrap) {
                    int rowCount = m_proxy->rowCount();
                    QModelIndex lastIndex = m_proxy->index(rowCount - 1, 0);
                    m_popup->setCurrentIndex(lastIndex);
                }
                return true;
            }
            return false;

        case Qt::Key_Down:
            if (curIndex.row() == m_proxy->rowCount() - 1) {
                if (m_wrap) {
                    QModelIndex firstIndex = m_proxy->index(0, 0);
                    m_popup->setCurrentIndex(firstIndex);
                }
                return true;
            }
            return false;

        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
            return false;
        }

        // Send the event to the widget. If the widget accepted the event, do nothing
        // If the widget did not accept the event, provide a default implementation
        m_eatFocusOut = false;
        (static_cast<QObject *>(m_widget))->event(ke);
        m_eatFocusOut = true;
        if (!m_widget || e->isAccepted() || !m_popup->isVisible()) {
            // widget lost focus, hide the popup
            if (m_widget && (!m_widget->hasFocus()
#ifdef QT_KEYPAD_NAVIGATION
                || (QApplication::keypadNavigationEnabled() && !m_widget->hasEditFocus())
#endif
                ))
                m_popup->hide();
            if (e->isAccepted())
                return true;
        }

        // default implementation for keys not handled by the widget when popup is open
        switch (key) {
#ifdef QT_KEYPAD_NAVIGATION
        case Qt::Key_Select:
            if (!QApplication::keypadNavigationEnabled())
                break;
#endif
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Tab:
            m_popup->hide();
            if (curIndex.isValid())
                //m__q_complete(curIndex);
                this->completerActivated(curIndex);
            break;

        case Qt::Key_F4:
            if (ke->modifiers() & Qt::AltModifier)
                m_popup->hide();
            break;

        case Qt::Key_Backtab:
        case Qt::Key_Escape:
            m_popup->hide();
            break;

        default:
            break;
        }

        return true;
    }

#ifdef QT_KEYPAD_NAVIGATION
    case QEvent::KeyRelease: {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        if (QApplication::keypadNavigationEnabled() && ke->key() == Qt::Key_Back) {
            // Send the event to the 'widget'. This is what we did for KeyPress, so we need
            // to do the same for KeyRelease, in case the widget's KeyPress event set
            // up something (such as a timer) that is relying on also receiving the
            // key release. I see this as a bug in Qt, and should really set it up for all
            // the affected keys. However, it is difficult to tell how this will affect
            // existing code, and I can't test for every combination!
            m_eatFocusOut = false;
            static_cast<QObject *>(m_widget)->event(ke);
            m_eatFocusOut = true;
        }
        break;
    }
#endif
    case QEvent::MouseButtonPress: {
#ifdef QT_KEYPAD_NAVIGATION
        if (QApplication::keypadNavigationEnabled()) {
            // if we've clicked in the widget (or its descendant), let it handle the click
            QWidget *source = qobject_cast<QWidget *>(o);
            if (source) {
                QPoint pos = source->mapToGlobal((static_cast<QMouseEvent *>(e))->pos());
                QWidget *target = QApplication::widgetAt(pos);
                if (target && (m_widget->isAncestorOf(target) ||
                    target == m_widget)) {
                    m_eatFocusOut = false;
                    static_cast<QObject *>(target)->event(e);
                    m_eatFocusOut = true;
                    return true;
                }
            }
        }
#endif
        if (!m_popup->underMouse()) {
            m_popup->hide();
            return true;
        }
        }
        return false;

    case QEvent::InputMethod:
    case QEvent::ShortcutOverride:
        QApplication::sendEvent(m_widget, e);
        break;

    default:
        return false;
    }
    return false;
}
