// Визуальный тест поворота персонажа через CharacterHeadingAnimComponent.
// Без выстрелов — только проверка, что персонаж разворачивается к цели.
//
// Пробуем AlignPosDirWS вместо SetTransform: этот метод работает на уровне
// анимационной физики, а не на уровне матрицы сущности, поэтому физика
// не должна его сбрасывать.
//
// Добавь компонент ME_DebugTurnComp на любую сущность в World Editor.

[ComponentEditorProps(category: "Debug", description: "Visual turn debug")]
class ME_DebugTurnCompClass : ScriptComponentClass
{
}

class ME_DebugTurnComp : ScriptComponent
{
	static const string PREFAB_US = "{3E18CC9634468249}Prefabs/Characters/Campaign/Final/BLUFOR/US_army/Regular/Campaign_US_Player_GL.et";

	protected IEntity m_Char;
	protected IEntity m_Target;
	protected CharacterAnimationComponent m_AnimComp;
	protected CharacterHeadingAnimComponent m_HeadingComp;

	protected int m_iFrame = 0;
	protected bool m_bSetupDone = false;
	protected bool m_bTurned = false;

	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.FRAME);
	}

	override protected void EOnFrame(IEntity owner, float timeSlice)
	{
		m_iFrame++;
		if (!m_bSetupDone && m_iFrame == 60)
			DoSetup();
		if (!m_bSetupDone)
			return;

		DoTurnLoop();
	}

	protected void DoSetup()
	{
		BaseWorld world = GetGame().GetWorld();

		float yChar   = world.GetSurfaceY(5, 5);
		float yTarget = world.GetSurfaceY(8, 5);

		m_Char = SpawnChar(PREFAB_US, 5, 5, yChar, world);
		if (!m_Char) { Print("[TurnDebug] FAIL: char not spawned", LogLevel.ERROR); return; }

		m_Target = SpawnChar(PREFAB_US, 8, 5, yTarget, world);
		if (!m_Target) { Print("[TurnDebug] FAIL: target not spawned", LogLevel.ERROR); return; }

		CharacterControllerComponent ctrl = CharacterControllerComponent.Cast(m_Char.FindComponent(CharacterControllerComponent));
		if (!ctrl) { Print("[TurnDebug] FAIL: no CharacterControllerComponent", LogLevel.ERROR); return; }

		m_AnimComp = ctrl.GetAnimationComponent();
		if (!m_AnimComp) { Print("[TurnDebug] FAIL: no CharacterAnimationComponent", LogLevel.ERROR); return; }

		m_HeadingComp = m_AnimComp.GetHeadingComponent();
		if (!m_HeadingComp) { Print("[TurnDebug] FAIL: no CharacterHeadingAnimComponent", LogLevel.ERROR); return; }

		Print("[TurnDebug] Setup done. char=" + m_Char.GetOrigin().ToString() + " target=" + m_Target.GetOrigin().ToString());
		Print("[TurnDebug] HeadingComp ok: " + m_HeadingComp.ToString());

		m_bSetupDone = true;
		m_iFrame = 0;
	}

	protected void DoTurnLoop()
	{
		// Ждём 300 кадров (~5 секунд) перед поворотом
		if (m_iFrame < 300)
			return;

		// Поворачиваем только один раз
		if (m_iFrame == 300)
		{
			vector charPos = m_Char.GetOrigin();
			vector targetPos = m_Target.GetOrigin();

			// Текущее направление персонажа до поворота
			vector matBefore[4];
			m_Char.GetTransform(matBefore);
			vector dirBefore = matBefore[2];
			dirBefore[1] = 0;
			if (dirBefore.LengthSq() > 0.0001)
				dirBefore = dirBefore.Normalized();

			Print("[TurnDebug] === ДО ПОВОРОТА (кадр 300) ===");
			Print("[TurnDebug] dirBefore=" + dirBefore.ToString());

			// Желаемое направление — горизонтальный вектор к цели
			vector desiredDir = targetPos - charPos;
			desiredDir[1] = 0;
			desiredDir = desiredDir.Normalized();

			Print("[TurnDebug] desiredDir=" + desiredDir.ToString());

			// Поворачиваем через HeadingAnimComponent — snap=true для мгновенного поворота
			m_HeadingComp.AlignPosDirWS(charPos, dirBefore, charPos, desiredDir, true);

			m_bTurned = true;

			Print("[TurnDebug] === ПОВОРОТ ВЫПОЛНЕН ===");
		}

		// Логируем направление через некоторое время после поворота
		if (m_bTurned && (m_iFrame == 360 || m_iFrame == 420 || m_iFrame == 480 || m_iFrame == 540))
		{
			vector matAfter[4];
			m_Char.GetTransform(matAfter);
			vector dirAfter = matAfter[2];
			dirAfter[1] = 0;
			if (dirAfter.LengthSq() > 0.0001)
				dirAfter = dirAfter.Normalized();

			Print("[TurnDebug] ПОСЛЕ ПОВОРОТА (кадр " + m_iFrame.ToString() + "): dirAfter=" + dirAfter.ToString());
		}
	}

	protected IEntity SpawnChar(string prefab, float x, float z, float y, BaseWorld world)
	{
		Resource res = Resource.Load(prefab);
		if (!res || !res.IsValid())
		{
			Print("[TurnDebug] FAIL load: " + prefab, LogLevel.ERROR);
			return null;
		}
		EntitySpawnParams p = new EntitySpawnParams();
		p.TransformMode = ETransformMode.WORLD;
		p.Transform[3] = Vector(x, y, z);
		return GetGame().SpawnEntityPrefab(res, world, p);
	}
}
