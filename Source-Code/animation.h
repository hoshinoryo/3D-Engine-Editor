/*==============================================================================

   アニメーション管理 [animation.h]
														 Author : Gu Anyi
														 Date   : 2025/11/21

--------------------------------------------------------------------------------

==============================================================================*/

#ifndef ANIMATION_H
#define ANIMATION_H

#include "model.h"

#include <string>
#include <vector>
#include <DirectXMath.h>


namespace SkeletonUtil
{
	const aiNode* FindNodeByName(const aiScene* scene, const std::string& name);
}

/*
// -------------------------------
AnimationClip
└─ tracks[]: ボーンのアニメーションデータ

AnimationPlayer
├─ Play()：指定されたAnimationClipとModelAssetを再生開始する
├─ Update()：アニメーション更新
├─ SampleLocalTransform() : 1ボーンに対して「ローカル変換行列」を生成する
├─ BuildModelSpacePoseRecursive() : 全ボーンの「モデル空間での姿勢行列」を構築する
└─ ComputeSkinMatrices() : GPUに送る「スキン行列（bone matrices）」を生成する

AnimationManager
└─ 全ての AnimationClip* の管理
// -------------------------------
*/


// キーフレーム
struct Keyframe
{
	double time;

	DirectX::XMFLOAT3 translation;
	DirectX::XMFLOAT4 rotation;
	DirectX::XMFLOAT3 scale;
};

// ボーンのアニメーション
struct BoneAnimTrack
{
	const aiNode* node = nullptr; // bone node
	std::string nodeName;

	std::vector<Keyframe> keyframes; // キーフレーム列
};

// アニメーションクリップ
struct AnimationClip
{
	std::string animName;
	double duration;
	double ticksPerSecond;

	std::vector<BoneAnimTrack> tracks;

	bool SourceYup = true;
	bool loop = true;
};

// アニメーションの読み込み
AnimationClip* Animation_LoadFromFile(const char* filename, const ModelAsset* asset, bool animYup);
void Animation_DestroyClip(AnimationClip* clip);


// アニメーション管理クラス
class AnimationManager
{
private:

	std::vector<AnimationClip*> m_Clips;


	AnimationManager() = default;
	~AnimationManager() = default;

	AnimationManager(const AnimationManager&) = delete;
	AnimationManager& operator=(const AnimationManager&) = delete;

public:

	static AnimationManager& Instance();

	int RegisterClip(AnimationClip* clip);
	void DestroyAll();

	AnimationClip* GetClipById(int id) const;
	AnimationClip* FindClipByName(const std::string& name) const;
};



class AnimationPlayer
{
private:

	const ModelAsset* m_Asset = nullptr;
	const AnimationClip* m_Clip = nullptr;
	bool m_Playing = false;
	bool m_Loop = true;
	double m_CurrentTimeTicks = 0.0;

	mutable std::unordered_map<const aiNode*, DirectX::XMMATRIX> m_CurrentPose; // last pose

private:

	DirectX::XMMATRIX SampleLocalTransform(const aiNode * node, double tickTimes) const;

	void BuildModelSpacePoseResursive(
		const aiNode* node,
		const DirectX::XMMATRIX& parentMtx,
		double timeTicks,
		std::unordered_map<const aiNode*, DirectX::XMMATRIX>& outNodeModelMtx
	) const;

public:

	AnimationPlayer();

	void Play(const AnimationClip* clip, const ModelAsset* asset, bool loop = true, double startTimeSec = 0.0);

	void Stop();

	void Update(double elapsed_time);

	bool IsPLaying() const { return m_Playing && m_Clip && m_Asset; }

	const ModelAsset* GetAsset();
	const AnimationClip* GetCurrentClip() const { return m_Clip; }
	double GetCurrentTimeSec() const;
	void GetCurrentPose(std::unordered_map<const aiNode*, DirectX::XMMATRIX>& outPose) const;

	void ComputeSkinMatrices(std::vector<DirectX::XMFLOAT4X4>& outBoneMatrix) const;
};


// スキニング用定数バッファの管理
bool Animation_InitializeSkinningCB(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Animation_ReleaseSkinningCB();
void Animation_UpdateSkinningCB(const AnimationPlayer& player);
//void Animation_DisableSkinning();

#endif // ANIMATION_H
