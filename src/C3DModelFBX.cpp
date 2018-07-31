#include "C3DModelFBX.h"

#include "FBXHelper.h"

#define HAS_STB_IMAGE 0

#if HAS_STB_IMAGE
//#include "stb_image.h" // to test image format that are incorrectly named.
#define STB_IMAGE_IMPLEMENTATION
#include "../../../core/dist/src/thirdparty/stb_image.h" // to test image format that are incorrectly named.
#endif

#define LOG_ERROR(...) printf("C3DModelFBX:"); printf(__VA_ARGS__);
#define LOG_INFO(...) printf("C3DModelFBX:"); printf(__VA_ARGS__);

#ifdef _WIN32
    #define PATH_SEP '\\'
#else
    #define PATH_SEP '/'
#endif

#define STU_EXPORT_SEQUENTIAL 1 //When enabled we write to the file at each model (much better memory usage, but may be slightly slower)

#define WRITE_VALUE(x)       do {Data.insert(Data.end(), (uint8_t *)&(x), (uint8_t *)&(x) + sizeof(x)); uSize += sizeof(x);} while((void)0,0)
#define WRITE_VALUES(x, c)   do {Data.insert(Data.end(), (uint8_t *)&(x), (uint8_t *)&(x) + sizeof(x) * (c)); uSize += sizeof(x) * (c);} while((void)0,0)
#define WRITE_BYTES(x, y, c) do {Data.insert(Data.end(), (uint8_t *)(x), (uint8_t *)(x) + sizeof(y) * (c)); uSize += sizeof(y) * (c);} while((void)0,0)

static inline glm::mat4 ConvertFbxToGLM(const FbxAMatrix &m)
{
    glm::mat4 matrix;
    for(int row = 0; row < 4; ++row)
    {
        for(int col = 0; col < 4; ++col)
        {
            matrix[row][col] = (float)m.Get(row, col);
        }
    }

    return matrix;
}

static inline glm::quat ConvertFbxToGLM(const FbxQuaternion &q)
{
    glm::quat quat((float)q[3], (float)q[0], (float)q[1], (float)q[2]);
    return quat;
}


static inline FbxAMatrix GetGeometryTransformation(FbxNode* inNode)
{
    const FbxVector4 T = inNode->GetGeometricTranslation(FbxNode::eSourcePivot);
    const FbxVector4 R = inNode->GetGeometricRotation(FbxNode::eSourcePivot);
    const FbxVector4 S = inNode->GetGeometricScaling(FbxNode::eSourcePivot);
    return FbxAMatrix(T, R, S);
}

static inline FbxFileTexture *GetMaterialFileTexture(const FbxSurfaceMaterial *pMaterial, const char *pPropertyName)
{
    if (pMaterial)
    {
        const FbxProperty property = pMaterial->FindProperty(pPropertyName);
        if (property.IsValid())
        {
            int layeredTextureCount = property.GetSrcObjectCount<FbxLayeredTexture>();

            if (layeredTextureCount > 0)
            {
                for (int j = 0; j < layeredTextureCount; j++)
                {
                    FbxLayeredTexture* pLayeredTexture = property.GetSrcObject<FbxLayeredTexture>(j);
                    int fileTextureCount = pLayeredTexture->GetSrcObjectCount<FbxFileTexture>();
                    if (fileTextureCount > 0)
                    {
                        return pLayeredTexture->GetSrcObject<FbxFileTexture>(0);
                    }
                }
            }
            else
            {
                // Directly get textures
                int fileTextureCount = property.GetSrcObjectCount<FbxFileTexture>();
                if (fileTextureCount > 0)
                {
                    return property.GetSrcObject<FbxFileTexture>(0);
                }
            }
        }
    }
    return YI_NULL;
}


static inline bool GetMaterialColor(glm::vec3 &result, const FbxSurfaceMaterial *pMaterial, const char *pPropertyName, const char *pFactorPropertyName)
{
    bool rc = false;
    if (pMaterial)
    {
        const FbxProperty property = pMaterial->FindProperty(pPropertyName);
        if (property.IsValid())
        {
            FbxDouble3 values = property.Get<FbxDouble3>();
            result.x = (float)values[0];
            result.y = (float)values[1];
            result.z = (float)values[2];

            const FbxProperty lFactorProperty = pMaterial->FindProperty(pFactorPropertyName);
            if(lFactorProperty.IsValid())
            {
                result *= static_cast<float>(lFactorProperty.Get<FbxDouble>());
            }

            rc = true;
        }
    }
    return rc;
}

static inline bool GetMaterialValue(float &result, const FbxSurfaceMaterial *pMaterial, const char *pPropertyName)
{
    bool rc = false;
    if (pMaterial)
    {
        const FbxProperty property = pMaterial->FindProperty(pPropertyName);
        if (property.IsValid())
        {
            result = static_cast<float>(property.Get<FbxDouble>());
            rc = true;
        }
    }
    return rc;
}

static inline bool ProcessFileTexture(std::string &result, const FbxString &destinationFolder, const FbxFileTexture *pFileTexture)
{
    bool rc = false;
    if (pFileTexture)
    {
        FbxString currrentFilePath = FbxPathUtils::Bind(destinationFolder, pFileTexture->GetRelativeFileName());

        FILE *srcFile = fopen(currrentFilePath, "rb");
        if (!srcFile)
        {
            std::string sFile = CFileExportSTUFormat::RemoveFoldersFromPaths(std::string(currrentFilePath));
            std::string sFullname = destinationFolder + PATH_SEP + sFile.c_str();

            srcFile = fopen(sFullname.c_str(), "rb");
            if (srcFile)
            {
                currrentFilePath = sFullname.c_str();
            }
            //SR: Anything else we can try? Add them here...
        }
        if (srcFile)
        {
#if HAS_STB_IMAGE
            stbi__context ctx;
            stbi__start_file(&ctx, srcFile);
#endif

            // file name without the path.
            FbxString srcFileName = FbxPathUtils::GetFileName(pFileTexture->GetFileName());

#if HAS_STB_IMAGE
            // rename the extension (in case it was incorrect)
            if (stbi__jpeg_test(&ctx) == 1)
            { 
                if (FbxPathUtils::GetExtensionName(srcFileName) != "jpg")
                {
                    srcFileName = FbxPathUtils::ChangeExtension(srcFileName, ".jpg");
                }
            }
            else if (stbi__png_test(&ctx) == 1)
            {
                if (FbxPathUtils::GetExtensionName(srcFileName) != "png")
                {
                    srcFileName = FbxPathUtils::ChangeExtension(srcFileName, ".png");
                }
            }
            else if (stbi__bmp_test(&ctx) == 1)
            { 
                if (FbxPathUtils::GetExtensionName(srcFileName) != "bmp")
                {
                    srcFileName = FbxPathUtils::ChangeExtension(srcFileName, ".bmp");
                }
            }
            else if (stbi__tga_test(&ctx) == 1)
            { 
                if (FbxPathUtils::GetExtensionName(srcFileName) != "tga")
                {
                    srcFileName = FbxPathUtils::ChangeExtension(srcFileName, ".tga");
                }
            }
#endif
            fclose(srcFile);

            result = srcFileName;

            FbxString destFilePath = destinationFolder + PATH_SEP + srcFileName;
            if (currrentFilePath != destFilePath)
            {
                if (std::rename(currrentFilePath, destFilePath) != 0)
                {
                    // that can't be happening. Abort() using YI_LOGF.
                    LOG_ERROR("Could not move / rename %s to %s", currrentFilePath.Buffer(), destFilePath.Buffer());
                }
                LOG_ERROR("**** FOUND %s\n     moved/renamed to %s", currrentFilePath.Buffer(), destFilePath.Buffer());
            }
            else
            {
                LOG_ERROR("**** FOUND %s", currrentFilePath.Buffer());
            }
            rc = true;
        }
        else
        {
            LOG_ERROR("Could non locate texture: %s", currrentFilePath.Buffer());
        }
    }
    return rc;
}

static inline bool GetMaterialTexturePath(std::string &result, const char * pFbxFileName, const FbxSurfaceMaterial *pMaterial, const char *pPropertyName)
{
    bool rc = false;
    if (pMaterial)
    {
        const FbxProperty property = pMaterial->FindProperty(pPropertyName);
        if (property.IsValid())
        {
            const FbxString fbxFilePath = FbxPathUtils::Resolve(pFbxFileName);
            const FbxString destinationFolder = FbxPathUtils::GetFolderName(fbxFilePath);

            // Check if it's layeredtextures
            int layeredTextureCount = property.GetSrcObjectCount<FbxLayeredTexture>();

            if (layeredTextureCount > 0)
            {
                for (int j = 0; j < layeredTextureCount; j++)
                {
                    FbxLayeredTexture* pLayeredTexture = property.GetSrcObject<FbxLayeredTexture>(j);
                    int fileTextureCount = pLayeredTexture->GetSrcObjectCount<FbxFileTexture>();

                    for (int k = 0; k < fileTextureCount; k++)
                    {
                        FbxFileTexture* pFileTexture = pLayeredTexture->GetSrcObject<FbxFileTexture>(k);
                        rc = ProcessFileTexture(result, destinationFolder, pFileTexture);
                    }
                }
            }
            else
            {
                // Directly get textures
                int fileTextureCount = property.GetSrcObjectCount<FbxFileTexture>();
                for (int j = 0; j < fileTextureCount; j++)
                {
                    FbxFileTexture* pFileTexture = property.GetSrcObject<FbxFileTexture>(j);
                    rc = ProcessFileTexture(result, destinationFolder, pFileTexture);
                }
            }
        }
    }
    return rc;
}

static inline FbxAMatrix GetGeometry(FbxNode* pNode)
{
    const FbxVector4 lT = pNode->GetGeometricTranslation(FbxNode::eSourcePivot);
    const FbxVector4 lR = pNode->GetGeometricRotation(FbxNode::eSourcePivot);
    const FbxVector4 lS = pNode->GetGeometricScaling(FbxNode::eSourcePivot);

    return FbxAMatrix(lT, lR, lS);
}

C3DModelFBX::C3DModelFBX() :
  m_uNumBones(0),
  m_bHasAnimations(false)
{
    m_pFBXManager = NULL;
    m_pFBXScene = NULL;

    // Prepare the FBX SDK.
    InitializeSdkObjects(m_pFBXManager, m_pFBXScene);

}

C3DModelFBX::~C3DModelFBX()
{
    // Destroy all objects created by the FBX SDK.
    DestroySdkObjects(m_pFBXManager, true);
}

bool C3DModelFBX::ExportToSTUFormat(const std::string &path)
{
    return ExportToSTUFormat(path, true);
}

bool C3DModelFBX::ExportToSTUFormat(const std::string &path, bool bFlipUV)
{
    m_path = path;

    m_sSTUPath = path;
    m_sSTUPath.append(".stu");
    std::remove(m_sSTUPath.c_str());

    if (!LoadScene(m_pFBXManager, m_pFBXScene, m_path.c_str()))
    {
        LOG_ERROR("An error occurred while loading the scene...");
        return false;
    }

    // Convert Axis System, if needed
    FbxAxisSystem SceneAxisSystem = m_pFBXScene->GetGlobalSettings().GetAxisSystem();
    FbxAxisSystem OurAxisSystem(FbxAxisSystem::eYAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eRightHanded);
    if( SceneAxisSystem != OurAxisSystem )
    {
        OurAxisSystem.ConvertScene(m_pFBXScene);
    }

//    // Convert Unit System to what is used in this example, if needed
//    FbxSystemUnit SceneSystemUnit = m_pFBXScene->GetGlobalSettings().GetSystemUnit();
//    if(glm::epsilonNotEqual(SceneSystemUnit.GetScaleFactor(), FbxSystemUnit::m.GetScaleFactor(), glm::epsilon<double>()) )
//    {
//        //Convert to meters (for VR)
//        FbxSystemUnit::m.ConvertScene(m_pFBXScene);
//    }

    // Get the list of all the animation stack.
    m_pFBXScene->FillAnimStackNameArray(m_AnimStackNameArray);

    // Convert mesh, NURBS and patch into triangle mesh, and split the mesh per
    // material so that we only have one material per mesh.
    FbxGeometryConverter lGeomConverter(m_pFBXManager);
    lGeomConverter.Triangulate(m_pFBXScene,true);
    lGeomConverter.SplitMeshesPerMaterial(m_pFBXScene, true);

    /***************************************************************************
     *SR: These are not REALLY needed, but may useful for info or debugging
     **************************************************************************/

#if 0
    // Display the scene.
    DisplayMetaData(m_pFBXScene);
#endif

#if 0
    FBXSDK_printf("\n\n---------------------\nGlobal Light Settings\n---------------------\n\n");
    DisplayGlobalLightSettings(&m_pFBXScene->GetGlobalSettings());
#endif

#if 0
    FBXSDK_printf("\n\n----------------------\nGlobal Camera Settings\n----------------------\n\n");
    DisplayGlobalCameraSettings(&m_pFBXScene->GetGlobalSettings());
#endif

#if 0
    FBXSDK_printf("\n\n--------------------\nGlobal Time Settings\n--------------------\n\n");
    DisplayGlobalTimeSettings(&m_pFBXScene->GetGlobalSettings());
#endif

#if 0
    FBXSDK_printf("\n\n---------\nHierarchy\n---------\n\n");
    DisplayHierarchy(m_pFBXScene);
#endif

#if 0
    FBXSDK_printf("\n\n------------\nNode Content\n------------\n\n");
    DisplayContent(m_pFBXScene);
#endif

#if 0
    FBXSDK_printf("\n\n----\nPose\n----\n\n");
    DisplayPose(m_pFBXScene);
#endif

#if 0
    FBXSDK_printf("\n\n---------\nAnimation\n---------\n\n");
    DisplayAnimation(m_pFBXScene);
#endif

#if 0
    FBXSDK_printf("\n\n---------\nGeneric Information\n---------\n\n");
    DisplayGenericInfo(m_pFBXScene);
#endif

    m_bFlipUVonY = bFlipUV;

    ParseSkeletons();

    if (m_fbxSkeletons.size() > 0)
    {
        m_VertexDataType = VertexDataType_Bones;
    }

    m_bHasAnimations = ParseAnimations(); // MC: TODO

    if (m_bHasAnimations)
    {
        ExportAnimations();
    }

    ExportTextures();
    ExportSceneTree();

    if (m_fbxSkeletons.size() > 0)
    {
        ExportBones();
    }

#if !STU_EXPORT_SEQUENTIAL
    m_Export.ExportFile(m_sSTUPath);
#endif

    return true;
}

void C3DModelFBX::ParseSkeletons()
{
    m_fbxSkinMeshes.clear();
    m_fbxSkeletons.clear();
    ParseSubSkeletons(m_pFBXScene->GetRootNode());
}

void C3DModelFBX::ParseSubSkeletons(FbxNode* pNode)
{

    for(int i = 0; i < pNode->GetNodeAttributeCount(); i++)
    {
        FbxNodeAttribute *attr = pNode->GetNodeAttributeByIndex(i);
        FbxNodeAttribute::EType attr_type = attr->GetAttributeType();

        switch(attr_type)
        {
        case FbxNodeAttribute::eMesh:
        {
            FbxMesh *pMesh = (FbxMesh*)attr;
            if (pMesh->GetDeformerCount(FbxDeformer::eSkin) > 0)
            {
//                DisplayLink((FbxMesh*)attr);
                m_fbxSkinMeshes.push_back((FbxMesh*)attr); // we use this one later for getting the bone weights out of the deformer (skin)
            }
            break;
        }
        case FbxNodeAttribute::eSkeleton:
        {
//            DisplaySkeleton(pNode);
            m_fbxSkeletons.push_back((FbxSkeleton*)attr);
            break;
        }
        default:
            break;
        }
    }

    int childCount = pNode->GetChildCount();
    for (int childIndex = 0; childIndex < childCount; ++childIndex)
    {
        ParseSubSkeletons(pNode->GetChild(childIndex));
    }
}

bool C3DModelFBX::ParseAnimations()
{
    bool bFoundAnimation = false;

    const int animStackCount = m_AnimStackNameArray.GetCount();
    for(int i = 0 ; i < animStackCount; ++i)
    {
        FbxString animStackName = *m_AnimStackNameArray[i];
        FbxAnimStack *pAnimStack = m_pFBXScene->FindMember<FbxAnimStack>(animStackName.Buffer());

        FbxTimeSpan timeSpan;

        FbxTakeInfo* pCurrentTakeInfo = m_pFBXScene->GetTakeInfo(animStackName);
        if (pCurrentTakeInfo)
        {
            timeSpan = pCurrentTakeInfo->mLocalTimeSpan;
        }
        else
        {
            m_pFBXScene->GetGlobalSettings().GetTimelineDefaultTimeSpan(timeSpan);
        }

        FbxTime duration = timeSpan.GetDuration();
        FbxTime start = timeSpan.GetStart();
        FbxTime end = timeSpan.GetStop();

        int animLayerCount = pAnimStack->GetMemberCount<FbxAnimLayer>();
        for (int animLayerId = 0; animLayerId < animLayerCount; ++animLayerId)
        {
            FbxAnimLayer* pAnimLayer = pAnimStack->GetMember<FbxAnimLayer>(animLayerId);

            mAnimations.resize(mAnimations.size()+1);
            Animation &animation = mAnimations.back();
            if (animLayerCount > 1)
            {
                animation.mName = animStackName + ".Layer" + std::to_string(animLayerId).c_str();
            }
            else
            {
                animation.mName = animStackName ;
            }
            animation.mTicksPerSecond = duration.GetFrameRate(duration.GetGlobalTimeMode());
            animation.mDuration = duration.GetFrameCountPrecise(duration.GetGlobalTimeMode());

            if(ParseAnimationLayer(animation, start, end, pAnimLayer, m_pFBXScene->GetRootNode()))
            {
                bFoundAnimation = true;
            }
            else
            {
                mAnimations.resize(mAnimations.size()-1);
            }
        }
    }

    return bFoundAnimation;
}

bool C3DModelFBX::ParseAnimationLayer(Animation &animation, FbxTime &start, FbxTime &end, FbxAnimLayer* pAnimLayer, FbxNode* pNode)
{
    bool bFoundAnimation = false;

    FbxAnimCurve* pPositionXAnimCurve = pNode->LclTranslation.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
    FbxAnimCurve* pPositionYAnimCurve = pNode->LclTranslation.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
    FbxAnimCurve* pPositionZAnimCurve = pNode->LclTranslation.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);
    FbxAnimCurve* pRotationXAnimCurve = pNode->LclRotation.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
    FbxAnimCurve* pRotationYAnimCurve = pNode->LclRotation.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
    FbxAnimCurve* pRotationZAnimCurve = pNode->LclRotation.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);
    FbxAnimCurve* pScalingXAnimCurve = pNode->LclScaling.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
    FbxAnimCurve* pScalingYAnimCurve = pNode->LclScaling.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
    FbxAnimCurve* pScalingZAnimCurve = pNode->LclScaling.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);

    if (pPositionXAnimCurve || pPositionYAnimCurve || pPositionZAnimCurve ||
        pRotationXAnimCurve || pRotationYAnimCurve || pRotationZAnimCurve ||
        pScalingXAnimCurve  || pScalingYAnimCurve  || pScalingZAnimCurve)
    {
        char lTimeString[256];

        animation.mChannels.resize(animation.mChannels.size()+1);
        NodeAnim &channel = animation.mChannels.back();
        channel.mNodeName = pNode->GetName();
        channel.mNodeHash = CFileExportSTUFormat::MakeHashFromName(pNode->GetName());
        channel.mPostState = AnimBehaviour_DEFAULT;
        channel.mPreState = AnimBehaviour_DEFAULT;

        FbxAMatrix defaultTransform = pNode->EvaluateLocalTransform();
        FbxVector4 defaultT = defaultTransform.GetT();
        FbxQuaternion defaultR = defaultTransform.GetQ();
        FbxVector4 defaultS = defaultTransform.GetS();

//        if (!(pPositionXAnimCurve || pPositionYAnimCurve || pPositionZAnimCurve))
        {
            channel.mPositionKeys.resize(1);
            channel.mPositionKeys[0].mTime = 0.0;
            channel.mPositionKeys[0].mValue = glm::vec3(defaultT[0], defaultT[1], defaultT[2]);
        }

//        if (!(pRotationXAnimCurve || pRotationYAnimCurve || pRotationZAnimCurve))
        {
            channel.mRotationKeys.resize(1);
            channel.mRotationKeys[0].mTime = 0.0;
            channel.mRotationKeys[0].mValue = glm::quat((float)defaultR[3], (float)defaultR[0], (float)defaultR[1], (float)defaultR[2]);
        }

//        if (!(pScalingXAnimCurve  || pScalingYAnimCurve  || pScalingZAnimCurve))
        {
            channel.mScalingKeys.resize(1);
            channel.mScalingKeys[0].mTime = 0.0;
            channel.mScalingKeys[0].mValue = glm::vec3(defaultS[0], defaultS[1], defaultS[2]);
        }

#define PARSE_T_CURVE(CURVE)\
        if (CURVE) {\
            int numKeys = CURVE->KeyGetCount();\
            for (int i = 0; i < numKeys; ++i) {\
                FbxTime keyTime = CURVE->KeyGetTime(i);\
                double time = atof(keyTime.GetTimeString(lTimeString, 256));\
                bool bFound = false;\
                for(size_t keyId = 1; keyId < channel.mPositionKeys.size(); ++keyId) {\
                    bFound = glm::epsilonEqual(time, channel.mPositionKeys[keyId].mTime, 0.00001);\
                    if (bFound) { break; }\
                }\
                if (!bFound) {\
                    FbxVector4 T = pNode->EvaluateLocalTransform(keyTime).GetT();\
                    channel.mPositionKeys.resize(channel.mPositionKeys.size()+1);\
                    channel.mPositionKeys.back().mTime = time;\
                    channel.mPositionKeys.back().mValue = glm::vec3(T[0], T[1], T[2]);\
                }\
            }\
        }

        PARSE_T_CURVE(pPositionXAnimCurve);
        PARSE_T_CURVE(pPositionYAnimCurve);
        PARSE_T_CURVE(pPositionZAnimCurve);

#define PARSE_R_CURVE(CURVE)\
        if (CURVE) {\
            int numKeys = CURVE->KeyGetCount();\
            for (int i = 0; i < numKeys; ++i) {\
                FbxTime keyTime = CURVE->KeyGetTime(i);\
                double time = atof(keyTime.GetTimeString(lTimeString, 256));\
                bool bFound = false;\
                for(size_t keyId = 1; keyId < channel.mRotationKeys.size(); ++keyId) {\
                    bFound = glm::epsilonEqual(time, channel.mRotationKeys[keyId].mTime, 0.00001);\
                    if (bFound) { break; }\
                }\
                if (!bFound) {\
                    FbxQuaternion Q = pNode->EvaluateLocalTransform(keyTime).GetQ();\
                    channel.mRotationKeys.resize(channel.mRotationKeys.size()+1);\
                    channel.mRotationKeys.back().mTime = time;\
                    channel.mRotationKeys.back().mValue = glm::normalize(glm::quat((float)Q[3], (float)Q[0], (float)Q[1], (float)Q[2]));\
                }\
            }\
        }

        PARSE_R_CURVE(pRotationXAnimCurve);
        PARSE_R_CURVE(pRotationYAnimCurve);
        PARSE_R_CURVE(pRotationZAnimCurve);

#define PARSE_S_CURVE(CURVE)\
        if (CURVE) {\
            size_t numKeys = CURVE->KeyGetCount();\
            for (int i = 0; i < numKeys; ++i) {\
                FbxTime keyTime = CURVE->KeyGetTime(i);\
                double time = atof(keyTime.GetTimeString(lTimeString, 256));\
                bool bFound = false;\
                for(size_t keyId = 1; keyId < channel.mScalingKeys.size(); ++keyId) {\
                    bFound = glm::epsilonEqual(time, channel.mScalingKeys[keyId].mTime, 0.00001);\
                    if (bFound) { break; }\
                }\
                if (!bFound) {\
                    FbxVector4 S = pNode->EvaluateLocalTransform(keyTime).GetS();\
                    channel.mScalingKeys.resize(channel.mScalingKeys.size()+1);\
                    channel.mScalingKeys.back().mTime = time;\
                    channel.mScalingKeys.back().mValue = glm::vec3(S[0], S[1], S[2]);\
                }\
            }\
        }

        PARSE_S_CURVE(pScalingXAnimCurve);
        PARSE_S_CURVE(pScalingYAnimCurve);
        PARSE_S_CURVE(pScalingZAnimCurve);

        bFoundAnimation =
                channel.mPositionKeys.size() > 1 ||
                channel.mRotationKeys.size() > 1 ||
                channel.mScalingKeys.size() > 1;

        if (!bFoundAnimation)
        {
            animation.mChannels.resize(animation.mChannels.size()-1);
        }

    }

// TODO mesh animation. Example from C3DConvertAssimp:
//    for (uint32_t j = 0; j < mAnimations[i].mNumMeshChannels; j++)
//    {
//        for (uint32_t k = 0; k < mAnimations[i].mMeshChannels[j]->mNumKeys; k++)
//        {
//            MeshKey key;
//            key.mTime = mAnimations[i].mMeshChannels[j]->mKeys[k].mTime;
//            key.mValue = mAnimations[i].mMeshChannels[j]->mKeys[k].mValue;
//            Data.insert(Data.end(), (uint8_t *)&key, (uint8_t *)&key + sizeof(key));
//            uSize += sizeof(key);
//        }
//    }

    for (int childNodeId = 0; childNodeId < pNode->GetChildCount(); ++childNodeId)
    {
        bFoundAnimation = ParseAnimationLayer(animation, start, end, pAnimLayer, pNode->GetChild(childNodeId)) || bFoundAnimation;
    }

    return bFoundAnimation;
}

void C3DModelFBX::ExportAnimations()
{
    uint32_t uSize = 0;
    std::vector< uint8_t > Data;

    uint32_t uValue = (uint32_t)mAnimations.size();
    WRITE_VALUE(uValue);

    for (uint32_t i = 0; i < mAnimations.size(); i++)
    {
        uSize += CFileExportSTUFormat::CopyString(mAnimations[i].mName.c_str(), &Data);

        double dValue = mAnimations[i].mTicksPerSecond;
        WRITE_VALUE(dValue);
        dValue = mAnimations[i].mDuration;
        WRITE_VALUE(dValue);
        uValue = (uint32_t)mAnimations[i].mChannels.size();
        WRITE_VALUE(uValue);

        for (uint32_t j = 0; j < mAnimations[i].mChannels.size(); j++)
        {
            uSize += CFileExportSTUFormat::CopyString(mAnimations[i].mChannels[j].mNodeName.c_str(), &Data);

            uValue = (uint32_t)mAnimations[i].mChannels[j].mPositionKeys.size();
            WRITE_VALUE(uValue);
            uValue = (uint32_t)mAnimations[i].mChannels[j].mRotationKeys.size();
            WRITE_VALUE(uValue);
            uValue = (uint32_t)mAnimations[i].mChannels[j].mScalingKeys.size();
            WRITE_VALUE(uValue);
            uValue = mAnimations[i].mChannels[j].mPostState;
            WRITE_VALUE(uValue);
            uValue = mAnimations[i].mChannels[j].mPreState;
            WRITE_VALUE(uValue);

            for (uint32_t k = 0; k < mAnimations[i].mChannels[j].mPositionKeys.size(); k++)
            {
                VectorKey key;
                key.mTime = mAnimations[i].mChannels[j].mPositionKeys[k].mTime;
                key.mValue = glm::vec3(mAnimations[i].mChannels[j].mPositionKeys[k].mValue.x, mAnimations[i].mChannels[j].mPositionKeys[k].mValue.y, mAnimations[i].mChannels[j].mPositionKeys[k].mValue.z);
                Data.insert(Data.end(), (uint8_t *)&key, (uint8_t *)&key + sizeof(key));
                uSize += sizeof(key);
            }
            for (uint32_t k = 0; k < mAnimations[i].mChannels[j].mRotationKeys.size(); k++)
            {
                QuatKey key;
                key.mTime = mAnimations[i].mChannels[j].mRotationKeys[k].mTime;
                key.mValue = glm::quat(mAnimations[i].mChannels[j].mRotationKeys[k].mValue.w, mAnimations[i].mChannels[j].mRotationKeys[k].mValue.x, mAnimations[i].mChannels[j].mRotationKeys[k].mValue.y, mAnimations[i].mChannels[j].mRotationKeys[k].mValue.z);
                Data.insert(Data.end(), (uint8_t *)&key, (uint8_t *)&key + sizeof(key));
                uSize += sizeof(key);
            }
            for (uint32_t k = 0; k < mAnimations[i].mChannels[j].mScalingKeys.size(); k++)
            {
                VectorKey key;
                key.mTime = mAnimations[i].mChannels[j].mScalingKeys[k].mTime;
                key.mValue = glm::vec3(mAnimations[i].mChannels[j].mScalingKeys[k].mValue.x, mAnimations[i].mChannels[j].mScalingKeys[k].mValue.y, mAnimations[i].mChannels[j].mScalingKeys[k].mValue.z);
                Data.insert(Data.end(), (uint8_t *)&key, (uint8_t *)&key + sizeof(key));
                uSize += sizeof(key);
            }
        }

        uValue = (uint32_t)mAnimations[i].mMeshChannels.size();
        WRITE_VALUE(uValue);

        for (uint32_t j = 0; j < mAnimations[i].mMeshChannels.size(); j++)
        {
            uSize += CFileExportSTUFormat::CopyString(mAnimations[i].mMeshChannels[j].mName.c_str(), &Data);

            uValue = (uint32_t)mAnimations[i].mMeshChannels[j].mKeys.size();
            WRITE_VALUE(uValue);

            for (uint32_t k = 0; k < mAnimations[i].mMeshChannels[j].mKeys.size(); k++)
            {
                MeshKey key;
                key.mTime = mAnimations[i].mMeshChannels[j].mKeys[k].mTime;
                key.mValue = mAnimations[i].mMeshChannels[j].mKeys[k].mValue;
                Data.insert(Data.end(), (uint8_t *)&key, (uint8_t *)&key + sizeof(key));
                uSize += sizeof(key);
            }
        }
    }

#if STU_EXPORT_SEQUENTIAL
    m_Export.AppendChunkToFile(m_sSTUPath, "Animations", &Data.at(0), uSize);
#else
    m_Export.WriteChunk("Animations", &Data.at(0), uSize);
#endif

}

void C3DModelFBX::ExportTextures()
{
    // FBX SDK Always extracts the embedded textures, so we will always have no
    // embedded textures (unless we decide to manually embed them later)
}

void C3DModelFBX::ExportSceneTree()
{
    m_uSubModelCount = 0;
    m_uSubModelVertexCount = 0;
    ExportSubTree(m_pFBXScene->GetRootNode());
}

void C3DModelFBX::ExportSubTree(FbxNode* pNode)
{
    uint32_t uSize = 0;
    uint32_t uValue;
    std::vector< uint8_t > Data;

    std::string nodeName;
    static uint32_t sUniqueFbxUnknownID = 0;
    if (strlen(pNode->GetName()) > 0)
    {
        nodeName = std::string("Fbx.(") + pNode->GetName() + "-" + std::to_string(sUniqueFbxUnknownID++) + ")";
    }
    else
    {
        nodeName = std::string("Fbx.(UNKNOWN-") + std::to_string(sUniqueFbxUnknownID++) + ")";
    }
    LOG_INFO("Found node '%s'\n", nodeName.c_str());

    uint32_t childCount = (uint32_t)pNode->GetChildCount();
    WRITE_VALUE(childCount);

    uSize += CFileExportSTUFormat::CopyString(nodeName.c_str(), &Data);
    uSize += CFileExportSTUFormat::CopyString(pNode->GetName(), &Data);

    // Get node transformation matrix
    glm::mat4 matrix = ConvertFbxToGLM(pNode->EvaluateLocalTransform());
    WRITE_VALUE(matrix);

    std::vector<FbxMesh*> meshes;
    for(int i = 0; i < pNode->GetNodeAttributeCount(); i++)
    {
        FbxNodeAttribute *attr = pNode->GetNodeAttributeByIndex(i);
        FbxNodeAttribute::EType attr_type = attr->GetAttributeType();

        switch(attr_type)
        {
        case FbxNodeAttribute::eMesh:
            meshes.push_back((FbxMesh*)attr);
            break;
        default:
            break;
        }
    }

    uint32_t meshCount = 0;

    for (size_t meshIndex = 0; meshIndex < meshes.size(); ++meshIndex)
    {
        FbxMesh* pMesh = meshes[meshIndex];
        if (pMesh->GetPolygonCount() == 0)
        {
            continue;
        }
        meshCount++;
    }

    WRITE_VALUE(meshCount);

    for (size_t meshIndex = 0; meshIndex < meshCount; ++meshIndex)
    {
        FbxMesh* pMesh = meshes[meshIndex];
        if (pMesh->GetPolygonCount() == 0)
        {
            continue;
        }
        FbxSurfaceMaterial *pMaterial = NULL;

        int numMaterial = pNode->GetSrcObjectCount<FbxSurfaceMaterial>();
        for (int materialIndex=0; materialIndex<numMaterial; materialIndex++ )
        {
            pMaterial = (FbxSurfaceMaterial*)pNode->GetSrcObject<FbxSurfaceMaterial>(materialIndex);
            if (pMaterial)
            {
                break;
            }
        }


        std::string meshName;
        static uint32_t sUniqueFbxUnknownMeshID = 0;
        if (strlen(pMesh->GetName()) > 0)
        {
            meshName = nodeName + ".mesh(" + pMesh->GetName() + "-" + std::to_string(sUniqueFbxUnknownMeshID++) + ")";
        }
        else
        {
            meshName = nodeName + ".mesh(UNKNOWN-" + std::to_string(sUniqueFbxUnknownMeshID++) + ")";
        }
        LOG_INFO("Found mesh '%s'\n", meshName.c_str());
        std::string VBOName = meshName + ".VBO";
        std::string IBOName = meshName + ".IBO";

        if (pMaterial)
        {
            LOG_INFO("Found material '%s'\n", pMaterial->GetName());
        }

        uSize += CFileExportSTUFormat::CopyString(meshName.c_str(), &Data);
        uSize += CFileExportSTUFormat::CopyString(pMesh->GetName(), &Data);

        LoadBones(pMesh);

        if (m_bHasAnimations)
        {
            uValue = 1;
        }
        else
        {
            uValue = 0;
        }
        WRITE_VALUE(uValue);

        uValue = pMesh->GetPolygonVertexCount();
        WRITE_VALUE(uValue);

        if (pMesh->GetElementNormalCount() > 0)
        {
            if (m_fbxSkeletons.size() > 0)
            {
                m_VertexDataType = VertexDataType_Bones;
            }
            else if (pMesh->GetElementUVCount() > 0)
            {
                m_VertexDataType = VertexDataType_Normals;
            }
            else if (pMesh->GetElementVertexColorCount() > 0)
            {
                m_VertexDataType = VertexDataType_Points;
            }
            else
            {
                m_VertexDataType = VertexDataType_Simple;
            }
        }
        else
        {
            if (pMesh->GetElementUVCount() > 0)
            {
                m_VertexDataType = VertexDataType_Textured;
            }
            else if (pMesh->GetElementVertexColorCount() > 0)
            {
                m_VertexDataType = VertexDataType_Points;
            }
            else
            {
                m_VertexDataType = VertexDataType_Simple;
            }
        }


        FbxFileTexture *pDiffuseTexture = GetMaterialFileTexture(pMaterial, FbxSurfaceMaterial::sDiffuse);

        WRITE_VALUE(m_VertexDataType);

        std::string sChunkname = std::string("Vx:") + std::to_string(m_uSubModelVertexCount++);
        ExportVertices(m_Export, m_sSTUPath, sChunkname, pMesh, pDiffuseTexture);

        // TODO Figure out how to get indices from FBX (do they need vertices to
        // begin with? unless I'm wrong, it looks like they're all coming in as
        // triangles (group of 3 vertices). Found 'indices' in
        // DisplayMaterialMapping and DisplayLink in their import samples. will
        // re-visit later.
        std::vector<uint16_t> indices;
        uValue = (uint32_t)indices.size();
        WRITE_VALUE(uValue);
        if (uValue)
        {
            WRITE_VALUES(indices[0], (uint32_t)indices.size());
        }

        // TODO figure out if FBX has points and lines, and how to extract them.
        uValue = PrimitiveType_TRIANGLE;
        WRITE_VALUE(uValue);

        // TODO figure out if FBX has one-sided vs two-sided polygon, and how to
        // extract that information. For now, let's assume two-sided.
        uValue = 0;
        WRITE_VALUE(uValue);


        glm::vec3 AmbientColor(0.5f);
        glm::vec3 DiffuseColor(0.75f);
        glm::vec3 SpecularColor(1.0f);
        glm::vec3 EmissiveColor(0.0f);
        float fShininess = 16.0f;
        float fAlpha = 1.0f;
        float fReflectivity = 0.0f;

        if(pMaterial && pMaterial->GetClassId().Is(FbxSurfaceLambert::ClassId))
        {
            FbxDouble3 color;
            FbxDouble factor;

            FbxSurfaceLambert *pLambertMaterial = static_cast<FbxSurfaceLambert*>(pMaterial);

            color = pLambertMaterial->Ambient.Get();
            factor = pLambertMaterial->AmbientFactor.Get();

            AmbientColor.r = (float)(color[0] * factor);
            AmbientColor.g = (float)(color[1] * factor);
            AmbientColor.b = (float)(color[2] * factor);

            color = pLambertMaterial->Diffuse.Get();
            factor = pLambertMaterial->DiffuseFactor.Get();

            DiffuseColor.r = (float)(color[0] * factor);
            DiffuseColor.g = (float)(color[1] * factor);
            DiffuseColor.b = (float)(color[2] * factor);

            color = pLambertMaterial->Emissive.Get();
            factor = pLambertMaterial->EmissiveFactor.Get();

            EmissiveColor.r = (float)(color[0] * factor);
            EmissiveColor.g = (float)(color[1] * factor);
            EmissiveColor.b = (float)(color[2] * factor);

            // Undocumented 'Opacity' Property. Found about it here:
            // https://forums.autodesk.com/t5/fbx-forum/opacity-vs-transparency-factor/td-p/4108582
            const FbxProperty opacityProperty = pLambertMaterial->FindProperty("Opacity");
            if (opacityProperty.IsValid())
            {
                fAlpha = static_cast<float>(opacityProperty.Get<FbxDouble>());
            }
            else
            {
                fAlpha = 1.0f - (float)pLambertMaterial->TransparencyFactor.Get();
            }

            if (pLambertMaterial->GetClassId().Is(FbxSurfacePhong::ClassId))
            {
                FbxSurfacePhong *pPhongMaterial = static_cast<FbxSurfacePhong*>(pLambertMaterial);

                color = pPhongMaterial->Specular.Get();
                factor = pPhongMaterial->SpecularFactor.Get();

                SpecularColor.r = (float)(color[0] * factor);
                SpecularColor.g = (float)(color[1] * factor);
                SpecularColor.b = (float)(color[2] * factor);

                fShininess = (float)pPhongMaterial->Shininess.Get();
                fReflectivity = (float)pPhongMaterial->ReflectionFactor.Get();
            }
        }
        else
        {
            LOG_ERROR("Unknown Material Type!");
        }
        WRITE_VALUE(AmbientColor);
        WRITE_VALUE(DiffuseColor);
        WRITE_VALUE(SpecularColor);
        WRITE_VALUE(fShininess);
        WRITE_VALUE(fAlpha);

        std::string texPath;
        if (GetMaterialTexturePath(texPath, m_path.c_str(), pMaterial, FbxSurfaceMaterial::sDiffuse))
        {
            uValue = TextureType_DIFFUSE;
            WRITE_VALUE(uValue);
            CImagePreProcess::ReplaceUnrecognizedInternalTextureFormats(texPath);
            uSize += CFileExportSTUFormat::CopyString(texPath.c_str(), &Data);
        }
        else if (GetMaterialColor(DiffuseColor, pMaterial, FbxSurfaceMaterial::sDiffuse, FbxSurfaceMaterial::sDiffuseFactor))
        {
            uValue = TextureType_COLOR_DIFFUSE;
            WRITE_VALUE(uValue);
            WRITE_VALUE(fAlpha);
        }
        else
        {
            uValue = TextureType_UNKNOWN;
            WRITE_VALUE(uValue);
        }

        if (GetMaterialTexturePath(texPath, m_path.c_str(), pMaterial, FbxSurfaceMaterial::sBump))
        {
            uValue = TextureType_HEIGHT;
            WRITE_VALUE(uValue);
            CImagePreProcess::ReplaceUnrecognizedInternalTextureFormats(texPath);
            uSize += CFileExportSTUFormat::CopyString(texPath.c_str(), &Data);
        }
        else
        {
            uValue = TextureType_UNKNOWN;
            WRITE_VALUE(uValue);
        }
    }

    std::string sChunkname = std::string("Model:") + std::to_string(m_uSubModelCount);
    m_uSubModelCount++;

#if STU_EXPORT_SEQUENTIAL
    m_Export.AppendChunkToFile(m_sSTUPath, sChunkname, &Data.at(0), (uint32_t)Data.size());
#else
    m_Export.WriteChunk(sChunkname, &Data.at(0), (uint32_t)Data.size());
#endif

    for (int i = 0; i < pNode->GetChildCount(); ++i)
    {
        ExportSubTree(pNode->GetChild(i));
    }
}

void C3DModelFBX::ExportBones()
{
    uint32_t uValue;
    uint32_t uSize = 0;
    std::vector< uint8_t > Data;

    uValue = (uint32_t)m_BoneMapping.size();
    WRITE_VALUE(uValue);

    std::map<uint32_t, uint32_t> ::iterator Itr = m_BoneMapping.begin();
    std::map<uint32_t, uint32_t> ::iterator End = m_BoneMapping.end();
    while (Itr != End)
    {
        WRITE_VALUE(Itr->first);
        WRITE_VALUE(Itr->second);

        Itr++;
    }
    uValue = (uint32_t)m_BoneInfo.size();
    WRITE_VALUE(uValue);

    if (uValue)
    {
        WRITE_VALUES(m_BoneInfo[0], (uint32_t)m_BoneInfo.size());
    }

#if STU_EXPORT_SEQUENTIAL
    m_Export.AppendChunkToFile(m_sSTUPath, "Bones", &Data.at(0), uSize);
#else
    m_Export.WriteChunk("Bones", &Data.at(0), uSize);
#endif
}

template <class VertexData>
void ExportVerticesTexcoord(int polygonId, int polygonVertexId, int controlPointId, int vertexId, VertexData &Vertex, FbxMesh *pMesh, FbxTexture *pDiffuseTexture, int elementUVIndex, bool bFlipUVonY)
{
    FbxVector2 value;
    FbxGeometryElementUV* pElement = pMesh->GetElementUV(elementUVIndex);

    if (!pElement)
    {
        //LOG_ERROR("Missing vertex UVs from the '%s' mesh", pMesh->GetName());
        return;
    }

    switch (pElement->GetMappingMode())
    {
        case FbxGeometryElement::eByControlPoint:
        {
            switch (pElement->GetReferenceMode())
            {
                case FbxGeometryElement::eDirect:
                {
                    value = pElement->GetDirectArray().GetAt(controlPointId);
                    break;
                }
                case FbxGeometryElement::eIndexToDirect:
                {
                    int id = pElement->GetIndexArray().GetAt(controlPointId);
                    value = pElement->GetDirectArray().GetAt(id);
                    break;
                }
                default: break;
            }
            break;
        }
        case FbxGeometryElement::eByPolygonVertex:
        {
            switch (pElement->GetReferenceMode())
            {
                case FbxGeometryElement::eDirect:
                {
                    int id = pMesh->GetTextureUVIndex(polygonId, polygonVertexId);
                    value = pElement->GetDirectArray().GetAt(id);
                    break;
                }
                case FbxGeometryElement::eIndexToDirect:
                {
                    int id = pElement->GetIndexArray().GetAt(vertexId);
                    value = pElement->GetDirectArray().GetAt(id);
                    break;
                }
                default: break;
            }
            break;
        }
        default: break;
    }

    if (pDiffuseTexture)
    {
        glm::mat4 UVTransform;
        UVTransform = glm::translate(UVTransform, glm::vec3(pDiffuseTexture->GetTranslationU(), pDiffuseTexture->GetTranslationV(), 0.0f));
        UVTransform = glm::rotate(UVTransform, (float)pDiffuseTexture->GetRotationU(), glm::vec3(1.0f, 0.0f, 0.0f));
        UVTransform = glm::rotate(UVTransform, (float)pDiffuseTexture->GetRotationV(), glm::vec3(0.0f, 1.0f, 0.0f));
        UVTransform = glm::rotate(UVTransform, (float)pDiffuseTexture->GetRotationW(), glm::vec3(0.0f, 0.0f, 1.0f));
        UVTransform = glm::scale(UVTransform, glm::vec3(pDiffuseTexture->GetScaleU(), pDiffuseTexture->GetScaleV(), 1.0f));

        glm::vec4 transformedUVs = glm::vec4(value[0], bFlipUVonY ? 1.0f - value[1] : value[1], 0.0f, 1.0f) * UVTransform;

        Vertex.texcoord.x = transformedUVs.x;
        Vertex.texcoord.y = transformedUVs.y;

        if (pDiffuseTexture->GetSwapUV())
        {
            float t = Vertex.texcoord.x;
            Vertex.texcoord.x = Vertex.texcoord.y;
            Vertex.texcoord.y = t;
        }
    }
    else
    {
        Vertex.texcoord.x = (float)value[0];
        Vertex.texcoord.y = (float)(bFlipUVonY ? 1.0f - value[1] : value[1]);
    }
}

template <class VertexData>
void ExportVerticesNormal(int controlPointId, int vertexId, VertexData &Vertex, FbxMesh *pMesh)
{
    FbxVector4 value;
    FbxGeometryElementNormal* pElement = pMesh->GetElementNormal();

    if (!pElement)
    {
        LOG_ERROR("Missing vertex normals from the '%s' mesh", pMesh->GetName());
        return;
    }

    switch (pElement->GetMappingMode())
    {
        case FbxGeometryElement::eByControlPoint:
        {
            switch (pElement->GetReferenceMode())
            {
                case FbxGeometryElement::eDirect:
                {
                    value = pElement->GetDirectArray().GetAt(controlPointId);
                    break;
                }
                case FbxGeometryElement::eIndexToDirect:
                {
                    int id = pElement->GetIndexArray().GetAt(controlPointId);
                    value = pElement->GetDirectArray().GetAt(id);
                    break;
                }
                default: break;
            }
            break;
        }
        case FbxGeometryElement::eByPolygonVertex:
        {
            switch (pElement->GetReferenceMode())
            {
                case FbxGeometryElement::eDirect:
                {
                    value = pElement->GetDirectArray().GetAt(vertexId);
                    break;
                }
                case FbxGeometryElement::eIndexToDirect:
                {
                    int id = pElement->GetIndexArray().GetAt(vertexId);
                    value = pElement->GetDirectArray().GetAt(id);
                    break;
                }
                default: break;
            }
            break;
        }
        default: break;
    }

    Vertex.normal.x = (float)value[0];
    Vertex.normal.y = (float)value[1];
    Vertex.normal.z = (float)value[2];
    Vertex.normal.w = (float)value[3];
}

template <class VertexData>
void ExportVerticesColor(int controlPointId, int vertexId, VertexData &Vertex, FbxMesh *pMesh)
{
    FbxColor color;
    const FbxGeometryElementVertexColor* pElement = pMesh->GetElementVertexColor();

    if (!pElement)
    {
        LOG_ERROR("Missing vertex colors from the '%s' mesh", pMesh->GetName());
        return;
    }

    switch (pElement->GetMappingMode())
    {
        case FbxGeometryElement::eByControlPoint:
        {
            switch (pElement->GetReferenceMode())
            {
                case FbxGeometryElement::eDirect:
                {
                    color = pElement->GetDirectArray().GetAt(controlPointId);
                    break;
                }
                case FbxGeometryElement::eIndexToDirect:
                {
                    int id = pElement->GetIndexArray().GetAt(controlPointId);
                    color = pElement->GetDirectArray().GetAt(id);
                    break;
                }
                default: break;
            }
            break;
        }
        case FbxGeometryElement::eByPolygonVertex:
        {
            switch (pElement->GetReferenceMode())
            {
                case FbxGeometryElement::eDirect:
                {
                    color = pElement->GetDirectArray().GetAt(vertexId);
                    break;
                }
                case FbxGeometryElement::eIndexToDirect:
                {
                    int id = pElement->GetIndexArray().GetAt(vertexId);
                    color = pElement->GetDirectArray().GetAt(id);
                    break;
                }
                default: break;
            }
            break;
        }
        default: break;
    }

    Vertex.color.r = (float)(color.mRed   * color.mAlpha);
    Vertex.color.g = (float)(color.mGreen * color.mAlpha);
    Vertex.color.b = (float)(color.mBlue  * color.mAlpha);
}

template <class VertexData>
void ExportVerticesBoneIdAndWeight(int controlPointId, VertexData &Vertex, std::vector<VertexBoneData> &Bones)
{
    float packedboneids1 = 0.0f;
    float packedboneids2 = 0.0f;
    float boneWeight0 = 1.0f;
    float boneWeight1 = 0.0f;
    float boneWeight2 = 0.0f;
    float boneWeight3 = 0.0f;
    if (controlPointId < (int)Bones.size())
    {
        boneWeight0 = Bones[controlPointId].SortedData[0].Weight;
        boneWeight1 = Bones[controlPointId].SortedData[1].Weight;
        boneWeight2 = Bones[controlPointId].SortedData[2].Weight;
        boneWeight3 = Bones[controlPointId].SortedData[3].Weight;

        packedboneids1 = float(Bones[controlPointId].SortedData[0].ID) +
            float(Bones[controlPointId].SortedData[1].ID) / 256.0f;    //256 allows for up to 200+ bones matrices before clashing.
        packedboneids2 = float(Bones[controlPointId].SortedData[2].ID) +
            float(Bones[controlPointId].SortedData[3].ID) / 256.0f;
    }
    else
    {
        //LOG_ERROR("Missing Bone vertex information for vertex #%d", controlPointId);
    }
    Vertex.bones.x = boneWeight0;
    Vertex.bones.y = boneWeight1;
    Vertex.bones.z = boneWeight2;
    Vertex.bones.w = boneWeight3;
    Vertex.texcoord.w = packedboneids1;
    Vertex.normal.w = packedboneids2;
}


// Disabled for these since they don't have UVs...
template <> void ExportVerticesTexcoord<VertexDataSimple>(int, int, int, int, VertexDataSimple &, FbxMesh *, FbxTexture *, int, bool) {}
template <> void ExportVerticesTexcoord<VertexDataPoints>(int, int, int, int, VertexDataPoints &, FbxMesh *, FbxTexture *, int, bool) {}

// Disabled for these since they don't have normals...
template <> void ExportVerticesNormal<VertexDataSimple>(int, int, VertexDataSimple&, FbxMesh *) {}
template <> void ExportVerticesNormal<VertexDataPoints>(int, int, VertexDataPoints&, FbxMesh *) {}
template <> void ExportVerticesNormal<VertexDataTextured>(int, int, VertexDataTextured&, FbxMesh *) {}

// Disabled for these since they don't have colors...
template <> void ExportVerticesColor<VertexDataSimple>(int, int, VertexDataSimple&, FbxMesh *) {}
template <> void ExportVerticesColor<VertexDataTextured>(int, int, VertexDataTextured&, FbxMesh *) {}
template <> void ExportVerticesColor<VertexDataWithNormals>(int, int, VertexDataWithNormals&, FbxMesh *) {}
template <> void ExportVerticesColor<VertexDataWithBones>(int, int, VertexDataWithBones&, FbxMesh *) {}

// Disabled for these since they don't have bone ID/Weights...
template <> void ExportVerticesBoneIdAndWeight<VertexDataSimple>(int, VertexDataSimple &, std::vector<VertexBoneData> &) {}
template <> void ExportVerticesBoneIdAndWeight<VertexDataPoints>(int, VertexDataPoints &, std::vector<VertexBoneData> &) {}
template <> void ExportVerticesBoneIdAndWeight<VertexDataTextured>(int, VertexDataTextured &, std::vector<VertexBoneData> &) {}
template <> void ExportVerticesBoneIdAndWeight<VertexDataWithNormals>(int, VertexDataWithNormals &, std::vector<VertexBoneData> &) {}


template <typename VertexData>
void ExportVerticesOfType(CFileExportSTUFormat &Export, const std::string &path, std::string sChunkname, FbxMesh *pMesh, FbxTexture *pDiffuseTexture, bool bFlipUVonY, std::vector<VertexBoneData> &Bones)
{
    // Since we can potentially have more than one UV (because of
    // multi-texturing), we have to pick the 'diffuse' one, and here is
    // how *i think* it can be obtained (I hope it's the right way)
    int elementUVIndex = 0;

    if (pDiffuseTexture)
    {
        FbxString diffuseUVSet = pDiffuseTexture->UVSet.Get();
        if (diffuseUVSet != "default") // if we get 'default', let's stick to elementUVIndex = 0 (a.k.a the first one)
        {
            FbxStringList uvSetList;
            pMesh->GetUVSetNames(uvSetList);

            for(int uvSetIndex = 0; uvSetIndex < uvSetList.GetCount(); ++uvSetIndex)
            {
                if (uvSetList[uvSetIndex] == diffuseUVSet)
                {
                    elementUVIndex = uvSetIndex;
                    break;
                }
            }
        }
    }

    FbxVector4* controlPoints = pMesh->GetControlPoints();

    uint32_t uSize = 0;
    uint8_t bytes[128] = { 0 };
    std::vector< uint8_t > Data;
    float bmin[3], bmax[3];

    bmin[0] = bmin[1] = bmin[2] = std::numeric_limits<float>::max();
    bmax[0] = bmax[1] = bmax[2] = -std::numeric_limits<float>::max();

    VertexData Vertex;
    int vertexId = 0;
    const int polygonCount = pMesh->GetPolygonCount();
    for (int polygonId = 0; polygonId < polygonCount ; polygonId++)
    {
        int polygonVertexCount = pMesh->GetPolygonSize(polygonId); // should always gives us '3', but let's not take a chance.
        for (int polygonVertexId = 0; polygonVertexId < polygonVertexCount; polygonVertexId++)
        {
            int controlPointId = pMesh->GetPolygonVertex(polygonId, polygonVertexId);

            Vertex.position.x = (float)controlPoints[controlPointId][0];
            Vertex.position.y = (float)controlPoints[controlPointId][1];
            Vertex.position.z = (float)controlPoints[controlPointId][2];
            bmin[0] = std::min(Vertex.position.x, bmin[0]);
            bmin[1] = std::min(Vertex.position.y, bmin[1]);
            bmin[2] = std::min(Vertex.position.z, bmin[2]);
            bmax[0] = std::max(Vertex.position.x, bmax[0]);
            bmax[1] = std::max(Vertex.position.y, bmax[1]);
            bmax[2] = std::max(Vertex.position.z, bmax[2]);

            ExportVerticesTexcoord(polygonId, polygonVertexId, controlPointId, vertexId, Vertex, pMesh, pDiffuseTexture, elementUVIndex, bFlipUVonY);
            ExportVerticesNormal(controlPointId, vertexId, Vertex, pMesh);
            ExportVerticesColor(controlPointId, vertexId, Vertex, pMesh);
            ExportVerticesBoneIdAndWeight(controlPointId, Vertex, Bones);

            WRITE_VALUE(Vertex);

            ++vertexId;
        }
    }
    if (Data.size() == 0)
    {
        return;
    }
#if STU_EXPORT_SEQUENTIAL
    Export.AppendChunkToFile(path, sChunkname, &Data.at(0), (int32_t)Data.size());
#else
    Export.WriteChunk(sChunkname, &Data.at(0), (int32_t)Data.size());
#endif
    std::vector< uint8_t >().swap(Data);

    //Write BBox chunk:
    WRITE_VALUE(bmin[0]);
    WRITE_VALUE(bmin[1]);
    WRITE_VALUE(bmin[2]);
    WRITE_VALUE(bmax[0]);
    WRITE_VALUE(bmax[1]);
    WRITE_VALUE(bmax[2]);

#if STU_EXPORT_SEQUENTIAL
    Export.AppendChunkToFile(path, sChunkname + "BB", &Data.at(0), (uint32_t)Data.size());
#else
    Export.WriteChunk(sChunkname + "BB", &Data.at(0), Data.size());
#endif
    std::vector< uint8_t >().swap(Data);
}

void C3DModelFBX::ExportVertices(CFileExportSTUFormat &Export, const std::string &path, std::string sChunkname, FbxMesh *pMesh, FbxTexture *pDiffuseTexture)
{
    switch (m_VertexDataType)
    {
        case VertexDataType_Simple:
        {
            ExportVerticesOfType<VertexDataSimple>(Export, path, sChunkname, pMesh, pDiffuseTexture, m_bFlipUVonY, m_Bones);
            break;
        }
        case VertexDataType_Points:
        {
            ExportVerticesOfType<VertexDataPoints>(Export, path, sChunkname, pMesh, pDiffuseTexture, m_bFlipUVonY, m_Bones);
            break;
        }
        case VertexDataType_Textured:
        {
            ExportVerticesOfType<VertexDataTextured>(Export, path, sChunkname, pMesh, pDiffuseTexture, m_bFlipUVonY, m_Bones);
            break;
        }
        case VertexDataType_Normals:
        {
            ExportVerticesOfType<VertexDataWithNormals>(Export, path, sChunkname, pMesh, pDiffuseTexture, m_bFlipUVonY, m_Bones);
            break;
        }
        case VertexDataType_Bones:
        {
            ExportVerticesOfType<VertexDataWithBones>(Export, path, sChunkname, pMesh, pDiffuseTexture, m_bFlipUVonY, m_Bones);
            break;
        }
    }
}

void C3DModelFBX::LoadBones(FbxMesh *pMesh)
{
    m_Bones.clear();

    int skinCount = pMesh->GetDeformerCount(FbxDeformer::eSkin);

    if (skinCount > 0)
    {
        m_Bones.resize(pMesh->GetPolygonVertexCount());
    }

    FbxAMatrix geometryTransform = GetGeometryTransformation(pMesh->GetNode());

    for (int skinIndex = 0; skinIndex < skinCount; ++skinIndex)
    {
        FbxSkin *pSkin = static_cast<FbxSkin *>(pMesh->GetDeformer(skinIndex, FbxDeformer::eSkin));
        int clusterCount = pSkin->GetClusterCount();
        for (int clusterIndex = 0; clusterIndex < clusterCount; ++clusterIndex)
        {
            FbxCluster *pCluster = pSkin->GetCluster(clusterIndex);
            FbxNode *pBoneNode = pCluster->GetLink();

            uint32_t BoneIndex = 0;
            std::string BoneName = pBoneNode->GetName();
            uint32_t BoneHash = CFileExportSTUFormat::MakeHashFromName(BoneName);

            if (m_BoneMapping.find(BoneHash) == m_BoneMapping.end())
            {
                BoneIndex = m_uNumBones;
                m_uNumBones++;
                if (m_uNumBones > MAX_BONES)
                {
                    LOG_ERROR("3D object Exceeded maximum bone count");
                    //m_uNumBones = MAX_BONES; //Can be capped here, or passes throuugh
                }

                // I have no idea what's going on here!!!
                // got this from here: https://forums.autodesk.com/t5/fbx-forum/useful-things-you-might-want-to-know-about-fbxsdk/td-p/4821177
                // and it works! It's now giving me the expected bone offset matrices.
                FbxAMatrix transformMatrix;
                FbxAMatrix transformLinkMatrix;
                pCluster->GetTransformMatrix(transformMatrix);
                pCluster->GetTransformLinkMatrix(transformLinkMatrix);
                FbxAMatrix globalBindposeInverseMatrix = transformLinkMatrix.Inverse() * transformMatrix * geometryTransform;

                m_BoneInfo.resize(m_BoneInfo.size() + 1);
                BoneInfo &bi = m_BoneInfo.back();
                m_BoneMapping[BoneHash] = BoneIndex;
                bi.BoneOffset = ConvertFbxToGLM(globalBindposeInverseMatrix);
            }
            else
            {
                BoneIndex = m_BoneMapping[BoneHash];
            }

            int indexCount = pCluster->GetControlPointIndicesCount();
            int* Indices = pCluster->GetControlPointIndices();
            double* Weights = pCluster->GetControlPointWeights();

            for (int indexID = 0; indexID < indexCount; ++indexID)
            {
                int VertexID = Indices[indexID];
                float Weight = (float)Weights[indexID];
                if (!glm::epsilonEqual(Weight, 0.0f, glm::epsilon<float>()))
                {
                    //LOG_ERROR("Vertex #%d influenced by bone #%d (%s) (weight=%f)", VertexID, BoneIndex, BoneName.c_str(), Weight);
                    m_Bones[VertexID].AddBoneData(BoneIndex, Weight);
                }
            }
        }
    }
}
