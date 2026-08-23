#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QToolBar>
#include <QToolButton>
#include <QStatusBar>
#include <QFileDialog>
#include <QPixmap>
#include <QImageReader>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QDir>
#include <QTimer>
#include <QMovie>
#include <QSvgRenderer>
#include <QPainter>
#include <QShortcut>
#include <QSettings>
#include <QClipboard>
#include <QFrame>
#include <QStandardPaths>
#include <QResizeEvent>
#include <QTransform>
#include <QFile>
#include <QGuiApplication>
#include <QScreen>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QEasingCurve>
#include <QIcon>
#include <QtConcurrent/QtConcurrentRun>
#include <QFutureWatcher>
#include <algorithm>
#include <vector>
#include <unordered_map>

/* ── cache key / loaded state ── */
enum LoadState { LS_NONE, LS_LOADING, LS_READY };

/* ──────────────────────────────────────────────────
   INLINE SVG ICONS
────────────────────────────────────────────────── */
static QIcon mkIcon(const char* svg) {
    QByteArray data(svg);
    QSvgRenderer r{data};
    QPixmap px(22, 22); px.fill(Qt::transparent);
    QPainter p(&px); r.render(&p); p.end();
    return QIcon(px);
}

static const char* S_OPEN  = R"(<svg viewBox="0 0 24 24" fill="none" stroke="#c0c0c0" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"/></svg>)";
static const char* S_FDIR = R"(<svg viewBox="0 0 24 24" fill="none" stroke="#c0c0c0" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"/><line x1="12" y1="11" x2="12" y2="17"/><line x1="9" y1="14" x2="15" y2="14"/></svg>)";
static const char* S_PREV = R"(<svg viewBox="0 0 24 24" fill="none" stroke="#c0c0c0" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="15 18 9 12 15 6"/></svg>)";
static const char* S_NEXT = R"(<svg viewBox="0 0 24 24" fill="none" stroke="#c0c0c0" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="9 18 15 12 9 6"/></svg>)";
static const char* S_ZIN  = R"(<svg viewBox="0 0 24 24" fill="none" stroke="#c0c0c0" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><circle cx="11" cy="11" r="8"/><line x1="21" y1="21" x2="16.65" y2="16.65"/><line x1="11" y1="8" x2="11" y2="14"/><line x1="8" y1="11" x2="14" y2="11"/></svg>)";
static const char* S_ZOUT = R"(<svg viewBox="0 0 24 24" fill="none" stroke="#c0c0c0" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><circle cx="11" cy="11" r="8"/><line x1="21" y1="21" x2="16.65" y2="16.65"/><line x1="8" y1="11" x2="14" y2="11"/></svg>)";
static const char* S_FIT  = R"(<svg viewBox="0 0 24 24" fill="none" stroke="#c0c0c0" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><polyline points="15 3 21 3 21 9"/><polyline points="9 21 3 21 3 15"/><line x1="21" y1="3" x2="14" y2="10"/><line x1="3" y1="21" x2="10" y2="14"/></svg>)";
static const char* S_ROTL = R"(<svg viewBox="0 0 24 24" fill="none" stroke="#c0c0c0" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><path d="M1 4v6h6"/><path d="M3.51 15a9 9 0 1 0 2.13-9.36L1 10"/></svg>)";
static const char* S_ROTR = R"(<svg viewBox="0 0 24 24" fill="none" stroke="#c0c0c0" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><path d="M23 4v6h-6"/><path d="M20.49 15a9 9 0 1 1-2.13-9.36L23 10"/></svg>)";
static const char* S_PLAY = R"(<svg viewBox="0 0 24 24" fill="none" stroke="#c0c0c0" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><polygon points="5 3 19 12 5 21 5 3"/></svg>)";
static const char* S_PAUS = R"(<svg viewBox="0 0 24 24" fill="none" stroke="#c0c0c0" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><rect x="6" y="4" width="4" height="16"/><rect x="14" y="4" width="4" height="16"/></svg>)";
static const char* S_COPY = R"(<svg viewBox="0 0 24 24" fill="none" stroke="#c0c0c0" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><rect x="9" y="9" width="13" height="13" rx="2" ry="2"/><path d="M5 15H4a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2h9a2 2 0 0 1 2 2v1"/></svg>)";
static const char* S_FULL = R"(<svg viewBox="0 0 24 24" fill="none" stroke="#c0c0c0" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><path d="M8 3H5a2 2 0 0 0-2 2v3"/><path d="M21 8V5a2 2 0 0 0-2-2h-3"/><path d="M16 21h3a2 2 0 0 0 2-2v-3"/><path d="M3 16v3a2 2 0 0 0 2 2h3"/></svg>)";
static const char* S_TRSH = R"(<svg viewBox="0 0 24 24" fill="none" stroke="#c0c0c0" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><polyline points="3 6 5 6 21 6"/><path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2"/></svg>)";

/* ──────────────────────────────────────────────────
   DARK STYLESHEET
────────────────────────────────────────────────── */
static const char* STYLE = R"(
* { font-family:'Segoe UI Variable Display','Segoe UI',Arial,sans-serif; }
QMainWindow, QWidget { background:#18181b; color:#d4d4d8; }
QScrollArea { background:#18181b; border:none; }
QStatusBar {
    background:#1f1f23; color:#a1a1aa;
    border-top:1px solid #2a2a2f;
    padding:0 12px; font-size:11px;
}
QStatusBar QLabel { color:#a1a1aa; }
QToolBar {
    background:#1f1f23;
    border-bottom:1px solid #2a2a2f;
    spacing:2px; padding:4px 8px;
}
QToolButton {
    background:transparent; color:#d4d4d8;
    border:1px solid transparent; border-radius:8px;
    padding:4px 8px; min-width:32px; min-height:30px;
}
QToolButton:hover {
    background:#2a2a2f; border:1px solid #3a3a40; color:#f4f4f5;
}
QToolButton:pressed {
    background:#333338; border:1px solid #52525b; color:#f4f4f5;
}
QToolButton:disabled {
    color:#3f3f46; background:transparent; border:1px solid transparent;
}
QFrame#sep { background:#3a3a40; min-width:1px; max-width:1px; margin:5px 4px; }
QLabel#zl {
    color:#71717a; font-size:10px;
    font-family:'Cascadia Code',Consolas,monospace;
    padding:0 6px; min-width:50px;
}
QScrollBar:vertical, QScrollBar:horizontal {
    background:#1f1f23; width:6px; height:6px; border:none;
}
QScrollBar::handle { background:#3f3f46; border-radius:3px; min-height:24px; }
QScrollBar::handle:hover { background:#52525b; }
QScrollBar::add-line, QScrollBar::sub-line { width:0; height:0; }
)";

/* ──────────────────────────────────────────────────
   CANVAS
────────────────────────────────────────────────── */
class Canvas : public QLabel {
    Q_OBJECT
    Q_PROPERTY(double zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
public:
    explicit Canvas(QWidget* parent = nullptr) : QLabel(parent) {
        setAlignment(Qt::AlignCenter);
        setMinimumSize(1, 1);
        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        setStyleSheet("background:#18181b;");
        m_smoothTimer.setSingleShot(true);
        connect(&m_smoothTimer, &QTimer::timeout, this, [this]{ applySmooth(); });
        m_zoomAnim = new QPropertyAnimation(this, "zoom");
        m_zoomAnim->setDuration(200);
        m_zoomAnim->setEasingCurve(QEasingCurve::OutCubic);
    }

    void animateZoomTo(double target) {
        if (m_src.isNull()) return;
        m_fit = false;
        m_zoomAnim->stop();
        m_zoomAnim->setStartValue(m_z);
        m_zoomAnim->setEndValue(std::clamp(target, 0.01, 32.0));
        m_zoomAnim->start();
    }

    void prepFadeIn() {
        auto* effect = qobject_cast<QGraphicsOpacityEffect*>(graphicsEffect());
        if (!effect) {
            effect = new QGraphicsOpacityEffect(this);
            setGraphicsEffect(effect);
        }
        effect->setOpacity(0.0);
    }

    void startFadeIn() {
        auto* effect = qobject_cast<QGraphicsOpacityEffect*>(graphicsEffect());
        if (!effect) return;
        auto* anim = new QPropertyAnimation(effect, "opacity", this);
        anim->setDuration(120);
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void load(const QPixmap& px, bool fit = true) {
        stopMovie();
        prepFadeIn();
        m_src = px;
        m_fit = fit;
        m_smoothTimer.stop();
        if (fit && parentWidget()) fitNow();
        else                       applyFast();
        startFadeIn();
    }

    void loadMovie(QMovie* mv) {
        stopMovie();
        m_src = QPixmap();
        m_movie = mv;
        connect(m_movie, &QMovie::frameChanged, this, [this]() {
            QPixmap f = m_movie->currentPixmap();
            if (!f.isNull()) { m_src = f; applyFast(); }
        });
        m_movie->start();
    }

    void fitToParent() { fitNow(); }

    void setZoom(double z) {
        m_fit = false;
        setZoomDirect(std::clamp(z, 0.01, 32.0));
    }
    double zoom()   const { return m_z; }
    bool   hasSrc() const { return !m_src.isNull(); }
    bool   isFit()  const { return m_fit; }
    QPixmap src()   const { return m_src; }
    void clearImage() {
        stopMovie();
        m_src = QPixmap();
        m_fit = false;
        m_smoothTimer.stop();
        setPixmap(QPixmap());
        resize(1, 1);
    }
    void applyRotated(const QPixmap& px) {
        stopMovie();
        m_src = px;
        m_fit = false;
        m_smoothTimer.stop();
        applyFast();
    }

signals:
    void zoomChanged(double z);

protected:
    void resizeEvent(QResizeEvent* e) override {
        QLabel::resizeEvent(e);
        if (m_fit) fitNow();
    }

private:
    void fitNow() {
        if (m_src.isNull() || !parentWidget()) return;
        QSize av = parentWidget()->size() - QSize(2, 2);
        double sx = (double)av.width()  / m_src.width();
        double sy = (double)av.height() / m_src.height();
        m_fit = true;
        m_smoothTimer.stop();
        setZoomDirect(std::min(sx, sy));
    }
    void setZoomDirect(double z) {
        m_z = z;
        applyFast();
        emit zoomChanged(m_z);
    }
    void applyFast() {
        if (m_src.isNull()) return;
        QSize sz = (m_src.size() * m_z).expandedTo(QSize(1, 1));
        setPixmap(m_src.scaled(sz, Qt::KeepAspectRatio, Qt::FastTransformation));
        resize(sz);
        m_smoothTimer.start(120);
    }
    void applySmooth() {
        if (m_src.isNull()) return;
        QSize sz = (m_src.size() * m_z).expandedTo(QSize(1, 1));
        setPixmap(m_src.scaled(sz, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        resize(sz);
    }
    void stopMovie() {
        if (m_movie) { m_movie->stop(); m_movie->deleteLater(); m_movie = nullptr; }
    }

    QPixmap               m_src;
    QMovie*               m_movie = nullptr;
    double                m_z   = 1.0;
    bool                  m_fit = true;
    QTimer                m_smoothTimer;
    QPropertyAnimation*   m_zoomAnim;
};

/* ──────────────────────────────────────────────────
   VIEW
────────────────────────────────────────────────── */
class View : public QScrollArea {
    Q_OBJECT
public:
    explicit View(QWidget* parent = nullptr) : QScrollArea(parent) {
        m_c = new Canvas(this);
        setWidget(m_c);
        setWidgetResizable(false);
        setAlignment(Qt::AlignCenter);
        setFrameShape(QFrame::NoFrame);
        setStyleSheet("background:#18181b;");
        connect(m_c, &Canvas::zoomChanged, this, &View::zoomChanged);
    }
    Canvas* c() { return m_c; }
signals:
    void zoomChanged(double z);
    void dropped(const QString& p);
protected:
    void wheelEvent(QWheelEvent* e) override {
        double d = e->angleDelta().y() > 0 ? 1.18 : (1.0 / 1.18);
        m_c->setZoom(m_c->zoom() * d);
        e->accept();
    }
    void mousePressEvent(QMouseEvent* e) override {
        if (e->button() == Qt::LeftButton) {
            m_pan = true; m_last = e->pos(); setCursor(Qt::ClosedHandCursor);
        }
        QScrollArea::mousePressEvent(e);
    }
    void mouseMoveEvent(QMouseEvent* e) override {
        if (m_pan) {
            QPoint d = e->pos() - m_last; m_last = e->pos();
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() - d.x());
            verticalScrollBar()->setValue  (verticalScrollBar()->value()   - d.y());
        }
        QScrollArea::mouseMoveEvent(e);
    }
    void mouseReleaseEvent(QMouseEvent* e) override {
        m_pan = false; setCursor(Qt::ArrowCursor);
        QScrollArea::mouseReleaseEvent(e);
    }
    void mouseDoubleClickEvent(QMouseEvent* e) override {
        if (e->button() == Qt::LeftButton) {
            if (m_c->isFit()) m_c->animateZoomTo(1.0);
            else               m_c->fitToParent();
        }
    }
    void dragEnterEvent(QDragEnterEvent* e) override {
        if (e->mimeData()->hasUrls()) e->acceptProposedAction();
    }
    void dropEvent(QDropEvent* e) override {
        if (e->mimeData()->hasUrls())
            emit dropped(e->mimeData()->urls().first().toLocalFile());
    }
private:
    Canvas* m_c;
    bool    m_pan  = false;
    QPoint  m_last;
};

/* ──────────────────────────────────────────────────
   ROTATE OVERLAY — smooth rotation animation
────────────────────────────────────────────────── */
class RotateOverlay : public QWidget {
    Q_OBJECT
    Q_PROPERTY(double angle READ angle WRITE setAngle)
public:
    RotateOverlay(QWidget* parent, const QPixmap& px)
        : QWidget(parent), m_px(px) {
        setGeometry(parent->rect());
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_OpaquePaintEvent);
        setAttribute(Qt::WA_NoSystemBackground);
        m_anim = new QPropertyAnimation(this, "angle", this);
        m_anim->setDuration(180);
        m_anim->setEasingCurve(QEasingCurve::OutCubic);
        connect(m_anim, &QPropertyAnimation::finished, this, [this]{
            hide();
            emit finished();
            deleteLater();
        });
    }
    void start(int deg) {
        m_anim->setStartValue(0.0);
        m_anim->setEndValue(deg);
        show();
        raise();
        m_anim->start();
    }
    double angle() const { return m_a; }
    void setAngle(double a) { m_a = a; update(); }
signals:
    void finished();
protected:
    void paintEvent(QPaintEvent*) override {
        if (m_px.isNull()) return;
        QPainter p(this);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        p.setRenderHint(QPainter::Antialiasing);
        QRect r = rect();
        p.fillRect(r, QColor("#18181b"));
        QSize s = m_px.size();
        QPointF c = r.center();
        p.translate(c);
        p.rotate(m_a);
        p.drawPixmap(-s.width()/2.0, -s.height()/2.0, m_px);
    }
private:
    QPixmap m_px;
    double m_a = 0;
    QPropertyAnimation* m_anim;
};

/* ──────────────────────────────────────────────────
   MAIN WINDOW
────────────────────────────────────────────────── */
class Win : public QMainWindow {
    Q_OBJECT

    static bool imgExt(const QString& p) {
        static const QSet<QString> E = []{
            QSet<QString> s;
            for (const auto& f : QImageReader::supportedImageFormats())
                s.insert(QString::fromLatin1(f).toLower());
            s.insert("svg");
            return s;
        }();
        return E.contains(QFileInfo(p).suffix().toLower());
    }

    /* async load image data (runs off main thread) */
    struct LoadResult {
        QImage img;
        int    width  = 0;
        int    height = 0;
        QString error;
    };
    static LoadResult loadFile(const QString& path) {
        QFileInfo fi(path);
        QString ext = fi.suffix().toLower();
        LoadResult r;

        if (ext == "svg") {
            QSvgRenderer svgr(path);
            if (svgr.isValid()) {
                QSize sz = svgr.defaultSize().isEmpty() ? QSize(1024, 768) : svgr.defaultSize();
                sz.scale(4096, 4096, Qt::KeepAspectRatio);
                QPixmap px(sz); px.fill(Qt::transparent);
                QPainter p(&px); svgr.render(&p); p.end();
                r.img = px.toImage();
                r.width = r.img.width(); r.height = r.img.height();
                return r;
            }
        }

        QImageReader rd(path);
        rd.setAutoTransform(true);
        QSize imgSz = rd.size();
        if (imgSz.isValid()) {
            QScreen* scr = QGuiApplication::primaryScreen();
            if (scr) {
                QSize vs = scr->virtualSize();
                int limit = std::max(vs.width(), vs.height()) * 2;
                if (imgSz.width() > limit || imgSz.height() > limit)
                    rd.setScaledSize(imgSz.scaled(limit, limit, Qt::KeepAspectRatio));
            }
        }
        r.img = rd.read();
        r.width  = imgSz.isValid() ? imgSz.width()  : r.img.width();
        r.height = imgSz.isValid() ? imgSz.height() : r.img.height();
        if (r.img.isNull()) r.error = rd.errorString();
        return r;
    }

public:
    Win() {
        setWindowTitle("LuminaView");
        qApp->setApplicationName("LuminaView");
        setMinimumSize(640, 440);
        resize(1080, 700);
        setAcceptDrops(true);

        QSettings s("luminaview","luminaview");
        restoreGeometry(s.value("geometry").toByteArray());

        m_v = new View(this);
        setCentralWidget(m_v);

        mkBar();
        mkStatus();
        mkKeys();
        preloadLater();

        connect(m_v, &View::dropped,     this, [this](const QString& p){ open(p); });
        connect(m_v, &View::zoomChanged, this, [this](double z){
            m_zl->setText(QString("%1%").arg(qRound(z * 100)));
        });

        m_slide = new QTimer(this);
        m_slide->setInterval(3500);
        connect(m_slide, &QTimer::timeout, this, &Win::next);

        placeholder();
    }
    ~Win() override { QSettings s("luminaview","luminaview"); s.setValue("geometry", saveGeometry()); }

protected:
    void keyPressEvent(QKeyEvent* e) override {
        switch (e->key()) {
        case Qt::Key_Right: case Qt::Key_Space:    next(); break;
        case Qt::Key_Left:  case Qt::Key_Backspace: prev(); break;
        case Qt::Key_Home:  go(0);                         break;
        case Qt::Key_End:   go((int)m_f.size()-1);         break;
        case Qt::Key_Escape: if (isFullScreen()) toggleFS(); break;
        case Qt::Key_Delete: del();                        break;
        default: QMainWindow::keyPressEvent(e);
        }
    }
    void dragEnterEvent(QDragEnterEvent* e) override {
        if (e->mimeData()->hasUrls()) e->acceptProposedAction();
    }
    void dropEvent(QDropEvent* e) override {
        if (e->mimeData()->hasUrls())
            open(e->mimeData()->urls().first().toLocalFile());
    }
    void closeEvent(QCloseEvent* e) override {
        QSettings s("luminaview","luminaview"); s.setValue("geometry", saveGeometry());
        QMainWindow::closeEvent(e);
    }

private:
    /* ── icons ── */
    QIcon m_iOpen = mkIcon(S_OPEN), m_iFdir = mkIcon(S_FDIR);
    QIcon m_iPrev = mkIcon(S_PREV), m_iNext = mkIcon(S_NEXT);
    QIcon m_iZin = mkIcon(S_ZIN), m_iZout = mkIcon(S_ZOUT), m_iFit = mkIcon(S_FIT);
    QIcon m_iRotl = mkIcon(S_ROTL), m_iRotr = mkIcon(S_ROTR);
    QIcon m_iPlay = mkIcon(S_PLAY), m_iPaus = mkIcon(S_PAUS);
    QIcon m_iCopy = mkIcon(S_COPY), m_iFull = mkIcon(S_FULL), m_iTrsh = mkIcon(S_TRSH);

    /* ── toolbar ── */
    QToolButton* btn(const QIcon& ic, const QString& tip) {
        auto* b = new QToolButton; b->setIcon(ic); b->setToolTip(tip); return b;
    }
    QFrame* sp() {
        auto* f = new QFrame; f->setObjectName("sep");
        f->setFrameShape(QFrame::VLine); return f;
    }
    void mkBar() {
        auto* tb = new QToolBar(this); tb->setMovable(false); addToolBar(tb);
        m_tb = tb;
        auto add = [&](QToolButton* b){ tb->addWidget(b); return b; };
        auto sep = [&]{ tb->addWidget(sp()); };

        connect(add(btn(m_iOpen,"Open (Ctrl+O)")),    &QToolButton::clicked, this, [this]{ open(); });
        connect(add(btn(m_iFdir,"Folder (Ctrl+Shift+O)")),&QToolButton::clicked, this, &Win::openDir);
        sep();
        m_bp = add(btn(m_iPrev,"Previous (Left)")); m_bp->setEnabled(false);
        m_bn = add(btn(m_iNext,"Next (Right)")); m_bn->setEnabled(false);
        sep();
        connect(add(btn(m_iZin,"Zoom In (+)")),  &QToolButton::clicked, this,[this]{ m_v->c()->animateZoomTo(m_v->c()->zoom() * 1.5); });
        connect(add(btn(m_iZout,"Zoom Out (-)")),&QToolButton::clicked, this,[this]{ m_v->c()->animateZoomTo(m_v->c()->zoom() / 1.5); });
        connect(add(btn(m_iFit,"Fit to Screen (F)")),&QToolButton::clicked, this,[this]{ m_v->c()->fitToParent(); });
        m_zl = new QLabel("—"); m_zl->setObjectName("zl"); tb->addWidget(m_zl);
        sep();
        connect(add(btn(m_iRotl,"Rotate Left (Ctrl+L)")), &QToolButton::clicked, this, [this]{ rot(-90); });
        connect(add(btn(m_iRotr,"Rotate Right (Ctrl+R)")),&QToolButton::clicked, this, [this]{ rot(90);  });
        sep();
        m_bs = add(btn(m_iPlay,"Slideshow (S)"));
        connect(m_bs, &QToolButton::clicked, this, &Win::toggleSlide);
        sep();
        connect(add(btn(m_iCopy,"Copy (Ctrl+C)")),&QToolButton::clicked, this, &Win::clip);
        connect(add(btn(m_iFull,"Fullscreen (F11)")),&QToolButton::clicked, this, &Win::toggleFS);
        sep();
        connect(add(btn(m_iTrsh,"Delete (Del)")), &QToolButton::clicked, this, &Win::del);

        connect(m_bp, &QToolButton::clicked, this, &Win::prev);
        connect(m_bn, &QToolButton::clicked, this, &Win::next);
    }
    void mkStatus() {
        m_sf = new QLabel; m_si = new QLabel;
        statusBar()->addWidget(m_sf, 1);
        statusBar()->addPermanentWidget(m_si);
    }
    void mkKeys() {
        auto sk = [&](const char* k, auto fn){
            connect(new QShortcut(QKeySequence(k), this), &QShortcut::activated, this, fn);
        };
        sk("Ctrl+O",       [this]{ open(); });
        sk("Ctrl+Shift+O", &Win::openDir);
        sk("+",   [this]{ m_v->c()->animateZoomTo(m_v->c()->zoom() * 1.5); });
        sk("=",   [this]{ m_v->c()->animateZoomTo(m_v->c()->zoom() * 1.5); });
        sk("-",   [this]{ m_v->c()->animateZoomTo(m_v->c()->zoom() / 1.5); });
        sk("0",   [this]{ m_v->c()->animateZoomTo(1.0); });
        sk("F",   [this]{ m_v->c()->fitToParent(); });
        sk("F11", &Win::toggleFS);
        sk("Ctrl+L", [this]{ rot(-90); }); sk("Ctrl+R", [this]{ rot(90); });
        sk("Ctrl+C", &Win::clip);
        sk("S", &Win::toggleSlide);
        sk("Delete", &Win::del);
    }

    /* ── image cache ── */
    static constexpr int MAX_CACHE = 12;
    struct CacheEntry {
        QImage img;
        int width  = 0;
        int height = 0;
        LoadState state = LS_NONE;
    };
    std::unordered_map<QString, CacheEntry> m_cache;

    CacheEntry& cacheSlot(const QString& path) {
        auto it = m_cache.find(path);
        if (it != m_cache.end()) return it->second;
        // full: evict one arbitrary non-loading entry (unordered_map order)
        if ((int)m_cache.size() >= MAX_CACHE) {
            QString victim;
            for (auto& [k, v] : m_cache)
                if (v.state != LS_LOADING) { victim = k; break; }
            if (!victim.isEmpty()) m_cache.erase(victim);
        }
        return m_cache[path];
    }

    void preloadAsync(const QString& path) {
        if (path.isEmpty()) return;
        auto& slot = cacheSlot(path);
        if (slot.state != LS_NONE) return; // already loaded or loading
        slot.state = LS_LOADING;

        auto* watcher = new QFutureWatcher<LoadResult>(this);
        connect(watcher, &QFutureWatcher<LoadResult>::finished, this, [this, watcher, path]() {
            auto r = watcher->result();
            auto& s = m_cache[path];
            if (!r.img.isNull()) {
                s.img = r.img;
                s.width  = r.width;
                s.height = r.height;
                s.state = LS_READY;
            } else {
                s.state = LS_NONE;
            }
            watcher->deleteLater();

            // If this is the current image, apply it
            if (m_i >= 0 && m_i < (int)m_f.size() && m_f[m_i] == path && !r.img.isNull()) {
                applyImg(r.img, r.width, r.height, path);
            }
            // after loading, try preloading more
            preloadLater();
        });
        watcher->setFuture(QtConcurrent::run([path]() {
            return loadFile(path);
        }));
    }

public:
    /* ── open / load ── */
    void open(const QString& path = {}) {
        QString p = path;
        if (p.isEmpty()) {
            static const QString F = []{
                QStringList pat;
                for (const auto& f : QImageReader::supportedImageFormats())
                    pat << ("*." + QString::fromLatin1(f).toLower());
                pat << "*.svg"; pat.sort(); pat.removeDuplicates();
                return "Images (" + pat.join(' ') + ");;All (*)";
            }();
            p = QFileDialog::getOpenFileName(this, "Open Image", m_dir, F);
            if (p.isEmpty()) return;
        }
        QFileInfo fi(p);
        if (fi.isDir())  { loadDir(p); return; }
        if (!fi.isFile() || !imgExt(p)) return;
        m_dir = fi.absolutePath();
        loadDir(fi.absolutePath(), p);
    }

    void openDir() {
        QString d = QFileDialog::getExistingDirectory(this, "Open Folder", m_dir);
        if (!d.isEmpty()) loadDir(d);
    }

    void loadDir(const QString& dir, const QString& sel = {}) {
        QDir d(dir);
        QStringList names = d.entryList(QDir::Files | QDir::Readable,
                                         QDir::Name | QDir::IgnoreCase);
        m_f.clear();
        m_cache.clear();
        int si = 0;
        for (auto& n : names) {
            QString abs = d.absoluteFilePath(n);
            if (!imgExt(abs)) continue;
            if (!sel.isEmpty() && abs == sel) si = (int)m_f.size();
            m_f.push_back(abs);
        }
        if (m_f.empty()) return;
        m_i = si;
        preloadAsync(m_f[m_i]);
        nav();
    }

private:
    void applyImg(const QImage& img, int w, int h, const QString& path) {
        if (img.isNull()) return;
        // check gif
        QFileInfo fi(path);
        if (fi.suffix().toLower() == "gif") {
            auto* mv = new QMovie(path, {}, this);
            if (mv->isValid()) {
                m_v->c()->loadMovie(mv);
                m_v->c()->fitToParent();
                st(fi.fileName());
                return;
            }
            delete mv;
        }
        m_cur = img;
        m_v->c()->load(QPixmap::fromImage(img));
        m_v->c()->fitToParent();
        st(QString("%1  \u00b7  %2 \u00d7 %3").arg(fi.fileName()).arg(w).arg(h));
    }

    void rot(int deg) {
        if (m_cur.isNull()) return;
        // remove opacity effect so grab is fully opaque
        m_v->c()->setGraphicsEffect(nullptr);
        QPixmap px = m_v->c()->grab();
        if (px.isNull()) return;
        // rotate source image in memory
        QTransform t; t.rotate(deg);
        m_cur = m_cur.transformed(t, Qt::SmoothTransformation);
        // hide canvas content — overlay will show the animation
        bool wasFit = m_v->c()->isFit();
        double z = m_v->c()->zoom();
        m_v->c()->clearImage();
        // animate old image rotating away
        auto* ov = new RotateOverlay(m_v, px);
        connect(ov, &RotateOverlay::finished, this, [this, wasFit, z]() {
            m_v->c()->applyRotated(QPixmap::fromImage(m_cur));
            if (wasFit) m_v->c()->fitToParent();
            else        m_v->c()->setZoom(z);
        });
        ov->start(deg);
    }

    void next() {
        if ((int)m_f.size() <= 1) return;
        m_i = (m_i + 1) % (int)m_f.size();
        goCache(m_i);
    }
    void prev() {
        if ((int)m_f.size() <= 1) return;
        m_i = (m_i - 1 + (int)m_f.size()) % (int)m_f.size();
        goCache(m_i);
    }
    void go(int i) {
        if (i<0||i>=(int)m_f.size()) return;
        m_i = i;
        goCache(m_i);
    }

    void goCache(int i) {
        const QString& path = m_f[i];
        auto it = m_cache.find(path);
        if (it != m_cache.end() && it->second.state == LS_READY) {
            auto& e = it->second;
            applyImg(e.img, e.width, e.height, path);
        } else {
            preloadAsync(path);
        }
        nav();

        // preload adjacent
        preloadLater();
    }

    void preloadLater() {
        int n = (int)m_f.size();
        if (n <= 1 || m_i < 0) return;
        int nextIdx = (m_i + 1) % n;
        int prevIdx = (m_i - 1 + n) % n;
        if (nextIdx != m_i) preloadAsync(m_f[nextIdx]);
        if (prevIdx != m_i) preloadAsync(m_f[prevIdx]);
    }

    void toggleSlide() {
        if (m_slide->isActive()) { m_slide->stop(); m_bs->setIcon(m_iPlay); }
        else                     { m_slide->start(); m_bs->setIcon(m_iPaus); }
    }

    void clip() {
        if (m_v->c()->hasSrc())
            QApplication::clipboard()->setPixmap(m_v->c()->src());
    }

    void toggleFS() {
        if (isFullScreen()) { showNormal(); m_tb->show(); statusBar()->show(); }
        else                { m_tb->hide(); statusBar()->hide(); showFullScreen(); }
    }

    void del() {
        if (m_i<0||m_i>=(int)m_f.size()) return;
        QFile::remove(m_f[m_i]);
        m_f.erase(m_f.begin()+m_i);
        if (m_f.empty()) { placeholder(); return; }
        m_i = std::min(m_i, (int)m_f.size()-1);
        goCache(m_i);
    }

    void nav() {
        int n = (int)m_f.size();
        m_bp->setEnabled(m_i > 0);
        m_bn->setEnabled(m_i < n-1);
        m_si->setText(n > 0 ? QString(" %1 / %2 ").arg(m_i+1).arg(n) : "");
    }

    void st(const QString& s) { m_sf->setText("  " + s); }

    void placeholder() {
        QPixmap px(560, 300); px.fill(QColor("#18181b"));
        QPainter p(&px); p.setRenderHint(QPainter::Antialiasing);
        QFont fnt("Segoe UI", 13); fnt.setWeight(QFont::Light);
        p.setFont(fnt);
        p.setPen(QColor("#71717a"));
        p.drawText(px.rect(), Qt::AlignCenter,
                   "Drop an image here\n\nCtrl+O  \u00b7  Ctrl+Shift+O");
        p.setPen(QColor("#3f3f46"));
        QFont sf("Segoe UI", 9); sf.setWeight(QFont::Light);
        p.setFont(sf);
        QRect br = px.rect(); br.setTop(br.center().y() + 50);
        p.drawText(br, Qt::AlignHCenter | Qt::AlignTop,
                   "Supports all major image formats");
        m_v->c()->load(px); m_v->c()->fitToParent();
        m_sf->clear(); m_si->clear();
    }

    View*        m_v;
    QToolBar*    m_tb;
    QToolButton* m_bp, *m_bn, *m_bs;
    QLabel*      m_zl, *m_sf, *m_si;
    QTimer*      m_slide;
    std::vector<QString> m_f;
    int          m_i   = -1;
    QImage       m_cur;
    QString      m_dir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
};

#include "main.moc"

int main(int argc, char** argv) {
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication app(argc, argv);
    app.setStyleSheet(STYLE);
    Win w;
    w.show();
    if (argc > 1) {
        QString p = QString::fromLocal8Bit(argv[1]);
        QTimer::singleShot(0, &w, [&w, p]{ w.open(p); });
    }
    return app.exec();
}
