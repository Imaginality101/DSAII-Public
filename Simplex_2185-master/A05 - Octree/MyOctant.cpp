// includes
#include "MyOctantExample.h"

// namespaces
using namespace Simplex;
using namespace std;

// variables
uint MyOctant::m_uOctantCount = 0;
uint MyOctant::m_uMaxLevel = 3;
uint MyOctant::m_uIdealEntityCount = 5;

// accessers
uint MyOctant::GetOctantCount(void) { return m_uOctantCount; }
float MyOctant::GetSize(void) { return m_fSize; }
bool MyOctant::IsLeaf(void) { return m_uChildren == 0; }
vector3 MyOctant::GetCenterGlobal(void) { return m_v3Center; }
vector3 MyOctant::GetMinGlobal(void) { return m_v3Min; }
vector3 MyOctant::GetMaxGlobal(void) { return m_v3Max; }
MyOctant * MyOctant::GetParent(void) { return m_pParent; }
MyOctant * MyOctant::GetChild(uint child) { if (child > 7) return nullptr; return m_pChild[child]; }

// creates the list of all leafs that contains objects and sets initial variables 
void MyOctant::Init(void)
{
	// default values
	m_uChildren = 0;
	m_fSize = 0.0f;
	m_uID = m_uOctantCount;
	m_uLevel = 0;
	m_v3Center = vector3(0.0f);
	m_v3Max = vector3(0.0f);
	m_v3Min = vector3(0.0f);
	m_pRoot = nullptr;
	m_pParent = nullptr;

	// gets a reference to the mesh and entity managers
	m_pMeshMngr = MeshManager::GetInstance();
	m_pEntityMngr = MyEntityManager::GetInstance();

	// sets up the list of leafs and sets all initial children to null
	for (uint n = 0; n < 8; n++)
	{
		m_pChild[n] = nullptr;
	}
}

// constructor
MyOctant::MyOctant(uint maxLevel, uint idealEntityCount)
{
	// initialize defualt values
	Init();

	// set values
	m_uOctantCount = 0;
	m_uMaxLevel = maxLevel;
	m_uIdealEntityCount = idealEntityCount;
	m_uID = m_uOctantCount;
	m_pRoot = this;
	m_lChild.clear();

	// vector "list" to hold mix/max vectors
	vector<vector3> lMinMax;

	// for all of the objects, get their min/max in global space
	uint numberObjects = m_pEntityMngr->GetEntityCount();
	for (uint i = 0; i < numberObjects; i++)
	{
		MyEntity* pEntity = m_pEntityMngr->GetEntity(i);
		MyRigidBody* pRigidBody = pEntity->GetRigidBody();

		lMinMax.push_back(pRigidBody->GetMinGlobal());
		lMinMax.push_back(pRigidBody->GetMaxGlobal());
	}

	// create a new rigidbody using that data
	MyRigidBody* pRigidBody = new MyRigidBody(lMinMax);

	// grab the halfwidth to set up a default max
	vector3 vHalfWidth = pRigidBody->GetHalfWidth();
	float fMax = vHalfWidth.x;

	// find which direction is the largest and set that as the max
	for (int i = 1; i < 3; i++)
	{
		if (fMax < vHalfWidth[i])
		{
			fMax = vHalfWidth[i];
		}
	}

	// set the center of the rigidbody
	vector3 v3Center = pRigidBody->GetCenterLocal();

	// clear the list and safely delete the rigidbody
	lMinMax.clear();
	SafeDelete(pRigidBody);

	// set up the values based on the data
	m_fSize = fMax * 2.0f;
	m_v3Center = v3Center;
	m_v3Min = m_v3Center - (vector3(fMax));
	m_v3Max = m_v3Center + (vector3(fMax));

	// increment the octant count
	m_uOctantCount++;

	// construct the tree
	ConstructTree(m_uMaxLevel);
}

// constructor
MyOctant::MyOctant(vector3 center, float size)
{
	// initialize defualt values
	Init();

	// set values
	m_v3Center = center;
	m_fSize = size;
	m_v3Min = m_v3Center - (vector3(m_fSize) / 2.0f);
	m_v3Max = m_v3Center + (vector3(m_fSize) / 2.0f);

	// increment the octant count
	m_uOctantCount++;
}

// copy constructor
MyOctant::MyOctant(MyOctant const& other)
{
	// copy over the values
	m_uChildren = other.m_uChildren;
	m_v3Center = other.m_v3Center;
	m_v3Min = other.m_v3Min;
	m_v3Max = other.m_v3Max;
	m_fSize = other.m_fSize;
	m_uID = other.m_uID;
	m_uLevel = other.m_uLevel;
	m_pParent = other.m_pParent;
	m_pRoot = other.m_pRoot;
	m_lChild = other.m_lChild;

	// gets a reference to the mesh and entity managers
	m_pMeshMngr = MeshManager::GetInstance();
	m_pEntityMngr = MyEntityManager::GetInstance();

	// copies over othe children in the list of leafs
	for (uint i = 0; i < 8; i++)
	{
		m_pChild[i] = other.m_pChild[i];
	}
}

// copy assignment operator
MyOctant& MyOctant::operator=(MyOctant const& other)
{
	// if they are not the same
	if (this != &other)
	{
		// release
		Release();

		// initialize
		Init();

		// create a temp one
		MyOctant temp(other);

		// swap the temp into this one
		Swap(temp);
	}

	// return a pointer to this
	return *this;
}

// gets the size of the octant and swaps it
void MyOctant::Swap(MyOctant& other)
{
	// swaps the values
	swap(m_uChildren, other.m_uChildren);
	swap(m_fSize, other.m_fSize);
	swap(m_uID, other.m_uID);
	swap(m_uLevel, other.m_uLevel);
	swap(m_pRoot, other.m_pRoot);
	swap(m_pParent, other.m_pParent);
	swap(m_lChild, other.m_lChild);
	swap(m_v3Center, other.m_v3Center);
	swap(m_v3Min, other.m_v3Max);

	// gets a reference to the mesh and entity managers
	m_pMeshMngr = MeshManager::GetInstance();
	m_pEntityMngr = MyEntityManager::GetInstance();

	// swaps the children in the list of leafs
	for (uint i = 0; i < 8; i++)
	{
		swap(m_pChild[i], other.m_pChild[i]);
	}
}

// displays based on the index, includes the objects underneath
void MyOctant::Display(uint index, vector3 color)
{
	// if the id equals the index
	if (m_uID == index)
	{
		// add a wire cude to the render list
		m_pMeshMngr->AddWireCubeToRenderList(glm::translate(IDENTITY_M4, m_v3Center) * glm::scale(vector3(m_fSize)), color, RENDER_WIRE);

		// return
		return;
	}

	// for all of the children, call the display method
	for (uint i = 0; i < m_uChildren; i++)
	{
		m_pChild[i]->Display(index);
	}
}

// displays in the color specified
void MyOctant::Display(vector3 color)
{
	// for all of the children, call the display method
	for (uint i = 0; i < m_uChildren; i++)
	{
		m_pChild[i]->Display(color);
	}

	// add a wire cude to the render list
	m_pMeshMngr->AddWireCubeToRenderList(glm::translate(IDENTITY_M4, m_v3Center) * glm::scale(vector3(m_fSize)), color, RENDER_WIRE);
}

// displays the non empty leafs
void MyOctant::DisplayLeafs(vector3 color)
{
	// gets the number of leafs
	uint nLeafs = m_lChild.size();

	// for all of the leafs
	for (uint i = 0; i < nLeafs; i++)
	{
		// display the leafs of the children
		m_lChild[i]->DisplayLeafs(color);
	}

	// add a wire cude to the render list
	m_pMeshMngr->AddWireCubeToRenderList(glm::translate(IDENTITY_M4, m_v3Center) * glm::scale(vector3(m_fSize)), color, RENDER_WIRE);
}

// makes eight smaller octant children
void MyOctant::Subdivide(void)
{
	// if max depth
	if (m_uLevel >= m_uMaxLevel)
	{
		// return
		return;
	}

	// if already subdivided
	if (m_uChildren != 0)
	{
		// return
		return;
	}

	// variables
	m_uChildren = 8;
	float fSize = m_fSize / 4.0f;
	float fSizeD = fSize * 2.0f;
	vector3 v3Center;

	// octant 0 - bottom left back
	v3Center = m_v3Center;
	v3Center.x -= fSize;
	v3Center.y -= fSize;
	v3Center.z -= fSize;
	m_pChild[0] = new MyOctant(v3Center, fSizeD);

	// octant 1 - bottom right back
	v3Center.x += fSizeD;
	m_pChild[1] = new MyOctant(v3Center, fSizeD);

	// octant 2 - bottom right front
	v3Center.z += fSizeD;
	m_pChild[2] = new MyOctant(v3Center, fSizeD);

	// octant 3 - bottom left front
	v3Center.x -= fSizeD;
	m_pChild[3] = new MyOctant(v3Center, fSizeD);

	// octant 4 - top left front
	v3Center.y += fSizeD;
	m_pChild[4] = new MyOctant(v3Center, fSizeD);

	// octant 5 - top left back
	v3Center.z -= fSizeD;
	m_pChild[5] = new MyOctant(v3Center, fSizeD);

	// octant 6 - top right back
	v3Center.x += fSizeD;
	m_pChild[6] = new MyOctant(v3Center, fSizeD);

	// octant 7 - top right front
	v3Center.z += fSizeD;
	m_pChild[7] = new MyOctant(v3Center, fSizeD);

	// for all of the children
	for (uint i = 0; i < 8; i++)
	{
		// link the connections and set the level
		m_pChild[i]->m_pRoot = m_pRoot;
		m_pChild[i]->m_pParent = this;
		m_pChild[i]->m_uLevel = m_uLevel + 1;

		// if the child contains more than the ideal entities
		if (m_pChild[i]->ContainsMoreThan(m_uIdealEntityCount))
		{
			// subdivide again
			m_pChild[i]->Subdivide();
		}
	}
}

// checks if there is a collision with the specific entity at an index
bool MyOctant::IsColliding(uint index)
{
	// get the number of entities
	uint nObjectCount = m_pEntityMngr->GetEntityCount();

	// if the index is greater than or equal to the number of elements contained
	if (index >= nObjectCount)
	{
		// there is no collision
		return false;
	}

	// variables
	MyEntity* pEntity = m_pEntityMngr->GetEntity(index);
	MyRigidBody* pRigidBody = pEntity->GetRigidBody();
	vector3 v3MinOther = pRigidBody->GetMinGlobal();
	vector3 v3MaxOther = pRigidBody->GetMaxGlobal();

	// check x -- AABB
	if (m_v3Max.x < v3MinOther.x) { return false; }
	if (m_v3Min.x > v3MaxOther.x) { return false; }

	// check y -- AABB
	if (m_v3Max.y < v3MinOther.y) { return false; }
	if (m_v3Min.y > v3MaxOther.y) { return false; }

	// check z -- AABB
	if (m_v3Max.z < v3MinOther.z) { return false; }
	if (m_v3Min.z > v3MaxOther.z) { return false; }

	// they are colliding
	return true;
}

// checks if it contains more than X
bool MyOctant::ContainsMoreThan(uint entities)
{
	// variables
	uint nCount = 0;
	uint nObjectCount = m_pEntityMngr->GetEntityCount();

	// for all of the entities
	for (uint i = 0; i < nObjectCount; i++)
	{
		// if they are colliding, up the count
		if (IsColliding(i)) { nCount++; }

		// the count is higher
		if (nCount > entities) { return true; }
	}

	// the count is lower
	return false;
}

// traverse the tree and sets objects to the index
void MyOctant::AssignIDtoEntity(void)
{
	// for all of the children
	for (uint i = 0; i < m_uChildren; i++)
	{
		// assign ids for all of the children
		m_pChild[i]->AssignIDtoEntity();
	}

	// if a leaf
	if (m_uChildren == 0)
	{
		// variables
		uint nEntities = m_pEntityMngr->GetEntityCount();

		// for all of the entities
		for (uint i = 0; i < nEntities; i++)
		{
			// if colliding
			if (IsColliding(i))
			{
				// add it to the list
				m_EntityList.push_back(i);

				// add a dimension
				m_pEntityMngr->AddDimension(i, m_uID);
			}
		}
	}
}

// creates the list of all leafs that contain objects
void MyOctant::ConstructList(void)
{
	// for all of the children
	for (uint i = 0; i < m_uChildren; i++)
	{
		// construct a list for the children
		m_pChild[i]->ConstructList();
	}

	// if there are no entities
	if (m_EntityList.size() > 0)
	{
		// this is the root
		m_pRoot->m_lChild.push_back(this);
	}
}

// creates a tree using subdivisions, and the max number of objects and levels
void MyOctant::ConstructTree(uint maxLevel)
{
	// if not the root, return
	if (m_uLevel != 0) { return; }

	// variables
	m_uMaxLevel = maxLevel;
	m_uOctantCount = 1;

	// clear entity list
	m_EntityList.clear();

	// clear tree
	KillBranches();
	m_lChild.clear();

	// if the base, subdivide
	if (ContainsMoreThan(m_uIdealEntityCount)) { Subdivide(); }

	// give entities an id
	AssignIDtoEntity();

	// construct the list
	ConstructList();
}

// clear the entity list for each node
void MyOctant::ClearEntityList(void)
{
	// for all of the children
	for (uint i = 0; i < m_uChildren; i++)
	{
		// clear all of their children
		m_pChild[i]->ClearEntityList();
	}

	// clear the list
	m_EntityList.clear();
}

// deletes all children subsequent children
void MyOctant::KillBranches(void)
{
	// for all of the children
	for (uint i = 0; i < m_uChildren; i++)
	{
		// kill branches on the child
		m_pChild[i]->KillBranches();

		// delete 
		delete m_pChild[i];
		m_pChild[i] = nullptr;
	}

	// clear the variable
	m_uChildren = 0;
}

// clears the data
void MyOctant::Release(void)
{
	// clear the branches on the root
	if (m_uLevel == 0)
	{
		KillBranches();
	}

	// clears the data
	m_uChildren = 0;
	m_fSize = 0.0f;
	m_EntityList.clear();
	m_lChild.clear();
}

// destructor
MyOctant::~MyOctant()
{
	// release
	Release();
}