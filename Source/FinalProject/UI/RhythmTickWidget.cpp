#include "RhythmTickWidget.h"
#include "Components/Image.h"

void URhythmTickWidget::SetTickColor(FLinearColor NouvelleCouleur)
{
	if (TickImage)
	{
		TickImage->SetColorAndOpacity(NouvelleCouleur);
	}
}

void URhythmTickWidget::SetTickOpacity(float Opacite)
{
	SetRenderOpacity(Opacite);
}
