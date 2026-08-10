[BaseContainerProps(category: "Autotest")]
class SCR_TEST_CharacterWeaponShootingSuite : SCR_AutotestSuiteBase
{
	override ResourceName GetWorldFile()
	{
		// allows us to specify the world the test suite will run in, uses MpTest by default
		return super.GetWorldFile();
	}
	// you can remove these overrides if you intend to use default values
}


[Test(suite: SCR_TEST_CharacterWeaponShootingSuite, timeoutS: 5)]
class SCR_TEST_CharacterWeaponShooting_MyTestCase_MyExpectedResult : SCR_AutotestCaseBase
{
	[Step(EStage.Setup)]
	void Setup()
	{
		// void returning methods will execute once
	}

	[Step(EStage.Setup)]
	bool Setup_Await()
	{
		// bool returning methods will keep executing until the method returns true
 		return true;
	}

	[Step(EStage.Main)]
	bool Execute_DoSomething()
	{
		Print("Execute_DoSomething");
		// return false; // uncomment this and the test will keep executing until it timeouts
		return true;
	}

	[Step(EStage.Main)]
	void Execute_Assert()
	{
		Print("Execute_Assert");

		// assert state of the test environment in separate method
		// if the state is different than expected SetResult of the test to failure.
		if (false)
		{
			SetResult(SCR_AutotestResult.AsFailure("Test failure due to some reason"));
			return;
		}

	 	if (true)
		{
			SetResult(SCR_AutotestResult.AsFailure("Test failure due to other reason"));
			return;
		}

		SetResult(SCR_AutotestResult.AsSuccess());
	}
}



class SCR_TEST_CharacterWeaponShootingCase : SCR_AutotestCaseBase
{
	static string CHARACTER_PREFAB = "{26A9756790131354}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman.et";
}

[Test(suite: SCR_TEST_CharacterWeaponShootingSuite, timeoutS: 5)]
class SCR_TEST_CharacterWeaponShooting_PlayerShoots_AiDies : SCR_TEST_CharacterWeaponShootingCase
{
	SCR_ChimeraCharacter m_PlayerCharacter;
	SCR_ChimeraCharacter m_TargetCharacter;

	[Step(EStage.Setup)]
	void Setup_CreateCharacters()
	{
		vector spawnPosPlayer = "120 1 120";
		vector spawnPosTarget = spawnPosPlayer + "0 0 2.5";

		// Entities created via SCR_TestLib.Spawn* methods will be automatilly deleted before subsequent tests will run
		m_PlayerCharacter = SCR_ChimeraCharacter.Cast(SCR_TestLib.SpawnPlayer(CHARACTER_PREFAB, spawnPosPlayer));
		m_TargetCharacter = SCR_ChimeraCharacter.Cast(SCR_TestLib.SpawnEntity(CHARACTER_PREFAB, spawnPosTarget));
	}

	[Step(EStage.Main)]
	bool Execute_DoFire()
	{
		// this will be executed every frame until the method returns true
		SCR_TestLib.SetPlayerLookAtEntity(m_TargetCharacter);
		SCR_TestLib.SetActionValue(SCR_TestLib.ACTION_FIRE, Math.RandomInt(0, 2));

		return !SCR_TestLib.IsCharacterAlive(m_TargetCharacter);
	}

	[Step(EStage.Main)]
	void Execute_Assert()
	{
		// for readability purposes it's the best to do all assertions in separate method
		// this test is pretty simple, if it reached this stage it was completed successfully.
		SetResult(SCR_AutotestResult.AsSuccess());
	}
}