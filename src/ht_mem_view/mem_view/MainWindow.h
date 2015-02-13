/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QGraphicsItem>
#include <QRadioButton>
#include <QLabel>
#include <QProgressBar>
#include <QProgressDialog>
#include <QStatusBar>
#include <QPushButton>
#include <QKeyEvent>

#include "/usr/include/stdint.h"            // needed for uint64_t etc

namespace Ui {
class MainWindow;
}

class DataItem;
class MainWindow;

class StatusData {
public:
    StatusData(const char *text, int min, int max)
            {
                pdP = new QProgressDialog(text, "Cancel", min, max, parent);
                pdP->setWindowModality(Qt::ApplicationModal);
                pdP->move(xPos, yPos);
                this->max = max;
            }
    static void    setPosition(QWidget *parent, int x, int y)
            {
                xPos = x;
                yPos = y;
                parent = parent;
            }
    void    setValue(int val)
            {
                pdP->setValue(val);
            }
    void    clear()
            {
                pdP->setValue(max);
            }
    bool    wasCanceled()
            {
                return(pdP->wasCanceled());
            }
    
private:
    QProgressDialog *pdP;
    int             max;
    static int      xPos;
    static int      yPos;
    static QWidget *parent;
};

class BankScene : public QGraphicsScene
{

public:
        BankScene(MainWindow *mainW, int xOrig, int yOrig, int xSize, int ySize)
            : QGraphicsScene(xOrig, yOrig, xSize, ySize)
        { this->mainW = mainW; }
        
   void    keyPressEvent(QKeyEvent *event);
   MainWindow   *mainW;
};


class BankItem : public QGraphicsPixmapItem
{
public:
        BankItem(int x, int y, const QPixmap &pixmap, QGraphicsItem *parent = 0)
           :  QGraphicsPixmapItem( pixmap, parent)
           { this->x = x; this->y = y; itemP=0; memReq=0;}

    int         x;
    int         y;
    DataItem    *itemP;
    int         memReq;
};
class BwLaItem : public QGraphicsPixmapItem
{
public:
        BwLaItem(int x, int y, const QPixmap &pixmap, QGraphicsItem *parent = 0)
           :  QGraphicsPixmapItem( pixmap, parent)
           { this->x = x; this->y = y;}

    int         x;
    int         y;
};
class MarkerItem : public QGraphicsPixmapItem
{
public:
        MarkerItem(int x, int y, const QPixmap &pixmap, QGraphicsItem *parent = 0)
           :  QGraphicsPixmapItem( pixmap, parent)
           { this->x = x; this->y = y;}

    int         x;
    int         y;
};

class MainWindow : public QMainWindow
{
Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
            ~MainWindow();

public slots:
    void    pointSelected();
    void    ctlrSelect(bool checked);               // select the memory ctlr or all
    void    zoomIn(bool checked);
    void    zoomFull(bool checked);
    void    zoomOut(bool checked);
    void    selectLeft(bool checked);
    void    selectRight(bool checked);
    void    open();
    void    fileSelected(QString fileSel) { fileNameSel = fileSel; };
    void    valueChanged(int value);
    

public:
    void    keyPressEvent(QKeyEvent *event);

private:
    Ui::MainWindow *ui;

    BankScene           *scene;
    BankScene           *bwlaScene;
    BankScene           *bwlaHScene;
    BankItem            *bankItemSelected;
    DataItem            *itemSelected;
    void                buildScene(QGraphicsScene *scene);
    void                setTimeBounds();
    bool                updateSlider();
    void                displayDataItem(DataItem *itemP);
    bool                updateBankSelectedItem(DataItem *itemP, bool left);

#define X_MAX           256     // time
#define BANKS           128     // banks
#define CTLR_MAX        8       // number of memory controllers
#define CTLR_ALL_C      (CTLR_MAX + 0)
#define CTLR_ALL_B      (CTLR_MAX + 1)

     BankItem           *pmiR[X_MAX][BANKS], *pmiG[X_MAX][BANKS], *pmiY[X_MAX][BANKS];
     BankItem           *currPmi[X_MAX][BANKS];
     BankItem           *pmiHR[X_MAX], *pmiHG[X_MAX], *pmiHY[X_MAX];
     BankItem           *currHPmi[X_MAX];
     int                bwlaY;
     BwLaItem           **bwlaR[X_MAX];                 // coproc display
     BwLaItem           **bwlaB[X_MAX];
     BwLaItem           **currBwLaH[X_MAX];             // host display
     BwLaItem           **bwlaRH[X_MAX];
     BwLaItem           **bwlaBH[X_MAX];
     BwLaItem           **currBwLa[X_MAX];
     float              pmiEndTime[X_MAX];              // end time of each pixel/box
     MarkerItem         *markerItemX;                   // marker for selection
     MarkerItem         *markerItemY;                   // marker for selection
     bool               markerDisplayed;
     float              currDelta;

     QRadioButton   *ctlr[CTLR_MAX + 2];               // +0= all_c  +1=all_b 
     int            currentCtlr;                       // current mem controller selected
     QLabel         *label[8];

     int64_t       startTimeMin;
     int64_t       startTimeCurr;
     int64_t       retireTimeMax;
     int64_t       retireTimeCurr;
     int64_t       deltaTimeMax;
     int64_t       widthMax;
     int64_t       widthCurr;
     QString        fileNameSel;
     bool           sliderUpdating;
     int            zoomLevel;              // 1.2.4.8...
};

#endif // MAINWINDOW_H
