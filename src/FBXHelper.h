#ifndef FBX_HELPER_H
#define FBX_HELPER_H

#include <fbxsdk.h>

void InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene);
void DestroySdkObjects(FbxManager* pManager, bool pExitStatus);
bool SaveScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename, int pFileFormat, bool pEmbedMedia);
bool LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename);

//SR: These are not REALLY needed, but may useful for info or debugging

void DisplayAnimation(FbxAnimStack* pAnimStack, FbxNode* pNode, bool isSwitcher = false);
void DisplayAnimation(FbxAnimLayer* pAnimLayer, FbxNode* pNode, bool isSwitcher = false);
void DisplayChannels(FbxNode* pNode, FbxAnimLayer* pAnimLayer, void(*DisplayCurve) (FbxAnimCurve* pCurve), void(*DisplayListCurve) (FbxAnimCurve* pCurve, FbxProperty* pProperty), bool isSwitcher);

void DisplayProperties(FbxObject* pObject);
void DisplayGenericInfo(FbxNode* pNode, int pDepth);

void DisplayCamera(FbxCamera* pCamera, char* pName, FbxNode* pTargetNode = NULL, FbxNode* pTargetUpNode = NULL);
void DisplayDefaultAnimationValues(FbxCamera* pCamera);
void DisplayRenderOptions(FbxCamera* pCamera);
void DisplayCameraViewOptions(FbxCamera* pCamera);
void DisplayBackgroundProperties(FbxCamera* pCamera);
void DisplayApertureAndFilmControls(FbxCamera* pCamera);
void DisplayViewingAreaControls(FbxCamera* pCamera);
void DisplayCameraPositionAndOrientation(FbxCamera* pCamera, FbxNode* pTargetNode, FbxNode* pUpTargetNode);

void DisplayDefaultAnimationValues(FbxLight* pLight);
void DisplayControlsPoints(FbxMesh* pMesh);
void DisplayPolygons(FbxMesh* pMesh);
void DisplayMaterialMapping(FbxMesh* pMesh);
void DisplayTextureNames(FbxProperty &pProperty, FbxString& pConnectionString);
void DisplayMaterialConnections(FbxMesh* pMesh);
void DisplayMaterialTextureConnections(FbxSurfaceMaterial* pMaterial, char * header, int pMatId, int l);

void DisplayGlobalLightSettings(FbxGlobalSettings* pGlobalSettings);
void DisplayGlobalCameraSettings(FbxGlobalSettings* pGlobalSettings);
void DisplayGlobalTimeSettings(FbxGlobalSettings* pGlobalSettings);
void DisplayHierarchy(FbxScene* pScene);
void DisplayHierarchy(FbxNode* pNode, int pDepth);
void DisplayTarget(FbxNode* pNode);
void DisplayMetaData(FbxScene* pScene);
void DisplayMetaDataConnections(FbxObject* pNode);
void DisplayString(const char* pHeader, const char* pValue  = "", const char* pSuffix  = "");
void DisplayBool(const char* pHeader, bool pValue, const char* pSuffix  = "");
void DisplayInt(const char* pHeader, int pValue, const char* pSuffix  = "");
void DisplayDouble(const char* pHeader, double pValue, const char* pSuffix  = "");
void Display2DVector(const char* pHeader, FbxVector2 pValue, const char* pSuffix  = "");
void Display3DVector(const char* pHeader, FbxVector4 pValue, const char* pSuffix  = "");
void DisplayColor(const char* pHeader, FbxColor pValue, const char* pSuffix  = "");
void Display4DVector(const char* pHeader, FbxVector4 pValue, const char* pSuffix  = "");

void DisplayMaterial(FbxGeometry* pGeometry);
void DisplayTextureInfo(FbxTexture* pTexture, int pBlendMode);
void FindAndDisplayTextureInfoByProperty(FbxProperty pProperty, bool& pDisplayHeader, int pMaterialIndex);
void DisplayTexture(FbxGeometry* pGeometry);
void DisplayCache(FbxGeometry* pGeometry);
void DisplayLink(FbxGeometry* pGeometry);
void DisplayPivotsAndLimits(FbxNode* pNode);
void DisplayShape(FbxGeometry* pGeometry);
void DisplaySkeleton(FbxNode* pNode);
void DisplayUserProperties(FbxObject* pObject);
void DisplayMesh(FbxNode* pNode);
void DisplayLight(FbxNode* pNode);
void DisplayCamera(FbxNode* pNode);
void DisplayAnimation(FbxScene* pScene);
void DisplayGenericInfo(FbxScene* pScene);

void DisplayContent(FbxScene* pScene);
void DisplayPose(FbxScene* pScene);
void DisplayContent(FbxNode* pNode);
void DisplayPatch(FbxNode* pNode);
void DisplayLodGroup(FbxNode* pNode);
void DisplayMarker(FbxNode* pNode);
void DisplayGeometricTransform(FbxNode* pNode);
void DisplayTransformPropagation(FbxNode* pNode);

#endif
