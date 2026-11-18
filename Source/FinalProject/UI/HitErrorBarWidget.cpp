#include "HitErrorBarWidget.h"

void UHitErrorBarWidget::CalculateHitData(float OffsetMs, float ToleranceMaxMs, float& OutPosX, FLinearColor& OutColor) const
{
	const float SafeTolerance = FMath::Max(ToleranceMaxMs, 1.0f);
	const float AbsOffset = FMath::Abs(OffsetMs);

	//position X sur la barre
	const float NormalizedRatio = FMath::Clamp(OffsetMs / SafeTolerance, -1.0f, 1.0f);
	OutPosX = NormalizedRatio * HalfBarWidth;

	//note ratée = Rouge tout au bout
	if (AbsOffset > SafeTolerance)
	{
		OutColor = MissColor;
		return;
	}

	//note réussie = couleur
	const float AbsRatio = FMath::Abs(NormalizedRatio);
	if (AbsRatio <= PerfectRatio)
	{
		OutColor = PerfectColor;
	}
	else if (AbsRatio <= GoodRatio)
	{
		OutColor = GoodColor;
	}
	else
	{
		OutColor = BadColor;
	}
}