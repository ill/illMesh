#ifndef ILL_CONVERTER_ANIM_SET_H_
#define ILL_CONVERTER_ANIM_SET_H_

#include <glm/glm.hpp>
#include <stdint.h>
#include <string>
#include <map>

#include <assimp/scene.h>

class AnimSet {
public:
    AnimSet()
        : m_creating(true)
    {}

    struct SceneBoneData {
        std::map<uint16_t, const aiNode*> m_boneIndexNodes;                 //bone index to node map
    };

    std::map<uint16_t, uint16_t> m_boneParentIndeces;                 //bone index to parent index map

    void load(const char * path);

    void findBones(const aiScene * scene);
    //void computeHeirarchies();

    void save(const char * path) const;
    
    bool m_creating;

    std::map<std::string, uint16_t> m_boneNameMap;       //bone name to bone index map
    
    std::map<const aiScene *, SceneBoneData> m_sceneBoneData;
};

#endif