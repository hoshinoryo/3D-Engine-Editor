/*==============================================================================

   アニメーション管理 [animation.cpp]
														 Author : Gu Anyi
														 Date   : 2025/11/21

--------------------------------------------------------------------------------

==============================================================================*/

#include "assimp/scene.h"
#include "assimp//Importer.hpp"
#include "assimp/postprocess.h"

#include "animation.h"
#include "model.h"
#include "skeleton_util.h"
#include "axis_util.h"
#include "direct3d.h"

#include <cassert>
#include <algorithm>
#include <DirectXMath.h>

using namespace DirectX;

static const int MAX_BONES = 256;

ID3D11Device* g_pDevice = nullptr;
ID3D11DeviceContext* g_pContext = nullptr;

ID3D11Buffer* g_pSkinningCB = nullptr; // skin weight用バッファポイント

struct BoneRef
{
	const aiBone* bone = nullptr; // aiMesh::mBones[b]
	const aiNode* node = nullptr; // SkeletonUtil::FindNodeByNameで見つけたノード
};

struct SkinningCBData
{
	XMFLOAT4X4 boneMatrices[MAX_BONES];
};

// ---- Assimp型 -> DirectXMath型変換 ----
static XMFLOAT3 ToXMFLOAT3(const aiVector3D& v);
static XMFLOAT4 ToXMFLOAT4(const aiQuaternion& q);

static XMMATRIX AiMatToXM(const aiMatrix4x4& m);
// ---- 線形補間法ヘルパー ----
static void LerpFloat3(const XMFLOAT3& a, const XMFLOAT3& b, float t, XMFLOAT3& out);
static void SlerpQuat(const XMFLOAT4& qa, const XMFLOAT4& qb, float t, XMFLOAT4& out);

static BoneAnimTrack CreateBoneAnimationFromChannel(
	const aiNodeAnim* channel,
	const MODEL* model,
	const aiScene* scene,
	double& outMaxTime
);
static XMMATRIX GetAxisFixMatrix(bool yUp); // Z-up to Y-up
static const BoneAnimTrack* FindTrackForNode(const AnimationClip* clip, const aiNode* node);
static void SampleTrack(
	const BoneAnimTrack& track,
	double timeTicks,
	XMFLOAT3& outT,
	XMFLOAT4& outR,
	XMFLOAT3& outS
);
static const char* GetShortName(const char* fullName);


// ---- 読み込み ----
AnimationClip* Animation_LoadFromFile(const char* filename, const MODEL* model, bool animYup)
{
	assert(filename);
	assert(model);
	assert(model->AiScene);

	//Assimp::Importer importer;

	const aiScene* scene = aiImportFile(
		filename,
		aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_ConvertToLeftHanded
	);
	if (!scene || !scene->HasAnimations()) return nullptr;

	const aiAnimation* anim = scene->mAnimations[0];
	if (!anim) return nullptr;

	AnimationClip* clip = new AnimationClip();
	clip->SourceYup = animYup; // IMPORTANT: import up axis

	if (anim->mName.length > 0)
		clip->animName = anim->mName.C_Str();
	else
		clip->animName = filename;

	clip->ticksPerSecond = (anim->mTicksPerSecond != 0.0) ? anim->mTicksPerSecond : 30.0;

	double durationFromAnim = anim->mDuration;
	double durationFromKey = 0.0;

	clip->tracks.reserve(anim->mNumChannels);

	for (unsigned int i = 0; i < anim->mNumChannels; ++i)
	{
		const aiNodeAnim* channel = anim->mChannels[i];
		if (!channel) continue;

		BoneAnimTrack track = CreateBoneAnimationFromChannel(channel, model, model->AiScene, durationFromKey);

		clip->tracks.push_back(std::move(track));
	}

	clip->duration = std::max(durationFromAnim, durationFromKey);
	clip->loop = true;

	return clip;
}

void Animation_DestroyClip(AnimationClip* clip)
{
	delete clip;
}

bool Animation_InitializeSkinningCB(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	if (!pDevice || !pContext) return false;
	g_pDevice = pDevice;
	g_pContext = pContext;

	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(SkinningCBData);
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffer_desc.CPUAccessFlags = 0;
	buffer_desc.MiscFlags = 0;

	SkinningCBData initData{};
	for (int i = 0; i < MAX_BONES; i++)
	{
		XMStoreFloat4x4(&initData.boneMatrices[i], XMMatrixIdentity());
	}

	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = &initData;

	HRESULT hr = g_pDevice->CreateBuffer(&buffer_desc, &sd, &g_pSkinningCB);
	if (FAILED(hr))
	{
		g_pSkinningCB = nullptr;
		return false;
	}

	return true;
}

void Animation_ReleaseSkinningCB()
{
	SAFE_RELEASE(g_pSkinningCB);
}

void Animation_UpdateSkinningCB(const AnimationPlayer& player)
{
	if (!g_pSkinningCB) return;
	
	std::vector<XMFLOAT4X4> skinMatrices;
	player.ComputeSkinMatrices(skinMatrices);

	SkinningCBData cbData{};
	for (int i = 0; i < MAX_BONES; ++i)
	{
		XMStoreFloat4x4(&cbData.boneMatrices[i], XMMatrixIdentity());
	}
	
	int count = static_cast<int>(skinMatrices.size());
	if (count > MAX_BONES) count = MAX_BONES;

	for (int i = 0; i < count; ++i)
	{
		cbData.boneMatrices[i] = skinMatrices[i];
	}

	g_pContext->UpdateSubresource(g_pSkinningCB, 0, nullptr, &cbData, 0, 0);
	g_pContext->VSSetConstantBuffers(3, 1, &g_pSkinningCB);
}

// no-op: shader variant decides skinning path
/*
void Animation_DisableSkinning()
{
	if (!g_pSkinningCB || !g_pContext) return;

	SkinningCBData cbData{};

	g_pContext->UpdateSubresource(g_pSkinningCB, 0, nullptr, &cbData, 0, 0);
	g_pContext->VSSetConstantBuffers(3, 1, &g_pSkinningCB);
}
*/


// ------------------------------------------
// Animation Manager
// ------------------------------------------

AnimationManager& AnimationManager::Instance()
{
	static AnimationManager s_instance;
	return s_instance;
}

int AnimationManager::RegisterClip(AnimationClip* clip)
{
	if (!clip) return -1;

	m_Clips.push_back(clip);
	return static_cast<int>(m_Clips.size() - 1);
}

void AnimationManager::DestroyAll()
{
	for (AnimationClip* c : m_Clips)
	{
		delete c;
	}

	m_Clips.clear();
}

AnimationClip* AnimationManager::GetClipById(int id) const
{
	if (id < 0 || id >= static_cast<int>(m_Clips.size())) return nullptr;
	return m_Clips[id];
}

AnimationClip* AnimationManager::FindClipByName(const std::string& name) const
{
	for (AnimationClip* c : m_Clips)
	{
		if (c && c->animName == name) return c;
	}

	return nullptr;
}

// ------------------------------------------
// Animation Player
// ------------------------------------------

AnimationPlayer::AnimationPlayer()
{
	m_Playing = false;
	m_Loop = true;
	m_CurrentTimeTicks = 0.0;
	m_Model = nullptr;
	m_Clip = nullptr;
}

void AnimationPlayer::Play(const AnimationClip* clip, const MODEL* model, bool loop, double startTimeSec)
{
	m_Clip = clip;
	m_Model = model;
	m_Loop = loop;

	if (!m_Clip || !m_Model)
	{
		m_Playing = false;
		m_CurrentTimeTicks = 0.0;
		return;
	}

	m_Playing = true;

	double startTicks = startTimeSec * m_Clip->ticksPerSecond;
	if (m_Clip->duration > 0.0)
	{
		startTicks = fmod(startTicks, m_Clip->duration);
		if (startTicks < 0.0)
		{
			startTicks += m_Clip->duration;
		}
	}

	m_CurrentTimeTicks = startTicks;
}

void AnimationPlayer::Stop()
{
	m_Playing = false;
	m_CurrentTimeTicks = 0.0;
}

void AnimationPlayer::Update(double elapsed_time)
{
	if (!m_Playing || !m_Clip) return;

	double deltaTicks = elapsed_time * m_Clip->ticksPerSecond;
	m_CurrentTimeTicks += deltaTicks;

	if (m_Clip->duration > 0.0)
	{
		// Keep in safe range
		if (m_Loop)
		{
			m_CurrentTimeTicks = fmod(m_CurrentTimeTicks, m_Clip->duration);
			if (m_CurrentTimeTicks < 0.0)
			{
				m_CurrentTimeTicks += m_Clip->duration;
			}
		}
		else
		{
			if (m_CurrentTimeTicks > m_Clip->duration)
			{
				m_CurrentTimeTicks = m_Clip->duration;
				m_Playing = false;
			}
		}
	}
}

const MODEL* AnimationPlayer::GetModel()
{
	return m_Model;
}

double AnimationPlayer::GetCurrentTimeSec() const
{
	if (!m_Clip || m_Clip->ticksPerSecond == 0.0)
	{
		return 0.0;
	}
	return m_CurrentTimeTicks / m_Clip->ticksPerSecond;
}

void AnimationPlayer::GetCurrentPose(std::unordered_map<const aiNode*, DirectX::XMMATRIX>& outPose) const
{
	outPose = m_CurrentPose;
}

// Get local transform matrix
XMMATRIX AnimationPlayer::SampleLocalTransform(const aiNode* node, double tickTimes) const
{
	const BoneAnimTrack* track = FindTrackForNode(m_Clip, node);

	XMFLOAT3 T(0, 0, 0);
	XMFLOAT4 R(0, 0, 0, 1);
	XMFLOAT3 S(1, 1, 1);

	// If has track -> use track animation
	if (track && !track->keyframes.empty())
	{
		SampleTrack(*track, tickTimes, T, R, S);

		// debug
		static double s_lastPrintTime = -1.0;
		if (node && strcmp(node->mName.C_Str(), "clavicle_l") == 0)
		{
			if (fabs(tickTimes - s_lastPrintTime) > 1.0)
			{
				char buf[256];
				sprintf_s(buf, "[SampleLocalTransform] node = %s t = %.3f R = (%.3f, %.3f, %.3f)\n",
					node->mName.C_Str(), 
					tickTimes,
					R.x, R.y, R.z);
				OutputDebugStringA(buf);
				s_lastPrintTime = tickTimes;
			}
		}
	}
	else // If has no track -> use bind pose
	{
		const aiMatrix4x4& m = node->mTransformation;
		XMMATRIX xm = AiMatToXM(m);

		return xm;
	}

	XMVECTOR vT = XMLoadFloat3(&T);
	XMVECTOR vR = XMLoadFloat4(&R);
	XMVECTOR vS = XMLoadFloat3(&S);
	
	XMMATRIX mtx =
		XMMatrixScalingFromVector(vS) *
		XMMatrixRotationQuaternion(vR) *
		XMMatrixTranslationFromVector(vT);

	return mtx;
}

// Matrix in model space for each bone
void AnimationPlayer::BuildModelSpacePoseResursive(
	const aiNode* node,
	const XMMATRIX& parentMtx,
	double timeTicks,
	std::unordered_map<const aiNode*, XMMATRIX>& outNodeModelMtx
) const
{
	if (!node) return;

	XMMATRIX local = SampleLocalTransform(node, timeTicks); // local matrix

	XMMATRIX modelSpace = local * parentMtx; // DO NOT DO ANY MORE AXIS FIXING

	outNodeModelMtx[node] = modelSpace;

	for (unsigned int i = 0; i < node->mNumChildren; ++i)
	{
		BuildModelSpacePoseResursive(node->mChildren[i], modelSpace, timeTicks, outNodeModelMtx);
	}
}

// All skin matrix data in model space
// Convert from bind pose to the playing pose
// MIND THE SCALING!!!
void AnimationPlayer::ComputeSkinMatrices(std::vector<DirectX::XMFLOAT4X4>& outBoneMatrix) const
{
	outBoneMatrix.clear();

	if (!m_Model || !m_Clip || !m_Model->AiScene) return;

	const aiScene* scene = m_Model->AiScene;

	// 1. Build aiNode and its model matrix
	std::unordered_map<const aiNode*, XMMATRIX> nodeModelMtx;
	XMMATRIX rootParent = XMMatrixIdentity(); // parent node for root node

	
	// Z-up tp Y-up
	if (m_Model && m_Clip)
	{
		UpAxis modelUp = UpFromBool(m_Model->SourceYup);
		UpAxis animUp = UpFromBool(m_Clip->SourceYup);

		if (animUp != modelUp)
		{
			XMMATRIX axisFix = GetAxisConversion(animUp, modelUp);
			//XMMATRIX axisFix = XMMatrixRotationX(-XM_PIDIV2);
			rootParent = axisFix;
		}
	}

	BuildModelSpacePoseResursive(scene->mRootNode, rootParent, m_CurrentTimeTicks, nodeModelMtx);

	m_CurrentPose = nodeModelMtx; // save the pose

	// 2. Collect all bones and make a table for name and index
	const auto& boneMap = m_Model->BoneNameToIndex;
	if (boneMap.empty()) 
		return;

	// 3. Bone name -> aiBone* テーブルの構築
	std::unordered_map<std::string, const aiBone*> nameToBone;

	for (unsigned int m = 0; m < scene->mNumMeshes; ++m)
	{
		const aiMesh* mesh = scene->mMeshes[m];
		if (!mesh || mesh->mNumBones == 0) continue;

		for (unsigned int b = 0; b < mesh->mNumBones; ++b)
		{
			const aiBone* bone = mesh->mBones[b];
			if (!bone) continue;

			std::string boneName = bone->mName.C_Str();

			if (nameToBone.find(boneName) == nameToBone.end())
			{
				nameToBone[boneName] = bone;
			}
		}
	}

	if (nameToBone.empty())
		return;

	outBoneMatrix.resize(boneMap.size());

	// debug flag: only log once
	static bool s_debugPrinted = false;

	// 4. Skin matrix computation for each bone
	// M_skin = M_offset * M_modelSpace
	for (const auto& kv : boneMap)
	{
		const std::string& boneName = kv.first;
		int index = kv.second;
		
		const aiNode* node = SkeletonUtil::FindNodeByName(scene, boneName);
		if (!node) continue;

		auto itM = nodeModelMtx.find(node);
		if (itM == nodeModelMtx.end()) continue;

		// debug: check if there is a track for this node
		/*
		if (!s_debugPrinted)
		{
			const BoneAnimTrack* tr = FindTrackForNode(m_Clip, node);

			if (tr)
				OutputDebugStringA((" [Anim] bone '" + boneName + "' use TRACK.\n").c_str());
			else
				OutputDebugStringA((" [Anim] bone '" + boneName + "' use BIND POSE.\n").c_str());
		}
		*/

		XMMATRIX G_current = itM->second; // model-space(animated)

		auto itB = nameToBone.find(boneName);
		if (itB == nameToBone.end() || !itB->second)
			continue;

		const aiBone* bone = itB->second;

		// DO NOT TOUCH!!!
		// Skin Matrix : local(vertex) -> bone space -> animated model space
		XMMATRIX offset = AiMatToXM(bone->mOffsetMatrix);
		XMMATRIX skinMtx = offset * G_current;

		// HLSLのへの転置
		XMMATRIX skinMtxT = XMMatrixTranspose(skinMtx);

		if (index >= 0 && index < static_cast<int>(outBoneMatrix.size()))
		{
			XMStoreFloat4x4(&outBoneMatrix[index], skinMtxT);
		}
	}
}


static XMFLOAT3 ToXMFLOAT3(const aiVector3D& v)
{
	return XMFLOAT3(v.x, v.y, v.z);
}

static XMFLOAT4 ToXMFLOAT4(const aiQuaternion& q)
{
	return XMFLOAT4(q.x, q.y, q.z, q.w);
}

static XMMATRIX AiMatToXM(const aiMatrix4x4& m)
{
	return XMMATRIX(
		m.a1, m.b1, m.c1, m.d1,
		m.a2, m.b2, m.c2, m.d2,
		m.a3, m.b3, m.c3, m.d3,
		m.a4, m.b4, m.c4, m.d4
	);
};

//template<typename T>
static void LerpFloat3(const XMFLOAT3& a, const XMFLOAT3& b, float t, XMFLOAT3& out)
{
	out.x = a.x + (b.x - a.x) * t;
	out.y = a.y + (b.y - a.y) * t;
	out.z = a.z + (b.z - a.z) * t;
}

static void SlerpQuat(const XMFLOAT4& qa, const XMFLOAT4& qb, float t, XMFLOAT4& out)
{
	XMVECTOR A = XMLoadFloat4(&qa);
	XMVECTOR B = XMLoadFloat4(&qb);

	XMVECTOR Q = XMQuaternionSlerp(A, B, t);
	XMStoreFloat4(&out, Q);
}

static XMMATRIX GetAxisFixMatrix(bool yUp)
{
	if (yUp)
	{
		return XMMatrixIdentity();
	}
	else
	{
		// Z-up to Y-up
		return XMMatrixRotationX(XM_PIDIV2);
	}
}

static BoneAnimTrack CreateBoneAnimationFromChannel(
	const aiNodeAnim* channel,
	const MODEL* model,
	const aiScene* scene,
	double& outMaxTime
)
{
	BoneAnimTrack track;

	// Find aiNode by name
	std::string nodeName = channel->mNodeName.C_Str();
	track.nodeName = nodeName;
	track.node = SkeletonUtil::FindNodeByName(scene, nodeName);

	// debug
	if (nodeName == "clavicle_l")
	{
		char buf[256];
		sprintf_s(buf, "[Channel] %s: pos = %u rot = %u \n",
			nodeName.c_str(),
			channel->mNumPositionKeys,
			channel->mNumRotationKeys);
		OutputDebugStringA(buf);
	}

	unsigned int numPos = channel->mNumPositionKeys;
	unsigned int numRot = channel->mNumRotationKeys;
	unsigned int numScl = channel->mNumScalingKeys;

	unsigned int numKeys = std::max({ numPos, numRot, numScl });
	track.keyframes.reserve(numKeys);

	for (unsigned int i = 0; i < numKeys; ++i)
	{
		Keyframe kf{};

		const aiVectorKey& posKey = channel->mPositionKeys[(numPos > 0) ? std::min(i, numPos - 1) : 0];
		const aiQuatKey&   rotKey = channel->mRotationKeys[(numRot > 0) ? std::min(i, numRot - 1) : 0];
		const aiVectorKey& sclKey = channel->mScalingKeys [(numScl > 0) ? std::min(i, numScl - 1) : 0];

		double timePos = (numPos > 0) ? posKey.mTime : 0.0;
		double timeRot = (numRot > 0) ? rotKey.mTime : 0.0;
		double timeScl = (numScl > 0) ? sclKey.mTime : 0.0;

		//kf.time = posKey.mTime;
		kf.time = std::max({ timePos, timeRot, timeScl });
		kf.translation = ToXMFLOAT3(posKey.mValue);
		kf.rotation = ToXMFLOAT4(rotKey.mValue);
		kf.scale = ToXMFLOAT3(sclKey.mValue);

		outMaxTime = std::max(outMaxTime, kf.time);
		
		track.keyframes.push_back(kf);
	}

	return track;
}

static const BoneAnimTrack* FindTrackForNode(const AnimationClip* clip, const aiNode* node)
{
	if (!clip || !node) return nullptr;

	const char* nodeNameFull = node->mName.C_Str();
	const char* nodeNameShort = GetShortName(nodeNameFull);

	const BoneAnimTrack* candidateShort = nullptr;
	const BoneAnimTrack* candidatePtr = nullptr;


	for (const BoneAnimTrack& t : clip->tracks)
	{
		if (t.nodeName.empty())
			continue;

		const char* trackNameFull = t.nodeName.c_str();
		const char* trackNameShort = GetShortName(trackNameFull);

		// strict full-name match
		if (strcmp(trackNameFull, nodeNameFull) == 0)
		{
			return &t;
		}

		// short-name match
		if (strcmp(trackNameShort, nodeNameShort) == 0)
		{
			if (!candidateShort)
			{
				candidateShort = &t; // remember first short-name match
			}
		}

		// remember pointer match
		if (t.node && t.node == node)
		{
			candidatePtr = &t;
		}
	}

	if (candidateShort)
		return candidateShort;
	if (candidatePtr)
		return candidatePtr;

	return nullptr;
}

static void SampleTrack(
	const BoneAnimTrack& track,
	double timeTicks,
	XMFLOAT3& outT,
	XMFLOAT4& outR,
	XMFLOAT3& outS
)
{
	const auto& keys = track.keyframes;
	if (keys.empty())
	{
		outT = XMFLOAT3(0, 0, 0);
		outR = XMFLOAT4(0, 0, 0, 1);
		outS = XMFLOAT3(1, 1, 1);
		return;
	}

	double duration = keys.back().time;
	if (duration > 0.0)
	{
		timeTicks = fmod(timeTicks, duration);
		if (timeTicks < 0.0) timeTicks += duration;
	}

	size_t k1 = 0;
	size_t k2 = 0;

	for (size_t i = 0; i < keys.size(); ++i)
	{
		if (keys[i].time >= timeTicks)
		{
			k2 = i;
			k1 = (i > 0) ? i - 1 : 0;
			break;
		}
	}

	if (k1 == k2)
	{
		outT = keys[k1].translation;
		outR = keys[k1].rotation;
		outS = keys[k1].scale;
		return;
	}

	const Keyframe& keyA = keys[k1];
	const Keyframe& keyB = keys[k2];

	double span = keyB.time - keyA.time;
	float t = 0.0f;
	if (span > 0.0)
	{
		t = static_cast<float>((timeTicks - keyA.time) / span);
	}

	LerpFloat3(keyA.translation, keyB.translation, t, outT);
	SlerpQuat(keyA.rotation, keyB.rotation, t, outR);
	LerpFloat3(keyA.scale, keyB.scale, t, outS);
}

static const char* GetShortName(const char* fullName)
{
	if (!fullName) return "";

	const char* last = fullName;
	const char* p = fullName;

	while (*p)
	{
		if (*p == '|' || *p == ':' || *p == '/' || *p == '\\')
		{
			last = p + 1;
		}
		++p;
	}

	return last;
}
