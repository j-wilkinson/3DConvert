#include "FBXHelper.h"

#ifdef IOS_REF
#undef  IOS_REF
#define IOS_REF (*(pManager->GetIOSettings()))
#endif

#define MAT_HEADER_LENGTH 200


int InterpolationFlagToIndex(int flags)
{
    if ((flags & FbxAnimCurveDef::eInterpolationConstant) == FbxAnimCurveDef::eInterpolationConstant) return 1;
    if ((flags & FbxAnimCurveDef::eInterpolationLinear) == FbxAnimCurveDef::eInterpolationLinear) return 2;
    if ((flags & FbxAnimCurveDef::eInterpolationCubic) == FbxAnimCurveDef::eInterpolationCubic) return 3;
    return 0;
}

int ConstantmodeFlagToIndex(int flags)
{
    if ((flags & FbxAnimCurveDef::eConstantStandard) == FbxAnimCurveDef::eConstantStandard) return 1;
    if ((flags & FbxAnimCurveDef::eConstantNext) == FbxAnimCurveDef::eConstantNext) return 2;
    return 0;
}

int TangentmodeFlagToIndex(int flags)
{
    if ((flags & FbxAnimCurveDef::eTangentAuto) == FbxAnimCurveDef::eTangentAuto) return 1;
    if ((flags & FbxAnimCurveDef::eTangentAutoBreak) == FbxAnimCurveDef::eTangentAutoBreak) return 2;
    if ((flags & FbxAnimCurveDef::eTangentTCB) == FbxAnimCurveDef::eTangentTCB) return 3;
    if ((flags & FbxAnimCurveDef::eTangentUser) == FbxAnimCurveDef::eTangentUser) return 4;
    if ((flags & FbxAnimCurveDef::eTangentGenericBreak) == FbxAnimCurveDef::eTangentGenericBreak) return 5;
    if ((flags & FbxAnimCurveDef::eTangentBreak) == FbxAnimCurveDef::eTangentBreak) return 6;
    return 0;
}

int TangentweightFlagToIndex(int flags)
{
    if ((flags & FbxAnimCurveDef::eWeightedNone) == FbxAnimCurveDef::eWeightedNone) return 1;
    if ((flags & FbxAnimCurveDef::eWeightedRight) == FbxAnimCurveDef::eWeightedRight) return 2;
    if ((flags & FbxAnimCurveDef::eWeightedNextLeft) == FbxAnimCurveDef::eWeightedNextLeft) return 3;
    return 0;
}

int TangentVelocityFlagToIndex(int flags)
{
    if ((flags & FbxAnimCurveDef::eVelocityNone) == FbxAnimCurveDef::eVelocityNone) return 1;
    if ((flags & FbxAnimCurveDef::eVelocityRight) == FbxAnimCurveDef::eVelocityRight) return 2;
    if ((flags & FbxAnimCurveDef::eVelocityNextLeft) == FbxAnimCurveDef::eVelocityNextLeft) return 3;
    return 0;
}

void InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene)
{
    //The first thing to do is to create the FBX Manager which is the object allocator for almost all the classes in the SDK
    pManager = FbxManager::Create();
    if( !pManager )
    {
        FBXSDK_printf("Error: Unable to create FBX Manager!\n");
        exit(1);
    }
    else FBXSDK_printf("Autodesk FBX SDK version %s\n", pManager->GetVersion());

    //Create an IOSettings object. This object holds all import/export settings.
    FbxIOSettings* ios = FbxIOSettings::Create(pManager, IOSROOT);
    pManager->SetIOSettings(ios);

    //Load plugins from the executable directory (optional)
    FbxString lPath = FbxGetApplicationDirectory();
    pManager->LoadPluginsDirectory(lPath.Buffer());

    //Create an FBX scene. This object holds most objects imported/exported from/to files.
    pScene = FbxScene::Create(pManager, "My Scene");
    if( !pScene )
    {
        FBXSDK_printf("Error: Unable to create FBX scene!\n");
        exit(1);
    }
}

void DestroySdkObjects(FbxManager* pManager, bool pExitStatus)
{
    //Delete the FBX Manager. All the objects that have been allocated using the FBX Manager and that haven't been explicitly destroyed are also automatically destroyed.
    if( pManager ) pManager->Destroy();
    if( pExitStatus ) FBXSDK_printf("\nFBX SDK Exiting.\n");
}

bool SaveScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename, int pFileFormat, bool pEmbedMedia)
{
    int lMajor, lMinor, lRevision;
    bool lStatus = true;

    // Create an exporter.
    FbxExporter* lExporter = FbxExporter::Create(pManager, "");

    if( pFileFormat < 0 || pFileFormat >= pManager->GetIOPluginRegistry()->GetWriterFormatCount() )
    {
        // Write in fall back format in less no ASCII format found
        pFileFormat = pManager->GetIOPluginRegistry()->GetNativeWriterFormat();

        //Try to export in ASCII if possible
        int lFormatIndex, lFormatCount = pManager->GetIOPluginRegistry()->GetWriterFormatCount();

        for (lFormatIndex=0; lFormatIndex<lFormatCount; lFormatIndex++)
        {
            if (pManager->GetIOPluginRegistry()->WriterIsFBX(lFormatIndex))
            {
                FbxString lDesc =pManager->GetIOPluginRegistry()->GetWriterFormatDescription(lFormatIndex);
                const char *lASCII = "ascii";
                if (lDesc.Find(lASCII)>=0)
                {
                    pFileFormat = lFormatIndex;
                    break;
                }
            }
        }
    }

    // Set the export states. By default, the export states are always set to
    // true except for the option eEXPORT_TEXTURE_AS_EMBEDDED. The code below
    // shows how to change these states.
    IOS_REF.SetBoolProp(EXP_FBX_MATERIAL,        true);
    IOS_REF.SetBoolProp(EXP_FBX_TEXTURE,         true);
    IOS_REF.SetBoolProp(EXP_FBX_EMBEDDED,        pEmbedMedia);
    IOS_REF.SetBoolProp(EXP_FBX_SHAPE,           true);
    IOS_REF.SetBoolProp(EXP_FBX_GOBO,            true);
    IOS_REF.SetBoolProp(EXP_FBX_ANIMATION,       true);
    IOS_REF.SetBoolProp(EXP_FBX_GLOBAL_SETTINGS, true);

    // Initialize the exporter by providing a filename.
    if(lExporter->Initialize(pFilename, pFileFormat, pManager->GetIOSettings()) == false)
    {
        FBXSDK_printf("Call to FbxExporter::Initialize() failed.\n");
        FBXSDK_printf("Error returned: %s\n\n", lExporter->GetStatus().GetErrorString());
        return false;
    }

    FbxManager::GetFileFormatVersion(lMajor, lMinor, lRevision);
    FBXSDK_printf("FBX file format version %d.%d.%d\n\n", lMajor, lMinor, lRevision);

    // Export the scene.
    lStatus = lExporter->Export(pScene);

    // Destroy the exporter.
    lExporter->Destroy();
    return lStatus;
}

void DisplayCurveKeys(FbxAnimCurve* pCurve)
{
    static const char* interpolation[] = { "?", "constant", "linear", "cubic"};
    static const char* constantMode[] =  { "?", "Standard", "Next" };
    static const char* cubicMode[] =     { "?", "Auto", "Auto break", "Tcb", "User", "Break", "User break" };
    static const char* tangentWVMode[] = { "?", "None", "Right", "Next left" };

    FbxTime   lKeyTime;
    float   lKeyValue;
    char    lTimeString[256];
    FbxString lOutputString;
    int     lCount;

    int lKeyCount = pCurve->KeyGetCount();

    for(lCount = 0; lCount < lKeyCount; lCount++)
    {
        lKeyValue = static_cast<float>(pCurve->KeyGetValue(lCount));
        lKeyTime  = pCurve->KeyGetTime(lCount);

        lOutputString = "            Key Time: ";
        lOutputString += lKeyTime.GetTimeString(lTimeString, FbxUShort(256));
        lOutputString += ".... Key Value: ";
        lOutputString += lKeyValue;
        lOutputString += " [ ";
        lOutputString += interpolation[ InterpolationFlagToIndex(pCurve->KeyGetInterpolation(lCount)) ];
        if ((pCurve->KeyGetInterpolation(lCount)&FbxAnimCurveDef::eInterpolationConstant) == FbxAnimCurveDef::eInterpolationConstant)
        {
            lOutputString += " | ";
            lOutputString += constantMode[ ConstantmodeFlagToIndex(pCurve->KeyGetConstantMode(lCount)) ];
        }
        else if ((pCurve->KeyGetInterpolation(lCount)&FbxAnimCurveDef::eInterpolationCubic) == FbxAnimCurveDef::eInterpolationCubic)
        {
            lOutputString += " | ";
            lOutputString += cubicMode[ TangentmodeFlagToIndex(pCurve->KeyGetTangentMode(lCount)) ];
            lOutputString += " | ";
            lOutputString += tangentWVMode[ TangentweightFlagToIndex(pCurve->KeyGet(lCount).GetTangentWeightMode()) ];
            lOutputString += " | ";
            lOutputString += tangentWVMode[ TangentVelocityFlagToIndex(pCurve->KeyGet(lCount).GetTangentVelocityMode()) ];
        }
        lOutputString += " ]";
        lOutputString += "\n";
        FBXSDK_printf (lOutputString);
    }
}

void DisplayListCurveKeys(FbxAnimCurve* pCurve, FbxProperty* pProperty)
{
    FbxTime   lKeyTime;
    int     lKeyValue;
    char    lTimeString[256];
    FbxString lListValue;
    FbxString lOutputString;
    int     lCount;

    int lKeyCount = pCurve->KeyGetCount();

    for(lCount = 0; lCount < lKeyCount; lCount++)
    {
        lKeyValue = static_cast<int>(pCurve->KeyGetValue(lCount));
        lKeyTime  = pCurve->KeyGetTime(lCount);

        lOutputString = "            Key Time: ";
        lOutputString += lKeyTime.GetTimeString(lTimeString, FbxUShort(256));
        lOutputString += ".... Key Value: ";
        lOutputString += lKeyValue;
        lOutputString += " (";
        lOutputString += pProperty->GetEnumValue(lKeyValue);
        lOutputString += ")";

        lOutputString += "\n";
        FBXSDK_printf (lOutputString);
    }
}

void DisplayMaterial(FbxGeometry* pGeometry)
{
    int lMaterialCount = 0;
    FbxNode* lNode = NULL;
    if(pGeometry){
        lNode = pGeometry->GetNode();
        if(lNode)
            lMaterialCount = lNode->GetMaterialCount();
    }

    if (lMaterialCount > 0)
    {
        FbxPropertyT<FbxDouble3> lKFbxDouble3;
        FbxPropertyT<FbxDouble> lKFbxDouble1;
        FbxColor theColor;

        for (int lCount = 0; lCount < lMaterialCount; lCount ++)
        {
            DisplayInt("        Material ", lCount);

            FbxSurfaceMaterial *lMaterial = lNode->GetMaterial(lCount);

            DisplayString("            Name: \"", (char *) lMaterial->GetName(), "\"");

            //Get the implementation to see if it's a hardware shader.
            const FbxImplementation* lImplementation = GetImplementation(lMaterial, FBXSDK_IMPLEMENTATION_HLSL);
            FbxString lImplemenationType = "HLSL";
            if(!lImplementation)
            {
                lImplementation = GetImplementation(lMaterial, FBXSDK_IMPLEMENTATION_CGFX);
                lImplemenationType = "CGFX";
            }
            if(lImplementation)
            {
                //Now we have a hardware shader, let's read it
                FBXSDK_printf("            Hardware Shader Type: %s\n", lImplemenationType.Buffer());
                const FbxBindingTable* lRootTable = lImplementation->GetRootTable();
                FbxString lFileName = lRootTable->DescAbsoluteURL.Get();
                FbxString lTechniqueName = lRootTable->DescTAG.Get();


                const FbxBindingTable* lTable = lImplementation->GetRootTable();
                size_t lEntryNum = lTable->GetEntryCount();

                for(int i=0;i <(int)lEntryNum; ++i)
                {
                    const FbxBindingTableEntry& lEntry = lTable->GetEntry(i);
                    const char* lEntrySrcType = lEntry.GetEntryType(true);
                    FbxProperty lFbxProp;


                    FbxString lTest = lEntry.GetSource();
                    FBXSDK_printf("            Entry: %s\n", lTest.Buffer());


                    if ( strcmp( FbxPropertyEntryView::sEntryType, lEntrySrcType ) == 0 )
                    {
                        lFbxProp = lMaterial->FindPropertyHierarchical(lEntry.GetSource());
                        if(!lFbxProp.IsValid())
                        {
                            lFbxProp = lMaterial->RootProperty.FindHierarchical(lEntry.GetSource());
                        }


                    }
                    else if( strcmp( FbxConstantEntryView::sEntryType, lEntrySrcType ) == 0 )
                    {
                        lFbxProp = lImplementation->GetConstants().FindHierarchical(lEntry.GetSource());
                    }
                    if(lFbxProp.IsValid())
                    {
                        if( lFbxProp.GetSrcObjectCount<FbxTexture>() > 0 )
                        {
                            //do what you want with the textures
                            for(int j=0; j<lFbxProp.GetSrcObjectCount<FbxFileTexture>(); ++j)
                            {
                                FbxFileTexture *lTex = lFbxProp.GetSrcObject<FbxFileTexture>(j);
                                FBXSDK_printf("           File Texture: %s\n", lTex->GetFileName());
                            }
                            for(int j=0; j<lFbxProp.GetSrcObjectCount<FbxLayeredTexture>(); ++j)
                            {
                                FbxLayeredTexture *lTex = lFbxProp.GetSrcObject<FbxLayeredTexture>(j);
                                FBXSDK_printf("        Layered Texture: %s\n", lTex->GetName());
                            }
                            for(int j=0; j<lFbxProp.GetSrcObjectCount<FbxProceduralTexture>(); ++j)
                            {
                                FbxProceduralTexture *lTex = lFbxProp.GetSrcObject<FbxProceduralTexture>(j);
                                FBXSDK_printf("     Procedural Texture: %s\n", lTex->GetName());
                            }
                        }
                        else
                        {
                            FbxDataType lFbxType = lFbxProp.GetPropertyDataType();
                            FbxString blah = lFbxType.GetName();
                            if(FbxBoolDT == lFbxType)
                            {
                                DisplayBool("                Bool: ", lFbxProp.Get<FbxBool>() );
                            }
                            else if ( FbxIntDT == lFbxType ||  FbxEnumDT  == lFbxType )
                            {
                                DisplayInt("                Int: ", lFbxProp.Get<FbxInt>());
                            }
                            else if ( FbxFloatDT == lFbxType)
                            {
                                DisplayDouble("                Float: ", lFbxProp.Get<FbxFloat>());

                            }
                            else if ( FbxDoubleDT == lFbxType)
                            {
                                DisplayDouble("                Double: ", lFbxProp.Get<FbxDouble>());
                            }
                            else if ( FbxStringDT == lFbxType
                                ||  FbxUrlDT  == lFbxType
                                ||  FbxXRefUrlDT  == lFbxType )
                            {
                                DisplayString("                String: ", lFbxProp.Get<FbxString>().Buffer());
                            }
                            else if ( FbxDouble2DT == lFbxType)
                            {
                                FbxDouble2 lDouble2 = lFbxProp.Get<FbxDouble2>();
                                FbxVector2 lVect;
                                lVect[0] = lDouble2[0];
                                lVect[1] = lDouble2[1];

                                Display2DVector("                2D vector: ", lVect);
                            }
                            else if ( FbxDouble3DT == lFbxType || FbxColor3DT == lFbxType)
                            {
                                FbxDouble3 lDouble3 = lFbxProp.Get<FbxDouble3>();


                                FbxVector4 lVect;
                                lVect[0] = lDouble3[0];
                                lVect[1] = lDouble3[1];
                                lVect[2] = lDouble3[2];
                                Display3DVector("                3D vector: ", lVect);
                            }

                            else if ( FbxDouble4DT == lFbxType || FbxColor4DT == lFbxType)
                            {
                                FbxDouble4 lDouble4 = lFbxProp.Get<FbxDouble4>();
                                FbxVector4 lVect;
                                lVect[0] = lDouble4[0];
                                lVect[1] = lDouble4[1];
                                lVect[2] = lDouble4[2];
                                lVect[3] = lDouble4[3];
                                Display4DVector("                4D vector: ", lVect);
                            }
                            else if ( FbxDouble4x4DT == lFbxType)
                            {
                                FbxDouble4x4 lDouble44 = lFbxProp.Get<FbxDouble4x4>();
                                for(int j=0; j<4; ++j)
                                {

                                    FbxVector4 lVect;
                                    lVect[0] = lDouble44[j][0];
                                    lVect[1] = lDouble44[j][1];
                                    lVect[2] = lDouble44[j][2];
                                    lVect[3] = lDouble44[j][3];
                                    Display4DVector("                4x4D vector: ", lVect);
                                }

                            }
                        }

                    }
                }
            }
            else if (lMaterial->GetClassId().Is(FbxSurfacePhong::ClassId))
            {
                // We found a Phong material.  Display its properties.

                // Display the Ambient Color
                lKFbxDouble3 =((FbxSurfacePhong *) lMaterial)->Ambient;
                theColor.Set(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
                DisplayColor("            Ambient: ", theColor);

                // Display the Diffuse Color
                lKFbxDouble3 =((FbxSurfacePhong *) lMaterial)->Diffuse;
                theColor.Set(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
                DisplayColor("            Diffuse: ", theColor);

                // Display the Specular Color (unique to Phong materials)
                lKFbxDouble3 =((FbxSurfacePhong *) lMaterial)->Specular;
                theColor.Set(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
                DisplayColor("            Specular: ", theColor);

                // Display the Emissive Color
                lKFbxDouble3 =((FbxSurfacePhong *) lMaterial)->Emissive;
                theColor.Set(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
                DisplayColor("            Emissive: ", theColor);

                //Opacity is Transparency factor now
                lKFbxDouble1 =((FbxSurfacePhong *) lMaterial)->TransparencyFactor;
                DisplayDouble("            Opacity: ", 1.0-lKFbxDouble1.Get());

                // Display the Shininess
                lKFbxDouble1 =((FbxSurfacePhong *) lMaterial)->Shininess;
                DisplayDouble("            Shininess: ", lKFbxDouble1.Get());

                // Display the Reflectivity
                lKFbxDouble1 =((FbxSurfacePhong *) lMaterial)->ReflectionFactor;
                DisplayDouble("            Reflectivity: ", lKFbxDouble1.Get());
            }
            else if(lMaterial->GetClassId().Is(FbxSurfaceLambert::ClassId) )
            {
                // We found a Lambert material. Display its properties.
                // Display the Ambient Color
                lKFbxDouble3=((FbxSurfaceLambert *)lMaterial)->Ambient;
                theColor.Set(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
                DisplayColor("            Ambient: ", theColor);

                // Display the Diffuse Color
                lKFbxDouble3 =((FbxSurfaceLambert *)lMaterial)->Diffuse;
                theColor.Set(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
                DisplayColor("            Diffuse: ", theColor);

                // Display the Emissive
                lKFbxDouble3 =((FbxSurfaceLambert *)lMaterial)->Emissive;
                theColor.Set(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
                DisplayColor("            Emissive: ", theColor);

                // Display the Opacity
                lKFbxDouble1 =((FbxSurfaceLambert *)lMaterial)->TransparencyFactor;
                DisplayDouble("            Opacity: ", 1.0-lKFbxDouble1.Get());
            }
            else
                DisplayString("Unknown type of Material");

            FbxPropertyT<FbxString> lString;
            lString = lMaterial->ShadingModel;
            DisplayString("            Shading Model: ", lString.Get().Buffer());
            DisplayString("");
        }
    }
}

void DisplayTextureInfo(FbxTexture* pTexture, int pBlendMode)
{
    FbxFileTexture *lFileTexture = FbxCast<FbxFileTexture>(pTexture);
    FbxProceduralTexture *lProceduralTexture = FbxCast<FbxProceduralTexture>(pTexture);

    DisplayString("            Name: \"", (char *) pTexture->GetName(), "\"");
    if (lFileTexture)
    {
        DisplayString("            Type: File Texture");
        DisplayString("            File Name: \"", (char *) lFileTexture->GetFileName(), "\"");
    }
    else if (lProceduralTexture)
    {
        DisplayString("            Type: Procedural Texture");
    }
    DisplayDouble("            Scale U: ", pTexture->GetScaleU());
    DisplayDouble("            Scale V: ", pTexture->GetScaleV());
    DisplayDouble("            Translation U: ", pTexture->GetTranslationU());
    DisplayDouble("            Translation V: ", pTexture->GetTranslationV());
    DisplayBool("            Swap UV: ", pTexture->GetSwapUV());
    DisplayDouble("            Rotation U: ", pTexture->GetRotationU());
    DisplayDouble("            Rotation V: ", pTexture->GetRotationV());
    DisplayDouble("            Rotation W: ", pTexture->GetRotationW());

    const char* lAlphaSources[] = { "None", "RGB Intensity", "Black" };

    DisplayString("            Alpha Source: ", lAlphaSources[pTexture->GetAlphaSource()]);
    DisplayDouble("            Cropping Left: ", pTexture->GetCroppingLeft());
    DisplayDouble("            Cropping Top: ", pTexture->GetCroppingTop());
    DisplayDouble("            Cropping Right: ", pTexture->GetCroppingRight());
    DisplayDouble("            Cropping Bottom: ", pTexture->GetCroppingBottom());

    const char* lMappingTypes[] = { "Null", "Planar", "Spherical", "Cylindrical",
        "Box", "Face", "UV", "Environment" };

    DisplayString("            Mapping Type: ", lMappingTypes[pTexture->GetMappingType()]);

    if (pTexture->GetMappingType() == FbxTexture::ePlanar)
    {
        const char* lPlanarMappingNormals[] = { "X", "Y", "Z" };

        DisplayString("            Planar Mapping Normal: ", lPlanarMappingNormals[pTexture->GetPlanarMappingNormal()]);
    }

    const char* lBlendModes[]   = { "Translucent", "Add", "Modulate", "Modulate2" };
    if(pBlendMode >= 0)
        DisplayString("            Blend Mode: ", lBlendModes[pBlendMode]);
    DisplayDouble("            Alpha: ", pTexture->GetDefaultAlpha());

    if (lFileTexture)
    {
        const char* lMaterialUses[] = { "Model Material", "Default Material" };
        DisplayString("            Material Use: ", lMaterialUses[lFileTexture->GetMaterialUse()]);
    }

    const char* pTextureUses[] = { "Standard", "Shadow Map", "Light Map",
        "Spherical Reflexion Map", "Sphere Reflexion Map", "Bump Normal Map" };

    DisplayString("            Texture Use: ", pTextureUses[pTexture->GetTextureUse()]);
    DisplayString("");

}

void FindAndDisplayTextureInfoByProperty(FbxProperty pProperty, bool& pDisplayHeader, int pMaterialIndex){

    if( pProperty.IsValid() )
    {
        int lTextureCount = pProperty.GetSrcObjectCount<FbxTexture>();

        for (int j = 0; j < lTextureCount; ++j)
        {
            //Here we have to check if it's layeredtextures, or just textures:
            FbxLayeredTexture *lLayeredTexture = pProperty.GetSrcObject<FbxLayeredTexture>(j);
            if (lLayeredTexture)
            {
                DisplayInt("    Layered Texture: ", j);
                int lNbTextures = lLayeredTexture->GetSrcObjectCount<FbxTexture>();
                for(int k =0; k<lNbTextures; ++k)
                {
                    FbxTexture* lTexture = lLayeredTexture->GetSrcObject<FbxTexture>(k);
                    if(lTexture)
                    {

                        if(pDisplayHeader){
                            DisplayInt("    Textures connected to Material ", pMaterialIndex);
                            pDisplayHeader = false;
                        }

                        //NOTE the blend mode is ALWAYS on the LayeredTexture and NOT the one on the texture.
                        //Why is that?  because one texture can be shared on different layered textures and might
                        //have different blend modes.

                        FbxLayeredTexture::EBlendMode lBlendMode;
                        lLayeredTexture->GetTextureBlendMode(k, lBlendMode);
                        DisplayString("    Textures for ", pProperty.GetName());
                        DisplayInt("        Texture ", k);
                        DisplayTextureInfo(lTexture, (int) lBlendMode);
                    }

                }
            }
            else
            {
                //no layered texture simply get on the property
                FbxTexture* lTexture = pProperty.GetSrcObject<FbxTexture>(j);
                if(lTexture)
                {
                    //display connected Material header only at the first time
                    if(pDisplayHeader){
                        DisplayInt("    Textures connected to Material ", pMaterialIndex);
                        pDisplayHeader = false;
                    }

                    DisplayString("    Textures for ", pProperty.GetName());
                    DisplayInt("        Texture ", j);
                    DisplayTextureInfo(lTexture, -1);
                }
            }
        }
    }//end if pProperty
}


void DisplayTexture(FbxGeometry* pGeometry)
{
    int lMaterialIndex;
    FbxProperty lProperty;
    if (pGeometry->GetNode() == NULL)
        return;
    int lNbMat = pGeometry->GetNode()->GetSrcObjectCount<FbxSurfaceMaterial>();
    for (lMaterialIndex = 0; lMaterialIndex < lNbMat; lMaterialIndex++){
        FbxSurfaceMaterial *lMaterial = pGeometry->GetNode()->GetSrcObject<FbxSurfaceMaterial>(lMaterialIndex);
        bool lDisplayHeader = true;

        //go through all the possible textures
        if (lMaterial){

            int lTextureIndex;
            FBXSDK_FOR_EACH_TEXTURE(lTextureIndex)
            {
                lProperty = lMaterial->FindProperty(FbxLayerElement::sTextureChannelNames[lTextureIndex]);
                FindAndDisplayTextureInfoByProperty(lProperty, lDisplayHeader, lMaterialIndex);
            }

        }//end if(lMaterial)

    }// end for lMaterialIndex
}

void DisplayCache(FbxGeometry* pGeometry)
{
    int lVertexCacheDeformerCount = pGeometry->GetDeformerCount( FbxDeformer::eVertexCache );

    for( int i = 0; i < lVertexCacheDeformerCount; ++i )
    {
        FbxVertexCacheDeformer* lDeformer = static_cast<FbxVertexCacheDeformer*>(pGeometry->GetDeformer(i, FbxDeformer::eVertexCache));
        if( !lDeformer ) continue;

        FbxCache* lCache = lDeformer->GetCache();
        if( !lCache ) continue;

        if (lCache->OpenFileForRead())
        {
            DisplayString("    Vertex Cache");
            int lChannelIndex = lCache->GetChannelIndex(lDeformer->Channel.Get());
            // skip normal channel
            if (lChannelIndex < 0)
                continue;

            FbxString lChnlName, lChnlInterp;

            FbxCache::EMCDataType lChnlType;
            FbxTime start, stop, rate;
            FbxCache::EMCSamplingType lChnlSampling;
            unsigned int lChnlSampleCount, lDataCount;

            lCache->GetChannelName(lChannelIndex, lChnlName);
            DisplayString("        Channel Name: ", lChnlName.Buffer());
            lCache->GetChannelDataType(lChannelIndex, lChnlType);
            switch (lChnlType)
            {
            case FbxCache::eUnknownData:
                DisplayString("        Channel Type: Unknown Data"); break;
            case FbxCache::eDouble:
                DisplayString("        Channel Type: Double"); break;
            case FbxCache::eDoubleArray:
                DisplayString("        Channel Type: Double Array"); break;
            case FbxCache::eDoubleVectorArray:
                DisplayString("        Channel Type: Double Vector Array"); break;
            case FbxCache::eInt32Array:
                DisplayString("        Channel Type: Int32 Array"); break;
            case FbxCache::eFloatArray:
                DisplayString("        Channel Type: Float Array"); break;
            case FbxCache::eFloatVectorArray:
                DisplayString("        Channel Type: Float Vector Array"); break;
            }
            lCache->GetChannelInterpretation(lChannelIndex, lChnlInterp);
            DisplayString("        Channel Interpretation: ", lChnlInterp.Buffer());
            lCache->GetChannelSamplingType(lChannelIndex, lChnlSampling);
            DisplayInt("        Channel Sampling Type: ", lChnlSampling);
            lCache->GetAnimationRange(lChannelIndex, start, stop);
            lCache->GetChannelSamplingRate(lChannelIndex, rate);
            lCache->GetChannelSampleCount(lChannelIndex, lChnlSampleCount);
            DisplayInt("        Channel Sample Count: ", lChnlSampleCount);

            // Only display cache data if the data type is float vector array
            if (lChnlType != FbxCache::eFloatVectorArray)
                continue;

            if (lChnlInterp == "normals")
                DisplayString("        Normal Cache Data");
            else
                DisplayString("        Points Cache Data");
            float* lBuffer = NULL;
            unsigned int lBufferSize = 0;
            int lFrame = 0;
            for (FbxTime t = start; t <= stop; t+=rate)
            {
                DisplayInt("            Frame ", lFrame);
                lCache->GetChannelPointCount(lChannelIndex, t, lDataCount);
                if (lBuffer == NULL)
                {
                    lBuffer = new float[lDataCount*3];
                    lBufferSize = lDataCount*3;
                }
                else if (lBufferSize < lDataCount*3)
                {
                    delete [] lBuffer;
                    lBuffer = new float[lDataCount*3];
                    lBufferSize = lDataCount*3;
                }
                else
                    memset(lBuffer, 0, lBufferSize*sizeof(float));

                lCache->Read(lChannelIndex, t, lBuffer, lDataCount);
                if (lChnlInterp == "normals")
                {
                    // display normals cache data
                    // the normal data is per-polygon per-vertex. we can get the polygon vertex index
                    // from the index array of polygon vertex
                    FbxMesh* lMesh = (FbxMesh*)pGeometry;

                    if (lMesh == NULL)
                    {
                        // Only Mesh can have normal cache data
                        continue;
                    }

                    DisplayInt("                Normal Count ", lDataCount);
                    int pi, j, lPolygonCount = lMesh->GetPolygonCount();
                    unsigned lNormalIndex = 0;
                    for (pi = 0; pi < lPolygonCount && lNormalIndex+2 < lDataCount*3; pi++)
                    {
                        DisplayInt("                    Polygon ", pi);
                        DisplayString("                    Normals for Each Polygon Vertex: ");
                        int lPolygonSize = lMesh->GetPolygonSize(pi);
                        for (j = 0; j < lPolygonSize && lNormalIndex+2 < lDataCount*3; j++)
                        {
                            FbxVector4 normal(lBuffer[lNormalIndex], lBuffer[lNormalIndex+1], lBuffer[lNormalIndex+2]);
                            Display3DVector("                       Normal Cache Data  ", normal);
                            lNormalIndex += 3;
                        }
                    }
                }
                else
                {
                    DisplayInt("               Points Count: ", lDataCount);
                    for (unsigned int j = 0; j < lDataCount*3; j=j+3)
                    {
                        FbxVector4 points(lBuffer[j], lBuffer[j+1], lBuffer[j+2]);
                        Display3DVector("                   Points Cache Data: ", points);
                    }
                }

                lFrame++;
            }

            if (lBuffer != NULL)
            {
                delete [] lBuffer;
                lBuffer = NULL;
            }

            lCache->CloseFile();
        }
    }
}

void DisplayLink(FbxGeometry* pGeometry)
{
    //Display cluster now

    //int i, lLinkCount;
    //FbxCluster* lLink;

    int i, j;
    int lSkinCount=0;
    int lClusterCount=0;
    FbxCluster* lCluster;

    lSkinCount=pGeometry->GetDeformerCount(FbxDeformer::eSkin);



    //lLinkCount = pGeometry->GetLinkCount();
    for(i=0; i!=lSkinCount; ++i)
    {
        lClusterCount = ((FbxSkin *) pGeometry->GetDeformer(i, FbxDeformer::eSkin))->GetClusterCount();
        for (j = 0; j != lClusterCount; ++j)
        {
            DisplayInt("    Cluster ", i);

            lCluster=((FbxSkin *) pGeometry->GetDeformer(i, FbxDeformer::eSkin))->GetCluster(j);
            //lLink = pGeometry->GetLink(i);

            const char* lClusterModes[] = { "Normalize", "Additive", "Total1" };

            DisplayString("    Mode: ", lClusterModes[lCluster->GetLinkMode()]);

            if(lCluster->GetLink() != NULL)
            {
                DisplayString("        Name: ", (char *) lCluster->GetLink()->GetName());
            }

            FbxString lString1 = "        Link Indices: ";
            FbxString lString2 = "        Weight Values: ";

            int k, lIndexCount = lCluster->GetControlPointIndicesCount();
            int* lIndices = lCluster->GetControlPointIndices();
            double* lWeights = lCluster->GetControlPointWeights();

            for(k = 0; k < lIndexCount; k++)
            {
                lString1 += lIndices[k];
                lString2 += (float) lWeights[k];

                if (k < lIndexCount - 1)
                {
                    lString1 += ", ";
                    lString2 += ", ";
                }
            }

            lString1 += "\n";
            lString2 += "\n";

            FBXSDK_printf(lString1);
            FBXSDK_printf(lString2);

            DisplayString("");

            FbxAMatrix lMatrix;

            lMatrix = lCluster->GetTransformMatrix(lMatrix);
            Display3DVector("        Transform Translation: ", lMatrix.GetT());
            Display3DVector("        Transform Rotation: ", lMatrix.GetR());
            Display3DVector("        Transform Scaling: ", lMatrix.GetS());

            lMatrix = lCluster->GetTransformLinkMatrix(lMatrix);
            Display3DVector("        Transform Link Translation: ", lMatrix.GetT());
            Display3DVector("        Transform Link Rotation: ", lMatrix.GetR());
            Display3DVector("        Transform Link Scaling: ", lMatrix.GetS());

            if (lCluster->GetAssociateModel() != NULL)
            {
                lMatrix = lCluster->GetTransformAssociateModelMatrix(lMatrix);
                DisplayString("        Associate Model: ", (char *) lCluster->GetAssociateModel()->GetName());
                Display3DVector("        Associate Model Translation: ", lMatrix.GetT());
                Display3DVector("        Associate Model Rotation: ", lMatrix.GetR());
                Display3DVector("        Associate Model Scaling: ", lMatrix.GetS());
            }

            DisplayString("");
        }
    }
}


void DisplayPivotsAndLimits(FbxNode* pNode)
{
    FbxVector4 lTmpVector;

    //
    // Pivots
    //
    FBXSDK_printf("    Pivot Information\n");

    FbxNode::EPivotState lPivotState;
    pNode->GetPivotState(FbxNode::eSourcePivot, lPivotState);
    FBXSDK_printf("        Pivot State: %s\n", lPivotState == FbxNode::ePivotActive ? "Active" : "Reference");

    lTmpVector = pNode->GetPreRotation(FbxNode::eSourcePivot);
    FBXSDK_printf("        Pre-Rotation: %f %f %f\n", lTmpVector[0], lTmpVector[1], lTmpVector[2]);

    lTmpVector = pNode->GetPostRotation(FbxNode::eSourcePivot);
    FBXSDK_printf("        Post-Rotation: %f %f %f\n", lTmpVector[0], lTmpVector[1], lTmpVector[2]);

    lTmpVector = pNode->GetRotationPivot(FbxNode::eSourcePivot);
    FBXSDK_printf("        Rotation Pivot: %f %f %f\n", lTmpVector[0], lTmpVector[1], lTmpVector[2]);

    lTmpVector = pNode->GetRotationOffset(FbxNode::eSourcePivot);
    FBXSDK_printf("        Rotation Offset: %f %f %f\n", lTmpVector[0], lTmpVector[1], lTmpVector[2]);

    lTmpVector = pNode->GetScalingPivot(FbxNode::eSourcePivot);
    FBXSDK_printf("        Scaling Pivot: %f %f %f\n", lTmpVector[0], lTmpVector[1], lTmpVector[2]);

    lTmpVector = pNode->GetScalingOffset(FbxNode::eSourcePivot);
    FBXSDK_printf("        Scaling Offset: %f %f %f\n", lTmpVector[0], lTmpVector[1], lTmpVector[2]);

    //
    // Limits
    //
    bool		lIsActive, lMinXActive, lMinYActive, lMinZActive;
    bool		lMaxXActive, lMaxYActive, lMaxZActive;
    FbxDouble3	lMinValues, lMaxValues;

    FBXSDK_printf("    Limits Information\n");

    lIsActive = pNode->TranslationActive;
    lMinXActive = pNode->TranslationMinX;
    lMinYActive = pNode->TranslationMinY;
    lMinZActive = pNode->TranslationMinZ;
    lMaxXActive = pNode->TranslationMaxX;
    lMaxYActive = pNode->TranslationMaxY;
    lMaxZActive = pNode->TranslationMaxZ;
    lMinValues = pNode->TranslationMin;
    lMaxValues = pNode->TranslationMax;

    FBXSDK_printf("        Translation limits: %s\n", lIsActive ? "Active" : "Inactive");
    FBXSDK_printf("            X\n");
    FBXSDK_printf("                Min Limit: %s\n", lMinXActive ? "Active" : "Inactive");
    FBXSDK_printf("                Min Limit Value: %f\n", lMinValues[0]);
    FBXSDK_printf("                Max Limit: %s\n", lMaxXActive ? "Active" : "Inactive");
    FBXSDK_printf("                Max Limit Value: %f\n", lMaxValues[0]);
    FBXSDK_printf("            Y\n");
    FBXSDK_printf("                Min Limit: %s\n", lMinYActive ? "Active" : "Inactive");
    FBXSDK_printf("                Min Limit Value: %f\n", lMinValues[1]);
    FBXSDK_printf("                Max Limit: %s\n", lMaxYActive ? "Active" : "Inactive");
    FBXSDK_printf("                Max Limit Value: %f\n", lMaxValues[1]);
    FBXSDK_printf("            Z\n");
    FBXSDK_printf("                Min Limit: %s\n", lMinZActive ? "Active" : "Inactive");
    FBXSDK_printf("                Min Limit Value: %f\n", lMinValues[2]);
    FBXSDK_printf("                Max Limit: %s\n", lMaxZActive ? "Active" : "Inactive");
    FBXSDK_printf("                Max Limit Value: %f\n", lMaxValues[2]);

    lIsActive = pNode->RotationActive;
    lMinXActive = pNode->RotationMinX;
    lMinYActive = pNode->RotationMinY;
    lMinZActive = pNode->RotationMinZ;
    lMaxXActive = pNode->RotationMaxX;
    lMaxYActive = pNode->RotationMaxY;
    lMaxZActive = pNode->RotationMaxZ;
    lMinValues = pNode->RotationMin;
    lMaxValues = pNode->RotationMax;

    FBXSDK_printf("        Rotation limits: %s\n", lIsActive ? "Active" : "Inactive");
    FBXSDK_printf("            X\n");
    FBXSDK_printf("                Min Limit: %s\n", lMinXActive ? "Active" : "Inactive");
    FBXSDK_printf("                Min Limit Value: %f\n", lMinValues[0]);
    FBXSDK_printf("                Max Limit: %s\n", lMaxXActive ? "Active" : "Inactive");
    FBXSDK_printf("                Max Limit Value: %f\n", lMaxValues[0]);
    FBXSDK_printf("            Y\n");
    FBXSDK_printf("                Min Limit: %s\n", lMinYActive ? "Active" : "Inactive");
    FBXSDK_printf("                Min Limit Value: %f\n", lMinValues[1]);
    FBXSDK_printf("                Max Limit: %s\n", lMaxYActive ? "Active" : "Inactive");
    FBXSDK_printf("                Max Limit Value: %f\n", lMaxValues[1]);
    FBXSDK_printf("            Z\n");
    FBXSDK_printf("                Min Limit: %s\n", lMinZActive ? "Active" : "Inactive");
    FBXSDK_printf("                Min Limit Value: %f\n", lMinValues[2]);
    FBXSDK_printf("                Max Limit: %s\n", lMaxZActive ? "Active" : "Inactive");
    FBXSDK_printf("                Max Limit Value: %f\n", lMaxValues[2]);

    lIsActive = pNode->ScalingActive;
    lMinXActive = pNode->ScalingMinX;
    lMinYActive = pNode->ScalingMinY;
    lMinZActive = pNode->ScalingMinZ;
    lMaxXActive = pNode->ScalingMaxX;
    lMaxYActive = pNode->ScalingMaxY;
    lMaxZActive = pNode->ScalingMaxZ;
    lMinValues = pNode->ScalingMin;
    lMaxValues = pNode->ScalingMax;

    FBXSDK_printf("        Scaling limits: %s\n", lIsActive ? "Active" : "Inactive");
    FBXSDK_printf("            X\n");
    FBXSDK_printf("                Min Limit: %s\n", lMinXActive ? "Active" : "Inactive");
    FBXSDK_printf("                Min Limit Value: %f\n", lMinValues[0]);
    FBXSDK_printf("                Max Limit: %s\n", lMaxXActive ? "Active" : "Inactive");
    FBXSDK_printf("                Max Limit Value: %f\n", lMaxValues[0]);
    FBXSDK_printf("            Y\n");
    FBXSDK_printf("                Min Limit: %s\n", lMinYActive ? "Active" : "Inactive");
    FBXSDK_printf("                Min Limit Value: %f\n", lMinValues[1]);
    FBXSDK_printf("                Max Limit: %s\n", lMaxYActive ? "Active" : "Inactive");
    FBXSDK_printf("                Max Limit Value: %f\n", lMaxValues[1]);
    FBXSDK_printf("            Z\n");
    FBXSDK_printf("                Min Limit: %s\n", lMinZActive ? "Active" : "Inactive");
    FBXSDK_printf("                Min Limit Value: %f\n", lMinValues[2]);
    FBXSDK_printf("                Max Limit: %s\n", lMaxZActive ? "Active" : "Inactive");
    FBXSDK_printf("                Max Limit Value: %f\n", lMaxValues[2]);
}

void DisplayShape(FbxGeometry* pGeometry)
{
    int lBlendShapeCount, lBlendShapeChannelCount, lTargetShapeCount;
    FbxBlendShape* lBlendShape;
    FbxBlendShapeChannel* lBlendShapeChannel;
    FbxShape* lShape;

    lBlendShapeCount = pGeometry->GetDeformerCount(FbxDeformer::eBlendShape);

    for (int lBlendShapeIndex = 0; lBlendShapeIndex < lBlendShapeCount; ++lBlendShapeIndex)
    {
        lBlendShape = (FbxBlendShape*) pGeometry->GetDeformer(lBlendShapeIndex, FbxDeformer::eBlendShape);
        DisplayString("    BlendShape ", (char *) lBlendShape->GetName());

        lBlendShapeChannelCount = lBlendShape->GetBlendShapeChannelCount();
        for(int lBlendShapeChannelIndex = 0;  lBlendShapeChannelIndex < lBlendShapeChannelCount; ++lBlendShapeChannelIndex)
        {
            lBlendShapeChannel = lBlendShape->GetBlendShapeChannel(lBlendShapeChannelIndex);
            DisplayString("    BlendShapeChannel ", (char *) lBlendShapeChannel->GetName());
            DisplayDouble("    Default Deform Value: ", lBlendShapeChannel->DeformPercent.Get());

            lTargetShapeCount = lBlendShapeChannel->GetTargetShapeCount();
            for (int lTargetShapeIndex = 0; lTargetShapeIndex < lTargetShapeCount; ++lTargetShapeIndex)
            {
                lShape = lBlendShapeChannel->GetTargetShape(lTargetShapeIndex);
                DisplayString("    TargetShape ", (char *) lShape->GetName());

                int j, lControlPointsCount = lShape->GetControlPointsCount();
                FbxVector4* lControlPoints = lShape->GetControlPoints();
                FbxLayerElementArrayTemplate<FbxVector4>* lNormals = NULL;
                bool lStatus = lShape->GetNormals(&lNormals);

                for(j = 0; j < lControlPointsCount; j++)
                {
                    DisplayInt("        Control Point ", j);
                    Display3DVector("            Coordinates: ", lControlPoints[j]);

                    if (lStatus && lNormals && lNormals->GetCount() == lControlPointsCount)
                    {
                        Display3DVector("            Normal Vector: ", lNormals->GetAt(j));
                    }
                }

                DisplayString("");
            }
        }
    }
}

void DisplaySkeleton(FbxNode* pNode)
{
    FbxSkeleton* lSkeleton = (FbxSkeleton*) pNode->GetSkeleton();

    DisplayString("Skeleton Name: ", (char *) pNode->GetName());
    DisplayMetaDataConnections(lSkeleton);

    const char* lSkeletonTypes[] = { "Root", "Limb", "Limb Node", "Effector" };

    DisplayString("    Type: ", lSkeletonTypes[lSkeleton->GetSkeletonType()]);

    if (lSkeleton->GetSkeletonType() == FbxSkeleton::eLimb)
    {
        DisplayDouble("    Limb Length: ", lSkeleton->LimbLength.Get());
    }
    else if (lSkeleton->GetSkeletonType() == FbxSkeleton::eLimbNode)
    {
        DisplayDouble("    Limb Node Size: ", lSkeleton->Size.Get());
    }
    else if (lSkeleton->GetSkeletonType() == FbxSkeleton::eRoot)
    {
        DisplayDouble("    Limb Root Size: ", lSkeleton->Size.Get());
    }

    DisplayColor("    Color: ", lSkeleton->GetLimbNodeColor());
}

void DisplayUserProperties(FbxObject* pObject)
{
    int lCount = 0;
    FbxString lTitleStr = "    Property Count: ";

    FbxProperty lProperty = pObject->GetFirstProperty();
    while (lProperty.IsValid())
    {
        if (lProperty.GetFlag(FbxPropertyFlags::eUserDefined))
            lCount++;

        lProperty = pObject->GetNextProperty(lProperty);
    }

    if (lCount == 0)
        return; // there are no user properties to display

    DisplayInt(lTitleStr.Buffer(), lCount);

    lProperty = pObject->GetFirstProperty();
    int i = 0;
    while (lProperty.IsValid())
    {
        if (lProperty.GetFlag(FbxPropertyFlags::eUserDefined))
        {
            DisplayInt("        Property ", i);
            FbxString lString = lProperty.GetLabel();
            DisplayString("            Display Name: ", lString.Buffer());
            lString = lProperty.GetName();
            DisplayString("            Internal Name: ", lString.Buffer());
            DisplayString("            Type: ", lProperty.GetPropertyDataType().GetName());
            if (lProperty.HasMinLimit()) DisplayDouble("            Min Limit: ", lProperty.GetMinLimit());
            if (lProperty.HasMaxLimit()) DisplayDouble("            Max Limit: ", lProperty.GetMaxLimit());
            DisplayBool  ("            Is Animatable: ", lProperty.GetFlag(FbxPropertyFlags::eAnimatable));

            FbxDataType lPropertyDataType=lProperty.GetPropertyDataType();

            // BOOL
            if (lPropertyDataType.GetType() == eFbxBool)
            {
                DisplayBool("            Default Value: ", lProperty.Get<FbxBool>());
            }
            // REAL
            else if (lPropertyDataType.GetType() == eFbxDouble || lPropertyDataType.GetType() == eFbxFloat)
            {
                DisplayDouble("            Default Value: ", lProperty.Get<FbxDouble>());
            }
            // COLOR
            else if (lPropertyDataType.Is(FbxColor3DT) || lPropertyDataType.Is(FbxColor4DT))
            {
                FbxColor lDefault;
                char      lBuf[64];

                lDefault = lProperty.Get<FbxColor>();
                FBXSDK_sprintf(lBuf, 64, "R=%f, G=%f, B=%f, A=%f", lDefault.mRed, lDefault.mGreen, lDefault.mBlue, lDefault.mAlpha);
                DisplayString("            Default Value: ", lBuf);
            }
            // INTEGER
            else if (lPropertyDataType.GetType() == eFbxInt)
            {
                DisplayInt("            Default Value: ", lProperty.Get<FbxInt>());
            }
            // VECTOR
            else if(lPropertyDataType.GetType() == eFbxDouble3 || lPropertyDataType.GetType() == eFbxDouble4)
            {
                FbxDouble3 lDefault;
                char   lBuf[64];

                lDefault = lProperty.Get<FbxDouble3>();
                FBXSDK_sprintf(lBuf, 64, "X=%f, Y=%f, Z=%f", lDefault[0], lDefault[1], lDefault[2]);
                DisplayString("            Default Value: ", lBuf);
            }
            // LIST
            else if (lPropertyDataType.GetType() == eFbxEnum)
            {
                DisplayInt("            Default Value: ", lProperty.Get<FbxEnum>());
            }
            // UNIDENTIFIED
            else
            {
                DisplayString("            Default Value: UNIDENTIFIED");
            }
            i++;
        }

        lProperty = pObject->GetNextProperty(lProperty);
    }
}


void DisplayMesh(FbxNode* pNode)
{
    FbxMesh* lMesh = (FbxMesh*) pNode->GetNodeAttribute ();

    DisplayString("Mesh Name: ", (char *) pNode->GetName());
    DisplayMetaDataConnections(lMesh);
    DisplayControlsPoints(lMesh);
    DisplayPolygons(lMesh);
    DisplayMaterialMapping(lMesh);
    DisplayMaterial(lMesh);
    DisplayTexture(lMesh);
    DisplayMaterialConnections(lMesh);
    DisplayLink(lMesh);
    DisplayShape(lMesh);

    DisplayCache(lMesh);
}


void DisplayControlsPoints(FbxMesh* pMesh)
{
    int i, lControlPointsCount = pMesh->GetControlPointsCount();
    FbxVector4* lControlPoints = pMesh->GetControlPoints();

    DisplayString("    Control Points");

    for (i = 0; i < lControlPointsCount; i++)
    {
        DisplayInt("        Control Point ", i);
        Display3DVector("            Coordinates: ", lControlPoints[i]);

        for (int j = 0; j < pMesh->GetElementNormalCount(); j++)
        {
            FbxGeometryElementNormal* leNormals = pMesh->GetElementNormal( j);
            if (leNormals->GetMappingMode() == FbxGeometryElement::eByControlPoint)
            {
                char header[100];
                FBXSDK_sprintf(header, 100, "            Normal Vector: ");
                if (leNormals->GetReferenceMode() == FbxGeometryElement::eDirect)
                    Display3DVector(header, leNormals->GetDirectArray().GetAt(i));
            }
        }
    }

    DisplayString("");
}


void DisplayPolygons(FbxMesh* pMesh)
{
    int i, j, lPolygonCount = pMesh->GetPolygonCount();
    FbxVector4* lControlPoints = pMesh->GetControlPoints();
    char header[100];

    DisplayString("    Polygons");

    int vertexId = 0;
    for (i = 0; i < lPolygonCount; i++)
    {
        DisplayInt("        Polygon ", i);
        int l;

        for (l = 0; l < pMesh->GetElementPolygonGroupCount(); l++)
        {
            FbxGeometryElementPolygonGroup* lePolgrp = pMesh->GetElementPolygonGroup(l);
            switch (lePolgrp->GetMappingMode())
            {
            case FbxGeometryElement::eByPolygon:
                if (lePolgrp->GetReferenceMode() == FbxGeometryElement::eIndex)
                {
                    FBXSDK_sprintf(header, 100, "        Assigned to group: ");
                    int polyGroupId = lePolgrp->GetIndexArray().GetAt(i);
                    DisplayInt(header, polyGroupId);
                    break;
                }
            default:
                // any other mapping modes don't make sense
                DisplayString("        \"unsupported group assignment\"");
                break;
            }
        }

        int lPolygonSize = pMesh->GetPolygonSize(i);

        for (j = 0; j < lPolygonSize; j++)
        {
            int lControlPointIndex = pMesh->GetPolygonVertex(i, j);

            Display3DVector("            Coordinates: ", lControlPoints[lControlPointIndex]);

            for (l = 0; l < pMesh->GetElementVertexColorCount(); l++)
            {
                FbxGeometryElementVertexColor* leVtxc = pMesh->GetElementVertexColor( l);
                FBXSDK_sprintf(header, 100, "            Color vertex: ");

                switch (leVtxc->GetMappingMode())
                {
                default:
                    break;
                case FbxGeometryElement::eByControlPoint:
                    switch (leVtxc->GetReferenceMode())
                    {
                    case FbxGeometryElement::eDirect:
                        DisplayColor(header, leVtxc->GetDirectArray().GetAt(lControlPointIndex));
                        break;
                    case FbxGeometryElement::eIndexToDirect:
                        {
                            int id = leVtxc->GetIndexArray().GetAt(lControlPointIndex);
                            DisplayColor(header, leVtxc->GetDirectArray().GetAt(id));
                        }
                        break;
                    default:
                        break; // other reference modes not shown here!
                    }
                    break;

                case FbxGeometryElement::eByPolygonVertex:
                    {
                        switch (leVtxc->GetReferenceMode())
                        {
                        case FbxGeometryElement::eDirect:
                            DisplayColor(header, leVtxc->GetDirectArray().GetAt(vertexId));
                            break;
                        case FbxGeometryElement::eIndexToDirect:
                            {
                                int id = leVtxc->GetIndexArray().GetAt(vertexId);
                                DisplayColor(header, leVtxc->GetDirectArray().GetAt(id));
                            }
                            break;
                        default:
                            break; // other reference modes not shown here!
                        }
                    }
                    break;

                case FbxGeometryElement::eByPolygon: // doesn't make much sense for UVs
                case FbxGeometryElement::eAllSame:   // doesn't make much sense for UVs
                case FbxGeometryElement::eNone:       // doesn't make much sense for UVs
                    break;
                }
            }
            for (l = 0; l < pMesh->GetElementUVCount(); ++l)
            {
                FbxGeometryElementUV* leUV = pMesh->GetElementUV( l);
                FBXSDK_sprintf(header, 100, "            Texture UV: ");

                switch (leUV->GetMappingMode())
                {
                default:
                    break;
                case FbxGeometryElement::eByControlPoint:
                    switch (leUV->GetReferenceMode())
                    {
                    case FbxGeometryElement::eDirect:
                        Display2DVector(header, leUV->GetDirectArray().GetAt(lControlPointIndex));
                        break;
                    case FbxGeometryElement::eIndexToDirect:
                        {
                            int id = leUV->GetIndexArray().GetAt(lControlPointIndex);
                            Display2DVector(header, leUV->GetDirectArray().GetAt(id));
                        }
                        break;
                    default:
                        break; // other reference modes not shown here!
                    }
                    break;

                case FbxGeometryElement::eByPolygonVertex:
                    {
                        int lTextureUVIndex = pMesh->GetTextureUVIndex(i, j);
                        switch (leUV->GetReferenceMode())
                        {
                        case FbxGeometryElement::eDirect:
                        case FbxGeometryElement::eIndexToDirect:
                            {
                                Display2DVector(header, leUV->GetDirectArray().GetAt(lTextureUVIndex));
                            }
                            break;
                        default:
                            break; // other reference modes not shown here!
                        }
                    }
                    break;

                case FbxGeometryElement::eByPolygon: // doesn't make much sense for UVs
                case FbxGeometryElement::eAllSame:   // doesn't make much sense for UVs
                case FbxGeometryElement::eNone:       // doesn't make much sense for UVs
                    break;
                }
            }
            for( l = 0; l < pMesh->GetElementNormalCount(); ++l)
            {
                FbxGeometryElementNormal* leNormal = pMesh->GetElementNormal( l);
                FBXSDK_sprintf(header, 100, "            Normal: ");

                if(leNormal->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
                {
                    switch (leNormal->GetReferenceMode())
                    {
                    case FbxGeometryElement::eDirect:
                        Display3DVector(header, leNormal->GetDirectArray().GetAt(vertexId));
                        break;
                    case FbxGeometryElement::eIndexToDirect:
                        {
                            int id = leNormal->GetIndexArray().GetAt(vertexId);
                            Display3DVector(header, leNormal->GetDirectArray().GetAt(id));
                        }
                        break;
                    default:
                        break; // other reference modes not shown here!
                    }
                }

            }
            for( l = 0; l < pMesh->GetElementTangentCount(); ++l)
            {
                FbxGeometryElementTangent* leTangent = pMesh->GetElementTangent( l);
                FBXSDK_sprintf(header, 100, "            Tangent: ");

                if(leTangent->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
                {
                    switch (leTangent->GetReferenceMode())
                    {
                    case FbxGeometryElement::eDirect:
                        Display3DVector(header, leTangent->GetDirectArray().GetAt(vertexId));
                        break;
                    case FbxGeometryElement::eIndexToDirect:
                        {
                            int id = leTangent->GetIndexArray().GetAt(vertexId);
                            Display3DVector(header, leTangent->GetDirectArray().GetAt(id));
                        }
                        break;
                    default:
                        break; // other reference modes not shown here!
                    }
                }

            }
            for( l = 0; l < pMesh->GetElementBinormalCount(); ++l)
            {

                FbxGeometryElementBinormal* leBinormal = pMesh->GetElementBinormal( l);

                FBXSDK_sprintf(header, 100, "            Binormal: ");
                if(leBinormal->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
                {
                    switch (leBinormal->GetReferenceMode())
                    {
                    case FbxGeometryElement::eDirect:
                        Display3DVector(header, leBinormal->GetDirectArray().GetAt(vertexId));
                        break;
                    case FbxGeometryElement::eIndexToDirect:
                        {
                            int id = leBinormal->GetIndexArray().GetAt(vertexId);
                            Display3DVector(header, leBinormal->GetDirectArray().GetAt(id));
                        }
                        break;
                    default:
                        break; // other reference modes not shown here!
                    }
                }
            }
            vertexId++;
        } // for polygonSize
    } // for polygonCount


    //check visibility for the edges of the mesh
    for(int l = 0; l < pMesh->GetElementVisibilityCount(); ++l)
    {
        FbxGeometryElementVisibility* leVisibility=pMesh->GetElementVisibility(l);
        FBXSDK_sprintf(header, 100, "    Edge Visibility : ");
        DisplayString(header);
        switch(leVisibility->GetMappingMode())
        {
        default:
            break;
            //should be eByEdge
        case FbxGeometryElement::eByEdge:
            //should be eDirect
            for(j=0; j!=pMesh->GetMeshEdgeCount();++j)
            {
                DisplayInt("        Edge ", j);
                DisplayBool("              Edge visibility: ", leVisibility->GetDirectArray().GetAt(j));
            }

            break;
        }
    }
    DisplayString("");
}

void DisplayTextureNames(FbxProperty &pProperty, FbxString& pConnectionString)
{
    int lLayeredTextureCount = pProperty.GetSrcObjectCount<FbxLayeredTexture>();
    if(lLayeredTextureCount > 0)
    {
        for(int j=0; j<lLayeredTextureCount; ++j)
        {
            FbxLayeredTexture *lLayeredTexture = pProperty.GetSrcObject<FbxLayeredTexture>(j);
            int lNbTextures = lLayeredTexture->GetSrcObjectCount<FbxTexture>();
            pConnectionString += " Texture ";

            for(int k =0; k<lNbTextures; ++k)
            {
                //lConnectionString += k;
                pConnectionString += "\"";
                pConnectionString += (char*)lLayeredTexture->GetName();
                pConnectionString += "\"";
                pConnectionString += " ";
            }
            pConnectionString += "of ";
            pConnectionString += pProperty.GetName();
            pConnectionString += " on layer ";
            pConnectionString += j;
        }
        pConnectionString += " |";
    }
    else
    {
        //no layered texture simply get on the property
        int lNbTextures = pProperty.GetSrcObjectCount<FbxTexture>();

        if(lNbTextures > 0)
        {
            pConnectionString += " Texture ";
            pConnectionString += " ";

            for(int j =0; j<lNbTextures; ++j)
            {
                FbxTexture* lTexture = pProperty.GetSrcObject<FbxTexture>(j);
                if(lTexture)
                {
                    pConnectionString += "\"";
                    pConnectionString += (char*)lTexture->GetName();
                    pConnectionString += "\"";
                    pConnectionString += " ";
                }
            }
            pConnectionString += "of ";
            pConnectionString += pProperty.GetName();
            pConnectionString += " |";
        }
    }
}

void DisplayMaterialTextureConnections(FbxSurfaceMaterial* pMaterial, char * header, int pMatId, int l)
{
    if(!pMaterial)
        return;

    FbxString lConnectionString = "            Material %d -- ";
    //Show all the textures

    FbxProperty lProperty;
    //Diffuse Textures
    lProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sDiffuse);
    DisplayTextureNames(lProperty, lConnectionString);

    //DiffuseFactor Textures
    lProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sDiffuseFactor);
    DisplayTextureNames(lProperty, lConnectionString);

    //Emissive Textures
    lProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sEmissive);
    DisplayTextureNames(lProperty, lConnectionString);

    //EmissiveFactor Textures
    lProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sEmissiveFactor);
    DisplayTextureNames(lProperty, lConnectionString);


    //Ambient Textures
    lProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sAmbient);
    DisplayTextureNames(lProperty, lConnectionString);

    //AmbientFactor Textures
    lProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sAmbientFactor);
    DisplayTextureNames(lProperty, lConnectionString);

    //Specular Textures
    lProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sSpecular);
    DisplayTextureNames(lProperty, lConnectionString);

    //SpecularFactor Textures
    lProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sSpecularFactor);
    DisplayTextureNames(lProperty, lConnectionString);

    //Shininess Textures
    lProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sShininess);
    DisplayTextureNames(lProperty, lConnectionString);

    //Bump Textures
    lProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sBump);
    DisplayTextureNames(lProperty, lConnectionString);

    //Normal Map Textures
    lProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sNormalMap);
    DisplayTextureNames(lProperty, lConnectionString);

    //Transparent Textures
    lProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sTransparentColor);
    DisplayTextureNames(lProperty, lConnectionString);

    //TransparencyFactor Textures
    lProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sTransparencyFactor);
    DisplayTextureNames(lProperty, lConnectionString);

    //Reflection Textures
    lProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sReflection);
    DisplayTextureNames(lProperty, lConnectionString);

    //ReflectionFactor Textures
    lProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sReflectionFactor);
    DisplayTextureNames(lProperty, lConnectionString);

    //Update header with material info
    bool lStringOverflow = (lConnectionString.GetLen() + 10 >= MAT_HEADER_LENGTH); // allow for string length and some padding for "%d"
    if (lStringOverflow)
    {
        // Truncate string!
        lConnectionString = lConnectionString.Left(MAT_HEADER_LENGTH - 10);
        lConnectionString = lConnectionString + "...";
    }
    FBXSDK_sprintf(header, MAT_HEADER_LENGTH, lConnectionString.Buffer(), pMatId, l);
    DisplayString(header);
}

void DisplayMaterialConnections(FbxMesh* pMesh)
{
    int i, l, lPolygonCount = pMesh->GetPolygonCount();

    char header[MAT_HEADER_LENGTH];

    DisplayString("    Polygons Material Connections");

    //check whether the material maps with only one mesh
    bool lIsAllSame = true;
    for (l = 0; l < pMesh->GetElementMaterialCount(); l++)
    {

        FbxGeometryElementMaterial* lMaterialElement = pMesh->GetElementMaterial(l);
        if( lMaterialElement->GetMappingMode() == FbxGeometryElement::eByPolygon)
        {
            lIsAllSame = false;
            break;
        }
    }

    //For eAllSame mapping type, just out the material and texture mapping info once
    if(lIsAllSame)
    {
        for (l = 0; l < pMesh->GetElementMaterialCount(); l++)
        {

            FbxGeometryElementMaterial* lMaterialElement = pMesh->GetElementMaterial( l);
            if( lMaterialElement->GetMappingMode() == FbxGeometryElement::eAllSame)
            {
                FbxSurfaceMaterial* lMaterial = pMesh->GetNode()->GetMaterial(lMaterialElement->GetIndexArray().GetAt(0));
                int lMatId = lMaterialElement->GetIndexArray().GetAt(0);
                if(lMatId >= 0)
                {
                    DisplayInt("        All polygons share the same material in mesh ", l);
                    DisplayMaterialTextureConnections(lMaterial, header, lMatId, l);
                }
            }
        }

        //no material
        if(l == 0)
            DisplayString("        no material applied");
    }

    //For eByPolygon mapping type, just out the material and texture mapping info once
    else
    {
        for (i = 0; i < lPolygonCount; i++)
        {
            DisplayInt("        Polygon ", i);

            for (l = 0; l < pMesh->GetElementMaterialCount(); l++)
            {

                FbxGeometryElementMaterial* lMaterialElement = pMesh->GetElementMaterial( l);
                FbxSurfaceMaterial* lMaterial = NULL;
                int lMatId = -1;
                lMaterial = pMesh->GetNode()->GetMaterial(lMaterialElement->GetIndexArray().GetAt(i));
                lMatId = lMaterialElement->GetIndexArray().GetAt(i);

                if(lMatId >= 0)
                {
                    DisplayMaterialTextureConnections(lMaterial, header, lMatId, l);
                }
            }
        }
    }
}


void DisplayMaterialMapping(FbxMesh* pMesh)
{
    const char* lMappingTypes[] = { "None", "By Control Point", "By Polygon Vertex", "By Polygon", "By Edge", "All Same" };
    const char* lReferenceMode[] = { "Direct", "Index", "Index to Direct"};

    int lMtrlCount = 0;
    FbxNode* lNode = NULL;
    if(pMesh){
        lNode = pMesh->GetNode();
        if(lNode)
            lMtrlCount = lNode->GetMaterialCount();
    }

    for (int l = 0; l < pMesh->GetElementMaterialCount(); l++)
    {
        FbxGeometryElementMaterial* leMat = pMesh->GetElementMaterial( l);
        if (leMat)
        {
            char header[100];
            FBXSDK_sprintf(header, 100, "    Material Element %d: ", l);
            DisplayString(header);


            DisplayString("           Mapping: ", lMappingTypes[leMat->GetMappingMode()]);
            DisplayString("           ReferenceMode: ", lReferenceMode[leMat->GetReferenceMode()]);

            int lMaterialCount = 0;
            FbxString lString;

            if (leMat->GetReferenceMode() == FbxGeometryElement::eDirect ||
                leMat->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
            {
                lMaterialCount = lMtrlCount;
            }

            if (leMat->GetReferenceMode() == FbxGeometryElement::eIndex ||
                leMat->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
            {
                int i;

                lString = "           Indices: ";

                int lIndexArrayCount = leMat->GetIndexArray().GetCount();
                for (i = 0; i < lIndexArrayCount; i++)
                {
                    lString += leMat->GetIndexArray().GetAt(i);

                    if (i < lIndexArrayCount - 1)
                    {
                        lString += ", ";
                    }
                }

                lString += "\n";

                FBXSDK_printf(lString);
            }
        }
    }

    DisplayString("");
}

void DisplayLight(FbxNode* pNode)
{
    FbxLight* lLight = (FbxLight*) pNode->GetNodeAttribute();

    DisplayString("Light Name: ", (char *) pNode->GetName());
    DisplayMetaDataConnections(lLight);

    const char* lLightTypes[] = { "Point", "Directional", "Spot" };

    DisplayString("    Type: ", lLightTypes[lLight->LightType.Get()]);
    DisplayBool("    Cast Light: ", lLight->CastLight.Get());

    if (!(lLight->FileName.Get().IsEmpty()))
    {
        DisplayString("    Gobo");

        DisplayString("        File Name: \"", lLight->FileName.Get().Buffer(), "\"");
        DisplayBool("        Ground Projection: ", lLight->DrawGroundProjection.Get());
        DisplayBool("        Volumetric Projection: ", lLight->DrawVolumetricLight.Get());
        DisplayBool("        Front Volumetric Projection: ", lLight->DrawFrontFacingVolumetricLight.Get());
    }

    DisplayDefaultAnimationValues(lLight);
}


void DisplayDefaultAnimationValues(FbxLight* pLight)
{
    DisplayString("    Default Animation Values");

    FbxDouble3 c = pLight->Color.Get();
    FbxColor lColor(c[0], c[1], c[2]);
    DisplayColor("        Default Color: ", lColor);
    DisplayDouble("        Default Intensity: ", pLight->Intensity.Get());
    DisplayDouble("        Default Outer Angle: ", pLight->OuterAngle.Get());
    DisplayDouble("        Default Fog: ", pLight->Fog.Get());
}

void DisplayCamera(FbxNode* pNode)
{
    DisplayCamera((FbxCamera*)pNode->GetNodeAttribute(), (char *)pNode->GetName(), pNode->GetTarget(), pNode->GetTargetUp());
}

void DisplayCamera(FbxCamera* pCamera, char* pName, FbxNode* pTargetNode, FbxNode* pTargetUpNode)
{
    DisplayString("Camera Name: ", pName);
    if (!pCamera)
    {
        DisplayString("NOT FOUND");
        return;
    }
    DisplayMetaDataConnections(pCamera);

    DisplayCameraPositionAndOrientation(pCamera, pTargetNode, pTargetUpNode);

    const char* lProjectionTypes[] = { "Perspective", "Orthogonal" };

    DisplayString("    Projection Type: ", lProjectionTypes[pCamera->ProjectionType.Get()]);

    DisplayViewingAreaControls(pCamera);

    // If camera projection type is set to FbxCamera::eOrthogonal, the
    // aperture and film controls are not relevant.
    if (pCamera->ProjectionType.Get() != FbxCamera::eOrthogonal)
    {
        DisplayApertureAndFilmControls(pCamera);
    }

    DisplayBackgroundProperties(pCamera);
    DisplayCameraViewOptions(pCamera);
    DisplayRenderOptions(pCamera);
    DisplayDefaultAnimationValues(pCamera);
}


void DisplayCameraPositionAndOrientation(FbxCamera* pCamera, FbxNode* pTargetNode, FbxNode* pTargetUpNode)
{
    DisplayString("    Camera Position and Orientation");
    Display3DVector("        Position: ", pCamera->Position.Get());

    if (pTargetNode)
    {
        DisplayString("        Camera Interest: ",(char *) pTargetNode->GetName());
    }
    else
    {
        Display3DVector("        Default Camera Interest Position: ", pCamera->InterestPosition.Get());
    }

    if (pTargetUpNode)
    {
        DisplayString("        Camera Up Target: ", (char *) pTargetUpNode->GetName());
    }
    else
    {
        Display3DVector("        Up Vector: ", pCamera->UpVector.Get());
    }

    DisplayDouble("        Roll: ", pCamera->Roll.Get());
}


void DisplayViewingAreaControls(FbxCamera* pCamera)
{
    DisplayString("    Viewing Area Controls");

    const char* lCameraFormat[] = { "Custom", "D1 NTSC", "NTSC", "PAL", "D1 PAL",
        "HD", "640x480", "320x200", "320x240", "128x128",
        "Full Screen"  };

    DisplayString("        Format: ", lCameraFormat[pCamera->GetFormat()]);

    const char* lAspectRatioModes[] = { "Window Size", "Fixed Ratio", "Fixed Resolution",
        "Fixed Width", "Fixed Height" };

    DisplayString("        Aspect Ratio Mode: ", lAspectRatioModes[pCamera->GetAspectRatioMode()]);

    // If the ratio mode is eWINDOW_SIZE, both width and height values aren't relevant.
    if (pCamera->GetAspectRatioMode() != FbxCamera::eWindowSize)
    {
        DisplayDouble("        Aspect Width: ", pCamera->AspectWidth.Get());
        DisplayDouble("        Aspect Height: ", pCamera->AspectHeight.Get());
    }

    DisplayDouble("        Pixel Ratio: ", pCamera->PixelAspectRatio.Get());
    DisplayDouble("        Near Plane: ", pCamera->NearPlane.Get());
    DisplayDouble("        Far Plane: ", pCamera->FarPlane.Get());
    DisplayBool("        Mouse Lock: ", pCamera->LockMode.Get());
}


void DisplayApertureAndFilmControls(FbxCamera* pCamera)
{
    DisplayString("    Aperture and Film Controls");

    const char* lCameraApertureFormats[] = { "Custom",
        "16mm Theatrical",
        "Super 16mm",
        "35mm Academy",
        "35mm TV Projection",
        "35mm Full Aperture",
        "35mm 1.85 Projection",
        "35mm Anamorphic",
        "70mm Projection",
        "VistaVision",
        "Dynavision",
        "Imax" };

    DisplayString("        Aperture Format: ", lCameraApertureFormats[pCamera->GetApertureFormat()]);

    const char* lCameraApertureModes[] = { "Horizontal and Vertical", "Horizontal", "Vertical", "Focal Length" };

    DisplayString("        Aperture Mode: ", lCameraApertureModes[pCamera->GetApertureMode()]);

    DisplayDouble("        Aperture Width: ", pCamera->GetApertureWidth(), " inches");
    DisplayDouble("        Aperture Height: ", pCamera->GetApertureHeight(), " inches");
    DisplayDouble("        Squeeze Ratio: ", pCamera->GetSqueezeRatio());
    DisplayDouble("        Focal Length: ", pCamera->FocalLength.Get(), "mm");
    DisplayDouble("        Field of View: ", pCamera->FieldOfView.Get(), " degrees");
}


void DisplayBackgroundProperties(FbxCamera* pCamera)
{
    DisplayString("    Background Properties");

    DisplayString("        Background File Name: \"", (char *) pCamera->GetBackgroundFileName(), "\"");

    const char* lBackgroundDisplayModes[] = { "Disabled", "Always", "When Media" };

    DisplayString("        Background Display Mode: ", lBackgroundDisplayModes[pCamera->ViewFrustumBackPlaneMode.Get()]);

    DisplayBool("        Foreground Matte Threshold Enable: ", pCamera->ShowFrontplate.Get());

    // This option is only relevant if background drawing mode is set to eFOREGROUND or eBACKGROUND_AND_FOREGROUND.
    if (pCamera->ForegroundOpacity.Get())
    {
        DisplayDouble("        Foreground Matte Threshold: ", pCamera->BackgroundAlphaTreshold.Get());
    }

    FbxString lBackgroundPlacementOptions;

    if (pCamera->GetBackPlateFitImage())
    {
        lBackgroundPlacementOptions += " Fit,";
    }
    if (pCamera->GetBackPlateCenter())
    {
        lBackgroundPlacementOptions += " Center,";
    }
    if (pCamera->GetBackPlateKeepRatio())
    {
        lBackgroundPlacementOptions += " Keep Ratio,";
    }
    if (pCamera->GetBackPlateCrop())
    {
        lBackgroundPlacementOptions += " Crop,";
    }
    if (!lBackgroundPlacementOptions.IsEmpty())
    {
        FbxString lString =  lBackgroundPlacementOptions.Left(lBackgroundPlacementOptions.GetLen() - 1);
        DisplayString("        Background Placement Options: ",lString.Buffer());
    }

    DisplayDouble("        Background Distance: ", pCamera->BackPlaneDistance.Get());

    const char* lCameraBackgroundDistanceModes[] = { "Relative to Interest", "Absolute from Camera" };

    DisplayString("        Background Distance Mode: ", lCameraBackgroundDistanceModes[pCamera->BackPlaneDistanceMode.Get()]);
}


void DisplayCameraViewOptions(FbxCamera* pCamera)
{
    DisplayString("    Camera View Options");

    DisplayBool("        View Camera Interest: ", pCamera->ViewCameraToLookAt.Get());
    DisplayBool("        View Near Far Planes: ", pCamera->ViewFrustumNearFarPlane.Get());
    DisplayBool("        Show Grid: ", pCamera->ShowGrid.Get());
    DisplayBool("        Show Axis: ", pCamera->ShowAzimut.Get());
    DisplayBool("        Show Name: ", pCamera->ShowName.Get());
    DisplayBool("        Show Info on Moving: ", pCamera->ShowInfoOnMoving.Get());
    DisplayBool("        Show Time Code: ", pCamera->ShowTimeCode.Get());
    DisplayBool("        Display Safe Area: ", pCamera->DisplaySafeArea.Get());

    const char* lSafeAreaStyles[] = { "Round", "Square" };
    FbxColor color;
    FbxDouble3 c;

    DisplayString("        Safe Area Style: ", lSafeAreaStyles[pCamera->SafeAreaDisplayStyle.Get()]);
    DisplayBool("        Show Audio: ", pCamera->ShowAudio.Get());

    c = pCamera->BackgroundColor.Get();
    color = FbxColor(c[0], c[1], c[2]);
    DisplayColor("        Background Color: ", color);

    c = pCamera->AudioColor.Get();
    color = FbxColor(c[0], c[1], c[2]);
    DisplayColor("        Audio Color: ", color);

    DisplayBool("        Use Frame Color: ", pCamera->UseFrameColor.Get());

    c = pCamera->FrameColor.Get();
    color = FbxColor(c[0], c[1], c[2]);
    DisplayColor("        Frame Color: ", color);
}


void DisplayRenderOptions(FbxCamera* pCamera)
{
    DisplayString("    Render Options");

    const char* lCameraRenderOptionsUsageTimes[] = { "Interactive", "At Render" };

    DisplayString("        Render Options Usage Time: ", lCameraRenderOptionsUsageTimes[pCamera->UseRealTimeDOFAndAA.Get()]);
    DisplayBool("        Use Antialiasing: ", pCamera->UseAntialiasing.Get());
    DisplayDouble("        Antialiasing Intensity: ", pCamera->AntialiasingIntensity.Get());

    const char* lCameraAntialiasingMethods[] = { "Oversampling Antialiasing", "Hardware Antialiasing" };

    DisplayString("        Antialiasing Method: ", lCameraAntialiasingMethods[pCamera->AntialiasingMethod.Get()]);

    // This option is only relevant if antialiasing method is set to eOVERSAMPLING_ANTIALIASING.
    if (pCamera->AntialiasingMethod.Get() == FbxCamera::eAAOversampling)
    {
        DisplayInt("        Number of Samples: ", pCamera->FrameSamplingCount.Get());
    }

    const char* lCameraSamplingTypes[] = { "Uniform", "Stochastic" };

    DisplayString("        Sampling Type: ", lCameraSamplingTypes[pCamera->FrameSamplingType.Get()]);
    DisplayBool("        Use Accumulation Buffer: ", pCamera->UseAccumulationBuffer.Get());
    DisplayBool("        Use Depth of Field: ", pCamera->UseDepthOfField.Get());

    const char* lCameraFocusDistanceSources[] = { "Camera Interest", "Specific Distance" };

    DisplayString("        Focus Distance Source: ", lCameraFocusDistanceSources[pCamera->FocusSource.Get()]);

    // This parameter is only relevant if focus distance source is set to eSPECIFIC_DISTANCE.
    if (pCamera->FocusSource.Get() == FbxCamera::eFocusSpecificDistance)
    {
        DisplayDouble("        Specific Distance: ", pCamera->FocusDistance.Get());
    }

    DisplayDouble("        Focus Angle: ", pCamera->FocusAngle.Get(), " degrees");
}


void DisplayDefaultAnimationValues(FbxCamera* pCamera)
{
    DisplayString("    Default Animation Values");

    DisplayDouble("        Default Field of View: ", pCamera->FieldOfView.Get());
    DisplayDouble("        Default Field of View X: ", pCamera->FieldOfViewX.Get());
    DisplayDouble("        Default Field of View Y: ", pCamera->FieldOfViewY.Get());
    DisplayDouble("        Default Optical Center X: ", pCamera->OpticalCenterX.Get());
    DisplayDouble("        Default Optical Center Y: ", pCamera->OpticalCenterY.Get());
    DisplayDouble("        Default Roll: ", pCamera->Roll.Get());
}

void DisplayGenericInfo(FbxScene* pScene)
{
    int i;
    FbxNode* lRootNode = pScene->GetRootNode();

    for (i = 0; i < lRootNode->GetChildCount(); i++)
    {
        DisplayGenericInfo(lRootNode->GetChild(i), 0);
    }

    //Other objects directly connected onto the scene
    for (i = 0; i < pScene->GetSrcObjectCount(); ++i)
    {
        DisplayProperties(pScene->GetSrcObject(i));
    }
}


void DisplayGenericInfo(FbxNode* pNode, int pDepth)
{
    FbxString lString;
    int i;

    for(i = 0; i < pDepth; i++)
    {
        lString += "     ";
    }

    lString += pNode->GetName();
    lString += "\n";

    DisplayString(lString.Buffer());

    //Display generic info about that Node
    DisplayProperties(pNode);
    DisplayString("");
    for(i = 0; i < pNode->GetChildCount(); i++)
    {
        DisplayGenericInfo(pNode->GetChild(i), pDepth + 1);
    }

}

void DisplayProperties(FbxObject* pObject)
{
    DisplayString("Name: ", (char *)pObject->GetName());

    // Display all the properties
    int i,  lCount = 0;
    FbxProperty lProperty = pObject->GetFirstProperty();
    while (lProperty.IsValid())
    {
        lCount++;
        lProperty = pObject->GetNextProperty(lProperty);
    }

    FbxString lTitleStr = "    Property Count: ";

    if (lCount == 0)
        return; // there are no properties to display

    DisplayInt(lTitleStr.Buffer(), lCount);

    i=0;
    lProperty = pObject->GetFirstProperty();
    while (lProperty.IsValid())
    {
        // exclude user properties

        FbxString lString;
        DisplayInt("        Property ", i);
        lString = lProperty.GetLabel();
        DisplayString("            Display Name: ", lString.Buffer());
        lString = lProperty.GetName();
        DisplayString("            Internal Name: ", lString.Buffer());
        lString = lProperty.GetPropertyDataType().GetName();
        DisplayString("            Type: ",lString.Buffer());
        if (lProperty.HasMinLimit()) DisplayDouble("            Min Limit: ", lProperty.GetMinLimit());
        if (lProperty.HasMaxLimit()) DisplayDouble("            Max Limit: ", lProperty.GetMaxLimit());
        DisplayBool  ("            Is Animatable: ", lProperty.GetFlag(FbxPropertyFlags::eAnimatable));


        switch (lProperty.GetPropertyDataType().GetType())
        {
        case eFbxBool:
            DisplayBool("            Default Value: ", lProperty.Get<FbxBool>());
            break;

        case eFbxDouble:
            DisplayDouble("            Default Value: ", lProperty.Get<FbxDouble>());
            break;

        case eFbxDouble4:
            {
                FbxColor lDefault;
                char      lBuf[64];

                lDefault = lProperty.Get<FbxColor>();
                FBXSDK_sprintf(lBuf, 64, "R=%f, G=%f, B=%f, A=%f", lDefault.mRed, lDefault.mGreen, lDefault.mBlue, lDefault.mAlpha);
                DisplayString("            Default Value: ", lBuf);
            }
            break;

        case eFbxInt:
            DisplayInt("            Default Value: ", lProperty.Get<FbxInt>());
            break;

        case eFbxDouble3:
            {
                FbxDouble3 lDefault;
                char   lBuf[64];

                lDefault = lProperty.Get<FbxDouble3>();
                FBXSDK_sprintf(lBuf, 64, "X=%f, Y=%f, Z=%f", lDefault[0], lDefault[1], lDefault[2]);
                DisplayString("            Default Value: ", lBuf);
            }
            break;

        //case FbxEnumDT:
        //    DisplayInt("            Default Value: ", lProperty.Get<FbxEnum>());
        //    break;

        case eFbxFloat:
            DisplayDouble("            Default Value: ", lProperty.Get<FbxFloat>());
            break;
        case eFbxString:
            lString = lProperty.Get<FbxString>();
            DisplayString("            Default Value: ", lString.Buffer());
            break;

        default:
            DisplayString("            Default Value: UNIDENTIFIED");
            break;
        }
        i++;
        lProperty = pObject->GetNextProperty(lProperty);
    }
}

void DisplayAnimation(FbxScene* pScene)
{
    int i;
    for (i = 0; i < pScene->GetSrcObjectCount<FbxAnimStack>(); i++)
    {
        FbxAnimStack* lAnimStack = pScene->GetSrcObject<FbxAnimStack>(i);

        FbxString lOutputString = "Animation Stack Name: ";
        lOutputString += lAnimStack->GetName();
        lOutputString += "\n\n";
        FBXSDK_printf(lOutputString);

        DisplayAnimation(lAnimStack, pScene->GetRootNode(), true);
        DisplayAnimation(lAnimStack, pScene->GetRootNode());
    }
}

void DisplayAnimation(FbxAnimStack* pAnimStack, FbxNode* pNode, bool isSwitcher)
{
    int l;
    int nbAnimLayers = pAnimStack->GetMemberCount<FbxAnimLayer>();
    FbxString lOutputString;

    lOutputString = "Animation stack contains ";
    lOutputString += nbAnimLayers;
    lOutputString += " Animation Layer(s)\n";
    FBXSDK_printf(lOutputString);

    for (l = 0; l < nbAnimLayers; l++)
    {
        FbxAnimLayer* lAnimLayer = pAnimStack->GetMember<FbxAnimLayer>(l);

        lOutputString = "AnimLayer ";
        lOutputString += l;
        lOutputString += "\n";
        FBXSDK_printf(lOutputString);

        DisplayAnimation(lAnimLayer, pNode, isSwitcher);
    }
}

void DisplayAnimation(FbxAnimLayer* pAnimLayer, FbxNode* pNode, bool isSwitcher)
{
    int lModelCount;
    FbxString lOutputString;

    lOutputString = "     Node Name: ";
    lOutputString += pNode->GetName();
    lOutputString += "\n\n";
    FBXSDK_printf(lOutputString);

    DisplayChannels(pNode, pAnimLayer, DisplayCurveKeys, DisplayListCurveKeys, isSwitcher);
    FBXSDK_printf ("\n");

    for(lModelCount = 0; lModelCount < pNode->GetChildCount(); lModelCount++)
    {
        DisplayAnimation(pAnimLayer, pNode->GetChild(lModelCount), isSwitcher);
    }
}


void DisplayChannels(FbxNode* pNode, FbxAnimLayer* pAnimLayer, void(*DisplayCurve) (FbxAnimCurve* pCurve), void(*DisplayListCurve) (FbxAnimCurve* pCurve, FbxProperty* pProperty), bool isSwitcher)
{
    FbxAnimCurve* lAnimCurve = NULL;

    // Display general curves.
    if (!isSwitcher)
    {
        lAnimCurve = pNode->LclTranslation.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
        if (lAnimCurve)
        {
            FBXSDK_printf("        TX\n");
            DisplayCurve(lAnimCurve);
        }
        lAnimCurve = pNode->LclTranslation.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
        if (lAnimCurve)
        {
            FBXSDK_printf("        TY\n");
            DisplayCurve(lAnimCurve);
        }
        lAnimCurve = pNode->LclTranslation.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);
        if (lAnimCurve)
        {
            FBXSDK_printf("        TZ\n");
            DisplayCurve(lAnimCurve);
        }

        lAnimCurve = pNode->LclRotation.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
        if (lAnimCurve)
        {
            FBXSDK_printf("        RX\n");
            DisplayCurve(lAnimCurve);
        }
        lAnimCurve = pNode->LclRotation.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
        if (lAnimCurve)
        {
            FBXSDK_printf("        RY\n");
            DisplayCurve(lAnimCurve);
        }
        lAnimCurve = pNode->LclRotation.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);
        if (lAnimCurve)
        {
            FBXSDK_printf("        RZ\n");
            DisplayCurve(lAnimCurve);
        }

        lAnimCurve = pNode->LclScaling.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
        if (lAnimCurve)
        {
            FBXSDK_printf("        SX\n");
            DisplayCurve(lAnimCurve);
        }
        lAnimCurve = pNode->LclScaling.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
        if (lAnimCurve)
        {
            FBXSDK_printf("        SY\n");
            DisplayCurve(lAnimCurve);
        }
        lAnimCurve = pNode->LclScaling.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);
        if (lAnimCurve)
        {
            FBXSDK_printf("        SZ\n");
            DisplayCurve(lAnimCurve);
        }
    }

    // Display curves specific to a light or marker.
    FbxNodeAttribute* lNodeAttribute = pNode->GetNodeAttribute();

    if (lNodeAttribute)
    {
        lAnimCurve = lNodeAttribute->Color.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COLOR_RED);
        if (lAnimCurve)
        {
            FBXSDK_printf("        Red\n");
            DisplayCurve(lAnimCurve);
        }
        lAnimCurve = lNodeAttribute->Color.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COLOR_GREEN);
        if (lAnimCurve)
        {
            FBXSDK_printf("        Green\n");
            DisplayCurve(lAnimCurve);
        }
        lAnimCurve = lNodeAttribute->Color.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COLOR_BLUE);
        if (lAnimCurve)
        {
            FBXSDK_printf("        Blue\n");
            DisplayCurve(lAnimCurve);
        }

        // Display curves specific to a light.
        FbxLight* light = pNode->GetLight();
        if (light)
        {
            lAnimCurve = light->Intensity.GetCurve(pAnimLayer);
            if (lAnimCurve)
            {
                FBXSDK_printf("        Intensity\n");
                DisplayCurve(lAnimCurve);
            }

            lAnimCurve = light->OuterAngle.GetCurve(pAnimLayer);
            if (lAnimCurve)
            {
                FBXSDK_printf("        Outer Angle\n");
                DisplayCurve(lAnimCurve);
            }

            lAnimCurve = light->Fog.GetCurve(pAnimLayer);
            if (lAnimCurve)
            {
                FBXSDK_printf("        Fog\n");
                DisplayCurve(lAnimCurve);
            }
        }

        // Display curves specific to a camera.
        FbxCamera* camera = pNode->GetCamera();
        if (camera)
        {
            lAnimCurve = camera->FieldOfView.GetCurve(pAnimLayer);
            if (lAnimCurve)
            {
                FBXSDK_printf("        Field of View\n");
                DisplayCurve(lAnimCurve);
            }

            lAnimCurve = camera->FieldOfViewX.GetCurve(pAnimLayer);
            if (lAnimCurve)
            {
                FBXSDK_printf("        Field of View X\n");
                DisplayCurve(lAnimCurve);
            }

            lAnimCurve = camera->FieldOfViewY.GetCurve(pAnimLayer);
            if (lAnimCurve)
            {
                FBXSDK_printf("        Field of View Y\n");
                DisplayCurve(lAnimCurve);
            }

            lAnimCurve = camera->OpticalCenterX.GetCurve(pAnimLayer);
            if (lAnimCurve)
            {
                FBXSDK_printf("        Optical Center X\n");
                DisplayCurve(lAnimCurve);
            }

            lAnimCurve = camera->OpticalCenterY.GetCurve(pAnimLayer);
            if(lAnimCurve)
            {
                FBXSDK_printf("        Optical Center Y\n");
                DisplayCurve(lAnimCurve);
            }

            lAnimCurve = camera->Roll.GetCurve(pAnimLayer);
            if(lAnimCurve)
            {
                FBXSDK_printf("        Roll\n");
                DisplayCurve(lAnimCurve);
            }
        }

        // Display curves specific to a geometry.
        if (lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh ||
            lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eNurbs ||
            lNodeAttribute->GetAttributeType() == FbxNodeAttribute::ePatch)
        {
            FbxGeometry* lGeometry = (FbxGeometry*) lNodeAttribute;

            int lBlendShapeDeformerCount = lGeometry->GetDeformerCount(FbxDeformer::eBlendShape);
            for(int lBlendShapeIndex = 0; lBlendShapeIndex<lBlendShapeDeformerCount; ++lBlendShapeIndex)
            {
                FbxBlendShape* lBlendShape = (FbxBlendShape*)lGeometry->GetDeformer(lBlendShapeIndex, FbxDeformer::eBlendShape);

                int lBlendShapeChannelCount = lBlendShape->GetBlendShapeChannelCount();
                for(int lChannelIndex = 0; lChannelIndex<lBlendShapeChannelCount; ++lChannelIndex)
                {
                    FbxBlendShapeChannel* lChannel = lBlendShape->GetBlendShapeChannel(lChannelIndex);
                    const char* lChannelName = lChannel->GetName();

                    lAnimCurve = lGeometry->GetShapeChannel(lBlendShapeIndex, lChannelIndex, pAnimLayer, true);
                    if(lAnimCurve)
                    {
                        FBXSDK_printf("        Shape %s\n", lChannelName);
                        DisplayCurve(lAnimCurve);
                    }
                }
            }
        }
    }

    // Display curves specific to properties
    FbxProperty lProperty = pNode->GetFirstProperty();
    while (lProperty.IsValid())
    {
        if (lProperty.GetFlag(FbxPropertyFlags::eUserDefined))
        {
            FbxString lFbxFCurveNodeName  = lProperty.GetName();
            FbxAnimCurveNode* lCurveNode = lProperty.GetCurveNode(pAnimLayer);

            if (!lCurveNode){
                lProperty = pNode->GetNextProperty(lProperty);
                continue;
            }

            FbxDataType lDataType = lProperty.GetPropertyDataType();
            if (lDataType.GetType() == eFbxBool || lDataType.GetType() == eFbxDouble || lDataType.GetType() == eFbxFloat || lDataType.GetType() == eFbxInt)
            {
                FbxString lMessage;

                lMessage =  "        Property ";
                lMessage += lProperty.GetName();
                if (lProperty.GetLabel().GetLen() > 0)
                {
                    lMessage += " (Label: ";
                    lMessage += lProperty.GetLabel();
                    lMessage += ")";
                };

                DisplayString(lMessage.Buffer());

                for( int c = 0; c < lCurveNode->GetCurveCount(0U); c++ )
                {
                    lAnimCurve = lCurveNode->GetCurve(0U, c);
                    if (lAnimCurve)
                        DisplayCurve(lAnimCurve);
                }
            }
            else if(lDataType.GetType() == eFbxDouble3 || lDataType.GetType() == eFbxDouble4 || lDataType.Is(FbxColor3DT) || lDataType.Is(FbxColor4DT))
            {
                char* lComponentName1 = (lDataType.Is(FbxColor3DT) ||lDataType.Is(FbxColor4DT)) ? (char*)FBXSDK_CURVENODE_COLOR_RED : (char*)"X";
                char* lComponentName2 = (lDataType.Is(FbxColor3DT) ||lDataType.Is(FbxColor4DT)) ? (char*)FBXSDK_CURVENODE_COLOR_GREEN : (char*)"Y";
                char* lComponentName3 = (lDataType.Is(FbxColor3DT) ||lDataType.Is(FbxColor4DT)) ? (char*)FBXSDK_CURVENODE_COLOR_BLUE  : (char*)"Z";
                FbxString      lMessage;

                lMessage =  "        Property ";
                lMessage += lProperty.GetName();
                if (lProperty.GetLabel().GetLen() > 0)
                {
                    lMessage += " (Label: ";
                    lMessage += lProperty.GetLabel();
                    lMessage += ")";
                }
                DisplayString(lMessage.Buffer());

                for( int c = 0; c < lCurveNode->GetCurveCount(0U); c++ )
                {
                    lAnimCurve = lCurveNode->GetCurve(0U, c);
                    if (lAnimCurve)
                    {
                        DisplayString("        Component ", lComponentName1);
                        DisplayCurve(lAnimCurve);
                    }
                }

                for( int c = 0; c < lCurveNode->GetCurveCount(1U); c++ )
                {
                    lAnimCurve = lCurveNode->GetCurve(1U, c);
                    if (lAnimCurve)
                    {
                        DisplayString("        Component ", lComponentName2);
                        DisplayCurve(lAnimCurve);
                    }
                }

                for( int c = 0; c < lCurveNode->GetCurveCount(2U); c++ )
                {
                    lAnimCurve = lCurveNode->GetCurve(2U, c);
                    if (lAnimCurve)
                    {
                        DisplayString("        Component ", lComponentName3);
                        DisplayCurve(lAnimCurve);
                    }
                }
            }
            else if (lDataType.GetType() == eFbxEnum)
            {
                FbxString lMessage;

                lMessage =  "        Property ";
                lMessage += lProperty.GetName();
                if (lProperty.GetLabel().GetLen() > 0)
                {
                    lMessage += " (Label: ";
                    lMessage += lProperty.GetLabel();
                    lMessage += ")";
                };
                DisplayString(lMessage.Buffer());

                for( int c = 0; c < lCurveNode->GetCurveCount(0U); c++ )
                {
                    lAnimCurve = lCurveNode->GetCurve(0U, c);
                    if (lAnimCurve)
                        DisplayListCurve(lAnimCurve, &lProperty);
                }
            }
        }

        lProperty = pNode->GetNextProperty(lProperty);
    } // while

}


void DisplayTransformPropagation(FbxNode* pNode)
{
    FBXSDK_printf("    Transformation Propagation\n");

    //
    // Rotation Space
    //
    EFbxRotationOrder lRotationOrder;
    pNode->GetRotationOrder(FbxNode::eSourcePivot, lRotationOrder);

    FBXSDK_printf("        Rotation Space: ");

    switch (lRotationOrder)
    {
    case eEulerXYZ:
        FBXSDK_printf("Euler XYZ\n");
        break;
    case eEulerXZY:
        FBXSDK_printf("Euler XZY\n");
        break;
    case eEulerYZX:
        FBXSDK_printf("Euler YZX\n");
        break;
    case eEulerYXZ:
        FBXSDK_printf("Euler YXZ\n");
        break;
    case eEulerZXY:
        FBXSDK_printf("Euler ZXY\n");
        break;
    case eEulerZYX:
        FBXSDK_printf("Euler ZYX\n");
        break;
    case eSphericXYZ:
        FBXSDK_printf("Spheric XYZ\n");
        break;
    }

    //
    // Use the Rotation space only for the limits
    // (keep using eEulerXYZ for the rest)
    //
    FBXSDK_printf("        Use the Rotation Space for Limit specification only: %s\n",
        pNode->GetUseRotationSpaceForLimitOnly(FbxNode::eSourcePivot) ? "Yes" : "No");


    //
    // Inherit Type
    //
    FbxTransform::EInheritType lInheritType;
    pNode->GetTransformationInheritType(lInheritType);

    FBXSDK_printf("        Transformation Inheritance: ");

    switch (lInheritType)
    {
    case FbxTransform::eInheritRrSs:
        FBXSDK_printf("RrSs\n");
        break;
    case FbxTransform::eInheritRSrs:
        FBXSDK_printf("RSrs\n");
        break;
    case FbxTransform::eInheritRrs:
        FBXSDK_printf("Rrs\n");
        break;
    }
}

void DisplayGeometricTransform(FbxNode* pNode)
{
    FbxVector4 lTmpVector;

    FBXSDK_printf("    Geometric Transformations\n");

    //
    // Translation
    //
    lTmpVector = pNode->GetGeometricTranslation(FbxNode::eSourcePivot);
    FBXSDK_printf("        Translation: %f %f %f\n", lTmpVector[0], lTmpVector[1], lTmpVector[2]);

    //
    // Rotation
    //
    lTmpVector = pNode->GetGeometricRotation(FbxNode::eSourcePivot);
    FBXSDK_printf("        Rotation:    %f %f %f\n", lTmpVector[0], lTmpVector[1], lTmpVector[2]);

    //
    // Scaling
    //
    lTmpVector = pNode->GetGeometricScaling(FbxNode::eSourcePivot);
    FBXSDK_printf("        Scaling:     %f %f %f\n", lTmpVector[0], lTmpVector[1], lTmpVector[2]);
}

void DisplayMarker(FbxNode* pNode)
{
    FbxMarker* lMarker = (FbxMarker*)pNode->GetNodeAttribute();
    FbxString     lString;

    DisplayString("Marker Name: ", (char *) pNode->GetName());
    DisplayMetaDataConnections(lMarker);

    // Type
    lString = "    Marker Type: ";
    switch (lMarker->GetType())
    {
    case FbxMarker::eStandard:    lString += "Standard";    break;
    case FbxMarker::eOptical:     lString += "Optical";     break;
    case FbxMarker::eEffectorIK: lString += "IK Effector"; break;
    case FbxMarker::eEffectorFK: lString += "FK Effector"; break;
    }
    DisplayString(lString.Buffer());

    // Look
    lString = "    Marker Look: ";
    switch (lMarker->Look.Get())
    {
    default:
      break;
    case FbxMarker::eCube:         lString += "Cube";        break;
    case FbxMarker::eHardCross:   lString += "Hard Cross";  break;
    case FbxMarker::eLightCross:  lString += "Light Cross"; break;
    case FbxMarker::eSphere:       lString += "Sphere";      break;
    }
    DisplayString(lString.Buffer());

    // Size
    lString = FbxString("    Size: ") + FbxString(lMarker->Size.Get());
    DisplayString(lString.Buffer());

    // Color
    FbxDouble3 c = lMarker->Color.Get();
    FbxColor color(c[0], c[1], c[2]);
    DisplayColor("    Color: ", color);

    // IKPivot
    Display3DVector("    IKPivot: ", lMarker->IKPivot.Get());
}

void DisplayLodGroup(FbxNode* pNode)
{
    const char* lDisplayLevels[] = { "UseLOD", "Show", "Hide" };

    DisplayString("LodGroup Name: ", (char *) pNode->GetName());

    DisplayInt("    ", pNode->GetChildCount(), " Geometries");
    for (int i = 0; i < pNode->GetChildCount(); i++)
    {
        FbxNode* lChildNode = pNode->GetChild(i);
        DisplayString("        ", lChildNode->GetName());
    }

    FbxLODGroup *lLodGroupAttr = (FbxLODGroup*)pNode->GetNodeAttribute();
    DisplayBool("    MinMaxDistance Enabled: ", lLodGroupAttr->MinMaxDistance.Get());
    if (lLodGroupAttr->MinMaxDistance.Get())
    {
        DisplayDouble("        Min Distance: ", lLodGroupAttr->MinDistance.Get());
        DisplayDouble("        Max Distance: ", lLodGroupAttr->MaxDistance.Get());
    }
    DisplayBool("    Is World Space: ", lLodGroupAttr->WorldSpace.Get());

    DisplayString("    Thresholds ");
    for (int i = 0; i < lLodGroupAttr->GetNumThresholds(); i++)
    {
        FbxDistance lThreshVal;
        if (lLodGroupAttr->GetThreshold(i, lThreshVal))
            DisplayDouble("        ", lThreshVal.value());
    }

    DisplayString("    DisplayLevels");
    for (int i = 0; i < lLodGroupAttr->GetNumDisplayLevels(); i++)
    {
        FbxLODGroup::EDisplayLevel lLevel;
        if (lLodGroupAttr->GetDisplayLevel(i, lLevel))
            DisplayString("        ", lDisplayLevels[lLevel]);
    }
}

void DisplayPatch(FbxNode* pNode)
{
    FbxPatch* lPatch = (FbxPatch*) pNode->GetNodeAttribute();

    DisplayString("Patch Name: ", (char *) pNode->GetName());
    DisplayMetaDataConnections(lPatch);

    const char* lSurfaceModes[] = { "Raw", "Low No Normals", "Low", "High No Normals", "High" };

    DisplayString("    Surface Mode: ", lSurfaceModes[lPatch->GetSurfaceMode()]);

    int i, lControlPointsCount = lPatch->GetControlPointsCount();
    FbxVector4* lControlPoints = lPatch->GetControlPoints();

    for (i = 0; i < lControlPointsCount; i++)
    {
        DisplayInt("    Control Point ", i);
        Display3DVector("        Coordinates: ", lControlPoints[i]);
        DisplayDouble("        Weight: ", lControlPoints[i][3]);
    }

    const char* lPatchTypes[] = { "Bezier", "Bezier Quadric", "Cardinal", "B-Spline", "Linear" };

    DisplayString("    Patch U Type: ", lPatchTypes[lPatch->GetPatchUType()]);
    DisplayInt("    U Count: ", lPatch->GetUCount());
    DisplayString("    Patch V Type: ", lPatchTypes[lPatch->GetPatchVType()]);
    DisplayInt("    V Count: ", lPatch->GetVCount());
    DisplayInt("    U Step: ", lPatch->GetUStep());
    DisplayInt("    V Step: ", lPatch->GetVStep());
    DisplayBool("    U Closed: ", lPatch->GetUClosed());
    DisplayBool("    V Closed: ", lPatch->GetVClosed());
    DisplayBool("    U Capped Top: ", lPatch->GetUCappedTop());
    DisplayBool("    U Capped Bottom: ", lPatch->GetUCappedBottom());
    DisplayBool("    V Capped Top: ", lPatch->GetVCappedTop());
    DisplayBool("    V Capped Bottom: ", lPatch->GetVCappedBottom());

    DisplayString("");

    DisplayTexture(lPatch);
    DisplayMaterial(lPatch);
    DisplayLink(lPatch);
    DisplayShape(lPatch);
}


void DisplayContent(FbxNode* pNode)
{
    FbxNodeAttribute::EType lAttributeType;
    int i;

    if (pNode->GetNodeAttribute() == NULL)
    {
        FBXSDK_printf("NULL Node Attribute\n\n");
    }
    else
    {
        lAttributeType = (pNode->GetNodeAttribute()->GetAttributeType());

        switch (lAttributeType)
        {
        default:
            break;
        case FbxNodeAttribute::eMarker:
            DisplayMarker(pNode);
            break;

        case FbxNodeAttribute::eSkeleton:
            DisplaySkeleton(pNode);
            break;

        case FbxNodeAttribute::eMesh:
            DisplayMesh(pNode);
            break;

        case FbxNodeAttribute::eNurbs:
            //DisplayNurb(pNode);
            break;

        case FbxNodeAttribute::ePatch:
            DisplayPatch(pNode);
            break;

        case FbxNodeAttribute::eCamera:
            DisplayCamera(pNode);
            break;

        case FbxNodeAttribute::eLight:
            DisplayLight(pNode);
            break;

        case FbxNodeAttribute::eLODGroup:
            DisplayLodGroup(pNode);
            break;
        }
    }

    DisplayUserProperties(pNode);
    DisplayTarget(pNode);
    DisplayPivotsAndLimits(pNode);
    DisplayTransformPropagation(pNode);
    DisplayGeometricTransform(pNode);

    for (i = 0; i < pNode->GetChildCount(); i++)
    {
        DisplayContent(pNode->GetChild(i));
    }
}

void DisplayPose(FbxScene* pScene)
{
    int      i,j,k,lPoseCount;
    FbxString  lName;

    lPoseCount = pScene->GetPoseCount();

    for (i = 0; i < lPoseCount; i++)
    {
        FbxPose* lPose = pScene->GetPose(i);

        lName = lPose->GetName();
        DisplayString("Pose Name: ", lName.Buffer());

        DisplayBool("    Is a bind pose: ", lPose->IsBindPose());

        DisplayInt("    Number of items in the pose: ", lPose->GetCount());

        DisplayString("","");

        for (j=0; j<lPose->GetCount(); j++)
        {
            lName = lPose->GetNodeName(j).GetCurrentName();
            DisplayString("    Item name: ", lName.Buffer());

            if (!lPose->IsBindPose())
            {
                // Rest pose can have local matrix
                DisplayBool("    Is local space matrix: ", lPose->IsLocalMatrix(j));
            }

            DisplayString("    Matrix value: ","");

            FbxString lMatrixValue;

            for (k=0; k<4; k++)
            {
                FbxMatrix  lMatrix = lPose->GetMatrix(j);
                FbxVector4 lRow = lMatrix.GetRow(k);
                char        lRowValue[1024];

                FBXSDK_sprintf(lRowValue, 1024, "%9.4f %9.4f %9.4f %9.4f\n", lRow[0], lRow[1], lRow[2], lRow[3]);
                lMatrixValue += FbxString("        ") + FbxString(lRowValue);
            }

            DisplayString("", lMatrixValue.Buffer());
        }
    }

    lPoseCount = pScene->GetCharacterPoseCount();

    for (i = 0; i < lPoseCount; i++)
    {
        FbxCharacterPose* lPose = pScene->GetCharacterPose(i);
        FbxCharacter*     lCharacter = lPose->GetCharacter();

        if (!lCharacter) break;

        DisplayString("Character Pose Name: ", lCharacter->GetName());

        FbxCharacterLink lCharacterLink;
        FbxCharacter::ENodeId  lNodeId = FbxCharacter::eHips;

        while (lCharacter->GetCharacterLink(lNodeId, &lCharacterLink))
        {
            FbxAMatrix& lGlobalPosition = lCharacterLink.mNode->EvaluateGlobalTransform(FBXSDK_TIME_ZERO);

            DisplayString("    Matrix value: ","");

            FbxString lMatrixValue;

            for (k=0; k<4; k++)
            {
                FbxVector4 lRow = lGlobalPosition.GetRow(k);
                char        lRowValue[1024];

                FBXSDK_sprintf(lRowValue, 1024, "%9.4f %9.4f %9.4f %9.4f\n", lRow[0], lRow[1], lRow[2], lRow[3]);
                lMatrixValue += FbxString("        ") + FbxString(lRowValue);
            }

            DisplayString("", lMatrixValue.Buffer());

            lNodeId = FbxCharacter::ENodeId(int(lNodeId) + 1);
        }
    }
}

void DisplayContent(FbxScene* pScene)
{
    int i;
    FbxNode* lNode = pScene->GetRootNode();

    if (lNode)
    {
        for (i = 0; i < lNode->GetChildCount(); i++)
        {
            DisplayContent(lNode->GetChild(i));
        }
    }
}

void DisplayGlobalLightSettings(FbxGlobalSettings* pGlobalSettings)
{
    DisplayColor("Ambient Color: ", pGlobalSettings->GetAmbientColor());
    DisplayString("");
}


void DisplayGlobalCameraSettings(FbxGlobalSettings* pGlobalSettings)
{
    DisplayString("Default Camera: ", pGlobalSettings->GetDefaultCamera());
    DisplayString("");
}


void DisplayGlobalTimeSettings(FbxGlobalSettings* pGlobalSettings)
{
    char lTimeString[256];

    DisplayString("Time Mode : ", FbxGetTimeModeName(pGlobalSettings->GetTimeMode()));

    FbxTimeSpan lTs;
    FbxTime     lStart, lEnd;
    pGlobalSettings->GetTimelineDefaultTimeSpan(lTs);
    lStart = lTs.GetStart();
    lEnd   = lTs.GetStop();
    DisplayString("Timeline default timespan: ");
    DisplayString("     Start: ", lStart.GetTimeString(lTimeString, FbxUShort(256)));
    DisplayString("     Stop : ", lEnd.GetTimeString(lTimeString, FbxUShort(256)));

    DisplayString("");
}

void DisplayHierarchy(FbxScene* pScene)
{
    int i;
    FbxNode* lRootNode = pScene->GetRootNode();

    for(i = 0; i < lRootNode->GetChildCount(); ++i)
    {
        DisplayHierarchy(lRootNode->GetChild(i), 0);
    }
}

void DisplayHierarchy(FbxNode* pNode, int pDepth)
{
    FbxString lString;
    int i;

    const char * nodeType;
    if (pNode->GetNodeAttribute() == NULL)
    {
        nodeType = "Node";
    }
    else
    {
        FbxNodeAttribute::EType lAttributeType = pNode->GetNodeAttribute()->GetAttributeType();
        switch(lAttributeType)
        {
        case FbxNodeAttribute::eUnknown          : nodeType = "eUnknown"; break;
        case FbxNodeAttribute::eNull             : nodeType = "eNull"; break;
        case FbxNodeAttribute::eMarker           : nodeType = "eMarker"; break;
        case FbxNodeAttribute::eSkeleton         : nodeType = "eSkeleton"; break;
        case FbxNodeAttribute::eMesh             : nodeType = "eMesh"; break;
        case FbxNodeAttribute::eNurbs            : nodeType = "eNurbs"; break;
        case FbxNodeAttribute::ePatch            : nodeType = "ePatch"; break;
        case FbxNodeAttribute::eCamera           : nodeType = "eCamera"; break;
        case FbxNodeAttribute::eCameraStereo     : nodeType = "eCameraStereo"; break;
        case FbxNodeAttribute::eCameraSwitcher   : nodeType = "eCameraSwitcher"; break;
        case FbxNodeAttribute::eLight            : nodeType = "eLight"; break;
        case FbxNodeAttribute::eOpticalReference : nodeType = "eOpticalReference"; break;
        case FbxNodeAttribute::eOpticalMarker    : nodeType = "eOpticalMarker"; break;
        case FbxNodeAttribute::eNurbsCurve       : nodeType = "eNurbsCurve"; break;
        case FbxNodeAttribute::eTrimNurbsSurface : nodeType = "eTrimNurbsSurface"; break;
        case FbxNodeAttribute::eBoundary         : nodeType = "eBoundary"; break;
        case FbxNodeAttribute::eNurbsSurface     : nodeType = "eNurbsSurface"; break;
        case FbxNodeAttribute::eShape            : nodeType = "eShape"; break;
        case FbxNodeAttribute::eLODGroup         : nodeType = "eLODGroup"; break;
        case FbxNodeAttribute::eSubDiv           : nodeType = "eSubDiv"; break;
        case FbxNodeAttribute::eCachedEffect     : nodeType = "eCachedEffect"; break;
        case FbxNodeAttribute::eLine             : nodeType = "eLine"; break;
        }
    }

    for(i = 0; i < pDepth; i++)
    {
        lString += "  ";
    }

    lString += "<";
    lString += nodeType;
    lString += " name=\"";
    lString += pNode->GetName();
    lString += "\">\n";

    FBXSDK_printf(lString.Buffer());

    for(i = 0; i < pNode->GetChildCount(); i++)
    {
        DisplayHierarchy(pNode->GetChild(i), pDepth + 1);
    }

    lString.Clear();
    for(i = 0; i < pDepth; i++)
    {
        lString += "  ";
    }
    lString += "</";
    lString += nodeType;
    lString += ">\n";
    FBXSDK_printf(lString.Buffer());
}

void DisplayTarget(FbxNode* pNode)
{
    if(pNode->GetTarget() != NULL)
    {
        DisplayString("    Target Name: ", (char *) pNode->GetTarget()->GetName());
    }
}

void DisplayMetaData(FbxScene* pScene)
{
    FbxDocumentInfo* sceneInfo = pScene->GetSceneInfo();
    if (sceneInfo)
    {
        FBXSDK_printf("\n\n--------------------\nMeta-Data\n--------------------\n\n");
        FBXSDK_printf("    Title: %s\n", sceneInfo->mTitle.Buffer());
        FBXSDK_printf("    Subject: %s\n", sceneInfo->mSubject.Buffer());
        FBXSDK_printf("    Author: %s\n", sceneInfo->mAuthor.Buffer());
        FBXSDK_printf("    Keywords: %s\n", sceneInfo->mKeywords.Buffer());
        FBXSDK_printf("    Revision: %s\n", sceneInfo->mRevision.Buffer());
        FBXSDK_printf("    Comment: %s\n", sceneInfo->mComment.Buffer());

        FbxThumbnail* thumbnail = sceneInfo->GetSceneThumbnail();
        if (thumbnail)
        {
            FBXSDK_printf("    Thumbnail:\n");

            switch (thumbnail->GetDataFormat())
            {
            case FbxThumbnail::eRGB_24:
                FBXSDK_printf("        Format: RGB\n");
                break;
            case FbxThumbnail::eRGBA_32:
                FBXSDK_printf("        Format: RGBA\n");
                break;
            }

            switch (thumbnail->GetSize())
            {
            default:
                break;
            case FbxThumbnail::eNotSet:
                FBXSDK_printf("        Size: no dimensions specified (%ld bytes)\n", thumbnail->GetSizeInBytes());
                break;
            case FbxThumbnail::e64x64:
                FBXSDK_printf("        Size: 64 x 64 pixels (%ld bytes)\n", thumbnail->GetSizeInBytes());
                break;
            case FbxThumbnail::e128x128:
                FBXSDK_printf("        Size: 128 x 128 pixels (%ld bytes)\n", thumbnail->GetSizeInBytes());
            }
        }
    }
}

void DisplayMetaDataConnections(FbxObject* pObject)
{
    int nbMetaData = pObject->GetSrcObjectCount<FbxObjectMetaData>();
    if (nbMetaData > 0)
        DisplayString("    MetaData connections ");

    for (int i = 0; i < nbMetaData; i++)
    {
        FbxObjectMetaData* metaData = pObject->GetSrcObject<FbxObjectMetaData>(i);
        DisplayString("        Name: ", (char*)metaData->GetName());
    }
}

void DisplayString(const char* pHeader, const char* pValue /* = "" */, const char* pSuffix /* = "" */)
{
    FbxString lString;

    lString = pHeader;
    lString += pValue;
    lString += pSuffix;
    lString += "\n";
    FBXSDK_printf(lString);
}


void DisplayBool(const char* pHeader, bool pValue, const char* pSuffix /* = "" */)
{
    FbxString lString;

    lString = pHeader;
    lString += pValue ? "true" : "false";
    lString += pSuffix;
    lString += "\n";
    FBXSDK_printf(lString);
}


void DisplayInt(const char* pHeader, int pValue, const char* pSuffix /* = "" */)
{
    FbxString lString;

    lString = pHeader;
    lString += pValue;
    lString += pSuffix;
    lString += "\n";
    FBXSDK_printf(lString);
}


void DisplayDouble(const char* pHeader, double pValue, const char* pSuffix /* = "" */)
{
    FbxString lString;
    FbxString lFloatValue = (float) pValue;

    lFloatValue = pValue <= -HUGE_VAL ? "-INFINITY" : lFloatValue.Buffer();
    lFloatValue = pValue >=  HUGE_VAL ?  "INFINITY" : lFloatValue.Buffer();

    lString = pHeader;
    lString += lFloatValue;
    lString += pSuffix;
    lString += "\n";
    FBXSDK_printf(lString);
}


void Display2DVector(const char* pHeader, FbxVector2 pValue, const char* pSuffix  /* = "" */)
{
    FbxString lString;
    FbxString lFloatValue1 = (float)pValue[0];
    FbxString lFloatValue2 = (float)pValue[1];

    lFloatValue1 = pValue[0] <= -HUGE_VAL ? "-INFINITY" : lFloatValue1.Buffer();
    lFloatValue1 = pValue[0] >=  HUGE_VAL ?  "INFINITY" : lFloatValue1.Buffer();
    lFloatValue2 = pValue[1] <= -HUGE_VAL ? "-INFINITY" : lFloatValue2.Buffer();
    lFloatValue2 = pValue[1] >=  HUGE_VAL ?  "INFINITY" : lFloatValue2.Buffer();

    lString = pHeader;
    lString += lFloatValue1;
    lString += ", ";
    lString += lFloatValue2;
    lString += pSuffix;
    lString += "\n";
    FBXSDK_printf(lString);
}


void Display3DVector(const char* pHeader, FbxVector4 pValue, const char* pSuffix /* = "" */)
{
    FbxString lString;
    FbxString lFloatValue1 = (float)pValue[0];
    FbxString lFloatValue2 = (float)pValue[1];
    FbxString lFloatValue3 = (float)pValue[2];

    lFloatValue1 = pValue[0] <= -HUGE_VAL ? "-INFINITY" : lFloatValue1.Buffer();
    lFloatValue1 = pValue[0] >=  HUGE_VAL ?  "INFINITY" : lFloatValue1.Buffer();
    lFloatValue2 = pValue[1] <= -HUGE_VAL ? "-INFINITY" : lFloatValue2.Buffer();
    lFloatValue2 = pValue[1] >=  HUGE_VAL ?  "INFINITY" : lFloatValue2.Buffer();
    lFloatValue3 = pValue[2] <= -HUGE_VAL ? "-INFINITY" : lFloatValue3.Buffer();
    lFloatValue3 = pValue[2] >=  HUGE_VAL ?  "INFINITY" : lFloatValue3.Buffer();

    lString = pHeader;
    lString += lFloatValue1;
    lString += ", ";
    lString += lFloatValue2;
    lString += ", ";
    lString += lFloatValue3;
    lString += pSuffix;
    lString += "\n";
    FBXSDK_printf(lString);
}

void Display4DVector(const char* pHeader, FbxVector4 pValue, const char* pSuffix /* = "" */)
{
    FbxString lString;
    FbxString lFloatValue1 = (float)pValue[0];
    FbxString lFloatValue2 = (float)pValue[1];
    FbxString lFloatValue3 = (float)pValue[2];
    FbxString lFloatValue4 = (float)pValue[3];

    lFloatValue1 = pValue[0] <= -HUGE_VAL ? "-INFINITY" : lFloatValue1.Buffer();
    lFloatValue1 = pValue[0] >=  HUGE_VAL ?  "INFINITY" : lFloatValue1.Buffer();
    lFloatValue2 = pValue[1] <= -HUGE_VAL ? "-INFINITY" : lFloatValue2.Buffer();
    lFloatValue2 = pValue[1] >=  HUGE_VAL ?  "INFINITY" : lFloatValue2.Buffer();
    lFloatValue3 = pValue[2] <= -HUGE_VAL ? "-INFINITY" : lFloatValue3.Buffer();
    lFloatValue3 = pValue[2] >=  HUGE_VAL ?  "INFINITY" : lFloatValue3.Buffer();
    lFloatValue4 = pValue[3] <= -HUGE_VAL ? "-INFINITY" : lFloatValue4.Buffer();
    lFloatValue4 = pValue[3] >=  HUGE_VAL ?  "INFINITY" : lFloatValue4.Buffer();

    lString = pHeader;
    lString += lFloatValue1;
    lString += ", ";
    lString += lFloatValue2;
    lString += ", ";
    lString += lFloatValue3;
    lString += ", ";
    lString += lFloatValue4;
    lString += pSuffix;
    lString += "\n";
    FBXSDK_printf(lString);
}

void DisplayColor(const char* pHeader, FbxColor pValue, const char* pSuffix /* = "" */)
{
    FbxString lString;

    lString = pHeader;
    lString += (float) pValue.mRed;

    lString += " (red), ";
    lString += (float) pValue.mGreen;

    lString += " (green), ";
    lString += (float) pValue.mBlue;

    lString += " (blue)";
    lString += pSuffix;
    lString += "\n";
    FBXSDK_printf(lString);
}

bool LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename)
{
    int lFileMajor, lFileMinor, lFileRevision;
    int lSDKMajor,  lSDKMinor,  lSDKRevision;
    //int lFileFormat = -1;
    int i, lAnimStackCount;
    bool lStatus;
    char lPassword[1024];

    // Get the file version number generate by the FBX SDK.
    FbxManager::GetFileFormatVersion(lSDKMajor, lSDKMinor, lSDKRevision);

    // Create an importer.
    FbxImporter* lImporter = FbxImporter::Create(pManager,"");

    // Initialize the importer by providing a filename.
    const bool lImportStatus = lImporter->Initialize(pFilename, -1, pManager->GetIOSettings());
    lImporter->GetFileVersion(lFileMajor, lFileMinor, lFileRevision);

    if( !lImportStatus )
    {
        FbxString error = lImporter->GetStatus().GetErrorString();
        FBXSDK_printf("Call to FbxImporter::Initialize() failed.\n");
        FBXSDK_printf("Error returned: %s\n\n", error.Buffer());

        if (lImporter->GetStatus().GetCode() == FbxStatus::eInvalidFileVersion)
        {
            FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);
            FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", pFilename, lFileMajor, lFileMinor, lFileRevision);
        }

        return false;
    }

    FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);

    if (lImporter->IsFBX())
    {
        FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", pFilename, lFileMajor, lFileMinor, lFileRevision);

        // From this point, it is possible to access animation stack information without
        // the expense of loading the entire file.

        FBXSDK_printf("Animation Stack Information\n");

        lAnimStackCount = lImporter->GetAnimStackCount();

        FBXSDK_printf("    Number of Animation Stacks: %d\n", lAnimStackCount);
        FBXSDK_printf("    Current Animation Stack: \"%s\"\n", lImporter->GetActiveAnimStackName().Buffer());
        FBXSDK_printf("\n");

        for(i = 0; i < lAnimStackCount; i++)
        {
            FbxTakeInfo* lTakeInfo = lImporter->GetTakeInfo(i);

            FBXSDK_printf("    Animation Stack %d\n", i);
            FBXSDK_printf("         Name: \"%s\"\n", lTakeInfo->mName.Buffer());
            FBXSDK_printf("         Description: \"%s\"\n", lTakeInfo->mDescription.Buffer());

            // Change the value of the import name if the animation stack should be imported
            // under a different name.
            FBXSDK_printf("         Import Name: \"%s\"\n", lTakeInfo->mImportName.Buffer());

            // Set the value of the import state to false if the animation stack should be not
            // be imported.
            FBXSDK_printf("         Import State: %s\n", lTakeInfo->mSelect ? "true" : "false");
            FBXSDK_printf("\n");
        }

        // Set the import states. By default, the import states are always set to
        // true. The code below shows how to change these states.
        IOS_REF.SetBoolProp(IMP_FBX_MATERIAL,        true);
        IOS_REF.SetBoolProp(IMP_FBX_TEXTURE,         true);
        IOS_REF.SetBoolProp(IMP_FBX_LINK,            true);
        IOS_REF.SetBoolProp(IMP_FBX_SHAPE,           true);
        IOS_REF.SetBoolProp(IMP_FBX_GOBO,            true);
        IOS_REF.SetBoolProp(IMP_FBX_ANIMATION,       true);
        IOS_REF.SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
    }

    // Import the scene.
    lStatus = lImporter->Import(pScene);

    if(lStatus == false && lImporter->GetStatus().GetCode() == FbxStatus::ePasswordError)
    {
        FBXSDK_printf("Please enter password: ");

        lPassword[0] = '\0';

        FBXSDK_CRT_SECURE_NO_WARNING_BEGIN
        scanf("%s", lPassword);
        FBXSDK_CRT_SECURE_NO_WARNING_END

        FbxString lString(lPassword);

        IOS_REF.SetStringProp(IMP_FBX_PASSWORD,      lString);
        IOS_REF.SetBoolProp(IMP_FBX_PASSWORD_ENABLE, true);

        lStatus = lImporter->Import(pScene);

        if(lStatus == false && lImporter->GetStatus().GetCode() == FbxStatus::ePasswordError)
        {
            FBXSDK_printf("\nPassword is wrong, import aborted.\n");
        }
    }

    // Destroy the importer.
    lImporter->Destroy();

    return lStatus;
}
