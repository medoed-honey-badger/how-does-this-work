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
}
