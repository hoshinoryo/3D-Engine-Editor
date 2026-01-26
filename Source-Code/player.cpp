/*==============================================================================

   プレイヤー制御 [player.cpp]
														 Author : Gu Anyi
														 Date   : 2025/12/17

--------------------------------------------------------------------------------

==============================================================================*/

#include "player.h"
//#include "model.h"
#include "model_asset.h"
#include "model_renderer.h"
#include "key_logger.h"
#include "light.h"
#include "collision.h"
#include "debug_draw_gate.h"

#include <DirectXMath.h>

using namespace DirectX;

static constexpr float AABB_HALF_W = 1.0f;
static constexpr float AABB_HALF_D = 1.0;
static constexpr float AABB_HEIGHT = 6.0f;

static constexpr float KILL_Y = -50.0f;


Player::Player()
	: m_State(AnimState::None)
{
}

Player::~Player()
{
	Finalize();
}

void Player::Initialize(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& front)
{
	m_Position = position;
	m_Velocity = { 0.0f, 0.0f, 0.0f };
	XMStoreFloat3(&m_Front, XMVector3Normalize(XMLoadFloat3(&front)));
	m_IsJump = false;

	// Load player asset
	m_Asset = ModelAsset_Load("resources/mannequin.FBX", false, 0.04f);

	// Customize AABB (hard code)
	m_LocalAABB.min = { -AABB_HALF_W,        0.0f, -AABB_HALF_D };
	m_LocalAABB.max = {  AABB_HALF_W, AABB_HEIGHT,  AABB_HALF_D };
	m_WorldAABB = m_LocalAABB;

	// Animation player
	m_AnimPlayer = new AnimationPlayer();

	// Load animation clip
	AnimationClip* idleClip = Animation_LoadFromFile("resources/Animation/Idle.fbx", m_Asset, true);
	int idleId = AnimationManager::Instance().RegisterClip(idleClip);
	m_ClipIdle = AnimationManager::Instance().GetClipById(idleId);

	AnimationClip* walkClip = Animation_LoadFromFile("resources/Animation/Walking.fbx", m_Asset, true);
	int walkId = AnimationManager::Instance().RegisterClip(walkClip);
	m_ClipWalk = AnimationManager::Instance().GetClipById(walkId);

	m_ClipRun = nullptr;

	AnimationClip* jumpClip = Animation_LoadFromFile("resources/Animation/Jump.fbx", m_Asset, true);
	int jumpId = AnimationManager::Instance().RegisterClip(jumpClip);
	m_ClipJump = AnimationManager::Instance().GetClipById(jumpId);

	AnimationClip* fallClip = Animation_LoadFromFile("resources/Animation/Falling.fbx", m_Asset, true);
	int fallId = AnimationManager::Instance().RegisterClip(fallClip);
	m_ClipFall = AnimationManager::Instance().GetClipById(fallId);

	// Initialize state
	ChangeState(AnimState::Idle);
}

void Player::Finalize()
{
	if (m_Asset)
	{
		ModelAsset_Release(m_Asset);
		m_Asset = nullptr;
	}

	m_ClipIdle = nullptr;
	m_ClipWalk = nullptr;
	m_ClipRun = nullptr;
	m_ClipJump = nullptr;
	m_ClipFall = nullptr;

	delete m_AnimPlayer;
	m_AnimPlayer = nullptr;
}

void Player::Update(double elapsed_time, const XMFLOAT3& cameraFront)
{
	UpdateMovement(elapsed_time, cameraFront);
	UpdatePhysics(elapsed_time);
	UpdateState();
	UpdateAnimation(elapsed_time);
}

void Player::UpdateAnimationOnly(double elapsed_time)
{
	UpdateAnimation(elapsed_time);
}

void Player::Draw(const XMFLOAT3& cameraPosition)
{
	if (!m_Asset) return;

	XMVECTOR pos = XMLoadFloat3(&m_Position);
	XMVECTOR front = XMLoadFloat3(&m_Front);

	if (XMVector3Equal(front, XMVectorZero()))
	{
		front = XMVectorSet(0, 0, 1, 0);
	}

	front = XMVectorSetY(front, 0.0f);
	front = XMVector3Normalize(front);

	XMVECTOR up = XMVectorSet(0, 1, 0, 0);

	// LookTo
	XMMATRIX rot = XMMatrixInverse(nullptr, XMMatrixLookToLH(XMVectorZero(), front, up));

	XMMATRIX modelFix = XMMatrixRotationY(XM_PI);

	XMMATRIX trans = XMMatrixTranslationFromVector(pos);
	XMMATRIX scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);

	XMMATRIX world = modelFix * scale * rot * trans;

	if (m_AnimPlayer)
	{
		Animation_UpdateSkinningCB(*m_AnimPlayer);
	}

	for (uint32_t mi = 0; mi < (uint32_t)m_Asset->meshes.size(); ++mi)
	{
		ModelRenderer_Draw(m_Asset, mi, world, cameraPosition);
	}
/*
#if defined(DEBUG) || defined(_DEBUG)
	Collision_DebugDraw(m_WorldAABB, { 0.0f, 0.0f, 1.0f, 1.0f });
#endif
*/
	if (DebugDraw_Allow(DebugDrawCategory::Collision))
	{
		Collision_DebugDraw(m_WorldAABB, { 0.0f, 0.0f, 1.0f, 1.0f });
	}

}

void Player::SetState(AnimState state)
{
	ChangeState(state);
}

// Movement
void Player::UpdateMovement(double elapsed_time, const XMFLOAT3& cameraFront)
{
	XMVECTOR velocity = XMLoadFloat3(&m_Velocity);

	// Camera forward -> XZ plane
	XMVECTOR front = XMLoadFloat3(&cameraFront);
	front = XMVectorSetY(front, 0.0f);

	if (XMVector3Equal(front, XMVectorZero()))
	{
		front = XMVectorSet(0, 0, 1, 0);
	}
	front = XMVector3Normalize(front);

	XMVECTOR up = XMVectorSet(0, 1, 0, 0);
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(up, front));

	float inputX = 0.0f;
	float inputZ = 0.0f;

	// Moving
	if (KeyLogger_IsPressed(KK_W))
	{
		inputZ += 1.0f;
	}
	if (KeyLogger_IsPressed(KK_S))
	{
		inputZ -= 1.0f;
	}
	if (KeyLogger_IsPressed(KK_A))
	{
		inputX -= 1.0f;
	}
	if (KeyLogger_IsPressed(KK_D))
	{
		inputX += 1.0f;
	}

	XMVECTOR moveDir = front * inputZ + right * inputX;

	//bool isMoving = false;
	m_IsMoving = false;

	if (!XMVector3Equal(moveDir, XMVectorZero()))
	{
		moveDir = XMVector3Normalize(moveDir);
		m_IsMoving = true;

		XMStoreFloat3(&m_Front, moveDir);

		XMVECTOR horizVel = moveDir * m_MoveSpeed;
		velocity = XMVectorSet(
			XMVectorGetX(horizVel),
			XMVectorGetY(velocity),
			XMVectorGetZ(horizVel),
			0.0f
		);
	}
	else
	{
		// Keep gravity
		velocity = XMVectorSet(0.0f, XMVectorGetY(velocity), 0.0f, 0.0f);
	}

	if (KeyLogger_IsTrigger(KK_SPACE) && IsOnGround())
	{
		velocity += XMVectorSet(0.0f, m_JumpVelocity, 0.0f, 0.0f);
		m_IsJump = true;
	}

	XMStoreFloat3(&m_Velocity, velocity);
}

// Gravity and collision simulation
void Player::UpdatePhysics(double elapsed_time)
{
	XMVECTOR position = XMLoadFloat3(&m_Position);
	XMVECTOR velocity = XMLoadFloat3(&m_Velocity);

	// 重力
	XMVECTOR gdir = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);
	velocity += gdir * m_Gravity * (float)elapsed_time;
	position += velocity * (float)elapsed_time;

	XMStoreFloat3(&m_Position, position);
	XMStoreFloat3(&m_Velocity, velocity);

	UpdateAABB();

	// 当たり判定
	bool grounded = false;
	CollisionSystem::ResolveAgainstStatic(*this, m_Position, 4, &grounded);

	UpdateAABB();

	bool prevGround = m_IsGround;
	m_IsGround = grounded || CheckGroundProbe(0.1f);

	if (m_IsGround)
	{
		if (m_Velocity.y < 0.0f) m_Velocity.y = 0.0f;
		m_IsJump = false;
	}

	CheckFallState(prevGround);
	ResetIfFallen(KILL_Y, { 0.0f, 0.0f, 0.0f });
}

void Player::UpdateAnimation(double elapsed_time)
{
	if (!m_AnimPlayer) return;

	m_AnimPlayer->Update(elapsed_time);
}

void Player::UpdateAABB()
{
	m_WorldAABB.min = {
		m_LocalAABB.min.x + m_Position.x,
		m_LocalAABB.min.y + m_Position.y,
		m_LocalAABB.min.z + m_Position.z
	};
	m_WorldAABB.max = {
		m_LocalAABB.max.x + m_Position.x,
		m_LocalAABB.max.y + m_Position.y,
		m_LocalAABB.max.z + m_Position.z 
	};
}

void Player::UpdateState()
{
	if (!m_IsGround)
	{
		// 上に向く
		if (m_Velocity.y > 0.1f)
		{
			ChangeState(AnimState::Jump);
			return;
		}
		// 下に降りる
		if (m_IsFall)
		{
			ChangeState(AnimState::Fall);
			return;
		}

		ChangeState(AnimState::Jump);
		return;
	}

	if (m_IsMoving)
	{
		ChangeState(AnimState::Walk);
	}
	else
	{
		ChangeState(AnimState::Idle);
	}
}

void Player::CheckFallState(bool wasGround)
{
	if (m_IsGround) // when tough the ground
	{
		m_FallDistance = 0.0f;
		m_IsFall = false;
		return;
	}

	if (wasGround)
	{
		m_FallStartY = m_Position.y;
		m_FallDistance = 0.0f;
	}

	if (m_Velocity.y <= m_FallSpeedThredhold)
	{
		m_FallDistance = std::max(0.0f, m_FallStartY - m_Position.y);

		if (!m_IsFall && m_FallDistance >= m_FallAnimThredhold)
		{
			m_IsFall = true;
		}
	}
}

void Player::ChangeState(AnimState newState)
{
	if (!m_AnimPlayer || !m_Asset) return;
	if (m_State == newState) return;

	m_State = newState;

	switch (m_State)
	{
	case AnimState::Idle:
		if (m_ClipIdle)
		{
			m_AnimPlayer->Play(m_ClipIdle, m_Asset, true, 0.0);
		}
		break;

	case AnimState::Walk:
		if (m_ClipWalk)
		{
			m_AnimPlayer->Play(m_ClipWalk, m_Asset, true, 0.0);
		}
		break;

	case AnimState::Run:
		if (m_ClipRun)
		{
			m_AnimPlayer->Play(m_ClipRun, m_Asset, true, 0.0);
		}
		break;

	case AnimState::Jump:
		if (m_ClipJump)
		{
			m_AnimPlayer->Play(m_ClipJump, m_Asset, false, 0.0);
		}
		else if (m_ClipWalk)
		{
			m_AnimPlayer->Play(m_ClipWalk, m_Asset, true, 0.0);
		}
		break;

	case AnimState::Fall:
		if (m_ClipFall)
		{
			m_AnimPlayer->Play(m_ClipFall, m_Asset, true, 0.0);
		}
		break;

	default:
		break;
	}
}

// detect player foot and the ground
bool Player::CheckGroundProbe(float probe) const
{
	AABB probeBox = m_WorldAABB;
	probeBox.max.y = probeBox.min.y + probe;
	probeBox.min.y -= probe;

	for (const AABB& s : CollisionSystem::AllStatics())
	{
		if (Collision_IsOverlapAABB(probeBox, s))
			return true;
	}

	return false;
}

void Player::ResetIfFallen(float killY, const DirectX::XMFLOAT3& respawnPos)
{
	if (m_Position.y < killY)
	{
		m_Position = respawnPos;
		m_Velocity = { 0.0f, 0.0f, 0.0f };
		m_IsJump = false;
		m_IsGround = false;

		UpdateAABB();
	}
}

bool Player::IsOnGround() const
{
	return m_IsGround;
}
