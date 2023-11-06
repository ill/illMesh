#include "Animation.h"
#include "Skeleton.h"
#include "AnimSet.h"

#include "illEngine/FileSystem/FileSystem.h"
#include "illEngine/FileSystem/File.h"

#include "illEngine/Logging/logging.h"

const uint64_t ANIM_MAGIC = 0x494C4C414E494D30;		//ILLANIM0 in 64 bit big endian

void Animation::import(const aiAnimation* animation, const Skeleton * skeleton, const AnimSet * animset) {
    m_animation = animation;

    //compute duration
    m_duration = (float) (m_animation->mDuration / m_animation->mTicksPerSecond);

    //compute bone transforms for each key frame relative to the bind pose
    for(uint16_t bone = 0; bone < (uint16_t) m_animation->mNumChannels; bone++) {
        aiNodeAnim* currAnim = m_animation->mChannels[bone];
        
        uint16_t boneIndex;

        {
            auto iter = animset->m_boneNameMap.find(currAnim->mNodeName.data);

            if(iter == animset->m_boneNameMap.end()) {
                LOG_FATAL_ERROR("Exporting animations failed.  Found a bone with name %s which isn't in the animset.  This is really weird.", currAnim->mNodeName.data);
            }

            boneIndex = iter->second;
        }

        assert(m_boneAnimation.find(boneIndex) == m_boneAnimation.end());

        AnimData& animData = m_boneAnimation[boneIndex];
        
        //decompose the bind pose
        glm::vec3 bindPosInverse = -getTransformPosition(skeleton->m_bones[boneIndex].m_relativeTransform);
        glm::quat bindRotInverse;
        glm::vec3 bindScaleInverse;

        getTransformRotationScale(skeleton->m_bones[boneIndex].m_relativeTransform, bindRotInverse, bindScaleInverse);
        bindRotInverse = glm::inverse(bindRotInverse);
        bindScaleInverse = 1.0f / bindScaleInverse;

        //for each position key, get the position relative to the bind pose instead
        for(unsigned int key = 0; key < currAnim->mNumPositionKeys; key++) {
            float time = (float) (currAnim->mPositionKeys[key].mTime / m_animation->mTicksPerSecond);
            
            //get the relative position offset
            animData.m_positionKeys[time] = glm::vec3(currAnim->mPositionKeys[key].mValue.x,
                currAnim->mPositionKeys[key].mValue.y,
                currAnim->mPositionKeys[key].mValue.z) /*+ bindPosInverse*/;
        }

        //for each rotation key, get the rotation relative to the bind pose instead
        for(unsigned int key = 0; key < currAnim->mNumRotationKeys; key++) {
            float time = (float) (currAnim->mRotationKeys[key].mTime / m_animation->mTicksPerSecond);

            animData.m_rotationKeys[time] = /*bindRotInverse **/ glm::quat(currAnim->mRotationKeys[key].mValue.w, 
                currAnim->mRotationKeys[key].mValue.x,
                currAnim->mRotationKeys[key].mValue.y,
                currAnim->mRotationKeys[key].mValue.z);
        }

        //for each scale key, get the scale relative to the bind pose instead
        for(unsigned int key = 0; key < currAnim->mNumScalingKeys; key++) {
            float time = (float) (currAnim->mScalingKeys[key].mTime / m_animation->mTicksPerSecond);

            //get the relative position offset
            animData.m_scalingKeys[time] = glm::vec3(currAnim->mScalingKeys[key].mValue.x,
                currAnim->mScalingKeys[key].mValue.y,
                currAnim->mScalingKeys[key].mValue.z)
                /** bindScaleInverse*/;
        }
    }
}

void Animation::save(const char * path) {
    illFileSystem::File * openFile = illFileSystem::fileSystem->openWrite(path);

    //write magic number
    openFile->writeB64(ANIM_MAGIC);

    //write duration in seconds
    openFile->writeLF(m_duration);

    //write number of bones
    openFile->writeL16((uint16_t) m_boneAnimation.size());

    //the bones
    for(auto boneIter = m_boneAnimation.cbegin(); boneIter != m_boneAnimation.end(); boneIter++) {
        //bone index
        openFile->writeL16(boneIter->first);

        //number of position keys
        openFile->writeL16((uint16_t) boneIter->second.m_positionKeys.size());

        //position keys
        for(auto keyIter = boneIter->second.m_positionKeys.cbegin(); keyIter != boneIter->second.m_positionKeys.end(); keyIter++) {
            //write time stamp
            openFile->writeLF(keyIter->first);

            //write the position
            openFile->writeLF(keyIter->second.x);
            openFile->writeLF(keyIter->second.y);
            openFile->writeLF(keyIter->second.z);
        }

        //number of rotation keys
        openFile->writeL16((uint16_t) boneIter->second.m_rotationKeys.size());

        //rotation keys
        for(auto keyIter = boneIter->second.m_rotationKeys.cbegin(); keyIter != boneIter->second.m_rotationKeys.end(); keyIter++) {
            //write time stamp
            openFile->writeLF(keyIter->first);

            //write the position
            openFile->writeLF(keyIter->second.x);
            openFile->writeLF(keyIter->second.y);
            openFile->writeLF(keyIter->second.z);
            openFile->writeLF(keyIter->second.w);
        }

        //number of scaling keys
        openFile->writeL16((uint16_t) boneIter->second.m_scalingKeys.size());

        //scaling keys
        for(auto keyIter = boneIter->second.m_scalingKeys.cbegin(); keyIter != boneIter->second.m_scalingKeys.end(); keyIter++) {
            //write time stamp
            openFile->writeLF(keyIter->first);

            //write the position
            openFile->writeLF(keyIter->second.x);
            openFile->writeLF(keyIter->second.y);
            openFile->writeLF(keyIter->second.z);
        }
    }

    delete openFile;
}