# Точки появления ботов и их показатели

| # | Location | Group type | Group Count | Probability of presence, % | Coords | Sum |
| :-- | ----------- |----------- |----------- |----------- |----------- |----------- |
| 1 | Airport | Fire Team | | 66 | 4953.892 28.38 11979.468 | |
| 2 | Airport | Sniper Team | | 33 | 4918.985 44.314 11787.571 | |
| 3 | Airport | Fire Team | | 65 | 0.948 -0.178 70.098 | |
| 4 | Airport | Fire Team | | 75 | -1.505 0.539 -110.659 | |
| 5 | Airport | Fire Team | | 90 | 0 0 0 | |
| 6 | Airport | MG Team | | 85 | -3.195 7.697 38.932 | |
| 7 | Airport | Rifle Squad |  | 95 | -17.685 0 11.49 | |
| 8 | Airport | Sniper Team | | 85 | -17.498 15.428 -2.189 | |
| 9 | | | | | | |
| 10 | | | | | | |
| 11 | | | | | | |
| 12 | | | | | | |
| 13 | | | | | | |
| 14 | | | | | | |
| 15 | | | | | | |
| 16 | | | | | | |
| 17 | | | | | | |












## Константы по составу группы
1. Доп. инфа находится в `SCR_CampaignFactionManager -> Unsorted -> Factions -> Нужная фракция -> Entity Catalogs -> Group -> Multi Lists -> Not Spawned -> Entities -> Нужная группа -> Entitie Data List -> AmbientPatrolsData -> Group Type`
2. *Probability of presence* срабатывает **только** если выбран случайный вариант в настройке точки появления

|    Тип группы   |         Prefab          | Состав | Probability of presence, % | Кол-во |
| --------------- | ----------------------- | ----------- | ----------- | ----------- |
| TEAM_SENTRY | | | | |
| TEAM_MG | | | | |
| TEAM_AT | Group_FIA_Team_AT_NotSpawned.et | *Group_FIA_Team_AT* | 50 | |
| FIRETEAM | Group_FIA_FireTeam_NotSpawned.et | *Group_FIA_FireTeam* \n 1. Character_FIA_SL\n2. Character_FIA_Medic\n3. Character_FIA_Rifleman\n4. Character_FIA_Rifleman\n5. Character_FIA_LAT | 30 | |
| SQUAD_RIFLE | | | | |
| TEAM_SNIPER | | | | |
| TEAM_MG_ELITE | | | | |
| PARTISAN_SNIPER | | | | |
| PARTISAN_TEAM | | | | |