/*==============================================================================

   AABB api [aabb_provider.h]
														 Author : Gu Anyi
														 Date   : 2026/01/11
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef AABB_PROVIDER_H
#define AABB_PROVIDER_H

struct AABB;

struct IAABBProvider
{
	virtual const AABB& GetAABB() const = 0;

protected:

	~IAABBProvider() = default;
};

#endif // AABB_PROVIDER_H
