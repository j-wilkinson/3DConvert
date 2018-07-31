// © You i Labs Inc. 2000-2016. All rights reserved.
//#include "vld.h"

#include "windows.h"
#include "C3DModelAssimp.h"
#include "C3DModelFBX.h"
#include "C3DModelOBJ.h"

//Command line parsing code
int opt = 0;
char* optarg = NULL;
int optind = 1;
bool bForceAssimp = false;

int getopt(int argc, char *const argv[], const char *optstring)
{
    if ((optind >= argc) || (argv[optind][0] != '-') || (argv[optind][0] == 0))
    {
        return -1;
    }

    int opt = argv[optind][1];
    const char *p = strchr(optstring, opt);

    if (p == NULL)
    {
        return '?';
    }
    //if (p[1] == ':')
    {
        optind++;
        if (optind >= argc)
        {
            return '?';
        }
        optarg = argv[optind];
    }
    return opt;
}

uint64_t YiGetTimeuS(void)
{
    LARGE_INTEGER li;

    static LONGLONG frequency = 0;
    if (frequency == 0)
    {
        if (!QueryPerformanceFrequency(&li))
        {
            // This should never fail, but we can set a default value so we don't come back in here
            frequency = 1;
        }
        frequency = li.QuadPart;
    }

    QueryPerformanceCounter(&li);
    static LONGLONG counterStart = li.QuadPart;

    return (uint64_t)((li.QuadPart - counterStart) * 1000000 / frequency);
}

void ConvertModel(std::string sName, bool bFlipUV)
{
    // measure the time before the update
    uint64_t uBeforeUpdateTimeuS = YiGetTimeuS();

    std::string sFile = sName;
    std::transform(sName.begin(), sName.end(), sName.begin(), ::tolower);

    if (bForceAssimp)
    {
        printf("Using ASSIMP Importer for conversion.\n");
        C3DModelAssimp * pModelViewAssimp = new C3DModelAssimp();
        pModelViewAssimp->ExportToSTUFormat(sFile, bFlipUV);
        delete pModelViewAssimp;
    }
    else
    {
        std::string sfbx = ".fbx";
        size_t start_pos = sName.find(sfbx);
        if (start_pos != std::string::npos)
        {
            printf("Using FBX SDK for conversion.\n");
            C3DModelFBX * pModelViewFBX = new C3DModelFBX();
            pModelViewFBX->ExportToSTUFormat(sFile, bFlipUV);
            delete pModelViewFBX;
        }
        else
        {
            std::string sfbx = ".obj";
            start_pos = sName.find(sfbx);
            if (start_pos != std::string::npos)
            {
                printf("Using OBJ Importer for conversion.\n");
                C3DModelOBJ * pModelViewOBJ = new C3DModelOBJ();
                pModelViewOBJ->ExportToSTUFormat(sFile, bFlipUV);
                delete pModelViewOBJ;
            }
            else
            {
                printf("Using ASSIMP Importer for conversion.\n");
                C3DModelAssimp * pModelViewAssimp = new C3DModelAssimp();
                pModelViewAssimp->ExportToSTUFormat(sFile, bFlipUV);
                delete pModelViewAssimp;
            }
        }
    }
    // calculate the total time consumed by the update call by measuring the time after the update
    uint64_t uConsumedTimeuS = YiGetTimeuS() - uBeforeUpdateTimeuS;

    printf("Time taken to load: %0.02f", uConsumedTimeuS / 1000000.0f);
}

void PrintInfo()
{
    printf("\n3DConvert converts standard model formats to the You.i Engine format (.stu).\n");
    printf("\n    Usage: Simple3DTestApp -a -f Modelfile [ -f Modelfile]...");
    printf("\n    -a  Force Assimp for convert (instead of Autodesk FBX etc)\n\n\n");
}

void ProcessCommandArgs(int argc, char ** argv)
{
    int processed = 0;
    if (argc > 1)
    {
        while ((opt = getopt(argc, argv, "af:")) != -1)
        {
            switch (opt)
            {
            case 'f':
            {
                ConvertModel(optarg, true);
                processed++;
                break;
            }
            case 'a':
            {
                bForceAssimp = true;
                break;
            }
            case '?':
                printf("\n3DConvert converts standard model formats to the You.i Engine format (.stu).\n");
                printf("\n    Usage: Simple3DTestApp -a -f Modelfile [ -f Modelfile]...");
                printf("\n    -a  Force Assimp for converting FBX or OBJ (instead of Autodesk FBX etc)\n\n\n");
                break;
            default:
                PrintInfo();
                break;
            }
        }
    }
    else
    {
        PrintInfo();
    }
    if (processed == 0)
    {
        PrintInfo();
    }
}

bool main(int argc, char **argv)
{
    ProcessCommandArgs(argc, argv);
    return 0;
}
