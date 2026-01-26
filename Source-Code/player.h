/*==============================================================================

   ÉvÉåÉCÉÑÅ[êßå‰ [player.h]
														 Author : Gu Anyi
														 Date   : 2025/12/17

--------------------------------------------------------------------------------

==============================================================================*/

#ifndef PLAYER_H
#define PLAYER_H

#include <DirectXMath.h>

#include "animation.h"
#include "collision.h"
#include "aabb_provider.h"

struct ModelAsset;

enum class AnimState
{
	Idle,
	Walk,
	Run,
	Jump,
	Fall,
	None
};

class Player : public IAABBProvider
{
public:

	Player();
	~Player();

	void Initialize(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& front);
	void Finalize();

	void Update(double elapsed_time, const DirectX::XMFLOAT3& cameraFront);
	void UpdateAnimationOnly(double elapsed_time);

	void Draw(const DirectX::XMFLOAT3& cameraPosition);

	const DirectX::XMFLOAT3& GetPosition() const { return m_Position; }
	void SetPosition(const DirectX::XMFLOAT3& pos) { m_Position = pos; }

	AnimState GetState() const { return m_State; }
	void SetState(AnimState state);

	const AABB& GetAABB() const override { return m_WorldAABB; }

	bool IsOnGround() const;

private:

	DirectX::XMFLOAT3 m_Position{};
	DirectX::XMFLOAT3 m_Velocity{};
	DirectX::XMFLOAT3 m_Front{ 0.0f, 0.0f, 1.0f };

	bool m_IsJump = false;
	bool m_IsGround = false;
	bool m_IsFall = false;
	bool m_IsMoving = false;

	float m_MoveSpeed = 8.0f;
	float m_Gravity = 9.8f * 10.0f;
	float m_JumpVelocity = 40.0f;

	// Fall judgement
	bool m_WasGround = false;
	float m_FallStartY = 0.0f;
	float m_FallDistance = 0.0f;
	float m_FallAnimThredhold = 8.0f;
	float m_FallSpeedThredhold = -0.5f;

	ModelAsset* m_Asset = nullptr;
	AABB m_LocalAABB{};
	AABB m_WorldAABB{};

	void UpdateMovement(double elapsed_time, const DirectX::XMFLOAT3& cameraFront);
	void UpdatePhysics(double elapsed_time);
	void UpdateAnimation(double elapsed_time);
	void UpdateAABB();
	void UpdateState();

	void ChangeState(AnimState newState); // Status machine
	bool CheckGroundProbe(float probe = 0.05f) const;
	void CheckFallState(bool wasGround);
	void ResetIfFallen(float killY, const DirectX::XMFLOAT3& respawnPos);

	// for animation
	AnimationPlayer* m_AnimPlayer = nullptr;
	AnimState m_State = AnimState::Idle;

	AnimationClip* m_ClipIdle = nullptr;
	AnimationClip* m_ClipWalk = nullptr;
	AnimationClip* m_ClipRun = nullptr;
	AnimationClip* m_ClipJump = nullptr;
	AnimationClip* m_ClipFall = nullptr;
};


#endif // PLAYER_H