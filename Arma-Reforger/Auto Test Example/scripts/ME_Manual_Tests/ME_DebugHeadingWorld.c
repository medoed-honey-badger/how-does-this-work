// Тест поворота персонажа через CharacterControllerComponent.SetHeadingAngle().
// Heading angle — угол в радианах (0..2pi) вокруг мировой оси Y.
// Это штатный игровой способ задать направление: движок сам проигрывает
// анимацию поворота и удерживает результат, в отличие от SetTransform
// (сбрасывается физикой) и AlignPosDirWS (плавно возвращается назад).
//
// Добавь компонент ME_DebugHeadingComp на любую сущность в World Editor.

[ComponentEditorProps(category: "Debug", description: "Heading angle turn debug")]
class ME_DebugHeadingCompClass : ScriptComponentClass
{
}

class ME_DebugHeadingComp : ScriptComponent
{
	static const string PREFAB_US = "{3E18CC9634468249}Prefabs/Characters/Campaign/Final/BLUFOR/US_army/Regular/Campaign_US_Player_GL.et";

	protected IEntity m_Char;
	protected IEntity m_Target;
	protected CharacterControllerComponent m_CharCtrl;

	protected int m_iFrame = 0;
	protected bool m_bSetupDone = false;
	protected bool m_bHeadingSet = false;
	protected float m_fDesiredHeading = 0;

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

		DoHeadingLoop();
	}

	protected void DoSetup()
	{
		BaseWorld world = GetGame().GetWorld();

		float yChar   = world.GetSurfaceY(5, 5);
		float yTarget = world.GetSurfaceY(8, 5);

		m_Char = SpawnChar(PREFAB_US, 5, 5, yChar, world);
		if (!m_Char) { Print("[HeadingDebug] FAIL: char not spawned", LogLevel.ERROR); return; }

		m_Target = SpawnChar(PREFAB_US, 8, 5, yTarget, world);
		if (!m_Target) { Print("[HeadingDebug] FAIL: target not spawned", LogLevel.ERROR); return; }

		m_CharCtrl = CharacterControllerComponent.Cast(m_Char.FindComponent(CharacterControllerComponent));
		if (!m_CharCtrl) { Print("[HeadingDebug] FAIL: no CharacterControllerComponent", LogLevel.ERROR); return; }

		Print("[HeadingDebug] Setup done. char=" + m_Char.GetOrigin().ToString() + " target=" + m_Target.GetOrigin().ToString());

		m_bSetupDone = true;
		m_iFrame = 0;
	}

	// Переводит горизонтальное направление в heading angle (радианы, вокруг Y).
	protected float DirToHeading(vector dir)
	{
		return Math.Atan2(dir[0], dir[2]);
	}

	// Текущий heading персонажа из его матрицы — для сравнения в логах.
	protected float GetActualHeading()
	{
		vector mat[4];
		m_Char.GetTransform(mat);
		vector fwd = mat[2];
		fwd[1] = 0;
		if (fwd.LengthSq() < 0.0001)
			return 0;
		fwd = fwd.Normalized();
		return DirToHeading(fwd);
	}

	protected void DoHeadingLoop()
	{
		// 5 секунд стоим на месте, чтобы визуально заметить момент поворота
		if (m_iFrame < 300)
			return;

		if (m_iFrame == 300)
		{
			vector charPos = m_Char.GetOrigin();
			vector targetPos = m_Target.GetOrigin();

			vector desiredDir = targetPos - charPos;
			desiredDir[1] = 0;
			desiredDir = desiredDir.Normalized();

			m_fDesiredHeading = DirToHeading(desiredDir);
			float actualBefore = GetActualHeading();

			Print("[HeadingDebug] === ДО ПОВОРОТА (кадр 300) ===");
			Print("[HeadingDebug] heading до=" + (actualBefore * Math.RAD2DEG).ToString() + " град");
			Print("[HeadingDebug] desiredDir=" + desiredDir.ToString()
				+ " heading желаемый=" + (m_fDesiredHeading * Math.RAD2DEG).ToString() + " град");

			// adjustAimingYaw=true — чтобы прицел поворачивался вместе с корпусом
			m_CharCtrl.SetHeadingAngle(m_fDesiredHeading, true);
			m_bHeadingSet = true;

			Print("[HeadingDebug] === SetHeadingAngle ВЫЗВАН ===");
		}

		// Проверяем, держится ли поворот: 1, 2, 3, 4 секунды после
		if (m_bHeadingSet && (m_iFrame == 360 || m_iFrame == 420 || m_iFrame == 480 || m_iFrame == 540))
		{
			float actual = GetActualHeading();
			float diff = (actual - m_fDesiredHeading) * Math.RAD2DEG;
			Print("[HeadingDebug] кадр " + m_iFrame.ToString()
				+ ": heading=" + (actual * Math.RAD2DEG).ToString() + " град"
				+ " отклонение=" + diff.ToString() + " град");
		}
	}

	protected IEntity SpawnChar(string prefab, float x, float z, float y, BaseWorld world)
	{
		Resource res = Resource.Load(prefab);
		if (!res || !res.IsValid())
		{
			Print("[HeadingDebug] FAIL load: " + prefab, LogLevel.ERROR);
			return null;
		}
		EntitySpawnParams p = new EntitySpawnParams();
		p.TransformMode = ETransformMode.WORLD;
		p.Transform[3] = Vector(x, y, z);
		return GetGame().SpawnEntityPrefab(res, world, p);
	}
}
