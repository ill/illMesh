#include <new>

#include "MeshMerger.h"
#include "illEngine/Util/Illmesh/IllmeshLoader.h"
#include "illEngine/Logging/logging.h"
#include "illEngine/FileSystem/FileSystem.h"
#include "illEngine/FileSystem/File.h"
#include "illEngine/Util/serial/Array.h"

void MeshMerger::merge() {
    uint32_t totalIndices = 0;
    uint32_t totalVertices = 0;
    FeaturesMask mergeFeatures = 0;
        
    Array<MeshData<>> importedMeshes(m_paths.size());

    {
        int meshInd = 0;

        for(auto iter = m_paths.begin(); iter != m_paths.end(); iter++, meshInd++) {
            IllmeshLoader loader(iter->c_str());

            if(loader.m_numGroups != 1) {
                LOG_FATAL_ERROR("At the moment mesh merger only merges meshes with 1 primitive group");
            }

            mergeFeatures |= loader.m_features;

            totalIndices += loader.m_numInd;
            totalVertices += loader.m_numVert;

            new (&importedMeshes[meshInd]) MeshData<>(loader.m_numInd, loader.m_numVert, loader.m_numGroups, loader.m_features, true);        
            loader.buildMesh(importedMeshes[meshInd]);
        }
    }

    illFileSystem::File * openFile = illFileSystem::fileSystem->openWrite(m_exportPath.c_str());
	
	//write magic string
    openFile->writeB64(MESH_MAGIC);

    //write the features mask
    {
        FeaturesMask features = 0;

        if(featureMaskHasPositions(mergeFeatures)) {
            features |= MeshFeatures::MF_POSITION;
        }

        if(featureMaskHasNormals(mergeFeatures)) {
            features |= MeshFeatures::MF_NORMAL;
        }

        if(featureMaskHasTangents(mergeFeatures)) {
            features |= MeshFeatures::MF_TANGENT;
        }

        if(featureMaskHasTexCoords(mergeFeatures)) {
            features |= MeshFeatures::MF_TEX_COORD;
        }

        if(featureMaskHasBlendData(mergeFeatures)) {
            features |= MeshFeatures::MF_BLEND_DATA;
        }

        if(featureMaskHasColors(mergeFeatures)) {
            features |= MeshFeatures::MF_COLOR;
        }

        openFile->write8(features);
    }

    //number of groups, set at 1.  Use the mesh merger tool to create multiple groups.
    openFile->write8(importedMeshes.size());

    //number of vertices
    openFile->writeL32((uint32_t) totalVertices);

    //number of indices
    openFile->writeL16((uint16_t) totalIndices);
    
    //write the group data
    {
        uint32_t indexOffset = 0;

        for(unsigned int meshInd = 0; meshInd < importedMeshes.size(); meshInd++) {
            MeshData<>::PrimitiveGroup& group = importedMeshes[meshInd].getPrimitveGroup(0);

            openFile->write8((uint8_t) group.m_type);
            openFile->writeL16((uint8_t) (group.m_beginIndex + indexOffset));
            openFile->writeL16((uint16_t) group.m_numIndices);

            indexOffset += importedMeshes[meshInd].getNumInd();
        }
    }

    //write the VBO data
    for(unsigned int meshInd = 0; meshInd < importedMeshes.size(); meshInd++) {
        MeshData<>& mesh = importedMeshes[meshInd];

        size_t numElements = mesh.getVertexSize() / sizeof(float);

        for(unsigned int vertex = 0, vboIndex = 0; vertex < mesh.getNumVert(); vertex++) {
            for(unsigned int elementIndex = 0; elementIndex < numElements; elementIndex++, vboIndex += sizeof(float)) {
				openFile->writeLF(*reinterpret_cast<float *>(mesh.getData() + vboIndex));
            }
        }
    }
    
    //write the IBO array
    {
        uint16_t indexOffset = 0;

        for(unsigned int meshInd = 0; meshInd < importedMeshes.size(); meshInd++) {
            MeshData<>& mesh = importedMeshes[meshInd];

            for(unsigned int index = 0, iboIndex = 0; index < mesh.getNumInd(); index++, iboIndex++) {
                openFile->writeL16(*(mesh.getIndices() + iboIndex) + indexOffset);
            }

            indexOffset += mesh.getNumVert();
        }
    }
    
    delete openFile;
}