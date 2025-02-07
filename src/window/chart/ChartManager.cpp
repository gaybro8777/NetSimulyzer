/*
 * NIST-developed software is provided by NIST as a public service. You may use,
 * copy and distribute copies of the software in any medium, provided that you
 * keep intact this entire notice. You may improve,modify and create derivative
 * works of the software or any portion of the software, and you may copy and
 * distribute such modifications or works. Modified works should carry a notice
 * stating that you changed the software and should note the date and nature of
 * any such change. Please explicitly acknowledge the National Institute of
 * Standards and Technology as the source of the software.
 *
 * NIST-developed software is expressly provided "AS IS." NIST MAKES NO
 * WARRANTY OF ANY KIND, EXPRESS, IMPLIED, IN FACT OR ARISING BY OPERATION OF
 * LAW, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTY OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, NON-INFRINGEMENT
 * AND DATA ACCURACY. NIST NEITHER REPRESENTS NOR WARRANTS THAT THE
 * OPERATION OF THE SOFTWARE WILL BE UNINTERRUPTED OR ERROR-FREE, OR THAT
 * ANY DEFECTS WILL BE CORRECTED. NIST DOES NOT WARRANT OR MAKE ANY
 * REPRESENTATIONS REGARDING THE USE OF THE SOFTWARE OR THE RESULTS THEREOF,
 * INCLUDING BUT NOT LIMITED TO THE CORRECTNESS, ACCURACY, RELIABILITY,
 * OR USEFULNESS OF THE SOFTWARE.
 *
 * You are solely responsible for determining the appropriateness of using and
 * distributing the software and you assume all risks associated with its use,
 * including but not limited to the risks and costs of program errors,
 * compliance with applicable laws, damage to or loss of data, programs or
 * equipment, and the unavailability or interruption of operation. This
 * software is not intended to be used in any situation where a failure could
 * cause risk of injury or damage to property. The software developed by NIST
 * employees is not subject to copyright protection within the United States.
 *
 * Author: Evan Black <evan.black@nist.gov>
 */

#include "ChartManager.h"
#include "ChartWidget.h"
#include <QDockWidget>
#include <QGraphicsLayout>
#include <QMainWindow>
#include <QMessageBox>
#include <QString>
#include <QtCharts/QCategoryAxis>
#include <QtCharts/QLogValueAxis>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QSplineSeries>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace {

void updateRange(QtCharts::QAbstractAxis *axis, qreal value) {
  // Amount to scale past the min/max
  // so we don't cut off the actual point
  const auto additionalScale = 0.05;

  qreal min;
  qreal max;
  if (const auto valueAxis = qobject_cast<QtCharts::QValueAxis *>(axis)) {
    min = valueAxis->min();
    max = valueAxis->max();
  } else if (const auto logAxis = qobject_cast<QtCharts::QLogValueAxis *>(axis)) {
    min = logAxis->min();
    max = logAxis->max();
  } else {
    std::cerr << "Error: Unhandled axis type in updateRange()\n";
    return;
  }

  if (value > max)
    axis->setMax(value + value * additionalScale);
  else if (value < min)
    axis->setMin(value - value * additionalScale);
}

} // namespace

namespace netsimulyzer {

ChartManager::XYSeriesTie ChartManager::makeTie(const parser::XYSeries &model) {
  ChartManager::XYSeriesTie tie;
  tie.model = model;
  switch (model.connection) {
  case parser::XYSeries::Connection::None: {
    auto scatterSeries = new QtCharts::QScatterSeries(this);

    // Hide the borders of points, as they cover up other points
    scatterSeries->setBorderColor(QColor(Qt::transparent));

    // Cut this down, as the default size (15 at time of writing) is quite large
    scatterSeries->setMarkerSize(5.0);
    tie.qtSeries = scatterSeries;
  } break;
  case parser::XYSeries::Connection::Line:
    tie.qtSeries = new QtCharts::QLineSeries(this);
    break;
  case parser::XYSeries::Connection::Spline:
    tie.qtSeries = new QtCharts::QSplineSeries(this);
    break;
  }

  switch (model.labelMode) {
  case parser::XYSeries::LabelMode::Hidden:
    tie.qtSeries->setPointLabelsVisible(false);
    break;
  case parser::XYSeries::LabelMode::Shown:
    tie.qtSeries->setPointLabelsVisible(true);
    break;
  }

  // It seems there's some difficulty with this setting on macOS,
  // so disable it
#ifndef __APPLE__
  // Note, this will only work with line/spline/scatter plots
  // if we add more plot types, this will have to be disabled
  // See: https://doc.qt.io/qt-5/qabstractseries.html#useOpenGL-prop
  tie.qtSeries->setUseOpenGL(true);
#endif

  tie.qtSeries->setColor(QColor::fromRgb(model.color.red, model.color.green, model.color.blue));
  tie.qtSeries->setName(QString::fromStdString(model.legend));

  // X Axis
  if (tie.model.xAxis.scale == parser::ValueAxis::Scale::Linear)
    tie.xAxis = new QtCharts::QValueAxis(this);
  else
    tie.xAxis = new QtCharts::QLogValueAxis(this);

  tie.xAxis->setTitleText(QString::fromStdString(model.xAxis.name));
  tie.xAxis->setRange(model.xAxis.min, model.xAxis.max);

  // Y Axis
  if (tie.model.yAxis.scale == parser::ValueAxis::Scale::Linear)
    tie.yAxis = new QtCharts::QValueAxis(this);
  else
    tie.yAxis = new QtCharts::QLogValueAxis(this);
  tie.yAxis = new QtCharts::QValueAxis(this);
  tie.yAxis->setTitleText(QString::fromStdString(model.yAxis.name));
  tie.yAxis->setRange(model.yAxis.min, model.yAxis.max);

  return tie;
}

ChartManager::SeriesCollectionTie ChartManager::makeTie(const parser::SeriesCollection &model) {
  ChartManager::SeriesCollectionTie tie;
  tie.model = model;

  // X Axis
  if (tie.model.xAxis.scale == parser::ValueAxis::Scale::Linear)
    tie.xAxis = new QtCharts::QValueAxis(this);
  else
    tie.xAxis = new QtCharts::QLogValueAxis(this);
  tie.xAxis->setTitleText(QString::fromStdString(model.xAxis.name));
  tie.xAxis->setRange(model.xAxis.min, model.xAxis.max);

  // Y Axis
  if (tie.model.yAxis.scale == parser::ValueAxis::Scale::Linear)
    tie.yAxis = new QtCharts::QValueAxis(this);
  else
    tie.yAxis = new QtCharts::QLogValueAxis(this);
  tie.yAxis = new QtCharts::QValueAxis(this);
  tie.yAxis->setTitleText(QString::fromStdString(model.yAxis.name));
  tie.yAxis->setRange(model.yAxis.min, model.yAxis.max);

  return tie;
}

ChartManager::CategoryValueTie ChartManager::makeTie(const parser::CategoryValueSeries &model) {
  CategoryValueTie tie;
  tie.model = model;
  tie.qtSeries = new QtCharts::QLineSeries(this);

  tie.qtSeries->setColor(QColor::fromRgb(model.color.red, model.color.green, model.color.blue));
  tie.qtSeries->setName(QString::fromStdString(model.legend));

  // X Axis (values)
  if (tie.model.xAxis.scale == parser::ValueAxis::Scale::Linear)
    tie.xAxis = new QtCharts::QValueAxis(this);
  else
    tie.xAxis = new QtCharts::QLogValueAxis(this);
  tie.xAxis->setTitleText(QString::fromStdString(model.xAxis.name));
  tie.xAxis->setRange(model.xAxis.min, model.xAxis.max);

  // Y axis (categories)
  auto yAxis = new QtCharts::QCategoryAxis(this);
  const auto &categories = tie.model.yAxis.values;

  yAxis->setTitleText(QString::fromStdString(model.yAxis.name));
  // Just to be safe
  if (!categories.empty()) {
    // Give slight padding before/after the min/max values
    yAxis->setMin(static_cast<double>(categories.front().id) - 0.1);
    yAxis->setMax(static_cast<double>(categories.back().id) + 0.1);
  }
  for (const auto &category : categories)
    yAxis->append(QString::fromStdString(category.name), category.id);

  // Center the label within the range
  // since we use the ID as the end
  // of the range, rather than the center
  yAxis->setLabelsPosition(QtCharts::QCategoryAxis::AxisLabelsPositionOnValue);

  tie.yAxis = yAxis;
  return tie;
}

ChartManager::ChartManager(QWidget *parent) : QObject(parent) {
}

void ChartManager::reset() {
  dropdownElements.clear();
  events.clear();
  undoEvents.clear();

  // Clear the child widgets first
  // since they may be holding on to series
  for (auto chartWidget : chartWidgets) {
    chartWidget->reset();
  }

  for (auto &iterator : series) {
    auto &value = iterator.second;
    if (std::holds_alternative<XYSeriesTie>(value)) {
      auto qtSeries = std::get<XYSeriesTie>(value).qtSeries;
      qtSeries->setParent(nullptr);
      qtSeries->deleteLater();
    } else if (std::holds_alternative<CategoryValueTie>(value)) {
      auto qtSeries = std::get<CategoryValueTie>(value).qtSeries;
      qtSeries->setParent(nullptr);
      qtSeries->deleteLater();
    }
    // No need to handle SeriesCollection since it has no pointers
  }

  series.clear();
}

void ChartManager::setChildrenSeries(const std::vector<DropdownValue> &values) {
  for (auto chartWidget : chartWidgets) {
    chartWidget->setSeries(values);
  }
}

std::vector<unsigned int> ChartManager::inCollections(unsigned int id) {
  std::vector<unsigned int> collections;

  for (const auto &[key, value] : series) {
    // We're only concerned about collections
    // so we can skip everything else
    if (!std::holds_alternative<SeriesCollectionTie>(value))
      continue;

    const auto &tieModel = std::get<SeriesCollectionTie>(value).model;
    if (std::find(tieModel.series.begin(), tieModel.series.end(), id) != tieModel.series.end())
      collections.emplace_back(tieModel.id);
  }

  return collections;
}

void ChartManager::clearSeries(const ChartWidget *except, unsigned int id) {
  for (auto widget : chartWidgets) {
    if (widget == except)
      continue;

    if (widget->getCurrentSeries() == id)
      widget->clearSelected();
  }
}

void ChartManager::timeAdvanced(parser::nanoseconds time) {
  auto handleEvent = [time, this](auto &&e) {
    // Strip off qualifiers, etc
    // so T holds just the type
    // so we can more easily match it
    using T = std::decay_t<decltype(e)>;

    if (e.time > time)
      return false;

    if constexpr (std::is_same_v<T, parser::XYSeriesAddValue>) {
      const auto &s = std::get<XYSeriesTie>(series[e.seriesId]);
      if (s.model.xAxis.boundMode == parser::ValueAxis::BoundMode::HighestValue) {
        updateRange(s.xAxis, e.point.x);
      }
      if (s.model.yAxis.boundMode == parser::ValueAxis::BoundMode::HighestValue) {
        updateRange(s.yAxis, e.point.y);
      }
      updateCollectionRanges(e.seriesId, e.point.x, e.point.y);
      s.qtSeries->append(e.point.x, e.point.y);
      undoEvents.emplace_back(undo::XYSeriesAddValue{e});
      events.pop_front();
      return true;
    }

    if constexpr (std::is_same_v<T, parser::XYSeriesAddValues>) {
      const auto &s = std::get<XYSeriesTie>(series[e.seriesId]);

      for (const auto &point : e.points) {
        if (s.model.xAxis.boundMode == parser::ValueAxis::BoundMode::HighestValue) {
          updateRange(s.xAxis, point.x);
        }
        if (s.model.yAxis.boundMode == parser::ValueAxis::BoundMode::HighestValue) {
          updateRange(s.yAxis, point.y);
        }

        updateCollectionRanges(e.seriesId, point.x, point.y);
        s.qtSeries->append(point.x, point.y);
      }

      undoEvents.emplace_back(undo::XYSeriesAddValues{e});
      events.pop_front();
      return true;
    }

    if constexpr (std::is_same_v<T, parser::XYSeriesClear>) {
      const auto &s = std::get<XYSeriesTie>(series[e.seriesId]);
      auto points = s.qtSeries->pointsVector();
      s.qtSeries->clear();

      undoEvents.emplace_back(undo::XYSeriesClear{e, std::move(points)});
      events.pop_front();
      return true;
    }

    if constexpr (std::is_same_v<T, parser::CategorySeriesAddValue>) {
      // Not const since we change the lastUpdatedTime
      auto &s = std::get<CategoryValueTie>(series[e.seriesId]);
      if (s.model.xAxis.boundMode == parser::ValueAxis::BoundMode::HighestValue) {
        updateRange(s.xAxis, e.value);
      }

      // Y axis on category charts is a fixed size

      s.lastUpdatedTime = time;
      s.qtSeries->append(e.value, e.category);
      updateCollectionRanges(e.seriesId, e.value, e.category);
      undoEvents.emplace_back(undo::CategorySeriesAddValue{e});
      events.pop_front();
      return true;
    }

    // Unhandled event type
    return false;
  };

  while (!events.empty() && std::visit(handleEvent, events.front())) {
    // Intentionally Blank
  }

  // Add "Fake Events" to keep the category value series moving
  // TODO: Maybe move to parse time
  for (auto &[key, s] : series) {
    // Only CategoryValueSeries may have auto-appended values
    if (!std::holds_alternative<CategoryValueTie>(s))
      continue;

    auto &value = std::get<CategoryValueTie>(s);
    if (!value.model.autoUpdate)
      continue;

    const auto &points = value.qtSeries->pointsVector();
    if (points.empty())
      continue;

    if (time - value.lastUpdatedTime < value.model.autoUpdateInterval)
      continue;

    const auto lastValue = points.last();
    parser::CategorySeriesAddValue fakeEvent;
    fakeEvent.time = time;
    fakeEvent.value = lastValue.x() + value.model.autoUpdateIncrement;
    fakeEvent.category = static_cast<unsigned int>(lastValue.y());
    fakeEvent.seriesId = key;

    if (value.model.xAxis.boundMode == parser::ValueAxis::BoundMode::HighestValue) {
      updateRange(value.xAxis, fakeEvent.value);
    }

    // Y axis on category charts is a fixed size

    value.qtSeries->append(fakeEvent.value, fakeEvent.category);
    updateCollectionRanges(fakeEvent.seriesId, fakeEvent.value, fakeEvent.category);
    undoEvents.emplace_back(undo::CategorySeriesAddValue{fakeEvent});

    value.lastUpdatedTime = time;
  }
}

void ChartManager::timeRewound(parser::nanoseconds time) {
  auto handleUndoEvent = [time, this](auto &&e) -> bool {
    // Strip off qualifiers, etc
    // so T holds just the type
    // so we can more easily match it
    using T = std::decay_t<decltype(e)>;

    // All events have a time
    // Make sure we don't handle one
    // Before it was originally applied
    if (time > e.event.time)
      return false;

    if constexpr (std::is_same_v<T, undo::XYSeriesAddValue>) {
      auto &s = std::get<XYSeriesTie>(series[e.event.seriesId]);

      s.qtSeries->remove(s.qtSeries->count() - 1);

      events.emplace_front(e.event);
      return true;
    }

    if constexpr (std::is_same_v<T, undo::XYSeriesAddValues>) {
      auto &s = std::get<XYSeriesTie>(series[e.event.seriesId]);
      const auto count = e.event.points.size();

      s.qtSeries->removePoints(s.qtSeries->count() - count, count);

      events.emplace_front(e.event);
      return true;
    }

    if constexpr (std::is_same_v<T, undo::XYSeriesClear>) {
      auto &s = std::get<XYSeriesTie>(series[e.event.seriesId]);
      s.qtSeries->replace(e.points);

      events.emplace_front(e.event);
      return true;
    }

    if constexpr (std::is_same_v<T, undo::CategorySeriesAddValue>) {
      auto &s = std::get<CategoryValueTie>(series[e.event.seriesId]);

      s.qtSeries->remove(s.qtSeries->count() - 1);

      events.emplace_front(e.event);
      return true;
    }

    return false;
  };

  while (!undoEvents.empty() && std::visit(handleUndoEvent, undoEvents.back())) {
    undoEvents.pop_back();
  }
}

void ChartManager::spawnWidget(QMainWindow *parent) {
  auto newWidget = new ChartWidget{parent, *this, dropdownElements};
  parent->addDockWidget(Qt::RightDockWidgetArea, newWidget);

  chartWidgets.emplace_back(newWidget);
}

void ChartManager::clearWidgets() {
  for (auto widget : chartWidgets) {
    widget->close();
    widget->deleteLater();
  }

  chartWidgets.clear();
}

void ChartManager::widgetClosed(ChartWidget *widget) {
  // Remove the closed widget from our list
  chartWidgets.erase(std::remove(chartWidgets.begin(), chartWidgets.end(), widget), chartWidgets.end());
}

void ChartManager::updateCollectionRanges(uint32_t seriesId, double x, double y) {
  for (auto &iterator : series) {
    // Only update collections
    if (!std::holds_alternative<ChartManager::SeriesCollectionTie>(iterator.second))
      continue;

    auto &collection = std::get<ChartManager::SeriesCollectionTie>(iterator.second);

    // Only update the collection's ranges if it actually contains the series
    if (std::find(collection.model.series.begin(), collection.model.series.end(), seriesId) !=
        collection.model.series.end()) {
      if (collection.model.xAxis.boundMode == parser::ValueAxis::BoundMode::HighestValue)
        updateRange(collection.xAxis, x);
      if (collection.model.yAxis.boundMode == parser::ValueAxis::BoundMode::HighestValue)
        updateRange(collection.yAxis, y);
    }
  }
}

ChartManager::TieVariant &ChartManager::getSeries(uint32_t seriesId) {
  const auto &seriesIterator = series.find(seriesId);
  if (seriesIterator == series.end()) {
    QMessageBox::critical(qobject_cast<QMainWindow *>(parent()), "Series not found",
                          "The selected series was not found");
    std::abort();
  }

  return seriesIterator->second;
}

void ChartManager::seriesSelected(const ChartWidget *widget, unsigned int selected) {
  if (selected == PlaceholderId)
    return;

  clearSeries(widget, selected);

  // If a collection was selected, clear the child series
  const auto &tie = series[selected];
  if (std::holds_alternative<SeriesCollectionTie>(tie)) {
    const auto &tieValue = std::get<SeriesCollectionTie>(tie);

    for (const auto seriesId : tieValue.model.series)
      clearSeries(widget, seriesId);
  } else if (std::holds_alternative<XYSeriesTie>(tie)) {
    // Clear all the collections this series belongs to as well
    // Only XYSeries may belong to collections
    const auto &tieModel = std::get<XYSeriesTie>(tie).model;
    const auto collections = inCollections(tieModel.id);
    for (const auto id : collections)
      clearSeries(widget, id);
  }
}

void ChartManager::timeChanged(parser::nanoseconds time, parser::nanoseconds increment) {
  if (increment > 0LL)
    timeAdvanced(time);
  else
    timeRewound(time);
}

void ChartManager::enqueueEvents(const std::vector<parser::ChartEvent> &e) {
  events.insert(events.end(), e.begin(), e.end());
}
void ChartManager::addSeries(const std::vector<parser::XYSeries> &xySeries,
                             const std::vector<parser::SeriesCollection> &collections,
                             const std::vector<parser::CategoryValueSeries> &categoryValueSeries) {

  for (const auto &collection : collections) {
    series.emplace(collection.id, makeTie(collection));
    dropdownElements.emplace_back(
        DropdownValue{QString::fromStdString(collection.name), SeriesType::Collection, collection.id});
  }

  for (const auto &xy : xySeries) {
    series.emplace(xy.id, makeTie(xy));

    if (xy.visible) {
      dropdownElements.emplace_back(DropdownValue{QString::fromStdString(xy.name), SeriesType::XY, xy.id});
    }
  }

  for (const auto &category : categoryValueSeries) {
    series.emplace(category.id, makeTie(category));

    if (category.visible) {
      dropdownElements.emplace_back(
          DropdownValue{QString::fromStdString(category.name), SeriesType::CategoryValue, category.id});
    }
  }

  setChildrenSeries(dropdownElements);
}
void ChartManager::setSortOrder(SettingsManager::ChartDropdownSortOrder value) {
  sortOrder = value;
  for (const auto widget : chartWidgets) {
    widget->setSortOrder(sortOrder);
  }
}

} // namespace netsimulyzer
