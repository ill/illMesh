#ifndef ILL_CONVERTER_ANIMATION_H_
#define ILL_CONVERTER_ANIMATION_H_

#include <assimp/scene.h>
#include <unordered_map>
#include <map>

#include "illEngine/Util/Geometry/Transform.h"

class AnimSet;
class Skeleton;

class Animation {
public:
    struct AnimData {        		
        std::map<float, glm::vec3> m_positionKeys;
        std::map<float, glm::quat> m_rotationKeys;
        std::map<float, glm::vec3> m_scalingKeys;
    };

    typedef std::unordered_map<uint16_t, AnimData> BoneAnimationMap;

    void save(const char * path);
    void import(const aiAnimation* animation, const Skeleton * skeleton, const AnimSet * animset);

    const aiAnimation* m_animation;

    float m_duration;
    BoneAnimationMap m_boneAnimation;
};

#endif