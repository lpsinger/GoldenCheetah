/*
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
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

#ifndef _GC_PowerHist_h
#define _GC_PowerHist_h 1
#include "GoldenCheetah.h"
#include "RideFile.h"
#include "MainWindow.h"
#include "Zones.h"
#include "HrZones.h"

#include <qwt_plot.h>
#include <qwt_plot_zoomer.h>
#include <qwt_compat.h>
#include <qsettings.h>
#include <qvariant.h>


class QwtPlotCurve;
class QwtPlotGrid;
class MainWindow;
class RideItem;
struct RideFilePoint;
class RideFileCache;
class HistogramWindow;
class PowerHistBackground;
class PowerHistZoneLabel;
class HrHistBackground;
class HrHistZoneLabel;
class LTMCanvasPicker;
class ZoneScaleDraw;


class penTooltip: public QwtPlotZoomer
{
    public:
         penTooltip(QwtPlotCanvas *canvas): QwtPlotZoomer(canvas), tip("") {
             // With some versions of Qt/Qwt, setting this to AlwaysOn
             // causes an infinite recursion.
             //setTrackerMode(AlwaysOn);
             setTrackerMode(AlwaysOn);
         }

        virtual QwtText trackerText(const QPoint &/*pos*/) const {
            QColor bg = QColor(255,255, 170); // toolyip yellow
#if QT_VERSION >= 0x040300
            bg.setAlpha(200);
#endif
            QwtText text;
            QFont def;
            //def.setPointSize(8); // too small on low res displays (Mac)
            //double val = ceil(pos.y()*100) / 100; // round to 2 decimal place
            //text.setText(QString("%1 %2").arg(val).arg(format), QwtText::PlainText);
            text.setText(tip);
            text.setFont(def);
            text.setBackgroundBrush( QBrush( bg ));
            text.setRenderFlags(Qt::AlignLeft | Qt::AlignTop);
            return text;
        }

        void setFormat(QString fmt) { format = fmt; }
        void setText(QString txt) { tip = txt; }

    private:
        QString format;
        QString tip;
};

class PowerHist : public QwtPlot
{
    Q_OBJECT
    G_OBJECT

    friend class ::HrHistBackground;
    friend class ::HrHistZoneLabel;
    friend class ::PowerHistBackground;
    friend class ::PowerHistZoneLabel;
    friend class ::HistogramWindow;

    public:

        PowerHist(MainWindow *mainWindow);
        ~PowerHist();

        double minX;

    public slots:

        // public setters
        void setShading(bool x) { shade=x; }
        void setSeries(RideFile::SeriesType series);
        void setData(RideItem *_rideItem, bool force=false);
        void setData(RideFileCache *source);
        void setlnY(bool value);
        void setWithZeros(bool value);
        void setZoned(bool value);
        void setSumY(bool value);
        void configChanged();
        void setAxisTitle(int axis, QString label);
        void setYMax();
        void setBinWidth(double value);
        int setBinWidthRealUnits(double value);

        // public getters
        double getDelta();
        double getBinWidthRealUnits();
        int getDigits();
        inline bool islnY() const { return lny; }
        inline bool withZeros() const { return withz; }
        inline double binWidth() const { return binw; }

        // react to plot signals
        void pointHover(QwtPlotCurve *curve, int index);

        // get told to refresh
        void recalc(bool force=false);
        void refreshZoneLabels();

        // redraw, reset zoom base
        void updatePlot();

    protected:

        void refreshHRZoneLabels();
        void setParameterAxisTitle();
        bool isSelected(const RideFilePoint *p, double);
        void percentify(QVector<double> &, double factor); // and a function to convert

        bool shadeZones() const; // check if zone shading is both wanted and possible
        bool shadeHRZones() const; // check if zone shading is both wanted and possible

        // plot settings
        RideItem *rideItem;
        MainWindow *mainWindow;
        RideFile::SeriesType series;
        bool lny;
        bool shade;
        bool zoned;        // show in zones
        double binw;
        bool withz;        // whether zeros are included in histogram
        double dt;         // length of sample
        bool absolutetime; // do we sum absolute or percentage?

    private:

        // plot objects
        QwtPlotGrid *grid;
        PowerHistBackground *bg;
        HrHistBackground *hrbg;
        penTooltip *zoomer;
        LTMCanvasPicker *canvasPicker;
        QwtPlotCurve *curve, *curveSelected;
        QList <PowerHistZoneLabel *> zoneLabels;
        QList <HrHistZoneLabel *> hrzoneLabels;

        // source cache
        RideFileCache *cache;

        // discritized unit for smoothing
        static const double wattsDelta = 1.0;
        static const double wattsKgDelta = 0.01;
        static const double nmDelta    = 0.1;
        static const double hrDelta    = 1.0;
        static const double kphDelta   = 0.1;
        static const double cadDelta   = 1.0;

        // digits for text entry validator
        static const int wattsDigits = 0;
        static const int wattsKgDigits = 2;
        static const int nmDigits    = 1;
        static const int hrDigits    = 0;
        static const int kphDigits   = 1;
        static const int cadDigits   = 0;

        // storage for data counts
        QVector<unsigned int> wattsArray, wattsZoneArray, wattsKgArray, nmArray, hrArray,
                              hrZoneArray, kphArray, cadArray;

        // storage for data counts in interval selected
        QVector<unsigned int> wattsSelectedArray, wattsZoneSelectedArray,
                              wattsKgSelectedArray,
                              nmSelectedArray, hrSelectedArray,
                              hrZoneSelectedArray, kphSelectedArray,
                              cadSelectedArray;

        enum Source { Ride, Cache } source, LASTsource;

        // last plot settings - to avoid lots of uneeded recalcs
        RideItem *LASTrideItem;
        RideFileCache *LASTcache;
        RideFile::SeriesType LASTseries;
        bool LASTshade;
        bool LASTuseMetricUnits;  // whether metric units are used (or imperial)
        bool LASTlny;
        bool LASTzoned;        // show in zones
        double LASTbinw;
        bool LASTwithz;        // whether zeros are included in histogram
        double LASTdt;         // length of sample
        bool LASTabsolutetime; // do we sum absolute or percentage?
};

/*----------------------------------------------------------------------
 * From here to the end of source file the routines for zone shading
 *--------------------------------------------------------------------*/

// define a background class to handle shading of power zones
// draws power zone bands IF zones are defined and the option
// to draw bonds has been selected
class PowerHistBackground: public QwtPlotItem
{
private:
    PowerHist *parent;

public:
    PowerHistBackground(PowerHist *_parent)
    {
        setZ(0.0);
	parent = _parent;
    }

    virtual int rtti() const
    {
        return QwtPlotItem::Rtti_PlotUserItem;
    }

    virtual void draw(QPainter *painter,
		      const QwtScaleMap &xMap, const QwtScaleMap &,
                      const QRectF &rect) const
    {
	RideItem *rideItem = parent->rideItem;

	if (! rideItem)
	    return;

	const Zones *zones       = rideItem->zones;
	int zone_range     = rideItem->zoneRange();

	if (parent->shadeZones() && (zone_range >= 0)) {
	    QList <int> zone_lows = zones->getZoneLows(zone_range);
	    int num_zones = zone_lows.size();
	    if (num_zones > 0) {
		for (int z = 0; z < num_zones; z ++) {
                    QRectF r = rect;

		    QColor shading_color =
			zoneColor(z, num_zones);
		    shading_color.setHsv(
					 shading_color.hue(),
					 shading_color.saturation() / 4,
					 shading_color.value()
					 );

            double wattsLeft = zone_lows[z];
            if (parent->series == RideFile::wattsKg) {
                wattsLeft = wattsLeft / rideItem->ride()->getWeight();
            }
            r.setLeft(xMap.transform(wattsLeft));

            if (z + 1 < num_zones)  {
                double wattsRight = zone_lows[z + 1];
                if (parent->series == RideFile::wattsKg) {
                    wattsRight = wattsRight / rideItem->ride()->getWeight();
                }
                r.setRight(xMap.transform(wattsRight));
            }

		    if (r.right() >= r.left())
			painter->fillRect(r, shading_color);
		}
	    }
	}
    }
};


// Zone labels are drawn if power zone bands are enabled, automatically
// at the center of the plot
class PowerHistZoneLabel: public QwtPlotItem
{
private:
    PowerHist *parent;
    int zone_number;
    double watts;
    QwtText text;

public:
    PowerHistZoneLabel(PowerHist *_parent, int _zone_number)
    {
	parent = _parent;
	zone_number = _zone_number;

	RideItem *rideItem = parent->rideItem;

	if (! rideItem)
	    return;

	const Zones *zones       = rideItem->zones;
	int zone_range     = rideItem->zoneRange();

	setZ(1.0 + zone_number / 100.0);

	// create new zone labels if we're shading
	if (parent->shadeZones() && (zone_range >= 0)) {
	    QList <int> zone_lows = zones->getZoneLows(zone_range);
	    QList <QString> zone_names = zones->getZoneNames(zone_range);
	    int num_zones = zone_lows.size();
	    assert(zone_names.size() == num_zones);
	    if (zone_number < num_zones) {
		watts =
		    (
		     (zone_number + 1 < num_zones) ?
		     0.5 * (zone_lows[zone_number] + zone_lows[zone_number + 1]) :
		     (
		      (zone_number > 0) ?
		      (1.5 * zone_lows[zone_number] - 0.5 * zone_lows[zone_number - 1]) :
		      2.0 * zone_lows[zone_number]
		      )
             );

		text = QwtText(zone_names[zone_number]);
		text.setFont(QFont("Helvetica",24, QFont::Bold));
		QColor text_color = zoneColor(zone_number, num_zones);
		text_color.setAlpha(64);
		text.setColor(text_color);
	    }
	}

    }

    virtual int rtti() const
    {
        return QwtPlotItem::Rtti_PlotUserItem;
    }

    void draw(QPainter *painter,
	      const QwtScaleMap &xMap, const QwtScaleMap &,
              const QRectF &rect) const
    {
        RideItem *rideItem = parent->rideItem;

    if (parent->shadeZones()) {
        double position = watts;
        if (parent->series == RideFile::wattsKg) {
            position = watts / rideItem->ride()->getWeight();
        }
        int x = xMap.transform(position);
	    int y = (rect.bottom() + rect.top()) / 2;

	    // the following code based on source for QwtPlotMarker::draw()
            QRect tr(QPoint(0, 0), text.textSize(painter->font()).toSize());
	    tr.moveCenter(QPoint(y, -x));
	    painter->rotate(90);             // rotate text to avoid overlap: this needs to be fixed
	    text.draw(painter, tr);
	}
    }
};

// define a background class to handle shading of HR zones
// draws power zone bands IF zones are defined and the option
// to draw bonds has been selected
class HrHistBackground: public QwtPlotItem
{
private:
    PowerHist *parent;

public:
    HrHistBackground(PowerHist *_parent)
    {
        setZ(0.0);
	parent = _parent;
    }

    virtual int rtti() const
    {
        return QwtPlotItem::Rtti_PlotUserItem;
    }

    virtual void draw(QPainter *painter,
		      const QwtScaleMap &xMap, const QwtScaleMap &,
                      const QRectF &rect) const
    {
	RideItem *rideItem = parent->rideItem;

	if (! rideItem)
	    return;

	const HrZones *zones       = parent->mainWindow->hrZones();
	int zone_range     = rideItem->hrZoneRange();

	if (parent->shadeHRZones() && (zone_range >= 0)) {
	    QList <int> zone_lows = zones->getZoneLows(zone_range);
	    int num_zones = zone_lows.size();
	    if (num_zones > 0) {
		for (int z = 0; z < num_zones; z ++) {
                    QRectF r = rect;

		    QColor shading_color =
			hrZoneColor(z, num_zones);
		    shading_color.setHsv(
					 shading_color.hue(),
					 shading_color.saturation() / 4,
					 shading_color.value()
					 );
		    r.setLeft(xMap.transform(zone_lows[z]));
		    if (z + 1 < num_zones)
			r.setRight(xMap.transform(zone_lows[z + 1]));
		    if (r.right() >= r.left())
			painter->fillRect(r, shading_color);
		}
	    }
	}
    }
};


// Zone labels are drawn if power zone bands are enabled, automatically
// at the center of the plot
class HrHistZoneLabel: public QwtPlotItem
{
private:
    PowerHist *parent;
    int zone_number;
    double watts;
    QwtText text;

public:
    HrHistZoneLabel(PowerHist *_parent, int _zone_number)
    {
	parent = _parent;
	zone_number = _zone_number;

	RideItem *rideItem = parent->rideItem;

	if (! rideItem)
	    return;

	const HrZones *zones       = parent->mainWindow->hrZones();
	int zone_range     = rideItem->hrZoneRange();

	setZ(1.0 + zone_number / 100.0);

	// create new zone labels if we're shading
	if (parent->shadeHRZones() && (zone_range >= 0)) {
	    QList <int> zone_lows = zones->getZoneLows(zone_range);
	    QList <QString> zone_names = zones->getZoneNames(zone_range);
	    int num_zones = zone_lows.size();
	    assert(zone_names.size() == num_zones);
	    if (zone_number < num_zones) {
                double min = parent->minX;
                if (zone_lows[zone_number]>min)
                    min = zone_lows[zone_number];

                watts =
		    (
		     (zone_number + 1 < num_zones) ?
                     0.5 * (min + zone_lows[zone_number + 1]) :
		     (
		      (zone_number > 0) ?
		      (1.5 * zone_lows[zone_number] - 0.5 * zone_lows[zone_number - 1]) :
		      2.0 * zone_lows[zone_number]
		      )
		     );

		text = QwtText(zone_names[zone_number]);
		text.setFont(QFont("Helvetica",24, QFont::Bold));
		QColor text_color = hrZoneColor(zone_number, num_zones);
		text_color.setAlpha(64);
		text.setColor(text_color);
	    }
	}

    }

    virtual int rtti() const
    {
        return QwtPlotItem::Rtti_PlotUserItem;
    }

    void draw(QPainter *painter,
	      const QwtScaleMap &xMap, const QwtScaleMap &,
              const QRectF &rect) const
    {
	if (parent->shadeHRZones()) {
            int x = xMap.transform(watts);
	    int y = (rect.bottom() + rect.top()) / 2;

	    // the following code based on source for QwtPlotMarker::draw()
            QRect tr(QPoint(0, 0), text.textSize(painter->font()).toSize());
	    tr.moveCenter(QPoint(y, -x));
	    painter->rotate(90);             // rotate text to avoid overlap: this needs to be fixed
	    text.draw(painter, tr);
	}
    }
};

#endif // _GC_PowerHist_h
