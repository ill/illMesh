#ifndef ILL_CONVERTER_MESH_MERGER_H_
#define ILL_CONVERTER_MESH_MERGER_H_

#include <string>
#include <vector>
#include "illEngine/Util/Geometry/MeshData.h"

class MeshMerger {
public:
    std::vector<std::string> m_paths;
    std::string m_exportPath;

    void merge();
};

#endif