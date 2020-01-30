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

#pragma once

#include "../../event/model.h"
#include "../../parser/model.h"
#include <deque>
#include <osg/Group>
#include <osg/MatrixTransform>
#include <osg/NodeCallback>
#include <osg/PositionAttitudeTransform>

namespace visualization {

class DecorationGroupEventCallback : public osg::NodeCallback {
public:
  void operator()(osg::Node *node, osg::NodeVisitor *nv) override;
};

/**
 * Non-operative model, in the scene only for show
 */
class DecorationGroup : public osg::Group {
  // Let the callback control position, scale, etc...
  friend DecorationGroupEventCallback;

  /**
   * Event queue that controls the Decoration
   */
  std::deque<DecorationEvent> events;

  /**
   * Relative position of the Decoration
   */
  osg::ref_ptr<osg::PositionAttitudeTransform> position;

  /**
   * Scale of the Decoration
   */
  osg::ref_ptr<osg::MatrixTransform> scale;

  /**
   * The overall orientation of the Decoration
   */
  osg::ref_ptr<osg::MatrixTransform> orientation;

  /**
   * Actual geometry of the Decoration
   */
  osg::ref_ptr<osg::Geode> geode;

public:
  /**
   * Setup the position, scale, geode from the configuration
   *
   * @param config
   * The configuration from the XML source to build the Decoration
   */
  explicit DecorationGroup(const Decoration &config);

  /**
   * Add an event to the queue.
   *
   * Events should be added in order.
   *
   * If an event in the past is added it will be executed during the
   * next update traversal.
   *
   * An event happening after the next event in the queue,
   * then all events will be held until that event is executed
   *
   * @param event
   * The event to add to the queue.
   */
  void enqueueEvent(const DecorationEvent &event);
};

} // namespace visualization
