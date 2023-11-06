#include <iostream>
#include <cstring>
#include <vector>

#include "illEngine/Logging/serial/SerialLogger.h"
#include "illEngine/Logging/StdioLogger.h"
#include "illEngine/FileSystem-Stdio/StdioFileSystem.h"

#include "illEngine/Logging/logging.h"

#include "MeshMerger.h"
#include "Importer.h"
#include "Skeleton.h"
#include "Mesh.h"
#include "AnimSet.h"
#include "Animation.h"

#include "asciiDump.h"

using namespace std;

illLogging::SerialLogger thisLogger;
illLogging::Logger * illLogging::logger = &thisLogger;

illStdio::StdioFileSystem thisFileSystem;
illFileSystem::FileSystem * illFileSystem::fileSystem = &thisFileSystem;

illLogging::StdioLogger stdioLogger;

void showHelp() {
    LOG_INFO("Help coming soon");
}

int main(int argc, const char ** argv) {
    try {

        illLogging::logger->addLogDestination(&stdioLogger);

        //process command line args
        if(argc <= 1) {
            LOG_INFO("No arguments specified");
            showHelp();
            return 1;
        }

        if(strncmp(argv[1], "-help", 10) == 0) {
            showHelp();
            return 0;
        }
        else if(strncmp(argv[1], "-ascii", 10) == 0) {
            LOG_INFO("Performing ascii dump");

            int arg = 2;

            while(arg < argc) {
                asciiDump(argv[arg++]);
            }

            return 0;
        }
        else if(strncmp(argv[1], "-mergemesh", 15) == 0) {
            LOG_INFO("Performing Merge Mesh");

            int arg = 2;

            MeshMerger merger;

            merger.m_exportPath = argv[arg++];

            while(arg < argc) {
                merger.m_paths.push_back(argv[arg++]);
            }

            merger.merge();

            return 0;
        }

    
        const char * asetFile = NULL;
    
        Importer importer;
        importer.m_mainSkeletonImport = 0;
        bool mainSkeletonImportSet = false;

        {
            enum class State {
                MAIN,
                PARSE_IN
            } state = State::MAIN;

            int arg = 1;

            while(arg < argc) {
                const char * currArg = argv[arg++];

                switch(state) {
                case State::MAIN: {
                    if(strncmp(currArg, "-anset", 10) == 0) {    //animset output file
                        if(asetFile) {
                            LOG_INFO("Warning: animation set file %s already specified", asetFile);
                        }

                        if(arg >= argc) {
                            LOG_FATAL_ERROR("Expecting a file name after the -anset parameter");
                        }

                        asetFile = argv[arg++];
		            }
                    else if(strncmp(currArg, "-main", 10) == 0) {
                        LOG_FATAL_ERROR("-main paramater needs to come after a filename");
                    }
                    else if(strncmp(currArg, "-mesh", 10) == 0) {
                        LOG_FATAL_ERROR("-mesh paramater needs to come after a filename");
                    }
                    else if(strncmp(currArg, "-anim", 10) == 0) {
                        LOG_FATAL_ERROR("-anim paramater needs to come after a filename");
                    }
                    else if(strncmp(currArg, "-skel", 10) == 0) {
                        LOG_FATAL_ERROR("-skel paramater needs to come after a filename");
                    }
                    else if(strncmp(currArg, "-skelin", 10) == 0) {
                        LOG_FATAL_ERROR("-skelin paramater needs to come after a filename");
                    }
                    else {
                        if(!illFileSystem::fileSystem->fileExists(currArg)) {
                            LOG_FATAL_ERROR("File %s doesn't exist for import", currArg);
                        }

                        //a file to import
                        importer.m_importFiles.push_back(Importer::Import());
                        importer.m_importFiles.back().m_importFile = currArg;

                        LOG_INFO("Importing file %s", currArg);

                        state = State::PARSE_IN;
                    }

                    } break;

                case State::PARSE_IN: {
                    if(strncmp(currArg, "-main", 10) == 0) {
                        if(mainSkeletonImportSet) {
                            LOG_INFO("Warning: main skeleton file was already specified for %s, overriding and using this one",
                                importer.m_importFiles.at(importer.m_mainSkeletonImport).m_importFile);
                        }

                        mainSkeletonImportSet = true;

                        LOG_INFO("Setting %s as the main skeleton import file.  All animations will be relative to this skeleton in this file.",
                            importer.m_importFiles.back().m_importFile);

                        importer.m_mainSkeletonImport = importer.m_importFiles.size() - 1;
                    }
                    else if(strncmp(currArg, "-mesh", 10) == 0) {     //mesh output file
                        if(importer.m_importFiles.back().m_meshOutFile) {
                            LOG_INFO("Warning: mesh file output %s already specified for %s",
                                importer.m_importFiles.back().m_skelOutFile,
                                importer.m_importFiles.back().m_importFile);
                        }

                        if(arg >= argc) {
                            LOG_FATAL_ERROR("Expecting a file name after the -mesh parameter or the -merge keyword");
                        }

                        if(strncmp(argv[arg++], "-merge", 10) == 0) {
                            importer.m_importFiles.back().m_mergeMesh = true;

                            if(arg >= argc) {
                                LOG_FATAL_ERROR("Expecting a file name after the -merge keyword");
                            }
                        }

                        importer.m_importFiles.back().m_meshOutFile = argv[arg++];
                        LOG_INFO("Exporting mesh file with name %s", importer.m_importFiles.back().m_meshOutFile);
                    }
                    else if(strncmp(currArg, "-anim", 10) == 0) {    //animation output file
                        if(importer.m_importFiles.back().m_animOutFile) {
                            LOG_INFO("Warning: animation file output %s already specified for %s",
                                importer.m_importFiles.back().m_skelOutFile,
                                importer.m_importFiles.back().m_importFile);
                        }

                        if(arg >= argc) {
                            LOG_FATAL_ERROR("Expecting a file name after the -anim parameter");
                        }

                        importer.m_importFiles.back().m_animOutFile = argv[arg++];
                        LOG_INFO("Exporting animation file with name %s", importer.m_importFiles.back().m_animOutFile);
                    }
                    else if(strncmp(currArg, "-skel", 10) == 0) {    //skeleton output file
                        if(importer.m_importFiles.back().m_skelOutFile) {
                            LOG_INFO("Warning: skeleton file output %s already specified for %s",
                                importer.m_importFiles.back().m_skelOutFile);
                        }
                        else if(importer.m_importFiles.back().m_skelInFile) {
                            LOG_INFO("Warning: skeleton file input %s already specified for %s, forcing no input and doing an output",
                                importer.m_importFiles.back().m_skelInFile);
                        }

                        if(arg >= argc) {
                            LOG_FATAL_ERROR("Expecting a file name after the -skel parameter");
                        }
                    
                        importer.m_importFiles.back().m_skelOutFile = argv[arg++];
                        importer.m_importFiles.back().m_skelInFile = NULL;

                        LOG_INFO("Exporting skeleton file with name %s", importer.m_importFiles.back().m_skelOutFile);
                    }
                    else if(strncmp(currArg, "-skelin", 10) == 0) {    //skeleton input file
                        if(mainSkeletonImportSet) {
                            LOG_INFO("Warning: main skeleton file was already specified for %s, overriding and using this one",
                                importer.m_importFiles.at(importer.m_mainSkeletonImport).m_importFile);
                        }

                        if(importer.m_importFiles.back().m_skelInFile) {
                            LOG_INFO("Warning: skeleton file input %s already specified for %s",
                                importer.m_importFiles.back().m_skelInFile);
                        }
                        else if(importer.m_importFiles.back().m_skelOutFile) {
                            LOG_INFO("Warning: skeleton file output %s already specified for %s, forcing no output and doing an input",
                                importer.m_importFiles.back().m_skelOutFile);
                        }
                    
                        if(arg >= argc) {
                            LOG_FATAL_ERROR("Expecting a file name after the -skelin parameter");
                        }
                    
                        importer.m_importFiles.back().m_skelOutFile = NULL;
                        importer.m_importFiles.back().m_skelInFile = argv[arg++];

                        if(!illFileSystem::fileSystem->fileExists(importer.m_importFiles.back().m_skelInFile)) {
                            LOG_FATAL_ERROR("Skeleton file %s doesn't exist for import", importer.m_importFiles.back().m_skelInFile);
                        }

                        LOG_INFO("Provided existing skeleton %s, importing and using", importer.m_importFiles.back().m_skelInFile);

                        importer.m_mainSkeletonImport = importer.m_importFiles.size() - 1;
                        mainSkeletonImportSet = true;
                    }
                    //TODO: add an arg for remapping coord systems
                    else {
                        //go back to main state
                        state = State::MAIN;
                        arg--;
                    }

                    } break;
                }
            }
        }
    
        //compute animset
        if(asetFile) {
            if(illFileSystem::fileSystem->fileExists(asetFile)) {
                LOG_INFO("Provided existing animset %s, importing and using", asetFile);
                importer.m_animSet.load(asetFile);
            }
            else {
                LOG_INFO("Exporting animset file with name %s", asetFile);
            }
        }
        else {
            LOG_FATAL_ERROR("No animset input or output provided.  Use the -anset flag.");
        }

        //compute bone data for all scenes being imported
        importer.computeBones();

        //import existing skeletons for animation reference
        for(auto iter = importer.m_importFiles.begin(); iter != importer.m_importFiles.end(); iter++) {
            if(iter->m_skelInFile) {
                if(importer.m_animSet.m_creating) {
                    LOG_FATAL_ERROR("Attempting to import a skeleton without also importing an animset.  Generate an animset first so bone indices are consistent.");
                }

                iter->m_skeletonOut = new Skeleton();
                iter->m_skeletonOut->load(iter->m_skelInFile, iter->m_scene);
            }
        }

        //do the imports of all the scenes for real now, creating the meshes, skeletons, animations
        importer.doImports();

        //for each imported file, do the corresponding exports
        if(importer.m_animSet.m_creating) {
            importer.m_animSet.save(asetFile);
        }

        for(auto iter = importer.m_importFiles.begin(); iter != importer.m_importFiles.end(); iter++) {
            if(iter->m_skelOutFile) {
                iter->m_skeletonOut->save(importer.computeSkeletonFileName(iter->m_skeletonOut, iter->m_skelOutFile).c_str(), &importer.m_animSet);
            }

            if(iter->m_meshOutFile) {
                MeshMerger merger;

                if(iter->m_mergeMesh) {
                    merger.m_exportPath = iter->m_meshOutFile;
                }
                
                for(auto saveIter = iter->m_meshOut.cbegin(); saveIter != iter->m_meshOut.end(); saveIter++) {
                    std::string computedMeshName = importer.computeMeshFileName(*saveIter, iter->m_scene, iter->m_meshOutFile);

                    (*saveIter)->save(computedMeshName.c_str());

                    if(iter->m_mergeMesh) {
                        merger.m_paths.push_back(computedMeshName);
                    }
                }

                if(iter->m_mergeMesh) {
                    merger.merge();

                    //delete the other files that were generated
                    for(auto delIter = merger.m_paths.begin(); delIter != merger.m_paths.end(); delIter++) {
                        if(delIter->compare(merger.m_exportPath) != 0) {
                            remove(delIter->c_str());
                        }
                    }
                }
            }

            if(iter->m_animOutFile) {
                for(auto saveIter = iter->m_animationOut.cbegin(); saveIter != iter->m_animationOut.end(); saveIter++) {
                    (*saveIter)->save(importer.computeAnimationFileName(*saveIter, iter->m_animOutFile).c_str());
                }
            }
        }
    }
    catch (...) {
        return 1;
    }

    return 0;
}