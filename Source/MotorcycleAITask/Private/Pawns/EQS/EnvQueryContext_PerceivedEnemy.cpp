#include "Pawns/EQS/EnvQueryContext_PerceivedEnemy.h"

#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "Pawns/AIControllers/NormalSoldierAIController.h"

void UEnvQueryContext_PerceivedEnemy::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	APawn* QueryOwner = Cast<APawn>(QueryInstance.Owner.Get());
	if (!QueryOwner)
	{
		return;
	}

	ANormalSoldierAIController* AIC = Cast<ANormalSoldierAIController>(QueryOwner->GetController());
	if (!AIC)
	{
		return;
	}

	AActor* Target = AIC->GetCurrentTarget();
	if (Target)
	{
		UEnvQueryItemType_Actor::SetContextHelper(ContextData, Target);
	}
}
