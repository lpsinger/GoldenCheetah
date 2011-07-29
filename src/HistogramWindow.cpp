/*
 * Copyright (c) 2009 Sean C. Rhea (srhea@srhea.net)
 *               2011 Mark Liversedge (liversedge@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "HistogramWindow.h"
#include "MainWindow.h"
#include "PowerHist.h"
#include "RideFile.h"
#include "RideFileCache.h"
#include "RideItem.h"
#include "Settings.h"
#include <QtGui>
#include <assert.h>

#include "Zones.h"
#include "HrZones.h"

HistogramWindow::HistogramWindow(MainWindow *mainWindow) : GcWindow(mainWindow), mainWindow(mainWindow), source(NULL)
{
    setInstanceName("Histogram Window");

    QWidget *c = new QWidget;
    QVBoxLayout *cl = new QVBoxLayout(c);
    setControls(c);

    // plot
    QVBoxLayout *vlayout = new QVBoxLayout;
    powerHist = new PowerHist(mainWindow);
    vlayout->addWidget(powerHist);
    setLayout(vlayout);

    // bin width
    QHBoxLayout *binWidthLayout = new QHBoxLayout;
    QLabel *binWidthLabel = new QLabel(tr("Bin width"), this);
    binWidthLineEdit = new QLineEdit(this);
    binWidthLineEdit->setFixedWidth(30);

    binWidthLayout->addWidget(binWidthLabel);
    binWidthLayout->addWidget(binWidthLineEdit);
    binWidthSlider = new QSlider(Qt::Horizontal);
    binWidthSlider->setTickPosition(QSlider::TicksBelow);
    binWidthSlider->setTickInterval(1);
    binWidthSlider->setMinimum(1);
    binWidthSlider->setMaximum(100);
    binWidthLayout->addWidget(binWidthSlider);
    cl->addLayout(binWidthLayout);

    showLnY = new QCheckBox;
    showLnY->setText(tr("Log Y"));
    cl->addWidget(showLnY);

    showZeroes = new QCheckBox;
    showZeroes->setText(tr("With zeros"));
    cl->addWidget(showZeroes);

    shadeZones = new QCheckBox;
    shadeZones->setText(tr("Shade zones"));
    shadeZones->setChecked(powerHist->shade);
    cl->addWidget(shadeZones);

    showInZones = new QCheckBox;
    showInZones->setText(tr("Show in zones"));
    cl->addWidget(showInZones);

    seriesCombo = new QComboBox();
    addSeries();
    cl->addWidget(seriesCombo);

    showSumY = new QComboBox();
    showSumY->addItem(tr("Absolute Time"));
    showSumY->addItem(tr("Percentage Time"));
    seasonCombo = new QComboBox(this);
    addSeasons();
    seasonCombo->setCurrentIndex(0); // default to current ride selected
    cl->addWidget(showSumY);
    cl->addWidget(seasonCombo);
    cl->addStretch();

    // sort out default values
    setHistTextValidator();
    showLnY->setChecked(powerHist->islnY());
    showZeroes->setChecked(powerHist->withZeros());
    binWidthSlider->setValue(powerHist->binWidth());
    setHistBinWidthText();

    // set the defaults etc
    updateChart();

    // the bin slider/input update each other
    // only the input box triggers an update to the chart
    connect(binWidthSlider, SIGNAL(valueChanged(int)), this, SLOT(setBinWidthFromSlider()));
    connect(binWidthLineEdit, SIGNAL(editingFinished()), this, SLOT(setBinWidthFromLineEdit()));

    // when season changes we need to retrieve data from the cache then update the chart
    connect(seasonCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(seasonSelected(int)));

    // if any of the controls change we pass the chart everything
    connect(showLnY, SIGNAL(stateChanged(int)), this, SLOT(updateChart()));
    connect(showZeroes, SIGNAL(stateChanged(int)), this, SLOT(updateChart()));
    connect(seriesCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateChart()));
    connect(showInZones, SIGNAL(stateChanged(int)), this, SLOT(updateChart()));
    connect(shadeZones, SIGNAL(stateChanged(int)), this, SLOT(updateChart()));
    connect(showSumY, SIGNAL(currentIndexChanged(int)), this, SLOT(updateChart()));

    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
    connect(mainWindow, SIGNAL(intervalSelected()), this, SLOT(intervalSelected()));
    connect(mainWindow, SIGNAL(zonesChanged()), this, SLOT(zonesChanged()));
    connect(mainWindow, SIGNAL(configChanged()), powerHist, SLOT(configChanged()));
}

void
HistogramWindow::rideSelected()
{
    if (!amVisible()) return;

    RideItem *ride = myRideItem;
    if (!ride || seasonCombo->currentIndex() != 0) return;

    // get range that applies to this ride
    powerRange = mainWindow->zones()->whichRange(ride->dateTime.date());
    hrRange = mainWindow->hrZones()->whichRange(ride->dateTime.date());

    // update
    updateChart();
}

void
HistogramWindow::intervalSelected()
{
    if (!amVisible()) return;

    RideItem *ride = myRideItem;

    // null? or not plotting current ride, ignore signal
    if (!ride || seasonCombo->currentIndex() != 0) return;

    // update
    interval = true;
    updateChart();
}

void
HistogramWindow::zonesChanged()
{
    if (!amVisible()) return;

    powerHist->refreshZoneLabels();
    powerHist->replot();
}

void HistogramWindow::seasonSelected(int index)
{
    RideFileCache *old = source;

    if (index > 0) {
        index--; // it is now an index into the season array

        // Set data from BESTS
        Season season = seasons.at(index);
        QDate start = season.getStart();
        QDate end = season.getEnd();
        if (end == QDate()) end = QDate(3000,12,31);
        if (start == QDate()) start = QDate(1900,1,1);
        source = new RideFileCache(mainWindow, start, end);

        if (old) delete old; // guarantee source pointer changes
    }
    updateChart();
}

void HistogramWindow::addSeries()
{
    // setup series list
    seriesList << RideFile::watts
               << RideFile::hr
               << RideFile::kph
               << RideFile::cad
               << RideFile::nm;

    foreach (RideFile::SeriesType x, seriesList) 
        seriesCombo->addItem(RideFile::seriesName(x), static_cast<int>(x));
}
void HistogramWindow::addSeasons()
{
    QFile seasonFile(mainWindow->home.absolutePath() + "/seasons.xml");
    QXmlInputSource source( &seasonFile );
    QXmlSimpleReader xmlReader;
    SeasonParser( handler );
    xmlReader.setContentHandler(&handler);
    xmlReader.setErrorHandler(&handler);
    xmlReader.parse( source );
    seasons = handler.getSeasons();
    Season season;
    season.setName(tr("All Seasons"));
    seasons.insert(0,season);

    seasonCombo->addItem("Selected Ride");
    foreach (Season season, seasons)
        seasonCombo->addItem(season.getName());
}

void
HistogramWindow::setBinWidthFromSlider()
{
    if (powerHist->binWidth() != binWidthSlider->value()) {
        powerHist->setBinWidth(binWidthSlider->value());
        setHistBinWidthText();

        updateChart();
    }
}

void
HistogramWindow::setHistBinWidthText()
{
    binWidthLineEdit->setText(QString("%1").arg(powerHist->getBinWidthRealUnits(), 0, 'g', 3));
}

void
HistogramWindow::setHistTextValidator()
{
    double delta = powerHist->getDelta();
    int digits = powerHist->getDigits();

    QValidator *validator;
    if (digits == 0) {

        validator = new QIntValidator(binWidthSlider->minimum() * delta,
                                      binWidthSlider->maximum() * delta,
                                      binWidthLineEdit);
    } else {

        validator = new QDoubleValidator(binWidthSlider->minimum() * delta,
                                         binWidthSlider->maximum() * delta,
                                         digits,
                                         binWidthLineEdit);
    }
    binWidthLineEdit->setValidator(validator);
}

void
HistogramWindow::setBinWidthFromLineEdit()
{
    double value = binWidthLineEdit->text().toDouble();
    if (value != powerHist->binWidth()) {
        binWidthSlider->setValue(powerHist->setBinWidthRealUnits(value));
        setHistBinWidthText();

        updateChart();
    }
}

void
HistogramWindow::updateChart()
{
    // set data
    if (seasonCombo->currentIndex() != 0 && source)
        powerHist->setData(source);
    else if (seasonCombo->currentIndex() == 0 && myRideItem)
        powerHist->setData(myRideItem, interval); // intervals selected forces data to
                                                  // be recomputed since interval selection
                                                  // has changed.

    // and now the controls
    powerHist->setShading(shadeZones->isChecked() ? true : false);
    powerHist->setZoned(showInZones->isChecked() ? true : false);
    powerHist->setlnY(showLnY->isChecked() ? true : false);
    powerHist->setWithZeros(showZeroes->isChecked() ? true : false);
    powerHist->setSumY(showSumY->currentIndex()== 0 ? true : false);
    powerHist->setBinWidth(binWidthLineEdit->text().toDouble());

    // and which series to plot
    powerHist->setSeries(static_cast<RideFile::SeriesType>(seriesCombo->itemData(seriesCombo->currentIndex()).toInt()));

    // now go plot yourself
    //powerHist->setAxisTitle(int axis, QString label);
    powerHist->recalc(interval); // interval changed? force recalc
    powerHist->replot();

    interval = false;// we force a recalc whem called coz intervals
                     // have been selected. The recalc routine in
                     // powerhist optimises out, but doesn't keep track
                     // of interval selection -- simplifies the setters
                     // and getters, so worth this 'hack'.
}
