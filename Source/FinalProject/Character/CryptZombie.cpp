// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/CryptZombie.h"

ACryptZombie::ACryptZombie()
{
	// Valeurs par défaut spécifiques au zombie si aucun DataAsset n'est assigné
	MaxHealth = 100.0f;
	AttackDamage = 15.0f;
	AttackRange = 150.0f;
	AttackCooldown = 1.5f;
	AttackHitDelay = 1.0f;
}
