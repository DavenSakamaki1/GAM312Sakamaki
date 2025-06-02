// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerChar.h"

// Sets default values
APlayerChar::APlayerChar()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Initial creation of camera component and naming
	PlayerCamComp = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Cam"));

	// Attaching camera component to character mesh
	PlayerCamComp->SetupAttachment(GetMesh(), "head");

	// Rotate camera with the PawnControlRotation
	PlayerCamComp->bUsePawnControlRotation = true;

	// Setting size of BuildingArray to 3
	BuildingArray.SetNum(3);

	// Setting size of ResourcesArray to 3
	ResourcesArray.SetNum(3);
	// Adding the Names of each resource to ResourcesNameArray
	ResourcesNameArray.Add(TEXT("Wood"));
	ResourcesNameArray.Add(TEXT("Stone"));
	ResourcesNameArray.Add(TEXT("Berry"));
}

// Called when the game starts or when spawned
void APlayerChar::BeginPlay()
{
	Super::BeginPlay();

	FTimerHandle StatsTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(StatsTimerHandle, this, &APlayerChar::DecreaseStats, 2.0f, true);
	
}

// Called every frame
void APlayerChar::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Updating player UI 
	playerUI->UpdateBars(Health, Hunger, Stamina);

	if (isBuilding)
	{
		if (spawnedPart)
		{
			// Get the location of the player camera
			FVector StartLocation = PlayerCamComp->GetComponentLocation();
			// Get the vector in the directin the player is facing up to 400.0 units in length
			FVector Direction = PlayerCamComp->GetForwardVector() * 400.0f;
			// Get the end location from where the player camera is and the direction vector
			FVector EndLocation = StartLocation + Direction;
			spawnedPart->SetActorLocation(EndLocation);
		}
	}
}

// Called to bind functionality to input
void APlayerChar::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	// Binding axis mappings to the function
	// Ex. MoveForward AxisMapping in project settings bound to MoveForward Function
	PlayerInputComponent->BindAxis("MoveForward", this, &APlayerChar::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &APlayerChar::MoveRight);
	PlayerInputComponent->BindAxis("LookUp", this, &APlayerChar::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Turn", this, &APlayerChar::AddControllerYawInput);
	// Binding action mappings to the function
	// Ex. Jump ActionMapping in project settings bound to StartJump and StopJump
	// This allows both functions to see if the jump button is either being pressed or not
	PlayerInputComponent->BindAction("JumpEvent", IE_Pressed, this, &APlayerChar::StartJump);
	PlayerInputComponent->BindAction("JumpEvent", IE_Released, this, &APlayerChar::StopJump);
	PlayerInputComponent->BindAction("InteractEvent", IE_Pressed, this, &APlayerChar::FindObject);
	PlayerInputComponent->BindAction("RotPart", IE_Pressed, this, &APlayerChar::RotateBuilding);
}

void APlayerChar::MoveForward(float axisValue)
{
	// Getting the char rotation and correct vector direction for char forward and back
	FVector Direction = FRotationMatrix(Controller->GetControlRotation()).GetScaledAxis(EAxis::X);
	AddMovementInput(Direction, axisValue);
}

void APlayerChar::MoveRight(float axisValue)
{
	// Getting the char rotation and correct vector direction for char left and right
	FVector Direction = FRotationMatrix(Controller->GetControlRotation()).GetScaledAxis(EAxis::Y);
	AddMovementInput(Direction, axisValue);
}

void APlayerChar::StartJump()
{
	// When Jump button is pressed
	bPressedJump = true;
}

void APlayerChar::StopJump()
{
	// When Jump button isn't pressed
	bPressedJump = false;
}

void APlayerChar::FindObject()
{
	FHitResult HitResult;
	// Get the location of the player camera
	FVector StartLocation = PlayerCamComp->GetComponentLocation();
	// Get the vector in the directin the player is facing up to 800.0 units in length
	FVector Direction = PlayerCamComp->GetForwardVector() * 800.0f;
	// Get the end location from where the player camera is and the direction vector
	FVector EndLocation = StartLocation + Direction;

	FCollisionQueryParams QueryParams;
	// Ignore itself when checking collision
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = true;
	QueryParams.bReturnFaceIndex = true;

	if (!isBuilding)
	{
		// Line Trace in the Visiblity channel
		if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, QueryParams))
		{
			// Casting the hit actor to AResource_M
			AResource_M* HitResource = Cast<AResource_M>(HitResult.GetActor());
			if (Stamina > 5.0f)
			{ 
				// Check if HitResource is valid
				if (HitResource)
				{
					FString hitName = HitResource->resourceName;
					int resourceValue = HitResource->resourceAmount;

					HitResource->totalResource = HitResource->totalResource - resourceValue;

					// If the resource has resources remaining then call function give resources of the resourceValue
					if (HitResource->totalResource > resourceValue)
					{
						GiveResources(resourceValue, hitName);
						GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Resource Collected"));

						// Spawning the decal at the location of hit with size 10, 10, 10 facing the player with lifespan of 2 seconds
						UGameplayStatics::SpawnDecalAtLocation(GetWorld(), hitDecal, FVector(10.0f, 10.0f, 10.0f), HitResult.Location, FRotator(-90, 0, 0), 2.0f);

						SetStamina(-5.0f);
					}
					else
					{
						// If there isn't enough resources then destroy the resource
						HitResource->Destroy();
						check(GEngine != nullptr);
						GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Resource Depleted"));
					}
				}
			}
		}
	}
	else
	{
		isBuilding = false;
	}
}

void APlayerChar::SetHealth(float amount)
{
	// If health + amount < 100 set health to the result
	if (Health + amount < 100)
	{
		Health = Health + amount;
	}
	else {
		Health = 100;
	}
}

void APlayerChar::SetHunger(float amount)
{
	// If hunger + amount < 100 set hunger to the result
	if (Hunger + amount < 100)
	{
		Hunger = Hunger + amount;
	}
	else
	{
		Hunger = 100;
	}
}

void APlayerChar::SetStamina(float amount)
{
	// If stamina + amount < 100 set stamina to the result
	if (Stamina + amount <= 100)
	{
		Stamina = Stamina + amount;
	}
	else {
		Stamina = 100;
	}
}

void APlayerChar::DecreaseStats()
{
	// If hunger is > 0 then subtract 1.0 from hunger
	if (Hunger > 0)
	{
		SetHunger(-1.0f);
	}

	// Increase Stamina by 10.0
	SetStamina(10.0f);

	// If hunger is <= 0 then subtract 3.0 from health
	if (Hunger <= 0)
	{
		SetHealth(-3.0f);
	}
}

void APlayerChar::GiveResources(float amount, FString resourceType)
{
	if (resourceType == "Wood")
	{
		ResourcesArray[0] = ResourcesArray[0] + amount;
	}

	if (resourceType == "Stone")
	{
		ResourcesArray[1] = ResourcesArray[1] + amount;
	}

	if (resourceType == "Berry")
	{
		ResourcesArray[2] = ResourcesArray[2] + amount;
	}
}

void APlayerChar::UpdateResources(float woodAmount, float stoneAmount, FString buildingObject)
{
	// Check if player has enough wood
	if (woodAmount <= ResourcesArray[0])
	{
		// Check if player has enough stone
		if (stoneAmount <= ResourcesArray[1])
		{
			// Removing each resource from the resource array
			ResourcesArray[0] = ResourcesArray[0] - woodAmount;
			ResourcesArray[1] = ResourcesArray[1] - stoneAmount;

			// If building wall add 1 to the count of built walls in building array
			if (buildingObject == "Wall")
			{
				BuildingArray[0] = BuildingArray[0] + 1;
			}

			// If building floor add 1 to the count of built floors in building array
			if (buildingObject == "Floor")
			{
				BuildingArray[1] = BuildingArray[1] + 1;
			}

			// If building ceiling add 1 to the count of built ceilings in building array
			if (buildingObject == "Ceiling")
			{
				BuildingArray[2] = BuildingArray[2] + 1;
			}
		}
	}
}

void APlayerChar::SpawnBuilding(int buildingID, bool& isSuccess)
{
	if (!isBuilding)
	{
		if (BuildingArray[buildingID] >= 1)
		{
			isBuilding = true;
			FActorSpawnParameters SpawnParams;
			// Get the location of the player camera
			FVector StartLocation = PlayerCamComp->GetComponentLocation();
			// Get the vector in the directin the player is facing up to 400.0 units in length
			FVector Direction = PlayerCamComp->GetForwardVector() * 400.0f;
			// Get the end location from where the player camera is and the direction vector
			FVector EndLocation = StartLocation + Direction;
			FRotator myRot(0, 0, 0);

			BuildingArray[buildingID] = BuildingArray[buildingID] - 1;

			spawnedPart = GetWorld()->SpawnActor<ABuildingPart>(BuildPartClass, EndLocation, myRot, SpawnParams);

			isSuccess = true;
		}

		isSuccess = false;
	}
}

void APlayerChar::RotateBuilding()
{
	// If isBuilding is true then rotate spawnedPart by 90 degrees
	if (isBuilding)
	{
		spawnedPart->AddActorWorldRotation(FRotator(0, 90, 0));
	}
}

