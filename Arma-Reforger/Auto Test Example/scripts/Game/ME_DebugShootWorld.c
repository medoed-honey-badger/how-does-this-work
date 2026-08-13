[ComponentEditorProps(category: "Debug", description: "Visual shoot debug")]
class ME_DebugShootCompClass : ScriptComponentClass
{
}

class ME_DebugShootComp : ScriptComponent
{
	static const string PREFAB_US = "{3E18CC9634468249}Prefabs/Characters/Campaign/Final/BLUFOR/US_army/Regular/Campaign_US_Player_GL.et";

	protected IEntity m_Shooter;
	protected IEntity m_Target;
	protected CharacterControllerComponent m_ShooterCtrl;
	protected SCR_DamageManagerComponent m_TargetDmgMgr;

	protected int m_iFrame = 0;
	protected int m_iLastShotFrame = -1;
	protected int m_iLastKnownAmmo = -1;
	protected bool m_bSetupDone = false;

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
		if (!m_bSetupDone || !m_ShooterCtrl || !m_Target)
			return;
		DoShootLoop();
	}

	protected void FaceToward(notnull IEntity entity, vector targetPos)
	{
		vector from = entity.GetOrigin();
		vector dir = targetPos - from;
		dir[1] = 0;
		if (dir.LengthSq() < 0.0001)
			return;
		dir = dir.Normalized();
		vector mat[4];
		Math3D.DirectionAndUpMatrix(dir, "0 1 0", mat);
		mat[3] = from;
		entity.SetTransform(mat);
	}

	protected void DoSetup()
	{
		BaseWorld world = GetGame().GetWorld();
		float yShooter = world.GetSurfaceY(5, 5);
		float yTarget  = world.GetSurfaceY(8, 5);
		Print("[DebugShoot] surface y: shooter=" + yShooter.ToString() + " target=" + yTarget.ToString());

		m_Shooter = SpawnChar(PREFAB_US, 5, 5, yShooter, world);
		if (!m_Shooter) { Print("[DebugShoot] FAIL: shooter not spawned", LogLevel.ERROR); return; }

		m_Target = SpawnChar(PREFAB_US, 8, 5, yTarget, world);
		if (!m_Target) { Print("[DebugShoot] FAIL: target not spawned", LogLevel.ERROR); return; }

		m_ShooterCtrl = CharacterControllerComponent.Cast(m_Shooter.FindComponent(CharacterControllerComponent));
		if (!m_ShooterCtrl) { Print("[DebugShoot] FAIL: no CharacterControllerComponent", LogLevel.ERROR); return; }

		m_TargetDmgMgr = SCR_DamageManagerComponent.GetDamageManager(m_Target);
		if (m_TargetDmgMgr)
			m_TargetDmgMgr.GetOnDamage().Insert(OnTargetDamaged);

		FaceToward(m_Shooter, m_Target.GetOrigin());
		Print("[DebugShoot] setup done. shooter=" + m_Shooter.GetOrigin().ToString() + " target=" + m_Target.GetOrigin().ToString());

		m_bSetupDone = true;
		m_iFrame = 0;
	}

	protected void OnTargetDamaged(BaseDamageContext ctx)
	{
		float hp = -1;
		string zone = "?";
		if (ctx.struckHitZone)
		{
			hp = ctx.struckHitZone.GetHealth();
			SCR_HitZone scrHz = SCR_HitZone.Cast(ctx.struckHitZone);
			if (scrHz)
				zone = typename.EnumToString(EHitZoneGroup, scrHz.GetHitZoneGroup());
		}
		float totalHp = -1;
		if (m_TargetDmgMgr)
			totalHp = m_TargetDmgMgr.GetHealth();
		Print("[DebugShoot] HIT zone=" + zone + " dmg=" + ctx.damageValue.ToString()
			+ " hp_zone=" + hp.ToString() + " hp_total=" + totalHp.ToString());
	}

	protected void DoShootLoop()
	{
		// Физика персонажа сбрасывает SetTransform каждый тик —
		// поворачиваем каждый кадр, иначе персонаж разворачивается обратно.
		FaceToward(m_Shooter, m_Target.GetOrigin());

		m_ShooterCtrl.SetWeaponRaised(true);

		int ammo = GetCurrentAmmo();
		if (m_iLastKnownAmmo < 0)
			m_iLastKnownAmmo = ammo;
		if (ammo < m_iLastKnownAmmo)
		{
			Print("[DebugShoot] frame=" + m_iFrame.ToString() + " SHOT ammo " + m_iLastKnownAmmo.ToString() + "->" + ammo.ToString());
			m_iLastShotFrame = m_iFrame;
			m_iLastKnownAmmo = ammo;
		}
		else if (ammo > m_iLastKnownAmmo)
			m_iLastKnownAmmo = ammo;

		if (ammo == 0 && m_iLastKnownAmmo == 0 && m_iFrame % 30 == 0)
			m_ShooterCtrl.ReloadWeapon();

		bool aimed = IsAimedAtTarget();
		if (aimed && (m_iFrame / 15) % 2 == 0)
			m_ShooterCtrl.SetFireWeaponWanted(true);
		else
			m_ShooterCtrl.SetFireWeaponWanted(false);

		if (m_iFrame % 60 == 0)
		{
			string tgtState = "?";
			if (m_TargetDmgMgr)
				tgtState = typename.EnumToString(EDamageState, m_TargetDmgMgr.GetState());
			string dead = "no";
			CharacterControllerComponent tgtCtrl = CharacterControllerComponent.Cast(m_Target.FindComponent(CharacterControllerComponent));
			if (tgtCtrl && tgtCtrl.IsDead())
				dead = "YES";
			string canFire = "false";
			if (m_ShooterCtrl.CanFire())
				canFire = "true";
			vector fwdMat[4];
			m_Shooter.GetTransform(fwdMat);
			Print("[DebugShoot] frame=" + m_iFrame.ToString()
				+ " ammo=" + ammo.ToString()
				+ " target=" + tgtState
				+ " dead=" + dead
				+ " CanFire=" + canFire
				+ " aimed=" + aimed.ToString()
				+ " fwd=" + fwdMat[2].ToString());
		}
	}

	protected bool IsAimedAtTarget()
	{
		if (!m_Shooter || !m_Target)
			return false;
		vector mat[4];
		m_Shooter.GetTransform(mat);
		vector forward = mat[2];
		forward[1] = 0;
		forward = forward.Normalized();
		vector toTarget = m_Target.GetOrigin() - m_Shooter.GetOrigin();
		toTarget[1] = 0;
		toTarget = toTarget.Normalized();
		return vector.Dot(forward, toTarget) > 0.95;
	}

	protected int GetCurrentAmmo()
	{
		if (!m_ShooterCtrl) return 0;
		BaseWeaponManagerComponent wpm = m_ShooterCtrl.GetWeaponManagerComponent();
		if (!wpm) return 0;
		BaseWeaponComponent wpn = wpm.GetCurrentWeapon();
		if (!wpn) return 0;
		BaseMuzzleComponent muzzle = wpn.GetCurrentMuzzle();
		if (!muzzle) return 0;
		return muzzle.GetAmmoCount();
	}

	protected IEntity SpawnChar(string prefab, float x, float z, float y, BaseWorld world)
	{
		Resource res = Resource.Load(prefab);
		if (!res || !res.IsValid())
		{
			Print("[DebugShoot] FAIL load: " + prefab, LogLevel.ERROR);
			return null;
		}
		EntitySpawnParams p = new EntitySpawnParams();
		p.TransformMode = ETransformMode.WORLD;
		p.Transform[3] = Vector(x, y, z);
		return GetGame().SpawnEntityPrefab(res, world, p);
	}
}
