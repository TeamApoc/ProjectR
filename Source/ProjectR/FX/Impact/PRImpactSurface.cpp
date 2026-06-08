// Copyright ProjectR. All Rights Reserved.

#include "PRImpactSurface.h"

FPRImpactResult IPRImpactSurface::ResolveImpactSurface_Implementation(const FPRImpactContext& Context) const
{
	// 구현하지 않은 대상은 Physical Surface fallback으로 넘어가도록 빈 결과 반환
	return FPRImpactResult();
}
