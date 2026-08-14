[ComponentEditorProps(category: "Debug", description: "Visual shoot debug")]
class ME_DebugShootCompClass : ScriptComponentClass
{
}

class ME_DebugShootComp : ScriptComponent
{
	static const string PREFAB_US = "{3E18CC9634468249}Prefabs/Characters/Campaign/Final/BLUFOR/US_army/Regular/Campaign_US_Player_GL.et";
	static const string PREFAB_USSR = "{F3C4020CA491A6E1}Prefabs/Characters/Campaign/Final/OPFOR/USSR_Army/Regular/Campaign_USSR_Player_GL.et";

	protected IEntity m_Shooter;
	protected IEntity m_Target;
	protected CharacterControllerComponent m_ShooterCtrl;
	protected AimingComponent m_ShooterAiming;
	protected SCR_DamageManagerComponent m_TargetDmgMgr;

	protected int m_iFrame = 0;
	protected int m_iLastShotFrame = -1;
	protected int m_iLastKnownAmmo = -1;
	protected int m_iShotsFired = 0;
	protected bool m_bSetupDone = false;
	protected bool m_bDone = false;

	// Мировая позиция Spine3 цели — вычисляется один раз в DoSetup
	protected vector m_vTargetTorso;
	// Индекс кости Spine3 цели — кэшируется в DoSetup
	protected TNodeId m_iSpineBone = -1;

	static const int MAX_SHOTS = 10;

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

	protected float DirToHeading(vector dir)
	{
		return Math.Atan2(dir[0], dir[2]);
	}

	protected void FaceToward(notnull IEntity entity, vector targetPos)
	{
		vector from = entity.GetOrigin();
		vector dir = targetPos - from;
		if (dir.LengthSq() < 0.0001)
			return;
		vector mat[4];
		Math3D.DirectionAndUpMatrix(dir.Normalized(), vector.Up, mat);
		mat[3] = from;
		entity.SetTransform(mat);
	}

	protected void DoSetup()
	{
		BaseWorld world = GetGame().GetWorld();
		float yShooter = world.GetSurfaceY(5, 5);
		float yTarget  = world.GetSurfaceY(8, 8);
		Print("[DebugShoot] surface y: shooter=" + yShooter.ToString() + " target=" + yTarget.ToString());

		m_Shooter = SpawnChar(PREFAB_US, 5, 5, yShooter, world);
		if (!m_Shooter) { Print("[DebugShoot] FAIL: shooter not spawned", LogLevel.ERROR); return; }

		m_Target = SpawnChar(PREFAB_USSR, 8, 8, yTarget, world);
		if (!m_Target) { Print("[DebugShoot] FAIL: target not spawned", LogLevel.ERROR); return; }

		m_ShooterCtrl = CharacterControllerComponent.Cast(m_Shooter.FindComponent(CharacterControllerComponent));
		if (!m_ShooterCtrl) { Print("[DebugShoot] FAIL: no CharacterControllerComponent", LogLevel.ERROR); return; }

		// Разворачиваем стрелка в сторону цели
		vector shooterPos = m_Shooter.GetOrigin();
		vector targetPos = m_Target.GetOrigin();
		vector toTarget = targetPos - shooterPos;
		toTarget[1] = 0;
		toTarget = toTarget.Normalized();
		float headingRad = Math.Atan2(toTarget[0], toTarget[2]);
		m_ShooterCtrl.SetHeadingAngle(headingRad, true);
		Print("[DebugShoot] SetHeadingAngle=" + headingRad.ToString() + " rad (" + (headingRad * 57.2958).ToString() + " deg)");
		if (!m_ShooterCtrl) { Print("[DebugShoot] FAIL: no CharacterControllerComponent", LogLevel.ERROR); return; }

		CharacterHeadAimingComponent headAiming = CharacterHeadAimingComponent.Cast(m_Shooter.FindComponent(CharacterHeadAimingComponent));
		if (headAiming)
			m_ShooterAiming = headAiming.GetCharacterAimingComponent();
		if (!m_ShooterAiming) { Print("[DebugShoot] WARN: no AimingComponent", LogLevel.WARNING); }

		m_TargetDmgMgr = SCR_DamageManagerComponent.GetDamageManager(m_Target);
		if (m_TargetDmgMgr)
			m_TargetDmgMgr.GetOnDamage().Insert(OnTargetDamaged);

		Print("[DebugShoot] setup done. shooter=" + m_Shooter.GetOrigin().ToString() + " target=" + m_Target.GetOrigin().ToString());
		LogTargetSkeleton();

		m_bSetupDone = true;
		m_iFrame = 0;
	}

	// Кэшируем индекс кости Spine3 и логируем все кости корпуса для справки.
	protected void LogTargetSkeleton()
	{
		if (!m_Target)
			return;

		Animation animation = m_Target.GetAnimation();
		if (!animation)
		{
			Print("[DebugShoot] WARN: target has no Animation", LogLevel.WARNING);
			return;
		}

		array<string> boneNames = {};
		animation.GetBoneNames(boneNames);
		Print("[DebugShoot] target skeleton bones=" + boneNames.Count().ToString());

		foreach (string boneName : boneNames)
		{
			string nameLower = boneName;
			nameLower.ToLower();
			if (nameLower.Contains("spine") || nameLower.Contains("chest") || nameLower.Contains("torso") || nameLower.Contains("pelvis"))
			{
				TNodeId bone = animation.GetBoneIndex(boneName);
				vector matrix[4];
				if (animation.GetBoneMatrix(bone, matrix))
				{
					vector worldPos = m_Target.GetOrigin() + matrix[3];
					Print("[DebugShoot] torsoBone name=" + boneName + " local=" + matrix[3].ToString() + " world=" + worldPos.ToString());
					if (boneName == "Spine3")
					{
						m_iSpineBone = bone;
						m_vTargetTorso = worldPos;
						Print("[DebugShoot] Spine3 cached, world=" + worldPos.ToString());
					}
				}
				else
					Print("[DebugShoot] torsoBone name=" + boneName + " matrix unavailable", LogLevel.WARNING);
			}
		}

		if (m_iSpineBone < 0)
			Print("[DebugShoot] WARN: Spine3 not found, torso aim disabled", LogLevel.WARNING);
	}

	// Компенсирует горизонтальное смещение ствола через SetAimingAngles.
	// Пуля вылетает из X≈5.21 (смещение +0.21 от origin).
	// Чтобы попасть в центр цели, нужен небольшой yaw влево.
	protected void AimTowardTorso()
	{
		if (!m_Target || m_iSpineBone < 0)
			return;

		Animation anim = m_Target.GetAnimation();
		if (!anim)
			return;

		vector boneMat[4];
		if (!anim.GetBoneMatrix(m_iSpineBone, boneMat))
			return;

		vector torsoWorld = m_Target.GetOrigin() + boneMat[3];

		// Позиция ствола с учётом поворота стрелка:
		// смещение +0.21 по локальной правой оси (mat[0]) и +1.47 по вертикали
		vector shooterMat[4];
		m_Shooter.GetTransform(shooterMat);
		vector right = shooterMat[0];
		vector muzzlePos = m_Shooter.GetOrigin();
		muzzlePos[0] = muzzlePos[0] + right[0] * 0.21;
		muzzlePos[1] = muzzlePos[1] + 1.47;
		muzzlePos[2] = muzzlePos[2] + right[2] * 0.21;

		vector toTorso = torsoWorld - muzzlePos;
		float horizDist = Math.Sqrt(toTorso[0] * toTorso[0] + toTorso[2] * toTorso[2]);
		if (horizDist < 0.01)
			return;

		// Только yaw — не трогаем pitch, пуля и так летит в торс по высоте
		float yawRad = Math.Atan2(toTorso[0], toTorso[2]);

		CharacterInputContext ctx = m_ShooterCtrl.GetInputContext();
		if (!ctx)
			return;

		ctx.SetAimingAngles(Vector(yawRad, 0, 0));
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

		// Позиция попадания относительно origin цели
		vector hitPos = ctx.hitPosition;
		vector tgtOrigin = vector.Zero;
		if (m_Target)
			tgtOrigin = m_Target.GetOrigin();
		vector hitRelative = hitPos - tgtOrigin;

		Print("[DebugShoot] HIT zone=" + zone + " dmg=" + ctx.damageValue.ToString()
			+ " hp_zone=" + hp.ToString() + " hp_total=" + totalHp.ToString()
			+ " hitPos=" + hitPos.ToString()
			+ " hitRelative=" + hitRelative.ToString());
	}

	protected void DoShootLoop()
	{
		if (m_bDone)
			return;

		m_ShooterCtrl.SetWeaponRaised(true);

		// Разворачиваем стрелка к цели каждый кадр
		vector sPos = m_Shooter.GetOrigin();
		vector tPos = m_Target.GetOrigin();
		vector toTgt2 = tPos - sPos;
		toTgt2[1] = 0;
		if (toTgt2.LengthSq() > 0.0001)
		{
			toTgt2 = toTgt2.Normalized();
			m_ShooterCtrl.SetHeadingAngle(Math.Atan2(toTgt2[0], toTgt2[2]), true);
		}

		// Прицеливаемся в Spine3 цели каждый кадр
		AimTowardTorso();

		int ammo = GetCurrentAmmo();
		if (m_iLastKnownAmmo < 0)
			m_iLastKnownAmmo = ammo;
		if (ammo < m_iLastKnownAmmo)
		{
			m_iShotsFired++;
			Print("[DebugShoot] frame=" + m_iFrame.ToString() + " SHOT " + m_iShotsFired.ToString()
				+ " ammo " + m_iLastKnownAmmo.ToString() + "->" + ammo.ToString());
			m_iLastShotFrame = m_iFrame;
			m_iLastKnownAmmo = ammo;

			if (m_iShotsFired >= MAX_SHOTS)
			{
				m_ShooterCtrl.SetFireWeaponWanted(false);
				m_bDone = true;
				Print("[DebugShoot] === DONE: достигнут лимит " + MAX_SHOTS.ToString() + " выстрелов, цель не обезврежена ===");
				return;
			}
		}
		else if (ammo > m_iLastKnownAmmo)
			m_iLastKnownAmmo = ammo;

		// Проверяем смерть цели после каждого выстрела
		CharacterControllerComponent deadCtrl = CharacterControllerComponent.Cast(m_Target.FindComponent(CharacterControllerComponent));
		if (deadCtrl && deadCtrl.IsDead())
		{
			m_ShooterCtrl.SetFireWeaponWanted(false);
			m_bDone = true;
			Print("[DebugShoot] === DONE: цель обезврежена после " + m_iShotsFired.ToString() + " выстрелов (frame=" + m_iFrame.ToString() + ") ===");
			return;
		}

		if (ammo == 0 && m_iLastKnownAmmo == 0 && m_iFrame % 30 == 0)
			m_ShooterCtrl.ReloadWeapon();

		// FaceToward ставит точный поворот каждый кадр — стреляем сразу
		if ((m_iFrame / 15) % 2 == 0)
			m_ShooterCtrl.SetFireWeaponWanted(true);
		else
			m_ShooterCtrl.SetFireWeaponWanted(false);

		if (m_iFrame % 60 == 0)
		{
			string tgtState = "?";
			if (m_TargetDmgMgr)
				tgtState = typename.EnumToString(EDamageState, m_TargetDmgMgr.GetState());
			string dead = "no";
			CharacterControllerComponent logCtrl = CharacterControllerComponent.Cast(m_Target.FindComponent(CharacterControllerComponent));
			if (logCtrl && logCtrl.IsDead())
				dead = "YES";
			string canFire = "false";
			if (m_ShooterCtrl.CanFire())
				canFire = "true";
			vector fwdMat[4];
			m_Shooter.GetTransform(fwdMat);
			string aimDirStr = "n/a";
			string aimRotStr = "n/a";
			string inputAnglesStr = "n/a";
			if (m_ShooterAiming)
			{
				aimDirStr = m_ShooterAiming.GetAimingDirectionWorld().ToString();
				aimRotStr = m_ShooterAiming.GetAimingRotation().ToString();
			}
			CharacterInputContext inputCtx = m_ShooterCtrl.GetInputContext();
			if (inputCtx)
				inputAnglesStr = inputCtx.GetAimingAngles().ToString();
			vector shooterPos = m_Shooter.GetOrigin();
			vector targetPos = m_Target.GetOrigin();
			vector toTgt = targetPos - shooterPos;
			toTgt[1] = 0;
			toTgt = toTgt.Normalized();
			vector fwdFlat = fwdMat[2];
			fwdFlat[1] = 0;
			fwdFlat = fwdFlat.Normalized();
			float dot = vector.Dot(fwdFlat, toTgt);
			Print("[DebugShoot] frame=" + m_iFrame.ToString()
				+ " ammo=" + ammo.ToString()
				+ " shots=" + m_iShotsFired.ToString()
				+ " target=" + tgtState
				+ " dead=" + dead
				+ " CanFire=" + canFire
				+ " shooterPos=" + shooterPos.ToString()
				+ " targetPos=" + targetPos.ToString()
				+ " toTgt=" + toTgt.ToString()
				+ " fwd=" + fwdMat[2].ToString()
				+ " dot=" + dot.ToString()
				+ " aimDir=" + aimDirStr
				+ " aimRot=" + aimRotStr
				+ " inputAngles=" + inputAnglesStr
				+ " torso=" + m_vTargetTorso.ToString());
		}
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
