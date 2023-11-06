#ifndef ILL_CONVERTER_MESH_H_
#define ILL_CONVERTER_MESH_H_

#include <stdint.h>
#include <assimp/scene.h>
#include <set>
#include <map>
#include <string>

class AnimSet;

class Mesh {
public:
    Mesh()
        : m_mesh(NULL),
        m_boneWeights(NULL)
    {}

    ~Mesh() {
        delete[] m_boneWeights;
    }

    void save(const char * path) const;
    void import(const aiMesh * mesh, const AnimSet * animset);

    const aiMesh* m_mesh;

    typedef std::map<uint16_t, float> BoneMap;
    BoneMap * m_boneWeights;   //map of bone index to weight for each vertex, the array is the size of verteces in the mesh
};

#endif