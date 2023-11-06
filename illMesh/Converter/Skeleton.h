#ifndef ILL_CONVERTER_SKELETON_H_
#define ILL_CONVERTER_SKELETON_H_

#include <glm/glm.hpp>
#include <set>
#include <string>
#include <assimp/scene.h>
#include "illEngine/Util/serial/Array.h"

class AnimSet;

class Skeleton {
public:
    void load(const char * path, const aiScene * scene);
    void save(const char * path, const AnimSet * animset) const;

    void import(const aiScene * scene, const AnimSet * animset);

    const aiScene * m_scene;

    struct Bone {
        glm::mat4 m_relativeTransform;  //the transform relative to the parent in the bind pose
        glm::mat4 m_offsetTransform;    //the inverse of the full transform in the bind pose
    };

    Array<Bone> m_bones;
};

#endif