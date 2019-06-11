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

#include "DecorationGroup.h"
#include <osg/BlendFunc>
#include <osg/Material>
#include <osgDB/ReadFile>

namespace visualization {

DecorationGroup::DecorationGroup(const Decoration &config) {
  osg::ref_ptr<osg::Node> model = osgDB::readRefNodeFile(config.model);

  if (config.opacity < 1) {
    auto stateSet = model->getOrCreateStateSet();

    auto attr = stateSet->getAttribute(osg::StateAttribute::MATERIAL);
    osg::ref_ptr<osg::Material> material;

    if (attr)
      material = dynamic_cast<osg::Material *>(attr);
    else
      material = new osg::Material;

    material->setColorMode(osg::Material::ColorMode::SPECULAR);
    material->setAlpha(osg::Material::FRONT_AND_BACK, static_cast<float>(config.opacity));
    stateSet->setAttributeAndModes(material, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

    stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    model->setStateSet(stateSet);

    model->getOrCreateStateSet()->setAttributeAndModes(
        new osg::BlendFunc(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA));
  }

  geode = new osg::Geode;
  geode->addChild(model);

  scale = new osg::MatrixTransform(osg::Matrix::scale(osg::Vec3d(config.scale, config.scale, config.scale)));
  scale->addChild(geode);

  position = new osg::PositionAttitudeTransform();
  position->setPosition(config.position);
  position->addChild(scale);
  
  addChild(position);
}

} // namespace visualization
