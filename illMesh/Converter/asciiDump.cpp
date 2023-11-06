#include <glm/gtc/quaternion.hpp>

#include <stdint.h>
#include "asciiDump.h"
#include "illEngine/FileSystem/FileSystem.h"
#include "illEngine/FileSystem/File.h"
#include "illEngine/Logging/logging.h"
#include "illEngine/Util/serial/Array.h"
#include "illEngine/Util/Geometry/MeshData.h"

const uint64_t ANIM_MAGIC = 0x494C4C414E494D30;		    //ILLANIM0 in 64 bit big endian
const uint64_t ANIMSET_MAGIC = 0x494C414E53455430;		//ILANSET0 in 64 bit big endian
const uint64_t MESH_MAGIC = 0x494C4C4D45534831;	        //ILLMESH1 in 64 bit big endian
const uint64_t SKEL_MAGIC = 0x494C4C534B454C30;		    //ILLSKEL0 in 64 bit big endian

void dumpAnimset(illFileSystem::File * openFile);
void dumpAnimation(illFileSystem::File * openFile);
void dumpSkeleton(illFileSystem::File * openFile);
void dumpMesh(illFileSystem::File * openFile);

void asciiDump(const char * path) {
    illFileSystem::File * openFile = illFileSystem::fileSystem->openRead(path);

    //figure out file type based on magic number
    {
        uint64_t magic;
        openFile->readB64(magic);

        switch(magic) {
        case ANIM_MAGIC:
            LOG_INFO("Dumping contents of Animation file %s\n", path);
            dumpAnimation(openFile);
            break;

        case ANIMSET_MAGIC:
            LOG_INFO("Dumping contents of Animation Set file %s\n", path);
            dumpAnimset(openFile);
            break;

        case MESH_MAGIC:
            LOG_INFO("Dumping contents of Mesh file %s\n", path);
            dumpMesh(openFile);
            break;

        case SKEL_MAGIC:
            LOG_INFO("Dumping contents of Skeleton file %s\n", path);
            dumpSkeleton(openFile);
            break;

        default:
            LOG_INFO("File %s is not a valid animset, animation, mesh, or skeleton file.", path);
            break;
        }
    }

    delete openFile;
}

void dumpAnimset(illFileSystem::File * openFile) {
    //num bones
    uint16_t numBones;
    openFile->readL16(numBones);

    LOG_INFO("%u bones\n", numBones);

    //bone names
    Array<char> strBuffer;

    for(uint16_t bone = 0; bone < numBones; bone++) {
        uint16_t stringBufferLength = openFile->readStringBufferLength();			
		strBuffer.reserve(stringBufferLength);
		openFile->readString(&strBuffer[0], stringBufferLength);

        LOG_INFO("Bone: %u Name: %s", bone, &strBuffer[0]);
    }
    
    LOG_INFO("\n");
    LOG_INFO("End of animation set file\n\n");
}

void dumpAnimation(illFileSystem::File * openFile) {
    //duration
    {
        float duration;
        openFile->readLF(duration);

        LOG_INFO("Duration %f seconds", duration);
    }

    //num bones
    uint16_t numBones;
    openFile->readL16(numBones);

    LOG_INFO("%u bones\n", numBones);

    for(uint16_t bone = 0; bone < numBones; bone++) {
        //bone index
        {
            uint16_t boneIndex;
            openFile->readL16(boneIndex);

            LOG_INFO("Bone index %u\n", boneIndex);
        }

        //position keys
        {
            uint16_t keys;
            openFile->readL16(keys);

            LOG_INFO("%u Position Keys\n", keys);

            for(uint16_t key = 0; key < keys; key++) {
                //time stamp
                float time;
                openFile->readLF(time);

                //position
                glm::vec3 data;
                openFile->readLF(data.x);
                openFile->readLF(data.y);
                openFile->readLF(data.z);

                LOG_INFO("Time %f Position (%f, %f, %f)", time, data.x, data.y, data.z);
            }
        }

        LOG_INFO("\n");

        //rotation keys
        {
            uint16_t keys;
            openFile->readL16(keys);

            LOG_INFO("%u Rotation Keys\n", keys);

            for(uint16_t key = 0; key < keys; key++) {
                //time stamp
                float time;
                openFile->readLF(time);

                //scale
                glm::quat data;
                openFile->readLF(data.x);
                openFile->readLF(data.y);
                openFile->readLF(data.z);
                openFile->readLF(data.w);

                LOG_INFO("Time %f Rotation quat XYZW (%f, %f, %f, %f)", time, data.x, data.y, data.z, data.w);
            }
        }

        LOG_INFO("\n");

        //scaling keys
        {
            uint16_t keys;
            openFile->readL16(keys);

            LOG_INFO("%u Scaling Keys\n", keys);

            for(uint16_t key = 0; key < keys; key++) {
                //time stamp
                float time;
                openFile->readLF(time);

                //scale
                glm::vec3 data;
                openFile->readLF(data.x);
                openFile->readLF(data.y);
                openFile->readLF(data.z);

                LOG_INFO("Time %f Scale (%f, %f, %f)", time, data.x, data.y, data.z);
            }
        }

        LOG_INFO("\n");
    }

    LOG_INFO("End of animation file\n\n");
}

void dumpSkeleton(illFileSystem::File * openFile) {
    //num bones
    uint16_t numBones;
    openFile->readL16(numBones);

    LOG_INFO("%u bones\n", numBones);

    for(uint16_t bone = 0; bone < numBones; bone++) {
        LOG_INFO("Bone: %u\n", bone);

        //bind pose
        {
            LOG_INFO("Bind pose relative transform");

            glm::mat4 mat;

            for(unsigned int matCol = 0; matCol < 4; matCol++) {
                for(unsigned int matRow = 0; matRow < 4; matRow++) {
                    openFile->readLF(mat[matCol][matRow]);
                }            
            }

            for(unsigned int row = 0; row < 4; row++) {
                LOG_INFO("[%7.4f %7.4f %7.4f %7.4f]", 
                    mat[0][row], mat[1][row], mat[2][row], mat[3][row]);
            }
        }

        LOG_INFO("\n");

        //offset
        {
            LOG_INFO("Offset matrix (inverse of full bind pose transform)");

            glm::mat4 mat;

            for(unsigned int matCol = 0; matCol < 4; matCol++) {
                for(unsigned int matRow = 0; matRow < 4; matRow++) {
                    openFile->readLF(mat[matCol][matRow]);
                }            
            }

            for(unsigned int row = 0; row < 4; row++) {
                LOG_INFO("[%7.4f %7.4f %7.4f %7.4f]", 
                    mat[0][row], mat[1][row], mat[2][row], mat[3][row]);
            }
        }

        LOG_INFO("\n");
    }

    //bone heirarchy
    LOG_INFO("Bone parent list\n");

    for(uint16_t bone = 0; bone < numBones; bone++) {
        uint16_t parent;
        openFile->readL16(parent);

        if(parent == bone) {
            LOG_INFO("Bone: %u Root", bone);
        }
        else {
            LOG_INFO("Bone: %u Parent: %u", bone, parent);
        }
    }

    LOG_INFO("\n");
    LOG_INFO("End of skeleton file\n\n");
}

void dumpMesh(illFileSystem::File * openFile) {
    //read features mask
    FeaturesMask features;
    openFile->read8(features);

    if(features & MeshFeatures::MF_POSITION) {
        LOG_INFO("Has positions");
    }
    else {
        LOG_INFO("Doesn't have positions");
    }

    if(features & MeshFeatures::MF_NORMAL) {
        LOG_INFO("Has normals");
    }
    else {
        LOG_INFO("Doesn't have normals");
    }

    if(features & MeshFeatures::MF_TANGENT) {
        LOG_INFO("Has tangents and binormals");
    }
    else {
        LOG_INFO("Doesn't have tangents and binormals");
    }

    if(features & MeshFeatures::MF_TEX_COORD) {
        LOG_INFO("Has texture coords");
    }
    else {
        LOG_INFO("Doesn't have texture coords");
    }

    if(features & MeshFeatures::MF_BLEND_DATA) {
        LOG_INFO("Has blend data");
    }
    else {
        LOG_INFO("Doesn't have blend data");
    }

    if(features & MeshFeatures::MF_COLOR) {
        LOG_INFO("Has colors");
    }
    else {
        LOG_INFO("Doesn't have colors");
    }

    LOG_INFO("\n");

    uint8_t numGroups;
    openFile->read8(numGroups);

    LOG_INFO("%u Primitive Groups", numGroups);

    uint32_t numVerts;
    openFile->readL32(numVerts);

    LOG_INFO("%u Vertices", numVerts);

    uint16_t numIndices;
    openFile->readL16(numIndices);

    LOG_INFO("%u Indices", numIndices);
    LOG_INFO("\n");
    
    //read group data
    for(uint8_t group = 0; group < numGroups; group++) {
        LOG_INFO("Group %u", group);

        //group type
        {
            uint8_t data;
            openFile->read8(data);

            switch(data) {
            case 0:
                LOG_INFO("0: Points");
                break;
            case 1:
                LOG_INFO("1: Lines");
                break;
            case 2:
                LOG_INFO("2: Line Loop");
                break;
            case 3:
                LOG_INFO("3: Triangles");
                break;
            case 4:
                LOG_INFO("4: Triangle Strip");
                break;
            case 5:
                LOG_INFO("5: Triangle Fan");
                break;
            default:
                LOG_INFO("%u: Unknown", data);
                break;
            }
        }

        //starting index
        {
            uint16_t data;
            openFile->readL16(data);

            LOG_INFO("Starting Index: %u", data);
        }

        //number of elements
        {
            uint16_t data;
            openFile->readL16(data);

            LOG_INFO("Number of elements: %u", data);
        }

        LOG_INFO("\n");
    }

    LOG_INFO("\n");

    //read the VBO data
    for(uint32_t vertex = 0; vertex < numVerts; vertex++) {
        LOG_INFO("Vertex %u\n", vertex);

        //position
        if(features & MeshFeatures::MF_POSITION) {
            glm::vec3 data;
            openFile->readLF(data.x);
            openFile->readLF(data.y);
            openFile->readLF(data.z);

            LOG_INFO("Position (%f, %f, %f)", data.x, data.y, data.z);
        }

        //normal
        if(features & MeshFeatures::MF_NORMAL) {
            glm::vec3 data;
            openFile->readLF(data.x);
            openFile->readLF(data.y);
            openFile->readLF(data.z);

            LOG_INFO("Normal (%f, %f, %f)", data.x, data.y, data.z);
        }

        //tangent
        if(features & MeshFeatures::MF_TANGENT) {
            glm::vec3 data;
            openFile->readLF(data.x);
            openFile->readLF(data.y);
            openFile->readLF(data.z);

            LOG_INFO("Tangent (%f, %f, %f)", data.x, data.y, data.z);

            openFile->readLF(data.x);
            openFile->readLF(data.y);
            openFile->readLF(data.z);

            LOG_INFO("Binormal (%f, %f, %f)", data.x, data.y, data.z);
        }
        
        //blend weights
        if(features & MeshFeatures::MF_BLEND_DATA) {
            glm::vec4 data;
            openFile->readLF(data.x);
            openFile->readLF(data.y);
            openFile->readLF(data.z);
            openFile->readLF(data.w);

            LOG_INFO("Blend Indices in float (%f, %f, %f, %f)", data.x, data.y, data.z, data.w);

            openFile->readLF(data.x);
            openFile->readLF(data.y);
            openFile->readLF(data.z);
            openFile->readLF(data.w);

            LOG_INFO("Blend weights (%f, %f, %f, %f)", data.x, data.y, data.z, data.w);
        }
        
        //tex coords
        if(features & MeshFeatures::MF_TEX_COORD) {
            glm::vec2 data;
            openFile->readLF(data.x);
            openFile->readLF(data.y);

            LOG_INFO("Texture Coordinates (AKA uv) (%f, %f)", data.x, data.y);
        }

        //write colors (at the moment only color channel 0 is supported)
        if(features & MeshFeatures::MF_COLOR) {
            glm::vec4 data;
            openFile->readLF(data.x);
            openFile->readLF(data.y);
            openFile->readLF(data.z);
            openFile->readLF(data.w);

            LOG_INFO("Vertex Color RGBA (%f, %f, %f, %f)", data.x, data.y, data.z, data.w);
        }

        LOG_INFO("\n");
    }
    
    //IBO
    for(uint16_t index = 0; index < numIndices; index++) {
        uint16_t data;
        openFile->readL16(data);

        LOG_INFO("Index %u %u", index, data);
    }

    //TODO: print out actual group data

    LOG_INFO("\n");
    LOG_INFO("End of mesh file\n\n");
}