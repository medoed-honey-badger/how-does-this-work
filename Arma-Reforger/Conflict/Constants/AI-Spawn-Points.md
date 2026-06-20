# Точки появления ботов и их показатели

## Airport

| # | Group type | Group Count | Probability of presence, % | Coords |
| :-- | ----------- |----------- |----------- |----------- |
| 1 | Fire Team | 5 | 66 | 4953.892 28.38 11979.468 |
| 2 | Sniper Team | 2 | 33 | 4918.985 44.314 11787.571 |
| 3 | Fire Team | 5 | 65 | 0.948 -0.178 70.098 |
| 4 | Fire Team | 5 | 75 | -1.505 0.539 -110.659 |
| 5 | Fire Team | 5 | 90 | 0 0 0 |
| 6 | MG Team | 2 | 85 | -3.195 7.697 38.932 |
| 7 | Rifle Squad | 7 | 95 | -17.685 0 11.49 |
| 8 | Sniper Team | 2 | 85 | -17.498 15.428 -2.189 |

Итого: **33**. Если все активируются

## Location_Name

## Location_Name

## Location_Name

## Location_Name

## Location_Name

## Location_Name

## Location_Name

## Location_Name

## Location_Name


## Константы по составу группы

1. Доп. инфа находится в `SCR_CampaignFactionManager -> Unsorted -> Factions -> Нужная фракция -> Entity Catalogs -> Group -> Multi Lists -> Not Spawned -> Entities -> Нужная группа -> Entitie Data List -> AmbientPatrolsData -> Group Type`
2. *Probability of presence* срабатывает **только** если выбран случайный вариант в настройке точки появления


### FIA

| # |    Тип группы   |         Prefab          | Состав | Probability of presence, % | Кол-во |
| :-| --------------- | ----------------------- | ----------- | :-----------: | -----------: |
| 1 | TEAM_SENTRY | Group_FIA_ReconTeam_NotSpawned.et | *Group_FIA_ReconTeam*<br><br>1. Character_FIA_Scout<br>2. Character_FIA_RTO | 100 | 2 |
| 2 | TEAM_MG | Group_FIA_MachineGunTeam_NotSpawned.et | *Group_FIA_MachineGunTeam*<br><br>1. Character_FIA_MG<br>2. Character_FIA_AMG | 50 | 2 |
| 3 | TEAM_AT | Group_FIA_Team_AT_NotSpawned.et | *Group_FIA_Team_AT*<br><br>1. Character_FIA_Rifleman<br>2. Character_FIA_AT<br>3. Character_FIA_AT<br>4. Character_FIA_AAT | 50 | 4 |
| 4 | FIRETEAM | Group_FIA_FireTeam_NotSpawned.et | *Group_FIA_FireTeam*<br><br>1. Character_FIA_SL<br>2. Character_FIA_Medic<br>3. Character_FIA_Rifleman<br>4. Character_FIA_Rifleman<br>5. Character_FIA_LAT | 30 | 5 |
| 5 | SQUAD_RIFLE | Group_FIA_RifleSquad_NotSpawned.et | *Group_FIA_RifleSquad*<br><br>1. Character_FIA_SL<br>2. Character_FIA_MG<br>3. Character_FIA_AMG<br>4. Character_FIA_LAT<br>5. Character_FIA_RTO<br>6. Character_FIA_Rifleman<br>7. Character_FIA_LAT | 15 | 7 |
| 6 | TEAM_SNIPER | Group_FIA_SniperTeam_NotSpawned.et | *Group_FIA_ReconTeam*<br><br>1. Character_FIA_Sharpshooter<br>2. Character_FIA_Scout | 50 | 2 |
| 7 | TEAM_MG_ELITE | Group_FIA_MachineGunTeam_Elite_NotSpawned.et | *Group_FIA_MachineGunTeam*<br><br>1. Character_FIA_AMG<br>2. Character_FIA_MG_Elite | 50 | 2 |
| 8 | PARTISAN_SNIPER | Group_FIA_LoneWolfSniper_NotSpawned.et | *Group_FIA_ReconTeam*<br><br>1. Character_FIA_Rebel_Sharpshooter| 30 | 1 |
| 9 | PARTISAN_TEAM | Group_FIA_PartisanTeam_NotSpawned.et | *Group_FIA_FireTeam*<br><br>1. Character_FIA_AC_Partisan<br>2. Character_FIA_AC_Partisan_Grenadier<br>3. Character_FIA_AC_Medic<br>4. Character_FIA_AC_Scout<br>5. Character_FIA_AC_Partisan| 30 | 5 |