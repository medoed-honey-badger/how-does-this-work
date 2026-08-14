// ============================================================================
// Диагностический тест прицеливания — проверяет, поворачивается ли персонаж
// и целится ли в цель. Детальное логирование всех параметров для отладки.
// ============================================================================

[BaseContainerProps(category: "Autotest")]
class SCR_TEST_MEAimingDiagnosticSuite : SCR_AutotestSuiteBase
{
}

class SCR_TEST_MEAimingDiagnosticCase : SCR_AutotestCaseBase
{
	static const string PREFAB_CHARACTER_US = "{3E18CC9634468249}Prefabs/Characters/Campaign/Final/BLUFOR/US_army/Regular/Campaign_US_Player_GL.et";

	protected ref array<IEntity> m_SpawnedEntities = {};

	IEntity SpawnTestPrefab(string prefabPath, float x = 0, float z = 0)
	{
		Resource resource = Resource.Load(prefabPath);
		if (!resource || !resource.IsValid())
		{
			Print("[AimDiag] Не удалось загрузить префаб: " + prefabPath, LogLevel.ERROR);
			return null;
		}

		BaseWorld world = GetGame().GetWorld();
		float y = world.GetSurfaceY(x, z);
		Print("[AimDiag] SpawnTestPrefab: x=" + x.ToString() + ", y=" + y.ToString() + ", z=" + z.ToString());

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = Vector(x, y, z);

		IEntity entity = GetGame().SpawnEntityPrefab(resource, world, spawnParams);
		if (entity)
		{
			m_SpawnedEntities.Insert(entity);
			Print("[AimDiag] Заспавнен: " + entity.ToString() + " в позиции " + entity.GetOrigin().ToString());
		}

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

// ============================================================================
// Тест: проверяет поворот персонажа лицом к цели
// ============================================================================

[Test(suite: SCR_TEST_MEAimingDiagnosticSuite, timeoutS: 30)]
class SCR_TEST_MEAimingDiagnostic_CharacterTurnsToFaceTarget : SCR_TEST_MEAimingDiagnosticCase
{
	IEntity m_Shooter;
	IEntity m_Target;
	CharacterControllerComponent m_ShooterController;
	int m_iFrameCounter = 0;

	[TestStep(TestStage.Setup)]
	void Setup_SpawnBoth()
	{
		Print("[AimDiag] ====== НАЧАЛО ТЕСТА ПРИЦЕЛИВАНИЯ ======");

		// Стрелок в точке (0, 0)
		m_Shooter = SpawnTestPrefab(PREFAB_CHARACTER_US, 0, 0);
		AssertTrue(m_Shooter != null, "Стрелок не заспавнился");
		if (GetFailure())
			return;

		// Цель на расстоянии 3 метра по оси X
		m_Target = SpawnTestPrefab(PREFAB_CHARACTER_US, 3, 0);
		AssertTrue(m_Target != null, "Цель не заспавнилась");
		if (GetFailure())
			return;

		m_ShooterController = CharacterControllerComponent.Cast(m_Shooter.FindComponent(CharacterControllerComponent));
		AssertTrue(m_ShooterController != null, "У стрелка нет CharacterControllerComponent");
		if (GetFailure())
			return;

		Print("[AimDiag] Оба персонажа заспавнены успешно");
	}

	// Логирует детальную информацию о позициях и ориентации
	void LogDetailedAimingInfo(string phase)
	{
		Print("[AimDiag] ========================================");
		Print("[AimDiag] ФАЗА: " + phase);

		if (!m_Shooter || !m_Target)
		{
			Print("[AimDiag] ОШИБКА: стрелок или цель = null");
			return;
		}

		// Позиции
		vector shooterPos = m_Shooter.GetOrigin();
		vector targetPos = m_Target.GetOrigin();
		Print("[AimDiag] Позиция стрелка: " + shooterPos.ToString());
		Print("[AimDiag] Позиция цели:    " + targetPos.ToString());

		// Дистанция
		vector toTarget = targetPos - shooterPos;
		float distance = toTarget.Length();
		Print("[AimDiag] Расстояние: " + distance.ToString() + " м");

		// Трансформация стрелка
		vector shooterMat[4];
		m_Shooter.GetTransform(shooterMat);
		Print("[AimDiag] Трансформация стрелка:");
		Print("[AimDiag]   Right:   " + shooterMat[0].ToString());
		Print("[AimDiag]   Up:      " + shooterMat[1].ToString());
		Print("[AimDiag]   Forward: " + shooterMat[2].ToString());
		Print("[AimDiag]   Pos:     " + shooterMat[3].ToString());

		// Направление взгляда (forward вектор, горизонтальная проекция)
		vector forward = shooterMat[2];
		vector forwardHoriz = forward;
		forwardHoriz[1] = 0;
		if (forwardHoriz.LengthSq() > 0.0001)
			forwardHoriz = forwardHoriz.Normalized();
		Print("[AimDiag] Forward (горизонт): " + forwardHoriz.ToString());

		// Вектор к цели (горизонтальная проекция)
		vector toTargetHoriz = toTarget;
		toTargetHoriz[1] = 0;
		if (toTargetHoriz.LengthSq() > 0.0001)
			toTargetHoriz = toTargetHoriz.Normalized();
		Print("[AimDiag] К цели (горизонт):  " + toTargetHoriz.ToString());

		// Скалярное произведение (cos угла)
		float dotProduct = vector.Dot(forwardHoriz, toTargetHoriz);
		Print("[AimDiag] Dot product: " + dotProduct.ToString() + " (1.0 = идеально, 0.95 = порог попадания)");

		// Угол в градусах
		float angleRad = Math.Acos(dotProduct);
		float angleDeg = angleRad * Math.RAD2DEG;
		Print("[AimDiag] Угол отклонения: " + angleDeg.ToString() + "°");

		// Проверка прицеливания
		bool isAimed = (dotProduct > 0.95);
		Print("[AimDiag] Прицелен: " + isAimed.ToString());
	}

	// Поворачивает стрелка лицом к цели через SetHeadingAngle (ПОДТВЕРЖДЁННЫЙ МЕТОД)
	void TurnTowardTarget(notnull CharacterControllerComponent ctrl, vector shooterPos, vector targetPos)
	{
		Print("[AimDiag] TurnTowardTarget: начинаем поворот через SetHeadingAngle");

		vector dir = targetPos - shooterPos;
		dir[1] = 0;

		Print("[AimDiag] TurnTowardTarget: from=" + shooterPos.ToString() + ", to=" + targetPos.ToString());
		Print("[AimDiag] TurnTowardTarget: вектор направления (до нормализации)=" + dir.ToString());

		if (dir.LengthSq() < 0.0001)
		{
			Print("[AimDiag] TurnTowardTarget: вектор слишком короткий, поворот отменён");
			return;
		}

		dir = dir.Normalized();
		Print("[AimDiag] TurnTowardTarget: нормализованный вектор=" + dir.ToString());

		// Math.Atan2 принимает (x, z) и возвращает угол в радианах
		float headingRad = Math.Atan2(dir[0], dir[2]);
		Print("[AimDiag] TurnTowardTarget: вычисленный угол=" + headingRad.ToString() + " рад (" + (headingRad * Math.RAD2DEG).ToString() + "°)");

		ctrl.SetHeadingAngle(headingRad, true);
		Print("[AimDiag] TurnTowardTarget: SetHeadingAngle вызван с adjustAimingYaw=true");
	}

	[TestStep(TestStage.Setup)]
	void Setup_LogInitialState()
	{
		if (GetFailure())
			return;

		LogDetailedAimingInfo("НАЧАЛЬНОЕ СОСТОЯНИЕ (до поворота)");
	}

	[TestStep(TestStage.Setup)]
	void Setup_TurnToTarget()
	{
		if (GetFailure())
			return;

		Print("[AimDiag] ------ Выполняем поворот к цели ------");
		TurnTowardTarget(m_ShooterController, m_Shooter.GetOrigin(), m_Target.GetOrigin());
	}

	[TestStep(TestStage.Setup)]
	void Setup_LogAfterTurn()
	{
		if (GetFailure())
			return;

		LogDetailedAimingInfo("СРАЗУ ПОСЛЕ ПОВОРОТА (кадр 0)");
	}

	// Ждём несколько кадров, логируем состояние
	[TestStep(TestStage.Setup)]
	bool Setup_WaitAndObserve()
	{
		if (GetFailure())
			return true;

		m_iFrameCounter++;

		// Логируем каждые 30 кадров
		if (m_iFrameCounter % 30 == 0)
		{
			LogDetailedAimingInfo("КАДР " + m_iFrameCounter.ToString());
		}

		// Завершаем через 180 кадров (~3 секунды при 60 FPS)
		if (m_iFrameCounter >= 180)
		{
			LogDetailedAimingInfo("ФИНАЛЬНОЕ СОСТОЯНИЕ (кадр " + m_iFrameCounter.ToString() + ")");
			return true;
		}

		return false;
	}

	[TestStep(TestStage.Setup)]
	void Setup_FinalCheck()
	{
		if (GetFailure())
			return;

		Print("[AimDiag] ====== ТЕСТ ЗАВЕРШЁН ======");
		Print("[AimDiag] Проверь логи выше:");
		Print("[AimDiag]   1. Forward вектор должен совпадать с направлением к цели после SetHeadingAngle");
		Print("[AimDiag]   2. Dot product должен оставаться близким к 1.0 без дополнительных вызовов");
		Print("[AimDiag]   3. Угол отклонения должен быть близок к 0° на всём протяжении теста");
		Print("[AimDiag]   4. Персонаж не должен визуально дёргаться или возвращаться обратно");
	}
}