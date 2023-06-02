/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_VARIABLES_H
#define GAME_VARIABLES_H
#undef GAME_VARIABLES_H // this file will be included several times


// client
MACRO_CONFIG_INT(ClPredict, cl_predict, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Predict client movements")
MACRO_CONFIG_INT(ClNameplates, cl_nameplates, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show name plates")
MACRO_CONFIG_INT(ClNameplatesAlways, cl_nameplates_always, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Always show name plates disregarding of distance")
MACRO_CONFIG_INT(ClNameplatesTeamcolors, cl_nameplates_teamcolors, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Use team colors for name plates")
MACRO_CONFIG_INT(ClNameplatesSize, cl_nameplates_size, 50, 0, 100, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Size of the name plates from 0 to 100%")
MACRO_CONFIG_INT(ClAutoswitchWeapons, cl_autoswitch_weapons, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Auto switch weapon on pickup")

MACRO_CONFIG_INT(ClShowhud, cl_showhud, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show ingame HUD")
MACRO_CONFIG_INT(ClShowChatFriends, cl_show_chat_friends, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show only chat messages from friends")
MACRO_CONFIG_INT(ClShowfps, cl_showfps, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show ingame FPS counter")

MACRO_CONFIG_INT(ClAirjumpindicator, cl_airjumpindicator, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "")
MACRO_CONFIG_INT(ClThreadsoundloading, cl_threadsoundloading, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Load sound files threaded")

MACRO_CONFIG_INT(ClWarningTeambalance, cl_warning_teambalance, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Warn about team balance")

MACRO_CONFIG_INT(ClMouseDeadzone, cl_mouse_deadzone, 300, 0, 0, CFGFLAG_CLIENT|CFGFLAG_SAVE, "")
MACRO_CONFIG_INT(ClMouseFollowfactor, cl_mouse_followfactor, 60, 0, 200, CFGFLAG_CLIENT|CFGFLAG_SAVE, "")
MACRO_CONFIG_INT(ClMouseMaxDistance, cl_mouse_max_distance, 800, 0, 0, CFGFLAG_CLIENT|CFGFLAG_SAVE, "")

MACRO_CONFIG_INT(EdShowkeys, ed_showkeys, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "")

//MACRO_CONFIG_INT(ClFlow, cl_flow, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "")

MACRO_CONFIG_INT(ClShowWelcome, cl_show_welcome, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "")
MACRO_CONFIG_INT(ClMotdTime, cl_motd_time, 10, 0, 100, CFGFLAG_CLIENT|CFGFLAG_SAVE, "How long to show the server message of the day")

MACRO_CONFIG_STR(ClVersionServer, cl_version_server, 100, "version.teeworlds.com", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Server to use to check for new versions")

MACRO_CONFIG_STR(ClLanguagefile, cl_languagefile, 255, "", CFGFLAG_CLIENT|CFGFLAG_SAVE, "What language file to use")

MACRO_CONFIG_INT(PlayerUseCustomColor, player_use_custom_color, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Toggles usage of custom colors")
MACRO_CONFIG_INT(PlayerColorBody, player_color_body, 65408, 0, 0xFFFFFF, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Player body color")
MACRO_CONFIG_INT(PlayerColorFeet, player_color_feet, 65408, 0, 0xFFFFFF, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Player feet color")
MACRO_CONFIG_STR(PlayerSkin, player_skin, 24, "default", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Player skin")

MACRO_CONFIG_INT(UiPage, ui_page, 6, 0, 10, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface page")
MACRO_CONFIG_INT(UiToolboxPage, ui_toolbox_page, 0, 0, 2, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Toolbox page")
MACRO_CONFIG_STR(UiServerAddress, ui_server_address, 64, "localhost:8303", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface server address")
MACRO_CONFIG_INT(UiScale, ui_scale, 100, 50, 150, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface scale")
MACRO_CONFIG_INT(UiMousesens, ui_mousesens, 100, 5, 100000, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Mouse sensitivity for menus/editor")

MACRO_CONFIG_INT(UiColorHue, ui_color_hue, 160, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface color hue")
MACRO_CONFIG_INT(UiColorSat, ui_color_sat, 70, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface color saturation")
MACRO_CONFIG_INT(UiColorLht, ui_color_lht, 175, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface color lightness")
MACRO_CONFIG_INT(UiColorAlpha, ui_color_alpha, 228, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface alpha")

MACRO_CONFIG_INT(GfxNoclip, gfx_noclip, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Disable clipping")

// server
MACRO_CONFIG_INT(SvWarmup, sv_warmup, 0, 0, 0, CFGFLAG_SERVER, "Number of seconds to do warmup before round starts")
MACRO_CONFIG_STR(SvMotd, sv_motd, 900, "", CFGFLAG_SERVER, "Message of the day to display for the clients")
MACRO_CONFIG_INT(SvTeamdamage, sv_teamdamage, 0, 0, 1, CFGFLAG_SERVER, "Team damage")
MACRO_CONFIG_STR(SvMaprotation, sv_maprotation, 768, "", CFGFLAG_SERVER, "Maps to rotate between")
MACRO_CONFIG_INT(SvRoundsPerMap, sv_rounds_per_map, 1, 1, 100, CFGFLAG_SERVER, "Number of rounds on each map before rotating")
MACRO_CONFIG_INT(SvRoundSwap, sv_round_swap, 1, 0, 1, CFGFLAG_SERVER, "Swap teams between rounds")
MACRO_CONFIG_INT(SvPowerups, sv_powerups, 1, 0, 1, CFGFLAG_SERVER, "Allow powerups like ninja")
MACRO_CONFIG_INT(SvScorelimit, sv_scorelimit, 0, 0, 1000, CFGFLAG_SERVER, "Score limit (0 disables)")
MACRO_CONFIG_INT(SvTimelimit, sv_timelimit, 4, 0, 1000, CFGFLAG_SERVER, "Time limit in minutes (0 disables)")
MACRO_CONFIG_STR(SvGametype, sv_gametype, 32, "alien", CFGFLAG_SERVER, "Game type (dm, tdm, ctf)")
MACRO_CONFIG_INT(SvTournamentMode, sv_tournament_mode, 0, 0, 1, CFGFLAG_SERVER, "Tournament mode. When enabled, players joins the server as spectator")
MACRO_CONFIG_INT(SvSpamprotection, sv_spamprotection, 1, 0, 1, CFGFLAG_SERVER, "Spam protection")

MACRO_CONFIG_INT(SvRespawnDelayTDM, sv_respawn_delay_tdm, 3, 0, 10, CFGFLAG_SERVER, "Time needed to respawn after death in tdm gametype")

MACRO_CONFIG_INT(SvSpectatorSlots, sv_spectator_slots, 0, 0, MAX_CLIENTS, CFGFLAG_SERVER, "Number of slots to reserve for spectators")
MACRO_CONFIG_INT(SvTeambalanceTime, sv_teambalance_time, 1, 0, 1000, CFGFLAG_SERVER, "How many minutes to wait before autobalancing teams")
MACRO_CONFIG_INT(SvInactiveKickTime, sv_inactivekick_time, 3, 0, 1000, CFGFLAG_SERVER, "How many minutes to wait before taking care of inactive players")
MACRO_CONFIG_INT(SvInactiveKick, sv_inactivekick, 1, 0, 2, CFGFLAG_SERVER, "How to deal with inactive players (0=move to spectator, 1=move to free spectator slot/kick, 2=kick)")

MACRO_CONFIG_INT(SvStrictSpectateMode, sv_strict_spectate_mode, 0, 0, 1, CFGFLAG_SERVER, "Restricts information in spectator mode")
MACRO_CONFIG_INT(SvVoteSpectate, sv_vote_spectate, 1, 0, 1, CFGFLAG_SERVER, "Allow voting to move players to spectators")
MACRO_CONFIG_INT(SvVoteSpectateRejoindelay, sv_vote_spectate_rejoindelay, 3, 0, 1000, CFGFLAG_SERVER, "How many minutes to wait before a player can rejoin after being moved to spectators by vote")
MACRO_CONFIG_INT(SvVoteKick, sv_vote_kick, 1, 0, 1, CFGFLAG_SERVER, "Allow voting to kick players")
MACRO_CONFIG_INT(SvVoteKickMin, sv_vote_kick_min, 0, 0, MAX_CLIENTS, CFGFLAG_SERVER, "Minimum number of players required to start a kick vote")
MACRO_CONFIG_INT(SvVoteKickBantime, sv_vote_kick_bantime, 5, 0, 1440, CFGFLAG_SERVER, "The time to ban a player if kicked by vote. 0 makes it just use kick")
MACRO_CONFIG_INT(SvPreStartDelay, sv_prestartdelay, 10,1,10000, CFGFLAG_SERVER, "Warmup-phase at start")
MACRO_CONFIG_INT(SvSingularityRangeW, sv_singularityrange_w, 800, 1, 9999, CFGFLAG_SERVER, "Weapon singularity range")
MACRO_CONFIG_INT(SvSingularityRangeS, sv_singularityrange_s, 1600, 1, 9999, CFGFLAG_SERVER, "Mapentity singularity range")
MACRO_CONFIG_INT(SvSingularityKillRange, sv_singularitykillrange, 40, 1, 1000, CFGFLAG_SERVER, "Deathzone of singularity")
MACRO_CONFIG_INT(SvSingularinatorTTL, sv_singularinatorttl,10,1,9999,CFGFLAG_SERVER, "Singularity TTL of Singularinator")
MACRO_CONFIG_INT(SvMinScorePlayers, sv_minscoreplayers,1,1,16,CFGFLAG_SERVER, "Minimum of players before high-score would be written.")
MACRO_CONFIG_STR(SvHighScorePath, sv_highscorepath, 2048, "./", CFGFLAG_SERVER, "Pathname for high-score files.")
MACRO_CONFIG_STR(SvStartMsgA, sv_startmsg_a, 128, "所以那又开始了...", CFGFLAG_SERVER, "Startmsg 1 for humans")
MACRO_CONFIG_STR(SvStartMsgB, sv_startmsg_b, 128, "一场突变的灾难...", CFGFLAG_SERVER, "Startmsg 2 for humans")
MACRO_CONFIG_STR(SvStartMsgC, sv_startmsg_c, 128, "这里还有希望吗?", CFGFLAG_SERVER, "Startmsg 3 for humans")
MACRO_CONFIG_STR(SvStartMsgD, sv_startmsg_d, 128, "救救我们!", CFGFLAG_SERVER, "Startmsg 4 for humans")
MACRO_CONFIG_INT(SvMutEmoteDelay, sv_mutemotedelay,20,5,9999,CFGFLAG_SERVER, "Base-delay of random protecting mutantic eye flashing (+10 secs rand)")
MACRO_CONFIG_INT(SvSGTurretNonExpDmg, sv_sgturret_nonexpdmg,2,0,4,CFGFLAG_SERVER, "Damage of stationary shotgun-turret (final_sum = +1 for exploding_dmg)")
MACRO_CONFIG_INT(SvAirStrikeTTL, sv_airstrikettl,1500,1,9999,CFGFLAG_SERVER, "TTL for airstrike grenades")
MACRO_CONFIG_INT(SvInitAirStrike, sv_initairstrike,1,0,1,CFGFLAG_SERVER, "Do random airstrike at start")
MACRO_CONFIG_INT(SvClusters, sv_clusters,6,0,64,CFGFLAG_SERVER, "How many clusters per grenade")
MACRO_CONFIG_INT(SvAirStrikeClusters, sv_airstrikeclusters,6,0,24,CFGFLAG_SERVER, "How many clusters per human airstrike-grenade")
MACRO_CONFIG_INT(SvAlicaAirStrikeStep, sv_alica_airstrikestep,128,32,9999,CFGFLAG_SERVER, "Size of space between bomb-carpet of alica")
MACRO_CONFIG_INT(SvHumAirStrikeStep, sv_hum_airstrikestep,128,32,9999,CFGFLAG_SERVER, "Size of space between bomb-carpet of humans")
MACRO_CONFIG_INT(SvReaperAmmo, sv_reaperammo,2,0,10,CFGFLAG_SERVER, "How many singularities at reaper start")
MACRO_CONFIG_INT(SvHealingAmmo, sv_healingammo,3,0,10,CFGFLAG_SERVER, "How many healing laser ammo")
MACRO_CONFIG_INT(SvGunMK2Force, sv_gunmk2force,20,0,9999,CFGFLAG_SERVER, "How many force for gun mk2")
MACRO_CONFIG_INT(SvShotMK2Force, sv_shotmk2force,10,0,9999,CFGFLAG_SERVER, "How many force for shotgun mk2")
MACRO_CONFIG_INT(SvShotSpeedkit, sv_shotspeedkit,700,250,2000,CFGFLAG_SERVER, "Shotgun-Speedkit reload delay")
MACRO_CONFIG_INT(SvScienceSuccessCount, sv_sciencesuccesscount,10,0,9999,CFGFLAG_SERVER, "How many shields for research (0 to disable)")
MACRO_CONFIG_INT(SvSaveScienceScore, sv_savesciencescore,0,0,9999,CFGFLAG_SERVER, "Save double researches after round at min score (0 to disable)")
MACRO_CONFIG_INT(SvScienceSpawn, sv_sciencespawn,400,0,2000,CFGFLAG_SERVER, "Time in msecs until hammer respawns at research")
MACRO_CONFIG_INT(SvSkipScienceStations, sv_skipsciencestations,0,0,1,CFGFLAG_SERVER, "Remove science stations when map is loaded")
MACRO_CONFIG_INT(SvMaxTurretAimVel, sv_maxturretaimvel,10,0,9999,CFGFLAG_SERVER, "Max movement speed before turret will fire on tee")
MACRO_CONFIG_INT(SvTurretAimNoHumans, sv_turretaimnohumans,0,0,1,CFGFLAG_SERVER, "Laserturrets should just aim mutants")
MACRO_CONFIG_INT(SvTurretRange, sv_turretrange,500,0,1000,CFGFLAG_SERVER, "Range of laserturrets")
MACRO_CONFIG_INT(SvLaserWallDelay, sv_laserwalldelay, 1, 0, 5, CFGFLAG_SERVER, "Delay before laserwall would be activated")
MACRO_CONFIG_INT(SvLaserWallLength, sv_laserwalllength, 266, 0, 9999, CFGFLAG_SERVER, "Length for one hit laserwall")
MACRO_CONFIG_INT(SvNormalBattery, sv_normalbattery,3000,1,9999,CFGFLAG_SERVER, "Time that one shield keep laserwall on power")
MACRO_CONFIG_INT(SvSuperBattery, sv_superbattery,4500,1,9999,CFGFLAG_SERVER, "Time that one shield keep laserwall on power")
MACRO_CONFIG_INT(SvLWLimit, sv_lw_limit,0,0,1,CFGFLAG_SERVER, "Limit TTL of laserwall to 10 x batterypower")
MACRO_CONFIG_INT(SvLWLimitNonAlMap, sv_lw_limit_non_al_map,1,0,1,CFGFLAG_SERVER, "Limit TTL of laserwall to 10 x batterypower in non-alien maps")
MACRO_CONFIG_INT(SvHeroAirstrike, sv_heroairstrike,0,0,1,CFGFLAG_SERVER, "Give airstrike when player get hero")
MACRO_CONFIG_INT(SvReapinatorDisabled, sv_reapinator_disabled,0,0,1,CFGFLAG_SERVER, "Disable reapinator")
MACRO_CONFIG_INT(SvReapinatorDelay, sv_reapinator_delay,120,1,9999,CFGFLAG_SERVER, "Init delay of reapinator (in secs)")
MACRO_CONFIG_INT(SvReapinatorMinScore, sv_reapinator_minscore,5,0,9999,CFGFLAG_SERVER, "Min player score for reapinator")
MACRO_CONFIG_INT(SvReaperMinScore, sv_reaper_minscore,6,0,9999,CFGFLAG_SERVER, "Min player score for reaper (without reapinator)")
MACRO_CONFIG_INT(SvMetamorphoseCount, sv_metamorphosecount,1,0,9999,CFGFLAG_SERVER, "How many kills until metamorphose")
MACRO_CONFIG_INT(SvHeroBecameScore, sv_becamehero_score,15,0,9999,CFGFLAG_SERVER, "Score to became hero")
MACRO_CONFIG_INT(SvAutoResearchScore, sv_autoresearchscore, 3,0,9999,CFGFLAG_SERVER, "BonusScore to init research lottery - (0 to disable)")
MACRO_CONFIG_INT(SvEmptyResearch, sv_emptyresearch, 1,0,1,CFGFLAG_SERVER, "Auto research only on maps without hammer pickup")
MACRO_CONFIG_STR(SvInfoGot, sv_infogot, 64, "得到:", CFGFLAG_SERVER, "Autoresearch text success text")
MACRO_CONFIG_STR(SvInfoResearched, sv_inforesearched, 64, "研究到:", CFGFLAG_SERVER, "Manual hammer research success text")
MACRO_CONFIG_STR(SvInfoHackedAirstrike, sv_hackedairstrike, 64, "黑进了 Alica-AI 并得到了空袭!", CFGFLAG_SERVER, "Became airstrike text at research")
MACRO_CONFIG_STR(SvInfoGotAirstrike, sv_gotairstrike, 64, "你得到了空袭机会!", CFGFLAG_SERVER, "Became airstrike text no-research")
MACRO_CONFIG_INT(SvScepticMsgOn, sv_scepticmsg_on, 1,0,1,CFGFLAG_SERVER, "Enable energynator sceptic msg.")
MACRO_CONFIG_INT(SvHappyMsgOn, sv_happymsg_on, 1,0,1,CFGFLAG_SERVER, "Enable energynator sceptic msg.")
MACRO_CONFIG_INT(SvBonusMsgOn, sv_bonusmsg_on, 1,0,1,CFGFLAG_SERVER, "Enable generic research and metamorph-messages.")
MACRO_CONFIG_STR(SvScepticEnergynatorA, sv_scepticenergynator_a, 128, "发电机: 你是个突变体, 不是吗?", CFGFLAG_SERVER, "Sceptic energynator text 1")
MACRO_CONFIG_STR(SvScepticEnergynatorB, sv_scepticenergynator_b, 128, "发电机: 启动修复... 嘿等会!", CFGFLAG_SERVER, "Sceptic energynator text 2")
MACRO_CONFIG_STR(SvScepticEnergynatorC, sv_scepticenergynator_c, 128, "发电机: 滚开可怕的突变体!", CFGFLAG_SERVER, "Sceptic energynator text 3")
MACRO_CONFIG_STR(SvScepticEnergynatorD, sv_scepticenergynator_d, 128, "发电机: (笑)", CFGFLAG_SERVER, "Sceptic energynator text 4")
MACRO_CONFIG_STR(SvScepticEnergynatorE, sv_scepticenergynator_e, 128, "发电机: 救命!!! 我要被突变体摧毁了!", CFGFLAG_SERVER, "Sceptic energynator text 5")
MACRO_CONFIG_STR(SvHappyReapinatorA, sv_happyreapinator_a, 128, "收割者: 你好啊死神突变体. 我是你忠实的粉丝 (别告诉发电机)", CFGFLAG_SERVER, "Happy reapinator text 1")
MACRO_CONFIG_STR(SvHappyReapinatorB, sv_happyreapinator_b, 128, "收割者: 提示:如果你的眼睛在闪烁，你几乎是坚不可摧的!", CFGFLAG_SERVER, "Happy reapinator text 2")
MACRO_CONFIG_STR(SvHappyReapinatorC, sv_happyreapinator_c, 128, "收割者: 我爱你!", CFGFLAG_SERVER, "Happy reapinator text 3")
MACRO_CONFIG_STR(SvHappyReapinatorD, sv_happyreapinator_d, 128, "收割者: 你太太太酷了!", CFGFLAG_SERVER, "Happy reapinator text 4")
MACRO_CONFIG_STR(SvHappyReapinatorE, sv_happyreapinator_e, 128, "收割者: 一个真正的死神!", CFGFLAG_SERVER, "Happy reapinator test 5")
MACRO_CONFIG_INT(SurviveBonus, sv_survivebonus, 3,0,9999,CFGFLAG_SERVER, "Score bonus for survive at game-end.")
MACRO_CONFIG_INT(SvDummy, sv_dummy, 1,0,1,CFGFLAG_SERVER, "Useless empty dummy for vote")



// debug
#ifdef CONF_DEBUG // this one can crash the server if not used correctly
	MACRO_CONFIG_INT(DbgDummies, dbg_dummies, 0, 0, 15, CFGFLAG_SERVER, "")
#endif

MACRO_CONFIG_INT(DbgFocus, dbg_focus, 0, 0, 1, CFGFLAG_CLIENT, "")
MACRO_CONFIG_INT(DbgTuning, dbg_tuning, 0, 0, 1, CFGFLAG_CLIENT, "")
#endif
