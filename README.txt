# MotorcycleAITask

Unreal Engine 5.3 ile geliştirilmiş, multiplayer destekli bir baskın simülasyonu projesidir. İki karşıt taraf (Friendly ve Enemy) karargahlarından asker üreterek birbirleriyle savaşır. Friendly taraf motosikletli birliklere, Enemy taraf ise piyade askerlerine sahiptir.

## Genel Mimari

Proje tamamen C++ üzerinde yazılmıştır. Blueprint'ler yalnızca mesh/animasyon atamaları ve DataTable konfigürasyonları için kullanılır.

### Taraf Sistemi
- **Team 0 (Friendly):** Motosiklet, sürücü ve atıcı üçlüsünden oluşan motorize birlikler
- **Team 1 (Enemy):** Piyade askerler (NormalSoldier)

### Pool Sistemi
Tüm pawn'lar önceden spawn edilir ve bir havuz (pool) içinde yönetilir. Performans açısından runtime'da yeni actor oluşturulmaz, var olanlar aktif/pasif yapılır.

```
EPawnPoolState: None → InPool → Alive → Dead → InPool (döngü)
```

- `PushInThePool()` — Pawn'u havuza gönderir, tüm bileşenlerini deaktif eder
- `PullFromThePool()` — Pawn'u havuzdan çıkarır, bileşenlerini aktif eder, sağlığını sıfırlar

### Replikasyon
Standart Unreal property replication yerine, hareket verileri için özel bir sıkıştırılmış replikasyon sistemi kullanılır:

1. Server her frame'de `SoldiersOnAliveIndexes` üzerinde batch halinde dolaşır
2. Pozisyon/rotasyon verileri minimal struct'lara (`FMotorcycleAIReplicationData`, `FNormalSoldierAIReplicationData`) paketlenir
3. Zlib ile sıkıştırılıp `NetMulticast Unreliable` RPC ile client'lara gönderilir
4. Client tarafında decompress edilip Lerp ile uygulanır

Durum bilgileri (PoolState, Health, MotorcycleState vb.) standart `DOREPLIFETIME` ile replicate edilir.

---

## Sınıf Yapısı

### Temel Sınıflar

| Sınıf | Dosya | Açıklama |
|-------|-------|----------|
| `ARaidSimulationBasePawn` | `Pawns/RaidSimulationBasePawn` | Tüm AI pawn'larının base class'ı. Pool yönetimi, hasar sistemi, sağlık, ölüm ve replikasyon temelini sağlar. `UAIPerceptionStimuliSourceComponent` ile AI algılama kaynağıdır. |
| `AHeadquarters` | `Headquarters/Headquarters` | Karargah aktörü. Pawn pool'unu oluşturur, `ContinuousSpawnTick` ile düzenli spawn yapar. Özel Zlib sıkıştırmalı replikasyon sistemini yönetir. DataTable'dan konfigürasyon okur. |
| `ARaidSimulationGameState` | `GameStates/RaidSimulationGameState` | `HeadquartersDatas` DataTable referansını tutar. |
| `ARaidSimulationGameMode` | `GameModes/RaidSimulationGameMode` | Oyun modu. |

### Motosiklet Sistemi

| Sınıf | Dosya | Açıklama |
|-------|-------|----------|
| `AMotorcyclePawn` | `Motorcycle/MotorcyclePawn` | Motosiklet pawn'u. `ISupplyInterface` implement eder. Sürücü ve atıcıyı taşır. Ekip üyesi ölümünde farklı davranışlar sergiler (durma, karargaha dönüş, ikmal). Perception stimuli deaktiftir — düşmanlar motosikleti değil, üzerindeki atıcıyı hedef alır. |
| `AMotorcycleAIController` | `Motorcycle/MotorcycleAIController` | Motosikletin AI controller'ı. Team 0. `SoldierCrowdFollowingComponent` kullanır. |
| `UMotorcycleMovementComponent` | `Motorcycle/Components/MotorcycleMovementComponent` | Motosiklet hareket bileşeni. İki mod destekler: `FollowActor` ve `FollowSpline`. Async trace ile zemin algılama, hız hesaplama, mesh eğim rotasyonu ve sahip rotasyonu yönetir. Client tarafında Lerp ile smooth hareket sağlar. |
| `ASplinePathActor` | `Motorcycle/SplinePathActor` | Level'e yerleştirilen USplineComponent sarmalayıcı. Motosikletler bu spline üzerinde ileri/geri hareket eder. İleri yönde spline sonu döngü yapar, geri yönde `OnSplineEndReached` broadcast eder. |
| `ISupplyInterface` | `Motorcycle/Interfaces/SupplyInterface` | `SupplySoldier()` fonksiyonunu tanımlayan interface. MotorcyclePawn implement eder. |

### Pawn Sınıfları

| Sınıf | Dosya | Açıklama |
|-------|-------|----------|
| `AMotorcycleShooterPawn` | `Pawns/MotorcycleShooterPawn` | Motosikletteki atıcı. Timer tabanlı ateş sistemi (`FireRate`, `FireDamage`, `FireRange`, `FireSpreadAngle`). `LineTraceSingleByChannel(ECC_Visibility)` ile hasar verir. Capsule: QueryOnly, Visibility'ye Block. Ölümünde motosiklete bildirir. |
| `AMotorcycleDriverPawn` | `Pawns/MotorcycleDriverPawn` | Motosiklet sürücüsü. Attach edildiğinde motosiklet hareketini aktifleştirir. Ölümünde motosiklete bildirir. Perception stimuli deaktiftir. |
| `ANormalSoldierPawn` | `Pawns/NormalSoldierPawn` | Piyade asker. Timer tabanlı ateş sistemi. `SkeletalMeshComponent` ile ragdoll ölüm animasyonu. Capsule: QueryAndPhysics, tüm kanallara Block. |

### AI Controller'lar

| Sınıf | Dosya | Açıklama |
|-------|-------|----------|
| `AMotorcycleShooterAIController` | `Pawns/AIControllers/MotorcycleShooterAIController` | Team 0. `UAIPerceptionComponent` + `UAISenseConfig_Sight` (360 derece görüş). En yakın düşmanı hedefler. `SetIsFiring(true/false)` ile ateş kontrolü. StateTree event'leri: Idle, Fire. |
| `ANormalSoldierAIController` | `Pawns/AIControllers/NormalSoldierAIController` | Team 1. `UAIPerceptionComponent` + `UAISenseConfig_Sight` (90 derece görüş). En yakın düşmanı hedefler. Düşman çok yakınsa geri çekilme davranışı (`RetreatDistance`). Devriye modu (`PatrolNavigate` — NavMesh üzerinde rastgele noktalara hareket). `SetIsFiring(true/false)` ile ateş kontrolü. StateTree event'leri: Patrol, FireAndChase. |
| `USoldierCrowdFollowingComponent` | `Pawns/AIControllers/SoldierCrowdFollowingComponent` | Crowd avoidance ayarları. |

### Hareket Bileşenleri

| Sınıf | Dosya | Açıklama |
|-------|-------|----------|
| `UMotorcycleMovementComponent` | `Motorcycle/Components/MotorcycleMovementComponent` | Spline/actor takip, hız hesaplama, zemin trace, mesh eğim rotasyonu. Server'da hesaplanır, client'a özel replikasyon ile gönderilir. |
| `USoldierMovementComponent` | `Pawns/Components/SoldierMovementComponent` | Piyade hareket bileşeni. Velocity bazlı rotasyon, yerçekimi, client tarafı Lerp interpolasyonu. |

### Animasyon

| Sınıf | Dosya | Açıklama |
|-------|-------|----------|
| `UMotorcycleShooterAnimInstance` | `Pawns/Components/MotorcycleShooterAnimInstance` | Atıcı animasyon instance'ı. `bIsFiring` ve `FireTarget` replike değerlerini okuyarak ateş animasyonu ve kafa dönüşü sağlar. |
| `UNormalSoldierAnimInstance` | `Pawns/Components/NormalSoldierAnimInstance` | Piyade animasyon instance'ı. |

### StateTree Task'ları

| Sınıf | Dosya | Açıklama |
|-------|-------|----------|
| `FSTTNormalSoldierPatrol` | `Pawns/StateTreeTasks/STT_NormalSoldierPatrol` | Devriye davranışı |
| `FSTTNormalSoldierFireAndChase` | `Pawns/StateTreeTasks/STTNormalSoldierFireAndChase` | Ateş et ve takip et davranışı |
| `FSTTMotorcycleShooterFire` | `Pawns/StateTreeTasks/STTMotorcycleShooterFire` | Atıcı ateş davranışı |
| `FSTTMotorcycleShooterIdle` | `Pawns/StateTreeTasks/STTMotorcycleShooterIdle` | Atıcı bekleme davranışı |

### Karargah Plugin Sistemi

| Sınıf | Dosya | Açıklama |
|-------|-------|----------|
| `UHeadquartersPluginBase` | `Headquarters/Plugins/HeadquartersPluginBase` | Karargah genişletme base component'i. DataAsset ile konfigüre edilir. |
| `USupplyAreaComponent` | `Headquarters/Plugins/SupplyAreaComponent` | İkmal alanı. Motosikletler bu alana girdiğinde ikmal tetiklenir (Overlap). |
| `USupplyAreaDA` | `Headquarters/Plugins/DataAssets/SupplyAreaDA` | İkmal alanı konfigürasyon DataAsset'i. |

### Veri Yapıları

| Struct | Açıklama |
|--------|----------|
| `FHeadquartersData` | DataTable satırı. Karargah mesh'i, max asker sayısı, spawn hızı, havuz verileri, plugin listesi. |
| `FSoldierPoolData` | Asker tipi, Blueprint sınıfı ve havuz sayısı. |
| `FHeadquartersPluginData` | Plugin sınıfı, DataAsset ve spawn transform'u. |
| `FMotorcycleAIReplicationData` | Motosiklet replikasyon paketi (10 byte). AI_ID, pozisyon (int16), rotasyonlar (uint8). |
| `FNormalSoldierAIReplicationData` | Piyade replikasyon paketi (9 byte). AI_ID, pozisyon (int16), velocity (int8), rotasyon (uint8). |

### Enum'lar

| Enum | Değerler | Açıklama |
|------|----------|----------|
| `EPartyType` | Friendly, Enemy | Taraf tipi |
| `ESoldierType` | Motorcycle, MotorcycleDriver, MotorcycleShooter, NormalShooter | Asker tipi |
| `EPawnPoolState` | None, InPool, Alive, Dead | Pawn havuz durumu |
| `EMotorcycleState` | Idle, Combat, ReturningToHQ, Resupplying, Stopped | Motosiklet durumu |
| `EMotorcycleFollowMode` | FollowActor, FollowSpline | Hareket takip modu |

---

## Akış Diyagramları

### Spawn ve Yaşam Döngüsü
```
HQ::BeginPlay
  └─ DataTable'dan konfigürasyon oku
  └─ Pool boyutu kadar pawn spawn et
  └─ Her pawn 2s sonra PushInThePool → InPool
  └─ 3s sonra PullFromPoolTryDelay → ilk spawn
  └─ ContinuousSpawnTick (her SpawnInterval saniye)
       ├─ Dead pawnları zorla InPool'a gönder (failsafe)
       ├─ SoldiersOnAliveIndexes temizle
       ├─ MaxAliveCount kontrolü
       ├─ SpawnActorAccordingToSoldierType(Motorcycle)
       └─ SpawnActorAccordingToSoldierType(NormalShooter)
```

### Motosiklet Ekip Yaşam Döngüsü
```
Motorcycle PullFromThePool
  ├─ Sürücü havuzdan çek → DriverSeatPoint'e attach et
  ├─ Atıcı havuzdan çek → GunnerSeatPoint'e attach et
  ├─ Spline takibini başlat (ileri yön, döngüsel)
  └─ MotorcycleState = Combat

Atıcı ölürse:
  ├─ MotorcycleState = ReturningToHQ
  ├─ Spline geri yön takibi başlat
  ├─ Spline başına ulaşınca SupplySoldier()
  │   ├─ Yeni atıcı havuzdan çek, attach et
  │   └─ Spline ileri yön, MotorcycleState = Combat
  └─ (Döngü devam eder)

Sürücü ölürse:
  ├─ MotorcycleState = Stopped
  └─ Hareket deaktif

Motosiklet ölürse:
  ├─ Sürücü ve atıcıya 9999 hasar
  └─ Tümü sırayla havuza döner
```

### Ateş Sistemi
```
AI Perception → düşman algıla → SetFireTarget + SetIsFiring(true)
  └─ Timer (FireRate aralıkla) → PerformFire()
       ├─ WeaponMesh'ten hedefe yön hesapla
       ├─ Spread açısı uygula
       ├─ LineTraceSingleByChannel(ECC_Visibility)
       ├─ Hit → DamageControl(FireDamage)
       ├─ MulticastRPC_OnFireVisual (client efektleri)
       └─ MulticastRPC_OnHitVisual (client efektleri)

Hedef kaybedilince → SetIsFiring(false) → Timer temizle
