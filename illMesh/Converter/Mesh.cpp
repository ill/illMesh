#include "Mesh.h"
#include "AnimSet.h"

#include "illEngine/FileSystem/FileSystem.h"
#include "illEngine/FileSystem/File.h"
#include "illEngine/Util/Geometry/MeshData.h"

const uint64_t MESH_MAGIC = 0x494C4C4D45534831;	//ILLMESH1 in 64 bit big endian

void Mesh::save(const char * path) const {
    illFileSystem::File * openFile = illFileSystem::fileSystem->openWrite(path);
	
	//write magic string
    openFile->writeB64(MESH_MAGIC);

    //write the features mask
    {
        FeaturesMask features = 0;

        if(m_mesh->HasPositions()) {
            features |= MeshFeatures::MF_POSITION;
        }

        if(m_mesh->HasNormals()) {
            features |= MeshFeatures::MF_NORMAL;
        }

        if(m_mesh->HasTangentsAndBitangents()) {
            features |= MeshFeatures::MF_TANGENT;
        }

        if(m_mesh->HasTextureCoords(0)) {
            features |= MeshFeatures::MF_TEX_COORD;
        }

        if(m_mesh->HasBones()) {
            features |= MeshFeatures::MF_BLEND_DATA;
        }

        if(m_mesh->HasVertexColors(0)) {
            features |= MeshFeatures::MF_COLOR;
        }

        openFile->write8(features);
    }

    //number of groups, set at 1.  Use the mesh merger tool to create multiple groups.
    openFile->write8(1);

    //number of vertices
    openFile->writeL32((uint32_t) m_mesh->mNumVertices);

    //number of indices
    openFile->writeL16((uint16_t) m_mesh->mNumFaces * 3);
    
    //write the group data
    openFile->write8(3);        //hardcoded as Triangles
    openFile->writeL16(0);
    openFile->writeL16((uint16_t) m_mesh->mNumFaces * 3);

    //write the VBO data
    for(unsigned int vertex = 0; vertex < m_mesh->mNumVertices; vertex++) {
        //write position)
        if(m_mesh->HasPositions()) {
            aiVector3D& currVec = m_mesh->mVertices[vertex];
            openFile->writeLF(currVec.x);
            openFile->writeLF(currVec.y);
            openFile->writeLF(currVec.z);
        }

        //write normal
        if(m_mesh->HasNormals()) {
            aiVector3D& currVec = m_mesh->mNormals[vertex];
            openFile->writeLF(currVec.x);
            openFile->writeLF(currVec.y);
            openFile->writeLF(currVec.z);
        }

        //write tangents
        if(m_mesh->HasTangentsAndBitangents()) {
            aiVector3D& currVec = m_mesh->mTangents[vertex];
            openFile->writeLF(currVec.x);
            openFile->writeLF(currVec.y);
            openFile->writeLF(currVec.z);

            currVec = m_mesh->mBitangents[vertex];
            openFile->writeLF(currVec.x);
            openFile->writeLF(currVec.y);
            openFile->writeLF(currVec.z);
        }

        //write blend weights
        if(m_mesh->HasBones()) {
            BoneMap& currBoneMap = m_boneWeights[vertex];

            //write the indeces
            {
                int bone = 0;

                for(auto iter = currBoneMap.begin(); iter != currBoneMap.end(); iter++) {
                    bone++;
                    openFile->writeLF((float) iter->first);
                }

                //pad the rest
                while(bone < 4) {
                    bone++;
                    openFile->writeLF(0.0f);
                }
            }

            //write the weights
            {
                int bone = 0;

                for(auto iter = currBoneMap.begin(); iter != currBoneMap.end(); iter++) {
                    bone++;
                    openFile->writeLF((float) iter->second);
                }

                //pad the rest
                while(bone < 4) {
                    bone++;
                    openFile->writeLF(0.0f);
                }
            }
        }

        //write tex coords (at the moment only tex coord channel 0 is supported)
        if(m_mesh->HasTextureCoords(0)) {
            aiVector3D& currVec = m_mesh->mTextureCoords[0][vertex];
            openFile->writeLF(currVec.x);
            openFile->writeLF(currVec.y);
        }

        //write colors (at the moment only color channel 0 is supported)
        if(m_mesh->HasVertexColors(0)) {
            aiColor4D& currVec = m_mesh->mColors[0][vertex];
            openFile->writeLF(currVec.r);
            openFile->writeLF(currVec.g);
            openFile->writeLF(currVec.b);
            openFile->writeLF(currVec.a);
        }
    }

    //write the IBO array
    for(unsigned int face = 0; face < m_mesh->mNumFaces; face++) {
        aiFace& currFace = m_mesh->mFaces[face];
        
        for(unsigned int vertex = 0; vertex < currFace.mNumIndices; ) {
            openFile->writeL16(currFace.mIndices[vertex++]);
        }
    }

    delete openFile;
}

void Mesh::import(const aiMesh * mesh, const AnimSet * animset) {
    m_mesh = mesh;

    //compute bone VBO data
    if(m_mesh->HasBones()) {
        m_boneWeights = new BoneMap[m_mesh->mNumVertices];

        //for each bone
        for(unsigned int bone = 0; bone < m_mesh->mNumBones; bone++) {
            aiBone* currBone = m_mesh->mBones[bone];

            //for each vertex affected by the bone
            for(unsigned int weight = 0; weight < currBone->mNumWeights; weight++) {
                aiVertexWeight& currWeight = currBone->mWeights[weight];

                //look up skeleton bone index by name of bone
                size_t boneIndex = animset->m_boneNameMap.at(currBone->mName.data);

                m_boneWeights[currWeight.mVertexId][boneIndex] = currWeight.mWeight;
            }
        }
    }
}