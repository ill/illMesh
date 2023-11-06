#include "illEngine/FileSystem/FileSystem.h"
#include "illEngine/FileSystem/File.h"
#include "illEngine/Util/serial/Array.h"
#include "illEngine/Util/util.h"

#include "illEngine/Logging/logging.h"

#include "AnimSet.h"

const uint64_t ANIMSET_MAGIC = 0x494C414E53455430;		//ILANSET0 in 64 bit big endian

void AnimSet::load(const char * path) {
    illFileSystem::File * openFile = illFileSystem::fileSystem->openRead(path);

    //read magic string
    {
		uint64_t magic;
        openFile->readB64(magic);

        if(magic != ANIMSET_MAGIC) {
            LOG_FATAL_ERROR("Not a valid ILANSET0 file.");      //TODO: make this not fatal somehow
        }
    }

    //read number of bones
	uint16_t numBones;
	openFile->readL16(numBones);

    //read bone names
	{
		Array<char> strBuffer;

		for(unsigned int bone = 0; bone < numBones; bone++) {
			uint16_t stringBufferLength = openFile->readStringBufferLength();			
			strBuffer.reserve(stringBufferLength);
			openFile->readString(&strBuffer[0], stringBufferLength);

			if(m_boneNameMap.find(&strBuffer[0]) != m_boneNameMap.end()) {
				LOG_FATAL_ERROR("Animset has duplicate bone name %s.  This will cause problems when animating.", &strBuffer[0]);
			}
			else {
				m_boneNameMap[&strBuffer[0]] = bone;
			}
		}
	}

    m_creating = false;
    
    delete openFile;
}

void AnimSet::findBones(const aiScene * scene) {
    SceneBoneData& sceneBoneData = m_sceneBoneData[scene];

    //it would be possible to abort recursing on bones already found,
    //this may make other scenes not get all of the bones in
    //look up all meshes and their bones

    //LOG_DEBUG("Beginning to add bones from scene with %u meshes\n", scene->mNumMeshes);

    for(unsigned int mesh = 0; mesh < scene->mNumMeshes; mesh++) {
        const aiMesh* currMesh = scene->mMeshes[mesh];

        //LOG_DEBUG("Processing mesh %d with name %s with %d bones\n", mesh, currMesh->mName.data, currMesh->mNumBones);

        for(unsigned int bone = 0; bone < currMesh->mNumBones; bone++) {
            const aiBone* currBone = currMesh->mBones[bone];
               
            //LOG_DEBUG("Processing bone %d with name %s", bone, currBone->mName.data);

            uint16_t currentBoneIndex;

            //look up bone index
            {
                auto iter = m_boneNameMap.find(currBone->mName.data);

                if(iter == m_boneNameMap.end()) {
                    if(!m_creating) {
                        LOG_FATAL_ERROR("Imported animset doesn't have bone with name %s. Just generate a new animset and start everything from scratch.", currBone->mName.data);
                    }

                    currentBoneIndex = m_boneNameMap.size();
                    m_boneNameMap[currBone->mName.data] = currentBoneIndex;

                    //LOG_DEBUG("New bone given index %u", currentBoneIndex);
                }
                else {
                    currentBoneIndex = iter->second;

                    //LOG_DEBUG("Bone is index %u", currentBoneIndex);
                }
            }
            
            //look up all parent nodes for the node with this name and add them too
            aiNode * currNode = scene->mRootNode->FindNode(currBone->mName);
            sceneBoneData.m_boneIndexNodes[currentBoneIndex] = currNode;

            uint16_t prevBoneIndex = currentBoneIndex;
            
            //recursively look up parents of the node
            while(currNode->mParent) {
                currNode = currNode->mParent;

                //look up bone index
                {
                    auto iter = m_boneNameMap.find(currNode->mName.data);

                    //LOG_DEBUG("Found bone with name %s", currNode->mName.data);

                    if(iter == m_boneNameMap.end()) {
                        if(!m_creating) {
                            LOG_FATAL_ERROR("Imported animset doesn't have bone with name %s. Just generate a new animset and start everything from scratch.", currNode->mName.data);
                        }

                        currentBoneIndex = m_boneNameMap.size();
                        m_boneNameMap[currNode->mName.data] = currentBoneIndex;

                        //LOG_DEBUG("New bone given index %u", currentBoneIndex);
                    }
                    else {
                        currentBoneIndex = iter->second;

                        //LOG_DEBUG("Bone is index %u", currentBoneIndex);
                    }
                }

                //LOG_DEBUG("Assigned bone %u as child of bone %u", prevBoneIndex, currentBoneIndex);

                m_boneParentIndeces[prevBoneIndex] = currentBoneIndex;      //mark the parent
                sceneBoneData.m_boneIndexNodes[currentBoneIndex] = currNode;

                prevBoneIndex = currentBoneIndex;
            }

            //LOG_DEBUG("Reached root node of heirarchy.\n");
        }

        //LOG_DEBUG("Done adding mesh nodes.\n");
    }

    //LOG_DEBUG("Done adding scene bones.\n");

    //debug consistency check to make sure the structures are correct
    /*for(unsigned int bone = 0; bone < m_boneNodes.size(); bone++) {
        if(m_boneIndeces.at(m_boneNodes.at(bone)->mName.data) != bone) {
            throw std::runtime_error("Bone consistency check problem.  The programmer isn't 1337 enough!");
        }
    }*/
}

/*void AnimSet::computeHeirarchies() {
    //doing a secondary pass to really make sure the heirarchies are set up after all bones in the animset are found
    //(I hate my life so much right now, but Assimp is both amazing at some things but very complex when it comes to finding bones correctly)

    for(auto sceneIter = m_sceneBoneData.begin(); sceneIter != m_sceneBoneData.end(); sceneIter++) {
        SceneBoneData& sceneBoneData = m_sceneBoneData[sceneIter->first];

        for(auto boneNameIter = m_boneNameMap.begin(); boneNameIter != m_boneNameMap.end(); boneNameIter++) {
            //try to find the node with this name
            aiNode * currNode = sceneIter->first->mRootNode->FindNode(boneNameIter->first.c_str());
            uint16_t currentBoneIndex = boneNameIter->second;

            //if the node was found, recurse through it and all parents, children may not necessarily be useful so don't go thatway
            //LOL die CHILDREN!!!!  Oh god I want this code to be done already!!!!  Maybe I should take a DooM break!!!
            if(currNode) {
                //check if the node's parent already has its parent tracked, if yes, just be done
                if(sceneBoneData.m_boneParentIndeces.find(currentBoneIndex) != sceneBoneData.m_boneParentIndeces.end()) {
                    LOG_DEBUG("Bone name %u %s already has its parent %u", currentBoneIndex, currNode->mName.data, sceneBoneData.m_boneParentIndeces.at(currentBoneIndex));
                    assert(sceneBoneData.m_boneIndexNodes.find(currentBoneIndex) != sceneBoneData.m_boneIndexNodes.end());
                    continue;
                }

                sceneBoneData.m_boneIndexNodes[currentBoneIndex] = currNode;
                uint16_t prevBoneIndex = currentBoneIndex;
            
                //recursively look up parents of the node
                while(currNode->mParent) {
                    currNode = currNode->mParent;

                    //look up bone index
                    currentBoneIndex = m_boneNameMap.at(currNode->mName.data);

                    //check if the node's parent already has its parent tracked, if yes, just be done
                    if(sceneBoneData.m_boneParentIndeces.find(currentBoneIndex) != sceneBoneData.m_boneParentIndeces.end()) {
                        LOG_DEBUG("Bone name %u %s already has its parent %u", currentBoneIndex, currNode->mName.data, sceneBoneData.m_boneParentIndeces.at(currentBoneIndex));
                        assert(sceneBoneData.m_boneIndexNodes.find(currentBoneIndex) != sceneBoneData.m_boneIndexNodes.end());
                        break;
                    }

                    sceneBoneData.m_boneParentIndeces[prevBoneIndex] = currentBoneIndex;      //mark the parent
                    prevBoneIndex = currentBoneIndex;
                }
            }
            else {
                LOG_DEBUG("Bone name %u %s not in scene", currentBoneIndex, boneNameIter->first.c_str());
            }
        }

        LOG_DEBUG("\n\n");
    }

    LOG_DEBUG("\n\n");
}*/

void AnimSet::save(const char * path) const {
    illFileSystem::File * openFile = illFileSystem::fileSystem->openWrite(path);

    //write magic number
    openFile->writeB64(ANIMSET_MAGIC);

    //number of bones
    openFile->writeL16(m_boneNameMap.size());

    //reverse the name to index map
    Array<const char *> reverseMap;
    reverseMap.resize(m_boneNameMap.size());

    for(auto iter = m_boneNameMap.cbegin(); iter != m_boneNameMap.end(); iter++) {
        reverseMap[iter->second] = iter->first.c_str();
    }

    //now print the bone names in order of index
    for(uint16_t bone = 0; bone < reverseMap.size(); bone++) {
        openFile->writeString(reverseMap[bone]);
    }

    delete openFile;
}