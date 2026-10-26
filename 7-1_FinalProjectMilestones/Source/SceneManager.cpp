///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture slots before loading image data
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}

	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
 *  LoadSceneTextures()
 *
 *  Loads the wood and metal textures used by the desk lamp.
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	// wood texture for the wooden lamp components
	CreateGLTexture(
		"../../Utilities/textures/knife_handle.jpg",
		"wood");

	// metal texture for the metal lamp components
	CreateGLTexture(
		"../../Utilities/textures/stainless.jpg",
		"metal");

	// light surface texture for the tabletop
	CreateGLTexture(
		"../../Utilities/textures/drywall.jpg",
		"surface");

	// bind the loaded images to OpenGL texture slots
	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  Defines the material properties used by the lamp and
 *  tabletop when lighting is applied.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	// Tabletop material with a soft matte response
	OBJECT_MATERIAL surfaceMaterial;
	surfaceMaterial.ambientColor = glm::vec3(0.32f, 0.28f, 0.24f);
	surfaceMaterial.ambientStrength = 0.28f;
	surfaceMaterial.diffuseColor = glm::vec3(0.48f, 0.40f, 0.32f);
	surfaceMaterial.specularColor = glm::vec3(0.05f, 0.04f, 0.03f);
	surfaceMaterial.shininess = 2.0f;
	surfaceMaterial.tag = "surfaceMaterial";
	m_objectMaterials.push_back(surfaceMaterial);

	// Wood material with a softer, less reflective surface
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.30f, 0.25f, 0.20f);
	woodMaterial.ambientStrength = 0.30f;
	woodMaterial.diffuseColor = glm::vec3(0.55f, 0.45f, 0.35f);
	woodMaterial.specularColor = glm::vec3(0.12f, 0.12f, 0.12f);
	woodMaterial.shininess = 4.0f;
	woodMaterial.tag = "woodMaterial";
	m_objectMaterials.push_back(woodMaterial);

	// Metal material with stronger highlights
	OBJECT_MATERIAL metalMaterial;
	metalMaterial.ambientColor = glm::vec3(0.20f, 0.20f, 0.20f);
	metalMaterial.ambientStrength = 0.25f;
	metalMaterial.diffuseColor = glm::vec3(0.38f, 0.38f, 0.38f);
	metalMaterial.specularColor = glm::vec3(0.35f, 0.35f, 0.35f);
	metalMaterial.shininess = 12.0f;
	metalMaterial.tag = "metalMaterial";
	m_objectMaterials.push_back(metalMaterial);
}

void SceneManager::SetupSceneLights()
{
	// turn on custom lighting for the scene
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Warm main light placed just below the lampshade
	m_pShaderManager->setVec3Value(
		"lightSources[0].position",
		-0.8f, 4.3f, 1.0f);

	m_pShaderManager->setVec3Value(
		"lightSources[0].ambientColor",
		0.09f, 0.055f, 0.025f);

	m_pShaderManager->setVec3Value(
		"lightSources[0].diffuseColor",
		0.58f, 0.34f, 0.15f);

	m_pShaderManager->setVec3Value(
		"lightSources[0].specularColor",
		0.16f, 0.10f, 0.05f);

	m_pShaderManager->setFloatValue(
		"lightSources[0].focalStrength",
		20.0f);

	m_pShaderManager->setFloatValue(
		"lightSources[0].specularIntensity",
		0.035f);


	// Weak warm fill light keeps the back side of the lamp visible
	m_pShaderManager->setVec3Value(
		"lightSources[1].position",
		5.0f, 6.0f, 4.0f);

	m_pShaderManager->setVec3Value(
		"lightSources[1].ambientColor",
		0.035f, 0.030f, 0.025f);

	m_pShaderManager->setVec3Value(
		"lightSources[1].diffuseColor",
		0.10f, 0.08f, 0.06f);

	m_pShaderManager->setVec3Value(
		"lightSources[1].specularColor",
		0.05f, 0.04f, 0.03f);

	m_pShaderManager->setFloatValue(
		"lightSources[1].focalStrength",
		20.0f);

	m_pShaderManager->setFloatValue(
		"lightSources[1].specularIntensity",
		0.02f);

	// unused light slots are disabled
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 1.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.0f);

	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 1.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.0f);
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	// load the textures before rendering the lamp
	LoadSceneTextures();

	// define how the textured surfaces respond to lighting
	DefineObjectMaterials();

	// add the lights used to illuminate the scene
	SetupSceneLights();

	// Load the meshes needed to build the objects
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadBoxMesh();

}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(30.0f, 1.0f, 12.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// apply a light texture across the tabletop
	SetShaderTexture("surface");
	SetTextureUVScale(4.0f, 4.0f);
	SetShaderMaterial("surfaceMaterial");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/****************************************************************/

	// Create the round base of the desk lamp
	scaleXYZ = glm::vec3(2.2f, 0.5f, 2.2f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(3.5f, 0.0f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// apply wood grain to the lamp base
	SetShaderTexture("wood");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("woodMaterial");

	m_basicMeshes->DrawCylinderMesh();

	// Create the lower vertical support of the desk lamp
	scaleXYZ = glm::vec3(0.30f, 4.0f, 0.30f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(3.5f, 0.5f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// tile the metal texture across the cylindrical support
	SetShaderTexture("metal");
	SetTextureUVScale(3.0f, 2.0f);
	SetShaderMaterial("metalMaterial");

	m_basicMeshes->DrawCylinderMesh();

	// Add the wooden section near the top of the vertical support
	scaleXYZ = glm::vec3(0.38f, 1.5f, 0.38f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(3.5f, 2.9f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// apply wood texture to the upper wooden sleeve
	SetShaderTexture("wood");
	SetTextureUVScale(1.0f, 1.5f);
	SetShaderMaterial("woodMaterial");

	m_basicMeshes->DrawCylinderMesh();


	// Create the pivot joint above the vertical support
	scaleXYZ = glm::vec3(0.55f, 0.55f, 0.55f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(3.5f, 4.5f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// apply metal texture to the lower pivot joint
	SetShaderTexture("metal");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("metalMaterial");

	m_basicMeshes->DrawSphereMesh();

	// Create the angled wooden arm starting from the lower joint
	scaleXYZ = glm::vec3(0.35f, 3.0f, 0.35f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 45.0f;

	positionXYZ = glm::vec3(3.5f, 4.5f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// stretch the wood grain along the longer angled arm
	SetShaderTexture("wood");
	SetTextureUVScale(1.0f, 2.5f);
	SetShaderMaterial("woodMaterial");

	m_basicMeshes->DrawCylinderMesh();


	// Create the upper pivot joint at the end of the angled arm
	scaleXYZ = glm::vec3(0.50f, 0.50f, 0.50f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(1.38f, 6.62f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// apply metal texture to the upper pivot joint
	SetShaderTexture("metal");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("metalMaterial");

	m_basicMeshes->DrawSphereMesh();

	// Create the tapered lampshade
	scaleXYZ = glm::vec3(1.6f, 2.0f, 1.6f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -45.0f;

	positionXYZ = glm::vec3(0.25f, 5.45f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// tile the metal texture around the lampshade
	SetShaderTexture("metal");
	SetTextureUVScale(3.0f, 2.0f);
	SetShaderMaterial("metalMaterial");

	m_basicMeshes->DrawConeMesh();

	/****************************************************************/
	// Create the left side of the book cover
	scaleXYZ = glm::vec3(3.2f, 0.12f, 2.8f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -7.0f;

	positionXYZ = glm::vec3(-3.55f, 0.12f, 1.45f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Use the wood texture to create a brown book cover
	SetShaderTexture("wood");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("woodMaterial");

	m_basicMeshes->DrawBoxMesh();


	/****************************************************************/
	// Create the right side of the book cover
	scaleXYZ = glm::vec3(3.2f, 0.12f, 2.8f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 7.0f;

	positionXYZ = glm::vec3(-0.55f, 0.12f, 1.45f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("wood");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("woodMaterial");

	m_basicMeshes->DrawBoxMesh();


	/****************************************************************/
	// Create the left group of open book pages
	scaleXYZ = glm::vec3(3.0f, 0.10f, 2.6f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -7.0f;

	positionXYZ = glm::vec3(-3.45f, 0.24f, 1.45f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Use a light page color for the open book
	SetShaderColor(0.92f, 0.86f, 0.72f, 1.0f);
	SetShaderMaterial("surfaceMaterial");

	m_basicMeshes->DrawBoxMesh();


	/****************************************************************/
	// Create the right group of open book pages
	scaleXYZ = glm::vec3(3.0f, 0.10f, 2.6f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 7.0f;

	positionXYZ = glm::vec3(-0.65f, 0.24f, 1.45f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.92f, 0.86f, 0.72f, 1.0f);
	SetShaderMaterial("surfaceMaterial");

	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	// Create the upright body of the mug
	scaleXYZ = glm::vec3(0.90f, 1.45f, 0.90f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-6.25f, 0.05f, 1.55f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Use a blue-gray color for the ceramic mug
	SetShaderColor(0.30f, 0.36f, 0.43f, 1.0f);
	SetShaderMaterial("surfaceMaterial");

	m_basicMeshes->DrawCylinderMesh();


	/****************************************************************/
	// Add a smaller dark surface at the top to make the mug look hollow
	scaleXYZ = glm::vec3(0.73f, 0.035f, 0.73f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-6.25f, 1.49f, 1.55f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Dark interior leaves the outer edge of the mug visible as a rim
	SetShaderColor(0.08f, 0.07f, 0.06f, 1.0f);
	SetShaderMaterial("surfaceMaterial");

	m_basicMeshes->DrawCylinderMesh();


	/****************************************************************/
	// Create the upper connection of the mug handle
	scaleXYZ = glm::vec3(0.75f, 0.18f, 0.28f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-7.18f, 1.15f, 1.55f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.30f, 0.36f, 0.43f, 1.0f);
	SetShaderMaterial("surfaceMaterial");

	m_basicMeshes->DrawBoxMesh();


	/****************************************************************/
	// Create the outer vertical section of the mug handle
	scaleXYZ = glm::vec3(0.22f, 0.90f, 0.28f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-7.45f, 0.75f, 1.55f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.30f, 0.36f, 0.43f, 1.0f);
	SetShaderMaterial("surfaceMaterial");

	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	// Create the lower connection of the mug handle
	scaleXYZ = glm::vec3(0.75f, 0.18f, 0.28f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-7.18f, 0.35f, 1.55f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.30f, 0.36f, 0.43f, 1.0f);
	SetShaderMaterial("surfaceMaterial");

	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	// Create the plant pot
	scaleXYZ = glm::vec3(1.05f, 1.35f, 1.05f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-5.05f, 0.05f, -0.65f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Light beige ceramic pot
	SetShaderColor(0.78f, 0.70f, 0.60f, 1.0f);
	SetShaderMaterial("surfaceMaterial");

	m_basicMeshes->DrawCylinderMesh();


	/****************************************************************/
	// Add dark soil to the top of the pot
	scaleXYZ = glm::vec3(0.88f, 0.05f, 0.88f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-5.05f, 1.36f, -0.65f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.14f, 0.08f, 0.04f, 1.0f);
	SetShaderMaterial("surfaceMaterial");

	m_basicMeshes->DrawCylinderMesh();


	/****************************************************************/
	// Create the center plant stem
	scaleXYZ = glm::vec3(0.09f, 1.9f, 0.09f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-5.05f, 1.35f, -0.65f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.18f, 0.35f, 0.12f, 1.0f);
	SetShaderMaterial("surfaceMaterial");

	m_basicMeshes->DrawCylinderMesh();


	/****************************************************************/
	// Create a left plant stem
	scaleXYZ = glm::vec3(0.08f, 1.45f, 0.08f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -18.0f;

	positionXYZ = glm::vec3(-5.15f, 1.45f, -0.65f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.18f, 0.35f, 0.12f, 1.0f);
	SetShaderMaterial("surfaceMaterial");

	m_basicMeshes->DrawCylinderMesh();


	/****************************************************************/
	// Create a right plant stem
	scaleXYZ = glm::vec3(0.08f, 1.55f, 0.08f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 20.0f;

	positionXYZ = glm::vec3(-4.95f, 1.45f, -0.65f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.18f, 0.35f, 0.12f, 1.0f);
	SetShaderMaterial("surfaceMaterial");

	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	// Create an upper leaf
	scaleXYZ = glm::vec3(0.35f, 0.75f, 0.18f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -25.0f;

	positionXYZ = glm::vec3(-5.35f, 3.15f, -0.65f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.22f, 0.48f, 0.15f, 1.0f);
	SetShaderMaterial("surfaceMaterial");

	m_basicMeshes->DrawSphereMesh();


	/****************************************************************/
	// Create an upper right leaf
	scaleXYZ = glm::vec3(0.35f, 0.75f, 0.18f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 30.0f;

	positionXYZ = glm::vec3(-4.72f, 2.95f, -0.65f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.20f, 0.44f, 0.13f, 1.0f);
	SetShaderMaterial("surfaceMaterial");

	m_basicMeshes->DrawSphereMesh();


	/****************************************************************/
	// Create a middle left leaf
	scaleXYZ = glm::vec3(0.32f, 0.65f, 0.18f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -55.0f;

	positionXYZ = glm::vec3(-5.62f, 2.35f, -0.65f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.25f, 0.52f, 0.16f, 1.0f);
	SetShaderMaterial("surfaceMaterial");

	m_basicMeshes->DrawSphereMesh();


	/****************************************************************/
	// Create a middle right leaf
	scaleXYZ = glm::vec3(0.32f, 0.65f, 0.18f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 55.0f;

	positionXYZ = glm::vec3(-4.50f, 2.35f, -0.65f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.25f, 0.52f, 0.16f, 1.0f);
	SetShaderMaterial("surfaceMaterial");

	m_basicMeshes->DrawSphereMesh();


	/****************************************************************/
	// Create a lower left leaf
	scaleXYZ = glm::vec3(0.30f, 0.60f, 0.18f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -70.0f;

	positionXYZ = glm::vec3(-5.55f, 1.90f, -0.65f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.20f, 0.45f, 0.13f, 1.0f);
	SetShaderMaterial("surfaceMaterial");

	m_basicMeshes->DrawSphereMesh();


	/****************************************************************/
	// Create a lower right leaf
	scaleXYZ = glm::vec3(0.30f, 0.60f, 0.18f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 70.0f;

	positionXYZ = glm::vec3(-4.58f, 1.90f, -0.65f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.20f, 0.45f, 0.13f, 1.0f);
	SetShaderMaterial("surfaceMaterial");

	m_basicMeshes->DrawSphereMesh();
	
	/****************************************************************/
	// Create the vertical background wall behind the scene
	scaleXYZ = glm::vec3(30.0f, 10.0f, 20.0f);

	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(0.0f, 5.0f, -5.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Use a light cream color so the warm lamp can illuminate the wall
	SetShaderColor(0.94f, 0.88f, 0.80f, 1.0f);
	SetShaderMaterial("surfaceMaterial");

	m_basicMeshes->DrawPlaneMesh();
}