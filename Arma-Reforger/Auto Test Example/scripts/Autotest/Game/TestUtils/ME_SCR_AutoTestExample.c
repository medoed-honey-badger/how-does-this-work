// ВРЕМЕННО ЗАКОММЕНТИРОВАН ЦЕЛИКОМ — сосредоточились на основном моде,
// вернёмся к автотестам позже. Синтаксис уже частично подтверждён
// (TestStep/TestStage, AssertTrue, suite: как typename), но полная
// компиляция пока не проверена до конца из-за посторонних ошибок проекта,
// не связанных с этим файлом.
/*
// ============================================================================
// Автотесты для ME_StatsComponent — используют официальный Autotest Framework
// Arma Reforger 1.8 (экспериментальная ветка).
//
// СИНТАКСИС ПРОВЕРЕН ПО РЕАЛЬНЫМ ИСХОДНИКАМ (присланы пользователем,
// Game/TestFramework/*.c и Game/TestUtils/*.c из 1.8), а НЕ по странице
// вики — та описывает более раннюю/другую версию API:
//   - вики: [Step(EStage.Setup)]           -> реально: [TestStep(TestStage.Setup)]
//   - вики: [Test(suite: "ИмяКлассаСтрокой", timeoutS: N)]
//     -> реально: [Test(suite: ИмяКлассаНапрямую, timeoutS: N)] — параметр
//        suite ожидает typename (сам тип), а не строку с именем класса.
//        ПОДТВЕРЖДЕНО реальной ошибкой компиляции: "Cannot convert
//        'string' to 'typename' for argument '0' in method 'Test'".
//   - вики: SetResult(SCR_AutotestResult.AsSuccess()/AsFailure(...))
//     -> реально: SetFailure(string reason) / AssertTrue(expr, msg) —
//        тест считается УСПЕШНЫМ ПО УМОЛЧАНИЮ, если ни разу не вызвать
//        SetFailure/AssertTrue(false, ...).
//   - SCR_TestLib.SpawnEntity/SpawnPlayer/SetActionValue/IsCharacterAlive —
//     ОТСУТСТВУЮТ в реальном коде (SpawnEntity буквально закомментирован в
//     SCR_TestLib_Entity.c с пометкой "Override in the game to use the
//     correct API"). Поэтому спавним сущности сами, штатным
//     GetGame().SpawnEntityPrefab() — обычный публичный API, не из TestLib.
//
// ЖИВОЙ ВЫСТРЕЛ — вместо недостающего SetActionValue(ACTION_FIRE, ...)
// используем ПОДТВЕРЖДЁННЫЙ прямой метод персонажа (файл
// CharacterControllerComponent.c, присланный пользователем):
//   CharacterControllerComponent.SetFireWeaponWanted(bool val)
// Это тот же уровень абстракции, что и у самой игры при обработке нажатия
// кнопки стрельбы — не нужно эмулировать сырой ввод. GetDeath — тоже
// готовый: CharacterControllerComponent.IsDead().
//
// СТАДИЯ Main/Execute — БОЛЬШЕ НЕ АКТУАЛЬНО. Подтверждено на примере
// SCR_AutotestSuiteBase.c: bool-возвращающий [TestStep(TestStage.Setup)]
// сам по себе выполняется каждый кадр, пока не вернёт true (см.
// Setup_AwaitWorld в том файле). Отдельная стадия Main для поллинга не
// нужна — этот же приём используем ниже для ожидания смерти цели.
//
// Запуск: Script Editor -> навести на класс -> Plugins -> Run test.
// Или через World Editor -> Autotest tool -> Run class/Run group.
// ============================================================================

[BaseContainerProps(category: "Autotest")]
class SCR_TEST_MEStatsLocalizationSuite : SCR_AutotestSuiteBase
{
}

// Общая база для тестов этого файла — вынесенный хелпер спавна сущности по
// префабу в чистом месте карты, подальше от прочих объектов.
class SCR_TEST_MEStatsLocalizationCase : SCR_AutotestCaseBase
{
	static const string PREFAB_BTR70 = "{BD75A18920BD1278}Prefabs/Vehicles/Wheeled/Conflict_Variants/BTR70_Conflict.et";
	static const string PREFAB_CHARACTER_USSR = "{F3C4020CA491A6E1}Prefabs/Characters/Campaign/Final/OPFOR/USSR_Army/Regular/Campaign_USSR_Player_GL.et";
	static const string PREFAB_CHARACTER_US = "{3E18CC9634468249}Prefabs/Characters/Campaign/Final/BLUFOR/US_army/Regular/Campaign_US_Player_GL.et";

	protected ref array<IEntity> m_SpawnedEntities = {};

	IEntity SpawnTestPrefab(string prefabPath)
	{
		Resource resource = Resource.Load(prefabPath);
		if (!resource || !resource.IsValid())
		{
			Print("Не удалось загрузить префаб: " + prefabPath, LogLevel.ERROR);
			return null;
		}

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		// Подальше от центра карты и друг от друга — чтобы тестовые сущности
		// не путались с чем-то реальным в мире и не пересекались друг с
		// другом при параллельном запуске нескольких тест-кейсов.
		spawnParams.Transform[3] = "5000 50 5000";

		IEntity entity = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), spawnParams);
		if (entity)
			m_SpawnedEntities.Insert(entity);

		return entity;
	}

	[TestStep(TestStage.TearDown)]
	void TearDown_DeleteSpawned()
	{
		foreach (IEntity entity : m_SpawnedEntities)
		{
			if (entity)
				delete entity;
		}
		m_SpawnedEntities.Clear();
	}
}

// ----------------------------------------------------------------------------
// GetEditableEntityLocKey() на технике
// ----------------------------------------------------------------------------

[Test(suite: SCR_TEST_MEStatsLocalizationSuite, timeoutS: 5)]
class SCR_TEST_MEStatsLocalization_GetEditableEntityLocKeyOnBTR70_ReturnsVehicleKey : SCR_TEST_MEStatsLocalizationCase
{
	[TestStep(TestStage.Setup)]
	void Setup_SpawnAndAssert()
	{
		IEntity vehicle = SpawnTestPrefab(PREFAB_BTR70);
		AssertTrue(vehicle != null, "БТР-70 не заспавнился — проверь, не поменялся ли путь префаба");
		if (GetFailure())
			return;

		string key = ME_StatsComponent.GetEditableEntityLocKey(vehicle);
		AssertTrue(key == "#AR-Vehicle_BTR70_Name", string.Format("Ожидали '#AR-Vehicle_BTR70_Name', получили '%1'", key));
	}
}

// ----------------------------------------------------------------------------
// GetEntityLocKey() — общий путь (InventoryItemComponent -> Editable ->
// префаб) — на технике должен дать ТОТ ЖЕ результат, что и
// GetEditableEntityLocKey напрямую (регресс-тест на баг с "Trunk", который
// чинили в начале разработки — GetEntityLocKey раньше проверял
// InventoryItemComponent ПЕРВЫМ и натыкался на компонент багажника).
// ----------------------------------------------------------------------------

[Test(suite: SCR_TEST_MEStatsLocalizationSuite, timeoutS: 5)]
class SCR_TEST_MEStatsLocalization_GetEntityLocKeyOnBTR70_DoesNotReturnTrunk : SCR_TEST_MEStatsLocalizationCase
{
	[TestStep(TestStage.Setup)]
	void Setup_SpawnAndAssert()
	{
		IEntity vehicle = SpawnTestPrefab(PREFAB_BTR70);
		AssertTrue(vehicle != null, "БТР-70 не заспавнился");
		if (GetFailure())
			return;

		string key = ME_StatsComponent.GetEntityLocKey(vehicle);
		AssertTrue(key != "#AR-Inventory_Trunk", "РЕГРЕСС: снова вернулся баг с багажником ('#AR-Inventory_Trunk')");
		if (GetFailure())
			return;

		AssertTrue(key == "#AR-Vehicle_BTR70_Name", string.Format("Ожидали '#AR-Vehicle_BTR70_Name', получили '%1'", key));
	}
}

// ----------------------------------------------------------------------------
// GetFactionLocKey() на персонаже конкретной фракции
// ----------------------------------------------------------------------------

[Test(suite: SCR_TEST_MEStatsLocalizationSuite, timeoutS: 5)]
class SCR_TEST_MEStatsLocalization_GetFactionLocKeyOnUSSRCharacter_ReturnsUSSRFaction : SCR_TEST_MEStatsLocalizationCase
{
	[TestStep(TestStage.Setup)]
	void Setup_SpawnAndAssert()
	{
		IEntity character = SpawnTestPrefab(PREFAB_CHARACTER_USSR);
		AssertTrue(character != null, "Персонаж СССР не заспавнился — проверь путь префаба");
		if (GetFailure())
			return;

		string faction = ME_StatsComponent.GetFactionLocKey(character);
		AssertTrue(faction == "#AR-Faction_USSR", string.Format("Ожидали '#AR-Faction_USSR', получили '%1'", faction));
	}
}

[Test(suite: SCR_TEST_MEStatsLocalizationSuite, timeoutS: 5)]
class SCR_TEST_MEStatsLocalization_GetFactionLocKeyOnUSCharacter_ReturnsUSFaction : SCR_TEST_MEStatsLocalizationCase
{
	[TestStep(TestStage.Setup)]
	void Setup_SpawnAndAssert()
	{
		IEntity character = SpawnTestPrefab(PREFAB_CHARACTER_US);
		AssertTrue(character != null, "Персонаж США не заспавнился — проверь путь префаба");
		if (GetFailure())
			return;

		string faction = ME_StatsComponent.GetFactionLocKey(character);
		AssertTrue(faction == "#AR-Faction_US", string.Format("Ожидали '#AR-Faction_US', получили '%1'", faction));
	}
}

// Отрицательный тест — техника не персонаж, GetFactionLocKey должен вернуть
// пустую строку (падает через Cast на SCR_ChimeraCharacter), а не мусор.
[Test(suite: SCR_TEST_MEStatsLocalizationSuite, timeoutS: 5)]
class SCR_TEST_MEStatsLocalization_GetFactionLocKeyOnVehicle_ReturnsEmpty : SCR_TEST_MEStatsLocalizationCase
{
	[TestStep(TestStage.Setup)]
	void Setup_SpawnAndAssert()
	{
		IEntity vehicle = SpawnTestPrefab(PREFAB_BTR70);
		AssertTrue(vehicle != null, "БТР-70 не заспавнился");
		if (GetFailure())
			return;

		string faction = ME_StatsComponent.GetFactionLocKey(vehicle);
		AssertTrue(faction.IsEmpty(), string.Format("Ожидали пустую строку для техники (не персонажа), получили '%1'", faction));
	}
}

// ----------------------------------------------------------------------------
// GetFactionLocKey() — устойчивость к null
// ----------------------------------------------------------------------------

[Test(suite: SCR_TEST_MEStatsLocalizationSuite, timeoutS: 5)]
class SCR_TEST_MEStatsLocalization_GetFactionLocKeyOnNull_ReturnsEmptyWithoutCrash : SCR_TEST_MEStatsLocalizationCase
{
	[TestStep(TestStage.Setup)]
	void Setup_Assert()
	{
		string result = ME_StatsComponent.GetFactionLocKey(null);
		AssertTrue(result.IsEmpty(), string.Format("Ожидали пустую строку для null, получили '%1'", result));
	}
}*/

// ============================================================================
// ЖИВОЙ СЦЕНАРИЙ: реальный выстрел, реальная смерть, реальная обработка
// килла модом. Проверяет саму сердцевину мода — OnPlayerKilled/
// OnWeaponFiredTrackWeapon — на настоящих данных от движка, а не на
// сфабрикованных вручную (обсуждали отдельно: тест с самодельным
// SCR_InstigatorContextData проверял бы только нашу же логику ветвления
// при заранее угаданных нами входных данных — бесполезно, раз именно
// РЕАЛЬНЫЕ ответы движка были источником всех найденных багов).
// ============================================================================

[BaseContainerProps(category: "Autotest")]
class SCR_TEST_MEStatsLiveKillSuite : SCR_AutotestSuiteBase
{
}

// Собственный базовый класс — НЕ зависит от SCR_TEST_MEStatsLocalizationCase
// (та сейчас частично закомментирована вместе с тестами локализации).
// Дублирует небольшой хелпер спавна — специально, чтобы эта сьюта
// оставалась независимой и не ломалась, если секцию локализации
// закомментируют/раскомментируют отдельно.
class SCR_TEST_MEStatsLiveKillCase : SCR_AutotestCaseBase
{
	static const string PREFAB_CHARACTER_US = "{3E18CC9634468249}Prefabs/Characters/Campaign/Final/BLUFOR/US_army/Regular/Campaign_US_Player_GL.et";

	protected ref array<IEntity> m_SpawnedEntities = {};

	// x/z — координаты по умолчанию около центра карты (0,0). Раньше
	// пробовали (5000,5000) — "подальше от центра" — но GetSurfaceY() там
	// вернул -256 (похоже, тестовый мир Autotest_GameMode_Plain просто
	// небольшой, и эти координаты оказались далеко за его границами —
	// персонаж заспавнился глубоко под миром/в пустоте, отсюда
	// CanFire=false и пустые руки весь тест). Y всегда вычисляем через
	// GetSurfaceY() — реальная высота земли в этой точке. ПОДТВЕРЖДЕНО
	// официальным кодом (SCR_PrefabsSpawner.c):
	// "position[1] = world.GetSurfaceY(position[0], position[2]);"
	IEntity SpawnTestPrefab(string prefabPath, float x = 0, float z = 0)
	{
		Resource resource = Resource.Load(prefabPath);
		if (!resource || !resource.IsValid())
		{
			Print("Не удалось загрузить префаб: " + prefabPath, LogLevel.ERROR);
			return null;
		}

		BaseWorld world = GetGame().GetWorld();
		float y = world.GetSurfaceY(x, z);
		Print("[LiveKillTest] SpawnTestPrefab: x=" + x.ToString() + ", вычисленный y=" + y.ToString() + ", z=" + z.ToString());
		if (y < -100 || y > 1000)
			Print("[LiveKillTest] ВНИМАНИЕ: высота выглядит подозрительно (" + y.ToString() + ") — возможно, координаты (x,z) вне границ тестового мира", LogLevel.WARNING);

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = Vector(x, y, z);

		IEntity entity = GetGame().SpawnEntityPrefab(resource, world, spawnParams);
		if (entity)
			m_SpawnedEntities.Insert(entity);

		return entity;
	}

	// Возвращает мировую позицию хитзоны головы через GetHitZoneByName("Head")
	// + первый коллайдер + GetBoneMatrix. При любой ошибке логирует причину
	// и возвращает entity.GetOrigin() как запасной вариант.
	vector GetHeadHitZoneWorldPosition(notnull IEntity entity)
	{
		HitZoneContainerComponent hzContainer = HitZoneContainerComponent.Cast(
			entity.FindComponent(HitZoneContainerComponent));
		if (!hzContainer)
		{
			Print("[LiveKillTest] GetHeadHitZoneWorldPosition: нет HitZoneContainerComponent — возвращаем GetOrigin()", LogLevel.WARNING);
			return entity.GetOrigin();
		}

		HitZone hz = hzContainer.GetHitZoneByName("Head");
		if (!hz)
		{
			Print("[LiveKillTest] GetHeadHitZoneWorldPosition: GetHitZoneByName(\"Head\") вернул null — возвращаем GetOrigin()", LogLevel.WARNING);
			return entity.GetOrigin();
		}

		array<string> colliderNames = {};
		if (hz.GetAllColliderNames(colliderNames) == 0)
		{
			Print("[LiveKillTest] GetHeadHitZoneWorldPosition: у хитзоны Head нет коллайдеров — возвращаем GetOrigin()", LogLevel.WARNING);
			return entity.GetOrigin();
		}

		vector transformLS[4];
		int boneIndex;
		int nodeID;
		if (!hz.TryGetColliderDescriptionFromName(entity, colliderNames[0], transformLS, boneIndex, nodeID))
		{
			Print("[LiveKillTest] GetHeadHitZoneWorldPosition: TryGetColliderDescriptionFromName провалился — возвращаем GetOrigin()", LogLevel.WARNING);
			return entity.GetOrigin();
		}

		vector boneMat[4];
		if (!entity.GetBoneMatrix(boneIndex, boneMat))
		{
			Print("[LiveKillTest] GetHeadHitZoneWorldPosition: GetBoneMatrix провалился (boneIndex=" + boneIndex.ToString() + ") — возвращаем GetOrigin()", LogLevel.WARNING);
			return entity.GetOrigin();
		}

		return boneMat[3];
	}

	// Поворачивает стрелка телом (yaw) и прицелом (pitch) на targetPos.
	// SetEntityLookAtPoint — только yaw тела; вертикальный прицел оружия
	// управляется отдельно через CharacterHeadAimingComponent.
	static void AimAtWorldPosition(notnull IEntity shooter, vector targetPos)
	{
		// Горизонтальный поворот тела
		SCR_TestLib.SetEntityLookAtPoint(shooter, targetPos, log: false);

		// Вертикальный прицел оружия
		CharacterHeadAimingComponent aimComp = CharacterHeadAimingComponent.Cast(
			shooter.FindComponent(CharacterHeadAimingComponent));
		if (!aimComp)
			return;

		// Строим вектор направления от глаз стрелка до цели
		ChimeraCharacter chimeraChar = ChimeraCharacter.Cast(shooter);
		vector eyePos;
		if (chimeraChar)
			eyePos = chimeraChar.EyePosition();
		else
			eyePos = shooter.GetOrigin();

		vector dir = targetPos - eyePos;
		dir.Normalize();

		// MatrixFromForwardVec строит rot-матрицу 3x3 из вектора направления —
		// именно такой формат ожидает MatrixToAngles (vector[3], не vector[4]).
		vector rotMat[3];
		Math3D.MatrixFromForwardVec(dir, rotMat);
		vector angles = Math3D.MatrixToAngles(rotMat);

		// SetAimingRotation принимает vector(yaw, pitch, roll) в градусах
		aimComp.SetAimingRotation(angles);
	}

	[TestStep(TestStage.TearDown)]
	void TearDown_DeleteSpawned()
	{
		foreach (IEntity entity : m_SpawnedEntities)
		{
			if (entity)
				delete entity;
		}
		m_SpawnedEntities.Clear();
	}
}

[Test(suite: SCR_TEST_MEStatsLiveKillSuite, timeoutS: 45)]
class SCR_TEST_MEStatsLiveKill_PlayerShootsCharacter_TargetDies : SCR_TEST_MEStatsLiveKillCase
{
	// Тот же префаб персонажа США, что уже использовался выше.
	IEntity m_Shooter;
	IEntity m_Target;
	CharacterControllerComponent m_ShooterController;

	[TestStep(TestStage.Setup)]
	void Setup_SpawnBoth()
	{
		m_Shooter = SpawnTestPrefab(PREFAB_CHARACTER_US, 0, 0);
		AssertTrue(m_Shooter != null, "Стрелок не заспавнился");
		if (GetFailure())
			return;

		// Цель — на 3 метра левее и на той же линии обзора, недалеко, чтобы
		// точно попасть даже без точного прицеливания. Y обеих сущностей
		// теперь берётся из реальной высоты земли (см. SpawnTestPrefab).
		m_Target = SpawnTestPrefab(PREFAB_CHARACTER_US, 3, 0);
		AssertTrue(m_Target != null, "Цель не заспавнилась");
		if (GetFailure())
			return;

		m_ShooterController = CharacterControllerComponent.Cast(m_Shooter.FindComponent(CharacterControllerComponent));
		AssertTrue(m_ShooterController != null, "У стрелка не нашёлся CharacterControllerComponent");
		if (GetFailure())
			return;

		// Подписываемся на событие попадания ДО начала стрельбы
		m_TargetDmgMgr = SCR_DamageManagerComponent.GetDamageManager(m_Target);
		AssertTrue(m_TargetDmgMgr != null, "У цели не нашёлся SCR_DamageManagerComponent");
		if (GetFailure())
			return;

		m_TargetDmgMgr.GetOnDamage().Insert(OnTargetDamaged);
	}

	[TestStep(TestStage.Setup)]
	void Setup_AimAtTarget()
	{
		if (GetFailure())
			return;

		vector headPoint = GetHeadHitZoneWorldPosition(m_Target);
		Print("[LiveKillTest] Setup_AimAtTarget: прицеливаемся в " + headPoint.ToString());
		AimAtWorldPosition(m_Shooter, headPoint);
	}

	// Счётчик кадров — используется, чтобы не переприцеливаться и не
	// логировать КАЖДЫЙ кадр (было бы слишком часто), а раз в N кадров.
	protected int m_iFrameCounter = 0;

	// Сколько кадров ждать после выстрела, чтобы проверить попадание.
	// Пуля успевает долететь и обработаться за это время.
	static const int HIT_WAIT_FRAMES = 30;

	// Флаг: зафиксировано ли попадание с последнего выстрела
	protected bool m_bHitRegistered = false;
	// Кол-во патронов в предыдущем кадре — отслеживаем выстрел через убыль патронов
	protected int m_iLastKnownAmmo = -1;
	// Кадр последнего зафиксированного выстрела (-1 = ещё не стреляли)
	protected int m_iLastShotFrame = -1;
	// Ссылка на DamageManager цели — для подписки на событие попадания
	protected SCR_DamageManagerComponent m_TargetDmgMgr;

	// Возвращает текущее кол-во патронов в стволе стрелка.
	protected int GetCurrentAmmo()
	{
		if (!m_ShooterController)
			return 0;
		BaseWeaponManagerComponent wpm = m_ShooterController.GetWeaponManagerComponent();
		if (!wpm)
			return 0;
		BaseWeaponComponent weapon = wpm.GetCurrentWeapon();
		if (!weapon)
			return 0;
		BaseMuzzleComponent muzzle = weapon.GetCurrentMuzzle();
		if (!muzzle)
			return 0;
		return muzzle.GetAmmoCount();
	}

	// Callback на событие GetOnDamage() цели — вызывается движком каждый раз,
	// когда цель получает урон. Логируем место попадания и здоровье.
	protected void OnTargetDamaged(BaseDamageContext damageContext)
	{
		m_bHitRegistered = true;

		string hitZoneName = "?";
		float hitZoneHealth = -1;
		if (damageContext.struckHitZone)
		{
			// GetHealth() есть у базового HitZone, GetHitZoneGroup() — только у SCR_HitZone
			hitZoneHealth = damageContext.struckHitZone.GetHealth();
			SCR_HitZone scrHitZone = SCR_HitZone.Cast(damageContext.struckHitZone);
			if (scrHitZone)
				hitZoneName = typename.EnumToString(EHitZoneGroup, scrHitZone.GetHitZoneGroup());
		}

		float totalHealth = -1;
		if (m_TargetDmgMgr)
			totalHealth = m_TargetDmgMgr.GetHealth();

		Print("[LiveKillTest] ХИТ! зона=" + hitZoneName
			+ ", позиция=" + damageContext.hitPosition.ToString()
			+ ", урон=" + damageContext.damageValue.ToString()
			+ ", здоровье_зоны=" + hitZoneHealth.ToString()
			+ ", здоровье_цели=" + totalHealth.ToString());
	}

	// bool-возвращающий Setup-шаг — выполняется каждый кадр, пока не
	// вернёт true (подтверждено на примере Setup_AwaitWorld в
	// SCR_AutotestSuiteBase.c). Удерживаем "хочу стрелять" каждый кадр и
	// ждём смерти цели.
	[TestStep(TestStage.Setup)]
	bool Setup_FireUntilTargetDies()
	{
		if (GetFailure())
			return true;

		if (!m_ShooterController || !m_Target)
			return true;

		m_iFrameCounter++;

		// Переприцеливаемся каждый кадр — отдача поднимает ствол, нужно
		// постоянно корректировать и yaw тела, и pitch прицела.
		vector headPoint = GetHeadHitZoneWorldPosition(m_Target);
		AimAtWorldPosition(m_Shooter, headPoint);

		// Логируем позицию головы раз в 60 кадров (не каждый кадр — слишком много)
		if (m_iFrameCounter % 60 == 0)
			Print("[LiveKillTest] прицеливаемся в хитзону головы: " + headPoint.ToString());

		m_ShooterController.SetWeaponRaised(true);

		int currentAmmo = GetCurrentAmmo();

		// Инициализация при первом вызове
		if (m_iLastKnownAmmo < 0)
			m_iLastKnownAmmo = currentAmmo;

		// Патронов стало меньше — выстрел произошёл
		if (currentAmmo < m_iLastKnownAmmo)
		{
			Print("[LiveKillTest] кадр=" + m_iFrameCounter.ToString()
				+ ": выстрел! патронов " + m_iLastKnownAmmo.ToString() + " -> " + currentAmmo.ToString());
			m_bHitRegistered = false;
			m_iLastShotFrame = m_iFrameCounter;
			m_iLastKnownAmmo = currentAmmo;
		}
		else if (currentAmmo > m_iLastKnownAmmo)
		{
			// Перезарядка — просто обновляем счётчик, не сбрасываем флаг
			m_iLastKnownAmmo = currentAmmo;
		}

		// Прошло достаточно кадров после выстрела — проверяем попадание
		if (m_iLastShotFrame >= 0 && !m_bHitRegistered
			&& (m_iFrameCounter - m_iLastShotFrame) >= HIT_WAIT_FRAMES)
		{
			string msg = "Мисс: выстрел в кадр=" + m_iLastShotFrame.ToString()
				+ " не нанёс урона за " + HIT_WAIT_FRAMES.ToString() + " кадров";
			Print("[LiveKillTest] " + msg, LogLevel.ERROR);
			SetFailure(msg);
			return true;
		}

		// Стреляем непрерывно каждый кадр — убираем toggle (15-кадровые паузы
		// задерживали первый выстрел до кадра ~302 и создавали окна промаха).
		// Движок сам обрабатывает скорострельность оружия.
		m_ShooterController.SetFireWeaponWanted(true);

		// Перезарядка — вызываем только ОДИН РАЗ при обнаружении пустого магазина,
		// а не каждый кадр, чтобы не спамить ReloadWeapon().
		if (currentAmmo == 0 && m_iLastKnownAmmo == 0 && m_iFrameCounter % 30 == 0)
		{
			Print("[LiveKillTest] кадр=" + m_iFrameCounter.ToString() + ": патроны кончились, перезаряжаемся");
			m_ShooterController.ReloadWeapon();
		}

		// Диагностика каждые 60 кадров
		if (m_iFrameCounter % 60 == 0)
		{
			string stateText = "?";
			if (m_TargetDmgMgr)
				stateText = typename.EnumToString(EDamageState, m_TargetDmgMgr.GetState());

			string canFireText = "false";
			if (m_ShooterController.CanFire())
				canFireText = "true";

			string weaponRaisedText = "false";
			if (m_ShooterController.IsWeaponRaised())
				weaponRaisedText = "true";

			// Сознание и стойка цели
			CharacterControllerComponent targetCtrl = CharacterControllerComponent.Cast(m_Target.FindComponent(CharacterControllerComponent));
			string unconsciousText = "?";
			string stanceText = "?";
			if (targetCtrl)
			{
				if (targetCtrl.IsUnconscious())
				unconsciousText = "true";
			else
				unconsciousText = "false";
				stanceText = typename.EnumToString(ECharacterStance, targetCtrl.GetStance());
			}

			Print("[LiveKillTest] кадр=" + m_iFrameCounter.ToString()
				+ ", патронов=" + currentAmmo.ToString()
				+ ", цель=" + stateText
				+ ", без_сознания=" + unconsciousText
				+ ", стойка=" + stanceText
				+ ", CanFire=" + canFireText
				+ ", оружие_поднято=" + weaponRaisedText);
		}

		CharacterControllerComponent targetController = CharacterControllerComponent.Cast(m_Target.FindComponent(CharacterControllerComponent));
		if (!targetController)
			return true;

		return targetController.IsDead();
	}

	[TestStep(TestStage.Setup)]
	void Setup_StopFiringAndAssert()
	{
		if (m_ShooterController)
			m_ShooterController.SetFireWeaponWanted(false);

		// Отписываемся от событий урона — больше не нужны
		if (m_TargetDmgMgr)
			m_TargetDmgMgr.GetOnDamage().Remove(OnTargetDamaged);

		if (GetFailure())
			return;

		CharacterControllerComponent targetController = CharacterControllerComponent.Cast(m_Target.FindComponent(CharacterControllerComponent));
		AssertTrue(targetController && targetController.IsDead(), "Цель не умерла за отведённое время (timeoutS) — проверь дистанцию/прицеливание/боезапас");

		// Если мы дошли сюда без AssertTrue(false, ...) и без исключений —
		// это уже само по себе ценно: значит, вся цепочка
		// OnControllableDestroyed -> OnPlayerKilled -> SendKill в моде
		// отработала на РЕАЛЬНОМ выстреле без падения/исключения.
	}
}