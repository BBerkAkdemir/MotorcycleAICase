# MotorcycleAITask — Güncelleme Notu

Tarih: 18–19 Şubat 2026

## 1. Çoklu Spline Saldırı Sistemi (Multi-Spline Attack Schema)

Motosiklet, Headquarters üzerinden yönetilen spline'lar aracılığıyla 3 fazlı bir saldırı rotası izler:

| Faz | Spline | Davranış |
|Approaching| ApproachSpline | Karargah → savaş alanı (tek yön, ileri) |
|Combat| CombatSpline | Savaş alanında sürekli döngü (bLoop=true) |
|Escaping| EscapeSplines[] | Rastgele seçilen geri dönüş rotası |

Spline Yönetimi
- Tüm spline referansları `AHeadquarters` üzerinde merkezi olarak tutulur (EditInstanceOnly)
- `ApproachSpline`: Karargahtan savaş alanına tek yönlü rota
- `CombatSpline`: Savaş alanında döngüsel rota (bLoop=true)
- `EscapeSplines[]`: Birden fazla kaçış rotası dizisi, `GetRandomEscapeSpline()` ile rastgele seçilir
- `MotorcycleMovementComponent::SetSplinePath(Spline, bForward, bLoop)` — yön ve döngü kontrolü

Faz Geçişleri
Spline sonuna ulaşıldığında `OnSplineEndReached` delegate'i tetiklenir ve state machine'e göre geçiş yapılır:
- Approaching → Combat: Approach spline bitti → CombatSpline'a geç (loop)
- Escaping → SupplySoldier(): Kaçış spline'ı bitti → ikmal noktasına ulaşıldı
- Döngü modunda (Combat): spline sonu → başa dön, delegate tetiklenmez

State Machine
`EMotorcycleState`: Idle → Approaching → Combat → Escaping → Resupplying → Approaching (döngü)

| Tetikleyici | Geçiş |
|---|---|
| Pool'dan çekilme | Idle → Approaching |
| Approach spline sonu | Approaching → Combat |
| Shooter ölümü/mermi bitimi (Approaching) | Approaching → Escaping (ApproachSpline geri) |
| Shooter ölümü/mermi bitimi (Combat) | Combat → Escaping (rastgele EscapeSpline) |
| Escape spline sonu | Escaping → SupplySoldier() → Approaching |
| Driver ölümü | → Stopped |
| Motor patlaması (HP=0) | → Dead (patlama efekti + crew ragdoll) |

---

## 2. İkmal Noktası Sistemi (Supply Area)

Mimari
İkmal noktası, Headquarters'a plugin olarak eklenen bir `USupplyAreaComponent` (USceneComponent) tarafından yönetilir:

- HeadquartersPluginBase: Headquarters'a eklenebilen plugin component'lerin base class'ı
- SupplyAreaComponent: StaticMeshComponent ile fiziksel alan oluşturur, overlap ile tetiklenir
- SupplyAreaDA (DataAsset): Alan mesh'i ve transform verileri

Çalışma Akışı
1. Headquarters `BeginPlay`'de plugin'leri `NewObject` ile dinamik oluşturur
2. `InitializeArea()` çağrılır → DataAsset'ten mesh ve transform yüklenir
3. `UStaticMeshComponent` runtime'da `NewObject<UStaticMeshComponent>` ile oluşturulur
4. Collision: `QueryOnly`, sadece `ECC_Pawn` ile overlap
5. Motosiklet overlap alanına girdiğinde `OnMeshBeginOverlap` → `ISupplyInterface::SupplySoldier()` çağrılır

SupplySoldier() İkmal Fonksiyonu
Sadece `Escaping` state'inde çalışır (guard koruması):
1. Ölü shooter'ı diriltir (`PullFromThePool` + GunnerSeat'e attach)
2. Tüm mürettebata `HealToMax()` uygular (Shooter + Driver + Motorcycle)
3. Shooter mermisini doldurur (`RefillAmmo()`)
4. State'i `Approaching` olarak günceller, yeni ApproachSpline atar

---

## 3. Mermi (Ammo) Sistemi

- `MotorcycleShooterPawn`: `MaxAmmo=90`, `CurrentAmmo` (Replicated)
- Her atışta mermi azalır; 0'a ulaşınca ateş durur
- Mermi bitince `OnShooterAmmoDepleted()` tetiklenir → motosiklet kaçış moduna geçer
- `RefillAmmo()` ikmal sırasında çağrılır
- Pool'dan çıkışta otomatik mermi dolar

---

4. Crew (Mürettebat) Yönetimi

- Crew, HQ pool'u üzerinden değil, doğrudan Motorcycle tarafından spawn edilir
- `DriverClass` ve `ShooterClass` (TSubclassOf) property'leri ile Blueprint'ten ayarlanabilir
- Crew üyeleri HQ Alive/Pool listelerine eklenmez

OnCrewMemberDied Mantığı
| Senaryo | Davranış |
|---|---|
| Sadece driver ölür | Motor durur (Stopped), shooter ateşe devam eder |
| Shooter da ölür (driver zaten ölü) | Pool dönüş timer'ı başlar |
| Sadece shooter ölür | Motor kaçış moduna geçer (Escaping) |
| Driver ölür ama shooter zaten ölü | Hemen pool dönüş timer'ı başlar |
| Motor patlar (HP=0) | Crew'a 10000 hasar → ragdoll + etrafa saçılma |

Motorcycle Patlaması
Motor öldüğünde (DamageControl → HP=0):
- Static mesh gizlenir (`SetVisibility(false)`)
- Patlama VFX (Niagara) spawn edilir
- Patlama sesi çalınır
- Crew üyelerine yüksek hasar verilir → ragdoll tetiklenir, yukarı+dışarı impulse ile saçılırlar
- `OnCrewMemberDied` guard: motorcycle `PoolState==Dead` ise skip (çift timer önleme)
- Pool'dan çekilince static mesh tekrar görünür olur

---

5. Pool Sistemi

- Parti bazlı spawn: Friendly HQ → sadece Motorcycle, Enemy HQ → sadece NormalShooter
- Stale alive list temizliği: ContinuousSpawnTick başında dead/invalid pawn'lar listeden çıkarılır
- Event-driven spawn döngüsü: MaxAlive'a ulaşınca timer durur, pool'a pawn dönünce tekrar başlar
- NavMesh projection: Spawn noktası NavMesh üzerine projelendiriliyor + Z offset
- GetPool() getter: Fire trace ignore listesi için pool erişimi

---

6. AI Perception ve Ateş Sistemi

Perception Stimuli Yönetimi
- `Internal_PushInThePool()`: stimuli unregister → pool'daki pawn'lar AI'ya görünmez
- `Internal_PullFromThePool()`: stimuli register → pool'dan çıkan pawn tekrar görünür
- Internal_OnDead(): stimuli unregister → ölü pawn'lar anında AI hedef listesinden düşer

Perception Konfigürasyonu
| Pawn | AI Tarafından Algılanır mı |
|---|---|
| MotorcyclePawn | Hayır (bAutoRegister=false) |
| MotorcycleDriverPawn | Evet |
| MotorcycleShooterPawn | Evet |
| NormalSoldierPawn | Evet |

NormalSoldier Ateş Güvenlik Kontrolleri
- `PerformFire()`: Hedef ölüyse (`PoolState != Alive`) → anında `SetIsFiring(false)` → ateş timer + ses durur
- `DamageControl` dönüş değeri kontrol edilir → hasar gerçekleşmediyse kan VFX gösterilmez
- Hibrit ateş sistemi: LineTrace + proximity fallback (75u eşik, %90 mesafe kontrolü)
- Friendly fire önleme: HQ pool'undaki tüm pawn'lar trace'den ignore edilir

NormalSoldier Hedefe Dönüş
- `SoldierMovementComponent::MakeRotate()`: Ateş ederken hareket yönüne değil, `FireTarget`'a doğru döner
- Server'da hesaplanır, custom replication ile client'lara senkronize edilir

---

7. EQS Tabanlı Geri Çekilme Sistemi

- NormalSoldierAIController'a `RetreatEQS` (UEnvQuery*) eklendi
- EQS sorgusu başarılıysa o noktaya git, değilse eski yönteme (sabit mesafe) düş
- `bIsRunningRetreatQuery` flag'i ile çift sorgu önlenir

---

8. Multiplayer Ses ve VFX Sistemi

Motor Sesi (MotorcyclePawn)
- MulticastRPC ile tüm client'larda çalışır

Patlama Efektleri (MotorcyclePawn)
- `ExplosionSound` + `ExplosionSoundRadius=5000` — ses efekti
- `ExplosionEffect` (UNiagaraSystem*) — görsel patlama particle'ı
- Her ikisi de `Internal_OnDead` içinde spawn → MulticastRPC_OnDead ile tüm client'lara ulaşır

Ateş Sesi (NormalSoldier + MotorcycleShooter)
- `FireAudioComponent` + `FireLoopSound` + `FireSoundRadius=2000`
- Server: `StartFiring()`/`StopFiring()` içinde ses başlatılır/durdurulur
- Client: `OnRep_IsFiring()` ile ses başlatma/durdurma

Niagara VFX
- `MulticastRPC_OnFireVisual`: Namlu alev efekti (MuzzleFlash) — Unreliable
- `MulticastRPC_OnHitVisual`: Vuruş kan efekti (HitBlood) — Unreliable, sadece hasar gerçekleştiğinde
- `GetMuzzleComponent()` virtual → alt sınıflar kendi WeaponMesh'lerini döndürür

---

9. Ragdoll / Ölüm Sistemi

- Ragdoll collision: `WorldStatic=Block`, `WorldDynamic=Ignore` (sadece statik geometri ile çarpışma)
- SkeletalMesh detach sonrası re-attach: `SetSimulatePhysics(false)` → `AttachToComponent` → relative transform reset
- Motor patlamasında crew ragdoll: yukarı + dışarı yönde impulse uygulanır

---

11. Düzeltilen Kritik Hatalar

| Hata | Çözüm |
|---|---|
| Paketlenmiş oyunda GC crash (SupplyAreaComponent) | `CreateDefaultSubobject` yerine runtime `NewObject` + `RF_Transient` |
| Ölü pawn'lara ateş devam ediyor | `Internal_OnDead`'de perception stimuli unregister + `PerformFire` hedef ölü kontrolü |
| Ölü pawn'larda kan VFX çıkıyor | `DamageControl` dönüş değeri kontrol ediliyor, false ise VFX yok |
| NormalSoldier ateş ederken dönmüyor | `MakeRotate`'e FireTarget'a dönüş eklendi |
| Driver ölünce motor hemen pool'a dönüyor | Pool timer kaldırıldı; sadece her iki crew de ölünce pool'a döner |
| Pool'daki pawn'lar hedef alınıyor | Perception stimuli unregister/register |
| Ateş sesi sadece server'da | `OnRep_IsFiring` ile client'larda da ses |
| Yeraltı spawn | NavMesh projection + Z offset |
| Ragdoll sonrası mesh detach | `SetSimulatePhysics(false)` → `AttachToComponent` |
| Friendly fire | HQ pool'undaki pawn'lar trace'den ignore |
| AreaMesh DOREPLIFETIME | Component pointer replication kaldırıldı |

---

12. Replikasyon Mimarisi

- `bIsFiring`, `FireTarget`: `ReplicatedUsing` + `OnRep_` callback (push-based)
- `PoolState`: `ReplicatedUsing` + `OnRep_PoolState` (late joiner sync)
- `CurrentAmmo`: `DOREPLIFETIME`
- Hareket: Zlib-compressed Multicast RPC
- VFX: Unreliable Multicast RPC
- Ses: Reliable Multicast RPC (OnDead) + OnRep (ateş sesi)


# MotoBikeAI - Unreal Engine 5 Motorcycle AI System

Tarih: 16 Şubat 2026

## Overview
C++ based multiplayer-compatible Motorcycle AI system with enemy AI targets for Unreal Engine 5.3.2.

A motorcycle carrying a Driver AI and a Gunner AI follows a spline path to infiltrate enemy lines. The gunner detects and engages enemies automatically. Enemy soldiers patrol, detect the motorcycle, and fire back using Behavior Trees and AI Perception.

## Requirements
- **Unreal Engine 5.3.2**
- **Visual Studio 2022** (or Rider)

## Setup Instructions

### 1. Open Project
1. Open Unreal Engine 5.3.2
2. Browse to `MotoBikeAI.uproject` and open it
3. Let the engine compile C++ code (first time may take a few minutes)

### 2. Create Main Level
1. **File > New Level > Open World** (or Basic)
2. Save as `Content/Maps/MainLevel`
3. Add a **Nav Mesh Bounds Volume** covering the play area (required for enemy patrol AI)

### 3. Create Behavior Tree for Enemy AI
1. **Content Browser > Right-click > Artificial Intelligence > Behavior Tree** - name it `BT_EnemySoldier`
2. **Content Browser > Right-click > Artificial Intelligence > Blackboard** - name it `BB_EnemySoldier`
3. Open `BB_EnemySoldier` and add these keys:
   - `TargetActor` (Object, base class: Actor)
   - `PatrolLocation` (Vector)
   - `HasTarget` (Bool)
   - `TargetDistance` (Float)
4. Open `BT_EnemySoldier`, set its Blackboard to `BB_EnemySoldier`
5. Build the tree:
```
Root
└── Selector
    ├── Sequence [Attack]
    │   ├── Decorator: BTDecorator_HasTarget (HasTargetKey = HasTarget)
    │   ├── Service: BTService_DetectEnemy (on Sequence node)
    │   └── BTTask_FireAtTarget (TargetActorKey = TargetActor)
    └── Sequence [Patrol]
        ├── Service: BTService_DetectEnemy (on Sequence node)
        ├── BTTask_FindPatrolLocation (PatrolLocationKey = PatrolLocation)
        └── MoveTo (BlackboardKey = PatrolLocation)
```

### 4. Create Blueprints
Create Blueprint classes for easy configuration:

#### BP_Motorcycle (Parent: MotorcyclePawn)
- Set `DriverClass` to your driver soldier Blueprint
- Set `GunnerClass` to your gunner soldier Blueprint
- Set `GunnerWeaponClass` to your weapon Blueprint
- Set `SplineMoveSpeed` (default: 800)

#### BP_DriverSoldier (Parent: BaseAISoldier)
- Assign a skeletal mesh (Mixamo character recommended)
- Set Health values

#### BP_GunnerSoldier (Parent: GunnerSoldier)
- Assign a skeletal mesh
- Set `DetectionRange` (default: 3000)
- Set `FireRange` (default: 2500)

#### BP_EnemySoldier (Parent: EnemySoldier)
- Assign a skeletal mesh and animation
- Set `WeaponClass`
- Set `AttackRange` (default: 2000)
- Set `SightRange` (default: 3000)
- Set AI Controller Class to `EnemyAIController`

#### BP_Weapon (Parent: WeaponBase)
- Set weapon mesh
- Set `FireRate`, `Damage`, `Range`
- Set muzzle flash and sound effects

#### BP_SplinePath (Parent: MotorcycleSplinePath)
- Place in level and edit spline points to define motorcycle route

### 5. Configure Game Mode
1. Open **Project Settings > Maps & Modes**
2. Set Default GameMode to your `BP_GameMode` (parent: MotoBikeGameMode)
3. In BP_GameMode, set:
   - `MotorcycleClass` = BP_Motorcycle
   - `EnemySoldierClass` = BP_EnemySoldier
   - `InitialEnemyCount` = 5
   - Enemy spawn locations

### 6. Import Assets
Recommended free assets:
- **Motorcycle Model**: Download from Unreal FAB
- **Character Animations**: Download from Mixamo (idle, rifle aim, firing, death)
- Set up Animation Blueprints with appropriate states

### 7. Play
Press **Play** - you'll spawn as a Spectator Pawn and can watch the AI in action.
- The motorcycle follows the spline path
- The gunner detects and fires at enemies
- Enemies patrol and engage the motorcycle
- **F** key toggles follow camera mode

## Architecture

### Core Systems

| Class | Description |
|-------|-------------|
| `AMotorcyclePawn` | Main vehicle, follows spline, manages crew |
| `UMotorcycleMovementComponent` | Custom movement with throttle/steering |
| `ABaseAISoldier` | Base soldier character with health |
| `AGunnerSoldier` | Gunner with target detection and firing |
| `AEnemySoldier` | Enemy with weapon and attack capability |
| `AWeaponBase` | Line-trace weapon with effects |
| `UHealthComponent` | Replicated health/damage system |
| `AMotorcycleSplinePath` | Spline path for motorcycle route |

### AI System

| Class | Description |
|-------|-------------|
| `AMotorcycleAIController` | Controls motorcycle movement |
| `AEnemyAIController` | Enemy AI with BT + Perception |
| `UBTTask_FindPatrolLocation` | Random patrol point via NavMesh |
| `UBTTask_FireAtTarget` | Engage target for duration |
| `UBTTask_StopAttack` | Cease fire |
| `UBTService_DetectEnemy` | Periodic enemy scanning |
| `UBTDecorator_HasTarget` | Condition check for target |
| `UEnvQueryContext_Motorcycle` | EQS context for motorcycle (Bonus) |

### Framework

| Class | Description |
|-------|-------------|
| `AMotoBikeGameMode` | Spawns motorcycle, enemies, wave system |
| `AMotoBikeSpectatorPawn` | Free camera + follow mode |
| `AMotoBikeHUD` | Canvas-based AI status display |
| `UAIStatusWidget` | UMG widget for AI status (Bonus) |
| `AEnemySpawnManager` | Configurable spawn point system (Bonus) |

### Multiplayer Replication
- All movement, health, and weapon systems are replicated
- Server-authoritative: damage and AI logic run on server
- NetMulticast: visual/audio effects broadcast to all clients
- `UPROPERTY(Replicated)` on critical state variables
- `UFUNCTION(Server, Reliable)` for client-to-server actions

### Key Design Decisions
1. **Custom Movement over Chaos Vehicles** - More control for spline-following AI
2. **Line-trace Weapons** - Simpler and more performant than projectile-based
3. **Component-based Health** - Reusable across all characters and vehicles
4. **Separation of Concerns** - Each AI has its own controller and behavior

## Bonus Features Implemented
- **EQS Context** (`UEnvQueryContext_Motorcycle`) - For enemy AI firing position selection
- **AI Status HUD** (`AMotoBikeHUD`) - Real-time driver/gunner/enemy status display
- **Enemy Spawn System** (`AEnemySpawnManager`) - Configurable spawn points with wave spawning
- **Wave Spawning** in GameMode - Periodic enemy reinforcements