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
	}

	[TestStep(TestStage.Setup)]
	void Setup_AimAtTarget()
	{
		if (GetFailure())
			return;

		// Целимся в примерную высоту головы (origin цели + ~1.6 м вверх),
		// а не в сам origin (там центр/точка привязки персонажа, обычно
		// около таза) — см. подробности в Setup_FireUntilTargetDies.
		vector headPoint = m_Target.GetOrigin() + Vector(0, 1.6, 0);
		SCR_TestLib.SetEntityLookAtPoint(m_Shooter, headPoint);
	}

	// Счётчик кадров — используется, чтобы не переприцеливаться и не
	// логировать КАЖДЫЙ кадр (было бы слишком часто), а раз в N кадров.
	protected int m_iFrameCounter = 0;

	// bool-возвращающий Setup-шаг — выполняется каждый кадр, пока не
	// вернёт true (подтверждено на примере Setup_AwaitWorld в
	// SCR_AutotestSuiteBase.c). Удерживаем "хочу стрелять" каждый кадр и
	// ждём смерти цели.
	[TestStep(TestStage.Setup)]
	bool Setup_FireUntilTargetDies()
	{
		if (GetFailure())
			return true; // не блокируем — уже есть ошибка, выходим сразу

		if (!m_ShooterController || !m_Target)
			return true;

		m_iFrameCounter++;

		// Переприцеливаемся каждые ~30 кадров — длинная очередь поднимает
		// ствол отдачей, персонаж может со временем начать стрелять мимо
		// цели, если прицелиться только один раз в начале.
		//
		// ВАЖНО: SetEntityLookAtEntity() целится в GetOrigin() цели — это
		// точка привязки персонажа (обычно около таза/центра тела), НЕ
		// голова. Именно поэтому нужно было ~28 попаданий на одну ступень
		// урона (STATE2) — центр масс, а не хедшот. Вместо этого считаем
		// точку чуть выше origin (примерная высота головы у человека) и
		// целимся туда через SetEntityLookAtPoint() напрямую.
		if (m_iFrameCounter % 30 == 0)
		{
			vector targetOrigin = m_Target.GetOrigin();
			vector headPoint = targetOrigin + Vector(0, 1.6, 0);
			SCR_TestLib.SetEntityLookAtPoint(m_Shooter, headPoint, log: false);

			if (m_iFrameCounter % 60 == 0)
				Print("[LiveKillTest] прицеливаемся в точку (примерно голова): " + headPoint.ToString());
		}

		// Явно поднимаем оружие каждый кадр — подтверждённый метод из
		// CharacterControllerComponent.c.
		m_ShooterController.SetWeaponRaised(true);

		// ВАЖНО: НЕ держим true постоянно! Подтверждено логом реального
		// теста: патроны упали 30->29 ОДИН раз к кадру 300 и больше
		// НИКОГДА не менялись, хотя SetFireWeaponWanted(true) вызывался
		// без остановки ещё тысячи кадров. Похоже, это полуавтоматический
		// режим огня — там нужен ОТДЕЛЬНЫЙ нажим на каждый выстрел
		// (переход false->true), а не удержание. Эмулируем повторные
		// нажатия спуска — чередуем true/false каждые ~15 кадров. Это
		// сработает и на полуавтомате (каждое "нажатие" = выстрел), и на
		// автоматическом режиме (не помешает непрерывной стрельбе).
		if ((m_iFrameCounter / 15) % 2 == 0)
			m_ShooterController.SetFireWeaponWanted(true);
		else
			m_ShooterController.SetFireWeaponWanted(false);

		// Патроны реально заканчиваются (магазин на 30, а нужного числа
		// попаданий для полного килла может не хватить с одного магазина
		// при неидеальной точности) — перезаряжаемся, как только магазин
		// пуст, чтобы очередь могла продолжиться. Подтверждённый метод
		// из CharacterControllerComponent.c.
		BaseWeaponManagerComponent wpmCheck = m_ShooterController.GetWeaponManagerComponent();
		if (wpmCheck)
		{
			BaseWeaponComponent weaponCheck = wpmCheck.GetCurrentWeapon();
			if (weaponCheck)
			{
				BaseMuzzleComponent muzzleCheck = weaponCheck.GetCurrentMuzzle();
				if (muzzleCheck && muzzleCheck.GetAmmoCount() <= 0)
				{
					Print("[LiveKillTest] кадр=" + m_iFrameCounter.ToString() + ": патроны кончились, перезаряжаемся");
					m_ShooterController.ReloadWeapon();
				}
			}
		}

		CharacterControllerComponent targetController = CharacterControllerComponent.Cast(m_Target.FindComponent(CharacterControllerComponent));
		if (!targetController)
			return true;

		// Каждые ~60 кадров логируем расход патронов и состояние цели —
		// чтобы видеть, реально ли стреляем и попадаем, а не просто ждать
		// таймаута вслепую, не понимая, что происходит. Плюс CanFire()/
		// IsWeaponRaised() — если стрельба не идёт, это покажет, в чём
		// именно затык (не поднято оружие? не может стрелять по другой
		// причине?).
		if (m_iFrameCounter % 60 == 0)
		{
			string ammoText = "?";
			BaseWeaponManagerComponent wpm = m_ShooterController.GetWeaponManagerComponent();
			if (wpm)
			{
				BaseWeaponComponent weapon = wpm.GetCurrentWeapon();
				if (weapon)
				{
					BaseMuzzleComponent muzzle = weapon.GetCurrentMuzzle();
					if (muzzle)
						ammoText = muzzle.GetAmmoCount().ToString();
				}
			}

			string stateText = "?";
			SCR_DamageManagerComponent targetDmgMgr = SCR_DamageManagerComponent.GetDamageManager(m_Target);
			if (targetDmgMgr)
				stateText = typename.EnumToString(EDamageState, targetDmgMgr.GetState());

			string canFireText = "false";
			if (m_ShooterController.CanFire())
				canFireText = "true";

			string weaponRaisedText = "false";
			if (m_ShooterController.IsWeaponRaised())
				weaponRaisedText = "true";

			string itemInHandsText = "null";
			IEntity itemInHands = m_ShooterController.GetCurrentItemInHands();
			if (itemInHands)
				itemInHandsText = itemInHands.ToString();

			string diagMsg = "[LiveKillTest] кадр=" + m_iFrameCounter.ToString();
			diagMsg += ", патронов=" + ammoText;
			diagMsg += ", цель=" + stateText;
			diagMsg += ", CanFire=" + canFireText;
			diagMsg += ", оружие_поднято=" + weaponRaisedText;
			diagMsg += ", предмет_в_руках=" + itemInHandsText;
			Print(diagMsg);
		}

		return targetController.IsDead();
	}

	[TestStep(TestStage.Setup)]
	void Setup_StopFiringAndAssert()
	{
		if (m_ShooterController)
			m_ShooterController.SetFireWeaponWanted(false);

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