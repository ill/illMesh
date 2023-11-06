#include <stdexcept>

#include <assimp/postprocess.h>

#include "Importer.h"
#include "Animation.h"
#include "Mesh.h"
#include "Skeleton.h"
#include "AnimSet.h"

#include "illEngine/Util/util.h"
#include "illEngine/FileSystem/FileSystem.h"
#include "illEngine/Logging/logging.h"

unsigned int MESH_FLAGS = 
    aiProcess_CalcTangentSpace |
    aiProcess_JoinIdenticalVertices |           //This seems to make some obscure bones dissapear in MD5, TODO: add a way to either force disable or force enable this
    aiProcess_Triangulate |
    aiProcess_GenSmoothNormals |
    //aiProcess_PreTransformVertices |          //This removes bones and animations, so we don't want this
    aiProcess_LimitBoneWeights |
    aiProcess_ValidateDataStructure |
    aiProcess_ImproveCacheLocality |
    aiProcess_RemoveRedundantMaterials |
    aiProcess_SortByPType |
    aiProcess_FindDegenerates |        
    aiProcess_FindInvalidData |
    aiProcess_GenUVCoords/* |
    aiProcess_OptimizeGraph*/;

unsigned int SKEL_FLAGS = 
    aiProcess_LimitBoneWeights |
    aiProcess_ValidateDataStructure |
    aiProcess_FindInvalidData/* |
    aiProcess_OptimizeGraph*/;

unsigned int ANIM_FLAGS = 
    aiProcess_LimitBoneWeights |
    aiProcess_ValidateDataStructure |
    aiProcess_FindInvalidData/* |
    aiProcess_OptimizeGraph*/;

void Importer::computeBones() {
    for(auto iter = m_importFiles.begin(); iter != m_importFiles.end(); iter++) {
        unsigned int processFlags = SKEL_FLAGS;

        if(iter->m_meshOutFile) {
            processFlags |= MESH_FLAGS;
        }

        if(iter->m_animOutFile) {
            processFlags |= ANIM_FLAGS;
        }

        iter->m_scene = iter->m_importer.ReadFile(iter->m_importFile, processFlags);

        /**
        Line 387 in JoinVerticesProcess for assimp mentions something about possibly removing bones
        which are attachment points for weapons in Md5s.  This is happening with SOUL_ATTACHER in 
        the doom 3 mppplayer.md5mesh and the run.md5anim files.
        */

        // If the import failed, report it
        if(!iter->m_scene) {
            LOG_FATAL_ERROR("The file %s failed to import", iter->m_importFile);
        }

        m_animSet.findBones(iter->m_scene);
    }

    //m_animSet.computeHeirarchies();
}

void Importer::doImports() {
    for(auto iter = m_importFiles.begin(); iter != m_importFiles.end(); iter++) {
        //first do the skeleton
        if(!iter->m_skeletonOut) {
            iter->m_skeletonOut = new Skeleton();
            iter->m_skeletonOut->import(iter->m_scene, &m_animSet);
        }
    }


    for(auto iter = m_importFiles.begin(); iter != m_importFiles.end(); iter++) {
        //the animations
        for(unsigned int animation = 0; animation < iter->m_scene->mNumAnimations; animation++) {
            iter->m_animationOut.push_back(new Animation());
            iter->m_animationOut.back()->import(iter->m_scene->mAnimations[animation], m_importFiles.at(m_mainSkeletonImport).m_skeletonOut, &m_animSet);
        }

        //the meshes
        for(unsigned int mesh = 0; mesh < iter->m_scene->mNumMeshes; mesh++) {
            iter->m_meshOut.push_back(new Mesh());
            iter->m_meshOut.back()->import(iter->m_scene->mMeshes[mesh], &m_animSet);
        }
    }
}

std::string Importer::computeAnimationFileName(Animation * animation, const char * path) {
    std::string filename(path);
    
    //figure out animation filename
    size_t extensionPos = filename.rfind('.');
        
    //strip extension
    std::string animFileName = extensionPos == filename.npos
        ? filename
        : filename.substr(0, extensionPos);
        
    //append animation name
    char * nameStr = NULL;

    if(animation->m_animation->mName.length > 0) {
        nameStr = (char *) animation->m_animation->mName.data;
    }

    //remove invalid chars since this will be the filename
    if(nameStr != NULL) {        
        char * curPos = nameStr;

        while(*curPos != '\0') {
            //for now just removes slashes and replaces them with underscores
            if(*curPos == '\\' || *curPos == '/') {
                *curPos = '_';
            }

            curPos++;
        }
    }

    std::string name(nameStr == NULL ? "" : nameStr);

    int dupNum = 0;

    //check if the mesh name has been used already, if so generate a new one with a number appended
    while(m_usedAnimationNames.find(name) != m_usedAnimationNames.end()) {
        //not the most efficient way to do this I guess, but I just want this to work already...
        name = formatString("%s%d", nameStr == NULL ? "" : nameStr, dupNum);

        dupNum++;
    }

    m_usedAnimationNames.insert(name);
        
    animFileName += name;
        
    //add extension back on
    if(extensionPos != filename.npos) {
        animFileName += filename.substr(extensionPos, filename.npos);
    }

    return animFileName;
}

std::string Importer::computeMeshFileName(Mesh * mesh, const aiScene * scene, const char * path) {
    std::string filename(path);
    
    //figure out mesh filename
    size_t extensionPos = filename.rfind('.');
        
    //strip extension
    std::string meshFileName = extensionPos == filename.npos
        ? filename
        : filename.substr(0, extensionPos);
        
    //append mesh name or material name
    char * nameStr = NULL;

    if(mesh->m_mesh->mName.length > 0) {
        nameStr = (char *) mesh->m_mesh->mName.data;    //typecasting away the const, I swear I won't do anything bad
    }
    else {
        scene->mMaterials[mesh->m_mesh->mMaterialIndex]->Get(AI_MATKEY_NAME, nameStr);
    }

    //remove invalid chars since this will be the filename
    if(nameStr != NULL) {        
        char * curPos = nameStr;

        while(*curPos != '\0') {
            //for now just removes slashes and replaces them with underscores
            if(*curPos == '\\' || *curPos == '/') {
                *curPos = '_';
            }

            curPos++;
        }
    }

    std::string name(nameStr == NULL ? "" : nameStr);

    int dupNum = 0;

    //check if the mesh name has been used already, if so generate a new one with a number appended
    while(m_usedMeshNames.find(name) != m_usedMeshNames.end()) {
        //not the most efficient way to do this I guess, but I just want this to work already...
        name = formatString("%s%d", nameStr == NULL ? "" : nameStr, dupNum);

        dupNum++;
    }

    m_usedMeshNames.insert(name);

    meshFileName += name;

    //add extension back on
    if(extensionPos != filename.npos) {
        meshFileName += filename.substr(extensionPos, filename.npos);
    }

    return meshFileName;
}

std::string Importer::computeSkeletonFileName(Skeleton * skeleton, const char * path) {
    std::string filename(path);
    
    //figure out skeleton filename
    size_t extensionPos = filename.rfind('.');
        
    //strip extension
    std::string skeletonFileName = extensionPos == filename.npos
        ? filename
        : filename.substr(0, extensionPos);
        
    int dupNum = 0;

    //check if the skeleton name has been used already, if so generate a new one with a number appended
    while(m_usedSkeletonNames.find(skeletonFileName) != m_usedSkeletonNames.end()) {
        //not the most efficient way to do this I guess, but I just want this to work already...
        skeletonFileName = formatString("%s%d", skeletonFileName, dupNum);

        dupNum++;
    }

    m_usedMeshNames.insert(skeletonFileName);
    
    //add extension back on
    if(extensionPos != filename.npos) {
        skeletonFileName += filename.substr(extensionPos, filename.npos);
    }

    return skeletonFileName;
}