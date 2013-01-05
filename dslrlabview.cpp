#include "dslrlabview.h"

// *******
// * progressffSequence
// *******
void QffSequence::onProgressStart(void)
{
    emit signal_progressStart();
}

void QffSequence::onProgress(double factor)
{
    emit signal_progress(factor);
}

void QffSequence::onProgressEnd(void)
{
    emit signal_progressEnd();
}

void QffSequence::onJustLoading(void)
{
    emit signal_justLoading();
}

void QffSequence::onJustOpened(void)
{
    emit signal_justOpened();
}

void QffSequence::onJustClosed(void)
{
    emit signal_justClosed();
}

void QffSequence::onJustErrored(void)
{
    emit signal_justErrored();
}

// *******
// * QDSLRLabView
// *******
DSLRLabView::DSLRLabView(QWidget *parent) :
    QWidget(parent),
    m_pGraphicsView(NULL),
    m_pGraphicsViewOverlay(NULL),
    m_pGraphicsScene(NULL),
    m_pGraphicsSceneOverlay(NULL)
{
    createObjects();
    initObjects();
    createAnimations();
}

void DSLRLabView::createObjects(void)
{
    m_pGraphicsView = new QBaseGraphicsView(this);
    m_pGraphicsViewOverlay = new QBaseGraphicsView(this, m_pGraphicsView);

    m_pGraphicsScene = new QGraphicsScene(this);
    m_pGraphicsSceneOverlay = new QGraphicsScene(this);

    m_pOverlayAnchor = new QGraphicsWidget;
    m_pGraphicsAnchorLayout = new QGraphicsAnchorLayout;

    m_pSlider = new QSlider(Qt::Horizontal);

    m_pProgressBar = new QProgressBar;
    m_pProgressTimeline = new QTimeLine(TIMELINE_DURATION, this);
    m_pTextPill = new QTextPill;

    m_pTimeLine = new QTimeLine(TIMELINE_DURATION, this);

    m_pGraphicsPixmapItem = new QGraphicsPixmapItem;

    m_pffSequence = new QffSequence;
}

void DSLRLabView::initObjects(void)
{
    m_pGraphicsView->setScene(m_pGraphicsScene);
    m_pGraphicsView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    m_pGraphicsView->setDragMode(QGraphicsView::ScrollHandDrag);

    m_pGraphicsViewOverlay->setStyleSheet("background:transparent;");
    m_pGraphicsViewOverlay->setFrameShape(QFrame::NoFrame);
    m_pGraphicsViewOverlay->setScene(m_pGraphicsSceneOverlay);

    m_pOverlayAnchor->setLayout(m_pGraphicsAnchorLayout);
    m_pGraphicsSceneOverlay->addItem(m_pOverlayAnchor);
    m_pGraphicsSceneOverlay->addItem(m_pTextPill);

    m_pTextPill->setPos(TEXT_PADDING_X, TEXT_PADDING_Y);

    m_pgwProgressBar = m_pGraphicsSceneOverlay->addWidget(m_pProgressBar);
    m_pProgressBar->setStyleSheet("background:transparent;");
    m_pProgressBar->setMaximumHeight(PROGRESS_HEIGHT);
    m_pProgressBar->setTextVisible(false);
    m_pgwProgressBar->setOpacity(DSLRVIEW_TRANSPARENT);

    m_pgwSlider = m_pGraphicsSceneOverlay->addWidget(m_pSlider);
    m_pSlider->setStyleSheet("background:transparent;");
    m_pgwSlider->setOpacity(DSLRVIEW_TRANSPARENT);
    m_pGraphicsViewOverlay->addActive(m_pgwSlider);

    m_pGraphicsScene->addItem(m_pGraphicsPixmapItem);

    m_pProgressTimeline->setUpdateInterval(TIMELINE_PROGRESS_UPDATE);
    m_pTimeLine->setUpdateInterval(TIMELINE_ZOOM_UPDATE);

    m_viewerPlane = ffViewer::Y;

    connect(m_pTimeLine, SIGNAL(valueChanged(qreal)),
            SLOT(onScaleTimeslice(qreal)));
    connect(m_pTimeLine, SIGNAL(finished()), SLOT(onScaleAnimFinished()));
    connect(this, SIGNAL(signal_error(QString)), this, SLOT(onError(QString)));
    connect(m_pffSequence, SIGNAL(signal_progressStart()), this,
            SLOT(onProgressStart()));
    connect(m_pffSequence, SIGNAL(signal_progress(double)), this,
            SLOT(onProgress(double)));
    connect(m_pffSequence, SIGNAL(signal_progressEnd()), this,
            SLOT(onProgressEnd()));
    connect(m_pffSequence, SIGNAL(signal_justLoading()), this,
            SLOT(onJustLoading()));
    connect(m_pffSequence, SIGNAL(signal_justOpened()), this,
            SLOT(onJustOpened()));
    connect(m_pffSequence, SIGNAL(signal_justClosed()), this,
            SLOT(onJustClosed()));
    connect(m_pffSequence, SIGNAL(signal_justErrored()), this,
            SLOT(onJustErrored()));
    connect(m_pProgressTimeline, SIGNAL(valueChanged(qreal)), this,
            SLOT(onProgressAnimation(qreal)));
    connect(m_pSlider, SIGNAL(valueChanged(int)), this,
            SLOT(onFrameChange(int)));
    connect(this, SIGNAL(signal_updateUI(ffSequence::ffSequenceState)), this,
            SLOT(onUpdateUI(ffSequence::ffSequenceState)));

    m_pGraphicsViewOverlay->viewport()->installEventFilter(
                m_pGraphicsViewOverlay);
}

void DSLRLabView::createAnimations(void)
{
    m_pFadePixmap = new QGraphicsOpacityEffect;
    m_pFadePixmapAnimation = new QPropertyAnimation(m_pFadePixmap, "opacity");
    m_pFadePixmapAnimation->setDuration(DSLRVIEW_DURATION_INTROFADEIN);
    m_pFadePixmapAnimation->setStartValue(DSLRVIEW_TRANSPARENT);
    m_pFadePixmapAnimation->setEndValue(DSLRVIEW_OPAQUE);

    m_pFadeProgressBar = new QGraphicsOpacityEffect;
    m_pFadeProgressBarAnimation = new QPropertyAnimation(m_pFadeProgressBar,
                                                         "opacity");
    m_pFadeProgressBarAnimation->setDuration(PROGRESS_FADE_DURATION);
    m_pFadeProgressBarAnimation->setStartValue(DSLRVIEW_TRANSPARENT);
    m_pFadeProgressBarAnimation->setEndValue(DSLRVIEW_OPAQUE);

    m_pFadeFrameScrubber = new QGraphicsOpacityEffect;
    m_pFadeFrameScrubberAnimation = new QPropertyAnimation(m_pFadeFrameScrubber,
                                                           "opacity");
    m_pFadeFrameScrubberAnimation->setDuration(PROGRESS_FADE_DURATION);
    m_pFadeFrameScrubberAnimation->setStartValue(DSLRVIEW_TRANSPARENT);
    m_pFadeFrameScrubberAnimation->setEndValue(DSLRVIEW_OPAQUE);
}

DSLRLabView::~DSLRLabView()
{
    // TODO sort out the deletion order to prevent unexpected outs.
    /*delete m_pFadeIn;
    delete m_pTextFadeOut;
    delete m_pFadeInAnimation;
    delete m_pTextFadeOutAnimation;
    delete m_pffSequence;
    delete m_pTextPill;
    delete m_pGraphicsViewOverlay;
    delete m_pGraphicsSceneOverlay;
    delete m_pGraphicsPixmapItem;
    delete m_pGraphicsScene;*/
}


/******************************************************************************
 * Functions
 ******************************************************************************/
void DSLRLabView::resetTransform()
{
    m_pGraphicsView->resetTransform();
}

void DSLRLabView::updateCurrentFrame(long frame)
{
    QImage *pImage = NULL;
    ffRawFrame *pRawFrame = m_pffSequence->setCurrentFrame(frame);

    switch (m_viewerPlane)
    {
    case (ffViewer::RGB):
    case (ffViewer::Y):
        pImage = new QImage(pRawFrame->m_pY,
                                    m_pffSequence->getLumaSize().m_width,
                                    m_pffSequence->getLumaSize().m_height,
                                    QImage::Format_Indexed8);
        break;
    case (ffViewer::Cb):
        pImage = new QImage(pRawFrame->m_pCb,
                                    m_pffSequence->getChromaSize().m_width,
                                    m_pffSequence->getChromaSize().m_height,
                                    QImage::Format_Indexed8);
        break;
    case (ffViewer::Cr):
        pImage = new QImage(pRawFrame->m_pCr,
                                    m_pffSequence->getChromaSize().m_width,
                                    m_pffSequence->getChromaSize().m_height,
                                    QImage::Format_Indexed8);
        break;
    }
    update();
    m_pGraphicsPixmapItem->setPixmap(QPixmap::fromImage(*pImage));
    delete pImage;
    m_pTextPill->start(tr("Frame ") +
                       QString::number(m_pffSequence->getCurrentFrame()));
}

void DSLRLabView::fitToView()
{
    m_pGraphicsView->fitInView(m_pGraphicsPixmapItem, Qt::KeepAspectRatio);
}


long DSLRLabView::getTotalFrames(void)
{
    return m_pffSequence->getTotalFrames();
}

QString DSLRLabView::getFileURI(void)
{
    return QString::fromStdString(m_pffSequence->getFileURI());
}

ffViewer::ViewerPlane DSLRLabView::getViewerPlane(void)
{
    return m_viewerPlane;
}

void DSLRLabView::setViewerPlane(ffViewer::ViewerPlane planeType)
{
    if ((getState() == ffSequence::isValid) && (planeType != m_viewerPlane))
    {
        m_viewerPlane = planeType;
        updateCurrentFrame(m_pffSequence->getCurrentFrame());
        m_pGraphicsView->setSceneRect(m_pGraphicsPixmapItem->boundingRect());
    }
}

ffSequence::ffSequenceState DSLRLabView::getState(void)
{
    return m_pffSequence->getState();
}

void DSLRLabView::openSequence(char *fileName)
{
    try
    {
        if (getState() == ffSequence::isValid)
            closeSequence();

        m_pffSequence->readFile(fileName);
        m_pGraphicsView->setSceneRect(0, 0,
                     m_pffSequence->getLumaSize().m_width,
                     m_pffSequence->getLumaSize().m_height);
    }
    catch (ffmpegError ffeff)
    {
        char errorC[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ffeff.getError(), errorC, AV_ERROR_MAX_STRING_SIZE);

        QString message = tr("FFMPEG: ") + QString(ffeff.what()) + " (" +
                QString(errorC) + ")";
        emit signal_error(message);
    }
    catch (ffError eff)
    {
        QString message = tr("FFSEQUENCE: ") + QString(eff.what()) + " (" +
                QString::number(eff.getError()) + ")";
        emit signal_error(message);
    }
}

void DSLRLabView::saveSequence(char *fileName, long start, long end)
{
    try
    {
        m_pffSequence->writeFile(fileName, start, end);
    }
    catch (ffError eff)
    {
        QString message = tr("FFSEQUENCE: ") + QString(eff.what()) + " (" +
                QString::number(eff.getError()) + ")";
        emit signal_error(message);
    }
}

void DSLRLabView::closeSequence(void)
{
    if (getState() == ffSequence::isValid)
        m_pffSequence->closeFile();
}

QffSequence * DSLRLabView::getQffSequence(void)
{
    return m_pffSequence;
}

QGraphicsPixmapItem* DSLRLabView::getGraphicsPixmapItem(void)
{
    return m_pGraphicsPixmapItem;
}

QTextPill* DSLRLabView::getTextPillItem(void)
{
    return m_pTextPill;
}

/******************************************************************************
 * Slots
 ******************************************************************************/
void DSLRLabView::onScaleTimeslice(qreal)
{
    QTransform matrix(m_pGraphicsView->matrix());
    qreal factor = 1.0 + qreal(m_numScheduledScalings) / 500.0;
    qreal scalefactor = matrix.m11() * factor;

    if ((scalefactor >= MAXIMUM_SCALE) && (factor > 1.0))
        scalefactor = MAXIMUM_SCALE;
    else if ((scalefactor <= MINIMUM_SCALE) && (factor < 1.0))
        scalefactor = MINIMUM_SCALE;

    matrix.setMatrix(scalefactor, matrix.m12(), matrix.m13(),
                     matrix.m21(), scalefactor, matrix.m23(),
                     matrix.m31(), matrix.m32(), matrix.m33());

    m_pGraphicsView->setTransform(QTransform(matrix));
}

void DSLRLabView::onScaleAnimFinished(void)
{
    if (m_numScheduledScalings > 0)
        m_numScheduledScalings--;
    else
        m_numScheduledScalings++;
}

void DSLRLabView::onError(QString)
{
    emit signal_updateUI(ffSequence::justErrored);
}

void DSLRLabView::onProgressStart(void)
{
    m_pProgressBar->setMinimum(0);
    m_pProgressBar->setMaximum(PROGRESS_MAXIMUM);
    m_pProgressBar->setValue(0);

    m_pgwProgressBar->setGraphicsEffect(m_pFadeProgressBar);
    m_pFadeProgressBarAnimation->setDirection(QAbstractAnimation::Forward);
    m_pgwProgressBar->setOpacity(DSLRVIEW_OPAQUE);
    m_pFadeProgressBarAnimation->start();
}

void DSLRLabView::onProgress(double factor)
{
    m_targetProgress = (factor * PROGRESS_MAXIMUM) +
            PROGRESS_MAXIMUM * TIMELINE_PROGRESS_DELAY;
    if (m_pProgressTimeline->state() != QTimeLine::Running)
        m_pProgressTimeline->start();
}

void DSLRLabView::onProgressEnd(void)
{
    m_pgwProgressBar->setGraphicsEffect(m_pFadeProgressBar);
    m_pFadeProgressBarAnimation->setDirection(QAbstractAnimation::Backward);
    m_pFadeProgressBarAnimation->start();
}

void DSLRLabView::onProgressAnimation(qreal)
{
    if (m_targetProgress >= m_pProgressBar->value())
    {
        int step = (((double)m_targetProgress -
                     (double)m_pProgressBar->value()) *
                    ((double)TIMELINE_PROGRESS_UPDATE /
                     (double)TIMELINE_DURATION)) + 1.5;
                ;
        m_pProgressBar->setValue(m_pProgressBar->value() + step);
        m_pProgressBar->update();
    }
}

void DSLRLabView::onJustLoading(void)
{
    emit signal_updateUI(ffSequence::justLoading);
}

void DSLRLabView::onJustOpened(void)
{
    emit signal_updateUI(ffSequence::justOpened);
}

void DSLRLabView::onJustClosed(void)
{
    emit signal_updateUI(ffSequence::justClosed);
}

void DSLRLabView::onJustErrored(void)
{
    emit signal_updateUI(ffSequence::justErrored);
}

void DSLRLabView::onUpdateUI(ffSequence::ffSequenceState state)
{
    switch (state)
    {
    case (ffSequence::isValid):
        break;
    case (ffSequence::isInvalid):
        disconnect(m_pSlider, SIGNAL(valueChanged(int)), this,
                   SLOT(onFrameChange(int)));
        m_pSlider->setEnabled(false);
        m_pSlider->setMinimum(0);
        m_pSlider->setValue(0);
        m_pSlider->setMaximum(0);
        break;
    case (ffSequence::justLoading):
    case (ffSequence::isLoading):
        disconnect(m_pSlider, SIGNAL(valueChanged(int)), this,
                   SLOT(onFrameChange(int)));
        m_pSlider->setEnabled(false);
        m_pSlider->setMinimum(0);
        m_pSlider->setValue(0);
        m_pSlider->setMaximum(0);
        break;
    case (ffSequence::justOpened):
        m_pTextPill->start(tr("Loaded file ") +
                           QString::fromStdString(m_pffSequence->getFileURI()));

        updateCurrentFrame(m_pffSequence->getCurrentFrame());
        fitToView();

        connect(m_pSlider, SIGNAL(valueChanged(int)), this,
                SLOT(onFrameChange(int)));
        m_pSlider->setMinimum(FF_FIRST_FRAME);
        m_pSlider->setMaximum(getTotalFrames());
        m_pSlider->setValue(FF_FIRST_FRAME);
        m_pSlider->setEnabled(true);

        m_pgwSlider->setGraphicsEffect(m_pFadeFrameScrubber);
        m_pFadeFrameScrubberAnimation->setDirection(QAbstractAnimation::Forward);
        m_pgwSlider->setOpacity(DSLRVIEW_OPAQUE);
        m_pFadeFrameScrubberAnimation->start();

        m_pGraphicsPixmapItem->setGraphicsEffect(m_pFadePixmap);
        m_pFadePixmapAnimation->setDirection(QAbstractAnimation::Forward);
        m_pFadePixmapAnimation->start();
        break;
    case (ffSequence::justErrored):
        break;
    case (ffSequence::justClosed):
        m_pGraphicsPixmapItem->setGraphicsEffect(m_pFadePixmap);
        m_pFadePixmapAnimation->setDirection(QAbstractAnimation::Backward);
        m_pFadePixmapAnimation->start();

        m_pgwSlider->setGraphicsEffect(m_pFadeFrameScrubber);
        m_pFadeFrameScrubberAnimation->setDirection(QAbstractAnimation::Backward);
        m_pFadeFrameScrubberAnimation->start();
        break;
    }
}

void DSLRLabView::onFrameChange(int frame)
{
    if (getState() == ffSequence::isValid)
        updateCurrentFrame(frame);
}

/*****************************************************************************
 * Event Overrides
 *****************************************************************************/
void DSLRLabView::wheelEvent(QWheelEvent* event)
{
    if (event->orientation() == Qt::Vertical)
    {
        int numDegrees = event->delta() / 8;
        int numSteps = numDegrees / 10;

        m_numScheduledScalings += numSteps;
        if (m_numScheduledScalings * numSteps < 0)
            m_numScheduledScalings = numSteps;

        if (m_pTimeLine->state() != QTimeLine::Running)
            m_pTimeLine->start();
    }
}

void DSLRLabView::resizeEvent(QResizeEvent *event)
{
    QRect rect(QPoint(0,0), event->size());
    m_pGraphicsView->setGeometry(rect);
    m_pGraphicsViewOverlay->setGeometry(rect);
    m_pGraphicsViewOverlay->setSceneRect(rect);

    QSizeF dest = event->size() / 2;

    m_pgwProgressBar->setGeometry(dest.width() / 2, dest.height() -
                                  (PROGRESS_HEIGHT / 2),
                                  dest.width(), PROGRESS_HEIGHT);

    m_pgwSlider->setGeometry(0, rect.size().height() - m_pgwSlider->geometry().height(),
                             rect.width(),
                             m_pgwSlider->geometry().height());

    m_pgwSlider->update();
    m_pgwProgressBar->update();
}

void DSLRLabView::mouseReleaseEvent(QMouseEvent *event)
{
    m_pGraphicsView->eventFilter(m_pGraphicsView, event);
    event->ignore();
}

void DSLRLabView::mousePressEvent(QMouseEvent *event)
{
    m_pGraphicsView->eventFilter(m_pGraphicsView, event);
    event->ignore();
}

void DSLRLabView::mouseMoveEvent(QMouseEvent *event)
{
    m_pGraphicsView->eventFilter(m_pGraphicsView, event);
    event->ignore();
}
