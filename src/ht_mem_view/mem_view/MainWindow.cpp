/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QPaintEvent>
#include <QPixmap>
#include <QGraphicsPixmapItem>
#include <stdio.h>
#include <QGraphicsSceneMouseEvent>
#include <QFileDialog>
#include <QProgressDialog>
#include <QProgressBar>
#include "data.h"
#include    <math.h>


int         StatusData::xPos = 0;
int         StatusData::yPos = 0;
QWidget     *StatusData::parent = 0;

#define MARK_TIME   1
#undef MARK_TIME

#ifndef MARK_TIME
void markTime(bool /*startT*/, const char */*str*/) {}

#else

#include <sys/time.h>
void
markTime(bool startT, const char *str)
{
    struct timeval          tv;
    static struct timeval   tvs;
    uint64_t                et;
    
    gettimeofday(&tv, 0);
    if(startT == true) {
        tvs = tv;
        printf("%s  %ld  %ld\n", str, tv.tv_sec, tv.tv_usec);
    } else {
        et = (tv.tv_sec - tvs.tv_sec) * 1000000;
        et += tv.tv_usec - tvs.tv_usec;
        printf("%s   %ld.%ld sec\n", str, et/1000000, et % 1000000);
    }
    fflush(stdout);
}

#endif      // MARK_TIME

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    int     x, y;
    int     i;

    ui->setupUi(this);
    setWindowTitle("Convey Graphical Memory Display");
    QPalette palette; 
    palette.setColor(menuBar()->backgroundRole(), Qt::black); 
    menuBar()->setPalette(palette);
    ui->menuBar->setPalette(palette);
    
    QAction *openAct = new QAction(tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

    QAction *exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

    
    QMenu *fileMenu = this->menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(openAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    scene = new BankScene(this, 0, 0, X_MAX * 2, BANKS * 3);
    connect(scene, SIGNAL(selectionChanged()), this, SLOT(pointSelected()));
    bwlaY = (ui->bwlaView->height() - 6) / 2;
    bwlaScene = new BankScene(this, 0, 0, X_MAX*2, bwlaY * 2);
    bwlaHScene = new BankScene(this, 0, 0, X_MAX*2, bwlaY * 2);

    QPixmap     red(2,2);
    QPixmap     blue(2,2);
    QPixmap     green(2,2);
    QPixmap     yellow(2,2);
    QPixmap     markerX(21, 2);
    QPixmap     markerY(2,21);
    QPainter    *painter;

    painter = new QPainter();
    
    painter->begin(&red);
    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setPen(QPen(Qt::red, 2, Qt::SolidLine, Qt::RoundCap));
    painter->drawPoint(0, 0);
    painter->drawPoint(0, 1);
    painter->drawPoint(1, 0);
    painter->drawPoint(1, 1);
    painter->end();
    
    painter->begin(&blue);
    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setPen(QPen(Qt::blue, 2, Qt::SolidLine, Qt::RoundCap));
    painter->drawPoint(0, 0);
    painter->drawPoint(0, 1);
    painter->drawPoint(1, 0);
    painter->drawPoint(1, 1);
    painter->end();

    painter->begin(&green);
    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setPen(QPen(Qt::green, 2, Qt::SolidLine, Qt::RoundCap));
    painter->drawPoint(0, 0);
    painter->drawPoint(0, 1);
    painter->drawPoint(1, 0);
    painter->drawPoint(1, 1);
    painter->end();

    painter->begin(&yellow);
    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setPen(QPen(Qt::yellow, 2, Qt::SolidLine, Qt::RoundCap));
    painter->drawPoint(0, 0);
    painter->drawPoint(0, 1);
    painter->drawPoint(1, 0);
    painter->drawPoint(1, 1);
    painter->end();
    delete painter;
    
    markerX.fill(Qt::black);
    markerY.fill(Qt::black);
    markerDisplayed = false;

    markerItemX = new MarkerItem(0, 0, markerX);
    markerItemY = new MarkerItem(0, 0, markerY);
    markerItemX->setZValue(100.0);                  // force to foreground
    markerItemY->setZValue(100.0);
    
    //
    //  build all the QGraphicItems (red, green, yellow) for the scene
    //
    for(x=0; x<X_MAX; x++) {
        pmiHR[x] = new BankItem(x, BANKS +1, red);
        pmiHR[x]->setOffset(x*2, BANKS * 3);
        pmiHG[x] = new BankItem(x, BANKS +1, green);
        pmiHG[x]->setOffset(x*2, BANKS * 3);
        pmiHY[x] = new BankItem(x, BANKS +1, yellow);
        pmiHY[x]->setOffset(x*2, BANKS * 3);
        for(y=0; y<BANKS; y++) {
            pmiR[x][y] = new BankItem(x,y,red);
            pmiR[x][y]->setOffset(x*2, y*3);
            pmiR[x][y]->setFlags(QGraphicsItem::ItemIsSelectable);
            pmiG[x][y] = new BankItem(x,y,green);
            pmiG[x][y]->setOffset(x*2, y*3);
            pmiG[x][y]->setFlags(QGraphicsItem::ItemIsSelectable);
            pmiY[x][y] = new BankItem(x,y,yellow);
            pmiY[x][y]->setOffset(x*2, y*3);
            pmiY[x][y]->setFlags(QGraphicsItem::ItemIsSelectable);
            currPmi[x][y] = 0;                              // no item at location, display background
        }
    } 
    for(x=0; x<X_MAX; x++) {
        currBwLa[x] = (BwLaItem **)malloc(sizeof(BwLaItem *) * bwlaY);
        bwlaR[x] = (BwLaItem **)malloc(sizeof(BwLaItem *) * bwlaY);
        bwlaB[x] = (BwLaItem **)malloc(sizeof(BwLaItem *) * bwlaY);
        for(y=0; y<bwlaY; y++) {
            bwlaR[x][y] = new BwLaItem(x, y, red);
            bwlaR[x][y]->setOffset(x*2, y*2);
            bwlaB[x][y] = new BwLaItem(x, y, blue);
            bwlaB[x][y]->setOffset(x*2, y*2);
            currBwLa[x][y] = 0;
        }
        // and the host
        currBwLaH[x] = (BwLaItem **)malloc(sizeof(BwLaItem *) * bwlaY);
        bwlaRH[x] = (BwLaItem **)malloc(sizeof(BwLaItem *) * bwlaY);
        bwlaBH[x] = (BwLaItem **)malloc(sizeof(BwLaItem *) * bwlaY);
        for(y=0; y<bwlaY; y++) {
            bwlaRH[x][y] = new BwLaItem(x, y, red);
            bwlaRH[x][y]->setOffset(x*2, y*2);
            bwlaBH[x][y] = new BwLaItem(x, y, blue);
            bwlaBH[x][y]->setOffset(x*2, y*2);
            currBwLaH[x][y] = 0;
        }
    }

    ctlr[0] = ui->radioButton_0;
    ctlr[1] = ui->radioButton_1;
    ctlr[2] = ui->radioButton_2;
    ctlr[3] = ui->radioButton_3;
    ctlr[4] = ui->radioButton_4;
    ctlr[5] = ui->radioButton_5;
    ctlr[6] = ui->radioButton_6;
    ctlr[7] = ui->radioButton_7;
    ctlr[CTLR_ALL_C] = ui->radioButton_all_c;
    ctlr[CTLR_ALL_B] = ui->radioButton_all_b;

    label[0] = ui->bankLabel_0;
    label[1] = ui->bankLabel_1;
    label[2] = ui->bankLabel_2;
    label[3] = ui->bankLabel_3;
    label[4] = ui->bankLabel_4;
    label[5] = ui->bankLabel_5;
    label[6] = ui->bankLabel_6;
    label[7] = ui->bankLabel_7;

    ctlr[CTLR_ALL_C]->setChecked(true);         // set the all button
    currentCtlr = 7;                            // forces a repaint
    ctlrSelect(true);                           // builds scene

    ui->graphicsView->setScene(scene);
    ui->bwlaView->setScene(bwlaScene);
    ui->bwlaView_h->setScene(bwlaHScene);
    
//    ui->bandwidth->setText("<font color='red'>Bandwidth</font>");
//    ui->latency->setText("<font color='blue'>Latency</font>");
//    ui->bandwidth->setStyleSheet("QLabel { color : red; }");
 //   ui->latency->setStyleSheet("QLabel { color : blue; }");
//    ui->bwLine->setStyleSheet("QLabel { color : red; }");
//    ui->latLine->setStyleSheet("QLine { color : blue; }");
    ui->bwLine->setStyleSheet("QLabel { background-color : red; color : red; }");
    ui->latLine->setStyleSheet("QLabel { background-color : blue; color : blue; }");

    for(i=0; i<CTLR_MAX + 2; i++) {
        connect(ctlr[i], SIGNAL(toggled(bool)), this, SLOT(ctlrSelect(bool)));
    }
    connect(ui->zoomIn, SIGNAL(clicked(bool)), this, SLOT(zoomIn(bool)));
    connect(ui->zoomOut, SIGNAL(clicked(bool)), this, SLOT(zoomOut(bool)));
    connect(ui->zoomFull, SIGNAL(clicked(bool)), this, SLOT(zoomFull(bool)));
    connect(ui->selectLeft, SIGNAL(clicked(bool)), this, SLOT(selectLeft(bool)));
    connect(ui->selectRight, SIGNAL(clicked(bool)), this, SLOT(selectRight(bool)));
    
    connect(ui->timeScroll, SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)));
    
    ui->selectLeft->setEnabled(false);
    ui->selectRight->setEnabled(false);
    itemSelected = 0;
    bankItemSelected = 0;
    sliderUpdating = false;
}

void
MainWindow::open()
{
    string      fileName;
    char        buf[256];
    size_t      pos;
    uint64_t    hostReq;
    uint64_t    coprocReq;
    QPoint      p;
    QFileDialog *qfdP = 0;
    
    p = ui->bankLabel_3->mapToGlobal(QPoint(0,0));      // get point on my widget
    StatusData::setPosition(this, p.x(), p.y());

    if(qfdP == 0) {
        qfdP = new QFileDialog(this, tr(""), tr(""), tr("HTMT (*.htmt);;All (*.*)"));
        connect(qfdP, SIGNAL(fileSelected(QString)), this, SLOT(fileSelected(QString)));
    }
    qfdP->move(p.x(), p.y());                           // put in screen
    qfdP->setViewMode(QFileDialog::List);
    fileNameSel = "";
    qfdP->exec();                                       // do the dialog

    if(fileNameSel.isEmpty() == true) {
        return;
    }
    if(bankItemSelected != 0) {
        bankItemSelected->setSelected(false);           // clears bankItemSelected
    }
    
    DataItem::cleanup();                           // remove any old data
    fileName = fileNameSel.toStdString();
    pos = fileName.find_last_of("/\\");
    if(pos == string::npos) {
        pos = 0;
    } else {
        pos++;              // skip the /
    }
    ui->loadedFileName->setText(fileName.substr(pos).c_str());
    ui->loadedFileName->repaint();
    if(DataItem::readData(fileName, coprocReq, hostReq) == false) {
        // aborted read
        ui->loadedFileName->setText("");
        ui->loadedFileName->repaint();
        return;
    }
    sprintf(buf, "%ld", coprocReq);
    ui->coprocReq->setText(buf);
    sprintf(buf, "%ld", hostReq);
    ui->hostReq->setText(buf);
    //
    //  setup time bounds
    //
    startTimeMin = startTimeCurr = DataItem::getFirstStart();
    retireTimeMax = retireTimeCurr = DataItem::getLastRetire();
    sprintf(buf, "%ld ns", startTimeMin);
    ui->startTime->setText(buf);
    sprintf(buf, "%ld ns", retireTimeMax);
    ui->stopTime->setText(buf);
    deltaTimeMax = retireTimeMax - startTimeMin;
    widthMax = widthCurr = retireTimeMax - startTimeMin;
    ui->zoomIn->setEnabled(true);
    ui->zoomOut->setEnabled(false);
    zoomLevel = 1;
    updateSlider();                     // can't fail
    buildScene(scene);
}

void
MainWindow::zoomIn(bool /*checked*/)
{
    zoomLevel *= 2;
    if(updateSlider() == false) {
        ui->zoomIn->setEnabled(false);
        zoomLevel /= 2;                     // failed
    } else {
        ui->zoomOut->setEnabled(true);
        buildScene(scene);
    }
}

void
MainWindow::zoomOut(bool /*checked*/)
{
    zoomLevel /= 2;
    if(zoomLevel <= 1) {
        ui->zoomOut->setEnabled(false);
    }
    if(zoomLevel == 0) {
        zoomLevel = 1;
        return;                             // at top level
    }
    if(updateSlider() == false) {
        zoomLevel *= 2;                     // failed
    } else {
        ui->zoomIn->setEnabled(true);
        buildScene(scene);
    }
}

void
MainWindow::zoomFull(bool /*checked*/)
{
    zoomLevel = 1;
    updateSlider();
    ui->zoomIn->setEnabled(true);
    ui->zoomOut->setEnabled(false);
    buildScene(scene);
}

//
//  called when an item in the scene is selected
//
void
BankScene::keyPressEvent(QKeyEvent *event)
{
    mainW->keyPressEvent(event);
}

void
MainWindow::keyPressEvent(QKeyEvent *event)
{
    if(ui->selectLeft->isEnabled() == true
                && event->matches(QKeySequence::MoveToPreviousChar) == true) {
        selectLeft(true);
    } else if(ui->selectRight->isEnabled() == true
                    && event->matches(QKeySequence::MoveToNextChar) == true) {
        selectRight(true);
    } else {
        QMainWindow::keyPressEvent(event);
    }
    return;
}

void
MainWindow::selectLeft(bool /*checked*/)
{
    DataItem    *nextDataItemP;

    if(itemSelected == 0) {
        return;
    }
    nextDataItemP = DataItem::getPrevItem(itemSelected);
    if(updateBankSelectedItem(nextDataItemP, true) == true) { // true if in scene
        displayDataItem(itemSelected);
        return;
    }
    if(nextDataItemP != 0) {
        while(ui->timeScroll->value() > 0) {
            ui->timeScroll->setValue(ui->timeScroll->value() - 1);
            if(updateBankSelectedItem(nextDataItemP, true) == true) { // true if in scene
                displayDataItem(itemSelected);
                break;
            }
        }
    }
}

void
MainWindow::selectRight(bool /*checked*/)
{
    DataItem    *nextDataItemP;

    if(itemSelected == 0) {
       return;
    }
    nextDataItemP = DataItem::getNextItem(itemSelected);
    if(updateBankSelectedItem(nextDataItemP, false) == true) { // true if in scene
        displayDataItem(itemSelected);
        return;
    }
    if(nextDataItemP != 0) {
        while(ui->timeScroll->value() < ui->timeScroll->maximum()) {
            ui->timeScroll->setValue(ui->timeScroll->value() + 1);
            if(updateBankSelectedItem(nextDataItemP, true) == true) { // true if in scene
                displayDataItem(itemSelected);
                break;
            }
        }
    }
}
//
//  clear old selection
//  
//  if in scene
//      set bankItemSelected and return true
//
//  if not in scene
//      return false
//

bool
MainWindow::updateBankSelectedItem(DataItem *itemP, bool left)     // rtn true if in scene
{
    BankItem    *newBankItemSelected;
    int64_t     retireTime;
    int         i;
    int         cBank;

    if(bankItemSelected != 0) {
        bankItemSelected->setSelected(false);           // removes (calls pointSelected)
    }
    if(itemP == 0) {
        return(false);                                  // no item, not on screen
    }
    if(currentCtlr != itemP->getCtlr()
            && currentCtlr != CTLR_ALL_C
            && currentCtlr != CTLR_ALL_B) {
        return(false);                                  // controller not visible
    }
    retireTime = itemP->getRetireTime();
    if(currentCtlr < CTLR_MAX) {
        cBank = itemP->getBank();                       // bank we selected
    } else if(currentCtlr == CTLR_ALL_C) {                       // select all_c
        cBank = itemP->getCtlr() * 16;
        cBank += itemP->getBank() / 8;
    } else {                                            // select all_b
        cBank = itemP->getBank();
    }
    if(retireTime < startTimeCurr || retireTime > retireTimeCurr) {
        return(false);                                  // not in scene
    }
    for(i=0; i<X_MAX; i++) {
        if(retireTime <= pmiEndTime[i]) {               // pmiEndTime = end time of pixel
            break;
        }
    }
    newBankItemSelected = currPmi[i][cBank];
    while(newBankItemSelected == 0 && left == true && i > 0) {
        newBankItemSelected = currPmi[--i][cBank];
    }
    while(newBankItemSelected == 0 && left == false && i < X_MAX) {
        newBankItemSelected = currPmi[++i][cBank];
    }
    newBankItemSelected->itemP = itemP;
    newBankItemSelected->setSelected(true);             // calls pointSelected sets bankItemSelected
    itemSelected = itemP;
    return(true);
}

void
MainWindow::setTimeBounds()
{
    char    buf[256];
    int     i;
    int64_t delta;

    sprintf(buf, "%ld ns", startTimeCurr);
    ui->minTime->setText(buf);
    sprintf(buf, "%ld ns", retireTimeCurr);
    ui->maxTime->setText(buf);
    delta = retireTimeCurr - startTimeCurr;
    currDelta = ((float)(retireTimeCurr - startTimeCurr)) / (float)X_MAX;
    for(i=0; i<X_MAX; i++) {
        pmiEndTime[i] = (float)startTimeCurr + currDelta * (float)(i + 1);
    }
}

bool
MainWindow::updateSlider()
{
    //
    //  setup the slider
    //
    sliderUpdating = true;
    float zoom = zoomLevel;

    int pageStep = round(zoom);
    int newMax = (pageStep * pageStep) - pageStep;
    int oldPos = ui->timeScroll->sliderPosition();
    int oldMax = ui->timeScroll->maximum();
    int newPos = 0;
    
    float ct = (float)(startTimeCurr + retireTimeCurr) / 2.0;
    float pt = (float)deltaTimeMax / (float)(newMax + 1);
    newPos = (ct / pt);
    float le = pt * (float)newPos;                       // left edge of bar
    float re = le + pt + startTimeMin;
    int64_t startTimeCurrN = le;
    if(startTimeCurrN == 0) {
        startTimeCurrN = startTimeMin;
    }
    int64_t retireTimeCurrN = re;
    if(startTimeCurrN < startTimeMin || retireTimeCurrN > retireTimeMax) {
        sliderUpdating = false;
        return(false);
    }
    if(retireTimeCurrN - startTimeCurrN < X_MAX) {
        sliderUpdating = false;
        return(false);
    }
    startTimeCurr = startTimeCurrN;
    retireTimeCurr = retireTimeCurrN;
    
    ui->timeScroll->setValue(0);       // force in range
    ui->timeScroll->setMinimum(0);
    ui->timeScroll->setMaximum(newMax);

    ui->timeScroll->setPageStep(pageStep);
    ui->timeScroll->setSingleStep(1);
    ui->timeScroll->setValue(newPos);
    
    if(0) {
        printf("slider setup   newPos %d   newMax %d   pageStep %d   zoom %3.5f   oldPos %d   oldMax %d\n"
               "st %ld   et %ld   zoom %3.4f\n",
            newPos, newMax, pageStep, zoom, oldPos, oldMax, startTimeCurr, retireTimeCurr, zoom);
        fflush(stdout);
    }
    setTimeBounds();
    sliderUpdating = false;
    return(true);
}
//
// the value of the slider has changed for 1 of two reasons
//  1. setTimeBounds updated it, do nothing here
//  2. the user moved the slider bar (or bumped with arrows),
//      set startTimeCurr and retireTimeCurr based on scroll position
//
void
MainWindow::valueChanged(int value)
{
    if(0) {
        int pageStep = ui->timeScroll->pageStep();
        int pos = ui->timeScroll->sliderPosition();
        printf("slider value %d   pageStep %d   position %d\n", value, pageStep, pos); fflush(stdout);
    }
    if(sliderUpdating == true) {
        return;                             // being updated in setTimeBounds
    }

    int max = ui->timeScroll->maximum();
    float pt = (float)deltaTimeMax / (float)(max + 1);
    float le = pt * (float)value;                       // left edge of bar
    float re = le + pt;
    startTimeCurr = le;
    if(startTimeCurr < startTimeMin) {
        startTimeCurr = startTimeMin;
    }
    retireTimeCurr = re;
    setTimeBounds();
    buildScene(scene);
}

//
//  Select the correct memory controller
//
void
MainWindow::ctlrSelect(bool checked)
{
    int     i, j;
    char    buf[256];

    if(checked == false) {
        return;
    }
    for(i=0; i<CTLR_MAX + 2; i++) {
        if(ctlr[i]->isChecked() == true) {
            break;
        }
    }
    if(i == CTLR_ALL_C && currentCtlr != CTLR_ALL_C) {
        //  select all_c and prev was not all_c
        for(j=0; j<CTLR_MAX; j++) {
            sprintf(buf, "Mem Ctlr %d", j);
            label[j]->setText(buf);
        }
    } else if(i == CTLR_ALL_B && currentCtlr != CTLR_ALL_B) {
        //  select all_b and prev was not all_b
        for(j=0; j<CTLR_MAX; j++) {
            sprintf(buf, "Ctlr 0-7 B %d-%d", j * 16, (j * 16) + 15);
            label[j]->setText(buf);
        }
    } else if(i < 8 && currentCtlr != i) {
        //  select single and prev was all  Banks 0-15
        for(j=0; j<CTLR_MAX; j++) {
            sprintf(buf, "Ctlr %d  Bnks %d-%d", i, j * 16, (j * 16) + 15);
            label[j]->setText(buf);
        }
    }
    currentCtlr = i;
    buildScene(scene);
}

void
MainWindow::buildScene(QGraphicsScene *scene)
{
    int                 t, b;
    BankItem            *newPmi;
    int                 requests;
    int64_t             time;
    DataItem            *itemP;                 // first item in time
    int                 bwRaw[X_MAX];           // number of busy banks
    int                 bwCMax = 0;
    int                 laRaw[X_MAX];           // latency for busy banks
    int                 laMax = 0;              // max latency
    int                 bwHRaw[X_MAX];          // raw bandwidth
    int                 bwHMax = 0;
    int                 laHRaw[X_MAX];          // latency for host
    int                 laHMax = 0;             // max host latency
    float               delta = (retireTimeCurr - startTimeCurr) / (float)X_MAX;
    int                 latency;
    int                 actCnt = 0;
    DataItem            *selItemP = 0;
    char                buf[256];
    int                 retireBytes;
    float               numBanks;    

    
    if(fileNameSel.length() == 0) {
        return;
    }

    if(delta < 1.0) {
        delta = 1.0;                                        // data smaller than screen
    }
    if(bankItemSelected != 0) {
        selItemP = itemSelected;
        bankItemSelected->setSelected(false);               // clears bankItemSelected
    }
    numBanks = (currentCtlr >= CTLR_MAX) ? (BANKS * CTLR_MAX) : BANKS;
    markTime(true, "Start scene");

    StatusData status("Building Memory Scene...", 0, X_MAX * BANKS);

    for(t=0; t<X_MAX; t++) {
        time = startTimeCurr + (int64_t)(delta * t);
        bwRaw[t] = 0;
        laRaw[t] = 0;
        DataItem::getHostData(time, (int64_t)delta, retireBytes, latency);
        bwHRaw[t] = retireBytes;
        if(bwHRaw[t] > bwHMax) {
            bwHMax = bwHRaw[t];
        }
        laHRaw[t] = latency;
        if(latency > laHMax) {
            laHMax = latency;
        }
        for(b=0; b<BANKS; b++) {
            actCnt++;
            if((actCnt%1000) == 0) {
                status.setValue(actCnt);
                if(status.wasCanceled()) {
                    return;
                }
            }

            requests = DataItem::getDataPoint(time, (int64_t)delta, currentCtlr, b, &itemP, retireBytes, latency);
            if(requests > 0) {
                bwRaw[t] += retireBytes;                      // count for bw
                if(bwRaw[t] > bwCMax) {
                    bwCMax = bwRaw[t];
                }
            }
            if(requests == 0) {
                newPmi = 0;                                 // show background
            } else if(requests <= 1) {
                newPmi = pmiG[t][b];                        // show green
            } else if(requests <= 4) {
                newPmi = pmiY[t][b];                        // show yellow
            } else {
                newPmi = pmiR[t][b];                        // show red
            }
            if(currPmi[t][b] != 0) {
                scene->removeItem(currPmi[t][b]);           // remove old color
            }
            if(newPmi != 0) {
                newPmi->itemP = itemP;
                newPmi->memReq = requests;
                scene->addItem(newPmi);                    // put in new color
            }
            currPmi[t][b] = newPmi;
            if(LATENCY == L_MAX) {
                if(latency > laRaw[t]) {
                    laRaw[t] = latency;                         // show max
                }
            } else {
                laRaw[t] += latency;                          // show avg
            }
        }
        if(LATENCY == L_MAX) {
            if(laRaw[t] > laMax) {
                laMax = laRaw[t];
            }
        } else {
            //laRaw[t] /= numBanks;
        }

    }
    status.clear();
    markTime(false, "Stop Scene");
    //
    //  now build the bandwidth and latency scene
    //  if single ctlr 128 requests = max bw
    //  if all ctlrs 1024 requests = max bw
    //
    int             x, y;
    int             bwCAvg[X_MAX];
    int             bwHAvg[X_MAX];
    int             laHAvg[X_MAX];
    

    for(x=0; x<X_MAX; x++) {                // scale the data
        bwCAvg[x] = ((float)bwRaw[x] / (float)bwCMax) * (float)bwlaY;
        
        laRaw[x] = ((float)laRaw[x] / (float)laMax) * (float)bwlaY; // 0->bwLaY
        
        bwHAvg[x] = (((float)bwHRaw[x] / (float)bwHMax)) * (float)bwlaY;            // 0->bwlaY
        laHAvg[x] = (((float)laHRaw[x] / (float)laHMax)) * (float)bwlaY;            // 0->bwlaY
    }

    for(x=0; x<X_MAX; x++) {
        for(y=0; y<bwlaY; y++) {
            //
            //  clear the old data
            //
            if(currBwLa[x][y] != 0) {
                bwlaScene->removeItem(currBwLa[x][y]);
                currBwLa[x][y] = 0;
            }
            if(currBwLaH[x][y] != 0) {
                bwlaHScene->removeItem(currBwLaH[x][y]);
                currBwLaH[x][y] = 0;
            }
        }
        //
        //  add bw coproc
        //
        y  = bwlaY - bwCAvg[x];
        if(y >= bwlaY) y = bwlaY - 1;
        if(y < 0) y = 0;
        bwlaScene->addItem(bwlaR[x][y]);
        currBwLa[x][y] = bwlaR[x][y];
        //
        //  add latency coproc
        //
        y = bwlaY - laRaw[x];
        if(y >= bwlaY) y = bwlaY - 1;
        if(y < 0) y = 0;
        if(currBwLa[x][y] == 0) {
            bwlaScene->addItem(bwlaB[x][y]);
            currBwLa[x][y] = bwlaB[x][y];
        }
        //
        //  add bw host
        //
        y  = bwlaY - bwHAvg[x];
        if(y >= bwlaY) y = bwlaY - 1;
        if(y < 0) y = 0;
        bwlaHScene->addItem(bwlaRH[x][y]);
        currBwLaH[x][y] = bwlaRH[x][y];
        //
        //  add latency host
        //
        y = bwlaY - laHAvg[x];
        if(y >= bwlaY) y = bwlaY - 1;
        if(y < 0) y = 0;
        if(currBwLaH[x][y] == 0) {
            bwlaHScene->addItem(bwlaBH[x][y]);
            currBwLaH[x][y] = bwlaBH[x][y];
        }
    }
//    printf("bwHMax %d bytes in %.0f ns   %2.2f GB/s\n", bwHMax, delta, (float)bwHMax / (float)delta); fflush(stdout);
//    printf("bwCMax %d bytes in %.0f ns   %2.2f GB/s\n", bwCMax, delta, (float)bwCMax / (float)delta); fflush(stdout);
    
    sprintf(buf, "%2.2f GB/s", (float)bwHMax / (float)delta);
    ui->hostBwMax->setText(buf);
    sprintf(buf, "%2.2f GB/s", (float)bwCMax / (float)delta);
    ui->coprocBwMax->setText(buf);
    sprintf(buf, "%d ns", laMax);
    ui->coprocLaMax->setText(buf);
    sprintf(buf, "%d ns", laHMax);
    ui->hostLaMax->setText(buf);
    if(selItemP != 0 && updateBankSelectedItem(selItemP, false) == true) { // true if in scene
        displayDataItem(itemSelected);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

//
//  The user selected a point in the scene
//  or we cleared or set a selection
//
void
MainWindow::pointSelected()                  // selection changed
{
    QList<QGraphicsItem *>  selectList;
    int                     i;
    int                     x;
    int                     y;

    bankItemSelected = 0;
    selectList = scene->selectedItems();
    for(i=0; i<selectList.size(); i++) {
        bankItemSelected = (BankItem *)selectList.at(i);
    }
    if(bankItemSelected != 0) {
        x = bankItemSelected->x;
        y = bankItemSelected->y;
        itemSelected = currPmi[x][y]->itemP;
        if(currentCtlr == CTLR_ALL_C) {
            y = itemSelected->getCtlr() * 16;
            y += itemSelected->getBank() / 8;
        }
        if(markerDisplayed == true) {
            scene->removeItem(markerItemX);
            scene->removeItem(markerItemY);
        }
        markerItemX->setOffset(x * 2 - 10, y * 3);
        scene->addItem(markerItemX);
        markerItemY->setOffset(x * 2, y * 3 - 9);
        scene->addItem(markerItemY);
        markerDisplayed = true;
    } else {
        itemSelected = 0;
        if(markerDisplayed == true) {
            scene->removeItem(markerItemX);
            scene->removeItem(markerItemY);
            markerDisplayed = false;
        }
    }
    displayDataItem(itemSelected);
}

void
MainWindow::displayDataItem(DataItem *itemP)
{
    char                    bufB[256];
    char                    bufR[256];
    char                    bufC[256];
    char                    bufT[256];
    char                    bufI[256];
    char                    bufM[256];
    char                    bufA[256];
    char                    bufF[256];
    char                    bufL[256];
    int                     i;

    if(itemP != 0) {
        ui->selectLeft->setEnabled(true);
        ui->selectRight->setEnabled(true);
        sprintf(bufC, "%d", itemP->getCtlr());
        if(1||currentCtlr < CTLR_MAX) {
            sprintf(bufB, "%d", itemP->getBank());      // bank from item
        } else {
            i = itemP->getBank() & (~0x07);
            sprintf(bufB, "%d-%d", (i%16) * 8, ((i%16) * 8) + 7);
        }
        sprintf(bufT, "%ld ns", itemP->getStartTime());
        sprintf(bufI, "%ld ns", itemP->getRetireTime());
        switch(itemP->getMemOp()) {
            case DataItem::RD:  sprintf(bufM, "Rd");    break;
            case DataItem::WR:  sprintf(bufM, "Wr");    break;
            default:            sprintf(bufM, "Unk%d", itemP->getMemOp()); break;
        }
        const char    *srcFileName;
        int     srcLineNumber;

        sprintf(bufA, "0x%lx", itemP->getAddress());
        itemP->getSrcFileLine(srcFileName, srcLineNumber);
        sprintf(bufF, "%s", srcFileName);
        sprintf(bufL, "%d", srcLineNumber);
        if(bankItemSelected != 0) {
            sprintf(bufR, "%d", bankItemSelected->memReq);
            ui->outstandingRequests->setText(bufR);         // outstanding requests
        } else {
            bufR[0] = 0;
        }
    } else {
        ui->selectLeft->setEnabled(false);
        ui->selectRight->setEnabled(false);
        bufB[0] = bufR[0] = bufC[0] = bufT[0] = bufI[0] = 0;
        bufM[0] = bufA[0] = bufF[0] = bufL[0] = bufR[0] = 0;
    }
    ui->requestStart->setText(bufT);
    ui->requestRetire->setText(bufI);
    ui->memCtlr->setText(bufC);                     // memory controller
    ui->bank->setText(bufB);                        // memory bank
    ui->outstandingRequests->setText(bufR);         // outstanding requests
    ui->requestType->setText(bufM);
    ui->requestAddress->setText(bufA);
    ui->fileName->setText(bufF);
    ui->fileLine->setText(bufL);
}






