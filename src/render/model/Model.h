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

#include "glm/glm.hpp"
#include <array>
#include <string>
#include <vector>

namespace visualization {

class Model {
public:
  struct ModelLoadInfo {
    unsigned long id;
    glm::vec3 min;
    glm::vec3 max;
  };

  struct ModelBounds {
    glm::vec3 min;
    glm::vec3 max;
  };

private:
  const unsigned long modelId;
  const glm::vec3 min;
  const glm::vec3 max;

  glm::vec3 position{0.0f};
  float targetHeightScale = 1.0f;
  float scale = 1.0f;
  std::array<float, 3> rotate{0.0f};

  /**
   * Final model matrix built from the
   * `position` `rotation` `targetHeightScale` & `scale`
   */
  glm::mat4 modelMatrix{1.0f};

public:
  Model(const ModelLoadInfo &info);
  Model(unsigned long modelId, const glm::vec3 &min, const glm::vec3 &max);

  void setPosition(const glm::vec3 &value);
  [[nodiscard]] const glm::vec3 &getPosition() const;

  void setTargetHeightScale(float value);
  [[nodiscard]] float getTargetHeightScale() const;

  void setScale(float value);
  [[nodiscard]] float getScale() const;

  void setRotate(float x = 0.0f, float y = 0.0f, float z = 0.0f);

  [[nodiscard]] unsigned long getModelId() const;

  [[nodiscard]] const glm::mat4 &getModelMatrix() const;

  [[nodiscard]] ModelBounds getBounds() const;

  void rebuildModelMatrix();
};

} // namespace visualization
