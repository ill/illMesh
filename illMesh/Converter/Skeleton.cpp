#include <glm/gtc/matrix_transform.hpp>

#include "Skeleton.h"
#include "AnimSet.h"

#include "illEngine/FileSystem/FileSystem.h"
#include "illEngine/FileSystem/File.h"

#include "illEngine/Logging/logging.h"

#include "illEngine/Util/Geometry/Transform.h"

const uint64_t SKEL_MAGIC = 0x494C4C534B454C30;		//ILLSKEL0 in 64 bit big endian

void Skeleton::load(const char * path, const aiScene * scene) {
    m_scene = scene;

    illFileSystem::File * openFile = illFileSystem::fileSystem->openRead(path);
	
	//read magic string
    {
		uint64_t magic;
        openFile->readB64(magic);

        if(magic != SKEL_MAGIC) {
            LOG_FATAL_ERROR("Not a valid ILLSKEL0 file.");      //TODO: make this not fatal somehow, I guess all meshes would be in their rest pose if they're supposed to have an associated skeleton
        }
    }
	
    //read number of bones
	{
		uint16_t numBones;
		openFile->readL16(numBones);
		m_bones.resize(numBones);
	}

    //read the bind poses and offsets
    for(unsigned bone = 0; bone < m_bones.size(); bone++) {
        //bind
        for(unsigned int matCol = 0; matCol < 4; matCol++) {
            for(unsigned int matRow = 0; matRow < 4; matRow++) {
                openFile->readLF(m_bones[bone].m_relativeTransform[matCol][matRow]);
            }            
        }

        //offset
        for(unsigned int matCol = 0; matCol < 4; matCol++) {
            for(unsigned int matRow = 0; matRow < 4; matRow++) {
                openFile->readLF(m_bones[bone].m_offsetTransform[matCol][matRow]);
            }            
        }
    }
    
    //no need to read the heirarchy, that gets computed separately in the animset stage, a bit weird...
	//TODO: never mind, I need to import the heirarchy later	

	delete openFile;
}

//a nice lazy way to get the full transform, very inefficient, but I don't feel like computing a heirarchy tree from the parent list
glm::mat4 getFullTransform(uint16_t boneIndex, const std::map<uint16_t, uint16_t>& boneParentMap, const Skeleton::Bone * bones) {
    //get parent index
    auto parentIndIter = boneParentMap.find(boneIndex);

    //if root
    if(parentIndIter == boneParentMap.end() || boneIndex == parentIndIter->second) {
        return bones[boneIndex].m_relativeTransform;
    }
    else {
        return getFullTransform(parentIndIter->second, boneParentMap, bones) * bones[boneIndex].m_relativeTransform;
    }
}

void Skeleton::import(const aiScene * scene, const AnimSet * animset) {
    m_scene = scene;
    const AnimSet::SceneBoneData& sceneBoneData = animset->m_sceneBoneData.at(scene);

    //allocate space for the bones
    uint16_t numBones = (uint16_t) animset->m_boneNameMap.size();
    m_bones.resize(numBones);

    //get their bind pose transforms
    for(uint16_t bone = 0; bone < numBones; bone++) {
        auto boneNodeIter = sceneBoneData.m_boneIndexNodes.find(bone);

        if(boneNodeIter == sceneBoneData.m_boneIndexNodes.end()) {
            m_bones[bone].m_relativeTransform = glm::mat4();
        }
        else {
            m_bones[bone].m_relativeTransform = glm::mat4(
                glm::vec4(boneNodeIter->second->mTransformation[0][0], 
                    boneNodeIter->second->mTransformation[1][0], 
                    boneNodeIter->second->mTransformation[2][0], 
                    boneNodeIter->second->mTransformation[3][0]),

                glm::vec4(boneNodeIter->second->mTransformation[0][1], 
                    boneNodeIter->second->mTransformation[1][1], 
                    boneNodeIter->second->mTransformation[2][1], 
                    boneNodeIter->second->mTransformation[3][1]),

                glm::vec4(boneNodeIter->second->mTransformation[0][2], 
                    boneNodeIter->second->mTransformation[1][2], 
                    boneNodeIter->second->mTransformation[2][2], 
                    boneNodeIter->second->mTransformation[3][2]),

                glm::vec4(boneNodeIter->second->mTransformation[0][3], 
                    boneNodeIter->second->mTransformation[1][3], 
                    boneNodeIter->second->mTransformation[2][3], 
                    boneNodeIter->second->mTransformation[3][3]));
        }

        //dirty hack to try rotating the skeleton 90 degrees
        /*if(animset->m_boneNameMap.at("origin") == bone) {
            m_bones[bone].m_relativeTransform = glm::rotate(m_bones[bone].m_relativeTransform, -90.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        }*/
    }

    //get their bone offsets, this is the inverse of the full bind pose
    for(uint16_t bone = 0; bone < numBones; bone++) {
        m_bones[bone].m_offsetTransform = glm::inverse(getFullTransform(bone, animset->m_boneParentIndeces, &m_bones[0]));
    }
}

void Skeleton::save(const char * path, const AnimSet * animset) const {
    illFileSystem::File * openFile = illFileSystem::fileSystem->openWrite(path);
	
	//write magic string
    openFile->writeB64(SKEL_MAGIC);
    	
    //write number of bones
    openFile->writeL16(m_bones.size());

    for(uint16_t bone = 0; bone < (uint16_t) m_bones.size(); bone++) {
        //bind
        for(unsigned int matCol = 0; matCol < 4; matCol++) {
            for(unsigned int matRow = 0; matRow < 4; matRow++) {
                openFile->writeLF(m_bones[bone].m_relativeTransform[matCol][matRow]);
            }            
        }

        //offset
        for(unsigned int matCol = 0; matCol < 4; matCol++) {
            for(unsigned int matRow = 0; matRow < 4; matRow++) {
                openFile->writeLF(m_bones[bone].m_offsetTransform[matCol][matRow]);
            }            
        }
    }
    
    //write the heirarchy
    for(uint16_t bone = 0; bone < (uint16_t) m_bones.size(); bone++) {
        //get parent index
        auto parentIndIter = animset->m_boneParentIndeces.find(bone);

        //LOG_DEBUG("Bone number %u", bone);

        //if root
        if(parentIndIter == animset->m_boneParentIndeces.end() 
            || bone == parentIndIter->second) {
            //LOG_DEBUG("Root");
            openFile->writeL16(bone);
        }
        else {
            //LOG_DEBUG("Parent %u", parentIndIter->second);
            openFile->writeL16(parentIndIter->second);
        }
        
    }
		
	delete openFile;
}