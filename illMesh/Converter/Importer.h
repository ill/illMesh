#ifndef ILL_IMPORTER_H_
#define ILL_IMPORTER_H_

#include <vector>
#include <string>
#include <map>
#include <set>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include "AnimSet.h"Impo

class Animation;
class Mesh;
class Skeleton;

class Importer {
public:
    struct Import {
        Import()
            : m_importFile(NULL),

            m_skelInFile(NULL),

            m_meshOutFile(NULL),
            m_animOutFile(NULL),
            m_skelOutFile(NULL),

            m_mergeMesh(false),

            m_scene(NULL),

            m_skeletonOut(NULL)
        {}

        const char * m_importFile;

        const char * m_skelInFile;

        const char * m_meshOutFile;
        const char * m_animOutFile;
        const char * m_skelOutFile;

        bool m_mergeMesh;

        Assimp::Importer m_importer;
        const aiScene * m_scene;

        std::vector<Animation *> m_animationOut;
        std::vector<Mesh *> m_meshOut;
        Skeleton * m_skeletonOut;
    };

    void computeBones();
    void doImports();

    std::string computeAnimationFileName(Animation * animation, const char * path);
    std::string computeMeshFileName(Mesh * mesh, const aiScene * scene, const char * path);
    std::string computeSkeletonFileName(Skeleton * skeleton, const char * path);
    
    std::set<std::string> m_usedAnimationNames;
    std::set<std::string> m_usedMeshNames;
    std::set<std::string> m_usedSkeletonNames;

    std::vector<Import> m_importFiles;
    size_t m_mainSkeletonImport;    //which file import's skeleton is the one used for all animations

    AnimSet m_animSet;
};

#endif