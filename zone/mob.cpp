/*	EQEMu: Everquest Server Emulator
	Copyright (C) 2001-2016 EQEMu Development Team (http://eqemu.org)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE. See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include "../common/spdat.h"
#include "../common/string_util.h"

#include "quest_parser_collection.h"
#include "string_ids.h"
#include "worldserver.h"

#include <limits.h>
#include <math.h>
#include <sstream>
#include <algorithm>

#ifdef BOTS
#include "bot.h"
#endif

extern EntityList entity_list;

extern Zone* zone;
extern WorldServer worldserver;

Mob::Mob(const char* in_name,
		const char* in_lastname,
		int32		in_cur_hp,
		int32		in_max_hp,
		uint8		in_gender,
		uint16		in_race,
		uint8		in_class,
		bodyType	in_bodytype,
		uint8		in_deity,
		uint8		in_level,
		uint32		in_npctype_id,
		float		in_size,
		float		in_runspeed,
		const glm::vec4& position,
		uint8		in_light,
		uint8		in_texture,
		uint8		in_helmtexture,
		uint16		in_ac,
		uint16		in_atk,
		uint16		in_str,
		uint16		in_sta,
		uint16		in_dex,
		uint16		in_agi,
		uint16		in_int,
		uint16		in_wis,
		uint16		in_cha,
		uint8		in_haircolor,
		uint8		in_beardcolor,
		uint8		in_eyecolor1, // the eyecolors always seem to be the same, maybe left and right eye?
		uint8		in_eyecolor2,
		uint8		in_hairstyle,
		uint8		in_luclinface,
		uint8		in_beard,
		uint32		in_drakkin_heritage,
		uint32		in_drakkin_tattoo,
		uint32		in_drakkin_details,
		uint32		in_armor_tint[_MaterialCount],

		uint8		in_aa_title,
		uint8		in_see_invis, // see through invis/ivu
		uint8		in_see_invis_undead,
		uint8		in_see_hide,
		uint8		in_see_improved_hide,
		int32		in_hp_regen,
		int32		in_mana_regen,
		uint8		in_qglobal,
		uint8		in_maxlevel,
		uint32		in_scalerate,
		uint8		in_armtexture,
		uint8		in_bracertexture,
		uint8		in_handtexture,
		uint8		in_legtexture,
		uint8		in_feettexture
		) :
		attack_timer(2000),
		attack_dw_timer(2000),
		ranged_timer(2000),
		tic_timer(6000),
		fast_tic_timer(1000), //C!Kayen
		mana_timer(2000),
		spellend_timer(0),
		rewind_timer(30000), //Timer used for determining amount of time between actual player position updates for /rewind.
		bindwound_timer(10000),
		stunned_timer(0),
		spun_timer(0),
		bardsong_timer(6000),
		gravity_timer(1000),
		viral_timer(0),
		m_FearWalkTarget(-999999.0f,-999999.0f,-999999.0f),
		m_TargetLocation(glm::vec3()),
		m_TargetV(glm::vec3()),
		flee_timer(FLEE_CHECK_TIMER),
		m_Position(position)
{
	targeted = 0;
	tar_ndx=0;
	tar_vector=0;
	currently_fleeing = false;

	AI_Init();
	SetMoving(false);
	moved=false;
	m_RewindLocation = glm::vec3();
	move_tic_count = 0;

	_egnode = nullptr;
	name[0]=0;
	orig_name[0]=0;
	clean_name[0]=0;
	lastname[0]=0;
	if(in_name) {
		strn0cpy(name,in_name,64);
		strn0cpy(orig_name,in_name,64);
	}
	if(in_lastname)
		strn0cpy(lastname,in_lastname,64);
	cur_hp		= in_cur_hp;
	max_hp		= in_max_hp;
	base_hp		= in_max_hp;
	gender		= in_gender;
	race		= in_race;
	base_gender	= in_gender;
	base_race	= in_race;
	class_		= in_class;
	bodytype	= in_bodytype;
	orig_bodytype = in_bodytype;
	deity		= in_deity;
	level		= in_level;
	orig_level = in_level;
	npctype_id	= in_npctype_id;
	size		= in_size;
	base_size	= size;
	runspeed	= in_runspeed;
	// neotokyo: sanity check
	if (runspeed < 0 || runspeed > 20)
		runspeed = 1.25f;
	base_runspeed = (int)((float)runspeed * 40.0f);
	// clients
	if (runspeed == 0.7f) {
		base_runspeed = 28;
		walkspeed = 0.3f;
		base_walkspeed = 12;
		fearspeed = 0.625f;
		base_fearspeed = 25;
		// npcs
	} else {
		base_walkspeed = base_runspeed * 100 / 265;
		walkspeed = ((float)base_walkspeed) * 0.025f;
		base_fearspeed = base_runspeed * 100 / 127;
		fearspeed = ((float)base_fearspeed) * 0.025f;
	}


	current_speed = base_runspeed;

	m_PlayerState	= 0;


	// sanity check
	if (runspeed < 0 || runspeed > 20)
		runspeed = 1.25f;

	m_Light.Type.Innate = in_light;
	m_Light.Level.Innate = m_Light.TypeToLevel(m_Light.Type.Innate);
	m_Light.Level.Equipment = m_Light.Type.Equipment = 0;
	m_Light.Level.Spell = m_Light.Type.Spell = 0;
	m_Light.Type.Active = m_Light.Type.Innate;
	m_Light.Level.Active = m_Light.Level.Innate;

	texture		= in_texture;
	helmtexture	= in_helmtexture;
	armtexture = in_armtexture;
	bracertexture = in_bracertexture;
	handtexture = in_handtexture;
	legtexture = in_legtexture;
	feettexture = in_feettexture;
	multitexture = (armtexture || bracertexture || handtexture || legtexture || feettexture);

	haircolor	= in_haircolor;
	beardcolor	= in_beardcolor;
	eyecolor1	= in_eyecolor1;
	eyecolor2	= in_eyecolor2;
	hairstyle	= in_hairstyle;
	luclinface	= in_luclinface;
	beard		= in_beard;
	drakkin_heritage	= in_drakkin_heritage;
	drakkin_tattoo		= in_drakkin_tattoo;
	drakkin_details		= in_drakkin_details;
	attack_speed = 0;
	attack_delay = 0;
	slow_mitigation = 0;
	findable	= false;
	trackable	= true;
	has_shieldequiped = false;
	has_twohandbluntequiped = false;
	has_twohanderequipped = false;
	has_numhits = false;
	has_MGB = false;
	has_ProjectIllusion = false;
	SpellPowerDistanceMod = 0;
	last_los_check = false;

	if(in_aa_title>0)
		aa_title	= in_aa_title;
	else
		aa_title	=0xFF;
	AC		= in_ac;
	ATK		= in_atk;
	STR		= in_str;
	STA		= in_sta;
	DEX		= in_dex;
	AGI		= in_agi;
	INT		= in_int;
	WIS		= in_wis;
	CHA		= in_cha;
	MR = CR = FR = DR = PR = Corrup = 0;

	ExtraHaste = 0;
	bEnraged = false;

	shield_target = nullptr;
	cur_mana = 0;
	max_mana = 0;
	hp_regen = in_hp_regen;
	mana_regen = in_mana_regen;
	oocregen = RuleI(NPC, OOCRegen); //default Out of Combat Regen
	maxlevel = in_maxlevel;
	scalerate = in_scalerate;
	invisible = false;
	invisible_undead = false;
	invisible_animals = false;
	sneaking = false;
	hidden = false;
	improved_hidden = false;
	invulnerable = false;
	IsFullHP	= (cur_hp == max_hp);
	qglobal=0;

	InitializeBuffSlots();

	// clear the proc arrays
	int i;
	int j;
	for (j = 0; j < MAX_PROCS; j++)
	{
		PermaProcs[j].spellID = SPELL_UNKNOWN;
		PermaProcs[j].chance = 0;
		PermaProcs[j].base_spellID = SPELL_UNKNOWN;
		PermaProcs[j].level_override = -1;
		SpellProcs[j].spellID = SPELL_UNKNOWN;
		SpellProcs[j].chance = 0;
		SpellProcs[j].base_spellID = SPELL_UNKNOWN;
		SpellProcs[j].level_override = -1;
		DefensiveProcs[j].spellID = SPELL_UNKNOWN;
		DefensiveProcs[j].chance = 0;
		DefensiveProcs[j].base_spellID = SPELL_UNKNOWN;
		DefensiveProcs[j].level_override = -1;
		RangedProcs[j].spellID = SPELL_UNKNOWN;
		RangedProcs[j].chance = 0;
		RangedProcs[j].base_spellID = SPELL_UNKNOWN;
		RangedProcs[j].level_override = -1;
	}

	for (i = 0; i < _MaterialCount; i++)
	{
		if (in_armor_tint)
		{
			armor_tint[i] = in_armor_tint[i];
		}
		else
		{
			armor_tint[i] = 0;
		}
	}

	m_Delta = glm::vec4();
	animation = 0;

	logging_enabled = false;
	isgrouped = false;
	israidgrouped = false;
	entity_id_being_looted = 0;
	_appearance = eaStanding;
	pRunAnimSpeed = 0;

	spellend_timer.Disable();
	bardsong_timer.Disable();
	bardsong = 0;
	bardsong_target_id = 0;
	casting_spell_id = 0;
	casting_spell_timer = 0;
	casting_spell_timer_duration = 0;
	casting_spell_inventory_slot = 0;
	casting_spell_aa_id = 0;
	target = 0;

	ActiveProjectileATK = false;
	for (int i = 0; i < MAX_SPELL_PROJECTILE; i++)
	{
		ProjectileAtk[i].increment = 0;
		ProjectileAtk[i].hit_increment = 0;
		ProjectileAtk[i].target_id = 0;
		ProjectileAtk[i].wpn_dmg = 0;
		ProjectileAtk[i].origin_x = 0.0f;
		ProjectileAtk[i].origin_y = 0.0f;
		ProjectileAtk[i].origin_z = 0.0f;
		ProjectileAtk[i].tlast_x = 0.0f;
		ProjectileAtk[i].tlast_y = 0.0f;
		ProjectileAtk[i].ranged_id = 0;
		ProjectileAtk[i].ammo_id = 0;
		ProjectileAtk[i].ammo_slot = 0;
		ProjectileAtk[i].skill = 0;
		ProjectileAtk[i].speed_mod = 0.0f;
		ProjectileAtk[i].spell_id = SPELL_UNKNOWN; //C!Kayen
		ProjectileAtk[i].dmod = 0; //C!Kayen
		ProjectileAtk[i].dmgpct = 0; //C!Kayen
	}

	memset(&itembonuses, 0, sizeof(StatBonuses));
	memset(&spellbonuses, 0, sizeof(StatBonuses));
	memset(&aabonuses, 0, sizeof(StatBonuses));
	spellbonuses.AggroRange = -1;
	spellbonuses.AssistRange = -1;
	pLastChange = 0;
	SetPetID(0);
	SetOwnerID(0);
	typeofpet = petNone; // default to not a pet
	petpower = 0;
	held = false;
	nocast = false;
	focused = false;
	_IsTempPet = false;
	pet_owner_client = false;
	pet_targetlock_id = 0;

	attacked_count = 0;
	mezzed = false;
	stunned = false;
	silenced = false;
	amnesiad = false;
	inWater = false;
	int m;
	for (m = 0; m < MAX_SHIELDERS; m++)
	{
		shielder[m].shielder_id = 0;
		shielder[m].shielder_bonus = 0;
	}

	destructibleobject = false;
	wandertype=0;
	pausetype=0;
	cur_wp = 0;
	m_CurrentWayPoint = glm::vec4();
	cur_wp_pause = 0;
	patrol=0;
	follow=0;
	follow_dist = 100;	// Default Distance for Follow
	flee_mode = false;
	currently_fleeing = false;
	flee_timer.Start();

	permarooted = (runspeed > 0) ? false : true;

	movetimercompleted = false;
	roamer = false;
	rooted = false;
	charmed = false;
	has_virus = false;
	for (i=0; i<MAX_SPELL_TRIGGER*2; i++) {
		viral_spells[i] = 0;
	}
	pStandingPetOrder = SPO_Follow;
	pseudo_rooted = false;

	see_invis = in_see_invis;
	see_invis_undead = in_see_invis_undead != 0;
	see_hide = in_see_hide != 0;
	see_improved_hide = in_see_improved_hide != 0;
	qglobal = in_qglobal != 0;

	// Bind wound
	bindwound_timer.Disable();
	bindwound_target = 0;

	trade = new Trade(this);
	// hp event
	nexthpevent = -1;
	nextinchpevent = -1;

	hasTempPet = false;
	count_TempPet = 0;

	m_is_running = false;

	nimbus_effect1 = 0;
	nimbus_effect2 = 0;
	nimbus_effect3 = 0;
	m_targetable = true;

    m_TargetRing = glm::vec3();

	flymode = FlyMode3;
	// Pathing
	PathingLOSState = UnknownLOS;
	PathingLoopCount = 0;
	PathingLastNodeVisited = -1;
	PathingLOSCheckTimer = new Timer(RuleI(Pathing, LOSCheckFrequency));
	PathingRouteUpdateTimerShort = new Timer(RuleI(Pathing, RouteUpdateFrequencyShort));
	PathingRouteUpdateTimerLong = new Timer(RuleI(Pathing, RouteUpdateFrequencyLong));
	DistractedFromGrid = false;
	PathingTraversedNodes = 0;
	hate_list.SetHateOwner(this);

	m_AllowBeneficial = false;
	m_DisableMelee = false;
	for (int i = 0; i < HIGHEST_SKILL+2; i++) { SkillDmgTaken_Mod[i] = 0; }
	for (int i = 0; i < HIGHEST_RESIST+2; i++) { Vulnerability_Mod[i] = 0; }

	emoteid = 0;
	endur_upkeep = false;

	PrimaryAggro = false;
	AssistAggro = false;
	npc_assist_cap = 0;
	
	//C!Kayen
	momentum = 0.0f; 
	for (int i = 0; i < HIGHEST_SKILL+2; i++) { WpnSkillDmgBonus[i] = 0; }
	for (int i = 0; i < HIGHEST_RESIST+2; i++) { SpellResistTypeDmgBonus[i] = 0; }

	leap.increment = 0;
	leap.spell_id = SPELL_UNKNOWN;
	leap.origin_x = 0.0f;
	leap.origin_y = 0.0f;
	leap.origin_z = 0.0f;

	leapSE.increment = 0;
	leapSE.spell_id = SPELL_UNKNOWN;
	leapSE.velocity = 0;
	leapSE.dest_x = 0.0f;
	leapSE.dest_y = 0.0f;
	leapSE.dest_z = 0.0f;
	leapSE.dest_h = 0.0f;
	leapSE.z_bound_mod = 0.0f;
	leapSE.mod = 0.0f;

	gflux.increment = 0;
	gflux.time_to_peak = 0;
	gflux.time_to_ground = 0;
	gflux.origin_z = 0;
	gflux.peak = 0;

	for (int i = 0; i < MAX_SPELL_PROJECTILE; i++)
	{
		ProjectileRing[i].increment = 0;
		ProjectileRing[i].hit_increment = 0;
		ProjectileRing[i].target_id = 0;
		ProjectileRing[i].spell_id = SPELL_UNKNOWN;
	}
		
	//for (int i = 0; i < MAX_SPELL_PROJECTILE; i++) {projectile_spell_id_ring[i] = 0; }
	//for (int i = 0; i < MAX_SPELL_PROJECTILE; i++) {projectile_target_id_ring[i] = 0; }
	//for (int i = 0; i < MAX_SPELL_PROJECTILE; i++) {projectile_increment_ring[i] = 0; }
	//for (int i = 0; i < MAX_SPELL_PROJECTILE; i++) {projectile_hit_ring[i] = 0; }

	UtilityTempPetSpellID = 0;
	ActiveProjectileRing = false;
	ProjectileAESpellHitTarget = false;

	MeleeChargeActive = false;
	MeleeCharge_target_id = 0;
	CastFromCrouchInterval = 0;
	CastFromCrouchIntervalProj = 0;
	casting_z_diff = 0;

	DisableTargetSpellAnimation = false;
	SpellPowerAmtHits = 0;
	WizardInnateActive = false;
	cured_count = 0;
	stun_resilience = 0;
	max_stun_resilience = 0;
	hard_MitigateAllDamage = 0;
	OnlyAggroLast = false;
	origin_caster_id = 0;
	AppearanceEffect = false;
	fast_tic_special_count = 0;
	has_fast_buff = false;

	charge_effect = 0;
	charge_effect_increment = 0;

	bravery_recast = 0;

	AI_no_chase = false;
	AE_no_target_found = false;

	use_targetring_override = false;
	disable_spell_effects = false;

	for (int i = 0; i < MAX_POSITION_TYPES + 1; i++) { RakePosition[i] = 0; }

	aeduration_iteration = 0;
	scaled_base_effect_value = 0;
	AggroLockEffect = 0;

	effect_field_timer.Disable();
	aura_field_timer.Disable();
	pet_buff_owner_timer.Disable();
	pet_resume_autofollow.Disable();
	fast_tic_special_timer.Disable();
	stun_resilience_timer.Disable();
	charge_effect_timer.Disable();
	leapSE_timer.Disable();

}

Mob::~Mob()
{
	AI_Stop();
	if (GetPet()) {
		if (GetPet()->Charmed())
			GetPet()->BuffFadeByEffect(SE_Charm);
		else
			SetPet(0);
	}

	EQApplicationPacket app;
	CreateDespawnPacket(&app, !IsCorpse());
	Corpse* corpse = entity_list.GetCorpseByID(GetID());
	if(!corpse || (corpse && !corpse->IsPlayerCorpse()))
		entity_list.QueueClients(this, &app, true);

	entity_list.RemoveFromTargets(this, true);

	if(trade) {
		Mob *with = trade->With();
		if(with && with->IsClient()) {
			with->CastToClient()->FinishTrade(with);
			with->trade->Reset();
		}
		delete trade;
	}

	if(HasTempPetsActive()){
		entity_list.DestroyTempPets(this);
	}
	entity_list.UnMarkNPC(GetID());
	safe_delete(PathingLOSCheckTimer);
	safe_delete(PathingRouteUpdateTimerShort);
	safe_delete(PathingRouteUpdateTimerLong);
	UninitializeBuffSlots();

#ifdef BOTS
	LeaveHealRotationTargetPool();
#endif
}

uint32 Mob::GetAppearanceValue(EmuAppearance iAppearance) {
	switch (iAppearance) {
		// 0 standing, 1 sitting, 2 ducking, 3 lieing down, 4 looting
		case eaStanding: {
			return ANIM_STAND;
		}
		case eaSitting: {
			return ANIM_SIT;
		}
		case eaCrouching: {
			return ANIM_CROUCH;
		}
		case eaDead: {
			return ANIM_DEATH;
		}
		case eaLooting: {
			return ANIM_LOOT;
		}
		//to shup up compiler:
		case _eaMaxAppearance:
			break;
	}
	return(ANIM_STAND);
}

void Mob::SetInvisible(uint8 state)
{
	invisible = state;
	SendAppearancePacket(AT_Invis, invisible);
	// Invis and hide breaks charms

	auto formerpet = GetPet();
	if (formerpet && formerpet->GetPetType() == petCharmed && (invisible || hidden || improved_hidden))
		formerpet->BuffFadeByEffect(SE_Charm);
}

//check to see if `this` is invisible to `other`
bool Mob::IsInvisible(Mob* other) const
{
	if(!other)
		return(false);

	uint8 SeeInvisBonus = 0;
	if (IsClient())
		SeeInvisBonus = aabonuses.SeeInvis;

	//check regular invisibility
	if (invisible && invisible > (other->SeeInvisible()))
		return true;

	//check invis vs. undead
	if (other->GetBodyType() == BT_Undead || other->GetBodyType() == BT_SummonedUndead) {
		if(invisible_undead && !other->SeeInvisibleUndead())
			return true;
	}

	//check invis vs. animals...
	if (other->GetBodyType() == BT_Animal){
		if(invisible_animals && !other->SeeInvisible())
			return true;
	}

	if(hidden){
		if(!other->see_hide && !other->see_improved_hide){
			return true;
		}
	}

	if(improved_hidden){
		if(!other->see_improved_hide){
			return true;
		}
	}

	//handle sneaking
	if(sneaking) {
		if(BehindMob(other, GetX(), GetY()) )
			return true;
	}

	return(false);
}

int Mob::_GetWalkSpeed() const {

	if (IsRooted() || IsStunned() || IsMezzed())
		return 0;

	else if (IsPseudoRooted())
		return 0;

	//speed_mod += GetMomentum(); //C!Kayen 

	int aa_mod = 0;
	int speed_mod = base_walkspeed;
	int base_run = base_runspeed;
	bool has_horse = false;
	int runspeedcap = RuleI(Character,BaseRunSpeedCap);
	runspeedcap += itembonuses.IncreaseRunSpeedCap + spellbonuses.IncreaseRunSpeedCap + aabonuses.IncreaseRunSpeedCap;
	aa_mod += aabonuses.BaseMovementSpeed;

	if (IsClient()) {
		Mob *horse = entity_list.GetMob(CastToClient()->GetHorseId());
		if (horse) {
			speed_mod = horse->GetBaseRunspeed();
			return speed_mod;
		}
	}

	int spell_mod = spellbonuses.movementspeed + itembonuses.movementspeed;
	int movemod = 0;

	if (spell_mod < 0)
		movemod += spell_mod;
	else if (spell_mod > aa_mod)
		movemod = spell_mod;
	else
		movemod = aa_mod;

	// hard cap
	if (runspeedcap > 225)
		runspeedcap = 225;

	if(movemod < -85) //cap it at moving very very slow
		movemod = -85;

	if (!has_horse && movemod != 0)
		speed_mod += (base_run * movemod / 100);

	if(speed_mod < 1)
		return(0);

	//runspeed cap.
	if(IsClient())
	{
		if(speed_mod > runspeedcap)
			speed_mod = runspeedcap;
	}
	return speed_mod;
}

int Mob::_GetRunSpeed() const {
	if (IsRooted() || IsStunned() || IsMezzed() || IsPseudoRooted())
		return 0;

	int aa_mod = 0;
	int speed_mod = base_runspeed;
	int base_walk = base_walkspeed;
	bool has_horse = false;
	if (IsClient())
	{
		if(CastToClient()->GetGMSpeed())
		{
			speed_mod = 325;
		}
		else
		{
			Mob* horse = entity_list.GetMob(CastToClient()->GetHorseId());
			if(horse)
			{
				speed_mod = horse->GetBaseRunspeed();
				base_walk = horse->GetBaseWalkspeed();
				has_horse = true;
			}
		}
	}

	int runspeedcap = RuleI(Character,BaseRunSpeedCap);
	runspeedcap += itembonuses.IncreaseRunSpeedCap + spellbonuses.IncreaseRunSpeedCap + aabonuses.IncreaseRunSpeedCap;

	aa_mod = itembonuses.IncreaseRunSpeedCap + spellbonuses.IncreaseRunSpeedCap + aabonuses.IncreaseRunSpeedCap;
	int spell_mod = spellbonuses.movementspeed + itembonuses.movementspeed;

	if (spell_mod < 0) //C!Kayen - Custom snare modifier
		spell_mod += spell_mod * spellbonuses.ImprovedSnare / 100;

	int movemod = 0;

	if(spell_mod < 0)
	{
		movemod += spell_mod;
	}
	else if(spell_mod > aa_mod)
	{
		movemod = spell_mod;
	}
	else
	{
		movemod = aa_mod;
	}

	if(movemod < -85) //cap it at moving very very slow
		movemod = -85;

	if (!has_horse && movemod != 0)
	{
		if (IsClient())
		{
			speed_mod += (speed_mod * movemod / 100);
		} else {
			if (movemod < 0) {
				speed_mod += (50 * movemod / 100);
				// basically stoped
				if(speed_mod < 1)
				{
					return(0);
				}
				// moving slowly
				if (speed_mod < 8)
					return(8);
			} else {
				speed_mod += GetBaseWalkspeed();
				if (movemod > 50)
					speed_mod += 4;
				if (movemod > 40)
					speed_mod += 3;
			}
		}
	}

	if(speed_mod < 1)
	{
		return(0);
	}
	//runspeed cap.
	if(IsClient())
	{
		if(speed_mod > runspeedcap)
			speed_mod = runspeedcap;
	}
	return speed_mod;
}

int Mob::_GetFearSpeed() const {

	if (IsRooted() || IsStunned() || IsMezzed())
		return 0;

	//float speed_mod = fearspeed;
	int speed_mod = GetBaseFearSpeed();

	// use a max of 1.75f in calcs.
	int base_run = std::min(GetBaseRunspeed(), 70);

	int spell_mod = spellbonuses.movementspeed + itembonuses.movementspeed;
	int movemod = 0;

	if(spell_mod < 0)
	{
		movemod += spell_mod;
	}

	if(movemod < -85) //cap it at moving very very slow
		movemod = -85;

	if (IsClient()) {
		if (CastToClient()->GetRunMode())
			speed_mod = GetBaseRunspeed();
		else
			speed_mod = GetBaseWalkspeed();
		if (movemod < 0)
			return GetBaseWalkspeed();
		speed_mod += (base_run * movemod / 100);
		return speed_mod;
	} else {
		int hp_ratio = GetIntHPRatio();
		// very large snares 50% or higher
		if (movemod < -49)
		{
			if (hp_ratio < 25)
			{
				return (0);
			}
			if (hp_ratio < 50)
				return (8);
			else
				return (12);
		}
		if (hp_ratio < 5) {
			speed_mod = base_walkspeed / 3;
		} else if (hp_ratio < 15) {
			speed_mod = base_walkspeed / 2;
		} else if (hp_ratio < 25) {
			speed_mod = base_walkspeed + 1; // add the +1 so they do the run animation
		} else if (hp_ratio < 50) {
			speed_mod *= 82;
			speed_mod /= 100;
		}
		if (movemod > 0) {
			speed_mod += GetBaseWalkspeed();
			if (movemod > 50)
				speed_mod += 4;
			if (movemod > 40)
				speed_mod += 3;
			return speed_mod;
		}
		else if (movemod < 0) {
			speed_mod += (base_run * movemod / 100);
		}
	}
	if (speed_mod < 1)
		return (0);
	if (speed_mod < 9)
		return (8);
	if (speed_mod < 13)
		return (12);

	return speed_mod;
}

int32 Mob::CalcMaxMana() {
	switch (GetCasterClass()) {
		case 'I':
			max_mana = (((GetINT()/2)+1) * GetLevel()) + spellbonuses.Mana + itembonuses.Mana;
			break;
		case 'W':
			max_mana = (((GetWIS()/2)+1) * GetLevel()) + spellbonuses.Mana + itembonuses.Mana;
			break;
		case 'N':
		default:
			max_mana = 0;
			break;
	}
	if (max_mana < 0) {
		max_mana = 0;
	}

	return max_mana;
}

int32 Mob::CalcMaxHP() {
	max_hp = (base_hp + itembonuses.HP + spellbonuses.HP);
	max_hp += max_hp * ((aabonuses.MaxHPChange + spellbonuses.MaxHPChange + itembonuses.MaxHPChange) / 10000.0f);
	return max_hp;
}

int32 Mob::GetItemHPBonuses() {
	int32 item_hp = 0;
	item_hp = itembonuses.HP;
	item_hp += item_hp * itembonuses.MaxHPChange / 10000;
	return item_hp;
}

int32 Mob::GetSpellHPBonuses() {
	int32 spell_hp = 0;
	spell_hp = spellbonuses.HP;
	spell_hp += spell_hp * spellbonuses.MaxHPChange / 10000;
	return spell_hp;
}

char Mob::GetCasterClass() const {
	switch(class_)
	{
	case CLERIC:
	case PALADIN:
	case RANGER:
	case DRUID:
	case SHAMAN:
	case BEASTLORD:
	case CLERICGM:
	case PALADINGM:
	case RANGERGM:
	case DRUIDGM:
	case SHAMANGM:
	case BEASTLORDGM:
		return 'W';
		break;

	case SHADOWKNIGHT:
	case BARD:
	case NECROMANCER:
	case WIZARD:
	case MAGICIAN:
	case ENCHANTER:
	case SHADOWKNIGHTGM:
	case BARDGM:
	case NECROMANCERGM:
	case WIZARDGM:
	case MAGICIANGM:
	case ENCHANTERGM:
		return 'I';
		break;

	default:
		return 'N';
		break;
	}
}

uint8 Mob::GetArchetype() const {
	switch(class_)
	{
	case PALADIN:
	case RANGER:
	case SHADOWKNIGHT:
	case BARD:
	case BEASTLORD:
	case PALADINGM:
	case RANGERGM:
	case SHADOWKNIGHTGM:
	case BARDGM:
	case BEASTLORDGM:
		return ARCHETYPE_HYBRID;
		break;
	case CLERIC:
	case DRUID:
	case SHAMAN:
	case NECROMANCER:
	case WIZARD:
	case MAGICIAN:
	case ENCHANTER:
	case CLERICGM:
	case DRUIDGM:
	case SHAMANGM:
	case NECROMANCERGM:
	case WIZARDGM:
	case MAGICIANGM:
	case ENCHANTERGM:
		return ARCHETYPE_CASTER;
		break;
	case WARRIOR:
	case MONK:
	case ROGUE:
	case BERSERKER:
	case WARRIORGM:
	case MONKGM:
	case ROGUEGM:
	case BERSERKERGM:
		return ARCHETYPE_MELEE;
		break;
	default:
		return ARCHETYPE_HYBRID;
		break;
	}
}

void Mob::CreateSpawnPacket(EQApplicationPacket* app, Mob* ForWho) {
	app->SetOpcode(OP_NewSpawn);
	app->size = sizeof(NewSpawn_Struct);
	app->pBuffer = new uchar[app->size];
	memset(app->pBuffer, 0, app->size);
	NewSpawn_Struct* ns = (NewSpawn_Struct*)app->pBuffer;
	FillSpawnStruct(ns, ForWho);

	if(strlen(ns->spawn.lastName) == 0)
	{
		switch(ns->spawn.class_)
		{
		case TRIBUTE_MASTER:
			strcpy(ns->spawn.lastName, "Tribute Master");
			break;
		case ADVENTURERECRUITER:
			strcpy(ns->spawn.lastName, "Adventure Recruiter");
			break;
		case BANKER:
			strcpy(ns->spawn.lastName, "Banker");
			break;
		case ADVENTUREMERCHANT:
			strcpy(ns->spawn.lastName,"Adventure Merchant");
			break;
		case WARRIORGM:
			strcpy(ns->spawn.lastName, "GM Warrior");
			break;
		case PALADINGM:
			strcpy(ns->spawn.lastName, "GM Paladin");
			break;
		case RANGERGM:
			strcpy(ns->spawn.lastName, "GM Ranger");
			break;
		case SHADOWKNIGHTGM:
			strcpy(ns->spawn.lastName, "GM Shadowknight");
			break;
		case DRUIDGM:
			strcpy(ns->spawn.lastName, "GM Druid");
			break;
		case BARDGM:
			strcpy(ns->spawn.lastName, "GM Bard");
			break;
		case ROGUEGM:
			strcpy(ns->spawn.lastName, "GM Rogue");
			break;
		case SHAMANGM:
			strcpy(ns->spawn.lastName, "GM Shaman");
			break;
		case NECROMANCERGM:
			strcpy(ns->spawn.lastName, "GM Necromancer");
			break;
		case WIZARDGM:
			strcpy(ns->spawn.lastName, "GM Wizard");
			break;
		case MAGICIANGM:
			strcpy(ns->spawn.lastName, "GM Magician");
			break;
		case ENCHANTERGM:
			strcpy(ns->spawn.lastName, "GM Enchanter");
			break;
		case BEASTLORDGM:
			strcpy(ns->spawn.lastName, "GM Beastlord");
			break;
		case BERSERKERGM:
			strcpy(ns->spawn.lastName, "GM Berserker");
			break;
		case MERCERNARY_MASTER:
			strcpy(ns->spawn.lastName, "Mercenary Recruiter");
			break;
		default:
			break;
		}
	}
}

void Mob::CreateSpawnPacket(EQApplicationPacket* app, NewSpawn_Struct* ns) {
	app->SetOpcode(OP_NewSpawn);
	app->size = sizeof(NewSpawn_Struct);

	app->pBuffer = new uchar[sizeof(NewSpawn_Struct)];

	// Copy ns directly into packet
	memcpy(app->pBuffer, ns, sizeof(NewSpawn_Struct));

	// Custom packet data
	NewSpawn_Struct* ns2 = (NewSpawn_Struct*)app->pBuffer;
	strcpy(ns2->spawn.name, ns->spawn.name);

	// Set default Last Names for certain Classes if not defined
	if (strlen(ns->spawn.lastName) == 0)
	{
		switch (ns->spawn.class_)
		{
			case TRIBUTE_MASTER:
				strcpy(ns2->spawn.lastName, "Tribute Master");
				break;
			case ADVENTURERECRUITER:
				strcpy(ns2->spawn.lastName, "Adventure Recruiter");
				break;
			case BANKER:
				strcpy(ns2->spawn.lastName, "Banker");
				break;
			case ADVENTUREMERCHANT:
				strcpy(ns2->spawn.lastName, "Adventure Merchant");
				break;
			case WARRIORGM:
				strcpy(ns2->spawn.lastName, "GM Warrior");
				break;
			case PALADINGM:
				strcpy(ns2->spawn.lastName, "GM Paladin");
				break;
			case RANGERGM:
				strcpy(ns2->spawn.lastName, "GM Ranger");
				break;
			case SHADOWKNIGHTGM:
				strcpy(ns2->spawn.lastName, "GM Shadowknight");
				break;
			case DRUIDGM:
				strcpy(ns2->spawn.lastName, "GM Druid");
				break;
			case BARDGM:
				strcpy(ns2->spawn.lastName, "GM Bard");
				break;
			case ROGUEGM:
				strcpy(ns2->spawn.lastName, "GM Rogue");
				break;
			case SHAMANGM:
				strcpy(ns2->spawn.lastName, "GM Shaman");
				break;
			case NECROMANCERGM:
				strcpy(ns2->spawn.lastName, "GM Necromancer");
				break;
			case WIZARDGM:
				strcpy(ns2->spawn.lastName, "GM Wizard");
				break;
			case MAGICIANGM:
				strcpy(ns2->spawn.lastName, "GM Magician");
				break;
			case ENCHANTERGM:
				strcpy(ns2->spawn.lastName, "GM Enchanter");
				break;
			case BEASTLORDGM:
				strcpy(ns2->spawn.lastName, "GM Beastlord");
				break;
			case BERSERKERGM:
				strcpy(ns2->spawn.lastName, "GM Berserker");
				break;
			case MERCERNARY_MASTER:
				strcpy(ns2->spawn.lastName, "Mercenary liaison");
				break;
			default:
				strcpy(ns2->spawn.lastName, ns->spawn.lastName);
				break;
		}
	}
	else
	{
		strcpy(ns2->spawn.lastName, ns->spawn.lastName);
	}
	memset(&app->pBuffer[sizeof(Spawn_Struct)-7], 0xFF, 7);
}

void Mob::FillSpawnStruct(NewSpawn_Struct* ns, Mob* ForWho)
{
	int i;

	strcpy(ns->spawn.name, name);
	if(IsClient()) {
		strn0cpy(ns->spawn.lastName, lastname, sizeof(ns->spawn.lastName));
	}

	ns->spawn.heading	= FloatToEQ19(m_Position.w);
	ns->spawn.x			= FloatToEQ19(m_Position.x);//((int32)x_pos)<<3;
	ns->spawn.y			= FloatToEQ19(m_Position.y);//((int32)y_pos)<<3;
	ns->spawn.z			= FloatToEQ19(m_Position.z);//((int32)z_pos)<<3;
	ns->spawn.spawnId	= GetID();
	ns->spawn.curHp	= static_cast<uint8>(GetHPRatio());
	ns->spawn.max_hp	= 100;		//this field needs a better name
	ns->spawn.race		= race;
	ns->spawn.runspeed	= runspeed;
	ns->spawn.walkspeed	= walkspeed;
	ns->spawn.class_	= class_;
	ns->spawn.gender	= gender;
	ns->spawn.level		= level;
	ns->spawn.PlayerState	= m_PlayerState;
	ns->spawn.deity		= deity;
	ns->spawn.animation	= 0;
	ns->spawn.findable	= findable?1:0;

	UpdateActiveLight();
	ns->spawn.light		= m_Light.Type.Active;

	ns->spawn.showhelm = (helmtexture && helmtexture != 0xFF) ? 1 : 0;

	ns->spawn.invis		= (invisible || hidden) ? 1 : 0;	// TODO: load this before spawning players
	ns->spawn.NPC		= IsClient() ? 0 : 1;
	ns->spawn.IsMercenary = IsMerc() ? 1 : 0;
	ns->spawn.targetable_with_hotkey = no_target_hotkey ? 0 : 1; // opposite logic!

	ns->spawn.petOwnerId	= ownerid;

	ns->spawn.haircolor = haircolor;
	ns->spawn.beardcolor = beardcolor;
	ns->spawn.eyecolor1 = eyecolor1;
	ns->spawn.eyecolor2 = eyecolor2;
	ns->spawn.hairstyle = hairstyle;
	ns->spawn.face = luclinface;
	ns->spawn.beard = beard;
	ns->spawn.StandState = GetAppearanceValue(_appearance);
	ns->spawn.drakkin_heritage = drakkin_heritage;
	ns->spawn.drakkin_tattoo = drakkin_tattoo;
	ns->spawn.drakkin_details = drakkin_details;
	ns->spawn.equip_chest2 = GetHerosForgeModel(1) != 0 || multitexture? 0xff : texture;

//	ns->spawn.invis2 = 0xff;//this used to be labeled beard.. if its not FF it will turn mob invis

	if (helmtexture && helmtexture != 0xFF && GetHerosForgeModel(0) == 0)
	{
		ns->spawn.helm=helmtexture;
	} else {
		ns->spawn.helm = 0;
	}

	ns->spawn.guildrank	= 0xFF;
	ns->spawn.size = size;
	ns->spawn.bodytype = bodytype;
	// The 'flymode' settings have the following effect:
	// 0 - Mobs in water sink like a stone to the bottom
	// 1 - Same as #flymode 1
	// 2 - Same as #flymode 2
	// 3 - Mobs in water do not sink. A value of 3 in this field appears to be the default setting for all mobs
	// (in water or not) according to 6.2 era packet collects.
	if(IsClient())
		ns->spawn.flymode = FindType(SE_Levitate) ? 2 : 0;
	else
		ns->spawn.flymode = flymode;

	if(IsBoat()) {
		ns->spawn.flymode = 1;
	}

	ns->spawn.lastName[0] = '\0';

	strn0cpy(ns->spawn.lastName, lastname, sizeof(ns->spawn.lastName));

	//for (i = 0; i < _MaterialCount; i++)
	for (i = 0; i < 9; i++)
	{
		// Only Player Races Wear Armor
		if (Mob::IsPlayerRace(race) || i > 6)
		{
			ns->spawn.equipment[i].Material = GetEquipmentMaterial(i);
			ns->spawn.equipment[i].EliteMaterial = IsEliteMaterialItem(i);
			ns->spawn.equipment[i].HeroForgeModel = GetHerosForgeModel(i);
			ns->spawn.colors[i].Color = GetEquipmentColor(i);
		}
	}

	memset(ns->spawn.set_to_0xFF, 0xFF, sizeof(ns->spawn.set_to_0xFF));
	if(IsNPC() && IsDestructibleObject())
	{
		ns->spawn.DestructibleObject = true;

		// Changing the first string made it vanish, so it has some significance.
		if(lastname)
			sprintf(ns->spawn.DestructibleModel, "%s", lastname);
		// Changing the second string made no visible difference
		sprintf(ns->spawn.DestructibleName2, "%s", ns->spawn.name);
		// Putting a string in the final one that was previously empty had no visible effect.
		sprintf(ns->spawn.DestructibleString, "");

		// Sets damage appearance level of the object.
		ns->spawn.DestructibleAppearance = luclinface; // Was 0x00000000
		//ns->spawn.DestructibleAppearance = static_cast<EmuAppearance>(_appearance);
		// #appearance 44 1 makes it jump but no visible damage
		// #appearance 44 2 makes it look completely broken but still visible
		// #appearance 44 3 makes it jump but not visible difference to 3
		// #appearance 44 4 makes it disappear altogether
		// #appearance 44 5 makes the client crash.

		ns->spawn.DestructibleUnk1 = 0x00000224;	// Was 0x000001f5;
		// These next 4 are mostly always sequential
		// Originally they were 633, 634, 635, 636
		// Changing them all to 633 - no visible effect.
		// Changing them all to 636 - no visible effect.
		// Reversing the order of these four numbers and then using #appearance gain had no visible change.
		// Setting these four ids to zero had no visible effect when the catapult spawned, nor when #appearance was used.
		ns->spawn.DestructibleID1 = 1968;
		ns->spawn.DestructibleID2 = 1969;
		ns->spawn.DestructibleID3 = 1970;
		ns->spawn.DestructibleID4 = 1971;
		// Next one was originally 0x1ce45008, changing it to 0x00000000 made no visible difference
		ns->spawn.DestructibleUnk2 = 0x13f79d00;
		// Next one was originally 0x1a68fe30, changing it to 0x00000000 made no visible difference
		ns->spawn.DestructibleUnk3 = 0x00000000;
		// Next one was already 0x00000000
		ns->spawn.DestructibleUnk4 = 0x13f79d58;
		// Next one was originally 0x005a69ec, changing it to 0x00000000 made no visible difference.
		ns->spawn.DestructibleUnk5 = 0x13c55b00;
		// Next one was originally 0x1a68fe30, changing it to 0x00000000 made no visible difference.
		ns->spawn.DestructibleUnk6 = 0x00128860;
		// Next one was originally 0x0059de6d, changing it to 0x00000000 made no visible difference.
		ns->spawn.DestructibleUnk7 = 0x005a8f66;
		// Next one was originally 0x00000201, changing it to 0x00000000 made no visible difference.
		// For the Minohten tents, 0x00000000 had them up in the air, while 0x201 put them on the ground.
		// Changing it it 0x00000001 makes the tent sink into the ground.
		ns->spawn.DestructibleUnk8 = 0x01;			// Needs to be 1 for tents?
		ns->spawn.DestructibleUnk9 = 0x00000002;	// Needs to be 2 for tents?

		ns->spawn.flymode = 0;
	}
}

void Mob::CreateDespawnPacket(EQApplicationPacket* app, bool Decay)
{
	app->SetOpcode(OP_DeleteSpawn);
	app->size = sizeof(DeleteSpawn_Struct);
	app->pBuffer = new uchar[app->size];
	memset(app->pBuffer, 0, app->size);
	DeleteSpawn_Struct* ds = (DeleteSpawn_Struct*)app->pBuffer;
	ds->spawn_id = GetID();
	// The next field only applies to corpses. If 0, they vanish instantly, otherwise they 'decay'
	ds->Decay = Decay ? 1 : 0;
}

void Mob::CreateHPPacket(EQApplicationPacket* app)
{
	this->IsFullHP=(cur_hp>=max_hp);
	app->SetOpcode(OP_MobHealth);
	app->size = sizeof(SpawnHPUpdate_Struct2);
	app->pBuffer = new uchar[app->size];
	memset(app->pBuffer, 0, sizeof(SpawnHPUpdate_Struct2));
	SpawnHPUpdate_Struct2* ds = (SpawnHPUpdate_Struct2*)app->pBuffer;

	ds->spawn_id = GetID();
	// they don't need to know the real hp
	ds->hp = (int)GetHPRatio();

	// hp event
	if (IsNPC() && (GetNextHPEvent() > 0))
	{
		if (ds->hp < GetNextHPEvent())
		{
			char buf[10];
			snprintf(buf, 9, "%i", GetNextHPEvent());
			buf[9] = '\0';
			SetNextHPEvent(-1);
			parse->EventNPC(EVENT_HP, CastToNPC(), nullptr, buf, 0);
		}
	}

	if (IsNPC() && (GetNextIncHPEvent() > 0))
	{
		if (ds->hp > GetNextIncHPEvent())
		{
			char buf[10];
			snprintf(buf, 9, "%i", GetNextIncHPEvent());
			buf[9] = '\0';
			SetNextIncHPEvent(-1);
			parse->EventNPC(EVENT_HP, CastToNPC(), nullptr, buf, 1);
		}
	}
}

// sends hp update of this mob to people who might care
void Mob::SendHPUpdate(bool skip_self)
{
	EQApplicationPacket hp_app;
	Group *group;

	// destructor will free the pBuffer
	CreateHPPacket(&hp_app);

	// send to people who have us targeted
	entity_list.QueueClientsByTarget(this, &hp_app, false, 0, false, true, BIT_AllClients);
	entity_list.QueueClientsByXTarget(this, &hp_app, false);
	entity_list.QueueToGroupsForNPCHealthAA(this, &hp_app);

	// send to group
	if(IsGrouped())
	{
		group = entity_list.GetGroupByMob(this);
		if(group) //not sure why this might be null, but it happens
			group->SendHPPacketsFrom(this);
	}

	if(IsClient()){
		Raid *r = entity_list.GetRaidByClient(CastToClient());
		if(r){
			r->SendHPPacketsFrom(this);
		}
	}

	// send to master
	if(GetOwner() && GetOwner()->IsClient())
	{
		GetOwner()->CastToClient()->QueuePacket(&hp_app, false);
		group = entity_list.GetGroupByClient(GetOwner()->CastToClient());
		if(group)
			group->SendHPPacketsFrom(this);
		Raid *r = entity_list.GetRaidByClient(GetOwner()->CastToClient());
		if(r)
			r->SendHPPacketsFrom(this);
	}

	// send to pet
	if(GetPet() && GetPet()->IsClient())
	{
		GetPet()->CastToClient()->QueuePacket(&hp_app, false);
	}

	// Update the damage state of destructible objects
	if(IsNPC() && IsDestructibleObject())
	{
		if (GetHPRatio() > 74)
		{
			if (GetAppearance() != eaStanding)
			{
					SendAppearancePacket(AT_DamageState, eaStanding);
					_appearance = eaStanding;
			}
		}
		else if (GetHPRatio() > 49)
		{
			if (GetAppearance() != eaSitting)
			{
				SendAppearancePacket(AT_DamageState, eaSitting);
				_appearance = eaSitting;
			}
		}
		else if (GetHPRatio() > 24)
		{
			if (GetAppearance() != eaCrouching)
			{
				SendAppearancePacket(AT_DamageState, eaCrouching);
				_appearance = eaCrouching;
			}
		}
		else if (GetHPRatio() > 0)
		{
			if (GetAppearance() != eaDead)
			{
				SendAppearancePacket(AT_DamageState, eaDead);
				_appearance = eaDead;
			}
		}
		else if (GetAppearance() != eaLooting)
		{
			SendAppearancePacket(AT_DamageState, eaLooting);
			_appearance = eaLooting;
		}
	}

	bool dospam = RuleB(Character, SpamHPUpdates);
	// send to self - we need the actual hps here
	if(IsClient() && (!skip_self || dospam)) {

		this->CastToClient()->SendHPUpdateMarquee();

		EQApplicationPacket* hp_app2 = new EQApplicationPacket(OP_HPUpdate,sizeof(SpawnHPUpdate_Struct));
		SpawnHPUpdate_Struct* ds = (SpawnHPUpdate_Struct*)hp_app2->pBuffer;
		ds->cur_hp = CastToClient()->GetHP() - itembonuses.HP;
		ds->spawn_id = GetID();
		ds->max_hp = CastToClient()->GetMaxHP() - itembonuses.HP;
		CastToClient()->QueuePacket(hp_app2);
		safe_delete(hp_app2);
	}
	if (!dospam)
		ResetHPUpdateTimer(); // delay the timer
}

// this one just warps the mob to the current location
void Mob::SendPosition()
{
	EQApplicationPacket* app = new EQApplicationPacket(OP_ClientUpdate, sizeof(PlayerPositionUpdateServer_Struct));
	PlayerPositionUpdateServer_Struct* spu = (PlayerPositionUpdateServer_Struct*)app->pBuffer;
	MakeSpawnUpdateNoDelta(spu);
	move_tic_count = 0;
	entity_list.QueueClients(this, app, true);
	safe_delete(app);
}

// this one is for mobs on the move, with deltas - this makes them walk
void Mob::SendPosUpdate(uint8 iSendToSelf) {
	EQApplicationPacket* app = new EQApplicationPacket(OP_ClientUpdate, sizeof(PlayerPositionUpdateServer_Struct));
	PlayerPositionUpdateServer_Struct* spu = (PlayerPositionUpdateServer_Struct*)app->pBuffer;
	MakeSpawnUpdate(spu);

	if (iSendToSelf == 2) {
		if (IsClient()) {
			CastToClient()->FastQueuePacket(&app,false);
		}
	}
	else
	{
		if(move_tic_count == RuleI(Zone, NPCPositonUpdateTicCount))
		{
			entity_list.QueueClients(this, app, (iSendToSelf == 0), false);
			move_tic_count = 0;
		}
		else if(move_tic_count % 2 == 0)
		{
			entity_list.QueueCloseClients(this, app, (iSendToSelf == 0), 700, nullptr, false);
			move_tic_count++;
		} 
		else {
			move_tic_count++;
		}
	}
	safe_delete(app);
}

// this is for SendPosition()
void Mob::MakeSpawnUpdateNoDelta(PlayerPositionUpdateServer_Struct *spu){
	memset(spu,0xff,sizeof(PlayerPositionUpdateServer_Struct));
	spu->spawn_id	= GetID();
	spu->x_pos		= FloatToEQ19(m_Position.x);
	spu->y_pos		= FloatToEQ19(m_Position.y);
	spu->z_pos		= FloatToEQ19(m_Position.z);
	spu->delta_x	= NewFloatToEQ13(0);
	spu->delta_y	= NewFloatToEQ13(0);
	spu->delta_z	= NewFloatToEQ13(0);
	spu->heading	= FloatToEQ19(m_Position.w);
	spu->animation	= 0;
	spu->delta_heading = NewFloatToEQ13(0);
	spu->padding0002	=0;
	spu->padding0006	=7;
	spu->padding0014	=0x7f;
	spu->padding0018	=0x5df27;

}

// this is for SendPosUpdate()
void Mob::MakeSpawnUpdate(PlayerPositionUpdateServer_Struct* spu) {
	spu->spawn_id	= GetID();
	spu->x_pos		= FloatToEQ19(m_Position.x);
	spu->y_pos		= FloatToEQ19(m_Position.y);
	spu->z_pos		= FloatToEQ19(m_Position.z);
	spu->delta_x	= NewFloatToEQ13(m_Delta.x);
	spu->delta_y	= NewFloatToEQ13(m_Delta.y);
	spu->delta_z	= NewFloatToEQ13(m_Delta.z);
	spu->heading	= FloatToEQ19(m_Position.w);
	spu->padding0002	=0;
	spu->padding0006	=7;
	spu->padding0014	=0x7f;
	spu->padding0018	=0x5df27;
	if(this->IsClient())
		spu->animation = animation;
	else
		spu->animation	= pRunAnimSpeed;//animation;
	spu->delta_heading = NewFloatToEQ13(m_Delta.w);
}

void Mob::ShowStats(Client* client)
{
	if (IsClient()) {
		CastToClient()->SendStatsWindow(client, RuleB(Character, UseNewStatsWindow));
	}
	else if (IsCorpse()) {
		if (IsPlayerCorpse()) {
			client->Message(0, "  CharID: %i  PlayerCorpse: %i", CastToCorpse()->GetCharID(), CastToCorpse()->GetCorpseDBID());
		}
		else {
			client->Message(0, "  NPCCorpse", GetID());
		}
	}
	else {
		client->Message(0, "  Level: %i  AC: %i  Class: %i  Size: %1.1f  Haste: %i", GetLevel(), GetAC(), GetClass(), GetSize(), GetHaste());
		client->Message(0, "  HP: %i  Max HP: %i",GetHP(), GetMaxHP());
		client->Message(0, "  Mana: %i  Max Mana: %i", GetMana(), GetMaxMana());
		client->Message(0, "  Total ATK: %i  Worn/Spell ATK (Cap %i): %i", GetATK(), RuleI(Character, ItemATKCap), GetATKBonus());
		client->Message(0, "  STR: %i  STA: %i  DEX: %i  AGI: %i  INT: %i  WIS: %i  CHA: %i", GetSTR(), GetSTA(), GetDEX(), GetAGI(), GetINT(), GetWIS(), GetCHA());
		client->Message(0, "  MR: %i  PR: %i  FR: %i  CR: %i  DR: %i Corruption: %i PhR: %i", GetMR(), GetPR(), GetFR(), GetCR(), GetDR(), GetCorrup(), GetPhR());
		client->Message(0, "  Race: %i  BaseRace: %i  Texture: %i  HelmTexture: %i  Gender: %i  BaseGender: %i", GetRace(), GetBaseRace(), GetTexture(), GetHelmTexture(), GetGender(), GetBaseGender());
		if (client->Admin() >= 100)
			client->Message(0, "  EntityID: %i  PetID: %i  OwnerID: %i AIControlled: %i Targetted: %i", GetID(), GetPetID(), GetOwnerID(), IsAIControlled(), targeted);

		if (IsNPC()) {
			NPC *n = CastToNPC();
			uint32 spawngroupid = 0;
			if(n->respawn2 != 0)
				spawngroupid = n->respawn2->SpawnGroupID();
			client->Message(0, "  NPCID: %u  SpawnGroupID: %u Grid: %i LootTable: %u FactionID: %i SpellsID: %u ", GetNPCTypeID(),spawngroupid, n->GetGrid(), n->GetLoottableID(), n->GetNPCFactionID(), n->GetNPCSpellsID());
			client->Message(0, "  Accuracy: %i MerchantID: %i EmoteID: %i Runspeed: %.3f Walkspeed: %.3f", n->GetAccuracyRating(), n->MerchantType, n->GetEmoteID(), static_cast<float>(0.025f * n->GetRunspeed()), static_cast<float>(0.025f * n->GetWalkspeed()));
			n->QueryLoot(client);
		}
		if (IsAIControlled()) {
			client->Message(0, "  AggroRange: %1.0f  AssistRange: %1.0f", GetAggroRange(), GetAssistRange());
		}
	}
}

void Mob::DoAnim(const int animnum, int type, bool ackreq, eqFilterType filter) {
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_Animation, sizeof(Animation_Struct));
	Animation_Struct* anim = (Animation_Struct*)outapp->pBuffer;
	anim->spawnid = GetID();
	if(type == 0){
		anim->action = animnum;
		anim->speed = 10;
	}
	else{
		anim->action = animnum;
		anim->speed = type;
	}
	entity_list.QueueCloseClients(this, outapp, false, 200, 0, ackreq, filter);
	safe_delete(outapp);
}

void Mob::ShowBuffs(Client* client) {
	if(SPDAT_RECORDS <= 0)
		return;
	client->Message(0, "Buffs on: %s", this->GetName());
	uint32 i;
	uint32 buff_count = GetMaxTotalSlots();
	for (i=0; i < buff_count; i++) {
		if (buffs[i].spellid != SPELL_UNKNOWN) {
			if (spells[buffs[i].spellid].buffdurationformula == DF_Permanent)
				client->Message(0, "  %i: %s: Permanent", i, spells[buffs[i].spellid].name);
			else
				client->Message(0, "  %i: %s: %i tics left", i, spells[buffs[i].spellid].name, buffs[i].ticsremaining);

		}
	}
	if (IsClient()){
		client->Message(0, "itembonuses:");
		client->Message(0, "Atk:%i Ac:%i HP(%i):%i Mana:%i", itembonuses.ATK, itembonuses.AC, itembonuses.HPRegen, itembonuses.HP, itembonuses.Mana);
		client->Message(0, "Str:%i Sta:%i Dex:%i Agi:%i Int:%i Wis:%i Cha:%i",
			itembonuses.STR,itembonuses.STA,itembonuses.DEX,itembonuses.AGI,itembonuses.INT,itembonuses.WIS,itembonuses.CHA);
		client->Message(0, "SvMagic:%i SvFire:%i SvCold:%i SvPoison:%i SvDisease:%i",
				itembonuses.MR,itembonuses.FR,itembonuses.CR,itembonuses.PR,itembonuses.DR);
		client->Message(0, "DmgShield:%i Haste:%i", itembonuses.DamageShield, itembonuses.haste );
		client->Message(0, "spellbonuses:");
		client->Message(0, "Atk:%i Ac:%i HP(%i):%i Mana:%i", spellbonuses.ATK, spellbonuses.AC, spellbonuses.HPRegen, spellbonuses.HP, spellbonuses.Mana);
		client->Message(0, "Str:%i Sta:%i Dex:%i Agi:%i Int:%i Wis:%i Cha:%i",
			spellbonuses.STR,spellbonuses.STA,spellbonuses.DEX,spellbonuses.AGI,spellbonuses.INT,spellbonuses.WIS,spellbonuses.CHA);
		client->Message(0, "SvMagic:%i SvFire:%i SvCold:%i SvPoison:%i SvDisease:%i",
				spellbonuses.MR,spellbonuses.FR,spellbonuses.CR,spellbonuses.PR,spellbonuses.DR);
		client->Message(0, "DmgShield:%i Haste:%i", spellbonuses.DamageShield, spellbonuses.haste );
	}
}

void Mob::ShowBuffList(Client* client) {
	if(SPDAT_RECORDS <= 0)
		return;

	client->Message(0, "Buffs on: %s", this->GetCleanName());
	uint32 i;
	uint32 buff_count = GetMaxTotalSlots();
	for (i=0; i < buff_count; i++) {
		if (buffs[i].spellid != SPELL_UNKNOWN) {
			if (spells[buffs[i].spellid].buffdurationformula == DF_Permanent)
				client->Message(0, "  %i: %s: Permanent", i, spells[buffs[i].spellid].name);
			else
				client->Message(0, "  %i: %s: %i tics left", i, spells[buffs[i].spellid].name, buffs[i].ticsremaining);
		}
	}
}

void Mob::GMMove(float x, float y, float z, float heading, bool SendUpdate) {

	Route.clear();

	if(IsNPC()) {
		entity_list.ProcessMove(CastToNPC(), x, y, z);
	}

	m_Position.x = x;
	m_Position.y = y;
	m_Position.z = z;
	if (m_Position.w != 0.01)
		this->m_Position.w = heading;
	if(IsNPC())
		CastToNPC()->SaveGuardSpot(true);
	if(SendUpdate)
		SendPosition();
}

void Mob::SendIllusionPacket(uint16 in_race, uint8 in_gender, uint8 in_texture, uint8 in_helmtexture, uint8 in_haircolor, uint8 in_beardcolor, uint8 in_eyecolor1, uint8 in_eyecolor2, uint8 in_hairstyle, uint8 in_luclinface, uint8 in_beard, uint8 in_aa_title, uint32 in_drakkin_heritage, uint32 in_drakkin_tattoo, uint32 in_drakkin_details, float in_size) {

	uint16 BaseRace = GetBaseRace();

	if (in_race == 0)
	{
		race = BaseRace;
		if (in_gender == 0xFF)
			gender = GetBaseGender();
		else
			gender = in_gender;
	}
	else
	{
		race = in_race;
		if (in_gender == 0xFF)
			gender = GetDefaultGender(race, gender);
		else
			gender = in_gender;
	}

	if (in_texture == 0xFF)
	{
		if (IsPlayerRace(in_race))
			texture = 0xFF;
		else
			texture = GetTexture();
	}
	else
	{
		texture = in_texture;
	}

	if (in_helmtexture == 0xFF)
	{
		if (IsPlayerRace(in_race))
			helmtexture = 0xFF;
		else if (in_texture != 0xFF)
			helmtexture = in_texture;
		else
			helmtexture = GetHelmTexture();
	}
	else
	{
		helmtexture = in_helmtexture;
	}

	if (in_haircolor == 0xFF)
		haircolor = GetHairColor();
	else
		haircolor = in_haircolor;

	if (in_beardcolor == 0xFF)
		beardcolor = GetBeardColor();
	else
		beardcolor = in_beardcolor;

	if (in_eyecolor1 == 0xFF)
		eyecolor1 = GetEyeColor1();
	else
		eyecolor1 = in_eyecolor1;

	if (in_eyecolor2 == 0xFF)
		eyecolor2 = GetEyeColor2();
	else
		eyecolor2 = in_eyecolor2;

	if (in_hairstyle == 0xFF)
		hairstyle = GetHairStyle();
	else
		hairstyle = in_hairstyle;

	if (in_luclinface == 0xFF)
		luclinface = GetLuclinFace();
	else
		luclinface = in_luclinface;

	if (in_beard == 0xFF)
		beard = GetBeard();
	else
		beard = in_beard;

	aa_title = in_aa_title;

	if (in_drakkin_heritage == 0xFFFFFFFF)
		drakkin_heritage = GetDrakkinHeritage();
	else
		drakkin_heritage = in_drakkin_heritage;

	if (in_drakkin_tattoo == 0xFFFFFFFF)
		drakkin_tattoo = GetDrakkinTattoo();
	else
		drakkin_tattoo = in_drakkin_tattoo;

	if (in_drakkin_details == 0xFFFFFFFF)
		drakkin_details = GetDrakkinDetails();
	else
		drakkin_details = in_drakkin_details;

	if (in_size <= 0.0f)
		size = GetSize();
	else
		size = in_size;

	// Reset features to Base from the Player Profile
	if (IsClient() && in_race == 0)
	{
		race = CastToClient()->GetBaseRace();
		gender = CastToClient()->GetBaseGender();
		texture = 0xFF;
		helmtexture = 0xFF;
		haircolor = CastToClient()->GetBaseHairColor();
		beardcolor = CastToClient()->GetBaseBeardColor();
		eyecolor1 = CastToClient()->GetBaseEyeColor();
		eyecolor2 = CastToClient()->GetBaseEyeColor();
		hairstyle = CastToClient()->GetBaseHairStyle();
		luclinface = CastToClient()->GetBaseFace();
		beard	= CastToClient()->GetBaseBeard();
		aa_title = 0xFF;
		drakkin_heritage = CastToClient()->GetBaseHeritage();
		drakkin_tattoo = CastToClient()->GetBaseTattoo();
		drakkin_details = CastToClient()->GetBaseDetails();
		switch(race){
			case OGRE:
				size = 9;
				break;
			case TROLL:
				size = 8;
				break;
			case VAHSHIR:
			case BARBARIAN:
				size = 7;
				break;
			case HALF_ELF:
			case WOOD_ELF:
			case DARK_ELF:
			case FROGLOK:
				size = 5;
				break;
			case DWARF:
				size = 4;
				break;
			case HALFLING:
			case GNOME:
				size = 3;
				break;
			default:
				size = 6;
				break;
		}
	}

	EQApplicationPacket* outapp = new EQApplicationPacket(OP_Illusion, sizeof(Illusion_Struct));
	Illusion_Struct* is = (Illusion_Struct*) outapp->pBuffer;
	is->spawnid = GetID();
	strcpy(is->charname, GetCleanName());
	is->race = race;
	is->gender = gender;
	is->texture = texture;
	is->helmtexture = helmtexture;
	is->haircolor = haircolor;
	is->beardcolor = beardcolor;
	is->beard = beard;
	is->eyecolor1 = eyecolor1;
	is->eyecolor2 = eyecolor2;
	is->hairstyle = hairstyle;
	is->face = luclinface;
	is->drakkin_heritage = drakkin_heritage;
	is->drakkin_tattoo = drakkin_tattoo;
	is->drakkin_details = drakkin_details;
	is->size = size;

	entity_list.QueueClients(this, outapp);
	safe_delete(outapp);

	/* Refresh armor and tints after send illusion packet */
	this->SendArmorAppearance();

	Log.Out(Logs::Detail, Logs::Spells, "Illusion: Race = %i, Gender = %i, Texture = %i, HelmTexture = %i, HairColor = %i, BeardColor = %i, EyeColor1 = %i, EyeColor2 = %i, HairStyle = %i, Face = %i, DrakkinHeritage = %i, DrakkinTattoo = %i, DrakkinDetails = %i, Size = %f",
		race, gender, texture, helmtexture, haircolor, beardcolor, eyecolor1, eyecolor2, hairstyle, luclinface, drakkin_heritage, drakkin_tattoo, drakkin_details, size);
}

bool Mob::RandomizeFeatures(bool send_illusion, bool set_variables)
{
	if (IsPlayerRace(GetRace())) {
		uint8 Gender = GetGender();
		uint8 Texture = 0xFF;
		uint8 HelmTexture = 0xFF;
		uint8 HairColor = 0xFF;
		uint8 BeardColor = 0xFF;
		uint8 EyeColor1 = 0xFF;
		uint8 EyeColor2 = 0xFF;
		uint8 HairStyle = 0xFF;
		uint8 LuclinFace = 0xFF;
		uint8 Beard = 0xFF;
		uint32 DrakkinHeritage = 0xFFFFFFFF;
		uint32 DrakkinTattoo = 0xFFFFFFFF;
		uint32 DrakkinDetails = 0xFFFFFFFF;

		// Set some common feature settings
		EyeColor1 = zone->random.Int(0, 9);
		EyeColor2 = zone->random.Int(0, 9);
		LuclinFace = zone->random.Int(0, 7);

		// Adjust all settings based on the min and max for each feature of each race and gender
		switch (GetRace()) {
		case HUMAN:
			HairColor = zone->random.Int(0, 19);
			if (Gender == MALE) {
				BeardColor = HairColor;
				HairStyle = zone->random.Int(0, 3);
				Beard = zone->random.Int(0, 5);
			}
			if (Gender == FEMALE) {
				HairStyle = zone->random.Int(0, 2);
			}
			break;
		case BARBARIAN:
			HairColor = zone->random.Int(0, 19);
			LuclinFace = zone->random.Int(0, 87);
			if (Gender == MALE) {
				BeardColor = HairColor;
				HairStyle = zone->random.Int(0, 3);
				Beard = zone->random.Int(0, 5);
			}
			if (Gender == FEMALE) {
				HairStyle = zone->random.Int(0, 2);
			}
			break;
		case ERUDITE:
			if (Gender == MALE) {
				BeardColor = zone->random.Int(0, 19);
				Beard = zone->random.Int(0, 5);
				LuclinFace = zone->random.Int(0, 57);
			}
			if (Gender == FEMALE) {
				LuclinFace = zone->random.Int(0, 87);
			}
			break;
		case WOOD_ELF:
			HairColor = zone->random.Int(0, 19);
			if (Gender == MALE) {
				HairStyle = zone->random.Int(0, 3);
			}
			if (Gender == FEMALE) {
				HairStyle = zone->random.Int(0, 2);
			}
			break;
		case HIGH_ELF:
			HairColor = zone->random.Int(0, 14);
			if (Gender == MALE) {
				HairStyle = zone->random.Int(0, 3);
				LuclinFace = zone->random.Int(0, 37);
				BeardColor = HairColor;
			}
			if (Gender == FEMALE) {
				HairStyle = zone->random.Int(0, 2);
			}
			break;
		case DARK_ELF:
			HairColor = zone->random.Int(13, 18);
			if (Gender == MALE) {
				HairStyle = zone->random.Int(0, 3);
				LuclinFace = zone->random.Int(0, 37);
				BeardColor = HairColor;
			}
			if (Gender == FEMALE) {
				HairStyle = zone->random.Int(0, 2);
			}
			break;
		case HALF_ELF:
			HairColor = zone->random.Int(0, 19);
			if (Gender == MALE) {
				HairStyle = zone->random.Int(0, 3);
				LuclinFace = zone->random.Int(0, 37);
				BeardColor = HairColor;
			}
			if (Gender == FEMALE) {
				HairStyle = zone->random.Int(0, 2);
			}
			break;
		case DWARF:
			HairColor = zone->random.Int(0, 19);
			BeardColor = HairColor;
			if (Gender == MALE) {
				HairStyle = zone->random.Int(0, 3);
				Beard = zone->random.Int(0, 5);
			}
			if (Gender == FEMALE) {
				HairStyle = zone->random.Int(0, 2);
				LuclinFace = zone->random.Int(0, 17);
			}
			break;
		case TROLL:
			EyeColor1 = zone->random.Int(0, 10);
			EyeColor2 = zone->random.Int(0, 10);
			if (Gender == FEMALE) {
				HairStyle = zone->random.Int(0, 3);
				HairColor = zone->random.Int(0, 23);
			}
			break;
		case OGRE:
			if (Gender == FEMALE) {
				HairStyle = zone->random.Int(0, 3);
				HairColor = zone->random.Int(0, 23);
			}
			break;
		case HALFLING:
			HairColor = zone->random.Int(0, 19);
			if (Gender == MALE) {
				BeardColor = HairColor;
				HairStyle = zone->random.Int(0, 3);
				Beard = zone->random.Int(0, 5);
			}
			if (Gender == FEMALE) {
				HairStyle = zone->random.Int(0, 2);
			}
			break;
		case GNOME:
			HairColor = zone->random.Int(0, 24);
			if (Gender == MALE) {
				BeardColor = HairColor;
				HairStyle = zone->random.Int(0, 3);
				Beard = zone->random.Int(0, 5);
			}
			if (Gender == FEMALE) {
				HairStyle = zone->random.Int(0, 2);
			}
			break;
		case IKSAR:
		case VAHSHIR:
			break;
		case FROGLOK:
			LuclinFace = zone->random.Int(0, 9);
		case DRAKKIN:
			HairColor = zone->random.Int(0, 3);
			BeardColor = HairColor;
			EyeColor1 = zone->random.Int(0, 11);
			EyeColor2 = zone->random.Int(0, 11);
			LuclinFace = zone->random.Int(0, 6);
			DrakkinHeritage = zone->random.Int(0, 6);
			DrakkinTattoo = zone->random.Int(0, 7);
			DrakkinDetails = zone->random.Int(0, 7);
			if (Gender == MALE) {
				Beard = zone->random.Int(0, 12);
				HairStyle = zone->random.Int(0, 8);
			}
			if (Gender == FEMALE) {
				Beard = zone->random.Int(0, 3);
				HairStyle = zone->random.Int(0, 7);
			}
			break;
		default:
			break;
		}

		if (set_variables) {
			haircolor = HairColor;
			beardcolor = BeardColor;
			eyecolor1 = EyeColor1;
			eyecolor2 = EyeColor2;
			hairstyle = HairStyle;
			luclinface = LuclinFace;
			beard = Beard;
			drakkin_heritage = DrakkinHeritage;
			drakkin_tattoo = DrakkinTattoo;
			drakkin_details = DrakkinDetails;
		}

		if (send_illusion) {
			SendIllusionPacket(GetRace(), Gender, Texture, HelmTexture, HairColor, BeardColor,
				EyeColor1, EyeColor2, HairStyle, LuclinFace, Beard, 0xFF, DrakkinHeritage,
				DrakkinTattoo, DrakkinDetails);
		}

		return true;
	}
	return false;
}


bool Mob::IsPlayerRace(uint16 in_race) {

	if ((in_race >= HUMAN && in_race <= GNOME) || in_race == IKSAR || in_race == VAHSHIR || in_race == FROGLOK || in_race == DRAKKIN)
	{
		return true;
	}

	return false;
}


uint8 Mob::GetDefaultGender(uint16 in_race, uint8 in_gender) {
	if (Mob::IsPlayerRace(in_race) || in_race == 15 || in_race == 50 || in_race == 57 || in_race == 70 || in_race == 98 || in_race == 118 || in_race == 23) {
		if (in_gender >= 2) {
			// Male default for PC Races
			return 0;
		}
		else
			return in_gender;
	}
	else if (in_race == 44 || in_race == 52 || in_race == 55 || in_race == 65 || in_race == 67 || in_race == 88 || in_race == 117 || in_race == 127 ||
		in_race == 77 || in_race == 78 || in_race == 81 || in_race == 90 || in_race == 92 || in_race == 93 || in_race == 94 || in_race == 106 || in_race == 112 || in_race == 471) {
		// Male only races
		return 0;

	}
	else if (in_race == 25 || in_race == 56) {
		// Female only races
		return 1;
	}
	else {
		// Neutral default for NPC Races
		return 2;
	}
}

void Mob::SendAppearancePacket(uint32 type, uint32 value, bool WholeZone, bool iIgnoreSelf, Client *specific_target) {
	if (!GetID())
		return;
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_SpawnAppearance, sizeof(SpawnAppearance_Struct));
	SpawnAppearance_Struct* appearance = (SpawnAppearance_Struct*)outapp->pBuffer;
	appearance->spawn_id = this->GetID();
	appearance->type = type;
	appearance->parameter = value;
	if (WholeZone)
		entity_list.QueueClients(this, outapp, iIgnoreSelf);
	else if(specific_target != nullptr)
		specific_target->QueuePacket(outapp, false, Client::CLIENT_CONNECTED);
	else if (this->IsClient())
		this->CastToClient()->QueuePacket(outapp, false, Client::CLIENT_CONNECTED);
	safe_delete(outapp);
}

void Mob::SendLevelAppearance(){
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_LevelAppearance, sizeof(LevelAppearance_Struct));
	LevelAppearance_Struct* la = (LevelAppearance_Struct*)outapp->pBuffer;
	la->parm1 = 0x4D;
	la->parm2 = la->parm1 + 1;
	la->parm3 = la->parm2 + 1;
	la->parm4 = la->parm3 + 1;
	la->parm5 = la->parm4 + 1;
	la->spawn_id = GetID();
	la->value1a = 1;
	la->value2a = 2;
	la->value3a = 1;
	la->value3b = 1;
	la->value4a = 1;
	la->value4b = 1;
	la->value5a = 2;
	entity_list.QueueCloseClients(this,outapp);
	safe_delete(outapp);
}

void Mob::SendStunAppearance()
{
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_LevelAppearance, sizeof(LevelAppearance_Struct));
	LevelAppearance_Struct* la = (LevelAppearance_Struct*)outapp->pBuffer;
	la->parm1 = 58;
	la->parm2 = 60;
	la->spawn_id = GetID();
	la->value1a = 2;
	la->value1b = 0;
	la->value2a = 2;
	la->value2b = 0;
	entity_list.QueueCloseClients(this,outapp);
	safe_delete(outapp);
}

void Mob::SendAppearanceEffect(uint32 parm1, uint32 parm2, uint32 parm3, uint32 parm4, uint32 parm5, Client *specific_target){
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_LevelAppearance, sizeof(LevelAppearance_Struct));
	LevelAppearance_Struct* la = (LevelAppearance_Struct*)outapp->pBuffer;
	la->spawn_id = GetID();
	la->parm1 = parm1;
	la->parm2 = parm2;
	la->parm3 = parm3;
	la->parm4 = parm4;
	la->parm5 = parm5;
	// Note that setting the b values to 0 will disable the related effect from the corresponding parameter.
	// Setting the a value appears to have no affect at all.s
	la->value1a = 1;
	la->value1b = 1;
	la->value2a = 1;
	la->value2b = 1;
	la->value3a = 1;
	la->value3b = 1;
	la->value4a = 1;
	la->value4b = 1;
	la->value5a = 1;
	la->value5b = 1;
	if(specific_target == nullptr) {
		entity_list.QueueClients(this,outapp);
	}
	else if (specific_target->IsClient()) {
		specific_target->CastToClient()->QueuePacket(outapp, false);
	}
	safe_delete(outapp);
}

void Mob::SendTargetable(bool on, Client *specific_target) {
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_Untargetable, sizeof(Untargetable_Struct));
	Untargetable_Struct *ut = (Untargetable_Struct*)outapp->pBuffer;
	ut->id = GetID();
	ut->targetable_flag = on == true ? 1 : 0;

	if(specific_target == nullptr) {
		entity_list.QueueClients(this, outapp);
	}
	else if (specific_target->IsClient()) {
		specific_target->CastToClient()->QueuePacket(outapp, false);
	}
	safe_delete(outapp);
}

void Mob::CameraEffect(uint32 duration, uint32 intensity, Client *c, bool global) {


	if(global == true)
	{
		ServerPacket* pack = new ServerPacket(ServerOP_CameraShake, sizeof(ServerCameraShake_Struct));
		ServerCameraShake_Struct* scss = (ServerCameraShake_Struct*) pack->pBuffer;
		scss->duration = duration;
		scss->intensity = intensity;
		worldserver.SendPacket(pack);
		safe_delete(pack);
		return;
	}

	EQApplicationPacket* outapp = new EQApplicationPacket(OP_CameraEffect, sizeof(Camera_Struct));
	Camera_Struct* cs = (Camera_Struct*) outapp->pBuffer;
	cs->duration = duration;	// Duration in milliseconds
	cs->intensity = ((intensity * 6710886) + 1023410176);	// Intensity ranges from 1023410176 to 1090519040, so simplify it from 0 to 10.

	if(c)
		c->QueuePacket(outapp, false, Client::CLIENT_CONNECTED);
	else
		entity_list.QueueClients(this, outapp);

	safe_delete(outapp);
}

void Mob::SendSpellEffect(uint32 effectid, uint32 duration, uint32 finish_delay, bool zone_wide, uint32 unk020, bool perm_effect, Client *c) {

	EQApplicationPacket* outapp = new EQApplicationPacket(OP_SpellEffect, sizeof(SpellEffect_Struct));
	SpellEffect_Struct* se = (SpellEffect_Struct*) outapp->pBuffer;
	se->EffectID = effectid;	// ID of the Particle Effect
	se->EntityID = GetID();
	se->EntityID2 = GetID();	// EntityID again
	se->Duration = duration;	// In Milliseconds
	se->FinishDelay = finish_delay;	// Seen 0
	se->Unknown020 = unk020;	// Seen 3000
	se->Unknown024 = 1;		// Seen 1 for SoD
	se->Unknown025 = 1;		// Seen 1 for Live
	se->Unknown026 = 0;		// Seen 1157

	if(c)
		c->QueuePacket(outapp, false, Client::CLIENT_CONNECTED);
	else if(zone_wide)
		entity_list.QueueClients(this, outapp);
	else
		entity_list.QueueCloseClients(this, outapp);

	safe_delete(outapp);

	if (perm_effect) {
		if(!IsNimbusEffectActive(effectid)) {
			SetNimbusEffect(effectid);
		}
	}

}

void Mob::TempName(const char *newname)
{
	char temp_name[64];
	char old_name[64];
	strn0cpy(old_name, GetName(), 64);

	if(newname)
		strn0cpy(temp_name, newname, 64);

	// Reset the name to the original if left null.
	if(!newname) {
		strn0cpy(temp_name, GetOrigName(), 64);
		SetName(temp_name);
		//CleanMobName(GetName(), temp_name);
		strn0cpy(temp_name, GetCleanName(), 64);
	}

	// Remove Numbers before making name unique
	EntityList::RemoveNumbers(temp_name);
	// Make the new name unique and set it
	entity_list.MakeNameUnique(temp_name);

	// Send the new name to all clients
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_MobRename, sizeof(MobRename_Struct));
	MobRename_Struct* mr = (MobRename_Struct*) outapp->pBuffer;
	strn0cpy(mr->old_name, old_name, 64);
	strn0cpy(mr->old_name_again, old_name, 64);
	strn0cpy(mr->new_name, temp_name, 64);
	mr->unknown192 = 0;
	mr->unknown196 = 1;
	entity_list.QueueClients(this, outapp);
	safe_delete(outapp);

	SetName(temp_name);
}

void Mob::SetTargetable(bool on) {
	if(m_targetable != on) {
		m_targetable = on;
		SendTargetable(on);
	}
}

const int32& Mob::SetMana(int32 amount)
{
	CalcMaxMana();
	int32 mmana = GetMaxMana();
	cur_mana = amount < 0 ? 0 : (amount > mmana ? mmana : amount);
/*
	if(IsClient())
		LogFile->write(EQEMuLog::Debug, "Setting mana for %s to %d (%4.1f%%)", GetName(), amount, GetManaRatio());
*/

	return cur_mana;
}


void Mob::SetAppearance(EmuAppearance app, bool iIgnoreSelf) {
	if (_appearance == app)
		return;
	_appearance = app;
	SendAppearancePacket(AT_Anim, GetAppearanceValue(app), true, iIgnoreSelf);
	if (this->IsClient() && this->IsAIControlled())
		SendAppearancePacket(AT_Anim, ANIM_FREEZE, false, false);
}

bool Mob::UpdateActiveLight()
{
	uint8 old_light_level = m_Light.Level.Active;

	m_Light.Type.Active = 0;
	m_Light.Level.Active = 0;

	if (m_Light.IsLevelGreater((m_Light.Type.Innate & 0x0F), m_Light.Type.Active)) { m_Light.Type.Active = m_Light.Type.Innate; }
	if (m_Light.Level.Equipment > m_Light.Level.Active) { m_Light.Type.Active = m_Light.Type.Equipment; } // limiter in property handler
	if (m_Light.Level.Spell > m_Light.Level.Active) { m_Light.Type.Active = m_Light.Type.Spell; } // limiter in property handler

	m_Light.Level.Active = m_Light.TypeToLevel(m_Light.Type.Active);

	return (m_Light.Level.Active != old_light_level);
}

void Mob::ChangeSize(float in_size = 0, bool bNoRestriction) {
	// Size Code
	if (!bNoRestriction)
	{
		if (this->IsClient() || this->petid != 0)
			if (in_size < 3.0)
				in_size = 3.0;


			if (this->IsClient() || this->petid != 0)
				if (in_size > 15.0)
					in_size = 15.0;
	}


	if (in_size < 1.0)
		in_size = 1.0;

	if (in_size > 255.0)
		in_size = 255.0;
	//End of Size Code
	this->size = in_size;
	SendAppearancePacket(AT_Size, (uint32) in_size);
}

Mob* Mob::GetOwnerOrSelf() {
	if (!GetOwnerID())
		return this;
	Mob* owner = entity_list.GetMob(this->GetOwnerID());
	if (!owner) {
		SetOwnerID(0);
		return(this);
	}
	if (owner->GetPetID() == this->GetID()) {
		return owner;
	}
	if(IsNPC() && CastToNPC()->GetSwarmInfo()){
		return (CastToNPC()->GetSwarmInfo()->GetOwner());
	}
	SetOwnerID(0);
	return this;
}

Mob* Mob::GetOwner() {
	Mob* owner = entity_list.GetMob(this->GetOwnerID());
	if (owner && owner->GetPetID() == this->GetID()) {

		return owner;
	}
	if(IsNPC() && CastToNPC()->GetSwarmInfo()){
		return (CastToNPC()->GetSwarmInfo()->GetOwner());
	}
	SetOwnerID(0);
	return 0;
}

Mob* Mob::GetUltimateOwner()
{
	Mob* Owner = GetOwner();

	if(!Owner)
		return this;

	while(Owner && Owner->HasOwner())
		Owner = Owner->GetOwner();

	return Owner ? Owner : this;
}

void Mob::SetOwnerID(uint16 NewOwnerID) {
	if (NewOwnerID == GetID() && NewOwnerID != 0) // ok, no charming yourself now =p
		return;
	ownerid = NewOwnerID;
	// if we're setting the owner ID to 0 and they're not either charmed or not-a-pet then
	// they're a normal pet and should be despawned
	if (ownerid == 0 && IsNPC() && GetPetType() != petCharmed && GetPetType() != petNone)
		Depop();
}

// used in checking for behind (backstab) and checking in front (melee LoS)
float Mob::MobAngle(Mob *other, float ourx, float oury) const {
	if (!other || other == this)
		return 0.0f;

	float angle, lengthb, vectorx, vectory, dotp;
	float mobx = -(other->GetX());	// mob xloc (inverse because eq)
	float moby = other->GetY();		// mob yloc
	float heading = other->GetHeading();	// mob heading
	heading = (heading * 360.0f) / 256.0f;	// convert to degrees
	if (heading < 270)
		heading += 90;
	else
		heading -= 270;

	heading = heading * 3.1415f / 180.0f;	// convert to radians
	vectorx = mobx + (10.0f * cosf(heading));	// create a vector based on heading
	vectory = moby + (10.0f * sinf(heading));	// of mob length 10

	// length of mob to player vector
	lengthb = (float) sqrtf(((-ourx - mobx) * (-ourx - mobx)) + ((oury - moby) * (oury - moby)));

	// calculate dot product to get angle
	// Handle acos domain errors due to floating point rounding errors
	dotp = ((vectorx - mobx) * (-ourx - mobx) +
			(vectory - moby) * (oury - moby)) / (10 * lengthb);
	// I haven't seen any errors that  cause problems that weren't slightly
	// larger/smaller than 1/-1, so only handle these cases for now
	if (dotp > 1)
		return 0.0f;
	else if (dotp < -1)
		return 180.0f;

	angle = acosf(dotp);
	angle = angle * 180.0f / 3.1415f;

	return angle;
}

void Mob::SetZone(uint32 zone_id, uint32 instance_id)
{
	if(IsClient())
	{
		CastToClient()->GetPP().zone_id = zone_id;
		CastToClient()->GetPP().zoneInstance = instance_id;
		CastToClient()->Save();
	}
	Save();
}

void Mob::Kill() {
	Death(this, 0, SPELL_UNKNOWN, SkillHandtoHand);
}

bool Mob::CanThisClassDualWield(void) const {
	if(!IsClient()) {
		return(GetSkill(SkillDualWield) > 0);
	}
	else if(CastToClient()->HasSkill(SkillDualWield)) {
		const ItemInst* pinst = CastToClient()->GetInv().GetItem(MainPrimary);
		const ItemInst* sinst = CastToClient()->GetInv().GetItem(MainSecondary);

		// 2HS, 2HB, or 2HP
		if(pinst && pinst->IsWeapon()) {
			const Item_Struct* item = pinst->GetItem();

			if((item->ItemType == ItemType2HBlunt) || (item->ItemType == ItemType2HSlash) || (item->ItemType == ItemType2HPiercing))
				return false;
		}

		// OffHand Weapon
		if(sinst && !sinst->IsWeapon())
			return false;

		// Dual-Wielding Empty Fists
		if(!pinst && !sinst)
			if(class_ != MONK && class_ != MONKGM && class_ != BEASTLORD && class_ != BEASTLORDGM)
				return false;

		return true;
	}

	return false;
}

bool Mob::CanThisClassDoubleAttack(void) const
{
	if(!IsClient()) {
		return(GetSkill(SkillDoubleAttack) > 0);
	} else {
		if(aabonuses.GiveDoubleAttack || itembonuses.GiveDoubleAttack || spellbonuses.GiveDoubleAttack) {
			return true;
		}
		return(CastToClient()->HasSkill(SkillDoubleAttack));
	}
}

bool Mob::CanThisClassTripleAttack() const
{
	if (!IsClient())
		return false; // When they added the real triple attack skill, mobs lost the ability to triple
	else
		return CastToClient()->HasSkill(SkillTripleAttack);
}

bool Mob::IsWarriorClass(void) const
{
	switch(GetClass())
	{
	case WARRIOR:
	case WARRIORGM:
	case ROGUE:
	case ROGUEGM:
	case MONK:
	case MONKGM:
	case PALADIN:
	case PALADINGM:
	case SHADOWKNIGHT:
	case SHADOWKNIGHTGM:
	case RANGER:
	case RANGERGM:
	case BEASTLORD:
	case BEASTLORDGM:
	case BERSERKER:
	case BERSERKERGM:
	case BARD:
	case BARDGM:
		{
			return true;
		}
	default:
		{
			return false;
		}
	}

}

bool Mob::CanThisClassParry(void) const
{
	if(!IsClient()) {
		return(GetSkill(SkillParry) > 0);
	} else {
		return(CastToClient()->HasSkill(SkillParry));
	}
}

bool Mob::CanThisClassDodge(void) const
{
	if(!IsClient()) {
		return(GetSkill(SkillDodge) > 0);
	} else {
		return(CastToClient()->HasSkill(SkillDodge));
	}
}

bool Mob::CanThisClassRiposte(void) const
{
	if(!IsClient()) {
		return(GetSkill(SkillRiposte) > 0);
	} else {
		return(CastToClient()->HasSkill(SkillRiposte));
	}
}

bool Mob::CanThisClassBlock(void) const
{
	if(!IsClient()) {
		return(GetSkill(SkillBlock) > 0);
	} else {
		return(CastToClient()->HasSkill(SkillBlock));
	}
}
/*
float Mob::GetReciprocalHeading(Mob* target) {
	float Result = 0;

	if(target) {
		// Convert to radians
		float h = (target->GetHeading() / 256.0f) * 6.283184f;

		// Calculate the reciprocal heading in radians
		Result = h + 3.141592f;

		// Convert back to eq heading from radians
		Result = (Result / 6.283184f) * 256.0f;
	}

	return Result;
}
*/
bool Mob::PlotPositionAroundTarget(Mob* target, float &x_dest, float &y_dest, float &z_dest, bool lookForAftArc) {
	bool Result = false;

	if(target) {
		float look_heading = 0;

		if(lookForAftArc)
			look_heading = GetReciprocalHeading(target->GetPosition());
		else
			look_heading = target->GetHeading();

		// Convert to sony heading to radians
		look_heading = (look_heading / 256.0f) * 6.283184f;

		float tempX = 0;
		float tempY = 0;
		float tempZ = 0;
		float tempSize = 0;
		const float rangeCreepMod = 0.25;
		const uint8 maxIterationsAllowed = 4;
		uint8 counter = 0;
		float rangeReduction= 0;

		tempSize = target->GetSize();
		rangeReduction = (tempSize * rangeCreepMod);

		while(tempSize > 0 && counter != maxIterationsAllowed) {
			tempX = GetX() + (tempSize * static_cast<float>(sin(double(look_heading))));
			tempY = GetY() + (tempSize * static_cast<float>(cos(double(look_heading))));
			tempZ = target->GetZ();

			if(!CheckLosFN(tempX, tempY, tempZ, tempSize)) {
				tempSize -= rangeReduction;
			}
			else {
				Result = true;
				break;
			}

			counter++;
		}

		if(!Result) {
			// Try to find an attack arc to position at from the opposite direction.
			look_heading += (3.141592 / 2);

			tempSize = target->GetSize();
			counter = 0;

			while(tempSize > 0 && counter != maxIterationsAllowed) {
				tempX = GetX() + (tempSize * static_cast<float>(sin(double(look_heading))));
				tempY = GetY() + (tempSize * static_cast<float>(cos(double(look_heading))));
				tempZ = target->GetZ();

				if(!CheckLosFN(tempX, tempY, tempZ, tempSize)) {
					tempSize -= rangeReduction;
				}
				else {
					Result = true;
					break;
				}

				counter++;
			}
		}

		if(Result) {
			x_dest = tempX;
			y_dest = tempY;
			z_dest = tempZ;
		}
	}

	return Result;
}

bool Mob::HateSummon() {
	// check if mob has ability to summon
	// 97% is the offical % that summoning starts on live, not 94
	// if the mob can summon and is charmed, it can only summon mobs it has LoS to
	Mob* mob_owner = nullptr;
	if(GetOwnerID())
		mob_owner = entity_list.GetMob(GetOwnerID());

	int summon_level = GetSpecialAbility(SPECATK_SUMMON);
	if(summon_level == 1 || summon_level == 2) {
		if(!GetTarget() || (mob_owner && mob_owner->IsClient() && !CheckLosFN(GetTarget()))) {
			return false;
		}
	} else {
		//unsupported summon level or OFF
		return false;
	}

	// validate hp
	int hp_ratio = GetSpecialAbilityParam(SPECATK_SUMMON, 1);
	hp_ratio = hp_ratio > 0 ? hp_ratio : 97;
	if(GetHPRatio() > static_cast<float>(hp_ratio)) {
		return false;
	}

	// now validate the timer
	int summon_timer_duration = GetSpecialAbilityParam(SPECATK_SUMMON, 0);
	summon_timer_duration = summon_timer_duration > 0 ? summon_timer_duration : 6000;
	Timer *timer = GetSpecialAbilityTimer(SPECATK_SUMMON);
	if (!timer)
	{
		StartSpecialAbilityTimer(SPECATK_SUMMON, summon_timer_duration);
	} else {
		if(!timer->Check())
			return false;

		timer->Start(summon_timer_duration);
	}

	// get summon target
	SetTarget(GetHateTop());
	if(target)
	{
		if(summon_level == 1) {
			entity_list.MessageClose(this, true, 500, MT_Say, "%s says,'You will not evade me, %s!' ", GetCleanName(), target->GetCleanName() );

			if (target->IsClient()) {
				target->CastToClient()->MovePC(zone->GetZoneID(), zone->GetInstanceID(), m_Position.x, m_Position.y, m_Position.z, target->GetHeading(), 0, SummonPC);
			}
			else {
#ifdef BOTS
				if(target && target->IsBot()) {
					// set pre summoning info to return to (to get out of melee range for caster)
					target->CastToBot()->SetHasBeenSummoned(true);
					target->CastToBot()->SetPreSummonLocation(glm::vec3(target->GetPosition()));

				}
#endif //BOTS
				target->GMMove(m_Position.x, m_Position.y, m_Position.z, target->GetHeading());
			}

			return true;
		} else if(summon_level == 2) {
			entity_list.MessageClose(this, true, 500, MT_Say, "%s says,'You will not evade me, %s!'", GetCleanName(), target->GetCleanName());
			GMMove(target->GetX(), target->GetY(), target->GetZ());
		}
	}
	return false;
}

void Mob::FaceTarget(Mob* MobToFace) {
	Mob* facemob = MobToFace;
	if(!facemob) {
		if(!GetTarget()) {
			return;
		}
		else {
			facemob = GetTarget();
		}
	}

	float oldheading = GetHeading();
	float newheading = CalculateHeadingToTarget(facemob->GetX(), facemob->GetY());
	if(oldheading != newheading) {
		SetHeading(newheading);
		if(moving)
			SendPosUpdate();
		else
		{
			SendPosition();
		}
	}

	if(IsNPC() && !IsEngaged()) {
		CastToNPC()->GetRefaceTimer()->Start(15000);
		CastToNPC()->GetRefaceTimer()->Enable();
	}
}

bool Mob::RemoveFromHateList(Mob* mob)
{
	SetRunAnimSpeed(0);
	bool bFound = false;
	if(IsEngaged())
	{
		bFound = hate_list.RemoveEntFromHateList(mob);
		if(hate_list.IsHateListEmpty())
		{
			AI_Event_NoLongerEngaged();
			zone->DelAggroMob();
			if (IsNPC() && !RuleB(Aggro, AllowTickPulling))
				ResetAssistCap();
		}
	}
	if(GetTarget() == mob)
	{
		SetTarget(hate_list.GetEntWithMostHateOnList(this));
	}

	return bFound;
}

void Mob::WipeHateList()
{
	if(IsEngaged())
	{
		hate_list.WipeHateList();
		AI_Event_NoLongerEngaged();
	}
	else
	{
		hate_list.WipeHateList();
	}
}

uint32 Mob::RandomTimer(int min,int max) {
	int r = 14000;
	if(min != 0 && max != 0 && min < max)
	{
		r = zone->random.Int(min, max);
	}
	return r;
}

uint32 NPC::GetEquipment(uint8 material_slot) const
{
	if(material_slot > 8)
		return 0;
	int16 invslot = Inventory::CalcSlotFromMaterial(material_slot);
	if (invslot == INVALID_INDEX)
		return 0;
	return equipment[invslot];
}

void Mob::SendArmorAppearance(Client *one_client)
{
	// one_client of 0 means sent to all clients
	//
	// Despite the fact that OP_NewSpawn and OP_ZoneSpawns include the
	// armor being worn and its mats, the client doesn't update the display
	// on arrival of these packets reliably.
	//
	// Send Wear changes if mob is a PC race and item is an armor slot.
	// The other packets work for primary/secondary.

	if (IsPlayerRace(race))
	{
		if (!IsClient())
		{
			const Item_Struct *item;
			for (int i = 0; i < 7; ++i)
			{
				item = database.GetItem(GetEquipment(i));
				if (item != 0)
				{
					SendWearChange(i, one_client);
				}
			}
		}
	}
}

void Mob::SendWearChange(uint8 material_slot, Client *one_client)
{
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_WearChange, sizeof(WearChange_Struct));
	WearChange_Struct* wc = (WearChange_Struct*)outapp->pBuffer;

	wc->spawn_id = GetID();
	wc->material = GetEquipmentMaterial(material_slot);
	wc->elite_material = IsEliteMaterialItem(material_slot);
	wc->hero_forge_model = GetHerosForgeModel(material_slot);
	wc->color.Color = GetEquipmentColor(material_slot);
	wc->wear_slot_id = material_slot;

	if (!one_client)
	{
		entity_list.QueueClients(this, outapp);
	}
	else
	{
		one_client->QueuePacket(outapp, false, Client::CLIENT_CONNECTED);
	}

	safe_delete(outapp);
}

void Mob::SendTextureWC(uint8 slot, uint16 texture, uint32 hero_forge_model, uint32 elite_material, uint32 unknown06, uint32 unknown18)
{
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_WearChange, sizeof(WearChange_Struct));
	WearChange_Struct* wc = (WearChange_Struct*)outapp->pBuffer;

	wc->spawn_id = this->GetID();
	wc->material = texture;
	if (this->IsClient())
		wc->color.Color = GetEquipmentColor(slot);
	else
		wc->color.Color = this->GetArmorTint(slot);
	wc->wear_slot_id = slot;

	wc->unknown06 = unknown06;
	wc->elite_material = elite_material;
	wc->hero_forge_model = hero_forge_model;
	wc->unknown18 = unknown18;


	entity_list.QueueClients(this, outapp);
	safe_delete(outapp);
}

void Mob::SetSlotTint(uint8 material_slot, uint8 red_tint, uint8 green_tint, uint8 blue_tint)
{
	uint32 color;
	color = (red_tint & 0xFF) << 16;
	color |= (green_tint & 0xFF) << 8;
	color |= (blue_tint & 0xFF);
	color |= (color) ? (0xFF << 24) : 0;
	armor_tint[material_slot] = color;

	EQApplicationPacket* outapp = new EQApplicationPacket(OP_WearChange, sizeof(WearChange_Struct));
	WearChange_Struct* wc = (WearChange_Struct*)outapp->pBuffer;

	wc->spawn_id = this->GetID();
	wc->material = GetEquipmentMaterial(material_slot);
	wc->hero_forge_model = GetHerosForgeModel(material_slot);
	wc->color.Color = color;
	wc->wear_slot_id = material_slot;

	entity_list.QueueClients(this, outapp);
	safe_delete(outapp);
}

void Mob::WearChange(uint8 material_slot, uint16 texture, uint32 color, uint32 hero_forge_model)
{
	armor_tint[material_slot] = color;

	EQApplicationPacket* outapp = new EQApplicationPacket(OP_WearChange, sizeof(WearChange_Struct));
	WearChange_Struct* wc = (WearChange_Struct*)outapp->pBuffer;

	wc->spawn_id = this->GetID();
	wc->material = texture;
	wc->hero_forge_model = hero_forge_model;
	wc->color.Color = color;
	wc->wear_slot_id = material_slot;

	entity_list.QueueClients(this, outapp);
	safe_delete(outapp);
}

int32 Mob::GetEquipmentMaterial(uint8 material_slot) const
{
	uint32 equipmaterial = 0;
	int32 ornamentationAugtype = RuleI(Character, OrnamentationAugmentType);
	const Item_Struct *item;
	item = database.GetItem(GetEquipment(material_slot));

	if (item != 0)
	{
		// For primary and secondary we need the model, not the material
		if (material_slot == MaterialPrimary || material_slot == MaterialSecondary)
		{
			if (this->IsClient())
			{
				int16 invslot = Inventory::CalcSlotFromMaterial(material_slot);
				if (invslot == INVALID_INDEX)
				{
					return 0;
				}
				const ItemInst* inst = CastToClient()->m_inv[invslot];
				if (inst)
				{
					if (inst->GetOrnamentationAug(ornamentationAugtype))
					{
						item = inst->GetOrnamentationAug(ornamentationAugtype)->GetItem();
						if (item && strlen(item->IDFile) > 2)
						{
							equipmaterial = atoi(&item->IDFile[2]);
						}
					}
					else if (inst->GetOrnamentationIDFile())
					{
						equipmaterial = inst->GetOrnamentationIDFile();
					}
				}
			}

			if (equipmaterial == 0 && strlen(item->IDFile) > 2)
			{
				equipmaterial = atoi(&item->IDFile[2]);
			}
		}
		else
		{
			equipmaterial = item->Material;
		}
	}

	return equipmaterial;
}

int32 Mob::GetHerosForgeModel(uint8 material_slot) const
{
	uint32 HeroModel = 0;
	if (material_slot >= 0 && material_slot < MaterialPrimary)
	{
		uint32 ornamentationAugtype = RuleI(Character, OrnamentationAugmentType);
		const Item_Struct *item;
		item = database.GetItem(GetEquipment(material_slot));
		int16 invslot = Inventory::CalcSlotFromMaterial(material_slot);

		if (item != 0 && invslot != INVALID_INDEX)
		{
			if (IsClient())
			{
				const ItemInst* inst = CastToClient()->m_inv[invslot];
				if (inst)
				{
					if (inst->GetOrnamentationAug(ornamentationAugtype))
					{
						item = inst->GetOrnamentationAug(ornamentationAugtype)->GetItem();
						HeroModel = item->HerosForgeModel;
					}
					else if (inst->GetOrnamentHeroModel())
					{
						HeroModel = inst->GetOrnamentHeroModel();
					}
				}
			}

			if (HeroModel == 0)
			{
				HeroModel = item->HerosForgeModel;
			}
		}

		if (IsNPC())
		{
			HeroModel = CastToNPC()->GetHeroForgeModel();
			// Robes require full model number, and should only be sent to chest, arms, wrists, and legs slots
			if (HeroModel > 1000 && material_slot != 1 && material_slot != 2 && material_slot != 3 && material_slot != 5)
			{
				HeroModel = 0;
			}
		}
	}

	// Auto-Convert Hero Model to match the slot
	// Otherwise, use the exact Model if model is > 999
	// Robes for example are 11607 to 12107 in RoF
	if (HeroModel > 0 && HeroModel < 1000)
	{
		HeroModel *= 100;
		HeroModel += material_slot;
	}

	return HeroModel;
}

uint32 Mob::GetEquipmentColor(uint8 material_slot) const
{
	const Item_Struct *item;

	if (armor_tint[material_slot])
	{
		return armor_tint[material_slot];
	}

	item = database.GetItem(GetEquipment(material_slot));
	if (item != 0)
		return item->Color;

	return 0;
}

uint32 Mob::IsEliteMaterialItem(uint8 material_slot) const
{
	const Item_Struct *item;

	item = database.GetItem(GetEquipment(material_slot));
	if(item != 0)
	{
		return item->EliteMaterial;
	}

	return 0;
}

// works just like a printf
void Mob::Say(const char *format, ...)
{
	char buf[1000];
	va_list ap;

	va_start(ap, format);
	vsnprintf(buf, 1000, format, ap);
	va_end(ap);

	Mob* talker = this;
	if(spellbonuses.VoiceGraft != 0) {
		if(spellbonuses.VoiceGraft == GetPetID())
			talker = entity_list.GetMob(spellbonuses.VoiceGraft);
		else
			spellbonuses.VoiceGraft = 0;
	}

	if(!talker)
		talker = this;

	entity_list.MessageClose_StringID(talker, false, 200, 10,
		GENERIC_SAY, GetCleanName(), buf);
}

//
// this is like the above, but the first parameter is a string id
//
void Mob::Say_StringID(uint32 string_id, const char *message3, const char *message4, const char *message5, const char *message6, const char *message7, const char *message8, const char *message9)
{
	char string_id_str[10];

	snprintf(string_id_str, 10, "%d", string_id);

	entity_list.MessageClose_StringID(this, false, 200, 10,
		GENERIC_STRINGID_SAY, GetCleanName(), string_id_str, message3, message4, message5,
		message6, message7, message8, message9
	);
}

void Mob::Say_StringID(uint32 type, uint32 string_id, const char *message3, const char *message4, const char *message5, const char *message6, const char *message7, const char *message8, const char *message9)
{
	char string_id_str[10];

	snprintf(string_id_str, 10, "%d", string_id);

	entity_list.MessageClose_StringID(this, false, 200, type,
		GENERIC_STRINGID_SAY, GetCleanName(), string_id_str, message3, message4, message5,
		message6, message7, message8, message9
	);
}

void Mob::Shout(const char *format, ...)
{
	char buf[1000];
	va_list ap;

	va_start(ap, format);
	vsnprintf(buf, 1000, format, ap);
	va_end(ap);

	entity_list.Message_StringID(this, false, MT_Shout,
		GENERIC_SHOUT, GetCleanName(), buf);
}

void Mob::Emote(const char *format, ...)
{
	char buf[1000];
	va_list ap;

	va_start(ap, format);
	vsnprintf(buf, 1000, format, ap);
	va_end(ap);

	entity_list.MessageClose_StringID(this, false, 200, 10,
		GENERIC_EMOTE, GetCleanName(), buf);
}

void Mob::QuestJournalledSay(Client *QuestInitiator, const char *str)
{
		entity_list.QuestJournalledSayClose(this, QuestInitiator, 200, GetCleanName(), str);
}

const char *Mob::GetCleanName()
{
	if(!strlen(clean_name))
	{
		CleanMobName(GetName(), clean_name);
	}

	return clean_name;
}

// hp event
void Mob::SetNextHPEvent( int hpevent )
{
	nexthpevent = hpevent;
}

void Mob::SetNextIncHPEvent( int inchpevent )
{
	nextinchpevent = inchpevent;
}
//warp for quest function,from sandy
void Mob::Warp(const glm::vec3& location)
{
	if(IsNPC())
		entity_list.ProcessMove(CastToNPC(), location.x, location.y, location.z);

	m_Position = glm::vec4(location, m_Position.w);

	Mob* target = GetTarget();
	if (target)
		FaceTarget( target );

	SendPosition();
}

int16 Mob::GetResist(uint8 type) const
{
	if (IsNPC())
	{
		if (type == 1)
			return MR + spellbonuses.MR + itembonuses.MR;
		else if (type == 2)
			return FR + spellbonuses.FR + itembonuses.FR;
		else if (type == 3)
			return CR + spellbonuses.CR + itembonuses.CR;
		else if (type == 4)
			return PR + spellbonuses.PR + itembonuses.PR;
		else if (type == 5)
			return DR + spellbonuses.DR + itembonuses.DR;
	}
	else if (IsClient())
	{
		if (type == 1)
			return CastToClient()->GetMR();
		else if (type == 2)
			return CastToClient()->GetFR();
		else if (type == 3)
			return CastToClient()->GetCR();
		else if (type == 4)
			return CastToClient()->GetPR();
		else if (type == 5)
			return CastToClient()->GetDR();
	}
	return 25;
}

uint32 Mob::GetLevelHP(uint8 tlevel)
{
	int multiplier = 0;
	if (tlevel < 10)
	{
		multiplier = tlevel*20;
	}
	else if (tlevel < 20)
	{
		multiplier = tlevel*25;
	}
	else if (tlevel < 40)
	{
		multiplier = tlevel*tlevel*12*((tlevel*2+60)/100)/10;
	}
	else if (tlevel < 45)
	{
		multiplier = tlevel*tlevel*15*((tlevel*2+60)/100)/10;
	}
	else if (tlevel < 50)
	{
		multiplier = tlevel*tlevel*175*((tlevel*2+60)/100)/100;
	}
	else
	{
		multiplier = tlevel*tlevel*2*((tlevel*2+60)/100)*(1+((tlevel-50)*20/10));
	}
	return multiplier;
}

int32 Mob::GetActSpellCasttime(uint16 spell_id, int32 casttime) {

	int32 cast_reducer = 0;
	cast_reducer += GetFocusEffect(focusSpellHaste, spell_id);

	if (level >= 60 && casttime > 1000)
	{
		casttime = casttime / 2;
		if (casttime < 1000)
			casttime = 1000;
	} else if (level >= 50 && casttime > 1000) {
		int32 cast_deduction = (casttime*(level - 49))/5;
		if (cast_deduction > casttime/2)
			casttime /= 2;
		else
			casttime -= cast_deduction;
	}

	casttime = (casttime*(100 - cast_reducer)/100);

	return(casttime);
}

void Mob::ExecWeaponProc(const ItemInst *inst, uint16 spell_id, Mob *on, int level_override) {

	// Changed proc targets to look up based on the spells goodEffect flag.
	// This should work for the majority of weapons.
	if(spell_id == SPELL_UNKNOWN || on->GetSpecialAbility(NO_HARM_FROM_CLIENT)) {
		//This is so 65535 doesn't get passed to the client message and to logs because it is not relavant information for debugging.
		return;
	}

	if (IsNoCast())
		return;

	if(!IsValidSpell(spell_id)) { // Check for a valid spell otherwise it will crash through the function
		if(IsClient()){
			Message(0, "Invalid spell proc %u", spell_id);
			Log.Out(Logs::Detail, Logs::Spells, "Player %s, Weapon Procced invalid spell %u", this->GetName(), spell_id);
		}
		return;
	}

	if(inst && IsClient()) {
		//const cast is dirty but it would require redoing a ton of interfaces at this point
		//It should be safe as we don't have any truly const ItemInst floating around anywhere.
		//So we'll live with it for now
		int i = parse->EventItem(EVENT_WEAPON_PROC, CastToClient(), const_cast<ItemInst*>(inst), on, "", spell_id);
		if(i != 0) {
			return;
		}
	}

	bool twinproc = false;
	int32 twinproc_chance = 0;

	if(IsClient())
		twinproc_chance = CastToClient()->GetFocusEffect(focusTwincast, spell_id);

	if(twinproc_chance && zone->random.Roll(twinproc_chance))
		twinproc = true;

	if (IsBeneficialSpell(spell_id)) {
		SpellFinished(spell_id, this, 10, 0, -1, spells[spell_id].ResistDiff, true, level_override);
		if(twinproc)
			SpellOnTarget(spell_id, this, false, false, 0, true, level_override);
	}
	else if(!(on->IsClient() && on->CastToClient()->dead)) { //dont proc on dead clients
		SpellFinished(spell_id, on, 10, 0, -1, spells[spell_id].ResistDiff, true, level_override);
		if(twinproc)
			SpellOnTarget(spell_id, on, false, false, 0, true, level_override);
	}
	return;
}

uint32 Mob::GetZoneID() const {
	return(zone->GetZoneID());
}

int Mob::GetHaste()
{
	// See notes in Client::CalcHaste
	// Need to check if the effect of inhibit melee differs for NPCs

	/*
	if (spellbonuses.haste < 0) {
		if (-spellbonuses.haste <= spellbonuses.inhibitmelee)
			return 100 - spellbonuses.inhibitmelee;
		else
			return 100 + spellbonuses.haste;
	}
	*/

	//C!Kayen Altered from above to account for stackable slow effect
	if (spellbonuses.haste < 0 || spellbonuses.hastetype5 < 0) {
		int slow1 = 0;
		int slow2 = 0;
		if (spellbonuses.haste > 0) { slow1 = 0; } else { slow1 = spellbonuses.haste; }
		if (spellbonuses.hastetype5 > 0) { slow2 = 0; } else { slow2 = spellbonuses.hastetype5; }
		int total_slow = slow1 + slow2;
		if (-total_slow <= spellbonuses.inhibitmelee)
			return 100 - spellbonuses.inhibitmelee;
		else
			return 100 + total_slow; 
	}

	if (spellbonuses.haste == 0 && spellbonuses.inhibitmelee)
		return 100 - spellbonuses.inhibitmelee;

	int h = 0;
	int cap = 0;
	int level = GetLevel();

	if (spellbonuses.haste)
		h += spellbonuses.haste + spellbonuses.hastetype5 - spellbonuses.inhibitmelee; //C!Kayen
	if (spellbonuses.hastetype2 && level > 49)
		h += spellbonuses.hastetype2 > 10 ? 10 : spellbonuses.hastetype2;

	// 26+ no cap, 1-25 10
	if (level > 25) // 26+
		h += itembonuses.haste;
	else // 1-25
		h += itembonuses.haste > 10 ? 10 : itembonuses.haste;

	// 60+ 100, 51-59 85, 1-50 level+25
	if (level > 59) // 60+
		cap = RuleI(Character, HasteCap);
	else if (level > 50) // 51-59
		cap = 85;
	else // 1-50
		cap = level + 25;

	if(h > cap)
		h = cap;

	// 51+ 25 (despite there being higher spells...), 1-50 10
	if (level > 50) // 51+
		h += spellbonuses.hastetype3 > 25 ? 25 : spellbonuses.hastetype3;
	else // 1-50
		h += spellbonuses.hastetype3 > 10 ? 10 : spellbonuses.hastetype3;

	h += ExtraHaste;	//GM granted haste.

	return 100 + h;
}

void Mob::SetTarget(Mob* mob) {
	if (target == mob) return;
	target = mob;
	entity_list.UpdateHoTT(this);
	if(IsNPC())
		parse->EventNPC(EVENT_TARGET_CHANGE, CastToNPC(), mob, "", 0);
	else if (IsClient())
		parse->EventPlayer(EVENT_TARGET_CHANGE, CastToClient(), "", 0);

	if(IsPet() && GetOwner() && GetOwner()->IsClient())
		GetOwner()->CastToClient()->UpdateXTargetType(MyPetTarget, mob);
}

float Mob::FindGroundZ(float new_x, float new_y, float z_offset)
{
	float ret = -999999;
	if (zone->zonemap != nullptr)
	{
		glm::vec3 me;
		me.x = new_x;
		me.y = new_y;
		me.z = m_Position.z + z_offset;
		glm::vec3 hit;
		float best_z = zone->zonemap->FindBestZ(me, &hit);
		if (best_z != -999999)
		{
			ret = best_z;
		}
	}
	return ret;
}

// Copy of above function that isn't protected to be exported to Perl::Mob
float Mob::GetGroundZ(float new_x, float new_y, float z_offset)
{
	float ret = -999999;
	if (zone->zonemap != 0)
	{
		glm::vec3 me;
		me.x = new_x;
		me.y = new_y;
		me.z = m_Position.z+z_offset;
		glm::vec3 hit;
		float best_z = zone->zonemap->FindBestZ(me, &hit);
		if (best_z != -999999)
		{
			ret = best_z;
		}
	}
	return ret;
}

//helper function for npc AI; needs to be mob:: cause we need to be able to count buffs on other clients and npcs
int Mob::CountDispellableBuffs()
{
	int val = 0;
	int buff_count = GetMaxTotalSlots();
	for(int x = 0; x < buff_count; x++)
	{
		if(!IsValidSpell(buffs[x].spellid))
			continue;

		if(buffs[x].counters)
			continue;

		if(spells[buffs[x].spellid].goodEffect == 0)
			continue;

		if(buffs[x].spellid != SPELL_UNKNOWN &&	spells[buffs[x].spellid].buffdurationformula != DF_Permanent)
			val++;
	}
	return val;
}

// Returns the % that a mob is snared (as a positive value). -1 means not snared
int Mob::GetSnaredAmount()
{
	int worst_snare = -1;

	int buff_count = GetMaxTotalSlots();
	for (int i = 0; i < buff_count; i++)
	{
		if (!IsValidSpell(buffs[i].spellid))
			continue;

		for(int j = 0; j < EFFECT_COUNT; j++)
		{
			if (spells[buffs[i].spellid].effectid[j] == SE_MovementSpeed)
			{
				int val = CalcSpellEffectValue_formula(spells[buffs[i].spellid].formula[j], spells[buffs[i].spellid].base[j], spells[buffs[i].spellid].max[j], buffs[i].casterlevel, buffs[i].spellid);
				//int effect = CalcSpellEffectValue(buffs[i].spellid, spells[buffs[i].spellid].effectid[j], buffs[i].casterlevel);
				if (val < 0 && std::abs(val) > worst_snare)
					worst_snare = std::abs(val);
			}
		}
	}

	return worst_snare;
}

void Mob::TriggerDefensiveProcs(Mob *on, uint16 hand, bool FromSkillProc, int damage)
{
	if (!on)
		return;

	if (!FromSkillProc)
		on->TryDefensiveProc(this, hand);

	//Defensive Skill Procs
	if (damage < 0 && damage >= -4) {
		uint16 skillinuse = 0;
		switch (damage) {
			case (-1):
				skillinuse = SkillBlock;
			break;

			case (-2):
				skillinuse = SkillParry;
			break;

			case (-3):
				skillinuse = SkillRiposte;
			break;

			case (-4):
				skillinuse = SkillDodge;
			break;
		}

		if (on->HasSkillProcs())
			on->TrySkillProc(this, skillinuse, 0, false, hand, true);

		if (on->HasSkillProcSuccess())
			on->TrySkillProc(this, skillinuse, 0, true, hand, true);
	}
}

void Mob::SetDelta(const glm::vec4& delta) {
	m_Delta = delta;
}

void Mob::SetEntityVariable(const char *id, const char *m_var)
{
	std::string n_m_var = m_var;
	m_EntityVariables[id] = n_m_var;
}

const char* Mob::GetEntityVariable(const char *id)
{
	std::map<std::string, std::string>::iterator iter = m_EntityVariables.find(id);
	if(iter != m_EntityVariables.end())
	{
		return iter->second.c_str();
	}
	return nullptr;
}

bool Mob::EntityVariableExists(const char *id)
{
	std::map<std::string, std::string>::iterator iter = m_EntityVariables.find(id);
	if(iter != m_EntityVariables.end())
	{
		return true;
	}
	return false;
}

void Mob::SetFlyMode(uint8 flymode)
{
	if(IsClient() && flymode >= 0 && flymode < 3)
	{
		this->SendAppearancePacket(AT_Levitate, flymode);
	}
	else if(IsNPC() && flymode >= 0 && flymode <= 3)
	{
		this->SendAppearancePacket(AT_Levitate, flymode);
		this->CastToNPC()->SetFlyMode(flymode);
	}
}

bool Mob::IsNimbusEffectActive(uint32 nimbus_effect)
{
	if(nimbus_effect1 == nimbus_effect || nimbus_effect2 == nimbus_effect || nimbus_effect3 == nimbus_effect)
	{
		return true;
	}
	return false;
}

void Mob::SetNimbusEffect(uint32 nimbus_effect)
{
	if(nimbus_effect1 == 0)
	{
		nimbus_effect1 = nimbus_effect;
	}
	else if(nimbus_effect2 == 0)
	{
		nimbus_effect2 = nimbus_effect;
	}
	else
	{
		nimbus_effect3 = nimbus_effect;
	}
}

void Mob::TryTriggerOnCast(uint32 spell_id, bool aa_trigger)
{
	if(!IsValidSpell(spell_id))
			return;

	if (aabonuses.SpellTriggers[0] || spellbonuses.SpellTriggers[0] || itembonuses.SpellTriggers[0]){

		for(int i = 0; i < MAX_SPELL_TRIGGER; i++){

			if(aabonuses.SpellTriggers[i] && IsClient())
				TriggerOnCast(aabonuses.SpellTriggers[i], spell_id,1);

			if(spellbonuses.SpellTriggers[i])
				TriggerOnCast(spellbonuses.SpellTriggers[i], spell_id,0);

			if(itembonuses.SpellTriggers[i])
				TriggerOnCast(spellbonuses.SpellTriggers[i], spell_id,0);
		}
	}
}


void Mob::TriggerOnCast(uint32 focus_spell, uint32 spell_id, bool aa_trigger)
{
	if(!IsValidSpell(focus_spell) || !IsValidSpell(spell_id))
		return;

	uint32 trigger_spell_id = 0;

	if (aa_trigger && IsClient()) {
		// focus_spell = aaid
		auto rank = zone->GetAlternateAdvancementRank(focus_spell);
		if (rank)
			trigger_spell_id = CastToClient()->CalcAAFocus(focusTriggerOnCast, *rank, spell_id);

		if(IsValidSpell(trigger_spell_id) && GetTarget())
			SpellFinished(trigger_spell_id, GetTarget(), 10, 0, -1, spells[trigger_spell_id].ResistDiff);
	}

	else{
		trigger_spell_id = CalcFocusEffect(focusTriggerOnCast, focus_spell, spell_id);

		if(IsValidSpell(trigger_spell_id) && GetTarget()){
			SpellFinished(trigger_spell_id, GetTarget(),10, 0, -1, spells[trigger_spell_id].ResistDiff);
			CheckNumHitsRemaining(NumHit::MatchingSpells, -1, focus_spell);
		}
	}
}

bool Mob::TrySpellTrigger(Mob *target, uint32 spell_id, int effect)
{
	if(!target || !IsValidSpell(spell_id))
		return false;

	int spell_trig = 0;
	// Count all the percentage chances to trigger for all effects
	for(int i = 0; i < EFFECT_COUNT; i++)
	{
		if (spells[spell_id].effectid[i] == SE_SpellTrigger)
			spell_trig += spells[spell_id].base[i];
	}
	// If all the % add to 100, then only one of the effects can fire but one has to fire.
	if (spell_trig == 100)
	{
		int trig_chance = 100;
		for(int i = 0; i < EFFECT_COUNT; i++)
		{
			if (spells[spell_id].effectid[i] == SE_SpellTrigger)
			{
				if(zone->random.Int(0, trig_chance) <= spells[spell_id].base[i])
				{
					// If we trigger an effect then its over.
					if (IsValidSpell(spells[spell_id].base2[i])){
						SpellFinished(spells[spell_id].base2[i], target, 10, 0, -1, spells[spells[spell_id].base2[i]].ResistDiff);
						return true;
					}
				}
				else
				{
					// Increase the chance to fire for the next effect, if all effects fail, the final effect will fire.
					trig_chance -= spells[spell_id].base[i];
				}
			}

		}
	}
	// if the chances don't add to 100, then each effect gets a chance to fire, chance for no trigger as well.
	else
	{
		if(zone->random.Int(0, 100) <= spells[spell_id].base[effect])
		{
			if (IsValidSpell(spells[spell_id].base2[effect])){
				SpellFinished(spells[spell_id].base2[effect], target, 10, 0, -1, spells[spells[spell_id].base2[effect]].ResistDiff);
				return true; //Only trigger once of these per spell effect.
			}
		}
	}
	return false;
}

void Mob::TryTriggerOnValueAmount(bool IsHP, bool IsMana, bool IsEndur, bool IsPet)
{
	/*
	At present time there is no obvious difference between ReqTarget and ReqCaster
	ReqTarget is typically used in spells cast on a target where the trigger occurs on that target.
	ReqCaster is typically self only spells where the triggers on self.
	Regardless both trigger on the owner of the buff.
	*/

	/*
	Base2 Range: 1004	 = Below < 80% HP
	Base2 Range: 500-520 = Below (base2 - 500)*5 HP
	Base2 Range: 521	 = Below (?) Mana UKNOWN - Will assume its 20% unless proven otherwise
	Base2 Range: 522	 = Below (40%) Endurance
	Base2 Range: 523	 = Below (40%) Mana
	Base2 Range: 220-?	 = Number of pets on hatelist to trigger (base2 - 220) (Set at 30 pets max for now)
	38311 = < 10% mana;
	*/

	if (!spellbonuses.TriggerOnValueAmount)
		return;

	if (spellbonuses.TriggerOnValueAmount){

		int buff_count = GetMaxTotalSlots();

		for(int e = 0; e < buff_count; e++){

			uint32 spell_id = buffs[e].spellid;

			if (IsValidSpell(spell_id)){

				for(int i = 0; i < EFFECT_COUNT; i++){

					if ((spells[spell_id].effectid[i] == SE_TriggerOnReqTarget) || (spells[spell_id].effectid[i] == SE_TriggerOnReqCaster)) {

						int base2 = spells[spell_id].base2[i];
						bool use_spell = false;

						if (IsHP){
							if ((base2 >= 500 && base2 <= 520) && GetHPRatio() < (base2 - 500)*5)
								use_spell = true;

							else if (base2 = 1004 && GetHPRatio() < 80)
								use_spell = true;
						}

						else if (IsMana){
							if ( (base2 = 521 && GetManaRatio() < 20) || (base2 = 523 && GetManaRatio() < 40))
								use_spell = true;

							else if (base2 = 38311 && GetManaRatio() < 10)
								use_spell = true;
						}

						else if (IsEndur){
							if (base2 = 522 && GetEndurancePercent() < 40){
								use_spell = true;
							}
						}

						else if (IsPet){
							int count = hate_list.GetSummonedPetCountOnHateList(this);
							if ((base2 >= 220 && base2 <= 250) && count >= (base2 - 220)){
								use_spell = true;
							}
						}

						if (use_spell){
							SpellFinished(spells[spell_id].base[i], this, 10, 0, -1, spells[spell_id].ResistDiff);

							if(!TryFadeEffect(e))
								BuffFadeBySlot(e);
						}
					}
				}
			}
		}
	}
}


//Twincast Focus effects should stack across different types (Spell, AA - when implemented ect)
void Mob::TryTwincast(Mob *caster, Mob *target, uint32 spell_id)
{
	if(!IsValidSpell(spell_id))
		return;

	if(IsClient())
	{
		int32 focus = CastToClient()->GetFocusEffect(focusTwincast, spell_id);

		if (focus > 0)
		{
			if(zone->random.Roll(focus))
			{
				Message(MT_Spells,"You twincast %s!",spells[spell_id].name);
				SpellFinished(spell_id, target, 10, 0, -1, spells[spell_id].ResistDiff);
			}
		}
	}

	//Retains function for non clients
	else if (spellbonuses.FocusEffects[focusTwincast] || itembonuses.FocusEffects[focusTwincast])
	{
		int buff_count = GetMaxTotalSlots();
		for(int i = 0; i < buff_count; i++)
		{
			if(IsEffectInSpell(buffs[i].spellid, SE_FcTwincast))
			{
				int32 focus = CalcFocusEffect(focusTwincast, buffs[i].spellid, spell_id);
				if(focus > 0)
				{
					if(zone->random.Roll(focus))
					{
						SpellFinished(spell_id, target, 10, 0, -1, spells[spell_id].ResistDiff);
					}
				}
			}
		}
	}
}

int32 Mob::GetVulnerability(Mob* caster, uint32 spell_id, uint32 ticsremaining)
{
	if (!IsValidSpell(spell_id))
		return 0;

	if (!caster)
		return 0;

	int32 value = 0;

	//Apply innate vulnerabilities
	if (Vulnerability_Mod[GetSpellResistType(spell_id)] != 0)
		value = Vulnerability_Mod[GetSpellResistType(spell_id)];


	else if (Vulnerability_Mod[HIGHEST_RESIST+1] != 0)
		value = Vulnerability_Mod[HIGHEST_RESIST+1];

	//Apply spell derived vulnerabilities
	if (spellbonuses.FocusEffects[focusSpellVulnerability]){

		int32 tmp_focus = 0;
		int tmp_buffslot = -1;

		int buff_count = GetMaxTotalSlots();
		for(int i = 0; i < buff_count; i++) {

			if((IsValidSpell(buffs[i].spellid) && IsEffectInSpell(buffs[i].spellid, SE_FcSpellVulnerability))){

				int32 focus = caster->CalcFocusEffect(focusSpellVulnerability, buffs[i].spellid, spell_id);

				if (!focus)
					continue;

				if (tmp_focus && focus > tmp_focus){
					tmp_focus = focus;
					tmp_buffslot = i;
				}

				else if (!tmp_focus){
					tmp_focus = focus;
					tmp_buffslot = i;
				}

			}
		}

		if (tmp_focus < -99)
			tmp_focus = -99;

		value += tmp_focus;

		if (tmp_buffslot >= 0)
			CheckNumHitsRemaining(NumHit::MatchingSpells, tmp_buffslot);
	}

	value += GetSpellResistTypeDmgBonus(); //C!Kayen
	value += spellbonuses.IncomingSpellDmgPct[GetSpellResistType(spell_id)] + spellbonuses.IncomingSpellDmgPct[HIGHEST_RESIST]; //C!Kayen
	return value;
}

int16 Mob::GetSkillDmgTaken(const SkillUseTypes skill_used)
{
	int skilldmg_mod = 0;

	// All skill dmg mod + Skill specific
	skilldmg_mod += itembonuses.SkillDmgTaken[HIGHEST_SKILL+1] + spellbonuses.SkillDmgTaken[HIGHEST_SKILL+1] +
					itembonuses.SkillDmgTaken[skill_used] + spellbonuses.SkillDmgTaken[skill_used];

	skilldmg_mod += SkillDmgTaken_Mod[skill_used] + SkillDmgTaken_Mod[HIGHEST_SKILL+1];

	skilldmg_mod += GetWpnSkillDmgBonusAmt(); //C!Kayen
	skilldmg_mod += GetScaleMitigationNumhits(); //C!Kayen

	if(skilldmg_mod < -100)
		skilldmg_mod = -100;

	return skilldmg_mod;
}

int16 Mob::GetHealRate(uint16 spell_id, Mob* caster) {

	int16 heal_rate = 0;

	heal_rate += itembonuses.HealRate + spellbonuses.HealRate + aabonuses.HealRate;
	heal_rate += GetFocusIncoming(focusFcHealPctIncoming, SE_FcHealPctIncoming, caster, spell_id);

	if(heal_rate < -99)
		heal_rate = -99;

	return heal_rate;
}

bool Mob::TryFadeEffect(int slot)
{
	if(IsValidSpell(buffs[slot].spellid))
	{
		for(int i = 0; i < EFFECT_COUNT; i++)
		{
			if (spells[buffs[slot].spellid].effectid[i] == SE_CastOnFadeEffectAlways ||
				spells[buffs[slot].spellid].effectid[i] == SE_CastOnRuneFadeEffect)
			{
				uint16 spell_id = spells[buffs[slot].spellid].base[i];
				BuffFadeBySlot(slot);

				if(spell_id)
				{

					if(spell_id == SPELL_UNKNOWN)
						return false;

					if(IsValidSpell(spell_id))
					{
						if (IsBeneficialSpell(spell_id)) {
							SpellFinished(spell_id, this, 10, 0, -1, spells[spell_id].ResistDiff);
						}
						else if(!(IsClient() && CastToClient()->dead)) {
							SpellFinished(spell_id, this, 10, 0, -1, spells[spell_id].ResistDiff);
						}
						return true;
					}
				}
			}
		}
	}
	return false;
}

void Mob::TrySympatheticProc(Mob *target, uint32 spell_id)
{
	if(target == nullptr || !IsValidSpell(spell_id) || !IsClient())
		return;

	uint16 focus_spell = CastToClient()->GetSympatheticFocusEffect(focusSympatheticProc,spell_id);

	if(!IsValidSpell(focus_spell))
		return;

	uint16 focus_trigger = GetSympatheticSpellProcID(focus_spell);

	if(!IsValidSpell(focus_trigger))
		return;

	// For beneficial spells, if the triggered spell is also beneficial then proc it on the target
	// if the triggered spell is detrimental, then it will trigger on the caster(ie cursed items)
	if(IsBeneficialSpell(spell_id))
	{
		if(IsBeneficialSpell(focus_trigger))
			SpellFinished(focus_trigger, target);

		else
			SpellFinished(focus_trigger, this, 10, 0, -1, spells[focus_trigger].ResistDiff);
	}
	// For detrimental spells, if the triggered spell is beneficial, then it will land on the caster
	// if the triggered spell is also detrimental, then it will land on the target
	else
	{
		if(IsBeneficialSpell(focus_trigger))
			SpellFinished(focus_trigger, this);

		else
			SpellFinished(focus_trigger, target, 10, 0, -1, spells[focus_trigger].ResistDiff);
	}

	CheckNumHitsRemaining(NumHit::MatchingSpells, -1, focus_spell);
}

int32 Mob::GetItemStat(uint32 itemid, const char *identifier)
{
	const ItemInst* inst = database.CreateItem(itemid);
	if (!inst)
		return 0;

	const Item_Struct* item = inst->GetItem();
	if (!item)
		return 0;

	if (!identifier)
		return 0;

	int32 stat = 0;

	std::string id = identifier;
	for(uint32 i = 0; i < id.length(); ++i)
	{
		id[i] = tolower(id[i]);
	}

	if (id == "itemclass")
		stat = int32(item->ItemClass);
	if (id == "id")
		stat = int32(item->ID);
	if (id == "idfile")
		stat = atoi(&item->IDFile[2]);
	if (id == "weight")
		stat = int32(item->Weight);
	if (id == "norent")
		stat = int32(item->NoRent);
	if (id == "nodrop")
		stat = int32(item->NoDrop);
	if (id == "size")
		stat = int32(item->Size);
	if (id == "slots")
		stat = int32(item->Slots);
	if (id == "price")
		stat = int32(item->Price);
	if (id == "icon")
		stat = int32(item->Icon);
	if (id == "loregroup")
		stat = int32(item->LoreGroup);
	if (id == "loreflag")
		stat = int32(item->LoreFlag);
	if (id == "pendingloreflag")
		stat = int32(item->PendingLoreFlag);
	if (id == "artifactflag")
		stat = int32(item->ArtifactFlag);
	if (id == "summonedflag")
		stat = int32(item->SummonedFlag);
	if (id == "fvnodrop")
		stat = int32(item->FVNoDrop);
	if (id == "favor")
		stat = int32(item->Favor);
	if (id == "guildfavor")
		stat = int32(item->GuildFavor);
	if (id == "pointtype")
		stat = int32(item->PointType);
	if (id == "bagtype")
		stat = int32(item->BagType);
	if (id == "bagslots")
		stat = int32(item->BagSlots);
	if (id == "bagsize")
		stat = int32(item->BagSize);
	if (id == "bagwr")
		stat = int32(item->BagWR);
	if (id == "benefitflag")
		stat = int32(item->BenefitFlag);
	if (id == "tradeskills")
		stat = int32(item->Tradeskills);
	if (id == "cr")
		stat = int32(item->CR);
	if (id == "dr")
		stat = int32(item->DR);
	if (id == "pr")
		stat = int32(item->PR);
	if (id == "mr")
		stat = int32(item->MR);
	if (id == "fr")
		stat = int32(item->FR);
	if (id == "astr")
		stat = int32(item->AStr);
	if (id == "asta")
		stat = int32(item->ASta);
	if (id == "aagi")
		stat = int32(item->AAgi);
	if (id == "adex")
		stat = int32(item->ADex);
	if (id == "acha")
		stat = int32(item->ACha);
	if (id == "aint")
		stat = int32(item->AInt);
	if (id == "awis")
		stat = int32(item->AWis);
	if (id == "hp")
		stat = int32(item->HP);
	if (id == "mana")
		stat = int32(item->Mana);
	if (id == "ac")
		stat = int32(item->AC);
	if (id == "deity")
		stat = int32(item->Deity);
	if (id == "skillmodvalue")
		stat = int32(item->SkillModValue);
	if (id == "skillmodtype")
		stat = int32(item->SkillModType);
	if (id == "banedmgrace")
		stat = int32(item->BaneDmgRace);
	if (id == "banedmgamt")
		stat = int32(item->BaneDmgAmt);
	if (id == "banedmgbody")
		stat = int32(item->BaneDmgBody);
	if (id == "magic")
		stat = int32(item->Magic);
	if (id == "casttime_")
		stat = int32(item->CastTime_);
	if (id == "reqlevel")
		stat = int32(item->ReqLevel);
	if (id == "bardtype")
		stat = int32(item->BardType);
	if (id == "bardvalue")
		stat = int32(item->BardValue);
	if (id == "light")
		stat = int32(item->Light);
	if (id == "delay")
		stat = int32(item->Delay);
	if (id == "reclevel")
		stat = int32(item->RecLevel);
	if (id == "recskill")
		stat = int32(item->RecSkill);
	if (id == "elemdmgtype")
		stat = int32(item->ElemDmgType);
	if (id == "elemdmgamt")
		stat = int32(item->ElemDmgAmt);
	if (id == "range")
		stat = int32(item->Range);
	if (id == "damage")
		stat = int32(item->Damage);
	if (id == "color")
		stat = int32(item->Color);
	if (id == "classes")
		stat = int32(item->Classes);
	if (id == "races")
		stat = int32(item->Races);
	if (id == "maxcharges")
		stat = int32(item->MaxCharges);
	if (id == "itemtype")
		stat = int32(item->ItemType);
	if (id == "material")
		stat = int32(item->Material);
	if (id == "casttime")
		stat = int32(item->CastTime);
	if (id == "elitematerial")
		stat = int32(item->EliteMaterial);
	if (id == "herosforgemodel")
		stat = int32(item->HerosForgeModel);
	if (id == "procrate")
		stat = int32(item->ProcRate);
	if (id == "combateffects")
		stat = int32(item->CombatEffects);
	if (id == "shielding")
		stat = int32(item->Shielding);
	if (id == "stunresist")
		stat = int32(item->StunResist);
	if (id == "strikethrough")
		stat = int32(item->StrikeThrough);
	if (id == "extradmgskill")
		stat = int32(item->ExtraDmgSkill);
	if (id == "extradmgamt")
		stat = int32(item->ExtraDmgAmt);
	if (id == "spellshield")
		stat = int32(item->SpellShield);
	if (id == "avoidance")
		stat = int32(item->Avoidance);
	if (id == "accuracy")
		stat = int32(item->Accuracy);
	if (id == "charmfileid")
		stat = int32(item->CharmFileID);
	if (id == "factionmod1")
		stat = int32(item->FactionMod1);
	if (id == "factionmod2")
		stat = int32(item->FactionMod2);
	if (id == "factionmod3")
		stat = int32(item->FactionMod3);
	if (id == "factionmod4")
		stat = int32(item->FactionMod4);
	if (id == "factionamt1")
		stat = int32(item->FactionAmt1);
	if (id == "factionamt2")
		stat = int32(item->FactionAmt2);
	if (id == "factionamt3")
		stat = int32(item->FactionAmt3);
	if (id == "factionamt4")
		stat = int32(item->FactionAmt4);
	if (id == "augtype")
		stat = int32(item->AugType);
	if (id == "ldontheme")
		stat = int32(item->LDoNTheme);
	if (id == "ldonprice")
		stat = int32(item->LDoNPrice);
	if (id == "ldonsold")
		stat = int32(item->LDoNSold);
	if (id == "banedmgraceamt")
		stat = int32(item->BaneDmgRaceAmt);
	if (id == "augrestrict")
		stat = int32(item->AugRestrict);
	if (id == "endur")
		stat = int32(item->Endur);
	if (id == "dotshielding")
		stat = int32(item->DotShielding);
	if (id == "attack")
		stat = int32(item->Attack);
	if (id == "regen")
		stat = int32(item->Regen);
	if (id == "manaregen")
		stat = int32(item->ManaRegen);
	if (id == "enduranceregen")
		stat = int32(item->EnduranceRegen);
	if (id == "haste")
		stat = int32(item->Haste);
	if (id == "damageshield")
		stat = int32(item->DamageShield);
	if (id == "recastdelay")
		stat = int32(item->RecastDelay);
	if (id == "recasttype")
		stat = int32(item->RecastType);
	if (id == "augdistiller")
		stat = int32(item->AugDistiller);
	if (id == "attuneable")
		stat = int32(item->Attuneable);
	if (id == "nopet")
		stat = int32(item->NoPet);
	if (id == "potionbelt")
		stat = int32(item->PotionBelt);
	if (id == "stackable")
		stat = int32(item->Stackable);
	if (id == "notransfer")
		stat = int32(item->NoTransfer);
	if (id == "questitemflag")
		stat = int32(item->QuestItemFlag);
	if (id == "stacksize")
		stat = int32(item->StackSize);
	if (id == "potionbeltslots")
		stat = int32(item->PotionBeltSlots);
	if (id == "book")
		stat = int32(item->Book);
	if (id == "booktype")
		stat = int32(item->BookType);
	if (id == "svcorruption")
		stat = int32(item->SVCorruption);
	if (id == "purity")
		stat = int32(item->Purity);
	if (id == "backstabdmg")
		stat = int32(item->BackstabDmg);
	if (id == "dsmitigation")
		stat = int32(item->DSMitigation);
	if (id == "heroicstr")
		stat = int32(item->HeroicStr);
	if (id == "heroicint")
		stat = int32(item->HeroicInt);
	if (id == "heroicwis")
		stat = int32(item->HeroicWis);
	if (id == "heroicagi")
		stat = int32(item->HeroicAgi);
	if (id == "heroicdex")
		stat = int32(item->HeroicDex);
	if (id == "heroicsta")
		stat = int32(item->HeroicSta);
	if (id == "heroiccha")
		stat = int32(item->HeroicCha);
	if (id == "heroicmr")
		stat = int32(item->HeroicMR);
	if (id == "heroicfr")
		stat = int32(item->HeroicFR);
	if (id == "heroiccr")
		stat = int32(item->HeroicCR);
	if (id == "heroicdr")
		stat = int32(item->HeroicDR);
	if (id == "heroicpr")
		stat = int32(item->HeroicPR);
	if (id == "heroicsvcorrup")
		stat = int32(item->HeroicSVCorrup);
	if (id == "healamt")
		stat = int32(item->HealAmt);
	if (id == "spelldmg")
		stat = int32(item->SpellDmg);
	if (id == "ldonsellbackrate")
		stat = int32(item->LDoNSellBackRate);
	if (id == "scriptfileid")
		stat = int32(item->ScriptFileID);
	if (id == "expendablearrow")
		stat = int32(item->ExpendableArrow);
	if (id == "clairvoyance")
		stat = int32(item->Clairvoyance);
	// Begin Effects
	if (id == "clickeffect")
		stat = int32(item->Click.Effect);
	if (id == "clicktype")
		stat = int32(item->Click.Type);
	if (id == "clicklevel")
		stat = int32(item->Click.Level);
	if (id == "clicklevel2")
		stat = int32(item->Click.Level2);
	if (id == "proceffect")
		stat = int32(item->Proc.Effect);
	if (id == "proctype")
		stat = int32(item->Proc.Type);
	if (id == "proclevel")
		stat = int32(item->Proc.Level);
	if (id == "proclevel2")
		stat = int32(item->Proc.Level2);
	if (id == "worneffect")
		stat = int32(item->Worn.Effect);
	if (id == "worntype")
		stat = int32(item->Worn.Type);
	if (id == "wornlevel")
		stat = int32(item->Worn.Level);
	if (id == "wornlevel2")
		stat = int32(item->Worn.Level2);
	if (id == "focuseffect")
		stat = int32(item->Focus.Effect);
	if (id == "focustype")
		stat = int32(item->Focus.Type);
	if (id == "focuslevel")
		stat = int32(item->Focus.Level);
	if (id == "focuslevel2")
		stat = int32(item->Focus.Level2);
	if (id == "scrolleffect")
		stat = int32(item->Scroll.Effect);
	if (id == "scrolltype")
		stat = int32(item->Scroll.Type);
	if (id == "scrolllevel")
		stat = int32(item->Scroll.Level);
	if (id == "scrolllevel2")
		stat = int32(item->Scroll.Level2);

	safe_delete(inst);
	return stat;
}

std::string Mob::GetGlobal(const char *varname) {
	int qgCharid = 0;
	int qgNpcid = 0;

	if (this->IsNPC())
		qgNpcid = this->GetNPCTypeID();

	if (this->IsClient())
		qgCharid = this->CastToClient()->CharacterID();

	QGlobalCache *qglobals = nullptr;
	std::list<QGlobal> globalMap;

	if (this->IsClient())
		qglobals = this->CastToClient()->GetQGlobals();

	if (this->IsNPC())
		qglobals = this->CastToNPC()->GetQGlobals();

	if(qglobals)
		QGlobalCache::Combine(globalMap, qglobals->GetBucket(), qgNpcid, qgCharid, zone->GetZoneID());

	std::list<QGlobal>::iterator iter = globalMap.begin();
	while(iter != globalMap.end()) {
		if ((*iter).name.compare(varname) == 0)
			return (*iter).value;

		++iter;
	}

	return "Undefined";
}

void Mob::SetGlobal(const char *varname, const char *newvalue, int options, const char *duration, Mob *other) {

	int qgZoneid = zone->GetZoneID();
	int qgCharid = 0;
	int qgNpcid = 0;

	if (this->IsNPC())
	{
		qgNpcid = this->GetNPCTypeID();
	}
	else if (other && other->IsNPC())
	{
		qgNpcid = other->GetNPCTypeID();
	}

	if (this->IsClient())
	{
		qgCharid = this->CastToClient()->CharacterID();
	}
	else if (other && other->IsClient())
	{
		qgCharid = other->CastToClient()->CharacterID();
	}
	else
	{
		qgCharid = -qgNpcid;		// make char id negative npc id as a fudge
	}

	if (options < 0 || options > 7)
	{
		//cerr << "Invalid options for global var " << varname << " using defaults" << endl;
		options = 0;	// default = 0 (only this npcid,player and zone)
	}
	else
	{
		if (options & 1)
			qgNpcid=0;
		if (options & 2)
			qgCharid=0;
		if (options & 4)
			qgZoneid=0;
	}

	InsertQuestGlobal(qgCharid, qgNpcid, qgZoneid, varname, newvalue, QGVarDuration(duration));
}

void Mob::TarGlobal(const char *varname, const char *value, const char *duration, int qgNpcid, int qgCharid, int qgZoneid)
{
	InsertQuestGlobal(qgCharid, qgNpcid, qgZoneid, varname, value, QGVarDuration(duration));
}

void Mob::DelGlobal(const char *varname) {

	int qgZoneid=zone->GetZoneID();
	int qgCharid=0;
	int qgNpcid=0;

	if (this->IsNPC())
		qgNpcid = this->GetNPCTypeID();

	if (this->IsClient())
		qgCharid = this->CastToClient()->CharacterID();
	else
		qgCharid = -qgNpcid;		// make char id negative npc id as a fudge

    std::string query = StringFormat("DELETE FROM quest_globals "
                                    "WHERE name='%s' && (npcid=0 || npcid=%i) "
                                    "&& (charid=0 || charid=%i) "
                                    "&& (zoneid=%i || zoneid=0)",
                                    varname, qgNpcid, qgCharid, qgZoneid);

	database.QueryDatabase(query);

	if(zone)
	{
		ServerPacket* pack = new ServerPacket(ServerOP_QGlobalDelete, sizeof(ServerQGlobalDelete_Struct));
		ServerQGlobalDelete_Struct *qgu = (ServerQGlobalDelete_Struct*)pack->pBuffer;

		qgu->npc_id = qgNpcid;
		qgu->char_id = qgCharid;
		qgu->zone_id = qgZoneid;
		strcpy(qgu->name, varname);

		entity_list.DeleteQGlobal(std::string((char*)qgu->name), qgu->npc_id, qgu->char_id, qgu->zone_id);
		zone->DeleteQGlobal(std::string((char*)qgu->name), qgu->npc_id, qgu->char_id, qgu->zone_id);

		worldserver.SendPacket(pack);
		safe_delete(pack);
	}
}

// Inserts global variable into quest_globals table
void Mob::InsertQuestGlobal(int charid, int npcid, int zoneid, const char *varname, const char *varvalue, int duration) {

	// Make duration string either "unix_timestamp(now()) + xxx" or "NULL"
	std::stringstream duration_ss;

	if (duration == INT_MAX)
		duration_ss << "NULL";
	else
		duration_ss << "unix_timestamp(now()) + " << duration;

	//NOTE: this should be escaping the contents of arglist
	//npcwise a malicious script can arbitrarily alter the DB
	uint32 last_id = 0;
	std::string query = StringFormat("REPLACE INTO quest_globals "
                                    "(charid, npcid, zoneid, name, value, expdate)"
                                    "VALUES (%i, %i, %i, '%s', '%s', %s)",
                                    charid, npcid, zoneid, varname, varvalue, duration_ss.str().c_str());
	database.QueryDatabase(query);

	if(zone)
	{
		//first delete our global
		ServerPacket* pack = new ServerPacket(ServerOP_QGlobalDelete, sizeof(ServerQGlobalDelete_Struct));
		ServerQGlobalDelete_Struct *qgd = (ServerQGlobalDelete_Struct*)pack->pBuffer;
		qgd->npc_id = npcid;
		qgd->char_id = charid;
		qgd->zone_id = zoneid;
		qgd->from_zone_id = zone->GetZoneID();
		qgd->from_instance_id = zone->GetInstanceID();
		strcpy(qgd->name, varname);

		entity_list.DeleteQGlobal(std::string((char*)qgd->name), qgd->npc_id, qgd->char_id, qgd->zone_id);
		zone->DeleteQGlobal(std::string((char*)qgd->name), qgd->npc_id, qgd->char_id, qgd->zone_id);

		worldserver.SendPacket(pack);
		safe_delete(pack);

		//then create a new one with the new id
		pack = new ServerPacket(ServerOP_QGlobalUpdate, sizeof(ServerQGlobalUpdate_Struct));
		ServerQGlobalUpdate_Struct *qgu = (ServerQGlobalUpdate_Struct*)pack->pBuffer;
		qgu->npc_id = npcid;
		qgu->char_id = charid;
		qgu->zone_id = zoneid;

		if(duration == INT_MAX)
			qgu->expdate = 0xFFFFFFFF;
		else
			qgu->expdate = Timer::GetTimeSeconds() + duration;

		strcpy((char*)qgu->name, varname);
		strcpy((char*)qgu->value, varvalue);
		qgu->id = last_id;
		qgu->from_zone_id = zone->GetZoneID();
		qgu->from_instance_id = zone->GetInstanceID();

		QGlobal temp;
		temp.npc_id = npcid;
		temp.char_id = charid;
		temp.zone_id = zoneid;
		temp.expdate = qgu->expdate;
		temp.name.assign(qgu->name);
		temp.value.assign(qgu->value);
		entity_list.UpdateQGlobal(qgu->id, temp);
		zone->UpdateQGlobal(qgu->id, temp);

		worldserver.SendPacket(pack);
		safe_delete(pack);
	}

}

// Converts duration string to duration value (in seconds)
// Return of INT_MAX indicates infinite duration
int Mob::QGVarDuration(const char *fmt)
{
	int duration = 0;

	// format:	Y#### or D## or H## or M## or S## or T###### or C#######

	int len = static_cast<int>(strlen(fmt));

	// Default to no duration
	if (len < 1)
		return 0;

	// Set val to value after type character
	// e.g., for "M3924", set to 3924
	int val = atoi(&fmt[0] + 1);

	switch (fmt[0])
	{
		// Forever
		case 'F':
		case 'f':
			duration = INT_MAX;
			break;
		// Years
		case 'Y':
		case 'y':
			duration = val * 31556926;
			break;
		case 'D':
		case 'd':
			duration = val * 86400;
			break;
		// Hours
		case 'H':
		case 'h':
			duration = val * 3600;
			break;
		// Minutes
		case 'M':
		case 'm':
			duration = val * 60;
			break;
		// Seconds
		case 'S':
		case 's':
			duration = val;
			break;
		// Invalid
		default:
			duration = 0;
			break;
	}

	return duration;
}

void Mob::DoKnockback(Mob *caster, uint32 pushback, uint32 pushup)
{
	if(IsClient())
	{
		CastToClient()->SetKnockBackExemption(true);

		EQApplicationPacket* outapp_push = new EQApplicationPacket(OP_ClientUpdate, sizeof(PlayerPositionUpdateServer_Struct));
		PlayerPositionUpdateServer_Struct* spu = (PlayerPositionUpdateServer_Struct*)outapp_push->pBuffer;

		double look_heading = caster->CalculateHeadingToTarget(GetX(), GetY());
		look_heading /= 256;
		look_heading *= 360;
		if(look_heading > 360)
			look_heading -= 360;

		//x and y are crossed mkay
		double new_x = pushback * sin(double(look_heading * 3.141592 / 180.0));
		double new_y = pushback * cos(double(look_heading * 3.141592 / 180.0));

		spu->spawn_id	= GetID();
		spu->x_pos		= FloatToEQ19(GetX());
		spu->y_pos		= FloatToEQ19(GetY());
		spu->z_pos		= FloatToEQ19(GetZ());
		spu->delta_x	= NewFloatToEQ13(static_cast<float>(new_x));
		spu->delta_y	= NewFloatToEQ13(static_cast<float>(new_y));
		spu->delta_z	= NewFloatToEQ13(static_cast<float>(pushup));
		spu->heading	= FloatToEQ19(GetHeading());
		spu->padding0002	=0;
		spu->padding0006	=7;
		spu->padding0014	=0x7f;
		spu->padding0018	=0x5df27;
		spu->animation = 0;
		spu->delta_heading = NewFloatToEQ13(0);
		outapp_push->priority = 6;
		entity_list.QueueClients(this, outapp_push, true);
		CastToClient()->FastQueuePacket(&outapp_push);
	}
}

void Mob::TrySpellOnKill(uint8 level, uint16 spell_id)
{
	if (spell_id != SPELL_UNKNOWN)
	{
		if(IsEffectInSpell(spell_id, SE_ProcOnSpellKillShot)) {
			for (int i = 0; i < EFFECT_COUNT; i++) {
				if (spells[spell_id].effectid[i] == SE_ProcOnSpellKillShot)
				{
					if (IsValidSpell(spells[spell_id].base2[i]) && spells[spell_id].max[i] <= level)
					{
						if(zone->random.Roll(spells[spell_id].base[i]))
							SpellFinished(spells[spell_id].base2[i], this, 10, 0, -1, spells[spells[spell_id].base2[i]].ResistDiff);
					}
				}
			}
		}
	}

	if (!aabonuses.SpellOnKill[0] && !itembonuses.SpellOnKill[0] && !spellbonuses.SpellOnKill[0])
		return;

	// Allow to check AA, items and buffs in all cases. Base2 = Spell to fire | Base1 = % chance | Base3 = min level
	for(int i = 0; i < MAX_SPELL_TRIGGER*3; i+=3) {

		if(aabonuses.SpellOnKill[i] && IsValidSpell(aabonuses.SpellOnKill[i]) && (level >= aabonuses.SpellOnKill[i + 2])) {
			if(zone->random.Roll(static_cast<int>(aabonuses.SpellOnKill[i + 1])))
				SpellFinished(aabonuses.SpellOnKill[i], this, 10, 0, -1, spells[aabonuses.SpellOnKill[i]].ResistDiff);
		}

		if(itembonuses.SpellOnKill[i] && IsValidSpell(itembonuses.SpellOnKill[i]) && (level >= itembonuses.SpellOnKill[i + 2])){
			if(zone->random.Roll(static_cast<int>(itembonuses.SpellOnKill[i + 1])))
				SpellFinished(itembonuses.SpellOnKill[i], this, 10, 0, -1, spells[aabonuses.SpellOnKill[i]].ResistDiff);
		}

		if(spellbonuses.SpellOnKill[i] && IsValidSpell(spellbonuses.SpellOnKill[i]) && (level >= spellbonuses.SpellOnKill[i + 2])) {
			if(zone->random.Roll(static_cast<int>(spellbonuses.SpellOnKill[i + 1])))
				SpellFinished(spellbonuses.SpellOnKill[i], this, 10, 0, -1, spells[aabonuses.SpellOnKill[i]].ResistDiff);
		}

	}
}

bool Mob::TrySpellOnDeath()
{
	if (IsNPC() && !spellbonuses.SpellOnDeath[0] && !itembonuses.SpellOnDeath[0])
		return false;

	if (IsClient() && !aabonuses.SpellOnDeath[0] && !spellbonuses.SpellOnDeath[0] && !itembonuses.SpellOnDeath[0])
		return false;

	for(int i = 0; i < MAX_SPELL_TRIGGER*2; i+=2) {
		if(IsClient() && aabonuses.SpellOnDeath[i] && IsValidSpell(aabonuses.SpellOnDeath[i])) {
			if(zone->random.Roll(static_cast<int>(aabonuses.SpellOnDeath[i + 1]))) {
				SpellFinished(aabonuses.SpellOnDeath[i], this, 10, 0, -1, spells[aabonuses.SpellOnDeath[i]].ResistDiff);
			}
		}

		if(itembonuses.SpellOnDeath[i] && IsValidSpell(itembonuses.SpellOnDeath[i])) {
			if(zone->random.Roll(static_cast<int>(itembonuses.SpellOnDeath[i + 1]))) {
				SpellFinished(itembonuses.SpellOnDeath[i], this, 10, 0, -1, spells[itembonuses.SpellOnDeath[i]].ResistDiff);
			}
		}

		if(spellbonuses.SpellOnDeath[i] && IsValidSpell(spellbonuses.SpellOnDeath[i])) {
			if(zone->random.Roll(static_cast<int>(spellbonuses.SpellOnDeath[i + 1]))) {
				SpellFinished(spellbonuses.SpellOnDeath[i], this, 10, 0, -1, spells[spellbonuses.SpellOnDeath[i]].ResistDiff);
				}
			}
		}

	BuffFadeAll();
	return false;
	//You should not be able to use this effect and survive (ALWAYS return false),
	//attempting to place a heal in these effects will still result
	//in death because the heal will not register before the script kills you.
}

int16 Mob::GetCritDmgMob(uint16 skill)
{
	int critDmg_mod = 0;

	// All skill dmg mod + Skill specific
	critDmg_mod += itembonuses.CritDmgMob[HIGHEST_SKILL+1] + spellbonuses.CritDmgMob[HIGHEST_SKILL+1] + aabonuses.CritDmgMob[HIGHEST_SKILL+1] +
					itembonuses.CritDmgMob[skill] + spellbonuses.CritDmgMob[skill] + aabonuses.CritDmgMob[skill];

	if(critDmg_mod < -100)
		critDmg_mod = -100;

	return critDmg_mod;
}

void Mob::SetGrouped(bool v)
{
	if(v)
	{
		israidgrouped = false;
	}
	isgrouped = v;

	if(IsClient())
	{
			parse->EventPlayer(EVENT_GROUP_CHANGE, CastToClient(), "", 0);

		if(!v)
			CastToClient()->RemoveGroupXTargets();
	}
}

void Mob::SetRaidGrouped(bool v)
{
	if(v)
	{
		isgrouped = false;
	}
	israidgrouped = v;

	if(IsClient())
	{
		parse->EventPlayer(EVENT_GROUP_CHANGE, CastToClient(), "", 0);
	}
}

int16 Mob::GetCriticalChanceBonus(uint16 skill)
{
	int critical_chance = 0;

	// All skills + Skill specific
	critical_chance +=	itembonuses.CriticalHitChance[HIGHEST_SKILL+1] + spellbonuses.CriticalHitChance[HIGHEST_SKILL+1] + aabonuses.CriticalHitChance[HIGHEST_SKILL+1] +
						itembonuses.CriticalHitChance[skill] + spellbonuses.CriticalHitChance[skill] + aabonuses.CriticalHitChance[skill];

	if(critical_chance < -100)
		critical_chance = -100;

	return critical_chance;
}

int16 Mob::GetMeleeDamageMod_SE(uint16 skill)
{
	int dmg_mod = 0;

	// All skill dmg mod + Skill specific
	dmg_mod += itembonuses.DamageModifier[HIGHEST_SKILL+1] + spellbonuses.DamageModifier[HIGHEST_SKILL+1] + aabonuses.DamageModifier[HIGHEST_SKILL+1] +
				itembonuses.DamageModifier[skill] + spellbonuses.DamageModifier[skill] + aabonuses.DamageModifier[skill];

	dmg_mod += itembonuses.DamageModifier2[HIGHEST_SKILL+1] + spellbonuses.DamageModifier2[HIGHEST_SKILL+1] + aabonuses.DamageModifier2[HIGHEST_SKILL+1] +
				itembonuses.DamageModifier2[skill] + spellbonuses.DamageModifier2[skill] + aabonuses.DamageModifier2[skill];

	if (HasShieldEquiped() && !IsOffHandAtk())
		dmg_mod += itembonuses.ShieldEquipDmgMod[0] + spellbonuses.ShieldEquipDmgMod[0] + aabonuses.ShieldEquipDmgMod[0];

	dmg_mod += GetScaleDamageNumhits(skill); //C!Kayen

	if(dmg_mod < -100)
		dmg_mod = -100;

	return dmg_mod;
}

int16 Mob::GetMeleeMinDamageMod_SE(uint16 skill)
{
	int dmg_mod = 0;

	dmg_mod = itembonuses.MinDamageModifier[skill] + spellbonuses.MinDamageModifier[skill] +
				itembonuses.MinDamageModifier[HIGHEST_SKILL+1] + spellbonuses.MinDamageModifier[HIGHEST_SKILL+1];

	if(dmg_mod < -100)
		dmg_mod = -100;

	return dmg_mod;
}

int16 Mob::GetCrippBlowChance()
{
	int16 crip_chance = 0;

	crip_chance += itembonuses.CrippBlowChance + spellbonuses.CrippBlowChance + aabonuses.CrippBlowChance;

	if(crip_chance < 0)
		crip_chance = 0;

	return crip_chance;
}

int16 Mob::GetSkillReuseTime(uint16 skill)
{
	int skill_reduction = this->itembonuses.SkillReuseTime[skill] + this->spellbonuses.SkillReuseTime[skill] + this->aabonuses.SkillReuseTime[skill];

	return skill_reduction;
}

int16 Mob::GetSkillDmgAmt(uint16 skill)
{
	int skill_dmg = 0;

	// All skill dmg(only spells do this) + Skill specific
	skill_dmg += spellbonuses.SkillDamageAmount[HIGHEST_SKILL+1] + itembonuses.SkillDamageAmount[HIGHEST_SKILL+1] + aabonuses.SkillDamageAmount[HIGHEST_SKILL+1]
				+ itembonuses.SkillDamageAmount[skill] + spellbonuses.SkillDamageAmount[skill] + aabonuses.SkillDamageAmount[skill];

	skill_dmg += spellbonuses.SkillDamageAmount2[HIGHEST_SKILL+1] + itembonuses.SkillDamageAmount2[HIGHEST_SKILL+1]
				+ itembonuses.SkillDamageAmount2[skill] + spellbonuses.SkillDamageAmount2[skill];

	return skill_dmg;
}

void Mob::MeleeLifeTap(int32 damage) {

	int32 lifetap_amt = 0;
	lifetap_amt = spellbonuses.MeleeLifetap + itembonuses.MeleeLifetap + aabonuses.MeleeLifetap
				+ spellbonuses.Vampirism + itembonuses.Vampirism + aabonuses.Vampirism;

	if(lifetap_amt && damage > 0){

		lifetap_amt = damage * lifetap_amt / 100;
		Log.Out(Logs::Detail, Logs::Combat, "Melee lifetap healing for %d damage.", damage);

		if (lifetap_amt > 0)
			HealDamage(lifetap_amt); //Heal self for modified damage amount.
		else
			Damage(this, -lifetap_amt,0, SkillEvocation,false); //Dmg self for modified damage amount.
	}

	MeleeManaTap(damage); //C!Kayen
	PetTapToOwner(damage); //C!Kayen
	MeleeEndurTap(damage); //C!Kayen
}

bool Mob::TryReflectSpell(uint32 spell_id)
{
	if (!spells[spell_id].reflectable)
 		return false;

	int chance = itembonuses.reflect_chance + spellbonuses.reflect_chance + aabonuses.reflect_chance;

	if(chance && zone->random.Roll(chance))
		return true;

	return false;
}

void Mob::DoGravityEffect()
{
	Mob *caster = nullptr;
	int away = -1;
	float caster_x, caster_y, amount, value, cur_x, my_x, cur_y, my_y, x_vector, y_vector, hypot;

	// Set values so we can run through all gravity effects and then apply the culmative move at the end
	// instead of many small moves if the mob/client had more than 1 gravity effect on them
	cur_x = my_x = GetX();
	cur_y = my_y = GetY();

	int buff_count = GetMaxTotalSlots();
	for (int slot = 0; slot < buff_count; slot++)
	{
		if (buffs[slot].spellid != SPELL_UNKNOWN && IsEffectInSpell(buffs[slot].spellid, SE_GravityEffect))
		{
			for (int i = 0; i < EFFECT_COUNT; i++)
			{
				if(spells[buffs[slot].spellid].effectid[i] == SE_GravityEffect) {

					int casterId = buffs[slot].casterid;
					if(casterId)
						caster = entity_list.GetMob(casterId);

					if(!caster || casterId == this->GetID())
						continue;

					caster_x = caster->GetX();
					caster_y = caster->GetY();

					value = static_cast<float>(spells[buffs[slot].spellid].base[i]);
					if(value == 0)
						continue;

					if(value > 0)
						away = 1;

					amount = std::abs(value) /
						 (100.0f); // to bring the values in line, arbitarily picked

					x_vector = cur_x - caster_x;
					y_vector = cur_y - caster_y;
					hypot = sqrt(x_vector*x_vector + y_vector*y_vector);

					if(hypot <= 5) // dont want to be inside the mob, even though we can, it looks bad
						continue;

					x_vector /= hypot;
					y_vector /= hypot;

					cur_x = cur_x + (x_vector * amount * away);
					cur_y = cur_y + (y_vector * amount * away);
				}
			}
		}
	}

	if ((std::abs(my_x - cur_x) > 0.01) || (std::abs(my_y - cur_y) > 0.01)) {
		float new_ground = GetGroundZ(cur_x, cur_y);
		// If we cant get LoS on our new spot then keep checking up to 5 units up.
		if(!CheckLosFN(cur_x, cur_y, new_ground, GetSize())) {
			for(float z_adjust = 0.1f; z_adjust < 5; z_adjust += 0.1f) {
				if(CheckLosFN(cur_x, cur_y, new_ground+z_adjust, GetSize())) {
					new_ground += z_adjust;
					break;
				}
			}
			// If we still fail, then lets only use the x portion(ie sliding around a wall)
			if(!CheckLosFN(cur_x, my_y, new_ground, GetSize())) {
				// If that doesnt work, try the y
				if(!CheckLosFN(my_x, cur_y, new_ground, GetSize())) {
					// If everything fails, then lets do nothing
					return;
				}
				else {
					cur_x = my_x;
				}
			}
			else {
				cur_y = my_y;
			}
		}

		if(IsClient())
			this->CastToClient()->MovePC(zone->GetZoneID(), zone->GetInstanceID(), cur_x, cur_y, new_ground, GetHeading()*2); // I know the heading thing is weird(chance of movepc to halve the heading value, too lazy to figure out why atm)
		else
			this->GMMove(cur_x, cur_y, new_ground, GetHeading());
	}
}

void Mob::SpreadVirus(uint16 spell_id, uint16 casterID)
{
	int num_targs = spells[spell_id].viral_targets;

	Mob* caster = entity_list.GetMob(casterID);
	Mob* target = nullptr;
	// Only spread in zones without perm buffs
	if(!zone->BuffTimersSuspended()) {
		for(int i = 0; i < num_targs; i++) {
			target = entity_list.GetTargetForVirus(this, spells[spell_id].viral_range);
			if(target) {
				// Only spreads to the uninfected
				if(!target->FindBuff(spell_id)) {
					if(caster)
						caster->SpellOnTarget(spell_id, target);

				}
			}
		}
	}
}

void Mob::RemoveNimbusEffect(int effectid)
{
	if (effectid == nimbus_effect1)
		nimbus_effect1 = 0;

	else if (effectid == nimbus_effect2)
		nimbus_effect2 = 0;

	else if (effectid == nimbus_effect3)
		nimbus_effect3 = 0;

	EQApplicationPacket* outapp = new EQApplicationPacket(OP_RemoveNimbusEffect, sizeof(RemoveNimbusEffect_Struct));
	RemoveNimbusEffect_Struct* rne = (RemoveNimbusEffect_Struct*)outapp->pBuffer;
	rne->spawnid = GetID();
	rne->nimbus_effect = effectid;
	entity_list.QueueClients(this, outapp);
	safe_delete(outapp);
}

bool Mob::IsBoat() const {
	return (race == 72 || race == 73 || race == 114 || race == 404 || race == 550 || race == 551 || race == 552);
}

void Mob::SetBodyType(bodyType new_body, bool overwrite_orig) {
	bool needs_spawn_packet = false;
	if(bodytype == 11 || bodytype >= 65 || new_body == 11 || new_body >= 65) {
		needs_spawn_packet = true;
	}

	if(overwrite_orig) {
		orig_bodytype = new_body;
	}
	bodytype = new_body;

	if(needs_spawn_packet) {
		EQApplicationPacket* app = new EQApplicationPacket;
		CreateDespawnPacket(app, true);
		entity_list.QueueClients(this, app);
		CreateSpawnPacket(app, this);
		entity_list.QueueClients(this, app);
		safe_delete(app);
	}
}


void Mob::ModSkillDmgTaken(SkillUseTypes skill_num, int value)
{
	if (skill_num == ALL_SKILLS)
		SkillDmgTaken_Mod[HIGHEST_SKILL+1] = value;

	else if (skill_num >= 0 && skill_num <= HIGHEST_SKILL)
		SkillDmgTaken_Mod[skill_num] = value;
}

int16 Mob::GetModSkillDmgTaken(const SkillUseTypes skill_num)
{
	if (skill_num == ALL_SKILLS)
		return SkillDmgTaken_Mod[HIGHEST_SKILL+1];

	else if (skill_num >= 0 && skill_num <= HIGHEST_SKILL)
		return SkillDmgTaken_Mod[skill_num];

	return 0;
}

void Mob::ModVulnerability(uint8 resist, int16 value)
{
	if (resist < HIGHEST_RESIST+1)
		Vulnerability_Mod[resist] = value;

	else if (resist == 255)
		Vulnerability_Mod[HIGHEST_RESIST+1] = value;
}

int16 Mob::GetModVulnerability(const uint8 resist)
{
	if (resist < HIGHEST_RESIST+1)
		return Vulnerability_Mod[resist];

	else if (resist == 255)
		return Vulnerability_Mod[HIGHEST_RESIST+1];

	return 0;
}

void Mob::CastOnCurer(uint32 spell_id)
{
	for(int i = 0; i < EFFECT_COUNT; i++)
	{
		if (spells[spell_id].effectid[i] == SE_CastOnCurer)
		{
			if(IsValidSpell(spells[spell_id].base[i]))
			{
				SpellFinished(spells[spell_id].base[i], this);
			}
		}
	}
}

void Mob::CastOnCure(uint32 spell_id)
{
	for(int i = 0; i < EFFECT_COUNT; i++)
	{
		if (spells[spell_id].effectid[i] == SE_CastOnCure)
		{
			if(IsValidSpell(spells[spell_id].base[i]))
			{
				SpellFinished(spells[spell_id].base[i], this);
			}
		}
	}
}

void Mob::CastOnNumHitFade(uint32 spell_id)
{
	if(!IsValidSpell(spell_id))
		return;

	for(int i = 0; i < EFFECT_COUNT; i++)
	{
		if (spells[spell_id].effectid[i] == SE_CastonNumHitFade)
		{
			if(IsValidSpell(spells[spell_id].base[i]))
			{
				SpellFinished(spells[spell_id].base[i], this);
			}
		}
	}
}

void Mob::SlowMitigation(Mob* caster)
{
	if (GetSlowMitigation() && caster && caster->IsClient())
	{
		if ((GetSlowMitigation() > 0) && (GetSlowMitigation() < 26))
			caster->Message_StringID(MT_SpellFailure, SLOW_MOSTLY_SUCCESSFUL);

		else if ((GetSlowMitigation() >= 26) && (GetSlowMitigation() < 74))
			caster->Message_StringID(MT_SpellFailure, SLOW_PARTIALLY_SUCCESSFUL);

		else if ((GetSlowMitigation() >= 74) && (GetSlowMitigation() < 101))
			caster->Message_StringID(MT_SpellFailure, SLOW_SLIGHTLY_SUCCESSFUL);

		else if (GetSlowMitigation() > 100)
			caster->Message_StringID(MT_SpellFailure, SPELL_OPPOSITE_EFFECT);
	}
}

uint16 Mob::GetSkillByItemType(int ItemType)
{
	switch (ItemType)
	{
		case ItemType1HSlash:
			return Skill1HSlashing;
		case ItemType2HSlash:
			return Skill2HSlashing;
		case ItemType1HPiercing:
			return Skill1HPiercing;
		case ItemType1HBlunt:
			return Skill1HBlunt;
		case ItemType2HBlunt:
			return Skill2HBlunt;
		case ItemType2HPiercing:
			if (IsClient() && CastToClient()->GetClientVersion() < ClientVersion::RoF2)
				return Skill1HPiercing;
			else
				return Skill2HPiercing;
		case ItemTypeBow:
			return SkillArchery;
		case ItemTypeLargeThrowing:
		case ItemTypeSmallThrowing:
			return SkillThrowing;
		case ItemTypeMartial:
			return SkillHandtoHand;
		default:
			return SkillHandtoHand;
	}
	return SkillHandtoHand;
 }

uint8 Mob::GetItemTypeBySkill(SkillUseTypes skill)
{
	switch (skill)
	{
		case SkillThrowing:
			return ItemTypeSmallThrowing;
		case SkillArchery:
			return ItemTypeArrow;
		case Skill1HSlashing:
			return ItemType1HSlash;
		case Skill2HSlashing:
			return ItemType2HSlash;
		case Skill1HPiercing:
			return ItemType1HPiercing;
		case Skill2HPiercing: // watch for undesired client behavior
			return ItemType2HPiercing;
		case Skill1HBlunt:
			return ItemType1HBlunt;
		case Skill2HBlunt:
			return ItemType2HBlunt;
		case SkillHandtoHand:
			return ItemTypeMartial;
		default:
			return ItemTypeMartial;
	}
	return ItemTypeMartial;
 }


bool Mob::PassLimitToSkill(uint16 spell_id, uint16 skill) {

	if (!IsValidSpell(spell_id))
		return false;

	for (int i = 0; i < EFFECT_COUNT; i++) {
		if (spells[spell_id].effectid[i] == SE_LimitToSkill){
			if (spells[spell_id].base[i] == skill){
				return true;
			}
		}
	}
	return false;
}

uint16 Mob::GetWeaponSpeedbyHand(uint16 hand) {

	uint16 weapon_speed = 0;
	switch (hand) {

		case 13:
			weapon_speed = attack_timer.GetDuration();
			break;
		case 14:
			weapon_speed = attack_dw_timer.GetDuration();
			break;
		case 11:
			weapon_speed = ranged_timer.GetDuration();
			break;
	}

	if (weapon_speed < RuleI(Combat, MinHastedDelay))
		weapon_speed = RuleI(Combat, MinHastedDelay);

	return weapon_speed;
}

int8 Mob::GetDecayEffectValue(uint16 spell_id, uint16 spelleffect) {

	if (!IsValidSpell(spell_id))
		return false;

	int spell_level = spells[spell_id].classes[(GetClass()%17) - 1];
	int effect_value = 0;
	int lvlModifier = 100;

	int buff_count = GetMaxTotalSlots();
	for (int slot = 0; slot < buff_count; slot++){
		if (IsValidSpell(buffs[slot].spellid)){
			for (int i = 0; i < EFFECT_COUNT; i++){
				if(spells[buffs[slot].spellid].effectid[i] == spelleffect) {

					int critchance = spells[buffs[slot].spellid].base[i];
					int decay = spells[buffs[slot].spellid].base2[i];
					int lvldiff = spell_level - spells[buffs[slot].spellid].max[i];

					if(lvldiff > 0 && decay > 0)
					{
						lvlModifier -= decay*lvldiff;
						if (lvlModifier > 0){
							critchance = (critchance*lvlModifier)/100;
							effect_value += critchance;
						}
					}

					else
						effect_value += critchance;
				}
			}
		}
	}

	return effect_value;
}

// Faction Mods for Alliance type spells
void Mob::AddFactionBonus(uint32 pFactionID,int32 bonus) {
	std::map <uint32, int32> :: const_iterator faction_bonus;
	typedef std::pair <uint32, int32> NewFactionBonus;

	faction_bonus = faction_bonuses.find(pFactionID);
	if(faction_bonus == faction_bonuses.end())
	{
		faction_bonuses.insert(NewFactionBonus(pFactionID,bonus));
	}
	else
	{
		if(faction_bonus->second<bonus)
		{
			faction_bonuses.erase(pFactionID);
			faction_bonuses.insert(NewFactionBonus(pFactionID,bonus));
		}
	}
}

// Faction Mods from items
void Mob::AddItemFactionBonus(uint32 pFactionID,int32 bonus) {
	std::map <uint32, int32> :: const_iterator faction_bonus;
	typedef std::pair <uint32, int32> NewFactionBonus;

	faction_bonus = item_faction_bonuses.find(pFactionID);
	if(faction_bonus == item_faction_bonuses.end())
	{
		item_faction_bonuses.insert(NewFactionBonus(pFactionID,bonus));
	}
	else
	{
		if((bonus > 0 && faction_bonus->second < bonus) || (bonus < 0 && faction_bonus->second > bonus))
		{
			item_faction_bonuses.erase(pFactionID);
			item_faction_bonuses.insert(NewFactionBonus(pFactionID,bonus));
		}
	}
}

int32 Mob::GetFactionBonus(uint32 pFactionID) {
	std::map <uint32, int32> :: const_iterator faction_bonus;
	faction_bonus = faction_bonuses.find(pFactionID);
	if(faction_bonus != faction_bonuses.end())
	{
		return (*faction_bonus).second;
	}
	return 0;
}

int32 Mob::GetItemFactionBonus(uint32 pFactionID) {
	std::map <uint32, int32> :: const_iterator faction_bonus;
	faction_bonus = item_faction_bonuses.find(pFactionID);
	if(faction_bonus != item_faction_bonuses.end())
	{
		return (*faction_bonus).second;
	}
	return 0;
}

void Mob::ClearItemFactionBonuses() {
	item_faction_bonuses.clear();
}

FACTION_VALUE Mob::GetSpecialFactionCon(Mob* iOther) {
	if (!iOther)
		return FACTION_INDIFFERENT;

	iOther = iOther->GetOwnerOrSelf();
	Mob* self = this->GetOwnerOrSelf();

	bool selfAIcontrolled = self->IsAIControlled();
	bool iOtherAIControlled = iOther->IsAIControlled();
	int selfPrimaryFaction = self->GetPrimaryFaction();
	int iOtherPrimaryFaction = iOther->GetPrimaryFaction();

	if (selfPrimaryFaction >= 0 && selfAIcontrolled)
		return FACTION_INDIFFERENT;
	if (iOther->GetPrimaryFaction() >= 0)
		return FACTION_INDIFFERENT;
/* special values:
	-2 = indiff to player, ally to AI on special values, indiff to AI
	-3 = dub to player, ally to AI on special values, indiff to AI
	-4 = atk to player, ally to AI on special values, indiff to AI
	-5 = indiff to player, indiff to AI
	-6 = dub to player, indiff to AI
	-7 = atk to player, indiff to AI
	-8 = indiff to players, ally to AI on same value, indiff to AI
	-9 = dub to players, ally to AI on same value, indiff to AI
	-10 = atk to players, ally to AI on same value, indiff to AI
	-11 = indiff to players, ally to AI on same value, atk to AI
	-12 = dub to players, ally to AI on same value, atk to AI
	-13 = atk to players, ally to AI on same value, atk to AI
*/
	switch (iOtherPrimaryFaction) {
		case -2: // -2 = indiff to player, ally to AI on special values, indiff to AI
			if (selfAIcontrolled && iOtherAIControlled)
				return FACTION_ALLY;
			else
				return FACTION_INDIFFERENT;
		case -3: // -3 = dub to player, ally to AI on special values, indiff to AI
			if (selfAIcontrolled && iOtherAIControlled)
				return FACTION_ALLY;
			else
				return FACTION_DUBIOUS;
		case -4: // -4 = atk to player, ally to AI on special values, indiff to AI
			if (selfAIcontrolled && iOtherAIControlled)
				return FACTION_ALLY;
			else
				return FACTION_SCOWLS;
		case -5: // -5 = indiff to player, indiff to AI
			return FACTION_INDIFFERENT;
		case -6: // -6 = dub to player, indiff to AI
			if (selfAIcontrolled && iOtherAIControlled)
				return FACTION_INDIFFERENT;
			else
				return FACTION_DUBIOUS;
		case -7: // -7 = atk to player, indiff to AI
			if (selfAIcontrolled && iOtherAIControlled)
				return FACTION_INDIFFERENT;
			else
				return FACTION_SCOWLS;
		case -8: // -8 = indiff to players, ally to AI on same value, indiff to AI
			if (selfAIcontrolled && iOtherAIControlled) {
				if (selfPrimaryFaction == iOtherPrimaryFaction)
					return FACTION_ALLY;
				else
					return FACTION_INDIFFERENT;
			}
			else
				return FACTION_INDIFFERENT;
		case -9: // -9 = dub to players, ally to AI on same value, indiff to AI
			if (selfAIcontrolled && iOtherAIControlled) {
				if (selfPrimaryFaction == iOtherPrimaryFaction)
					return FACTION_ALLY;
				else
					return FACTION_INDIFFERENT;
			}
			else
				return FACTION_DUBIOUS;
		case -10: // -10 = atk to players, ally to AI on same value, indiff to AI
			if (selfAIcontrolled && iOtherAIControlled) {
				if (selfPrimaryFaction == iOtherPrimaryFaction)
					return FACTION_ALLY;
				else
					return FACTION_INDIFFERENT;
			}
			else
				return FACTION_SCOWLS;
		case -11: // -11 = indiff to players, ally to AI on same value, atk to AI
			if (selfAIcontrolled && iOtherAIControlled) {
				if (selfPrimaryFaction == iOtherPrimaryFaction)
					return FACTION_ALLY;
				else
					return FACTION_SCOWLS;
			}
			else
				return FACTION_INDIFFERENT;
		case -12: // -12 = dub to players, ally to AI on same value, atk to AI
			if (selfAIcontrolled && iOtherAIControlled) {
				if (selfPrimaryFaction == iOtherPrimaryFaction)
					return FACTION_ALLY;
				else
					return FACTION_SCOWLS;


			}
			else
				return FACTION_DUBIOUS;
		case -13: // -13 = atk to players, ally to AI on same value, atk to AI
			if (selfAIcontrolled && iOtherAIControlled) {
				if (selfPrimaryFaction == iOtherPrimaryFaction)
					return FACTION_ALLY;
				else
					return FACTION_SCOWLS;
			}
			else
				return FACTION_SCOWLS;
		default:
			return FACTION_INDIFFERENT;
	}
}

bool Mob::HasSpellEffect(int effectid)
{
    int i;

    int buff_count = GetMaxTotalSlots();
    for(i = 0; i < buff_count; i++)
    {
        if(buffs[i].spellid == SPELL_UNKNOWN) { continue; }

        if(IsEffectInSpell(buffs[i].spellid, effectid))
        {
            return(1);
        }
    }
    return(0);
}

int Mob::GetSpecialAbility(int ability) {
	if(ability >= MAX_SPECIAL_ATTACK || ability < 0) {
		return 0;
	}

	return SpecialAbilities[ability].level;
}

int Mob::GetSpecialAbilityParam(int ability, int param) {
	if(param >= MAX_SPECIAL_ATTACK_PARAMS || param < 0 || ability >= MAX_SPECIAL_ATTACK || ability < 0) {
		return 0;
	}

	return SpecialAbilities[ability].params[param];
}

void Mob::SetSpecialAbility(int ability, int level) {
	if(ability >= MAX_SPECIAL_ATTACK || ability < 0) {
		return;
	}

	SpecialAbilities[ability].level = level;
}

void Mob::SetSpecialAbilityParam(int ability, int param, int value) {
	if(param >= MAX_SPECIAL_ATTACK_PARAMS || param < 0 || ability >= MAX_SPECIAL_ATTACK || ability < 0) {
		return;
	}

	SpecialAbilities[ability].params[param] = value;
}

void Mob::StartSpecialAbilityTimer(int ability, uint32 time) {
	if (ability >= MAX_SPECIAL_ATTACK || ability < 0) {
		return;
	}

	if(SpecialAbilities[ability].timer) {
		SpecialAbilities[ability].timer->Start(time);
	} else {
		SpecialAbilities[ability].timer = new Timer(time);
		SpecialAbilities[ability].timer->Start();
	}
}

void Mob::StopSpecialAbilityTimer(int ability) {
	if (ability >= MAX_SPECIAL_ATTACK || ability < 0) {
		return;
	}

	safe_delete(SpecialAbilities[ability].timer);
}

Timer *Mob::GetSpecialAbilityTimer(int ability) {
	if (ability >= MAX_SPECIAL_ATTACK || ability < 0) {
		return nullptr;
	}

	return SpecialAbilities[ability].timer;
}

void Mob::ClearSpecialAbilities() {
	for(int a = 0; a < MAX_SPECIAL_ATTACK; ++a) {
		SpecialAbilities[a].level = 0;
		safe_delete(SpecialAbilities[a].timer);
		for(int p = 0; p < MAX_SPECIAL_ATTACK_PARAMS; ++p) {
			SpecialAbilities[a].params[p] = 0;
		}
	}
}

void Mob::ProcessSpecialAbilities(const std::string &str) {
	ClearSpecialAbilities();

	std::vector<std::string> sp = SplitString(str, '^');
	for(auto iter = sp.begin(); iter != sp.end(); ++iter) {
		std::vector<std::string> sub_sp = SplitString((*iter), ',');
		if(sub_sp.size() >= 2) {
			int ability = std::stoi(sub_sp[0]);
			int value = std::stoi(sub_sp[1]);
			SetSpecialAbility(ability, value);
			switch(ability) {
			case SPECATK_QUAD:
				if(value > 0) {
					SetSpecialAbility(SPECATK_TRIPLE, 1);
				}
				break;
			case DESTRUCTIBLE_OBJECT:
				if(value == 0) {
					SetDestructibleObject(false);
				} else {
					SetDestructibleObject(true);
				}
				break;
			default:
				break;
			}

			for(size_t i = 2, p = 0; i < sub_sp.size(); ++i, ++p) {
				if(p >= MAX_SPECIAL_ATTACK_PARAMS) {
					break;
				}

				SetSpecialAbilityParam(ability, p, std::stoi(sub_sp[i]));
			}
		}
	}
}

// derived from client to keep these functions more consistent
// if anything seems weird, blame SoE
bool Mob::IsFacingMob(Mob *other)
{
	if (!other)
		return false;
	float angle = HeadingAngleToMob(other);
	// what the client uses appears to be 2x our internal heading
	float heading = GetHeading() * 2.0f;

	if (angle > 472.0 && heading < 40.0)
		angle = heading;
	if (angle < 40.0 && heading > 472.0)
		angle = heading;

	if (std::abs(angle - heading) <= 80.0)
		return true;

	return false;
}

// All numbers derived from the client
float Mob::HeadingAngleToMob(Mob *other)
{
	float mob_x = other->GetX();
	float mob_y = other->GetY();
	float this_x = GetX();
	float this_y = GetY();

	float y_diff = std::abs(this_y - mob_y);
	float x_diff = std::abs(this_x - mob_x);
	if (y_diff < 0.0000009999999974752427)
		y_diff = 0.0000009999999974752427;

	float angle = atan2(x_diff, y_diff) * 180.0f * 0.3183099014828645f; // angle, nice "pi"

	// return the right thing based on relative quadrant
	// I'm sure this could be improved for readability, but whatever
	if (this_y >= mob_y) {
		if (mob_x >= this_x)
			return (90.0f - angle + 90.0f) * 511.5f * 0.0027777778f;
		if (mob_x <= this_x)
			return (angle + 180.0f) * 511.5f * 0.0027777778f;
	}
	if (this_y > mob_y || mob_x > this_x)
		return angle * 511.5f * 0.0027777778f;
	else
		return (90.0f - angle + 270.0f) * 511.5f * 0.0027777778f;
}

int32 Mob::GetSpellStat(uint32 spell_id, const char *identifier, uint8 slot)
{
	if (!IsValidSpell(spell_id))
		return 0;

	if (!identifier)
		return 0;

	int32 stat = 0;

	if (slot > 0)
		slot = slot - 1;

	std::string id = identifier;
	for(uint32 i = 0; i < id.length(); ++i)
	{
		id[i] = tolower(id[i]);
	}

	if (slot < 16){
		if (id == "classes") {return spells[spell_id].classes[slot]; }
		else if (id == "dieties") {return spells[spell_id].deities[slot];}
	}

	if (slot < 12){
		if (id == "base") {return spells[spell_id].base[slot];}
		else if (id == "base2") {return spells[spell_id].base2[slot];}
		else if (id == "max") {return spells[spell_id].max[slot];}
		else if (id == "formula") {return spells[spell_id].formula[slot];}
		else if (id == "effectid") {return spells[spell_id].effectid[slot];}
	}

	if (slot < 4){
		if (id == "components") { return spells[spell_id].components[slot];}
		else if (id == "component_counts") { return spells[spell_id].component_counts[slot];}
		else if (id == "NoexpendReagent") {return spells[spell_id].NoexpendReagent[slot];}
	}

	if (id == "range") {return static_cast<int32>(spells[spell_id].range); }
	else if (id == "aoerange") {return static_cast<int32>(spells[spell_id].aoerange);}
	else if (id == "pushback") {return static_cast<int32>(spells[spell_id].pushback);}
	else if (id == "pushup") {return static_cast<int32>(spells[spell_id].pushup);}
	else if (id == "cast_time") {return spells[spell_id].cast_time;}
	else if (id == "recovery_time") {return spells[spell_id].recovery_time;}
	else if (id == "recast_time") {return spells[spell_id].recast_time;}
	else if (id == "buffdurationformula") {return spells[spell_id].buffdurationformula;}
	else if (id == "buffduration") {return spells[spell_id].buffduration;}
	else if (id == "AEDuration") {return spells[spell_id].AEDuration;}
	else if (id == "mana") {return spells[spell_id].mana;}
	else if (id == "LightType") {stat = spells[spell_id].LightType;} 
	else if (id == "goodEffect") {return spells[spell_id].goodEffect;}
	else if (id == "Activated") {return spells[spell_id].Activated;}
	else if (id == "resisttype") {return spells[spell_id].resisttype;}
	else if (id == "targettype") {return spells[spell_id].targettype;}
	else if (id == "basedeiff") {return spells[spell_id].basediff;}
	else if (id == "skill") {return spells[spell_id].skill;}
	else if (id == "zonetype") {return spells[spell_id].zonetype;}
	else if (id == "EnvironmentType") {return spells[spell_id].EnvironmentType;}
	else if (id == "TimeOfDay") {return spells[spell_id].TimeOfDay;}
	else if (id == "CastingAnim") {return spells[spell_id].CastingAnim;}
	else if (id == "SpellAffectIndex") {return spells[spell_id].SpellAffectIndex; }
	else if (id == "disallow_sit") {return spells[spell_id].disallow_sit; }
	else if (id == "spellanim") {stat = spells[spell_id].spellanim; }
	else if (id == "uninterruptable") {return spells[spell_id].uninterruptable; }
	else if (id == "ResistDiff") {return spells[spell_id].ResistDiff; }
	else if (id == "dot_stacking_exemp") {return spells[spell_id].dot_stacking_exempt; }
	else if (id == "RecourseLink") {return spells[spell_id].RecourseLink; }
	else if (id == "no_partial_resist") {return spells[spell_id].no_partial_resist; }
	else if (id == "short_buff_box") {return spells[spell_id].short_buff_box; }
	else if (id == "descnum") {return spells[spell_id].descnum; }
	else if (id == "effectdescnum") {return spells[spell_id].effectdescnum; }
	else if (id == "npc_no_los") {return spells[spell_id].npc_no_los; }
	else if (id == "reflectable") {return spells[spell_id].reflectable; }
	else if (id == "bonushate") {return spells[spell_id].bonushate; }
	else if (id == "EndurCost") {return spells[spell_id].EndurCost; }
	else if (id == "EndurTimerIndex") {return spells[spell_id].EndurTimerIndex; }
	else if (id == "IsDisciplineBuf") {return spells[spell_id].IsDisciplineBuff; }
	else if (id == "HateAdded") {return spells[spell_id].HateAdded; }
	else if (id == "EndurUpkeep") {return spells[spell_id].EndurUpkeep; }
	else if (id == "numhitstype") {return spells[spell_id].numhitstype; }
	else if (id == "numhits") {return spells[spell_id].numhits; }
	else if (id == "pvpresistbase") {return spells[spell_id].pvpresistbase; }
	else if (id == "pvpresistcalc") {return spells[spell_id].pvpresistcalc; }
	else if (id == "pvpresistcap") {return spells[spell_id].pvpresistcap; }
	else if (id == "spell_category") {return spells[spell_id].spell_category; }
	else if (id == "can_mgb") {return spells[spell_id].can_mgb; }
	else if (id == "dispel_flag") {return spells[spell_id].dispel_flag; }
	else if (id == "MinResist") {return spells[spell_id].MinResist; }
	else if (id == "MaxResist") {return spells[spell_id].MaxResist; }
	else if (id == "viral_targets") {return spells[spell_id].viral_targets; }
	else if (id == "viral_timer") {return spells[spell_id].viral_timer; }
	else if (id == "NimbusEffect") {return spells[spell_id].NimbusEffect; }
	else if (id == "directional_start") {return static_cast<int32>(spells[spell_id].directional_start); }
	else if (id == "directional_end") {return static_cast<int32>(spells[spell_id].directional_end); }
	else if (id == "not_focusable") {return spells[spell_id].not_focusable; }
	else if (id == "suspendable") {return spells[spell_id].suspendable; }
	else if (id == "viral_range") {return spells[spell_id].viral_range; }
	else if (id == "spellgroup") {return spells[spell_id].spellgroup; }
	else if (id == "rank") {return spells[spell_id].rank; }
	else if (id == "no_resist") {return spells[spell_id].no_resist; }
	else if (id == "CastRestriction") {return spells[spell_id].CastRestriction; }
	else if (id == "AllowRest") {return spells[spell_id].AllowRest; }
	else if (id == "InCombat") {return spells[spell_id].InCombat; }
	else if (id == "OutofCombat") {return spells[spell_id].OutofCombat; }
	else if (id == "aemaxtargets") {return spells[spell_id].aemaxtargets; }
	else if (id == "no_heal_damage_item_mod") {return spells[spell_id].no_heal_damage_item_mod; }
	else if (id == "persistdeath") {return spells[spell_id].persistdeath; }
	else if (id == "min_dist") {return static_cast<int32>(spells[spell_id].min_dist); }
	else if (id == "min_dist_mod") {return static_cast<int32>(spells[spell_id].min_dist_mod); }
	else if (id == "max_dist") {return static_cast<int32>(spells[spell_id].max_dist); }
	else if (id == "min_range") {return static_cast<int32>(spells[spell_id].min_range); }
	else if (id == "DamageShieldType") {return spells[spell_id].DamageShieldType; }

	return stat;
}

bool Mob::CanClassEquipItem(uint32 item_id)
{
	const Item_Struct* itm = nullptr;
	itm = database.GetItem(item_id);

	if (!itm)
		return false;

	if(itm->Classes == 65535 )
		return true;

	if (GetClass() > 16)
		return false;

	int bitmask = 1;
	bitmask = bitmask << (GetClass() - 1);

	if(!(itm->Classes & bitmask))
		return false;
	else
		return true;
}

void Mob::SendAddPlayerState(PlayerState new_state)
{
	auto app = new EQApplicationPacket(OP_PlayerStateAdd, sizeof(PlayerState_Struct));
	auto ps = (PlayerState_Struct *)app->pBuffer;

	ps->spawn_id = GetID();
	ps->state = static_cast<uint32>(new_state);

	AddPlayerState(ps->state);
	entity_list.QueueClients(nullptr, app);
	safe_delete(app);
}

void Mob::SendRemovePlayerState(PlayerState old_state)
{
	auto app = new EQApplicationPacket(OP_PlayerStateRemove, sizeof(PlayerState_Struct));
	auto ps = (PlayerState_Struct *)app->pBuffer;

	ps->spawn_id = GetID();
	ps->state = static_cast<uint32>(old_state);

	RemovePlayerState(ps->state);
	entity_list.QueueClients(nullptr, app);
	safe_delete(app);
}

void Mob::SetCurrentSpeed(int in){

	if (in == 0)//C!Kayen Hot Fix
		 moved = false;

	if (current_speed != in)
	{
		current_speed = in;
		tar_ndx = 20;
		if (in == 0) {
			SetRunAnimSpeed(0);
			SetMoving(false);
			SendPosition();
		}
	}
}

int32 Mob::GetMeleeMitigation() {
	int32 mitigation = 0;
	mitigation += spellbonuses.MeleeMitigationEffect;
	mitigation += itembonuses.MeleeMitigationEffect;
	mitigation += aabonuses.MeleeMitigationEffect;
	return mitigation;
}

/* this is the mob being attacked.
 * Pass in the weapon's ItemInst
 */
int Mob::ResistElementalWeaponDmg(const ItemInst *item)
{
	if (!item)
		return 0;
	int magic = 0, fire = 0, cold = 0, poison = 0, disease = 0, chromatic = 0, prismatic = 0, physical = 0,
	    corruption = 0;
	int resist = 0;
	int roll = 0;
	/*  this is how the client does the resist rolls for these.
	 *  Given the difficulty of parsing out these resists, I'll trust the client
	 */
	if (item->GetItemElementalDamage(magic, fire, cold, poison, disease, chromatic, prismatic, physical, corruption, true)) {
		if (magic) {
			resist = GetMR();
			if (resist >= 201) {
				magic = 0;
			} else {
				roll = zone->random.Int(0, 200) - resist;
				if (roll < 1)
					magic = 0;
				else if (roll < 100)
					magic = magic * roll / 100;
			}
		}

		if (fire) {
			resist = GetFR();
			if (resist >= 201) {
				fire = 0;
			} else {
				roll = zone->random.Int(0, 200) - resist;
				if (roll < 1)
					fire = 0;
				else if (roll < 100)
					fire = fire * roll / 100;
			}
		}

		if (cold) {
			resist = GetCR();
			if (resist >= 201) {
				cold = 0;
			} else {
				roll = zone->random.Int(0, 200) - resist;
				if (roll < 1)
					cold = 0;
				else if (roll < 100)
					cold = cold * roll / 100;
			}
		}

		if (poison) {
			resist = GetPR();
			if (resist >= 201) {
				poison = 0;
			} else {
				roll = zone->random.Int(0, 200) - resist;
				if (roll < 1)
					poison = 0;
				else if (roll < 100)
					poison = poison * roll / 100;
			}
		}

		if (disease) {
			resist = GetDR();
			if (resist >= 201) {
				disease = 0;
			} else {
				roll = zone->random.Int(0, 200) - resist;
				if (roll < 1)
					disease = 0;
				else if (roll < 100)
					disease = disease * roll / 100;
			}
		}

		if (corruption) {
			resist = GetCorrup();
			if (resist >= 201) {
				corruption = 0;
			} else {
				roll = zone->random.Int(0, 200) - resist;
				if (roll < 1)
					corruption = 0;
				else if (roll < 100)
					corruption = corruption * roll / 100;
			}
		}

		if (chromatic) {
			resist = GetFR();
			int temp = GetCR();
			if (temp < resist)
				resist = temp;

			temp = GetMR();
			if (temp < resist)
				resist = temp;

			temp = GetDR();
			if (temp < resist)
				resist = temp;

			temp = GetPR();
			if (temp < resist)
				resist = temp;

			if (resist >= 201) {
				chromatic = 0;
			} else {
				roll = zone->random.Int(0, 200) - resist;
				if (roll < 1)
					chromatic = 0;
				else if (roll < 100)
					chromatic = chromatic * roll / 100;
			}
		}

		if (prismatic) {
			resist = (GetFR() + GetCR() + GetMR() + GetDR() + GetPR()) / 5;
			if (resist >= 201) {
				prismatic = 0;
			} else {
				roll = zone->random.Int(0, 200) - resist;
				if (roll < 1)
					prismatic = 0;
				else if (roll < 100)
					prismatic = prismatic * roll / 100;
			}
		}

		if (physical) {
			resist = GetPhR();
			if (resist >= 201) {
				physical = 0;
			} else {
				roll = zone->random.Int(0, 200) - resist;
				if (roll < 1)
					physical = 0;
				else if (roll < 100)
					physical = physical * roll / 100;
			}
		}
	}

	return magic + fire + cold + poison + disease + chromatic + prismatic + physical + corruption;
}

/* this is the mob being attacked.
 * Pass in the weapon's ItemInst
 */
int Mob::CheckBaneDamage(const ItemInst *item)
{
	if (!item)
		return 0;

	int damage = item->GetItemBaneDamageBody(GetBodyType(), true);
	damage += item->GetItemBaneDamageRace(GetRace(), true);

	return damage;
}

#ifdef BOTS
bool Mob::JoinHealRotationTargetPool(std::shared_ptr<HealRotation>* heal_rotation)
{
	if (IsHealRotationTarget())
		return false;
	if (!heal_rotation->use_count())
		return false;
	if (!(*heal_rotation))
		return false;
	if (!IsHealRotationTargetMobType(this))
		return false;

	if (!(*heal_rotation)->AddTargetToPool(this))
		return false;

	m_target_of_heal_rotation = *heal_rotation;

	return IsHealRotationTarget();
}

bool Mob::LeaveHealRotationTargetPool()
{
	if (!IsHealRotationTarget()) {
		m_target_of_heal_rotation.reset();
		return true;
	}

	m_target_of_heal_rotation->RemoveTargetFromPool(this);
	m_target_of_heal_rotation.reset();
	
	return !IsHealRotationTarget();
}

uint32 Mob::HealRotationHealCount()
{
	if (!IsHealRotationTarget())
		return 0;

	return m_target_of_heal_rotation->HealCount(this);
}

uint32 Mob::HealRotationExtendedHealCount()
{
	if (!IsHealRotationTarget())
		return 0;

	return m_target_of_heal_rotation->ExtendedHealCount(this);
}

float Mob::HealRotationHealFrequency()
{
	if (!IsHealRotationTarget())
		return 0.0f;

	return m_target_of_heal_rotation->HealFrequency(this);
}

float Mob::HealRotationExtendedHealFrequency()
{
	if (!IsHealRotationTarget())
		return 0.0f;

	return m_target_of_heal_rotation->ExtendedHealFrequency(this);
}
#endif


//!CKayen - Custom MOB functions

//#### C!SpecialAASystem - see client.h - Helper functions for purchasing AA

void Client::UnscribeSpellByGroup(uint16 spellid) {
	
	int spellgroup = spells[spellid].spellgroup;

	if (!spellgroup)
		return;

	for(int i = 0; i < MAX_PP_SPELLBOOK; i++) {
		if(IsValidSpell(m_pp.spell_book[i])){
			if (spells[m_pp.spell_book[i]].spellgroup == spellgroup){
				
				UnscribeSpell(i, true);

				for(int d = 0; d < MAX_PP_MEMSPELL; d++){
					if(IsValidSpell(m_pp.mem_spells[d]) && (spells[m_pp.mem_spells[d]].spellgroup == spellgroup))
						UnmemSpell(d, true);
				}
			}
		}
	}
}

void Client::UnscribeDiscByGroup(uint16 spellid) {

	int spellgroup = spells[spellid].spellgroup;

	if (!spellgroup)
		return;

	for(int i = 0; i < MAX_PP_DISCIPLINES; i++)
	{
		if(IsValidSpell(m_pp.disciplines.values[i])){
			if (spells[m_pp.disciplines.values[i]].spellgroup == spellgroup) {
				UntrainDisc(i, true);
			}
		}
	}
}

bool Client::TrainDisciplineBySpellid(uint16 spell_id) {

	int myclass = GetClass();

	if(!IsValidSpell(spell_id)) 
		return(false);
	
	const SPDat_Spell_Struct &spell = spells[spell_id];
	uint8 level_to_use = spell.classes[myclass - 1];
	if(level_to_use == 255) {
		Message(13, "Your class cannot learn from this discipline.");
		return(false);
	}

	if(level_to_use > GetLevel()) {
		Message(13, "You must be at least level %d to learn this discipline.", level_to_use);
		return(false);
	}

	//add it to PP.
	int r;
	for(r = 0; r < MAX_PP_DISCIPLINES; r++) {
		if(m_pp.disciplines.values[r] == spell_id) {
			Message(13, "You already know this discipline.");
			return(false);
		} else if(m_pp.disciplines.values[r] == 0) {
			m_pp.disciplines.values[r] = spell_id;
			SendDisciplineUpdate();
			Message(0, "You have learned a new discipline!");
			return(true);
		}
	}

	Message(13, "You have learned too many disciplines and can learn no more.");
	return(false);
}

void Client::RefundAAType(uint32 sof_type) {

	/*

	int cur = 0;
	bool refunded = false;

	Message(13, "Refund Type %i.", sof_type);

	for(int x = 0; x < aaHighestID; x++) {
		cur = GetAA(x);

		if(cur > 0){
			SendAA_Struct* curaa = zone->FindAA(x);
			if(cur){
				if (!sof_type || curaa->sof_type == sof_type){
					for(int j = 0; j < cur; j++) {
						AddAlternateCurrencyValue(1,(curaa->cost + (curaa->cost_inc * j)));
						m_pp.aapoints = 20; //Need to have real AA or client won't let you buy.
						Message(13, "Refund AA %s amount %i.", curaa->name, (curaa->cost + (curaa->cost_inc * j)));
						refunded = true;
					}
				}
			}
			else //C!Kayen - Not really sure why this code is here but will leave.
			{
				if (!sof_type || curaa->sof_type == sof_type){
					AddAlternateCurrencyValue(1,cur);
					m_pp.aapoints = 20; //Need to have real AA or client won't let you buy.
					refunded = true;
				}
			}
		}
	}

	if(refunded) {

		//Need to reset player profile after or it will bug.
		uint32 i;
		for(i=0;i<MAX_PP_AA_ARRAY;i++){

			if (aa[i]->AA) {
				Message(13, "Attempt: Reset AA in PP %i value %i.", aa[i]->AA, aa[i]->value);
				SendAA_Struct* curaa2 = zone->FindAA(aa[i]->AA - (aa[i]->value - 1));

				if (curaa2->sof_type == sof_type){
					aa[i]->AA = 0;
					aa[i]->value = 0;
					Message(13, "Reset AA in PP %s.", curaa2->name);
				}
			}
		}
		std::map<uint32,uint8>::iterator itr;
		for(itr=aa_points.begin();itr!=aa_points.end();++itr){
			
			Message(13, "Attempt: Reset aa_point %i %i.", aa_points[itr->first], itr->first);
			if (aa_points[itr->first]){
			
			SendAA_Struct* curaa3 = zone->FindAA(itr->first);

				if (curaa3->sof_type == sof_type){
				aa_points[itr->first] = 0;
				Message(13, "Reset aa_point %s.", curaa3->name);
				}
			}
		}

		Save();
		SpellFinished(1566, this); //Egress instead of Kick() to reset
		//Kick();
	}

	Message(13, "No AA in %i to refund.", sof_type);

	*/
}

uint32 Client::GetAltCurrencyItemid(uint32 alt_currency_id) {
	
	std::list<AltCurrencyDefinition_Struct>::iterator iter = zone->AlternateCurrencies.begin();
	while(iter != zone->AlternateCurrencies.end()) {

	
		if ((*iter).id == alt_currency_id)
			return (*iter).item_id;

		++iter;
	}
	return 0;
}

//#### C!Uncategorized

void Mob::CastOnClosestTarget(uint16 spell_id, int16 resist_adjust,int maxtargets, std::list<Mob*> m_list)
{
	int hit_count = 0;
	int32 AmtHit_mod = GetSpellPowerAmtHitsEffect(spell_id);

	if (spells[spell_id].aemaxtargets)
		hit_count = AOEMaxHitCount(spell_id); //Uses [Numhits] should equal or < aemaxtargets. 

	uint32 CurrentDistance, ClosestDistance = 4294967295u;
	Mob *ClosestMob, *erase_value = nullptr;
	std::list<Mob*>::iterator iter;

	for (int i = 0; i < maxtargets; i++) {

		ClosestMob = nullptr;
		erase_value = nullptr;
		CurrentDistance = 0; 
		ClosestDistance = 4294967295u;
		iter = m_list.begin();
					
		while(iter != m_list.end())
		{
			if (*iter) {
							
				CurrentDistance = (((*iter)->GetY() - GetY()) * ((*iter)->GetY() - GetY())) +
				(((*iter)->GetX() - GetX()) * ((*iter)->GetX() - GetX()));
							
				if (CurrentDistance < ClosestDistance) {
					ClosestDistance = CurrentDistance;
					ClosestMob = (*iter);
				}
			}
					
			++iter;
		}

		if (ClosestMob) {
			
			if (AmtHit_mod)
				ClosestMob->SetSpellPowerAmtHits((i * AmtHit_mod));

			if (!AOEMaxHitCount(spell_id))
				SpellOnTarget(spell_id, ClosestMob, false, true, resist_adjust);
			else{
				//Hit target until hit count is depleted
				int list_size = m_list.size();

				if (list_size > hit_count){
					SpellOnTarget(spell_id, ClosestMob, false, true, resist_adjust);
					hit_count -= 1;
				}
				else {
					while(list_size <= hit_count) {
						SpellOnTarget(spell_id, ClosestMob, false, true, resist_adjust);
						hit_count -= 1;
					}
				}
			}
			m_list.remove(ClosestMob);
		}
	}
}

void Mob::ClientFaceTarget(Mob* MobToFace)
{
	Mob* facemob = MobToFace;
	if(!IsClient() || !facemob) 
		return;

	float oldheading = GetHeading();
	float newheading = CalculateHeadingToTarget(facemob->GetX(), facemob->GetY());
	if(oldheading != newheading) {
		CastToClient()->MovePC(zone->GetZoneID(), zone->GetInstanceID(), GetX(), GetY(), GetZ(), newheading*2);
	}
}

bool Mob::AACastSpell(uint16 spell_id, uint16 target_id, uint16 slot,
	int32 cast_time, int32 mana_cost, uint32* oSpellWillFinish, uint32 item_slot,
	uint32 timer, uint32 timer_duration, int16 *resist_adjust,
	uint32 aa_id)
{
	if (!IsValidSpell(spell_id))
		return false;

	//Wizard Innate Weave of Power AA Toggle
	if (IsAAToggleSpell(spell_id)){
		if (FindBuff(spell_id)){
			BuffFadeBySpellID(spell_id);
			Message(11, "%s disabled.", spells[spell_id].name);
			return true;
		}
	}

	if (!CastSpell(spell_id, target_id, ALTERNATE_ABILITY_SPELL_SLOT, -1, -1, 0, -1, timer, timer_duration, nullptr, aa_id)){
		return false;
	}

	return true;
}

bool Mob::AACastSpellResourceCheck(uint16 spell_id, uint16 target_id)
{
	if (!IsValidSpell(spell_id))
		return false;

	if (spells[spell_id].RecourseLink == 210){
		if (CastToClient()->GetEndurancePercent() <= 20){
			Message(11, "You are too fatigued to use this skill right now.");
			return false;
		}
	}

	return true;
}
bool Mob::PassCasterRestriction(bool UseCasterRestriction,  uint16 spell_id, int16 value)
{
	//IS THIS IS EVEN USED ANYMORE?????? Kayen 10-16-15

	//This value is always defined as a NEGATIVE in the database when doing CasterRestrictions.
	//*NOTE IMPLMENTED YET FOR FROM CastRestriction Field - Will write as needed.
	/*If return TRUE spell met all restrictions and can continue (this = CASTER).
	This check is used when the spell_new field CastRestriction is defined OR spell effect '0'(DD/Heal) has a defined limit

	Range 20000 - 200010	: Limit to CastFromCrouch Interval Projectile
	THIS IS A WORK IN PROGRESS
	*/ 

	if (value >= 0)
		return true;

	if (value == CASTER_RESTRICT_NO_CAST_SELF)
		return true; //Handled else where.

	value = -value; //Convert to positive for calculations

	if (value >= 20000 && value <= 20010) {
		if ((value - 20000) <= GetCastFromCrouchIntervalProj())
			return true;
	}

	return false;
}

void EntityList::TriggeredBeneficialAESpell(Mob *caster, Mob *center, uint16 spell_id)
{ 
	/*
	Special Function to trigger a beneficial AE spell that hits clients at the location of NPC when its buff fades.
	The spell receives bonus from the caster
	*/

	if (!center || !caster)
		return;

	if (!IsValidSpell(spell_id) || !IsBeneficialSpell(spell_id))
		return;

	Mob *curmob;
	float dist = caster->GetAOERange(spell_id);
	float dist2 = dist * dist;
	float dist_targ = 0;

	for (auto it = mob_list.begin(); it != mob_list.end(); ++it) {
		curmob = it->second;
		if (curmob->IsClient() && !curmob->CastToClient()->ClientFinishedLoading())
			continue;
		if (curmob == center)	//do not affect center
			continue;
		if (curmob->IsNPC() && !curmob->IsPet())
			continue;

		dist_targ = DistanceSquared(curmob->GetPosition(), center->GetPosition());

		if (dist_targ > dist2)	//make sure they are in range
			continue;

		if (curmob)
			caster->SpellOnTarget(spell_id, curmob, false, true, 0);
	}
}

void EntityList::ApplyAuraCustom(Mob *caster, Mob *center, uint16 aura_spell_id, uint16 spell_id)
{ 
	//This is not used at present time - See ApplyAuraField
	if (!IsValidSpell(spell_id) || !IsValidSpell(aura_spell_id))
		return;
	
	if (!center || !caster)
		return;

	Mob *curmob;
	float dist = caster->GetAOERange(aura_spell_id);
	float dist2 = dist * dist;
	float dist_targ = 0;

	for (auto it = mob_list.begin(); it != mob_list.end(); ++it) {
		curmob = it->second;
		if (curmob->IsClient() && !curmob->CastToClient()->ClientFinishedLoading())
			continue;
		//if (curmob == center)	//do not affect center
			//continue;
		if (curmob->IsNPC() && !curmob->IsPet())
			continue;

		dist_targ = DistanceSquared(curmob->GetPosition(), center->GetPosition());

		if (dist_targ > dist2)	//make sure they are in range
			continue;

		if (curmob)
			caster->SpellOnTarget(spell_id, curmob, false, true, 0);
	}
}

//#### C!Momentum

void Mob::MomentumDamage(Mob* defender, int32 &damage){
	//Momentum = mass * velocity
	//NPC special attack that lets you set Momentum Damage Mod , Size Modifier, Momentum value(rate), max momentum)
	//float size_mod = defender->GetSize()/100.0f + 1.0f;
	float momentum_mod = GetMomentum()*10;
	float size_mod = defender->GetSize();

	//Shout("Sod [%.2f] Mmod [%.2f] Tmod [%.2f]", size_mod, momentum_mod, size_mod * momentum_mod);
	//Shout("Damage PRE %i ", damage);

	
	damage += static_cast<int>(damage*(momentum_mod)*(size_mod)/100);
	
	if (GetMomentum() > GetMomentumSpeed() * (100/2)) {
		entity_list.MessageClose(this, true, 200, MT_NPCFlurry, "%s slams into %s !", GetCleanName(), defender->GetCleanName());
		defender->Stun(1000);
	}
	SetMomentum(0);
	//Shout("Damage POST %i ", damage);

}

//#### C!DirectionalCalcs

bool Mob::InAngleMob(Mob *other, float start_angle, float stop_angle) const
{ 
	if(!other || other == this)
		return false;

	float angle_start = start_angle + (other->GetHeading() * 360.0f / 256.0f);
	float angle_end = stop_angle + (other->GetHeading() * 360.0f / 256.0f);

	while(angle_start > 360.0f)
		angle_start -= 360.0f;

	while(angle_end > 360.0f)
		angle_end -= 360.0f;

	float heading_to_target = (other->CalculateHeadingToTarget(GetX(), GetY()) * 360.0f / 256.0f);

	while(heading_to_target < 0.0f)
		heading_to_target += 360.0f;

	while(heading_to_target > 360.0f)
		heading_to_target -= 360.0f;

	if(angle_start > angle_end) {
		if((heading_to_target >= angle_start && heading_to_target <= 360.0f) ||	(heading_to_target >= 0.0f && heading_to_target <= angle_end))
			return true;
	}
	else if(heading_to_target >= angle_start && heading_to_target <= angle_end)
		return true;

	return false;
}

bool Mob::SingleTargetSpellInAngle(uint16 spell_id, Mob* spell_target, uint16 target_id){

	if (!spells[spell_id].directional_start && !spells[spell_id].directional_end)
		return true;

	if (target_id && !IsTargetableSpell(spell_id))
		return true;//If used from before casting starts... undecided on this.

	Mob* target = nullptr;

	if (!spell_target)
		target = entity_list.GetMob(target_id);
	else
		target = spell_target;

	if (target ){

		if (target == this)
			return true;

		if (!target->InAngleMob(this, spells[spell_id].directional_start,spells[spell_id].directional_end)){
			//Message_StringID(13,CANT_SEE_TARGET);
			Message(MT_SpellFailure, "You must face your target to use this ability!");
			return false;
		}
	}
	return true;
}

bool Mob::SpellDirectionalTarget(uint16 spell_id, Mob *target)
{
	//!NOT USED!
	//Checks if target is within a spells directioanl cone.

	if (!target)
		return false;

	float angle_start = spells[spell_id].directional_start + (GetHeading() * 360.0f / 256.0f);
	float angle_end = spells[spell_id].directional_end + (GetHeading() * 360.0f / 256.0f);

	while(angle_start > 360.0f)
		angle_start -= 360.0f;

	while(angle_end > 360.0f)
		angle_end -= 360.0f;

	
	float heading_to_target = (CalculateHeadingToTarget(target->GetX(),target->GetY()) * 360.0f / 256.0f);
	
	while(heading_to_target < 0.0f)
		heading_to_target += 360.0f;

	while(heading_to_target > 360.0f)
		heading_to_target -= 360.0f;

	if(angle_start > angle_end)
	{
		if((heading_to_target >= angle_start && heading_to_target <= 360.0f) ||
			(heading_to_target >= 0.0f && heading_to_target <= angle_end))
				return true;
		
		else if(heading_to_target >= angle_start && heading_to_target <= angle_end)
				return true;
	}

	return false;
}

//#### C!CustomSkillBonus

void Mob::SetWpnSkillDmgBonus(SkillUseTypes skill_used, int32 damage)
{return; //Disabled
	if (IsNPC()) {

		int32 DmgAmt = GetMaxHP() * 5 / 100;

		if (WpnSkillDmgBonus[skill_used] < (GetMaxHP() * 3 / 100)) {
			WpnSkillDmgBonus[skill_used] += damage;
			
			if (WpnSkillDmgBonus[skill_used] >= DmgAmt){
				WpnSkillDmgBonus[skill_used] = DmgAmt;

				int pct = 24;
				//NEED TO FINISH MESSAGES
				if (Skill1HBlunt)
					entity_list.MessageClose(this, true, 200, MT_CritMelee, "%s defenses weaken from the many successive blade strikes! (%i / %i)", GetCleanName(), GetWpnSkillDmgBonusAmt(), pct);
				else if (Skill2HBlunt)
					entity_list.MessageClose(this, true, 200, MT_CritMelee, "%s defenses falter under a relentless barrage of blade strikes %s ! (%i / %i)", GetCleanName(), "ice",GetWpnSkillDmgBonusAmt(), pct);
				else if (Skill1HSlashing)
					entity_list.MessageClose(this, true, 200, MT_CritMelee, "%s phyisical resilience succumbs to the power of %s ! (%i / %i)", GetCleanName(), "magic",GetWpnSkillDmgBonusAmt(), pct);
				else if (Skill2HSlashing)
					entity_list.MessageClose(this, true, 200, MT_CritMelee, "%s phyisical resilience succumbs to the power of %s ! (%i / %i)", GetCleanName(), "poison",GetWpnSkillDmgBonusAmt(), pct);
				else if (Skill1HPiercing)
					entity_list.MessageClose(this, true, 200, MT_CritMelee, "%s phyisical resilience succumbs to the power of %s ! (%i / %i)", GetCleanName(), "disease",GetWpnSkillDmgBonusAmt(), pct);
				else if (Skill1HPiercing)
					entity_list.MessageClose(this, true, 200, MT_CritMelee, "%s phyisical resilience succumbs to the power of %s ! (%i / %i)", GetCleanName(), "disease",GetWpnSkillDmgBonusAmt(), pct);
				else if (SkillArchery)
					entity_list.MessageClose(this, true, 200, MT_CritMelee, "%s phyisical resilience succumbs to the power of %s ! (%i / %i)", GetCleanName(), "disease",GetWpnSkillDmgBonusAmt(), pct);
				else if (SkillHandtoHand)
					entity_list.MessageClose(this, true, 200, MT_CritMelee, "%s phyisical resilience succumbs to the power of %s ! (%i / %i)", GetCleanName(), "disease",GetWpnSkillDmgBonusAmt(), pct);
				else if (SkillThrowing)
					entity_list.MessageClose(this, true, 200, MT_CritMelee, "%s phyisical resilience succumbs to the power of %s ! (%i / %i)", GetCleanName(), "disease",GetWpnSkillDmgBonusAmt(), pct);
			}
		}
	}
}

int Mob::GetWpnSkillDmgBonusAmt()
{ return 0; //Disabled
	int _WpnSkillDmgBonus = 0;
	if (IsNPC()) {
		
		_WpnSkillDmgBonus += (100*WpnSkillDmgBonus[Skill1HBlunt])/GetMaxHP();
		_WpnSkillDmgBonus += (100*WpnSkillDmgBonus[Skill2HBlunt])/GetMaxHP();
		_WpnSkillDmgBonus += (100*WpnSkillDmgBonus[Skill1HSlashing])/GetMaxHP();
		_WpnSkillDmgBonus += (100*WpnSkillDmgBonus[Skill2HSlashing])/GetMaxHP();
		_WpnSkillDmgBonus += (100*WpnSkillDmgBonus[Skill1HPiercing])/GetMaxHP();
		_WpnSkillDmgBonus += (100*WpnSkillDmgBonus[SkillHandtoHand])/GetMaxHP();
		_WpnSkillDmgBonus += (100*WpnSkillDmgBonus[SkillArchery])/GetMaxHP();
		_WpnSkillDmgBonus += (100*WpnSkillDmgBonus[SkillThrowing])/GetMaxHP();
	}

	if (_WpnSkillDmgBonus == 24)
		_WpnSkillDmgBonus += 6; //Bonus if ALL skills are met.

	return _WpnSkillDmgBonus;
}

void Mob::SetSpellResistTypeDmgBonus(uint16 spell_id, int32 damage)
{return; //DISABLED
	if (IsNPC()) {

		int32 resist_type = GetSpellResistType(spell_id);
		int32 DmgAmt = GetMaxHP() * 5 / 100;
		
		if (SpellResistTypeDmgBonus[resist_type] < DmgAmt){
			SpellResistTypeDmgBonus[resist_type] += damage;

			if (SpellResistTypeDmgBonus[resist_type] >= DmgAmt){
				SpellResistTypeDmgBonus[resist_type] = DmgAmt;

				int pct = 25;
				//NEED TO FINISH MESSAGES
				if (RESIST_FIRE)
					entity_list.MessageClose(this, true, 200, MT_SpellCrits, "%s spell resilience succumbs to the power of %s ! (%i / %i)", GetCleanName(), "flames", GetSpellResistTypeDmgBonus(), pct);
				else if (RESIST_COLD)
					entity_list.MessageClose(this, true, 200, MT_SpellCrits, "%s spell resilience succumbs to the power of %s ! (%i / %i)", GetCleanName(), "ice",GetSpellResistTypeDmgBonus(), pct);
				else if (RESIST_MAGIC)
					entity_list.MessageClose(this, true, 200, MT_SpellCrits, "%s spell resilience succumbs to the power of %s ! (%i / %i)", GetCleanName(), "magic",GetSpellResistTypeDmgBonus(), pct);
				else if (RESIST_POISON)
					entity_list.MessageClose(this, true, 200, MT_SpellCrits, "%s spell resilience succumbs to the power of %s ! (%i / %i)", GetCleanName(), "poison",GetSpellResistTypeDmgBonus(), pct);
				else if (RESIST_DISEASE)
					entity_list.MessageClose(this, true, 200, MT_SpellCrits, "%s spell resilience succumbs to the power of %s ! (%i / %i)", GetCleanName(), "disease",GetSpellResistTypeDmgBonus(), pct);
			}
		}
	}
}

int Mob::GetSpellResistTypeDmgBonus()
{return 0;//Disabled
	int _SpellResistTypeDmgBonus = 0;
	if (IsNPC()) {
		
		_SpellResistTypeDmgBonus += (100*SpellResistTypeDmgBonus[RESIST_MAGIC])/GetMaxHP();
		_SpellResistTypeDmgBonus += (100*SpellResistTypeDmgBonus[RESIST_FIRE])/GetMaxHP();
		_SpellResistTypeDmgBonus += (100*SpellResistTypeDmgBonus[RESIST_COLD])/GetMaxHP();
		_SpellResistTypeDmgBonus += (100*SpellResistTypeDmgBonus[RESIST_DISEASE])/GetMaxHP();
		_SpellResistTypeDmgBonus += (100*SpellResistTypeDmgBonus[RESIST_POISON])/GetMaxHP();
	}

	if (_SpellResistTypeDmgBonus == 25)
		_SpellResistTypeDmgBonus += 5; //Bonus if ALL skills are met.

	return _SpellResistTypeDmgBonus;
}

//#### C!MiscTargetRing

void Mob::TargetRingTempPet(uint16 spell_id)
{   //Used with spell effect SE_TemporaryPetsNoAggro, which spawms a temporary NPC without a target (Used with Target Ring)
	if (!IsValidSpell(spell_id))
		return;

	char pet_name[64];
	snprintf(pet_name, sizeof(pet_name), "%s`s manifestation", GetCleanName());
	TemporaryPets(spell_id, nullptr, pet_name,false,false); //Create pet.
}

bool Mob::TryTargetRingEffects(uint16 spell_id)
{
	//Function is meant to check various different spell effects that require special behaviors from target ring.
	if (!IsValidSpell(spell_id))
		return false;

	if (spells[spell_id].targettype == ST_Ring || spells[spell_id].targettype == ST_TargetLocation){
	
		for (int i = 0; i <= EFFECT_COUNT; i++) {
			if (spells[spell_id].effectid[i] == SE_TeleportLocation){
				if(IsClient()){
					
					CastToClient()->MovePC(zone->GetZoneID(), zone->GetInstanceID(), GetTargetRingX(), GetTargetRingY(), GetTargetRingZ(), GetHeading()*2);
					Message(MT_Spells, "You enter a temporal rift.");
					SendSpellEffect(spells[spell_id].spellanim, 4000, 0,true, false);
				}
				else
					GMMove(GetTargetRingX(), GetTargetRingY(), GetTargetRingZ(), GetHeading());
			}

			else if (spells[spell_id].effectid[i] == SE_LeapSpellEffect){

				int velocity = spells[spell_id].base[i];
				float zmod1 = static_cast<float>(spells[spell_id].base2[i])/100;
				float zmod2 = zmod1/2.0f;
				int max_range_anim = spells[spell_id].max[i];
				if (!max_range_anim)
					max_range_anim = 3;
				
				//if(IsClient()){
				//	SetLeapSpellEffect(spell_id, spells[spell_id].base[i],0,0, GetTargetRingX(), GetTargetRingY(), GetTargetRingZ(), CalculateHeadingToTarget(GetTargetRingX(), GetTargetRingY()));
				//}
				if (ApplyLeapToPet(spell_id) && GetPet()){

					int anim_speed = 3;

					//Modify NPC jump animation for distance - Still needs some work...
					if (max_range_anim){
						anim_speed =  static_cast<int>(CalculateDistance(GetTargetRingX(), GetTargetRingY(), GetTargetRingZ()))/(static_cast<int>(spells[spell_id].range)/max_range_anim);
						anim_speed = max_range_anim + (max_range_anim - anim_speed);
					}

					//Need range check
					GetPet()->SetPetOrder(SPO_Guard);
					GetPet()->DoAnim(19,anim_speed); //This should be replaced by target animation [Distance 100 = 3]
					GetPet()->SetLeapSpellEffect(spell_id, velocity,zmod1,zmod2, GetTargetRingX(), GetTargetRingY(), GetTargetRingZ(), CalculateHeadingToTarget(GetTargetRingX(), GetTargetRingY()));
				}
			}
		}
	}

	return true;
}

bool Mob::TryLeapSECastingConditions(uint16 spell_id)
{
	if (spells[spell_id].targettype == ST_Ring && IsEffectInSpell(spell_id, SE_LeapSpellEffect) && ApplyLeapToPet(spell_id) && GetPet()){
	
		float dist = GetPet()->CalculateDistance(GetTargetRingX(), GetTargetRingY(), GetTargetRingZ());

		if (dist <= 40){ //Hard coded for now
			Message(MT_SpellFailure, "Your warder is too close to the selected location to perform this ability.");
			return false;
		}

		if (dist > spells[spell_id].range){
			Message(MT_SpellFailure, "Your warder is too far from the selected location to perform this ability.");
			return false;
		}
	}

	return true;

}

void Mob::AENoTargetFoundRecastAdjust(uint16 spell_id)
{
	if (!IsClient())
		return;

	for(int i = 0; i < EFFECT_COUNT; i++)
	{
		if (spells[spell_id].effectid[i] == SE_TryCastonSpellFinished)
		{
			if(spells[spell_id].max[i] == 2) //Flag for when used to adjust recast times based on AE No Cast
			{
				SetAENoTargetFound(true);
				return;
			}
		}
	}
}

void Mob::SetTargetLocationLoc(uint16 target_id, uint16 spell_id)
{
	if (spells[spell_id].targettype == ST_TargetLocation){
		Mob* target = entity_list.GetMob(target_id);
		if (target) {
			m_TargetRing = glm::vec3(target->GetX(), target->GetY(), target->GetZ());
		}
		else {
			m_TargetRing = glm::vec3(0.0f, 0.0f, 0.0f);
		}
	}
}

void Mob::CustomSpellMessages(uint16 target_id, uint16 spell_id, int id){
	
	Mob* target = nullptr;

	if (spells[spell_id].targettype == ST_TargetLocation) {
		target = entity_list.GetMob(target_id);
	
		if (id == 1 && target) //Triggered on starting to cast spell.
			entity_list.MessageClose(this, true, spells[spell_id].aoerange, 15, "The ground beneath you beings to tremble! (%s) (Caster: %s Target: %s AOE: %i : %i) ", spells[spell_id].name, GetCleanName(), target->GetCleanName(), static_cast<int>(spells[spell_id].aoerange), static_cast<int>(spells[spell_id].min_range) );
		else if (id == 2 && target) //Triggered when spell casting is finished.
			target->Message(15, "You avoided %s 's %s ! ",  GetCleanName(), spells[spell_id].name);
		
		return;
	}
}

//#### C!ProjectileTargetRing

bool Mob::ProjectileTargetRing(uint16 spell_id, bool IsMeleeCharge)
{
	if (!IsValidSpell(spell_id))
		return false;

	uint32 duration = 60;
	if (!IsMeleeCharge)
		duration = 10; //Fade in 10 seconds if used from projectile

	TypesTemporaryPets(GetProjectileTargetRingPetDBID(), nullptr, "#",duration, false);
	NPC* temppet = nullptr;
	temppet = entity_list.GetOwnersTempPetByNPCTypeID(GetProjectileTargetRingPetDBID(), GetID(), true, spell_id);
	
	if (temppet){
		temppet->GMMove(GetTargetRingX(), GetTargetRingY(),GetTargetRingZ(), 0, true);
		temppet->ChangeSize(GetSize()); //Seems to work well. - Ensures straight line arc.
		//temppet->ChangeSize(1); //Seems to work well. - Makes projectile land at feet.
		
		if (IsMeleeCharge){

			Mob *hate_top = GetHateMost();
			if (hate_top) {
				SetSpecialAbility(IMMUNE_TAUNT, 1);
				AddToHateList(temppet, CastToNPC()->GetNPCHate(hate_top) + 10000);
				SetSpecialAbility(IMMUNE_AGGRO, 1);
				SetDisableMelee(1);
				SetMeleeChargeActive(true);
				SetMeleeChargeTargetID(temppet->GetID());
			}
			else{
				temppet->Depop();
				return false;
			}
		}

		else {
			if (!TrySpellProjectileTargetRing(temppet, spell_id)){
				temppet->Depop();
				return false;
			}
		}
		
		return true;
	}
	else
		Shout("DEBUG::ProjectileTargetRing: Critical error no temppet (%i) Found in database", GetProjectileTargetRingPetDBID());

	return false;
}

NPC *EntityList::GetOwnersTempPetByNPCTypeID(uint32 npctype_id, uint16 ownerid, bool SetVarTargetRing, uint16 spell_id)
{
	if (npctype_id == 0 || npc_list.empty())
		return nullptr;

	auto it = npc_list.begin();
	while (it != npc_list.end()) {
		if (it->second->GetNPCTypeID() == npctype_id){
			if (it->second->GetSwarmOwner() == ownerid){
				if (SetVarTargetRing){
					if (!it->second->GetUtilityTempPetSpellID()){
						it->second->SetUtilityTempPetSpellID(spell_id);
						return it->second;
					}
				}
				else
					return it->second;
			}
		}
		++it;
	}
	return nullptr;
}


bool Mob::TrySpellProjectileTargetRing(Mob* spell_target,  uint16 spell_id){
	
	if (!spell_target || !IsValidSpell(spell_id))
		return false;
	
	int slot = -1;

	//Make sure there is an avialable bolt to be cast.
	for (int i = 0; i < MAX_SPELL_PROJECTILE; i++) {
		if (ProjectileRing[i].target_id == 0){
			slot = i;
			break;
		}
	}

	if (slot < 0)
		return false;

	const char *item_IDFile = spells[spell_id].player_1;

	bool SendProjectile = true;
	int caster_anim = GetProjCastingAnimation(spell_id);
	float speed = static_cast<float>(GetProjSpeed(spell_id)) / 1000.0f; 
	float angle = static_cast<float>(GetProjAngle(spell_id));
	float tilt = static_cast<float>(GetProjTilt(spell_id));
	float arc = static_cast<float>(GetProjArc(spell_id));

	if (tilt == 525){ //Straightens out most melee weapons. [May need to re evaluate this]
		//angle = 1000.0f;

		if ( (GetZ() - spell_target->GetZ()) > 10.0f) //This may need adjustment
			tilt = 200.0f; //Ajdust til for Z axis - Not perfect but good enough.
	}

	float speed_mod = speed * 0.45f; //Constant for adjusting speeds to match calculated impact time.
	float distance = spell_target->CalculateDistance(GetX(), GetY(), GetZ());
	float hit = 60.0f + (distance / speed_mod);

	//Shout("Debug: Compare Hit Values [NEW %i OLD %i]", static_cast<uint16>(hit), GetOldProjectileHit(spell_target, spell_id));

	ProjectileRing[slot].increment = 1;
	ProjectileRing[slot].hit_increment = static_cast<uint16>(hit);
	ProjectileRing[slot].target_id = spell_target->GetID();
	ProjectileRing[slot].spell_id = spell_id;
	SetProjectileRing(true);
	
	SkillUseTypes skillinuse;
	
	if (caster_anim != 44) //44 is standard 'nuke' spell animation.
		skillinuse = static_cast<SkillUseTypes>(caster_anim);
	else
		skillinuse = SkillArchery;

	if (SendProjectile)
		ProjectileAnimation(spell_target,0, false, speed,angle,tilt,arc, item_IDFile,skillinuse);

	if (IsClient())
		ClientFaceTarget(spell_target);
	else
		FaceTarget(spell_target);
	
	//Override the default projectile animation which is based on item type.
	if (caster_anim == 44)
		DoAnim(caster_anim, 0, true, IsClient() ? FilterPCSpells : FilterNPCSpells);
	
	return true;
}

void Mob::SpellProjectileEffectTargetRing()
{
	if (!HasProjectileRing())
		return;

	for (int i = 0; i < MAX_SPELL_PROJECTILE; i++) {
	
		if (!ProjectileRing[i].increment)
			continue;
		
		if (ProjectileRing[i].increment > ProjectileRing[i].hit_increment){
			Mob* target = entity_list.GetMobID(ProjectileRing[i].target_id);
			if (target){
				uint16 p_spell_id = ProjectileRing[i].spell_id;
				if (IsValidSpell(p_spell_id)){
					if (IsProjectile(p_spell_id)){
						entity_list.AESpell(this, target, p_spell_id, false, spells[p_spell_id].ResistDiff);

						if (HasProjectileAESpellHitTarget())
							TryApplyEffectProjectileHit(p_spell_id, this);
						else{
							//ProjectileTargetRingFailMessage(p_spell_id); //No longer needed since does graphic.
							//This provides a particle effect even if the projectile MISSES. Casts a graphic only spell.
							SendSpellAnimGFX(target->GetID(),GetGraphicSpellID(p_spell_id), (spells[p_spell_id].range + 10.0f));
						}
						
						//Reset Projectile Ring Variables
						SetCastFromCrouchIntervalProj(0);
						SetProjectileAESpellHitTarget(false);
					}
				}
				//target->Depop(); //Depop Temp Pet at Ring Location - Now pets auto depop after 10 seconds
			}

			//Reset Projectile Ring variables.
			ProjectileRing[i].increment = 0;
			ProjectileRing[i].hit_increment = 0;
			ProjectileRing[i].target_id = 0;
			ProjectileRing[i].spell_id = SPELL_UNKNOWN;
			
			if (!ExistsProjectileRing())
				SetProjectileRing(false);

		}
		else
			ProjectileRing[i].increment++;
	}
}

bool Mob::ExistsProjectileRing()
{
	for (int i = 0; i < MAX_SPELL_PROJECTILE; i++) {
	
		if (ProjectileRing[i].increment)
			return true;
	}
	return false;
}

void Mob::TryApplyEffectProjectileHit(uint16 spell_id, Mob* target)
{
	if(!IsValidSpell(spell_id))
		return;

	if (!target)
		return;

	for(int i = 0; i < EFFECT_COUNT; i++){
		if (spells[spell_id].effectid[i] == SE_ApplyEffectProjectileHit){
			if(zone->random.Int(0, 100) <= spells[spell_id].base[i]){

				if (!IsValidSpell(spells[spell_id].base2[i]))
					continue;

				if (!GetCastFromCrouchIntervalProj())
					SpellFinished(spells[spell_id].base2[i], target, 10, 0, -1, spells[spells[spell_id].base2[i]].ResistDiff);

				else {
					for(int j = 0; j < GetCastFromCrouchIntervalProj(); j++){
						SpellFinished(spells[spell_id].base2[i], target, 10, 0, -1, spells[spells[spell_id].base2[i]].ResistDiff);
					}
				}
			}
		}
	}
}

float Mob::CalcSpecialProjectile(uint16 spell_id)
{
	float head = GetHeading()/2;
	float value = 0.0f;
	//These conversions set the angle correctly for each heading for Spectral Blades spell
	//North to West
	if (head >= 0 && head <= 64)
		value = 400 - ((64 - head) * 1.5625f);
	//West to South
	else if (head > 64 && head <= 128)
		value = 570 - ((128 - head) * 2.6562f);
	//South to East
	else if (head > 128 && head <= 192)
		value = 670 - ((192 - head) * 1.5625f);
	//East to North
	else if (head > 192 && head <= 256)
		value = 300 - ((256 - head) * 1.5625f);

	return value;
}

//#### C!SpellProjectileCustom

bool Mob::TrySpellProjectileCustom(Mob* spell_target,  uint16 spell_id){

	if (!spell_target || !IsValidSpell(spell_id))
		return false;

	int slot = -1;

	//Make sure there is an avialable bolt to be cast.
	for (int i = 0; i < MAX_SPELL_PROJECTILE; i++) {
		if (ProjectileAtk[i].target_id == 0){
			slot = i;
			break;
		}
	}

	if (slot < 0)
		return false;

	const char *item_IDFile = spells[spell_id].player_1;

	//Coded narrowly for Enchanter effect.
	if ((strcmp(item_IDFile, "PROJECTILE_WPN")) == 0){
		if (IsClient()) {
			ItemInst* inst = CastToClient()->m_inv.GetItem(MainPrimary);
			if (inst && CastToClient()->IsSpectralBladeEquiped()) 
				 item_IDFile = inst->GetItem()->IDFile;
			else 
				return false;
		}
	}

	bool SendProjectile = true;
	int caster_anim = GetProjCastingAnimation(spell_id);
	float speed = static_cast<float>(GetProjSpeed(spell_id)) / 1000.0f; 
	float angle = static_cast<float>(GetProjAngle(spell_id));
	float tilt = static_cast<float>(GetProjTilt(spell_id));
	float arc = static_cast<float>(GetProjArc(spell_id));

	if (tilt == 525){ //Straightens out most melee weapons. [May need to re evaluate this]
		//angle = 1000.0f;

		if ( (GetZ() - spell_target->GetZ()) > 10.0f) //This may need adjustment
			tilt = 200.0f; //Ajdust til for Z axis - Not perfect but good enough.
	}

	if (CheckLosFN(spell_target)) {
		float speed_mod = speed * 0.45f; //Constant for adjusting speeds to match calculated impact time.
		float distance = spell_target->CalculateDistance(GetX(), GetY(), GetZ());
		float hit = 60.0f + (distance / speed_mod);

		ProjectileAtk[slot].increment = 1;
		ProjectileAtk[slot].hit_increment = static_cast<uint16>(hit); //This projected hit time if target does NOT MOVE
		ProjectileAtk[slot].target_id = spell_target->GetID();
		ProjectileAtk[slot].wpn_dmg = spell_id; //Store spell_id in weapon damage field
		ProjectileAtk[slot].origin_x = GetX();
		ProjectileAtk[slot].origin_y = GetY();
		ProjectileAtk[slot].origin_z = GetZ();
		ProjectileAtk[slot].skill = SkillConjuration;
		ProjectileAtk[slot].speed_mod = speed_mod;

		SetProjectileAttack(true);
	}

	SkillUseTypes skillinuse;

	if (caster_anim != 44) //44 is standard 'nuke' spell animation.
		skillinuse = static_cast<SkillUseTypes>(caster_anim);
	else
		skillinuse = SkillArchery;

	if (SendProjectile){

		ProjectileAnimation(spell_target,0, false, speed,angle,tilt,arc, item_IDFile,skillinuse);

		//Enchanter triple blade effect - Requires adjusting angle based on heading to get correct appearance.
		if (spells[spell_id].spellgroup == SPELL_GROUP_SPECTRAL_BLADE_FLURRY){ //This will likely need to be adjusted.
			ClientFaceTarget(spell_target);
			float _angle =  CalcSpecialProjectile(spell_id);
			//Shout("DEBUG: TrySpellProjectileTargetRingSpecial ::  Angle %.2f [Heading %.2f]", _angle, GetHeading()/2);
			if (GetCastFromCrouchInterval() >= 1){
				ProjectileAnimation(spell_target,0, false, speed,_angle,tilt,600, item_IDFile,skillinuse);
				ProjectileAnimation(spell_target,0, false, speed,(_angle - 70),tilt,600, item_IDFile,skillinuse);
			}
			if (GetCastFromCrouchInterval() >= 2){
				ProjectileAnimation(spell_target,0, false, (speed - 0.2f),_angle,tilt,600, item_IDFile,skillinuse);
				ProjectileAnimation(spell_target,0, false, (speed - 0.2f),(_angle - 70),tilt,600, item_IDFile,skillinuse);
			}

			if (GetCastFromCrouchInterval() >= 3){
				ProjectileAnimation(spell_target,0, false, (speed - 0.4f),_angle,tilt,600, item_IDFile,skillinuse);
				ProjectileAnimation(spell_target,0, false, (speed - 0.4f),(_angle - 70),tilt,600, item_IDFile,skillinuse);
			}
		}
	}
	
	//Override the default projectile animation which is based on item type.
	if (caster_anim == 44)
		DoAnim(caster_anim, 0, true, IsClient() ? FilterPCSpells : FilterNPCSpells); 

	return true;
}		

//#### C!MeleeCharge

void Mob::MeleeCharge()
{
	if (!IsMeleeChargeActive())
		return;

	Shout("IS MOVING %i", IsMoving());

	if (!IsMoving()){
		Shout("TODO: Trigger MeleeChargeEffect");
		SetSpecialAbility(IMMUNE_TAUNT, 0);
		SetSpecialAbility(IMMUNE_AGGRO, 0);
		SetDisableMelee(0);
		SetMeleeChargeActive(false);
		
		Mob* target = entity_list.GetMobID(GetMeleeChargeTargetID());
		if (target) {
			target->Depop();
			SetMeleeChargeTargetID(0);
		}
	}
}

//#### C!BaseSpellPower

int32 Mob::GetBaseSpellPower(int32 value, uint16 spell_id, bool IsDamage, bool IsHeal, int16 buff_focus)
{
	/*
	Non focus % based stackable spell modifiers. - Works on NPC and Clients
	*Order of custom multipliers*
	PRE FOCUS BASE MODIFIERS
	--------------------------------------------------------------
	1. Distance Modifier
	2. CalcTotalBaseModifierCurrentHP - See function for included effects (all mods add together but typically will only have one)
	------------------------------------------------------------
	3. Wizard Innate Buff / Enchanter Mana Modifier
	4. Stackable BaseSpellPower
	------------------------------------------------------------
	5. Regular Focus / Damage Adders / Vulnerability
	*/

	if (!IsValidSpell(spell_id))
		return 0;

	int16 mod = 0;
	//("Mob::GetBaseSpellPower Buff Slot Focus %i  slot %i ", buff_focus , buff_focus);
	if (buff_focus >= 0) //Default is -1 (Therefore we only check this when checking over time)
		value += value*buff_focus/100;
	else {
		value += value*GetBaseSpellPowerWizard()/100; //Wizard Special
		
		if (CastFromPetOwner(spell_id))
			value += value*GetSpellPowerModFromPet(spell_id)/100;
		else
			value += value*CalcSpellPowerManaMod(spell_id)/100;//Enchanter Special
	}

	mod = spellbonuses.BaseSpellPower + itembonuses.BaseSpellPower + aabonuses.BaseSpellPower; //All effects
	
	//Heal
	if (IsHeal)
		mod += spellbonuses.BaseSpellPowerHeal + itembonuses.BaseSpellPowerHeal + aabonuses.BaseSpellPowerHeal;
	//Dmg
	if (IsDamage){
		mod +=	spellbonuses.BaseSpellPowerDmg[spells[spell_id].resisttype] + 
				itembonuses.BaseSpellPowerDmg[spells[spell_id].resisttype] + 
				aabonuses.BaseSpellPowerDmg[spells[spell_id].resisttype];
	}

	value += value*mod/100;
	//Shout("Mob::GetBaseSpellPower Final Base Focus value %i mod %i", value, mod);
	return value; //This is final damage/heal or whatever returned.
}

int32 Mob::CalcTotalBaseModifierCurrentHP(int32 damage, uint16 spell_id, Mob* caster, int effectid)
{
	int mod = 0;
	mod += GetSpellPowerAmtHits(); //Scale based on how many targets were hit by spell prior to this target.
	mod += CalcSpellPowerHeightMod(spell_id, caster);
	mod += CalcFromCrouchMod(spell_id,caster, effectid);
	mod += CalcSpellPowerFromBuffSpellGroup(spell_id, caster);
	mod += CalcSpellPowerAmtClients(spell_id, effectid, caster);
	mod += CalcSpellPowerTargetPctHP(spell_id, caster);

	//mod += CalcSpellPowerFromAEDuration(spell_id, caster, 1);
	//mod += caster->GetAEDurationIteration() * 10; //Need to revise this.

	Shout("DEBUG::CalcTotalBaseModifierCurrentHP :: PRE DMG %i Mod %i", damage,mod);
	if (mod)
		damage += damage*mod/100;

	Shout("DEBUG::CalcTotalBaseModifierCurrentHP :: POST DMG %i Mod %i", damage,mod);
	return damage;
}

//#### C!LastName

void Mob::ChangeNPCLastName(const char* in_lastname) 
{

	EQApplicationPacket* outapp = new EQApplicationPacket(OP_GMLastName, sizeof(GMLastName_Struct));
	GMLastName_Struct* gmn = (GMLastName_Struct*)outapp->pBuffer;
	strcpy(gmn->name, GetName());
	strcpy(gmn->gmname, GetName());
	strcpy(gmn->lastname, in_lastname);
	gmn->unknown[0]=1;
	gmn->unknown[1]=1;
	gmn->unknown[2]=1;
	gmn->unknown[3]=1;
	entity_list.QueueClients(this, outapp, false);
	safe_delete(outapp);
}

void Mob::ClearNPCLastName()
{
	std::string WT;
	WT = '\0'; //Clear Last Name
	ChangeNPCLastName( WT.c_str());
}

void Mob::SpellCastingTimerDisplay()
{

	/*Will display a cast time count down in 1 second interval which accurate to actual cast time.
	Packet is only sent once per second max
	To do potentially start a 1 second timer to sync with the count down start time, unsure if neccessary.
	*/
	if (IsCasting() && IsEngaged()){
		
		uint32 remain_time = GetSpellEndTime().GetRemainingTime(); 
		uint32 flat_time = remain_time / 1000;
		uint32 flat_time_cmp =  (flat_time * 1000) + 30;
		//Shout("DEBUG: remain %i < flat_time_cmp %i", remain_time, flat_time_cmp);
		if (remain_time < flat_time_cmp) {

			std::string WT;

			//Build string to send if cast is not completed.
			if (remain_time > 35){

				WT = spells[CastingSpellID()].name;
				WT += " ";
				WT += "< ";
				WT += itoa(flat_time);
				WT += " >";
				//Shout("DEBUG: Remain Time %i :: %s", GetSpellEndTime().GetRemainingTime(), WT);
			
				if (strlen(WT.c_str()) >= 64)
					WT = '\0'; //Prevent buff overflow
			}

			else
				WT = '\0'; //Clear Last Name

			ChangeNPCLastName( WT.c_str());
		}
	}
}

//#### C!AdjustRecast

void Client::DoAdjustRecastTimer()
{
	if (!HasAdjustRecastTimer())
		return;

	bool Disable = false;

	for(int i =0 ; i < MAX_PP_MEMSPELL; ++i) {
		if(recast_mem_spells[i]) {
			if (IsValidSpell(m_pp.mem_spells[i])){
				uint32 RemainTime = GetPTimers().GetRemainingTime(pTimerSpellStart + CastToClient()->m_pp.mem_spells[i]);
				//Shout("DEBUG: DoAdjustRecastTimer(): RemainTime %i RecastEnd %i   [%s]", RemainTime,recast_mem_spells[i], spells[m_pp.mem_spells[i]].name);
				//Shout("DEBUG: DoAdjustRecastTimer(): <%s> %i < %i", spells[m_pp.mem_spells[i]].name, RemainTime, recast_mem_spells[i]);
				if (RemainTime <= recast_mem_spells[i]){
					//Shout("DEBUG: DoAdjustRecastTimer::: Trigger (Adjust Time) [%i] NAME: %s", recast_mem_spells[i], spells[CastToClient()->m_pp.mem_spells[i]].name);
					
					//Cast the Refresh Spell specified in the array.
					if (IsValidSpell(refreshid_mem_spells[i])){
						SpellFinished(refreshid_mem_spells[i],this);
					}
				
					recast_mem_spells[i] = 0;
					refreshid_mem_spells[i] = 0;
					Disable = true;
				}

				else
					Disable = false;
			}
		}
	}

	if (Disable){
		SetAdjustRecastTimer(false);
		adjustrecast_timer.Disable();
	}
}

void Client::EffectAdjustRecastTimer(uint16 spell_id, int effectid)
{
	/*
	Complete Hack way of adjusting a spells recast timer after spell has been cast. **WE ONLY LOOK FOR MATCHING 'SPELL GROUPS'
	Use:	Spell Effect 1007 SE_AdjustRecastTimer (Can be primary)
			Base = Flat Amt Recast Time decreased (Should be in 1000 MS intervals)
			Limit = Refresher Spell ID
			Max =  UNUSED
			Ie. Set Recast Time to 2 seconds for Spell ID 1000 using Limit Spell ID 1001 to refresh gem.
	
	Use:	Spell Effect 1008 SE_AdjustRecastTimerCondition (Put with effect you want it to trigger on with using SE 1009) 
			Base = Flat Amt Recast Time decreased (Should be in 1000 MS intervals)
			Limit = Refresher Spell ID
			Max = Condition to activiate [This is left open ended for specific uses]
			Ie. Set Recast Time to 2 seconds for 'THIS' Spell ID using Limit Spell ID 1001 to refresh gem when condition X is met

	When triggered it searches player profile for matching spell [ONLY MATCHES BY SPELL GROUP] that has a recast time remaining
	great than what the new time would be set to. If found it puts the new recast time value into
	an array (that matches player profile for mem_spells) and activiates the timer(check in clientprocess DoAdjustRecastTimer()).
	When the current remaining time matches the new remaining time a spell using SE_FcTimerRefresh (id stored in array) with
	limits matching the specific SPELL GROUP used is triggered and thus the gem is restored. 
	
	Interval timer for DoAdjustRecastTimer set at 1 second. (May need adjust but seems fine

	SE_TryCastonSpellFinished 1009 is the effect used to 'recourse' the conditional version so that it triggers off that base spell.
	This is checked by TryCastonSpellFinished() function at the end of spellsfinsihed.
	*/

	uint32 recast_adjust = 0;
	uint16 spellid_refresh = 0;
	uint16 spellgroupadjust = 0;
	uint16 memspell_id = 0;
	uint32 RemainTime = 0;
	bool SpellGroupLimited = false;

	if(IsValidSpell(spell_id)){
		
		recast_adjust = spells[spell_id].base[effectid]/1000; //Time amount subtracted
		spellid_refresh = spells[spell_id].base2[effectid];  //Spell ID that does the refresh effect.

		if (!recast_adjust)
			return; //Incase we set to zero as a place holder just end here.

		if(!IsValidSpell(spellid_refresh))
			return; //NO Refresher ID
		
		spellgroupadjust = GetSpellGroupFromLimit(spellid_refresh); //Obtains spell group from the refresh spell LIMIT
		
		if (spellgroupadjust)
			SpellGroupLimited = true;
	}

	//Shout("DEBUG: EffectAdjustRecastTimer::: Recast Adjust:[ %i] SpellRefresh ID: [%i] SpellGroup: [%i] Spell Group Limit Bool [%i]", recast_adjust, spellid_refresh, spellgroupadjust,SpellGroupLimited);
	if (recast_adjust){
		for(unsigned int i =0 ; i < MAX_PP_MEMSPELL; ++i) {
			memspell_id = m_pp.mem_spells[i];
			RemainTime = 0;
			if(IsValidSpell(memspell_id)) {
			//Shout("DEBUG: [%i]	pp.Group [%i] SpelltoAdjust.Group",i, spells[memspell_id].spellgroup, spellgroupadjust);

				if (SpellGroupLimited && (spellgroupadjust && (spells[memspell_id].spellgroup == spellgroupadjust)))
					RemainTime = GetPTimers().GetRemainingTime(pTimerSpellStart + CastToClient()->m_pp.mem_spells[i]);
				else if (!SpellGroupLimited)
					RemainTime = GetPTimers().GetRemainingTime(pTimerSpellStart + CastToClient()->m_pp.mem_spells[i]);

				//Shout("DEBUG: RemainTime %i  > recast_adjust %i", RemainTime, recast_adjust);
				if (RemainTime && (RemainTime > recast_adjust)){
					recast_mem_spells[i] = recast_adjust;
					refreshid_mem_spells[i] = spellid_refresh;
					//Shout("DEBUG: EffectAdjustRecastTimer::: <Activiated %s > Recast ADjust %i [ %i ] Remain Time: %i ", spells[memspell_id].name,recast_adjust,recast_mem_spells[i], RemainTime);
					SetAdjustRecastTimer(true);
					adjustrecast_timer.Start(1000);
				}
			}
		}
	}
	
	//Shout("DEBUG: %i SE_FcTimerRefresh:::: Remain %i", i, CastToClient()->GetPTimers().GetRemainingTime(pTimerSpellStart + CastToClient()->m_pp.mem_spells[i]));
	//Shout("DEBUG: %i SE_FcTimerRefresh:::: Expired %i",i,CastToClient()->GetPTimers().Expired(&database, pTimerSpellStart + spell_id, false));
}

uint16 Mob::GetSpellGroupFromLimit(uint16 spell_id)
{
	if (!IsValidSpell(spell_id))
		return 0;

	for (int i = 0; i <= EFFECT_COUNT; i++)
	{
		if (spells[spell_id].effectid[i] == SE_LimitSpellGroup){
			if (spells[spell_id].base[i])
				return spells[spell_id].base[i];
		}
	}
	return 0;
}


//#### C!CastFromCrouch - Spell Field CastFromCrouch

bool Client::CastFromCrouch(uint16 spell_id)
{
	/* 
	Allow for 'charging' of effects where cast time is the charger and duck/jump is the release.
	This function is checked in	Client::Handle_OP_SpawnAppearance when a duck/jump appearance packet is sent.
	Field 170 repurposed customly to CastFromCrouch which flags a spell able to check this.
	Value of CastFromCrouch field is an effect modifier.
	
	Method 1: Damage multiplied by Amount of Cast Time used.
	When a setting a spell to use this, the base damage/heal is the MIN amount it can do.
	The value is then increased base on the amount of cast time elapased. Where max damage % increase = Cast Time / 100
	The value of CastFromCrouch field can then be used the modify this modifer.
	
	Method 2: Damage based on Cast Time %
	When setting a spell to use this, the base damage/heal value is the MAX amount it can do.
	This value is then decreased based on the percentage of cast time remaining.
	The value of CastFromCrouch field can then be used to modify the cast time percent modifier

	*USED*Method 3: Modifier based on charge intervals 1-5 (each 20%)
	You get a set modifier for ending charge for intervals of 20% on the cast bar with 5 being the highest.
	This set as 1-5 in SetCastFromCrouchInterval() and used to correlate for conditionals
	
	spells[spell_id].cast_from_crouch //Value of 100 = No MOD
	spells[spell_id].numhits - Will determine special interval with default = 5

	[IMPORTANT] case SE_CurrentHP: Final Modifier is check in this function to adjust base value.

	TODO: Need to likely increase the mod the longer the cast time to give incentive OR make recast times appropriately different.
	*/


	if(spell_id == SPELL_UNKNOWN)
		spell_id = casting_spell_id;

	if (!IsValidSpell(spell_id) || !spells[spell_id].cast_from_crouch|| !IsCasting())
		return false;

	int32 mod = 0;
	int32 t_start = GetActSpellCasttime(spell_id, spells[spell_id].cast_time);
	uint32 remain_time = spellend_timer.GetRemainingTime();
	int32 time_casting = t_start - remain_time; //MS
	int32 pct_casted = 100 - (remain_time*100/t_start);
	int charge_interval = spells[spell_id].numhits;

	if (!charge_interval)
		charge_interval = 5;

	/*Method 1
	mod = (time_casting)/100;
	mod = mod*spells[spell_id].cast_from_crouch/100;
	*/

	/*Method 2
	mod = 100 - (remain_time*100/t_start); // % Cast Time Used
	mod = mod*spells[spell_id].cast_from_crouch/100;
	mod = mod * -1;
	*/

	//Default
	if (charge_interval == 5) {
		if (pct_casted >= 0 && pct_casted <= 20)
			charge_interval = 1;
		else if (pct_casted > 20 && pct_casted <= 40)
			charge_interval = 2;
		else if (pct_casted > 40 && pct_casted <= 60)
			charge_interval = 3;
		else if (pct_casted > 60 && pct_casted <= 80)
			charge_interval = 4;
		else if (pct_casted > 80 && pct_casted <= 100)
			charge_interval = 5;
	}

	else if (charge_interval == 3) {
		if (pct_casted >= 0 && pct_casted <= 34)
			charge_interval = 1;
		else if (pct_casted > 34 && pct_casted <= 68)
			charge_interval = 2;
		else if (pct_casted > 68 && pct_casted <= 100)
			charge_interval = 3;
	}

	//mod = 100 - (remain_time*100/t_start); // % Cast Time Used
	//mod = mod*spells[spell_id].cast_from_crouch/100;

	//SetChargeTimeCasting(time_casting);
	if (IsProjectile(spell_id)){ //Need to use seperate variable if dealing with projectile (clears at different time).
		SetCastFromCrouchIntervalProj(charge_interval);
		SetCastFromCrouchInterval(charge_interval); //For recast adjust code purposes.
	}
	else
		SetCastFromCrouchInterval(charge_interval);
	
	spellend_timer.Start(1);

	//Set Standing Apperance from duck/jump
	/*
	SendAppearancePacket(AT_Anim, ANIM_STAND);
	playeraction = 0;
	SetFeigned(false);
	BindWound(this, false, true);
	camp_timer.Disable();
	*/

	return true;
}

int32 Mob::CalcFromCrouchMod(uint16 spell_id, Mob* caster, int effectid){

	if (!caster || (IsValidSpell(spell_id) && !spells[spell_id].cast_from_crouch))
		return 0;

	int32 interval = 0;
	if (IsProjectile(spell_id)){
		/*Fire MORE projectiles instead of scaling. - Limited case [base2 = -1 through -3 to determine how many hits]
		SEE Mob::ApplyCastFromCrouchProjectileDamage*/
		if (spells[spell_id].spellgroup == SPELL_GROUP_SPECTRAL_BLADE_FLURRY)
			return 0;

		interval = caster->GetCastFromCrouchIntervalProj();
	}
	else
		interval = caster->GetCastFromCrouchInterval();

	//Define (-1) if no damage modifier is applied from the crounch spell (Ie when it mods somethinge else)
	if (!interval || spells[spell_id].cast_from_crouch == -1)
		return 0;

	//Spell will fizzel if allowed to go full duration.
	//if (!interval)
		//interval = 5; //Indicates that casting time completely finished.

	int32 modifier = (interval - 1)*100;
	modifier = modifier*spells[spell_id].cast_from_crouch/100; //Base cast_from_crouch = 100;
	
	/*
	if (modifier)
		damage += damage*modifier/100;
	*/

	return modifier;

}

int32 Mob::CalcCrouchModFromType(uint16 spell_id, int type)
{
	//Valid spell checked earlier.
	if (!spells[spell_id].cast_from_crouch)
		return 0;

	/*
	1 = empty
	2 = Range Mod - Percent Increase
	3 = Stun Mod - Flat second increase
	*/

	for(int i = 0; i < EFFECT_COUNT; i++){
		if (spells[spell_id].effectid[i] == SE_SpellPowerCrouchType){
			if (type == spells[spell_id].base2[i]){
				return GetCastFromCrouchInterval() * spells[spell_id].base[i];
			}
		}
	}
	return 0;
}

bool Mob::ApplyCastFromCrouchProjectileDamage(uint16 spell_id, int16 limit)
{
	if (limit > 0)
		return false;
	/*
	Determines how many direct damage spells will be applied based on Charge Interval 
	[Set to Negative to avoid conflict with CasterRestriction Check]
	*/
	if (spells[spell_id].spellgroup == SPELL_GROUP_SPECTRAL_BLADE_FLURRY)
	{
		limit = -limit;

		if (limit <= GetCastFromCrouchIntervalProj())
			return true;
	}
	return false;
}


//#### C!Wizard :: Functions related to spell power from endurance

int16 Mob::GetBaseSpellPowerWizard()
{
	int16 wizard_bonus = spellbonuses.BaseSpellPowerWizard + itembonuses.BaseSpellPowerWizard + aabonuses.BaseSpellPowerWizard;
	if (wizard_bonus && IsClient()){
		if (GetEndurancePercent() >= 20){
			SetWizardInnateActive(true);
			return wizard_bonus;
		}
		else {
			Message(11, "You are too fatigued to use this skill right now.");
			SetWizardInnateActive(false);
		}
	}

	return 0;
}

void Mob::TryWizardEnduranceConsume(uint16 spell_id)
{
	if (IsWizardInnateActive() && IsClient()){ //Wizard innate 'weave of power'
		CastToClient()->SetEndurance(CastToClient()->GetEndurance() - CastToClient()->GetMaxEndurance()/5);
		SetWizardInnateActive(false);
	}
}

//#### C!Enchanter :: Functions related to spell power from mana amount

int32 Mob::CalcSpellPowerManaMod(uint16 spell_id)
{
	if (GetClass() != ENCHANTER || !IsValidSpell(spell_id))
		return 0;

	//Returns spell modifer based on amount of mana coverted to focus
	//Base1: AMOUNT of mana required for 1 percent focus increase
	//Base2: UNUSED
	
	int effect_mod = 0;
	int limit_mod = 0;

	for(int i = 0; i < EFFECT_COUNT; i++){
		if (spells[spell_id].effectid[i] == SE_SpellPowerManaMod){
			effect_mod = spells[spell_id].base[i]; //AMT percent increase per 1 mana (divided by 1000)
			limit_mod = spells[spell_id].base2[i]; //UNUSED
			break;
		}
	}

	if (!effect_mod)
		return 0;

	return ((GetMana() * effect_mod)/1000);
}

/*
int32 Mob::CalcSpellPowerManaMod(uint16 spell_id)
{
	if (GetClass() != ENCHANTER || !IsValidSpell(spell_id))
		return 0;

	//int32 effect_mod = GetSpellPowerManaModValue(spell_id); //Percent increase of base damage.
	int effect_mod = 0;
	int limit_mod = 0;

	for(int i = 0; i < EFFECT_COUNT; i++){
		if (spells[spell_id].effectid[i] == SE_SpellPowerManaMod){
			effect_mod = spells[spell_id].base[i]; //Modifies bonus from mana ratio interval
			limit_mod = spells[spell_id].base2[i]; //Modifies bonus from max mana amount
			break;
		}
	}
	
	//If set to (-1) then do not focus effect, but will still require/drain mana.
	if (!effect_mod || effect_mod < 0)
		return 0;

	uint8 pct_mana = GetManaPercent();

	int mana_amt_mod = 0;
	int mana_divider = 1000; //Default value, overwritten by Limit value.
	
	if (limit_mod > 0)
		mana_divider = limit_mod;

	if (limit_mod != -1 && (mana_divider >= 5))
		mana_amt_mod = ((GetMaxMana()*100)/5)/(mana_divider/5); //For every 1000 mana gain 1% bonus (This will need to be tuned).
	
	int focus_mod = 0;

	if (pct_mana >= 20 && pct_mana  < 40)
		focus_mod = 0;
	else if (pct_mana >= 40 && pct_mana < 60)
		focus_mod = 1;
	else if (pct_mana >= 60 && pct_mana < 80)
		focus_mod = 2;
	else if (pct_mana > 80 && pct_mana < 100)
		focus_mod = 3;
	else if (pct_mana == 100)
		focus_mod = 4;

	int total_mod = (effect_mod*focus_mod) + ((mana_amt_mod * focus_mod) / 100);
	//Shout("DEBUG::CalcSpellPowerManaMod :: Effect Mod %i Mana_Mod %i Total Mod %i", (effect_mod*focus_mod), ((mana_amt_mod * focus_mod) / 100), total_mod);
	return total_mod;
}
*/

bool Mob::TryEnchanterCastingConditions(uint16 spell_id)
{
	if (IsClient() && GetClass() == ENCHANTER) {
		
		/*
		if (GetSpellPowerManaModValue(spell_id)) {
			if (GetManaPercent() >= 20){
				return true;
			}
			else { 
				//Message_StringID(13, INSUFFICIENT_MANA);
				Message(13, "Insufficient Mana to cast this spell! This ability requires at least 20 percent Mana.");
				return false;
			}
		}
		*/

		//For ENC class abilities that require Spectral Blade equiped - Item LightType = 1 and Spell LightType = 1
		if((SpellRequiresSpectralBlade(spell_id)) && !CastToClient()->IsSpectralBladeEquiped()){
			Message(MT_SpellFailure, "You must have a spectral blade equiped to use this ability.");
			return false;
		}		

		/*
		//Check if 'Mind Over Matter' Mana drain from tanking effect - Can not cast spectral blades while active.
		if (spells[spell_id].spellgroup == 2000 || spells[spell_id].spellgroup == 2006){ //Spectral Blade spells
			if (spellbonuses.ManaAbsorbPercentDamage[0]){
				Message(13, "You lack the concentratation to project spectral blades while using using Mind Over Matter.");
				Debug("DEBUG: Mob::TryEnchanterCastingConditions :: This may need to be altered.");
				return false;
			}
		}
		*/
	}
	return true;
}

void Mob::TryEnchanterManaFocusConsume(uint16 spell_id)
{
	if (IsClient() && GetClass() == ENCHANTER && IsValidSpell(spell_id)) {
		for(int i = 0; i < EFFECT_COUNT; i++){
			if (spells[spell_id].effectid[i] == SE_SpellPowerManaMod){
			
				if (spells[spell_id].base[i]){
					SetMana(0);
				}
			}
		}
	}
}

int32 Mob::GetSpellPowerModFromPet(uint16 spell_id)
{
	//Used with enchanter casted swarm pets who channel spells through the enchanter, therefore we transfer
	//the derived spell power mod (From NPC::ApplyCustomPetBonuses(Mob* owner, uint16 spell_id);) to the pets owner.
	if (!IsValidSpell(spell_id))
		return 0;

	int focus = 0;
	Mob* pet = nullptr;
	
	if (GetOriginCasterID()){
		pet = entity_list.GetMob(GetOriginCasterID());

		if (pet)
			focus = pet->CastToNPC()->GetSpellFocusDMG();
	}

	return focus;
}

void NPC::ApplyCustomPetBonuses(Mob* owner, uint16 spell_id)
{
	if (!owner || !IsValidSpell(spell_id))
		return;

	std::string WT;
	int size_divider = 0;

	int mod = 0;
	int enc_mod = owner->CalcSpellPowerManaMod(spell_id);
	//Need to set a stat on NPC that is equal to the spell ID to be cast.
	mod = enc_mod;

	//1: Check for any special pet 'type' behaviors
	const char *pettype = spells[spell_id].player_1; //Constant for each type of pet

	//Enchater Pets
	if ((strcmp(pettype, "spectral_animation")) == 0){
		WearChange(7,owner->GetEquipmentMaterial(MaterialPrimary),0); //ENC Animation spell to set graphic same as sword.
		WearChange(8,0,0);
		SetOnlyAggroLast(true);
		SpellFinished(2013, this, 10, 0, -1, spells[spell_id].ResistDiff);
		WT = owner->GetCleanName();	
		WT += "'s_animation";
		if (strlen(WT.c_str()) <= 64)
			TempName(WT.c_str());
	}

	else if ((strcmp(pettype, "reaper")) == 0){
		SpellFinished(2050, this, 10, 0, -1, spells[spell_id].ResistDiff);
		size_divider = 2;
		WT = owner->GetCleanName();	
		WT += "'s_reaper";
		if (strlen(WT.c_str()) <= 64)
			TempName(WT.c_str());
	}

	else if ((strcmp(pettype, "tk_bladestorm")) == 0){
		WT = owner->GetCleanName();	
		size_divider = 20;
		WT += "'s_bladestorm";
		if (strlen(WT.c_str()) <= 64)
			TempName(WT.c_str());
		SetOnlyAggroLast(true);
		//SendSpellEffect(567, 500, 0, 1, 3000, true);
	}

	//Ranger Pets
	else if ((strcmp(pettype, "rng_mistwalker")) == 0){
		SpellFinished(4015, this, 10, 0, -1, spells[spell_id].ResistDiff);
		WT = owner->GetCleanName();	
		WT += "'s_mistwalker";
		if (strlen(WT.c_str()) <= 64)
			TempName(WT.c_str());
	}

	//2: Target RING - Determine if target pets spawn at location or path to location. [Limit value in SE_TemporaryPetNoAgggro]
	if (spells[spell_id].targettype == ST_Ring && (IsEffectInSpell(spell_id,SE_TemporaryPetsNoAggro))){
		
		int limit = 0;
		for(int i = 0; i < EFFECT_COUNT; i++){
			if (spells[spell_id].effectid[i] == SE_TemporaryPetsNoAggro)
				limit = spells[spell_id].base2[i];
		}

		if (!limit)
			GMMove(owner->GetTargetRingX(), owner->GetTargetRingY(), owner->GetTargetRingZ(), GetHeading(), true);
		else if (limit == 1){
			auto position = glm::vec4(owner->GetTargetRingX(), owner->GetTargetRingY(),owner->GetTargetRingZ(), GetHeading());
			MoveTo(position, true);
		}
	}

	//3. Effect Fields on Temp Pets
	if (IsEffectFieldSpell(spell_id)){
		TempName("#");
		for(int i = 0; i < EFFECT_COUNT; i++){
			if (spells[spell_id].effectid[i] == SE_CastEffectFieldSpell){
				if (IsValidSpell(spells[spell_id].base[i]))
					owner->SpellOnTarget(spells[spell_id].base[i], this, false, true, 0);
			}
		}
		SetOnlyAggroLast(true);
	}
	

	//4: Scale pet based on any focus modifiers.
	//Shout("DEBUG: ApplyCustomPetBonuses :: PRE Mod %i :: MaxHP %i MaxDmg %i MinDmg %i AC %i Size %.2f ", mod, base_hp, max_dmg, min_dmg, AC, GetSize());
	
	base_hp  += base_hp * mod / 100;
	max_dmg += max_dmg * mod / 100;
	min_dmg += min_dmg * mod / 100;
	AC += AC * mod / 100;
	
	if (size_divider)
		ChangeSize(GetSize() + (GetSize() * static_cast<float>(mod/size_divider) / 100));

	SetSpellFocusDMG(GetSpellFocusDMG() + mod);
	SetSpellFocusHeal(GetSpellFocusHeal() + mod);
    //Shout("DEBUG: ApplyCustomPetBonuses :: POST Mod %i :: MaxHP %i MaxDmg %i MinDmg %i AC %i Size %.2f ", mod, base_hp, max_dmg, min_dmg, AC, GetSize());
	SetHP(1000000);
}

bool Client::IsSpectralBladeEquiped()
{
	if (GetClass() != ENCHANTER)
		return false;

	ItemInst* inst = m_inv.GetItem(MainPrimary);
	
	if (inst && inst->GetItem()->Light == 1 && inst->GetItem()->ItemType ==  ItemType1HPiercing)
		return true;

	else 
		return false;
}

//C!SpellEffects :: SE_CastOnSpellCastCountAmt

void Mob::TryCastonSpellCastCountAmt(int slot, uint16 spell_id)
{
	if (!IsClient())
		return;

	//This is the general cast count incrementer for all spells
	if (CastToClient()->GetSpellCastCount(slot) < 65535)
		CastToClient()->SetSpellCastCount(slot, SPELL_UNKNOWN, (CastToClient()->GetSpellCastCount(slot) + 1));


	//This function is primarily used to reset recast timers after a spell has been cast a specific amount of time.
	for(int i = 0; i < EFFECT_COUNT; i++)
	{
		if (spells[spell_id].effectid[i] == SE_CastOnSpellCastCountAmt)
		{
			uint16 cast_count = CastToClient()->GetSpellCastCount(-1, spell_id);
			if (cast_count == (spells[spell_id].max[i])){
				if (IsValidSpell(spells[spell_id].base2[i])){
					if(zone->random.Int(0, 100) <= spells[spell_id].base[i]) {
						SpellFinished(spells[spell_id].base2[i], this, 10, 0, -1, spells[spells[spell_id].base2[i]].ResistDiff);
						return;
					}
				}
			}
			
			//The next cast after the trigger reset the cast count.
			else if (cast_count == (spells[spell_id].max[i]+1)){
				CastToClient()->SetSpellCastCount(-1, spell_id, 0);
				return;
			}
		}
	}
}

uint16 Client::GetSpellCastCount(int slot, uint16 spell_id)
{
	if (spell_id == SPELL_UNKNOWN){
		if (slot >= 0 && slot < MAX_PP_MEMSPELL)
		return spell_cast_count[slot];
	}

	else if (IsValidSpell(spell_id)){

		for(unsigned int i =0 ; i < MAX_PP_MEMSPELL; ++i) {
			if(IsValidSpell(m_pp.mem_spells[i])) {
				if (m_pp.mem_spells[i] == spell_id)
					return spell_cast_count[i];
			}
		}
	}

	return 0;
}

void Client::SetSpellCastCount(int slot, uint16 spell_id, int value)
{
	if (spell_id == SPELL_UNKNOWN){
		if (slot >= 0 && slot < MAX_PP_MEMSPELL)
			spell_cast_count[slot] = value;
	}

	else if (IsValidSpell(spell_id)){

		for(unsigned int i =0 ; i < MAX_PP_MEMSPELL; ++i) {
			if(IsValidSpell(m_pp.mem_spells[i])) {
				if (m_pp.mem_spells[i] == spell_id)
					spell_cast_count[i] = value;
			}
		}
	}
}

uint16 Client::GetDiscCastCount(int slot, uint16 spell_id)
{
	if (spell_id == SPELL_UNKNOWN){
		if (slot >= 0 && slot < MAX_DISCIPLINE_TIMERS + 25)
		return disc_cast_count[slot];
	}

	else if (IsValidSpell(spell_id)){

		for(unsigned int i =0 ; i < MAX_PP_DISCIPLINES; ++i) {
			if(IsValidSpell(m_pp.disciplines.values[i])) {
				if (m_pp.disciplines.values[i] == spell_id)
					return disc_cast_count[spells[spell_id].EndurTimerIndex];
			}
		}
	}
	return 0;
}

void Client::SetDiscCastCount(int slot, uint16 spell_id, int value)
{
	if (spell_id == SPELL_UNKNOWN){
		if (slot >= 0 && slot < MAX_DISCIPLINE_TIMERS + 25)
			disc_cast_count[slot] = value;
	}

	else if (IsValidSpell(spell_id)){

		for(unsigned int i =0 ; i < MAX_PP_DISCIPLINES; ++i) {
			if(IsValidSpell(m_pp.disciplines.values[i])) {
				if (m_pp.disciplines.values[i] == spell_id){
					disc_cast_count[spells[spell_id].EndurTimerIndex] = value;
					return;
				}
			}
		}
	}
}

uint32 Client::TryCastonDiscCastCountAmt(int slot, uint16 spell_id, uint32 reduced_recast)
{

	//This is the general cast count incrementer for all discs
	if (GetDiscCastCount(slot) < 65535)
		SetDiscCastCount(slot, SPELL_UNKNOWN, (CastToClient()->GetDiscCastCount(slot) + 1));

	bool reset_count = false;
	uint16 cast_count = 0;
	int _reduced_recast = reduced_recast;

	cast_count = GetDiscCastCount(slot);

	for(int i = 0; i < EFFECT_COUNT; i++)
	{
		if (spells[spell_id].effectid[i] == SE_CastOnSpellCastCountAmt)
		{
			if (cast_count == (spells[spell_id].max[i])){
				if (IsValidSpell(spells[spell_id].base2[i])){
					if(zone->random.Int(0, 100) <= spells[spell_id].base[i]) {
						SpellFinished(spells[spell_id].base2[i], this, 10, 0, -1, spells[spells[spell_id].base2[i]].ResistDiff);
						reset_count = true;
					}
				}
			}
		}

		//Adjust DISC recast timer WHEN casted (MAX) amount of times, else use default timer.  [B: Chance L: Modifier M: Count Amt]
		if (spells[spell_id].effectid[i] == SE_DiscAdjustRecastonCountAmt)
		{
			//Shout("DEBUG::TryCastonDiscCastCountAmt [%i] Cast Count %i B: %i L: %i M: %i", reduced_recast, cast_count, spells[spell_id].base[i],spells[spell_id].base2[i],spells[spell_id].max[i]);
			if (cast_count == (spells[spell_id].max[i])){
				if (spells[spell_id].base2[i]){ //Modifer to recast timer.
					if(zone->random.Int(0, 100) <= spells[spell_id].base[i]) {
						_reduced_recast += (_reduced_recast * spells[spell_id].base2[i])/100;
						reset_count = true;
					}
				}
			}
		}

		//Adjust DISC recast timer per use until casted (MAX) amount of times then use default timers.  [B: Chance L: Modifier M: Count Amt]
		if (spells[spell_id].effectid[i] == SE_DiscAdjustRecastTillCountAmt)
		{
			//Shout("DEBUG:: TryCastonDiscCastCountAmt [%i] Cast Count %i B: %i L: %i M: %i", reduced_recast, cast_count, spells[spell_id].base[i],spells[spell_id].base2[i],spells[spell_id].max[i]);
			if (cast_count != spells[spell_id].max[i]){
				if (spells[spell_id].base2[i]){ //Modifer to recast timer.
					if(zone->random.Int(0, 100) <= spells[spell_id].base[i]) {
						_reduced_recast += (_reduced_recast * spells[spell_id].base2[i])/100;

						if (!spells[spell_id].max[i])
							Message(15, "You catch a second wind!");
					}
				}
			}
			else if (spells[spell_id].max[i] && cast_count == spells[spell_id].max[i])
				reset_count = true;
		}
	}

	if (reset_count)
		SetDiscCastCount(slot, SPELL_UNKNOWN, 0);

	if (_reduced_recast < 0)
		_reduced_recast = 0;

	return static_cast<uint32>(_reduced_recast);
}

bool Client::DiscSpamLimiter(uint16 spell_id)
{
	if (!IsValidSpell(spell_id) && (spells[spell_id].EndurTimerIndex <= MAX_DISCIPLINE_TIMERS))
		return false;

	for(int i = 0; i < EFFECT_COUNT; i++)
	{
		if (spells[spell_id].effectid[i] == SE_DiscSpamLimiter){
			SetDiscSpamLimiter(spells[spell_id].EndurTimerIndex,GetDiscSpamLimiter(spells[spell_id].EndurTimerIndex) + spells[spell_id].base[i]);
			if (GetDiscSpamLimiter(spells[spell_id].EndurTimerIndex) <= spells[spell_id].base2[i]){
				return true;
			}else {
				Message(11, "You look exhausted!");
				return false;
			}
		}
	}
	return false;
}

//C!SpellEffects :: SE_TryCastonSpellFinished

void Mob::TryCastonSpellFinished(Mob *target, uint16 spell_id)
{
	if(!IsClient() || target == nullptr || !IsValidSpell(spell_id))
		return;
	
	for(int i = 0; i < EFFECT_COUNT; i++)
	{
		if (spells[spell_id].effectid[i] == SE_TryCastonSpellFinished)
		{
			if(zone->random.Int(1, 100) <= spells[spell_id].base[i])
			{
				if(target)
					SpellFinished(spells[spell_id].base2[i], target, 10, 0, -1, spells[spells[spell_id].base[i]].ResistDiff);
			}
		}
	}
}

//#### C!SpellEffects :: SE_SpellPowerFromBuffSpellGroup

int32 Mob::CalcSpellPowerFromBuffSpellGroup(uint16 spell_id, Mob* caster)
{
	if (!IsValidSpell(spell_id) || !caster)
		return 0;

	int mod = 0;
	int amt_effects = 0;
	int amt_effects_found = 0;

	for(int i = 0; i < EFFECT_COUNT; i++){

		int spellgroup_id = 0;
		int spid = SPELL_UNKNOWN;

		if (spells[spell_id].effectid[i] == SE_SpellPowerFromBuffSpellGroup){
			amt_effects++;
			spellgroup_id = spells[spell_id].base[i]; //Buff Spell Groupid

			if (spellgroup_id) {
				spid = GetBuffSpellidBySpellGroup(spellgroup_id);

				if (IsValidSpell(spid)) {
					mod += spells[spell_id].base2[i] * spells[spid].rank;
					amt_effects_found++;
					//Shout("DEBUG::CalcSpellPowerFromBuffSpellGroup :: Spellid [%i] MOD %i",spid, mod);
				}
			}
		}
	}
	//Shout("DEBUG::CalcSpellPowerFromBuffSpellGroupFinal :: MOD %i Effecst Found [%i / %i]", mod , amt_effects_found,  amt_effects);

	if (amt_effects_found < amt_effects)
		return 0;

	return mod;
}

//#### C!SpellEffects :: SE_SpellPowerAmtHits

int32 Mob::GetSpellPowerAmtHitsEffect(uint16 spell_id)
{
	if(!IsValidSpell(spell_id))
		return 0;

	for(int i = 0; i < EFFECT_COUNT; i++)
	{
		if (spells[spell_id].effectid[i] == SE_SpellPowerAmtHits)
			return spells[spell_id].base[i];
	}

	return 0;
}

//#### C!SpellEffects :: SE_SpellPowerHeightMod

int32 Mob::CalcSpellPowerHeightMod(uint16 spell_id, Mob* caster){

	if (!IsValidSpell(spell_id) || !caster)
		return 0;

	for (int i = 0; i <= EFFECT_COUNT; i++)
	{
		if (spells[spell_id].effectid[i] == SE_SpellPowerHeightMod){

			if (spells[spell_id].base[i]){

				int distance = caster->GetCastingZDiff();
				int max_distance = spells[spell_id].base[i];
				int max_modifier = spells[spell_id].base2[i]*100;

				if (distance > max_distance)
					distance = max_distance;

				caster->SetCastingZDiff(0);
				return ((distance * ((max_modifier*100)/max_distance))/100);
			}
		}
	}
	return 0;
}

void Mob::CalcSpellPowerHeightZDiff(uint16 spell_id, Mob* spell_target)
{
	if (spell_target && IsEffectInSpell(spell_id, SE_SpellPowerHeightMod)){
		float origin_z = GetZ();
		
		//Hack to get accurate Z measurements during a controlled gravity flux of power 500
		if (gflux.increment){
			//mod = UP Max Height/Interval at Max Height (130/85)
			//mod = DOWN 130/145
			int mod_up = (130 * 100)/gflux.time_to_peak;
			int mod_down = (130 * 100)/gflux.time_to_ground;

			if (!gflux.peak)
				origin_z = gflux.origin_z + ((gflux.increment * mod_up) / 100);
			else
				origin_z = gflux.origin_z + (((145 - (gflux.increment - gflux.peak)) * mod_down) / 100);
		}
		SetCastingZDiff((static_cast<int>(origin_z) - static_cast<int>(spell_target->GetZ())));
	}
}

void Mob::SetGFluxEffectVars(uint16 spell_id, int base)
{

	//[Note: 250 = 67 units with peak time of 85 and total time of 170] - Conclusion time to hit max is same regardless of force.
	int z_max = GetDistanceToCeiling(130, GetZ()); //Distance for effect value 500 is 130 units 

	if (z_max > 130)
		z_max = 130;
	
	gflux.increment = 1;
	gflux.time_to_peak = (85 * (z_max*100/130))/100;
	gflux.time_to_ground = (230 * (z_max*100/130))/100; //85 + 145 from peak
	gflux.origin_z = static_cast<int>(GetZ());
}

int Mob::GetDistanceToCeiling(int max_z, float origin_z)
{
	if (!max_z)
		max_z = 130;

	int interval = 0;
	float origin_x = GetX();
	float origin_y = GetY();
	float origin_Z = origin_z;
	while (interval <= max_z)
	{
		if (!CheckLosFN(origin_x,origin_y,origin_Z + (static_cast<float>(interval)),GetSize()))
			return interval;

		interval += 5;
	}

	return interval;
}

void Mob::GravityFlux()
{
	if (gflux.increment){

		//int time_to_ground = 170; //(170/-250 power Zchange 67) 0.68 mod (240/500 Zchange 130)

		//FOR POWER 500 - NARROWLY CODED

		gflux.increment++;

		if (gflux.increment ==  gflux.time_to_peak){
			//std::string health_update_notification = StringFormat("Health: %u%%", 999); //DEBUG JUNK
			//this->CastToClient()->SendMarqueeMessage(15, 510, 0, 3000, 3000, health_update_notification); //DEBUG JUNK
			gflux.peak = gflux.increment;
		}

		else if (gflux.increment > gflux.time_to_ground){
			gflux.increment = 0;
			gflux.origin_z = 0;
			gflux.peak = 0;
			gflux.time_to_peak = 0;
			gflux.time_to_ground = 0;
		}
	}
}

//#### C!SpellEffects :: Appearance Effects

void Mob::SendAppearanceEffect2(uint32 parm1, uint32 parm2, uint32 parm3, uint32 parm4, uint32 parm5, Client *specific_target){

	//Set apperance effect on NPC that will fade when the NPC dies. (Use original function if want perma effects)

	EQApplicationPacket* outapp = new EQApplicationPacket(OP_LevelAppearance, sizeof(LevelAppearance_Struct));
	LevelAppearance_Struct* la = (LevelAppearance_Struct*)outapp->pBuffer;
	la->spawn_id = GetID();
	la->parm1 = parm1;
	la->parm2 = parm2;
	la->parm3 = parm3;
	la->parm4 = parm4;
	la->parm5 = parm5;
	// Note that setting the b values to 0 will disable the related effect from the corresponding parameter.
	// Setting the a value appears to have no affect at all.s
	la->value1a = 2;
	la->value1b = 0;
	la->value2a = 2;
	la->value2b = 0;
	la->value3a = 2;
	la->value3b = 0;
	la->value4a = 2;
	la->value4b = 0;
	la->value5a = 2;
	la->value5b = 0;
	if(specific_target == nullptr) {
		entity_list.QueueClients(this,outapp);
	}
	else if (specific_target->IsClient()) {
		specific_target->CastToClient()->QueuePacket(outapp, false);
	}
	safe_delete(outapp);
}

bool Mob::HasAppearanceEffects(int slot)
{
	uint32 buff_max = GetMaxTotalSlots();
		
	for(uint32 d = 0; d < buff_max; d++) {
		if(slot != d && IsValidSpell(buffs[d].spellid) && spells[buffs[d].spellid].AppEffect)
			return true;
	}

	SetAppearanceEffect(false);
	return false;
}

void EntityList::SendAppearanceEffects(Client *c)
{
	if (!c)
		return;

	auto it = mob_list.begin();
	while (it != mob_list.end()) {
		Mob *cur = it->second;

		if (cur) {
			if (cur == c) {
				++it;
				continue;
			}
			if (cur->GetAppearanceEffect()) 
				cur->SendAllAppearanceEffects(c);	
		}
		++it;
	}
}

void Mob::SendAllAppearanceEffects(Client* c)
{
	if (!c)
		return;

	uint32 buff_max = GetMaxTotalSlots();
	for(uint32 d = 0; d < buff_max; d++) {
				
		if(IsValidSpell(buffs[d].spellid) && spells[buffs[d].spellid].AppEffect){
			SendAppearanceEffect2(spells[buffs[d].spellid].AppEffect, 0, 0, 0, 0, c);			
		}
	}
}

//#### C!SpellEffects :: SE_EffectField

void Mob::DoEffectField()
{
	/* Effect Field is an AOE at a location in which anything that enters the area gets the buff, this
	buff then immediately fades upon exiting the area. The bonus is placed from spell effect 1035 which
	has its base value the spell_id of the buff/debuff which is applied to the mobs who enter the effect field.
	This bonus therefore is placed on an NPC or Swarmpet at the location center of the field. When the owner of this
	bonus dies/depops the buff/debuff fades on all mobs.
	RANGE of field is determined by the applied buff/debuffs RANGE field in spells_new
	*/
	if (spellbonuses.EffectField) {
		Mob* caster = this;
		
		if (IsNPC() && CastToNPC()->GetSwarmOwner())
			caster = entity_list.GetClientByID(CastToNPC()->GetSwarmOwner());
		
		if (caster)
			entity_list.ApplyEffectField(caster, this, spellbonuses.EffectField, false);
	}
	else
		effect_field_timer.Disable();
}


void EntityList::ApplyEffectField(Mob *caster, Mob *center, uint16 spell_id, bool affect_caster)
{ 
	if (!IsValidSpell(spell_id) || !center || !caster)
		return;
	
	Mob *curmob;
	float dist = spells[spell_id].range;
	float dist2 = dist * dist;
	float dist_targ = 0;
	bool bad = IsDetrimentalSpell(spell_id);

	for (auto it = mob_list.begin(); it != mob_list.end(); ++it) {
		curmob = it->second;
		if (!curmob)
			continue;
		if (curmob->IsClient() && !curmob->CastToClient()->ClientFinishedLoading())
			continue;
		if (curmob == center)
			continue;
		if (curmob == caster && !affect_caster)
			continue;
		
		dist_targ = DistanceSquared(curmob->GetPosition(), center->GetPosition());

		if (dist_targ > dist2){
			if (curmob->FindBuff(spell_id))
				curmob->BuffFadeBySpellID(spell_id);
			
			continue;
		}

		if (bad){
			if (!caster->IsAttackAllowed(curmob, true))
				continue;
			if (center && !center->CheckLosFN(curmob))
				continue;
		}
		else {
			if (caster->IsAttackAllowed(curmob, true))
				continue;
			if (caster->CheckAggro(curmob))
				continue;
		}

		if (curmob && !curmob->FindBuff(spell_id) && !curmob->IsImmuneToSpellEffectField(spell_id))
			caster->SpellOnTarget(spell_id, curmob, false, true, 0); 
	}
}

bool Mob::IsImmuneToSpellEffectField(uint16 spell_id)
{
	//Scaled down version of immune check to prevent spam, since these effects generally unresistable unless immmune.
	if (GetSpecialAbility(IMMUNE_MAGIC))
		return true;

	//Do not check effects if these immunities do not exist.
	if (!GetSpecialAbility(UNSLOWABLE) && !GetSpecialAbility(UNSNAREABLE) && !GetSpecialAbility(UNFEARABLE) && !GetSpecialAbility(UNMEZABLE))
		return false;

	for (int j = 0; j < EFFECT_COUNT; j++){

		int effect = spells[spell_id].effectid[j];

		if (effect == SE_AttackSpeed && GetSpecialAbility(UNSLOWABLE))
			return true;

		if ((effect == SE_Root || effect == SE_MovementSpeed) && GetSpecialAbility(UNSNAREABLE))
			return true;

		if (effect == SE_Fear && GetSpecialAbility(UNFEARABLE))
			return true;

		if (effect == SE_Mez && GetSpecialAbility(UNMEZABLE))
			return true;
	}

	return false;
}

void EntityList::FadeCastersBuffFromAll(uint16 caster_id, uint16 spell_id)
{ 
	//Removes from all npcs the effect field buff when the effect field base spell fades.
	if (!IsValidSpell(spell_id) || !caster_id)
		return;

	Mob *curmob;
	for (auto it = mob_list.begin(); it != mob_list.end(); ++it) {
		curmob = it->second;

		if (curmob && curmob->FindBuff(spell_id))
			curmob->BuffFadeBySpellIDCaster(spell_id, caster_id);
	}
}

void Mob::DoAuraField()
{
	if (spellbonuses.AuraField) {
		entity_list.ApplyAuraField(this, this, spellbonuses.AuraField);
	}
	else
		aura_field_timer.Disable();
}

void EntityList::ApplyAuraField(Mob *caster, Mob *center, uint16 spell_id)
{ 
	if (!IsValidSpell(spell_id) || !center || !caster)
		return;

	Mob *curmob;
	float dist = spells[spell_id].range;
	float dist2 = dist * dist;
	float dist_targ = 0;

	for (auto it = mob_list.begin(); it != mob_list.end(); ++it) {
		curmob = it->second;
		if (!curmob)
			continue;
		if (curmob->IsClient() && !curmob->CastToClient()->ClientFinishedLoading())
			continue;
		if (curmob->IsNPC() && !curmob->IsPetOwnerClient())
			continue;

		dist_targ = DistanceSquared(curmob->GetPosition(), center->GetPosition());

		if (dist_targ > dist2){	//make sure they are in range
			if (curmob->FindBuff(spell_id))
				curmob->BuffFadeBySpellID(spell_id);
			
			continue;
		}

		if (curmob && !curmob->FindBuff(spell_id))
			caster->SpellOnTarget(spell_id, curmob, false, true, 0);
	}
}

//#### C!SpellEffects :: SE_MeleeManaTap / SE_MeleeLifeTapPetOwner / SE_MeleeManaTapPetOwner	

void Mob::MeleeManaTap(int32 damage)
{
	int16 manatap_bonus = spellbonuses.MeleeManaTap + itembonuses.MeleeManaTap + aabonuses.MeleeManaTap;

	if(manatap_bonus && damage > 0)
		SetMana(GetMana() + (damage * manatap_bonus / 100));
}

void Mob::MeleeEndurTap(int32 damage)
{
	if (!IsClient())
		return;

	int16 endur_bonus = spellbonuses.MeleeEndurTap + itembonuses.MeleeEndurTap + aabonuses.MeleeEndurTap;

	if (GetClass() == WARRIOR)
		endur_bonus += 100; //Gain 10% of melee damage back as endurance.

	if(endur_bonus && damage > 0)
		CastToClient()->SetEndurance(CastToClient()->GetEndurance() + (damage * endur_bonus / 100));
}

void Mob::PetTapToOwner(int32 damage)
{
	//Melee damage done by pet is converted to heal/mana gain on owner.
	if ((IsPet() || IsTempPet()) && damage > 0) {

		Mob* owner = GetOwner();

		if (!owner)
			return;

		int16 lifetap_amt = 0;
		lifetap_amt = spellbonuses.MeleeLifeTapPetOwner + itembonuses.MeleeLifeTapPetOwner + aabonuses.MeleeLifeTapPetOwner;

		if(lifetap_amt){

			lifetap_amt = damage * lifetap_amt / 100;

			if (lifetap_amt > 0)
				owner->HealDamage(lifetap_amt); //Heal self for modified damage amount.
			else
				owner->Damage(owner, -lifetap_amt,0, SkillEvocation,false); //Dmg self for modified damage amount.
		}

		int16 manatap_bonus = spellbonuses.MeleeManaTapPetOwner + itembonuses.MeleeManaTapPetOwner + aabonuses.MeleeManaTapPetOwner;
		if(manatap_bonus)
			owner->SetMana(owner->GetMana() + (damage * manatap_bonus / 100));
	}
}

//#### C!SpellEffects :: SE_CastOnLeap / SE_TossUp

void Mob::SetLeapEffect(uint16 spell_id){
	//Use in SE_TossUp (84) to set timer on client MAX value must be set to (1).
	if(IsClient())
	{
		BuffFadeByEffect(SE_Levitate);
		leap.spell_id = spell_id;
		leap.origin_x = GetX();
		leap.origin_y = GetY();
		leap.origin_z = GetZ();
		leap.increment = 1;

		//Break deterimental snares and roots.
		BreakMovementDebuffs();
	}
}

void Mob::LeapProjectileEffect()
{
	//Due to my inability to calculate the predicted distance - Hack job for this ability.
	//Using 8 pushback and 30 pushup results in a distance ~ 56 which takes about 75 increments
	//Trigger is set on TryOnClientUpdate when increment = 75

	if (!leap.increment)
		return;

	leap.increment++;
	return;
}

//#### C!SpellEffects :: SE_PetLifeShare

void Mob::PetLifeShare(SkillUseTypes skill_used, int32 &damage, Mob* attacker)
{
	if (!attacker || damage <= 0) 
		return;
	//Base Penalty value is 100, meaning receives no change from base damage.

	if (spellbonuses.PetLifeShare[1]){

		int slot = -1;
		int32 damage_to_reduce = 0;
		int32 base_damage = damage;

		if (!GetPet()){
			BuffFadeByEffect(SE_PetLifeShare); //Fade if no pet and has buff.
			return;
		}

		slot = spellbonuses.PetLifeShare[0];
		if(slot >= 0)
		{
			damage_to_reduce = damage * spellbonuses.PetLifeShare[1] / 100;

			if(spellbonuses.PetLifeShare[3] && (damage_to_reduce >= static_cast<int32>(buffs[slot].melee_rune))){
				damage -= buffs[slot].melee_rune;
				if(!TryFadeEffect(slot))
					BuffFadeBySlot(slot);
			}
			else{
				if (spellbonuses.PetLifeShare[3])
					buffs[slot].melee_rune = (buffs[slot].melee_rune - damage_to_reduce);
				
				damage -= damage_to_reduce;
			}
		
		
			int32 damage_to_pet = base_damage - damage_to_reduce;

			if (spellbonuses.PetLifeShare[2]){
				if (GetPet()){
					damage_to_pet = (damage_to_pet*spellbonuses.PetLifeShare[2])/100;
					GetPet()->Damage(attacker, damage, SPELL_UNKNOWN, skill_used, false);
				}
			}
		}
	}
}

//#### C!SpellEffects :: SE_ApplyEffectOrder

void Mob::TryApplyEffectOrder(Mob* target, uint16 spell_id)
{
	//Spell Effect puts - Spell ID of buffs to be cast in a specific order, if not found it will cast it and remove the prior.
	if(!target || !IsValidSpell(spell_id))
		return;

	bool effect_found = false;
	uint16 current_buff = SPELL_UNKNOWN;

	for(int i = 0; i < EFFECT_COUNT; i++)
	{
		if (spells[spell_id].effectid[i] == SE_ApplyEffectOrder)
		{
			effect_found = true;
			if (IsValidSpell(spells[spell_id].base[i]))
			{
				if (target->FindBuff(spells[spell_id].base[i]))
					current_buff = spells[spell_id].base[i];
			}
		}
	}

	if (!effect_found){
		return; //If no effect found just exit function
	}

	for(int i = 0; i < EFFECT_COUNT; i++)
	{
		if (spells[spell_id].effectid[i] == SE_ApplyEffectOrder)
		{
			if (IsValidSpell(spells[spell_id].base[i]))
			{
				if (current_buff == SPELL_UNKNOWN) {
					//Shout("DEBUG::TryApplyEffectOrder :1 Cast %i", spells[spell_id].base[i]);
					SpellFinished(spells[spell_id].base[i], target, 10, 0, -1, spells[spells[spell_id].base[i]].ResistDiff);
					return;
				}

				else if (current_buff == spells[spell_id].base[i]){

					if (IsValidSpell(spells[spell_id].base[i + 1])){
						target->BuffFadeBySpellID(spells[spell_id].base[i]);
						//Shout("DEBUG::TryApplyEffectOrder :2 Cast %i", spells[spell_id].base[i+1]);
						SpellFinished(spells[spell_id].base[i + 1], target, 10, 0, -1, spells[spells[spell_id].base[i+1]].ResistDiff);
						return;
					}
					else {
						target->BuffFadeBySpellID(spells[spell_id].base[i-1]);
						//Shout("DEBUG::TryApplyEffectOrder :3 Cast %i", spells[spell_id].base[i]);
						SpellFinished(spells[spell_id].base[i], target, 10, 0, -1, spells[spells[spell_id].base[i]].ResistDiff);
						return;
					}
				}
			}
		}
	}

	//Shout("DEBUG::TryApplyEffectOrder END Fail [%i]",spell_id);
}

//#### C!SpellEffects :: SE_CastOnCurerFromCure / SE_CastOnCureFromCure

void Mob::CuredEffect()
{
	SetCuredCount((GetCuredCount() + 2));
}

void Mob::CastOnCurerFromCure(uint16 spell_id)
{  
	//When 'CastonCurer' is placed on the Cure spell, 'this' is CURER
	uint16 trigger_spell = SPELL_UNKNOWN;
	for(int i = 0; i < EFFECT_COUNT; i++){
		if (spells[spell_id].effectid[i] == SE_CastOnCurerFromCure){
			if (spells[spell_id].base2[i] <= GetCuredCount())
				trigger_spell = spells[spell_id].base[i];
		}
	}

	if (IsValidSpell(trigger_spell))
		SpellFinished(trigger_spell, this, 10, 0, -1, spells[spell_id].ResistDiff);
}

void Mob::CastOnCureFromCure(uint16 spell_id)
{  
	//When 'CastonCure' is placed on the Cure spell, 'this' is target being cured
	uint16 trigger_spell = SPELL_UNKNOWN;
	for(int i = 0; i < EFFECT_COUNT; i++){
		if (spells[spell_id].effectid[i] == SE_CastOnCureFromCure){
			if (spells[spell_id].base2[i] <= GetCuredCount())
				trigger_spell = spells[spell_id].base[i];
		}
	}

	if (IsValidSpell(trigger_spell))
		SpellFinished(trigger_spell, this, 10, 0, -1, spells[spell_id].ResistDiff);
}

//#### C!StunResilience

bool Mob::CalcStunResilience(int effect_value, Mob* caster)
{
	if (!IsNPC() || !GetMaxStunResilience())
		return false;

	if (IsStunned() && caster){
		caster->Message(MT_SpellFailure, "Your targets stun resilience is completely depleted.");
		return false;
	}
	/* OLD when = 100
	//Regen ~100 pt in every 18 seconds - All values are in intervals of 100 for STA and Neg SR
	int SR = int(GetStunResilience()/100) * 100;
	if (SR > 0){
		
		int new_value = SR + effect_value;

		if (new_value <= 99){
			new_value = 0;
			entity_list.MessageClose(this, false, 200, MT_Stun, "%s stun resilience falters!  (%i / %i)", GetCleanName(), new_value,GetMaxStunResilience());
		}
		else
			entity_list.MessageClose(this, false, 200, MT_Stun, "%s stun resilience is diminished! (%i / %i)", GetCleanName(),new_value,GetMaxStunResilience());

		SetStunResilience(new_value);
		return true;
	}
	*/

	if (GetStunResilience() > 0){
		
		int new_sr = GetStunResilience() + effect_value; //Effect value is typically a negative value thus lowering it.

		if (new_sr <= 0){
			new_sr = 0;
			entity_list.MessageClose(this, false, 200, MT_Stun, "%s stun resilience falters!  (%i / %i)", GetCleanName(), new_sr,GetMaxStunResilience());
		}
		else
			entity_list.MessageClose(this, false, 200, MT_Stun, "%s stun resilience is diminished! (%i / %i)", GetCleanName(),new_sr,GetMaxStunResilience());

		if (!stun_resilience_timer.Check())
			stun_resilience_timer.Start(12000);

		SetStunResilience(new_sr);
		return true;
	}

	return false;
}

void Mob::OpportunityFromStunCheck()
{
	if (IsCasting() && IsNPC()){
		SetOpportunityMitigation(50);
		entity_list.MessageClose(this, false, 200, MT_Stun, "%s collapses to the ground completely exhausted!", GetCleanName());
		DisableTargetSpellAnim(true);
		SetAppearance(eaDead);
	}
}

void Mob::OpportunityFromStunClear()
{
	if (GetOpportunityMitigation()) {
		DisableTargetSpellAnim(false);
		SetOpportunityMitigation(0);
		SetAppearance(eaStanding);
	}
}

void Mob::StunResilienceRegen()
{
	if (GetStunResilience() < GetMaxStunResilience()){
		SetStunResilience(GetStunResilience()+1);
	}
	//If stun resilience reaches max, disable the regeration timer.
	else {
		SetStunResilience(GetMaxStunResilience());
		stun_resilience_timer.Disable();
	}

}

//#### C!BUFFS - Buff Related Functions

int Mob::GetBuffSlotFromSpellID(uint16 spell_id)
{
	int i;
	int buff_count = GetMaxTotalSlots();
	for(i = 0; i < buff_count; i++)
		if(buffs[i].spellid == spell_id)
			return i;

	return -1;
}

void Mob::BuffFadeBySpellIDCaster(uint16 spell_id, uint16 caster_id)
{
	int buff_count = GetMaxTotalSlots();
	for (int j = 0; j < buff_count; j++)
	{
		if (buffs[j].spellid == spell_id && buffs[j].casterid == caster_id)
			BuffFadeBySlot(j, false);
	}

	CalcBonuses();
}

uint16 Mob::GetBuffSpellidBySpellGroup(int spellgroupid)
{
	int i;

	int buff_count = GetMaxTotalSlots();
	for(i = 0; i < buff_count; i++)
		if(IsValidSpell(buffs[i].spellid)){
			if (spells[buffs[i].spellid].spellgroup == spellgroupid)
				return buffs[i].spellid;
		}
	return 0;
}

//C! - New uncategorized functions

void Mob::DirectionalFailMessage(uint16 spell_id)
{
	//Message given for failed directional abilities, message will differ based on type of spell.
	//Message(MT_SpellFailure, "Your spell failed to find a target.");
	
	//if (spells[spell_id].IsDisciplineBuff)
	//	Message(MT_SpellFailure, "No target found for this ability.");
	//else
	//	Message(MT_SpellFailure, "No target found for this spell.");

	DoAnim(spells[spell_id].CastingAnim);

	if (spells[spell_id].IsDisciplineBuff)
		Message(MT_SpellFailure, "Your ability failed to find a target in range.");
	else
		Message(MT_SpellFailure, "Your spell failed to find a target in range.");

}

void Mob::ProjectileTargetRingFailMessage(uint16 spell_id)
{
	if (spells[spell_id].spellgroup == SPELL_GROUP_SPECTRAL_BLADE)
		Message(MT_SpellFailure, "Your spectral blade missed!");
	else if (spells[spell_id].spellgroup == SPELL_GROUP_SPECTRAL_BLADE_FLURRY)
		Message(MT_SpellFailure, "Your spectral blades missed!");

	if (GetClass() == CLERIC)
		Message(MT_SpellFailure, "The holy flame dissipates.");

	else
		Message(MT_SpellFailure, "Your spell failed to find a target.");

}

void Mob::Debug(const char *str)
{
	entity_list.MessageStatus(0, 100, 326, "%s", str);
}

bool Mob::CustomResistSpell(uint16 spell_id, Mob *caster)
{
	/*
	Valid spell checked before running in ResistSpell
	This function will be used for custom resist checks as needed
	*/
	if (IsEffectInSpell(spell_id, SE_SpellPowerFromBuffSpellGroup) && caster){
		if (!CalcSpellPowerFromBuffSpellGroup(spell_id, caster))//Resist if all effects are not present
			return true;
	}
	
	return false;

}

void Mob::DoSpecialFastBuffTick()
{
	//Primary used for rapid regeneration abilities that require more percision. !ONLY MANA IMPLEMENTED

	if (spellbonuses.FastManaRegen[0] || spellbonuses.FastManaRegen[1])
	{
		//Coded very narrowly to only allow one effect up (If necessary can change from bonus to iterating buffs) LIMITED
		SetMana(GetMana() + spellbonuses.FastManaRegen[0] + (GetMaxMana() * spellbonuses.FastManaRegen[1] / 100));
		fast_tic_special_count += 1;

		if (fast_tic_special_count >= spellbonuses.FastManaRegen[2]){
			BuffFadeBySpellID(spellbonuses.FastManaRegen[3]);
			fast_tic_special_count = 0;
		}
	}
	else
		fast_tic_special_timer.Disable();
}

void Client::RelequishFlesh(uint16 spell_id, Mob *target, const char *name_override, int pet_count, int pet_duration, int aehate)
{
	if(!IsValidSpell(spell_id))
		return;

	PetRecord record;
	if(!database.GetPetEntry(spells[spell_id].teleport_zone, &record))
	{
		Message(13, "Unable to find data for pet %s", spells[spell_id].teleport_zone);
		return;
	}

	AA_SwarmPet pet;
	pet.count = pet_count;
	pet.duration = pet_duration;
	pet.npc_id = record.npc_type;

	NPCType *made_npc = nullptr;

	const NPCType *npc_type = database.LoadNPCTypesData(pet.npc_id);
	if(npc_type == nullptr) {
		Message(0,"Unable to find pet!");
		return;
	}
	// make a custom NPC type for this
	made_npc = new NPCType;
	memcpy(made_npc, npc_type, sizeof(NPCType));

	strcpy(made_npc->name, name_override);
	made_npc->level = GetLevel();
	made_npc->race = GetBaseRace();
	made_npc->gender = GetBaseGender();
	made_npc->size = GetBaseSize();
	made_npc->AC = GetAC();
	made_npc->STR = GetSTR();
	made_npc->STA = GetSTA();
	made_npc->DEX = GetDEX();
	made_npc->AGI = GetAGI();
	made_npc->MR = GetMR();
	made_npc->FR = GetFR();
	made_npc->CR = GetCR();
	made_npc->DR = GetDR();
	made_npc->PR = GetPR();
	made_npc->Corrup = GetCorrup();
	made_npc->max_hp = GetMaxHP();
	// looks
	made_npc->texture = GetEquipmentMaterial(MaterialChest);
	made_npc->helmtexture = GetEquipmentMaterial(MaterialHead);
	made_npc->haircolor = GetHairColor();
	made_npc->beardcolor = GetBeardColor();
	made_npc->eyecolor1 = GetEyeColor1();
	made_npc->eyecolor2 = GetEyeColor2();
	made_npc->hairstyle = GetHairStyle();
	made_npc->luclinface = GetLuclinFace();
	made_npc->beard = GetBeard();
	made_npc->drakkin_heritage = GetDrakkinHeritage();
	made_npc->drakkin_tattoo = GetDrakkinTattoo();
	made_npc->drakkin_details = GetDrakkinDetails();
	made_npc->d_melee_texture1 = 0;
	made_npc->d_melee_texture2 = 0;
	for (int i = EmuConstants::MATERIAL_BEGIN; i <= EmuConstants::MATERIAL_END; i++)	{
		made_npc->armor_tint[i] = GetEquipmentColor(i);
	}
	made_npc->loottable_id = 0;

	npc_type = made_npc;

	int summon_count = 0;
	summon_count = pet.count;

	if(summon_count > MAX_SWARM_PETS)
		summon_count = MAX_SWARM_PETS;

	static const glm::vec2 swarmPetLocations[MAX_SWARM_PETS] = {
		glm::vec2(5, 5), glm::vec2(-5, 5), glm::vec2(5, -5), glm::vec2(-5, -5),
		glm::vec2(10, 10), glm::vec2(-10, 10), glm::vec2(10, -10), glm::vec2(-10, -10),
		glm::vec2(8, 8), glm::vec2(-8, 8), glm::vec2(8, -8), glm::vec2(-8, -8)
	};

	while(summon_count > 0) {
		NPCType *npc_dup = nullptr;
		if(made_npc != nullptr) {
			npc_dup = new NPCType;
			memcpy(npc_dup, made_npc, sizeof(NPCType));
		}

		NPC* npca = new NPC(
				(npc_dup!=nullptr)?npc_dup:npc_type,	//make sure we give the NPC the correct data pointer
				0,
				GetPosition() + glm::vec4(swarmPetLocations[summon_count], 0.0f, 0.0f),
				FlyMode3);

		if(!npca->GetSwarmInfo()){
			AA_SwarmPetInfo* nSI = new AA_SwarmPetInfo;
			npca->SetSwarmInfo(nSI);
			npca->GetSwarmInfo()->duration = new Timer(pet_duration*1000);
		}
		else{
			npca->GetSwarmInfo()->duration->Start(pet_duration*1000);
		}

		npca->GetSwarmInfo()->owner_id = GetID();

		//we allocated a new NPC type object, give the NPC ownership of that memory
		if(npc_dup != nullptr)
			npca->GiveNPCTypeData(npc_dup);

		entity_list.AddNPC(npca);

		//Relquish Flesh Effect
		if (aehate) 
		{
			entity_list.AddClientHateToTempPet(this, npca, spell_id);
		}


		summon_count--;
	}
}

void EntityList::AddClientHateToTempPet(Mob *caster, Mob* temppet, uint16 spell_id)
{ 
	if (!IsValidSpell(spell_id) || !caster || !temppet)
		return;

	Mob *curnpc;

	for (auto it = npc_list.begin(); it != npc_list.end(); ++it) {
		curnpc = it->second;
		if (curnpc && curnpc->CheckAggro(caster)){
			int32 hateamount = curnpc->GetHateAmount(caster);
			curnpc->AddToHateList(temppet, hateamount + 100);
		}
	}
}

bool Mob::MeleeDiscCombatRange(uint32 target_id, uint16 spell_id)
{
	if (!IsMeleeRangeSpellEffect(spell_id))
		return true;

	Mob* target = nullptr;
	target = entity_list.GetMob(target_id);

	if (!target)
		return false;

	if (GetDiscHPRestriction(spell_id) > 0){
		if (static_cast<int>(target->GetHPRatio()) > GetDiscHPRestriction(spell_id)){
			Message(13, "Your target must be weakened under %i percent health to execute this attack!", GetDiscHPRestriction(spell_id));
			return false;
		}
	}
		
	if (!CombatRange(target)){
		Message_StringID(13, TARGET_OUT_OF_RANGE);
		return false;
	}

	if (!IsFacingMob(target)){
		Message_StringID(13, CANT_SEE_TARGET);
		return false;
	}
				
	if (GetDiscLimitToBehind(spell_id)){ //1 = Must attack from behind.
		if (!BehindMobCustom(target, GetX(), GetY())){
			Message(13, "You must be behind your target to execute this attack!");
			return false;
		}
	}

	return true;
}

bool Mob::PassDiscRestriction(uint16 spell_id)
{
	if (GetDiscHPRestriction(spell_id) < 0){
		if (static_cast<int>(GetHPRatio()) > (GetDiscHPRestriction(spell_id) * -1)){
			Message(13, "You must be weakened under %i percent health to use this ability!", (GetDiscHPRestriction(spell_id) * -1));
			return false;
		}
	}
	return true;
}



int16 Mob::GetScaleMitigationNumhits()
{
	if (!spellbonuses.ScaleMitigationNumhits[0])
		return 0;

	int slot = spellbonuses.ScaleMitigationNumhits[1];

	if (slot >= 0 && IsValidSpell(buffs[slot].spellid))
		return (static_cast<int16>(buffs[slot].numhits) * spellbonuses.ScaleMitigationNumhits[0] / 100);
	
	return 0;
}

int16 Mob::GetScaleDamageNumhits(uint16 skill)
{
	if (!spellbonuses.ScaleDamageNumhits[0])
		return 0;

	int slot = spellbonuses.ScaleDamageNumhits[1];

	if (spellbonuses.ScaleDamageNumhits[2] == skill || spellbonuses.ScaleDamageNumhits[2] == (HIGHEST_SKILL + 1)){
		if (slot >= 0 && IsValidSpell(buffs[slot].spellid))
			return (static_cast<int16>(buffs[slot].numhits) * spellbonuses.ScaleDamageNumhits[0] / 100);
	}
	return 0;
}

float Mob::GetScaleHitChanceNumhits(SkillUseTypes skillinuse)
{
	if (!spellbonuses.ScaleHitChanceNumhits[0])
		return 0;

	int slot = spellbonuses.ScaleHitChanceNumhits[1];

	if (spellbonuses.ScaleHitChanceNumhits[2] == skillinuse || spellbonuses.ScaleHitChanceNumhits[2] == (HIGHEST_SKILL + 1)){
		if (slot >= 0 && IsValidSpell(buffs[slot].spellid))
			return static_cast<float>(buffs[slot].numhits * spellbonuses.ScaleHitChanceNumhits[0] / 100);
	}
	return 0;
}

void Client::TryChargeEffect()
{
	if (!spellbonuses.ChargeEffect[0])
		return;

	int slot = spellbonuses.ChargeEffect[1];

	if (slot >= 0){
		float dist = 0.0f;
		dist = CalculateDistance(static_cast<float>(buffs[slot].caston_x), static_cast<float>(buffs[slot].caston_y), static_cast<float>(buffs[slot].caston_z));
		uint32 dist_int = static_cast<uint32>(dist);

		if (dist_int == GetChargeEffect()){
			buffs[slot].caston_x = static_cast<int32>(GetX());
			buffs[slot].caston_y = static_cast<int32>(GetY());
			buffs[slot].caston_z = static_cast<int32>(GetZ());
			charge_effect_increment = 0;
			SetChargeEffect(dist_int);
		}
		else{
			SetChargeEffect(dist_int);
			if (!charge_effect_increment)
				charge_effect_increment = 1;
		}

		//Shout("DEBUG: Distance %i Increment %i",GetChargeEffect(), charge_effect_increment);
	}
}

void Client::TryChargeHit()
{
	if (!spellbonuses.ChargeEffect[0])
		return;

	charge_effect_increment++;
	//Shout("DEBUG: [%i] Client: X %.2f Y %.2f Z %.2f",charge_effect_increment, GetX(), GetY(), GetZ());
	 
	Mob* target = nullptr;
	target = GetTarget();

	if (!target || !target->IsNPC() || target->IsPetOwnerClient())
		return;

	if (!CombatRange(target))
		return;

	if (charge_effect_increment <= 200){
		Message(MT_SpellFailure,"Your charge failed to gain enough momentum.");
		target->AddToHateList(this, 1);
		AdjustDiscTimer(19, 0);	
	}

	else{

		int focus = charge_effect_increment/10;
		int damage = 1 + GetAC()/2; //Will need some adjusting

		int mod = 0;
		int distance = charge_effect_increment;
		int max_distance = 1200;
		int max_modifier = spellbonuses.ChargeEffect[0]*100; //100;

		if (distance > max_distance)
			distance = max_distance;

		mod = 1 + ((distance * (max_modifier/max_distance))/100);

		damage += damage*mod/100;

		//Shout("DEBUG: Trigger Charge Hit: Incr [%i] Damage [%i / %i] Mod [%i]", charge_effect_increment, 1 + GetAC()/2 ,damage, mod);

		Message(MT_Spells, "You slams into %s !", target->GetCleanName());
		entity_list.MessageClose(this, true, 200, MT_Spells, "%s slams into %s !", GetCleanName(), target->GetCleanName());
		DoSpecialAttackDamage(target, SkillBash,  damage, 1, -1,0, false, false);
		Stun(500);

		if (target && !target->HasDied() && IsValidSpell(spellbonuses.ChargeEffect[2]))
			SpellFinished(spellbonuses.ChargeEffect[2], target, 10, 0, -1, spells[spellbonuses.ChargeEffect[2]].ResistDiff);
	}

	BuffFadeBySlot(spellbonuses.ChargeEffect[1]);
	SetChargeEffect(0);
	charge_effect_increment = 0;
	charge_effect_timer.Disable();
}

void Client::AdjustDiscTimer(uint32 timer_id, uint32 duration)
{

	pTimerType DiscTimer = pTimerDisciplineReuseStart + timer_id;

	//Clear Timer
	if (duration == 0){
		if (GetPTimers().Enabled((uint32)DiscTimer))
			GetPTimers().Clear(&database, (uint32)DiscTimer);
	}
	else
		CastToClient()->GetPTimers().Start(DiscTimer, duration);

	EQApplicationPacket *outapp = new EQApplicationPacket(OP_DisciplineTimer, sizeof(DisciplineTimer_Struct));
	DisciplineTimer_Struct *dts = (DisciplineTimer_Struct *)outapp->pBuffer;
	dts->TimerID = timer_id;
	dts->Duration = duration;

	QueuePacket(outapp);
	safe_delete(outapp);
}

void Client::TryOnClientUpdate()
{
	/*
	CHECKED AT END OF - void Client::Handle_OP_ClientUpdate(const EQApplicationPacket *app)
	This allows us to check any conditions immediately after the client position is updated.
	*/

	if (GetKnockBackMeleeImmune())
		SetKnockBackMeleeImmune(false);

	if (leap.increment) { 
		if (leap.increment >= 75) { //Trigger spell on next possible position update after landing.
			float dist = CalculateDistance(leap.origin_x, leap.origin_y,  leap.origin_z);
			if (dist > 40.0f) {
				if (IsValidSpell(leap.spell_id)){
					for (int i=0; i < EFFECT_COUNT; i++){
						if(spells[leap.spell_id].effectid[i] == SE_CastOnLeap){
							if (IsValidSpell(spells[leap.spell_id].base[i]) && leap.spell_id != spells[leap.spell_id].base[i])
								SpellFinished(spells[leap.spell_id].base[i], this, 10, 0, -1, spells[spells[leap.spell_id].base[i]].ResistDiff);
						}
					}
				}
			}
			else
				Message(MT_SpellFailure, "Your leap failed to gather enough momentum.");

			 //Reset variable to end
			leap.spell_id = SPELL_UNKNOWN;
			leap.increment = 0;
			leap.origin_x = 0.0f;
			leap.origin_y = 0.0f;
			leap.origin_z = 0.0f;
		}
	}
	//Shout("DEBUG:TryonClientUpdate :: X %.2f Y %.2f Z %.2f D: %.2f", GetX(), GetY(), GetZ(), m_DistanceSinceLastPositionCheck);
}

void Mob::BuffFastProcess()
{
	if (!HasFastBuff())
		return;

	int buff_count = GetMaxTotalSlots();

	for (int buffs_i = 0; buffs_i < buff_count; ++buffs_i)
	{
		if (buffs[buffs_i].spellid != SPELL_UNKNOWN)
		{
			if (!IsFastBuffTicSpell(buffs[buffs_i].spellid))
				continue;

			//If I decided to keep this using regular buff timer...
			//int remainder = buffs[buffs_i].fastticsremaining % 6;
			//if (!remainder)
			//DoBuffTic(buffs[buffs_i].spellid, buffs_i, buffs[buffs_i].ticsremaining, buffs[buffs_i].casterlevel, entity_list.GetMob(buffs[buffs_i].casterid));
			DoBuffTic(buffs[buffs_i], buffs_i, entity_list.GetMob(buffs[buffs_i].casterid));

			if(buffs[buffs_i].spellid == SPELL_UNKNOWN)
				continue;

			--buffs[buffs_i].fastticsremaining;
			buffs[buffs_i].ticsremaining = 1 + buffs[buffs_i].fastticsremaining/6;
			
			//Shout("DEBUG :: BuffFastProcess %i / %i [%i] [R: %i]", buffs[buffs_i].ticsremaining,0, buffs[buffs_i].fastticsremaining, 0);
			if (buffs[buffs_i].fastticsremaining == 0) 
				BuffFadeBySlot(buffs_i);

			if(IsClient() && !(CastToClient()->GetClientVersionBit() & BIT_SoFAndLater))
				buffs[buffs_i].UpdateClient = true;

			if(IsClient())
			{
				if(buffs[buffs_i].UpdateClient == true)
				{
					CastToClient()->SendBuffDurationPacket(buffs[buffs_i]);
					// Hack to get UF to play nicer, RoF seems fine without it
					if (CastToClient()->GetClientVersion() == ClientVersion::UF && buffs[buffs_i].numhits > 0)
						CastToClient()->SendBuffNumHitPacket(buffs[buffs_i], buffs_i);
					buffs[buffs_i].UpdateClient = false;
				}
			}
		}
	}
}

void Mob::DoFastBuffTic(uint16 spell_id, int slot, uint32 ticsremaining, uint8 caster_level, Mob* caster) {
	//No effects defined for use yet in this function.
	return;
	
	int effect, effect_value;

	if(!IsValidSpell(spell_id))
		return;

	const SPDat_Spell_Struct &spell = spells[spell_id];
	
	if (spell_id == SPELL_UNKNOWN)
		return;

	for (int i = 0; i < EFFECT_COUNT; i++)
	{
		if(IsBlankSpellEffect(spell_id, i))
			continue;

		effect = spell.effectid[i];
		effect_value = spell.base[i];

		/*
		switch(effect)
		{
			default:
			{
				// do we need to do anyting here?
			}
		}
		*/
	}
}

void Mob::ClearHasFastBuff(int exclude_slot)
{
	if (!HasFastBuff())
		return;

	int buff_max = GetMaxTotalSlots();
		
	for(int d = 0; d < buff_max; d++) {
	
		if (exclude_slot == d)
			continue;

		if(IsValidSpell(buffs[d].spellid) && (IsFastBuffTicSpell(buffs[d].spellid)))
			return;
	}

	SetFastBuff(false);
}

int16 Mob::GetCriticalChanceFlankBonus(Mob *defender, uint16 skill)
{
	int critical_chance = 0;

	critical_chance +=	itembonuses.CritHitChanceFlank[HIGHEST_SKILL+1] + spellbonuses.CritHitChanceFlank[HIGHEST_SKILL+1] + aabonuses.CritHitChanceFlank[HIGHEST_SKILL+1] +
						itembonuses.CritHitChanceFlank[skill] + spellbonuses.CritHitChanceFlank[skill] + aabonuses.CritHitChanceFlank[skill];

	if (critical_chance && defender && FlankMob(defender, GetX(), GetY())){
		
		if(critical_chance < -100)
			critical_chance = -100;
		
		return critical_chance;
	}
	else
		return 0;
}

void Client::CustomTickUpdates()
{
	if (GetBraveryRecast()){
		if (GetBraveryRecast() <= 0)
			SetBraveryRecast(0);
		else
			SetBraveryRecast(GetBraveryRecast() - 6);
	}

	for(int index_timer = 0; index_timer <= MAX_DISCIPLINE_TIMERS; index_timer++)
	{
		if (GetDiscSpamLimiter(index_timer)){
		if (GetDiscSpamLimiter(index_timer) <= 0)
			SetDiscSpamLimiter(index_timer, 0);
		else
			SetDiscSpamLimiter(index_timer, GetDiscSpamLimiter(index_timer) - 6);
		}
	}

	//Tick down 'Hunter's Efficacy'
	if (GetClass() == RANGER && spellbonuses.RangerGainNumhitsSP[0] && !GetAggroCount())
	{
		int slot = spellbonuses.RangerGainNumhitsSP[1];
		if (slot >= 0 && buffs[slot].numhits){

			if (buffs[slot].numhits <= 10)
				buffs[slot].numhits = 1;
			else
				buffs[slot].numhits -= 10;

			SendBuffNumHitPacket(buffs[slot], slot);
		}
	}

	//Tick down 'Ferocity'
	if (GetClass() == BEASTLORD && spellbonuses.BeastGainNumhitsSP[0] && !GetAggroCount())
	{
		int slot = spellbonuses.BeastGainNumhitsSP[1];
		if (slot >= 0 && buffs[slot].numhits){

			if (buffs[slot].numhits <= 25)
				buffs[slot].numhits = 1;
			else
				buffs[slot].numhits -= 25;

			SendBuffNumHitPacket(buffs[slot], slot);
		}
	}

	if (GetClass() == CLERIC && !spellbonuses.Faith[0])
	{
		SpellFinished(SPELL_FAITH, this, 10, 0, -1, spells[SPELL_FAITH].ResistDiff);
		SpellFinished(SPELL_LOSE_FAITH_1, this, 10, 0, -1, spells[SPELL_LOSE_FAITH_1].ResistDiff);
	}

}

void Mob::LifeShare(SkillUseTypes skill_used, int32 &damage, Mob* attacker)
{
	if (!attacker) 
		return;

	if (damage <= 0)
		return;
	//Base Penalty value is 100, meaning receives no change from base damage.

	if (spellbonuses.LifeShare[1]){ //Mitigation exists

		int slot = -1;
		int damage_to_reduce = 0;
		int32 base_damage = damage;

		slot = spellbonuses.LifeShare[0];
		if(slot >= 0)
		{
			Mob* caster = nullptr;
			caster = entity_list.GetMob(buffs[slot].casterid);

			if (!caster){
				BuffFadeByEffect(SE_LifeShare); //Fade if caster does not exist.
				return;
			}

			int damage_to_reduce = damage * spellbonuses.LifeShare[1] / 100;

			if(spellbonuses.LifeShare[3] && (damage_to_reduce >= static_cast<int32>(buffs[slot].melee_rune))){
				damage -= buffs[slot].melee_rune;
				if(!TryFadeEffect(slot))
					BuffFadeBySlot(slot);
			}
			else{
				if (spellbonuses.LifeShare[3])
					buffs[slot].melee_rune = (buffs[slot].melee_rune - damage_to_reduce);
				
				damage -= damage_to_reduce;
			}
		
			int32 dmg_to_caster = base_damage - damage_to_reduce;

			if (spellbonuses.LifeShare[2]){ //Penalty
				if (caster){
					dmg_to_caster = (dmg_to_caster*spellbonuses.LifeShare[2])/100;
					caster->Damage(attacker, damage, SPELL_UNKNOWN, skill_used, false);
				}
			}
		}
	}
}

int Mob::CalcDistributionModifer(int range, int min_range, int max_range, int min_mod, int max_mod)
{
	if (range > max_range)
		range = max_range;

	int dm_range = max_range - min_range;
	int dm_mod_interval = max_mod - min_mod;
	int dist_from_min = range - min_range;

	if (!dm_range)
		return 0;//Saftey
	
	int mod = min_mod + (dist_from_min * (dm_mod_interval/dm_range));
	return mod;
}

float Mob::CalcDistributionModiferFloat(float range, float min_range, float max_range, float min_mod, float max_mod)
{
	if (range > max_range)
		range = max_range;

	float dm_range = max_range - min_range;
	float dm_mod_interval = max_mod - min_mod;
	float dist_from_min = range - min_range;

	if (!dm_range)
		return 0;//Saftey
	
	float mod = min_mod + (dist_from_min * (dm_mod_interval/dm_range));
	return mod;
}

int Mob::CalcBaseEffectValueByLevel(float formula_mod, float ubase, float max, float caster_level, float max_level, uint16 spell_id)
{
	/*TO CALCULATE AS INTS - DevNote: Just leaving as float for now
	int level_mod = 100 + ((35.0 - (caster_level)) * 4);
	int interval = ((max * 100) - ubase) / (max_level - 1);
	int base_mod = (interval*100/level_mod) * (caster_level);
	return ubase + (base_mod/100);
	*/

	/*ALPHA - Main Damage/Heal Formula
	Finds an even distrubution between min and max damage(at 50)
	A level based gradient modifier is applied to the interval value so that scaling curve more accurately
	reflects live, with smaller increments of damage gained at lower levels and higher amoounts as you progress.
	Value 0.041 is dervived from (MAX_GRADIENT - MIN_GRADIENT) / (MAX_LEVEL - 1) :: 0.408 = (3 - 1 / 50-1)
	Increasing MAX_GRADIENT will cause a more expodiental growth curve. While lowering it flattens the curve ect.
	[When making new spells, set base value to -1 and max to whatever max. Value at level 1 will be the first interval]
	*/
	//heal (2) for max 500P
	//damage(3) max 800HP

	float level_distribution = 0;
	float level_distribution_max = 0;
	float level_mod = 1;

	if (formula_mod){ //If 5000 ignore level modifiers and do even interval split
		level_distribution_max = formula_mod / 100.0f;
		level_distribution = (level_distribution_max - 1.0f) / (max_level - 1.0f);
		level_mod = 1.0f + (max_level - caster_level) * level_distribution;
	}

	float interval = (max - ubase) / (max_level - 1.0f);
	float base_mod = (interval/level_mod) * (caster_level);

	//Used when calcing how mana cost at end of casting. This avoid recalcing value in these circumstances.
	//TODO: Can we optimize this to use in more circumstances, because we are calcing this twice (1: Get mana ratio 2: Actual value)
	//Problem: Saving a single variable would cause conflicts if a spell applys another spell that also uses formula. Hence, narrowly used for now.
	if (spells[spell_id].cast_from_crouch)
		SetScaledBaseEffectValue(static_cast<int>(ubase + base_mod));
	
	Shout("DEBUG: Scaled By Level: %i [spellid %i]",static_cast<int>(ubase + base_mod), spell_id);
	return static_cast<int>(ubase + base_mod);
}

int Mob::GetBaseEffectValueByLevel(int formula, int ubase, int max, Mob* caster, uint16 spell_id)
{
	if((formula >= 5000) && (formula < 7000)){
		
		if (!caster)
			return 1;
		
		int use_level = caster->GetLevel();
		int formula_mod = 0;
		
		if((formula >= 5000) && (formula < 6000)){
			formula_mod = formula - 5000;
		}

		else if((formula >= 6000) && (formula < 7000)){
			formula_mod = formula - 6000;
			if (GetLevel() < use_level)
				use_level = GetLevel();
		}
		
		return CalcBaseEffectValueByLevel(static_cast<float>(formula), static_cast<float>(ubase),
				static_cast<float>(max), static_cast<float>(use_level), CLIENT_MAX_LEVEL, spell_id);
	}

	return max;
}

void Mob::AbsorbMelee(int32 &damage, Mob* attacker)
{
	if (!attacker || attacker->IsClient())
		return;

	if (!spellbonuses.AbsorbMeleeDamage[3])//Must have rune amt defined
		return;

	int slot = spellbonuses.AbsorbMeleeDamage[1];

	if(slot >= 0)
	{
		damage += damage * spellbonuses.AbsorbMeleeDamage[0] / 100;
		
		if(spellbonuses.AbsorbMeleeDamage[3] && (damage >= static_cast<int32>(buffs[slot].melee_rune))){
			if(!TryFadeEffect(slot))
				BuffFadeBySlot(slot);
		}
		else{
			if (spellbonuses.AbsorbMeleeDamage[3]){
				buffs[slot].melee_rune = buffs[slot].melee_rune - damage;
			}
		}

		//Shout("DEBUG :: AbsorbMelee D %i Rune %i / %i", damage, buffs[slot].melee_rune, spellbonuses.AbsorbMeleeDamage[3]);
	}
}

void EntityList::FadeBuffFromCaster(uint16 caster_id, uint16 spell_id)
{ 
	//Removes from all npcs the effect field buff when the effect field base spell fades.
	if (!IsValidSpell(spell_id) || !caster_id)
		return;

	Mob *curmob;
	for (auto it = mob_list.begin(); it != mob_list.end(); ++it) {
		curmob = it->second;

		if (curmob && curmob->GetID() == caster_id && curmob->FindBuff(spell_id))
			curmob->BuffFadeBySpellID(spell_id);
	}
}

void Mob::FadeLinkedBuff(uint16 casterid, uint16 spellid)
{
	for (int i=0; i < EFFECT_COUNT; i++)
	{
		if (spells[spellid].effectid[i] == SE_FadeBuffFromCaster) 
		{
			entity_list.FadeBuffFromCaster(casterid, spells[spellid].base[i]);
			break;	
		}
	}
}

void Mob::ApplyEffectResource(uint16 spellid, int slot)
{
	//Limit Negative = MUST BE BELLOW THIS AMOUNT TO TRIGGER
	//Limit Positive = MUST BE ABOVE THIS AMOUNT TO TRIGGER
	uint16 trigger_spell_id = SPELL_UNKNOWN;

	//for (int i=0; i < EFFECT_COUNT; i++)
	//{
		//if (spells[spellid].effectid[i] == SE_ApplyEffectResource) 
		//{
	 
			int i = slot;

			if (spells[spellid].max[i] == 0){
				if (spells[spellid].base2[i] > 0){
					if (static_cast<int>(GetHPRatio()) > spells[spellid].base2[i])
						trigger_spell_id = spells[spellid].base[i];
				}
				else{
					if (static_cast<int>(GetHPRatio()) < (spells[spellid].base2[i] * -1))
						trigger_spell_id = spells[spellid].base[i];
				}
			}

			else if (spells[spellid].max[i] == 1){
				if (spells[spellid].base2[i] > 0){
					if (static_cast<int>(GetManaRatio()) > spells[spellid].base2[i])
						trigger_spell_id = spells[spellid].base[i];
				}
				else{
					if (static_cast<int>(GetManaRatio()) < (spells[spellid].base2[i] * -1))
						trigger_spell_id = spells[spellid].base[i];
				}
			}

			else if (IsClient() && spells[spellid].max[i] == 2){
				if (spells[spellid].base2[i] > 0){
					if (CastToClient()->GetEndurancePercent() > spells[spellid].base2[i])
						trigger_spell_id = spells[spellid].base[i];
				}
				else{
					if (CastToClient()->GetEndurancePercent() < (spells[spellid].base2[i] * -1))
						trigger_spell_id = spells[spellid].base[i];
				}
			}
		//}
	//}

	if (IsValidSpell(trigger_spell_id))
		SpellFinished(trigger_spell_id, this,10, 0, -1, spells[trigger_spell_id].ResistDiff);
}


void Mob::RangerGainNumHitsOutgoing(NumHit type, SkillUseTypes skill_used)
{
	if (!spellbonuses.RangerGainNumhitsSP[0])
		return;
	//Shout("DEBUG: Type: %i Amt %i Buff %i Max %i",type, spellbonuses.RangerGainNumhitsSP[0],spellbonuses.RangerGainNumhitsSP[1],spellbonuses.RangerGainNumhitsSP[2]);
	int amt = 0;
	int slot = -1;

	if (type == NumHit::OutgoingHitSuccess) {

		if (skill_used == Skill1HPiercing)
			amt = spellbonuses.RangerGainNumhitsSP[0];
		else if (skill_used == SkillBackstab)
			amt = spellbonuses.RangerGainNumhitsSP[0] + 10;

		slot = spellbonuses.RangerGainNumhitsSP[1];

		if (slot >= 0 && buffs[slot].numhits && IsClient()){

			if (buffs[slot].numhits >= static_cast<uint16>(spellbonuses.RangerGainNumhitsSP[2]))
				return;
		
			int _numhits = buffs[slot].numhits + amt;
						
			if (_numhits <= 0)
				_numhits = 1; //Min
			else if (_numhits >= spellbonuses.RangerGainNumhitsSP[2])
				_numhits = spellbonuses.RangerGainNumhitsSP[2]; //Max

			buffs[slot].numhits = _numhits;

			CastToClient()->SendBuffNumHitPacket(buffs[slot], slot);
		}
	}

	else if (type == NumHit::OutgoingHitAttempts){

		if (skill_used == SkillArchery){
	
			slot = spellbonuses.RangerGainNumhitsSP[1];
			if (slot >= 0 && buffs[slot].numhits){
				if (--buffs[slot].numhits == 0) {
					CastOnNumHitFade(buffs[slot].spellid);
					if (!TryFadeEffect(slot))
						BuffFadeBySlot(slot, true);
				} else if (IsClient()) { // still have numhits and client, update
					CastToClient()->SendBuffNumHitPacket(buffs[slot], slot);
				}
			}
		}
	}
}

void Client::SpinAttack() {
	
	if (!IsStunned() || !spellbonuses.SpinAttack[0])
	{
		CastToClient()->SetAllowPositionUpdate(true);
		spin_attack_increment = 0;
		spun_timer.Disable();
		return;
	}
	
	float head = GetHeading() - 15.0f;

	if (head > 510.0f)
		head = 0.0f;

	if (head < 0.0f)
		head = 510.0f;

	int anim = zone->random.Int(0,1);

	if (anim)
		DoAnim(anim1HPiercing);
	else
		DoAnim(animDualWield);

	MovePC(zone->GetZoneID(), zone->GetInstanceID(), GetX(), GetY(), GetZ(), head);

	//This determines interval of spell trigger (5) = every 500 seconds
	int divider = 5;
	int remainder = 0;

	if (spellbonuses.SpinAttack[1])
		divider = spellbonuses.SpinAttack[1];

	if (divider > 0)
		remainder = spin_attack_increment % divider;

	if (!remainder && IsValidSpell(spellbonuses.SpinAttack[0]))
		SpellFinished(spellbonuses.SpinAttack[0], this,10, 0, -1, spells[spellbonuses.SpinAttack[0]].ResistDiff);

	spin_attack_increment++;
}

void Mob::TryBackstabSpellEffect(Mob *other) {
	
	if(!other)
		return;

	bool bIsBehind = false;
	bool bCanFrontalBS = false;

	//make sure we have a proper weapon if we are a client.
	if(IsClient()) {
		const ItemInst *wpn = CastToClient()->GetInv().GetItem(MainPrimary);
		if(!wpn || (wpn->GetItem()->ItemType != ItemType1HPiercing)){
			Message_StringID(13, BACKSTAB_WEAPON);
			return;
		}
	}

	int tripleChance = itembonuses.TripleBackstab + spellbonuses.TripleBackstab + aabonuses.TripleBackstab;

	if (BehindMobCustom(other, GetX(), GetY()))
		bIsBehind = true;

	else {
		int FrontalBSChance = itembonuses.FrontalBackstabChance + spellbonuses.FrontalBackstabChance + aabonuses.FrontalBackstabChance;

		if (FrontalBSChance && (FrontalBSChance > zone->random.Int(0, 100)))
			bCanFrontalBS = true;
	}
	
	if (bIsBehind || bCanFrontalBS){ // Player is behind other OR can do Frontal Backstab

		if (bCanFrontalBS) 
			CastToClient()->Message(0,"Your fierce attack is executed with such grace, your target did not see it coming!");
		
		DoBackstabSpellEffect(other,false);
		if(IsClient() && CastToClient()->CheckDoubleAttack())
		{
			if(other->GetHP() > 0)
				DoBackstabSpellEffect(other,false);

			if (tripleChance && other->GetHP() > 0 && tripleChance > zone->random.Int(0, 100))
				DoBackstabSpellEffect(other,false);
			
		}
	}

	else if(aabonuses.FrontalBackstabMinDmg || itembonuses.FrontalBackstabMinDmg || spellbonuses.FrontalBackstabMinDmg) {

		//we can stab from any angle, we do min damage though.
		DoBackstabSpellEffect(other, true);
		if(IsClient() && CastToClient()->CheckDoubleAttack())
		{
			if(other->GetHP() > 0)
				DoBackstabSpellEffect(other,true);

			if (tripleChance && other->GetHP() > 0 && tripleChance > zone->random.Int(0, 100))
				DoBackstabSpellEffect(other,true);
		}
	}
	else { //We do a single regular attack if we attack from the front without chaotic stab
		Attack(other, MainPrimary, false,false,true);
	}
}

void Mob::DoBackstabSpellEffect(Mob* other, bool min_damage)
{
	if (!other)
		return;

	int32 ndamage = 0;
	int32 max_hit = 0;
	int32 min_hit = 0;
	int32 hate = 0;
	int32 primaryweapondamage = 0;
	int32 backstab_dmg = 0;
	int ReuseTime = 10;

	if(IsClient()){
		const ItemInst *wpn = nullptr;
		wpn = CastToClient()->GetInv().GetItem(MainPrimary);
		if(wpn) {
			primaryweapondamage = GetWeaponDamage(other, wpn);
			backstab_dmg = wpn->GetItem()->BackstabDmg;
			for (int i = 0; i < EmuConstants::ITEM_COMMON_SIZE; ++i)
			{
				ItemInst *aug = wpn->GetAugment(i);
				if(aug)
				{
					backstab_dmg += aug->GetItem()->BackstabDmg;
				}
			}
		}
	}
	else{
		primaryweapondamage = (GetLevel()/7)+1; // fallback incase it's a npc without a weapon, 2 dmg at 10, 10 dmg at 65
		backstab_dmg = primaryweapondamage;
	}
	
	if(primaryweapondamage > 0){
	
		if(level > 25){
			max_hit = (((2*backstab_dmg) * GetDamageTable(Skill1HPiercing) / 100) * 10 * GetSkill(Skill1HPiercing) / 355) + ((level-25)/3) + 1;
			hate = 20 * backstab_dmg * GetSkill(Skill1HPiercing) / 355;
		}
		else{
			max_hit = (((2*backstab_dmg) * GetDamageTable(Skill1HPiercing) / 100) * 10 * GetSkill(Skill1HPiercing) / 355) + 1;;
			hate = 20 * backstab_dmg * GetSkill(Skill1HPiercing) / 355;
		}

		// determine minimum hits
		if (level < 51) {
			min_hit = (level*15/10);
		}
		else {
			min_hit = (level * ( level*5 - 105)) / 100;
		}

		if(!other->CheckHitChance(this, Skill1HPiercing, 0))	{
			ndamage = 0;
		}
		else{
			if(min_damage){
				ndamage = min_hit;
			}
			else {
				if (max_hit < min_hit)
					max_hit = min_hit;

				ndamage = max_hit;
			}
		}
	}
	else{
		ndamage = -5;
	}

	ndamage = mod_backstab_damage(ndamage);
	
	uint32 Assassinate_Dmg = 0;
	Assassinate_Dmg = TryAssassinate(other, Skill1HPiercing, ReuseTime);
	
	if (Assassinate_Dmg) {
		ndamage = Assassinate_Dmg;
		entity_list.MessageClose_StringID(this, false, 200, MT_CritMelee, ASSASSINATES, GetName());
	}

	DoSpecialAttackDamage(other, SkillBackstab, ndamage, min_hit, hate, ReuseTime, false, false);
	DoAnim(anim1HPiercing);
}

void Mob::TryBackstabHeal(Mob* other, uint16 spell_id)
{
	if (!other || !IsValidSpell(spell_id))
		return;

	if(IsClient()) {
		const ItemInst *wpn = CastToClient()->GetInv().GetItem(MainPrimary);
		if(!wpn || (wpn->GetItem()->ItemType != ItemType1HPiercing)){
			Message_StringID(13, BACKSTAB_WEAPON);
			return;
		}
	}

	bool bIsBehind = false;

	if (BehindMobCustom(other, GetX(), GetY()))
		bIsBehind = true;

	TryApplyEffectBackstab(spell_id, other, bIsBehind);
}

void Mob::TryApplyEffectBackstab(uint16 spell_id, Mob* target, bool backstab)
{
	if(!IsValidSpell(spell_id))
		return;

	if (!target)
		return;

	if (target->GetID() == GetID() ){
		Message_StringID(13,TRY_ATTACKING_SOMEONE);
		return;
	}

	for(int i = 0; i < EFFECT_COUNT; i++){
		if (spells[spell_id].effectid[i] == SE_ApplyEffectBackstab){
			if(zone->random.Int(0, 100) <= spells[spell_id].base[i]){

				if (backstab && spells[spell_id].max[i] == 1){
					if (IsValidSpell(spells[spell_id].base2[i]))
						SpellFinished(spells[spell_id].base2[i], target, 10, 0, -1, spells[spells[spell_id].base2[i]].ResistDiff);
					
					return;
				}

				else if (!backstab && !spells[spell_id].max[i]){
					if (IsValidSpell(spells[spell_id].base2[i]))
						SpellFinished(spells[spell_id].base2[i], target, 10, 0, -1, spells[spells[spell_id].base2[i]].ResistDiff);
					
					return;
				}
			}
		}
	}
}

void Mob::DoPetEffectOnOwner()
{
	//Pet will autocast buff on owner if in range, buff fades if out of range of pet
	if (spellbonuses.PetEffectOnOwner) {
		Mob* owner = nullptr;
		if (IsNPC() && GetOwner())
			owner = GetOwner();
		
		if (!owner)
			return;
			
		uint16 spell_id = spellbonuses.PetEffectOnOwner;
		if (IsValidSpell(spell_id)){

			float dist2 = spells[spell_id].range * spells[spell_id].range;
			float dist_targ = 0;
			dist_targ = DistanceSquared(owner->GetPosition(), GetPosition());

			if (dist_targ > dist2){
				if (owner->FindBuff(spell_id))
					owner->BuffFadeBySpellID(spell_id);
			
				return;
			}
			if(!owner->FindBuff(spell_id))
				SpellFinished(spell_id, owner, 10, 0, -1, spells[spell_id].ResistDiff);
		}
	}
	else
		pet_buff_owner_timer.Disable();
}

void Client::ArcheryAttackSpellEffect(Mob* target, uint16 spell_id, int i)
{
	if (!target)
		return;

	int numattacks = spells[spell_id].base[i];

	if (!numattacks)
		return;

	const ItemInst* RangeWeapon = m_inv[MainRange];
	const ItemInst* Ammo = m_inv[MainAmmo];

	if (!RangeWeapon || !RangeWeapon->IsType(ItemClassCommon)) 
		return;

	if (!Ammo || !Ammo->IsType(ItemClassCommon))
		return;
	
	const Item_Struct* RangeItem = RangeWeapon->GetItem();
	const Item_Struct* AmmoItem = Ammo->GetItem();

	if(RangeItem->ItemType != ItemTypeBow)
		return;

	if(AmmoItem->ItemType != ItemTypeArrow)
		return;

	float speed = 4.0f;
	uint16 _spell_id = spell_id;
	int hit_chance = spells[spell_id].max[i];
	int16 dmgpct = spells[spell_id].base2[i];
	int16 dmod = -1;

	if (GetSpellPowerDistanceMod())
		dmod = 0;

	if (IsNoTargetRequiredSpell(spell_id)) {

		//Min Distance checks for directional spells - Hit Chance -10000
		float dist = DistanceSquared(target->GetPosition(), GetPosition());

		float min_range = static_cast<float>(RuleI(Combat, MinRangedAttackDist));

		if (spells[spell_id].min_range > min_range)
			min_range = spells[spell_id].min_range;
		
		if (dist < min_range*min_range){
			hit_chance = -10000; //Too close!
		}
	}

	if (hit_chance == 10001){//Increase hit chance only if from position
		if (PassEffectLimitToDirection(target, spell_id)){
			hit_chance = 10000;
		}
		else {
			if (IsEffectInSpell(spell_id, SE_ApplyEffectProjectileHit))
				_spell_id = SPELL_UNKNOWN; //For proc chance requiring hit from flank.

			hit_chance = 0;
		}
	}

	if (spells[spell_id].MinResist){
		int ratio_modifier = 0;
		int dmod = 0;

		if (PassEffectLimitToDirection(target, spell_id)){
			ratio_modifier = 101 - static_cast<int>(target->GetHPRatio());
			if (ratio_modifier > 0){
				dmod = CalcDistributionModifer(ratio_modifier, 1, 100, spells[spell_id].MinResist, spells[spell_id].MaxResist);
				dmgpct = dmod + 1;
			}
		}
	}
	
	if (GetMinAtks(spell_id)) //Number of attacks MIN for random amount of attacks [LightType].
		numattacks = zone->random.Int(GetMinAtks(spell_id), spells[spell_id].base[i]);

	if (spells[spell_id].targettype == ST_Directional)
		speed = zone->random.Real(3.5, 4.5);//So they don't all clump
	for(int x = 0; x < numattacks; x++){
		if (!HasDied()){
			DoArcheryAttackDmg(target,  RangeWeapon, Ammo, 0, hit_chance, 0, 0, 0, 0, AmmoItem, MainAmmo, speed, _spell_id, dmod, dmgpct);
		}
	}
}

bool Mob::RangeDiscCombatRange(uint32 target_id, uint16 spell_id)
{
	if (!IsRangeSpellEffect(spell_id))
		return true;

	//Item Check for AOE effects.
	if (spells[spell_id].aoerange){
		if (CastToClient()->GetArcheryRange(nullptr, true) == -1)//ITEM CHECK ONLY
			return true;
		else
			return false;
	}


	if (spells[spell_id].range != UseRangeFromRangedWpn())
		return true;

	Mob* target = nullptr;
	target = entity_list.GetMob(target_id);

	if (!target){
		Message(13, "You must first select a target for this ability!");
		return false;
	}

	if (GetDiscHPRestriction(spell_id) > 0){
		if (static_cast<int>(target->GetHPRatio()) > GetDiscHPRestriction(spell_id)){
			Message(13, "Your target must be weakened under %i percent health to execute this attack!", GetDiscHPRestriction(spell_id));
			return false;
		}
	}

	float range = CastToClient()->GetArcheryRange(target);

	if (range == 0)
		return false;
	
	float range_spell = spells[spell_id].range + GetRangeDistTargetSizeMod(target);
	
	if (!spells[spell_id].range)
		range_spell = spells[spell_id].aoerange;

	if (!UseEquipedBowRange(spell_id))//Use Default value [Spell Range set to 351]
		range = range_spell;

	float min_range = static_cast<float>(RuleI(Combat, MinRangedAttackDist));
	float min_range_sp = spells[spell_id].min_range;

	if (min_range_sp > min_range)
		min_range = min_range_sp;

	float dist = DistanceSquared(target->GetPosition(), GetPosition() );
	//Shout("DEBUG: Dist %.2f Max %.2f Min %.2f", dist, range, min_range);	
	if(dist > (range * range)) {
		Message_StringID(13, TARGET_OUT_OF_RANGE);
		return false;
	}

	if(dist < (min_range * min_range)) {
		if (min_range == min_range_sp)
			Message_StringID(13, TARGET_TOO_CLOSE);
		else
			Message_StringID(15, RANGED_TOO_CLOSE);
		
		return false;
	}

	if (!IsFacingMob(target)){
		Message_StringID(13, CANT_SEE_TARGET);
		return false;
	}
				
	if (GetDiscLimitToBehind(spell_id)){ //1 = Must attack from behind.
		if (!BehindMobCustom(target, GetX(), GetY())){
			Message(13, "You must be behind your target to execute this attack!");
			return false;
		}
	}

	return true;
}

float Client::GetArcheryRange(Mob* other, bool ItemCheck)
{
	const ItemInst* RangeWeapon = m_inv[MainRange];
	const ItemInst* Ammo = m_inv[MainAmmo];

	if (!RangeWeapon || !RangeWeapon->IsType(ItemClassCommon)){ 
		Message(13, "You must have a ranged weapon equiped!");
		return 0;
	}

	if (!Ammo || !Ammo->IsType(ItemClassCommon)){
		Message(13, "You must have ammo equiped!");
		return 0;
	}
	
	const Item_Struct* RangeItem = RangeWeapon->GetItem();
	const Item_Struct* AmmoItem = Ammo->GetItem();

	if(!RangeItem || RangeItem->ItemType != ItemTypeBow){
		Message(13, "You must have a ranged weapon equiped!");
		return 0;
	}

	if(!AmmoItem || AmmoItem->ItemType != ItemTypeArrow){
		Message(13, "You must have ammo equiped!");
		return 0;
	}
	
	//Set Graphic to bow if attempting to use ability from spells/disc
	int32 material = 0;
	if (strlen(RangeItem->IDFile) > 2)
		material = atoi(&RangeItem->IDFile[2]);
	else
		material = RangeItem->Material;

	if (material){
		WearChange(MaterialPrimary, 0, 0);
		WearChange(MaterialSecondary, material, 0);
	}

	if (ItemCheck)
		return -1;
	
	float range = 0.0f;

	if (other)
		range = RangeItem->Range + AmmoItem->Range + GetRangeDistTargetSizeMod(other);
	else
		range = RangeItem->Range + AmmoItem->Range + 18.0f;
	
	return range;
}

bool Mob::PassEffectLimitToDirection(Mob* other, uint16 spell_id)
{
	if (!other)
		return false;

	if (GetEffectLimitToBehind(spell_id) && !BehindMobCustom(other, GetX(), GetY()))
		return false;
	
	else if (GetEffectLimitToFront(spell_id) && !InFrontMob(other, GetX(), GetY()))
		return false;

	else if (GetEffectLimitToFlank(spell_id) && !FlankMob(other, GetX(), GetY()))
		return false;

	return true;
}

bool Mob::TryRangerCastingConditions(uint16 spell_id, uint16 target_id)
{
	if (IsClient() && GetClass() == RANGER) {
		
		if (!RangeDiscCombatRange(target_id, spell_id))
			return false;
	}
	return true;
}

void Mob::BalanceResourceEffect(uint16 spell_id, int e)
{
	if (!IsValidSpell(spell_id))
		return;

	int mod = spells[spell_id].base2[e];

	int amt = (GetManaPercent() + CastToClient()->GetEndurancePercent()) / 2;
	amt += amt * mod / 100;

	SetMana((GetMaxMana() * amt / 100));
	CastToClient()->SetEndurance((CastToClient()->GetMaxEndurance() * amt / 100));
}

void Mob::BreakMovementDebuffs()
{
	//Break deterimental snares and roots.
	int buff_count = GetMaxTotalSlots();
	for(int i = 0; i < buff_count; i++) {
		if (IsValidSpell(buffs[i].spellid)){

			if (spells[buffs[i].spellid].dispel_flag)
				continue;

			for(int d = 0; d < EFFECT_COUNT; d++)
			{
				if (spells[buffs[i].spellid].effectid[d] == SE_Root){
					BuffFadeBySlot(i);
					continue;
				}

				if (spells[buffs[i].spellid].effectid[d] == SE_MovementSpeed && !spells[buffs[i].spellid].goodEffect){
					BuffFadeBySlot(i);
					continue;
				}
			}
		}
	}
}

bool Mob::MinCastingRange(uint16 spell_id, uint16 target_id)
{
	if (!spells[spell_id].min_range)
		return false;

	float dist2 = 0;
	bool target_ring = false;

	if (IsTargetableSpell(spell_id)){

		Mob* spell_target = nullptr;
		spell_target = entity_list.GetMob(target_id);

		if (!spell_target || spell_target == nullptr || spell_target == this)
			return false;

		dist2 = DistanceSquared(m_Position, spell_target->GetPosition());
	}

	if (IsTargetRingSpell(spell_id)){
		dist2 =  DistanceSquared(static_cast<glm::vec3>(GetPosition()), GetTargetRingLocation());
		target_ring = true;
	}

	if (dist2){

		float min_range2 = spells[spell_id].min_range * spells[spell_id].min_range;
		if (dist2 < min_range2){
			
			if (!target_ring)
				Message_StringID(13, TARGET_TOO_CLOSE);
			else
				Message(MT_SpellFailure, "You are too close to your targeted location. Get farther away.");
			return(true);
		}
	}

	return false;
}

int Mob::CustomBuffDurationMods(Mob *caster, uint16 spell_id, int duration)
{
	
	//res += caster->CalcSpellPowerManaMod(spell_id); //C!Kayen - Add buff ticks
	duration += GetSpellPowerDistanceMod()/100; //C!Kayen - Add buff ticks based on distance modifer
	return duration;
}

int Mob::GetCustomSpellResistMod(uint16 spell_id)
{
	
	int mod = spellbonuses.SpellResistMod[HIGHEST_RESIST] + itembonuses.SpellResistMod[HIGHEST_RESIST] + aabonuses.SpellResistMod[HIGHEST_RESIST] +
	spellbonuses.SpellResistMod[spells[spell_id].resisttype] + itembonuses.SpellResistMod[spells[spell_id].resisttype] + 
	aabonuses.SpellResistMod[spells[spell_id].resisttype];
	return mod;
}

void Mob::DoLeapSpellEffect(uint16 spell_id, int anim, int anim_speed, int DirOpts, int d_interval, int d_max, int velocity, float zmod1, float zmod2, 
							float set_x, float set_y, float set_z)
{
	//PERL FUNCTION - Leap
	float dX = 0;
	float dY = 0;
	float dZ = 0;

	float Direction = 0;

	if (!spell_id)
		spell_id = SPELL_UNKNOWN;

	bool loc_override = false;
	if (set_x && set_y && set_z){
		loc_override = true;
	}

	//Headings
	if (!DirOpts){
		Direction = GetHeading();
	}
	else if (DirOpts < 0) { // Automatic Special Cases
		if (DirOpts == -1)//Jump Backwards
			Direction = GetReverseHeading(GetHeading());

		else if (DirOpts == -10)//Jump Random
			Direction = zone->random.Real(0.0, 255.0);

		else if (DirOpts == -11) //Random Front Angle
			Direction = GetHeading() + zone->random.Real((- 45.0f), (45.0f));
		
		else if (DirOpts == -2) //Random Back Angle
			Direction = GetReverseHeading(GetHeading()) + zone->random.Real((- 45.0f), (45.0f));

		if (Direction >= 256)//Fail safe
			Direction = Direction - 256;
		else if (Direction < 0)//Fail safe
			Direction = 256 + Direction;
	}
	else{
		if (DirOpts >= 255)
			Direction = GetHeading();
		else
			Direction = DirOpts; //Set Direction manually
	}

	if (!d_interval)
		d_interval = 5;

	if (!d_max)
		d_max = 100;

	if (!loc_override)
		GetFurthestLocationLOS(Direction, d_interval , d_max, dX, dY, dZ); //Pass cooridinates through
	else{
		dX = set_x;
		dY = set_y;
		dZ = set_z;
	}

	float distance = CalculateDistance(dX, dY, dZ); //[TODO: Bestanimation based on distance 100 = 3)]
					
	float Face = CalculateHeadingToTarget(dX, dY);

	if (DirOpts  == -1 || DirOpts  == -2)
		Face = GetHeading();//Jump Backwards

	if (!anim)
		anim = 19;
	if (!anim_speed)
		anim_speed = 3;

	if (distance > 0){
		DoAnim(anim,anim_speed); //This should be replaced by target animation
		SetLeapSpellEffect(spell_id, velocity,zmod1,zmod2, dX, dY, dZ, Face);
	}
}

void Mob::SetLeapSpellEffect(uint16 spell_id, int velocity, float zmod1, float zmod2, float dX, float dY, float dZ, float dH)
{
	if (leapSE.increment)
		return; //Do not set a leap effect if currently in motion.

	if (!zmod1)
		zmod1 = 3.0f;
	if (!zmod2)
		zmod2 = 1.5f;
	if (zmod2 == -1)
		zmod2 = zmod1/2.0f;

	if (!velocity)
		velocity = 2;

	leapSE.increment = 1;
	leapSE.spell_id = spell_id;
	leapSE.velocity = velocity;
	leapSE.dest_x = dX;
	leapSE.dest_y = dY;
	leapSE.dest_z = dZ;
	leapSE.dest_h = dH;
	float pre_mod = CalculateDistance(dX, dY, dZ);
	//Shout("DEBUG: Leap Debug: Zmod1 %.2f Zmod2 %.2f [Distance %.2f", zmod1,zmod2, pre_mod);
	leapSE.z_bound_mod = pre_mod / zmod1;
	leapSE.mod = pre_mod / zmod2;
	
	SetFlyMode(1);
	SetAINoChase(1);
	leapSE_timer.Start(100);
}

void Mob::ClearLeapSpellEffectData()
{
	leapSE.increment = 0;
	leapSE.spell_id = SPELL_UNKNOWN;
	leapSE.velocity = 0;
	leapSE.dest_x = 0;
	leapSE.dest_y = 0;
	leapSE.dest_z = 0;
	leapSE.dest_h = 0;
	leapSE.z_bound_mod = 0;
	leapSE.mod = 0;
	
	SetFlyMode(0);
	SetAINoChase(0);
	leapSE_timer.Disable();
}

void Mob::LeapSpellEffect()
{
	//This requires further review and refinement when ultimately used.
	//#15 Seems acceptable for clients
	//Timer is mili second, to move faster do it mulitiple times per mili second (veloctiy value)
	if (!leapSE.increment)
		return;
	
	for(int i = 0; i < leapSE.velocity; i++){
      
		if(leapSE.increment >= leapSE.mod){

			if (IsNPC()){
				char temp[64];
				sprintf(temp, "%d", leapSE.spell_id);
				parse->EventNPC(EVENT_LEAP_LAND, CastToNPC(), nullptr, temp, 0);//PERLCUSTOM
			}

			CastOnLeapSELand(leapSE.spell_id);			
			ClearLeapSpellEffectData();

			if (IsPet()){
				pet_resume_autofollow.Start(6000); //Cleans up animation issues.
				SetPetOrder(SPO_Guard);
				CastToNPC()->SaveGuardSpot();
			}

			return;
		}

		GlideWithBounceTimer(
		GetX(),
		GetY(),
		GetZ(),		
		leapSE.dest_x,
		leapSE.dest_y, 
		leapSE.dest_z,
		leapSE.dest_h,
		leapSE.z_bound_mod,
		leapSE.mod,
		leapSE.increment);

		leapSE.increment++;
	 }

	leapSE.increment++;
	return;
}

void Mob::GlideWithBounceTimer(float StartX, float StartY, float StartZ, float DestX, float DestY, float DestZ, float DestH, float z_bounce, float mod, int i)
{
	float dir_x = DestX - StartX;
    float dir_y = DestY - StartY;
    float dir_z = DestZ - StartZ;
    float cur_x = (i / mod) * dir_x + StartX;
    float cur_y = (i / mod) * dir_y + StartY;
    float cur_z = (i / mod) * dir_z + StartZ;
      
    if((i / mod) <= 0.5) {
        cur_z += z_bounce * (i / mod);
    } else {
        cur_z += z_bounce * (1 - (i / mod));
    }
   
	if (IsClient())
		CastToClient()->MovePC(zone->GetZoneID(), zone->GetInstanceID(),  cur_x, cur_y, cur_z, DestH);
	else
		GMMove(cur_x, cur_y, cur_z, DestH);
}

bool Mob::GetFurthestLocationLOS(float heading, int d_interval, int d_max, float &loc_X, float &loc_Y, float &loc_Z,
								 bool FromLocs, float origin_x, float origin_y, float origin_z)
{
	
	float dX = 0;
	float dY = 0;
	float dZ = 0;
		
	int current_distance = d_interval;
	
	float last_dX = 0;
	float last_dY = 0;
	float last_dZ = 0;

	if (!FromLocs){
		last_dX = GetX();
		last_dY = GetY();
		last_dZ = GetZ();
	}
	else{
		last_dX = origin_x;
		last_dY = origin_y;
		last_dZ = origin_z;

	}
	
	while (current_distance <= d_max)
	{
		CalcDestFromHeading(heading, static_cast<float>(current_distance), 0, GetX(), GetY(), dX, dY, dZ); //Pass cooridinates	

		if (!CheckLosFN(dX,dY,dZ,GetSize()))
		{
			loc_X = last_dX;
			loc_Y = last_dY;
			loc_Z = last_dZ;
			return 1;
		}
		
		current_distance = current_distance + d_interval;
		
		if (current_distance > d_max)
		{
			loc_X = dX;
			loc_Y = dY;
			loc_Z = dZ;	
			return 1;
		}
		
		last_dX = dX;
		last_dY = dY;
		last_dZ = dZ;
	}

	return 0;
}

bool Mob::GetRandLocFromDistance(float distance, float &loc_X, float &loc_Y, float &loc_Z)
{
	float dX = 0;
	float dY = 0;
	float dZ = 0;

	for(int i = 0; i < 100; i++){
		
		int rnd_Heading = zone->random.Int(0, 256);

		CalcDestFromHeading(static_cast<float>(rnd_Heading), distance, 0, GetX(), GetY(), dX, dY, dZ); //Pass cooridinates

		if (CheckLosFN(dX,dY,dZ,GetSize()))
		{
			loc_X = dX;
			loc_Y = dY;
			loc_Z = dZ;
			return 1;
		}
	}

	return 0;
}

void Mob::CastOnLeapSELand(uint16 spell_id)
{
	if (!IsValidSpell(spell_id))
		return;

	for (int i=0; i < EFFECT_COUNT; i++){
		if(spells[spell_id].effectid[i] == SE_CastOnLeapSELand){
			if (IsValidSpell(spells[spell_id].base[i]) && spell_id != spells[spell_id].base[i]){
				SpellFinished(spells[spell_id].base[i], this, 10, 0, -1, spells[spells[spell_id].base[i]].ResistDiff);
			}
		}
	}
}

void Mob::Push(Mob *caster, uint16 spell_id, int i)
{

	if (IsNPC() && (IsRooted() || IsPseudoRooted()))
		return; //Rooted NPC can not be knocked back.

	float dX = 0;
	float dY = 0;
	float dZ = 0;

	float Direction = GetReverseHeading(GetHeading());

	if (caster){
		Direction = CalculateHeadingToTarget(caster->GetX(), caster->GetY());
		Direction = GetReverseHeading(Direction);
	}

	GetFurthestLocationLOS(Direction, 1, spells[spell_id].base[i], dX, dY, dZ); //Pass cooridinates through
	DoAnim(spells[spell_id].TargetAnim,3); //20 = Jump Up
	SetLeapSpellEffect(spell_id, 2,static_cast<float>(spells[spell_id].base2[i]),-1, dX, dY, dZ, GetHeading());

	//GetFurthestLocationLOS(Direction, 1, spells[spell_id].base[i], dX, dY, dZ); //Pass cooridinates through
	//GMMove(dX, dY, dZ + 2.0f, GetHeading());
}

float Mob::GetReverseHeading(float Heading)
{
	float ReverseHeading = 128 + Heading;
	
	if (ReverseHeading >= 256)
		ReverseHeading = ReverseHeading - 256;

	return ReverseHeading;
}

int Mob::GetRakePositionBonus(Mob* target)
{
	if (!target)
		return 0;

	/*Position Key
	Left	= 0
	Right	= 1
	Back	= 2
	Front	= 3
	NPC ID  = 4
	*/
	int position = -1;

	if (LeftMob(target))
		position = 0;
	else if (RightMob(target))
		position = 1;
	else if (BehindMobCustom(target, GetX(), GetY()))
		position = 2;
	else if (InFrontMob(target,GetX(), GetY()))
		position = 3;

	//Shout("DEBUG: Find POsition %i ID COMP %i %i", position, RakePosition[4], target->GetID());

	if (position == -1)
		return 0; //Error No Position found

	//Reset if change NPC
	if (!RakePosition[4])
		RakePosition[4] = target->GetID();
	else if (RakePosition[4] != target->GetID()){
		for (int i = 0; i < MAX_POSITION_TYPES + 1; i++) { RakePosition[i] = 0; }
		RakePosition[4] = target->GetID();
	}
			
    int highest = 0;

    for(int i=0;i<MAX_POSITION_TYPES;i++)
    {
        if(RakePosition[i] > 0)
			highest++;
    }

	//Shout("DEBUG: Highest %i", highest);
	//Shout("DEBUG: Array %i %i %i %i ID %i", RakePosition[0],RakePosition[1],RakePosition[2],RakePosition[3],RakePosition[4]);

	if (!RakePosition[position]){
		RakePosition[position] = 1;

		if (highest == 3){
			for (int i = 0; i < MAX_POSITION_TYPES + 1; i++) { RakePosition[i] = 0; }
		}

		return highest;
	}
	else{ //Reset count if same 2x in a row
		for (int i = 0; i < MAX_POSITION_TYPES + 1; i++) { RakePosition[i] = 0; }
	}
	
	return 0;
}

void Mob::TryMonkAbilitySpellEffect(Mob *other, uint16 spell_id, int effectid) {
	
	if(!other)
		return;

	int32 max_dmg = 0;
	int32 min_dmg = 1;
	SkillUseTypes skill_type;

	int bonus = spells[spell_id].base[effectid];
	
	switch(spells[spell_id].skill){

		case SkillFlyingKick:{
			skill_type = SkillFlyingKick;
			max_dmg = ( (GetSTR()+GetSkill(SkillOffense)) * (RuleI(Combat, FlyingKickBonus) + bonus) / 100) + 35;
			min_dmg = ((level*8)/10);
			ApplySpecialAttackMod(SkillFlyingKick, max_dmg, min_dmg);
			break;
		}

		case SkillDragonPunch:{
			skill_type = SkillDragonPunch;
			max_dmg = ((GetSTR()+GetSkill(SkillOffense)) * RuleI(Combat, DragonPunchBonus) / 100) + 26;
			ApplySpecialAttackMod(skill_type, max_dmg, min_dmg);
			break;
		}

		case SkillEagleStrike:{
			skill_type = SkillEagleStrike;
			max_dmg = ((GetSTR()+GetSkill(SkillOffense)) * RuleI(Combat, EagleStrikeBonus) / 100) + 19;
			ApplySpecialAttackMod(skill_type, max_dmg, min_dmg);
			break;
		}

		case SkillTigerClaw:{
			skill_type = SkillTigerClaw;
			max_dmg = ((GetSTR()+GetSkill(SkillOffense)) * RuleI(Combat, TigerClawBonus) / 100) + 12;
			ApplySpecialAttackMod(skill_type, max_dmg, min_dmg);
			break;
		}

		case SkillRoundKick:{
			skill_type = SkillRoundKick;
			max_dmg = ((GetSTR()+GetSkill(SkillOffense)) * RuleI(Combat, RoundKickBonus) / 100) + 10;
			ApplySpecialAttackMod(skill_type, max_dmg, min_dmg);
			break;
		}

		default:
			return;
	}

	DoSpecialAttackDamage(other, skill_type, max_dmg, min_dmg, -1, 10, true);
}

void Mob::BeastGainNumHitsOutgoing(NumHit type, SkillUseTypes skill_used)
{
	if (!spellbonuses.BeastGainNumhitsSP[0])
		return;
	//Shout("DEBUG: Type: %i Amt %i Buff %i Max %i",type, spellbonuses.RangerGainNumhitsSP[0],spellbonuses.RangerGainNumhitsSP[1],spellbonuses.RangerGainNumhitsSP[2]);
	int amt = 0;
	int slot = -1;

	if (type == NumHit::OutgoingHitSuccess) {

		amt = spellbonuses.BeastGainNumhitsSP[0];
		//Different skillls will increase AMT, ie kick ect
		slot = spellbonuses.BeastGainNumhitsSP[1];

		if (slot >= 0 && buffs[slot].numhits && IsClient()){

			if (buffs[slot].numhits >= static_cast<uint16>(spellbonuses.BeastGainNumhitsSP[2]))
				return;
		
			int _numhits = buffs[slot].numhits + amt;
						
			if (_numhits <= 0)
				_numhits = 1; //Min
			else if (_numhits >= spellbonuses.BeastGainNumhitsSP[2])
				_numhits = spellbonuses.BeastGainNumhitsSP[2]; //Max

			buffs[slot].numhits = _numhits;

			CastToClient()->SendBuffNumHitPacket(buffs[slot], slot);
		}
	}
}

int Mob::CalcSpellEffectValue_formula_custom(Mob* caster, int formula, int base, int max, int caster_level, uint16 spell_id, int ticsremaining)
{
	if (!caster)
		return base;

	// AEDuration interval Check - Need to consider how debuff will stack with multiple clients...
	if (formula >= 4000 && formula < 5000){
		base = base + ((formula - 4000) * caster->GetAEDurationIteration());
	}

	//Degrade effect by value
	else if (formula >= 5000 && formula < 5100){
		int ticdif = spells[spell_id].buffduration - (ticsremaining - 1);
		if(ticdif < 0)
			ticdif = 0;

		base = base - ((formula - 5000) * ticdif);
	}

	//Upgrade effect by value (Use this if you want slows to get weaker)
	else if (formula >= 5100 && formula < 5200){
		int ticdif = spells[spell_id].buffduration - (ticsremaining - 1);
		if(ticdif < 0)
			ticdif = 0;

		base = base + ((formula - 5100) * ticdif);
	}

	if (max != 0)
	{
		if (base > max)
			base = max;
	}

	return base;
}

int32 Mob::CalcSpellPowerFromAEDuration(uint16 spell_id, Mob* caster, int type)
{
	if (!caster || !spells[spell_id].AEDuration)
		return 0;

	/*TYPE
	1 = Dmg Focus
	2 = Ramge

	*/

	for (int i = 0; i <= EFFECT_COUNT; i++)
	{
		if (spells[spell_id].effectid[i] == SE_SpellPowerAEDurationType){
			if (spells[spell_id].base2[i] == type)
				return caster->GetAEDurationIteration() * spells[spell_id].base[i];
		}
	}
	return 0;
}

void Mob::AdjustNumHitsFaith(uint16 spell_id, int effectid)
{
	//If used as a resource to be drained after casting completed, set Limit to -1
	//Otherwise spell is checked in regular spell effect routine.

	if (IsClient() && spellbonuses.Faith[0] && IsValidSpell(spell_id)){

		//Shout("DEBUG: SE_NumHitsAmtFaith 1 Faith %i %i %i %i [%i]", spellbonuses.Faith[0],spellbonuses.Faith[1],spellbonuses.Faith[2],spellbonuses.Faith[3], effectid);
		
		int i = effectid;
		if (i == -1)//Determines if checked as resource drain.
			i = GetSpellEffectIndex(spell_id, SE_NumHitsAmtFaith); //-1 = No effect found

		if (i == -1 || (effectid == -1 && spells[spell_id].base2[i] != -1))
			return; //Do not try a consume at spell casting finished if not used as resource.

		int slot = spellbonuses.Faith[1];
		if (slot >= 0 && spellbonuses.Faith[3] == buffs[slot].spellid && i >= 0){

			int _numhits = static_cast<int>(buffs[slot].numhits) + spells[spell_id].base[i];

			if (_numhits <= 0)
				_numhits = 0; //Min
			else if	(_numhits >= spellbonuses.Faith[2])
				_numhits = spellbonuses.Faith[2]; //Max

			buffs[slot].numhits = _numhits;

			CastToClient()->SendBuffNumHitPacket(buffs[slot], slot);
		}
	}
}

bool Mob::TryClericCastingConditions(uint16 spell_id)
{
	if (!spellbonuses.Faith[0])
		return true;

	int slot = spellbonuses.Faith[1];
	if (slot >= 0){

		int RequiredFaith = GetRequiredFaith(spell_id);
		if (RequiredFaith && buffs[slot].numhits < RequiredFaith){
			Message(MT_SpellFailure, "You have insufficient faith to use this ability.");
			return false;
		}
	}
	return true;
}

bool Mob::TryCustomCastingConditions(uint16 spell_id, uint16 target_id)
{
	//Note this method of interrupting casting does not work well for instant spells (cast time 150 is shortest)

	if (!TryEnchanterCastingConditions(spell_id)) //Why do i have this checked in 2 places.
		return false;

	if (!TryRangerCastingConditions(spell_id, target_id))
		return false;

	if (!TryClericCastingConditions(spell_id))
		return false;

	if (!TryLeapSECastingConditions(spell_id))
		return false;

	//Check Min RANGE since client does not auto stop it.
	if (MinCastingRange(spell_id, target_id))
		return false;

	//if (!SingleTargetSpellInAngle(spell_id, nullptr, target_id))//Undecided if want to check pre casting.
		//return false;

	return true;
}

bool Mob::TryCustomResourceConsume(uint16 spell_id)
{
	TryWizardEnduranceConsume(spell_id);
	TryEnchanterManaFocusConsume(spell_id);
	AdjustNumHitsFaith(spell_id, -1);
	return true;
}

//C!Misc - Functions still in development



void Mob::SpellGraphicTempPet(int type, uint16 spell_id, float aoerange, Mob* target)
{
	if (!IsValidSpell(spell_id) || !aoerange)
		return;

	if (spells[spell_id].GFX == -1) //Disable!
		return;

	/*Type
	1 = PBAE/Directional AE
	2 = Beam AE / Targeted Beam AE
	3 = Target Ring AOE
	4 = AOE Rain [Target = Beacon]
	*/

	int ROW_COUNT = 1;

	if (aoerange > 800)
		ROW_COUNT = 7;
	else if (aoerange <= 800 && aoerange > 550)
		ROW_COUNT = 6;
	else if (aoerange <= 550 && aoerange > 350)
		ROW_COUNT = 5;
	else if (aoerange <= 350 && aoerange > 200)
		ROW_COUNT = 4;
	if (aoerange <= 200 && aoerange > 100)
		ROW_COUNT = 3;
	else if (aoerange <= 100 && aoerange > 50)
		ROW_COUNT = 2;
	else if (aoerange <= 50)
		ROW_COUNT = 1;

	int column_distance = 0;
	float FIRST_ROW = 0; //Distance for beam first row.
	//Shout("DEBUG: ROW COUNT %i Mult %i", ROW_COUNT, GetGFXMultiplier(spell_id));
	ROW_COUNT = ROW_COUNT * GetGFXMultiplier(spell_id);

	if (type == 1)
		column_distance = static_cast<int>(aoerange)/ROW_COUNT;
	else if (type == 2){
		FIRST_ROW = 10.0f; //This may need to be modified based on size
		column_distance = static_cast<int>(aoerange - FIRST_ROW)/ROW_COUNT;
	}

	float origin_heading = GetHeading();
	if (target && type == 2)
		origin_heading = CalculateHeadingToTarget(target->GetX(), target->GetY());

	float origin_x = GetX();
	float origin_y = GetY();
	float origin_z = GetZ();

	if (type == 3){
		origin_x = GetTargetRingX();
		origin_y = GetTargetRingY();
		origin_z = GetTargetRingZ();
	}
	if (type == 4 && target && target->IsBeacon()){
		origin_x = target->GetX();
		origin_y = target->GetY();
		origin_z = target->GetZ();
	}

	//Shout("DEBUG: Start GFX: Column Distance %i ROW COUNT %i [AOE RANGE %i] [Heading %.2f]", column_distance, ROW_COUNT, static_cast<int>(aoerange), origin_heading);

	if (type == 1){ //PBAE / Directional
		for(int i = 1; i <= ROW_COUNT; i++){

			if (i == 1 && spells[spell_id].targettype == ST_Directional) //On first row, place a single graphic close to caster as apex of cone.
				SpawnSpellGraphicAOETempPet(type, spell_id, static_cast<float>(column_distance/3),-1,origin_heading, origin_x, origin_y); //WAS static_cast<float>(column_distance/2) - Best way to do this?

			SpawnSpellGraphicAOETempPet(type, spell_id, aoerange - (((ROW_COUNT) - i)*column_distance),i,origin_heading, origin_x, origin_y);
		}
	}
	else if (type == 2){ //Beam
		for(int i = 1; i <= ROW_COUNT; i++){

			if (i == 1)
				SpawnSpellGraphicBeamTempPet(type, spell_id, FIRST_ROW,-1,origin_heading,origin_x,origin_y);

			SpawnSpellGraphicBeamTempPet(type, spell_id, aoerange - (((ROW_COUNT) - i)*column_distance),i,origin_heading,origin_x,origin_y); 
		}
	}
	else if (type == 3 || type == 4){//Targert RING AOE or Location based attacks
		for(int i = 1; i <= ROW_COUNT; i++){

			if (i == 1){
				SpawnSpellGraphicSingleTempPetLocation(type, spell_id, aoerange, origin_x, origin_y, origin_z);
				if (aoerange < 30)
					return; //Keep it a simple one spawn for short range target ring AOE
			}
			
			SpawnSpellGraphicAOETempPet(type, spell_id, aoerange - (((ROW_COUNT) - i)*column_distance),i,origin_heading, origin_x, origin_y);
		}
	}
	return;
}

float Mob::GetSpacerAngle(float aoerange, float total_angle)
{
	//DevNote: All spell angles will be in intervals of 15 degrees
	//return total_angle / ((total_angle + 7.5f)/7.5f); Bad formula? - Can Delete at later time

	int standard_angle = static_cast<int>(total_angle) % 30; //If zero, then use 30 degree formulas
	if (standard_angle != 0)
		standard_angle = static_cast<int>(total_angle-1) % 15;//Check if a 15 degree interval which will be 16,46 ect
	//[16 = 352/8]

	if (standard_angle == 0){
		if (aoerange > 150)
			return 7.5f;
		if (aoerange <= 150 && aoerange > 100)
			return 7.5f;
		if (aoerange <= 100 && aoerange > 50)
			return 15.0f;
		if (aoerange <= 50 && aoerange > 0){
			if (total_angle >= 16)
				return 30.0f;
			else
				return 1.0f;
		}
	}
	else{
		Shout("DEBUG: Critical Development Error: INVALID SPELL ANGLE %.2f [AOE RANGE %.2f]", total_angle, aoerange);
	}
	
	return 0.0f;
}

void Mob::SpawnSpellGraphicAOETempPet(int type, uint16 spell_id, float aoerange, int row, float origin_heading, float origin_x, float origin_y)
{
	//Shout("START DEBUG: AOE RANGE %.2f [SIZE %i DURATIOn %i]", aoerange, GetGFXSize(spell_id), GetGFXDuration(spell_id));

	if (aoerange <= spells[spell_id].min_range)
		return;

	uint32 duration = GetGFXDuration(spell_id);
	uint32 gfx_npctype_id = GetGraphicNPCTYPEID(spell_id);
	uint16 gfx_spell_id = SPELL_UNKNOWN;
	if (type == 4)
		gfx_spell_id = GetAERainGFXSpellID(spell_id);
	else
		gfx_spell_id = GetGraphicSpellID(spell_id);

	float total_angle = 360.0f;

	if (spells[spell_id].targettype == ST_Directional)
		total_angle = spells[spell_id].directional_end * 2.0f;

	float angle_length = GetSpacerAngle(aoerange,total_angle);
	float start_angle = 0;

	int COLUMN_COUNT = 0;

	if (row == -1){ //Override for apex graphic right in front of caster.
		COLUMN_COUNT = 1;
		start_angle = origin_heading;
	}
	else {
		if (angle_length == 1.0f){//Need to revaluate this one
			COLUMN_COUNT = 1;
			start_angle = origin_heading;
		}
		else {
			COLUMN_COUNT = total_angle/angle_length + 1;
			start_angle = origin_heading - GetHeadingChangeFromAngle(spells[spell_id].directional_end);
			start_angle = FixHeadingAngle(start_angle);
			angle_length =  GetHeadingChangeFromAngle(angle_length); //Convert 
		}
	}

	for(int i = 0; i < COLUMN_COUNT; i++){

		float dX = 0.0f;
		float dY = 0.0f;
		float dZ = 0.0f;
		
		CalcDestFromHeading(start_angle, aoerange - 5.0f, 0, origin_x, origin_y, dX, dY, dZ); // -5 distance so graphic is better[Adjust?]

		NPC* temppet = nullptr;
		temppet = TypesTemporaryPetsGFX(gfx_npctype_id, "#",duration, dX, dY,dZ + 5.0f, spell_id); //Spawn pet
	
		if (temppet){
			if (spells[spell_id].npc_no_los || CheckLosFN(temppet))
				SendSpellAnimGFX(temppet->GetID(), gfx_spell_id, aoerange);

			start_angle = FixHeadingAngle((start_angle + angle_length));
		}
		else
			Shout("DEBUG:: Critical error SPELL %i no temppet (NPCTYPE ID %i) Found in database", spell_id, gfx_npctype_id );
	}
}

void Mob::SpawnSpellGraphicBeamTempPet(int type, uint16 spell_id, float aoerange, int row, float origin_heading,float origin_x, float origin_y)
{
	//Shout("DEBUG: START BEAM DEBUG: Height %.2f Width %.2f [GFX Size %i NPCID %i]", aoerange, spells[spell_id].aoerange, GetGFXSize(spell_id),GetGraphicNPCTYPEID(spell_id));

	uint32 duration = GetGFXDuration(spell_id);
	uint32 gfx_npctype_id = GetGraphicNPCTYPEID(spell_id);
	uint16 gfx_spell_id = GetGraphicSpellID(spell_id);

	float length = aoerange;
	float width = spells[spell_id].aoerange;

	float width_mod = 5.0f; //Fine if using intervals of 10
	//width = width - (width_mod*2); //If decide not to use 5, will need to modify this.
	
	float spacer_length = 10.0f; //Will need algorithm
	int COLUMN_COUNT = 0;
	COLUMN_COUNT = width/spacer_length;

	//This will center the graphic line for narrow AOE
	if (width < 20 || COLUMN_COUNT == 1){
		COLUMN_COUNT = 1;
		width = spacer_length;
		width_mod = 5.0;
	}


	float packet_range = length;
		if (width > length)
			packet_range = width;

	//Shout("DEBUG: START BEAM DEBUG: COLUMN_COUNT  %i", COLUMN_COUNT);
	
	float start_dX = 0.0f;
	float start_dY = 0.0f;
	float start_dZ = 0.0f;

	//1: Move center point straight aheada specific amount from caster.
	CalcDestFromHeading(origin_heading, length - 4.0f, 0, origin_x, origin_y, start_dX, start_dY, start_dZ);
	//2: Move starting point to far left of rectangle width.
	CalcDestFromHeading(FixHeadingAngle(origin_heading - 64.0f), (width/2.0f) - width_mod, 0, start_dX, start_dY, start_dX, start_dY, start_dZ);
	//3: Loop then spawns from far left to right.

	float distance = 0.0f;

	for(int i = 0; i < COLUMN_COUNT; i++){

		float dX = 0.0f;
		float dY = 0.0f;
		float dZ = 0.0f;
		
		if (i == 0){
			dX = start_dX;
			dY = start_dY;
			dZ = start_dZ;
		}
		else
			CalcDestFromHeading(FixHeadingAngle(origin_heading + 64.0f), distance, 0, start_dX, start_dY, dX, dY, dZ);

		NPC* temppet = nullptr;
		temppet = TypesTemporaryPetsGFX(gfx_npctype_id, "#",duration, dX, dY,dZ + 5.0f, spell_id); //Spawn pet
	
		if (temppet){
			if (spells[spell_id].npc_no_los || CheckLosFN(temppet))
				SendSpellAnimGFX(temppet->GetID(), gfx_spell_id, packet_range);

			distance = distance + spacer_length;
			//Shout("DEBUG: START BEAM DEBUG: New DISTANCE  %.2f [C Count %i] [Spellid %i] [Z %.2f]", distance, i, gfx_spell_id, dZ);
		}
		else
			Shout("DEBUG:: Critical error SPELL %i no temppet (NPCTYPE ID %i) Found in database", spell_id, gfx_npctype_id );
	}
}

void Mob::SpawnSpellGraphicSingleTempPetLocation(int type, uint16 spell_id, float aoerange, float locX, float locY, float locZ)
{
	uint32 duration = GetGFXDuration(spell_id);
	uint32 gfx_npctype_id = GetGraphicNPCTYPEID(spell_id);
	uint16 gfx_spell_id = SPELL_UNKNOWN;
	if (type == 4)
		gfx_spell_id = GetAERainGFXSpellID(spell_id);
	else
		gfx_spell_id = GetGraphicSpellID(spell_id);

	NPC* temppet = nullptr;

	temppet = TypesTemporaryPetsGFX(gfx_npctype_id, "#",duration, locX, locY,locZ, spell_id); //Spawn pet
	
	if (temppet){
		SendSpellAnimGFX(temppet->GetID(), gfx_spell_id, spells[spell_id].range + 10);
	}
}

void Mob::SpawnProjectileGraphicArcheryTempPet(int type, uint16 spell_id, float aoerange, int row, float origin_heading, float origin_x, float origin_y, float origin_z)
{
	if (!IsEffectInSpell(spell_id,SE_AttackArchery))
		return;

	uint16 gfx_spell_id = 1001001;

	int ammo_slot = MainAmmo;
	const ItemInst* Ammo = CastToClient()->m_inv[MainAmmo];
	if (!Ammo || !Ammo->IsType(ItemClassCommon)) 
		return;
	const Item_Struct* AmmoItem = Ammo->GetItem();
	if (!AmmoItem)
		return;

	float dX = 0.0f;
	float dY = 0.0f;
	float dZ = 0.0f;
	uint32 duration = 5000;

	if (type == 2) {//Beam
		GetFurthestLocationLOS(origin_heading, 5, aoerange, dX, dY, dZ, true,origin_x, origin_y, origin_z);
		NPC* temppet = nullptr;
		temppet = TypesTemporaryPetsGFX(gfx_spell_id, "#",duration, dX, dY,dZ + 5.0f, spell_id); //Spawn pet
		if (temppet)
			SendItemAnimation(temppet, AmmoItem, SkillArchery);
	}

	if (type == 1){//AOE/Directional

		float total_angle = 360.0f;

		if (spells[spell_id].targettype == ST_Directional){
			if (spells[spell_id].directional_start > spells[spell_id].directional_end)//Centered
				total_angle = (spells[spell_id].directional_end + 360) - (spells[spell_id].directional_start);
			else if (spells[spell_id].directional_start < spells[spell_id].directional_end)//Off center
				total_angle = spells[spell_id].directional_end - spells[spell_id].directional_start;
		}
		
		float angle_length = 7.5f;
		float start_angle = 0;

		int COLUMN_COUNT = 0;

		if (spells[spell_id].aemaxtargets > 1){
			COLUMN_COUNT = spells[spell_id].aemaxtargets;
			angle_length = (total_angle/static_cast<float>(spells[spell_id].aemaxtargets - 1));
		}
		else
			COLUMN_COUNT = total_angle/angle_length + 1;
		
		start_angle = origin_heading - GetHeadingChangeFromAngle(spells[spell_id].directional_end);
		start_angle = FixHeadingAngle(start_angle);
		angle_length =  GetHeadingChangeFromAngle(angle_length); //Convert 

		float single_shot_mod = 0; //Centers the arrow.

		if (total_angle == 7.5){
			COLUMN_COUNT = 1;
			single_shot_mod = total_angle/2;
		}

		//Shout("DEBUG: total %.2f COUNT %i [MAX %i] Angle Lenth %.2f", total_angle, COLUMN_COUNT, spells[spell_id].aemaxtargets, angle_length);

		for(int i = 0; i < COLUMN_COUNT; i++){

			float dX = 0.0f;
			float dY = 0.0f;
			float dZ = 0.0f;
		
			GetFurthestLocationLOS(start_angle + (single_shot_mod), 5, aoerange, dX, dY, dZ, true,origin_x, origin_y, origin_z);
			NPC* temppet = nullptr;
			temppet = TypesTemporaryPetsGFX(gfx_spell_id, "#",duration, dX, dY,dZ + 5.0f, spell_id); //Spawn pet
			if (temppet){
				SendItemAnimation(temppet, AmmoItem, SkillArchery);
				start_angle = FixHeadingAngle((start_angle + angle_length));
			}
			else
				Shout("DEBUG:: Critical error SPELL %i no temppet (NPCTYPE ID %i) Found in database", spell_id, 1001000 );
		}
	}
}

void EntityList::AEBeamDirectional(Mob *caster, uint16 spell_id, int16 resist_adjust, bool FromTarget, Mob* target)
{
	//Optmized Beam Directional Function.
	if (!caster || (FromTarget && !target))
		return;

	if (!FromTarget)
		caster->SpellGraphicTempPet(2, spell_id, spells[spell_id].range);
	else
		caster->SpellGraphicTempPet(2, spell_id, caster->CalculateDistance(target->GetX(), target->GetY(), target->GetZ()), target);

	Mob *curmob;

	float ae_width = spells[spell_id].aoerange; //This is the width of the AE that will hit targets.
	float ae_length = spells[spell_id].range; //This is total area checked for targets.
	int maxtargets = spells[spell_id].aemaxtargets;
	
	float ae_height = ae_length;
		if (ae_width > ae_length)
			ae_height = ae_width;

	bool bad = IsDetrimentalSpell(spell_id);	
	bool target_client_only = false;
	bool target_npc_only = false;

	bool target_found = false;
	std::list<Mob*> targets_in_rectangle;

	if (caster->IsClient() || caster->IsPetOwnerClient()){ //When client/pet is caster
		if (!bad)
			target_client_only = true;
		else
			target_npc_only = true;
	}
	else //When NPC is caster
		target_client_only = true;

	if (!bad && DirectionalAffectCaster(spell_id))//Affect caster
		caster->SpellOnTarget(spell_id,caster, false, true, resist_adjust);

	//Start - Calculate cooridinates of rectangle
	float origin_heading = caster->GetHeading();
	
	float target_heading = 0.0f;
	if (FromTarget){
		origin_heading = caster->CalculateHeadingToTarget(target->GetX(), target->GetY());
		target_heading = target->CalculateHeadingToTarget(caster->GetX(), caster->GetY());
	}

	float origin_x = caster->GetX();
	float origin_y = caster->GetY();
	float origin_z = caster->GetZ();

	float aX = 0.0f;
	float aY = 0.0f;
	float aZ = 0.0f;
	caster->CalcDestFromHeading(caster->FixHeadingAngle(origin_heading - 64.0f), (spells[spell_id].aoerange/2.0f), 0, origin_x, origin_y, aX, aY, aZ);

	float bX = 0.0f;
	float bY = 0.0f;
	float bZ = 0.0f;
	caster->CalcDestFromHeading(caster->FixHeadingAngle(origin_heading + 64.0f), (spells[spell_id].aoerange/2.0f), 0, origin_x, origin_y, bX, bY, bZ);

	float dX = 0.0f;
	float dY = 0.0f;
	float dZ = 0.0f;

	if (FromTarget)
		caster->CalcDestFromHeading(caster->FixHeadingAngle(target_heading - 64.0f), (spells[spell_id].aoerange/2.0f), 0, target->GetX(), target->GetY(), dX, dY, dZ);
	else
		caster->CalcDestFromHeading(caster->FixHeadingAngle(origin_heading), (spells[spell_id].range), 0, aX, aY, dX, dY, dZ);
	//End

	for (auto it = mob_list.begin(); it != mob_list.end(); ++it) {
		curmob = it->second;
		if (!curmob)
			continue;

		if (curmob->IsClient() && !curmob->CastToClient()->ClientFinishedLoading())
			continue;

		if (curmob->GetUtilityTempPetSpellID())  //Skip Projectile and SpellGFX temp pets.
			continue;

		if (target_client_only && (curmob->IsNPC() && !curmob->IsPetOwnerClient()))//Exclude NPC that are not pets
			continue;

		if (target_npc_only && (curmob->IsClient() || curmob->IsPetOwnerClient()))//Exclude Clients and Pets
			continue;

		if (!curmob->InDirectionalAOEArea(caster, spells[spell_id].min_range, (ae_length + ae_width), ae_height))//Is in Broad AE range.
			continue;
		
		if (curmob->BehindMob(caster, curmob->GetX(),curmob->GetY())) //Fail safe (Do we need this with the improved formula?)
			continue;

		if (!curmob->InRectangleByCoordinates(aX,  aY, bX, bY, dX, dY)) //BEAM CHECK
			continue;

		if(spells[spell_id].npc_no_los || caster->CheckLosFN((curmob))) {
			curmob->CalcSpellPowerDistanceMod(spell_id, 0, caster);
			target_found = true;
			if (maxtargets) 
				targets_in_rectangle.push_back(curmob);
			else
				caster->SpellOnTarget(spell_id, curmob, false, true, resist_adjust);
		}
	}

	if (maxtargets)
		caster->CastOnClosestTarget(spell_id, resist_adjust, maxtargets, targets_in_rectangle);

	if (!target_found){
		caster->DirectionalFailMessage(spell_id);//Planning on disabling this 
		caster->SpawnProjectileGraphicArcheryTempPet(2, spell_id, ae_length, 0, origin_heading, origin_x, origin_y, origin_z);
	}
	
	return;
}

void EntityList::AEConeDirectional(Mob *caster, uint16 spell_id, int16 resist_adjust)
{
	//Optmized Cone Directional Function.
	if (!caster)
		return;

	caster->SpellGraphicTempPet(1, spell_id, spells[spell_id].aoerange);

	Mob *curmob;

	float aoerange = spells[spell_id].aoerange;
	float min_aoerange = spells[spell_id].min_range;

	float angle_start = spells[spell_id].directional_start + (caster->GetHeading() * 360.0f / 256.0f);
	float angle_end = spells[spell_id].directional_end + (caster->GetHeading() * 360.0f / 256.0f);

	while (angle_start > 360.0f)
		angle_start -= 360.0f;

	while (angle_end > 360.0f)
		angle_end -= 360.0f;

	float origin_x = caster->GetX();
	float origin_y = caster->GetY();
	float origin_z = caster->GetZ();
	float origin_heading = caster->GetHeading();

	if (caster->IsClient() && IsRangeSpellEffect(spell_id)){
		min_aoerange = 0.0f; //Calculated in the archery fire as a miss.
		if (aoerange == caster->UseRangeFromRangedWpn())//Flags aoe range to use weapon ranage.
			aoerange = caster->CastToClient()->GetArcheryRange(nullptr);
	}

	int maxtargets = spells[spell_id].aemaxtargets;
	
	bool bad = IsDetrimentalSpell(spell_id);	
	bool target_client_only = false;
	bool target_npc_only = false;

	bool target_found = false;
	std::list<Mob*> targets_in_cone;

	if (caster->IsClient() || caster->IsPetOwnerClient()){ //When client/pet is caster
		if (!bad)
			target_client_only = true;
		else
			target_npc_only = true;
	}
	else //When NPC is caster
		target_client_only = true;

	if (!bad && DirectionalAffectCaster(spell_id))//Affect caster
		caster->SpellOnTarget(spell_id,caster, false, true, resist_adjust);

	for (auto it = mob_list.begin(); it != mob_list.end(); ++it) {
		curmob = it->second;
		if (!curmob)
			continue;

		if (curmob->IsClient() && !curmob->CastToClient()->ClientFinishedLoading())
			continue;

		if (curmob->GetUtilityTempPetSpellID())  //Skip Projectile and SpellGFX temp pets.
			continue;

		if (target_client_only && (curmob->IsNPC() && !curmob->IsPetOwnerClient()))//Exclude NPC that are not pets
			continue;

		if (target_npc_only && (curmob->IsClient() || curmob->IsPetOwnerClient()))//Exclude Clients and Pets
			continue;

		if (!curmob->InDirectionalAOEArea(caster,  min_aoerange, aoerange, aoerange / 2))//Is in Broad AE range.
			continue;
		
		float heading_to_target = (caster->CalculateHeadingToTarget(curmob->GetX(), curmob->GetY()) * 360.0f / 256.0f);
		if (!curmob->InCone(heading_to_target, angle_start, angle_end)) //CONE CHECK
			continue;

		if(spells[spell_id].npc_no_los || caster->CheckLosFN((curmob))) {
			curmob->CalcSpellPowerDistanceMod(spell_id, 0, caster);
			target_found = true;
			if (maxtargets) 
				targets_in_cone.push_back(curmob);
			else
				caster->SpellOnTarget(spell_id, curmob, false, true, resist_adjust);
		}
	}

	if (maxtargets)
		caster->CastOnClosestTarget(spell_id, resist_adjust, maxtargets, targets_in_cone);

	if (!target_found){
		caster->DirectionalFailMessage(spell_id);
		caster->SpawnProjectileGraphicArcheryTempPet(1, spell_id, aoerange, 0, origin_heading, origin_x, origin_y, origin_z);
	}
	
	return;
}

bool Mob::InDirectionalAOEArea(Mob *start, float min_radius, float radius, float height)
{
	float x_diff = GetX() - start->GetX();
	float y_diff = GetY() - start->GetY();
	float z_diff = GetZ() - start->GetZ();

	x_diff *= x_diff;
	y_diff *= y_diff;
	z_diff *= z_diff;

	if ((x_diff + y_diff) <= (radius * radius) && (x_diff + y_diff) >= (min_radius * min_radius)){
		if(z_diff <= (height * height))
			return true;
	}

	return false;
}

bool Mob::InRectangle(uint16 spell_id, float target_x, float target_y, float origin_heading)
{   //Not used in any functions [SEE InRectangleByCoordinates], but useful for testing graphic displays
	float mx = target_x;
	float my = target_y;

	float aX = 0.0f;
	float aY = 0.0f;
	float aZ = 0.0f;
	CalcDestFromHeading(FixHeadingAngle(origin_heading - 64.0f), (spells[spell_id].aoerange/2.0f), 0, GetX(), GetY(), aX, aY, aZ);

	float bX = 0.0f;
	float bY = 0.0f;
	float bZ = 0.0f;
	CalcDestFromHeading(FixHeadingAngle(origin_heading + 64.0f), (spells[spell_id].aoerange/2.0f), 0, GetX(), GetY(), bX, bY, bZ);

	float dX = 0.0f;
	float dY = 0.0f;
	float dZ = 0.0f;
	CalcDestFromHeading(FixHeadingAngle(origin_heading), (spells[spell_id].range), 0, aX, aY, dX, dY, dZ);

	/*FOR DEBUG DISPLAY OF RECTANGLE
	float cX = 0.0f;
	float cY = 0.0f;
	float cZ = 0.0f;
	CalcDestFromHeading(FixHeadingAngle(origin_heading), (spells[spell_id].range), 0, bX, bY, cX, cY, cZ);

	TypesTemporaryPetsGFX(GetSpellGraphicPetDBID(), "#",15, aX, aY,aZ, spell_id);
	TypesTemporaryPetsGFX(GetSpellGraphicPetDBID(), "#",15, bX, bY,bZ, spell_id);
	TypesTemporaryPetsGFX(GetSpellGraphicPetDBID(), "#",15, cX, cY,cZ, spell_id);
	TypesTemporaryPetsGFX(GetSpellGraphicPetDBID(), "#",15, dX, dY,dZ, spell_id);
	*/

	float bax = bX - aX;
	float bay = bY - aY;
	float dax = dX - aX;
	float day = dY - aY;

	if ((mx - aX) * bax + (my - aY) * bay < 0.0) return false;
	if ((mx - bX) * bax + (my - bY) * bay > 0.0) return false;
	if ((mx - aX) * dax + (my - aY) * day < 0.0) return false;
	if ((mx - dX) * dax + (my - dY) * day > 0.0) return false;

	return true;
}

bool Mob::InRectangleByCoordinates(float aX, float aY, float bX, float bY, float dX, float dY)
{
	float mx = GetX();
	float my = GetY();

	float bax = bX - aX;
	float bay = bY - aY;
	float dax = dX - aX;
	float day = dY - aY;

	if ((mx - aX) * bax + (my - aY) * bay < 0.0) return false;
	if ((mx - bX) * bax + (my - bY) * bay > 0.0) return false;
	if ((mx - aX) * dax + (my - aY) * day < 0.0) return false;
	if ((mx - dX) * dax + (my - dY) * day > 0.0) return false;

	return true;

}

bool Mob::InCone(float heading_to_target, float angle_start, float angle_end)
{
	//SEE [Mob::InAngle for more broad usage]
	while(heading_to_target < 0.0f)
		heading_to_target += 360.0f;

	while(heading_to_target > 360.0f)
		heading_to_target -= 360.0f;

	if(angle_start > angle_end) {
		if((heading_to_target >= angle_start && heading_to_target <= 360.0f) ||	(heading_to_target >= 0.0f && heading_to_target <= angle_end))
			return true;
	}
	else if(heading_to_target >= angle_start && heading_to_target <= angle_end)
		return true;

	return false;

}

NPC* Mob::TypesTemporaryPetsGFX(uint32 typesid, const char *name_override, uint32 duration_override, float dX, float dY, float dZ, uint16 spell_id) {

	//Used to optimize temp pets used for spell graphics, this spawns them at the location above.

	AA_SwarmPet pet;
	pet.count = 1;
	pet.duration = 1;

	pet.npc_id = typesid;

	NPCType *made_npc = nullptr;

	const NPCType *npc_type = database.LoadNPCTypesData(typesid);
	if(npc_type == nullptr) {
		//log write
		Log.Out(Logs::General, Logs::Error, "[TypesTemporaryPetsGFX] Unknown npc type for swarm pet type id: %d", typesid);
		Message(0,"[TypesTemporaryPetsGFX] Unable to find pet!");
		return nullptr;
	}

	if(name_override != nullptr) {
		//we have to make a custom NPC type for this name change
		made_npc = new NPCType;
		memcpy(made_npc, npc_type, sizeof(NPCType));
		strcpy(made_npc->name, name_override);
		npc_type = made_npc;
	}

	int summon_count = 0;
	summon_count = pet.count;

	if (summon_count) {//This is always 1
		int pet_duration = pet.duration;
		if(duration_override > 0)
			pet_duration = duration_override;

		//this is a little messy, but the only way to do it right
		//it would be possible to optimize out this copy for the last pet, but oh well
		NPCType *npc_dup = nullptr;
		if(made_npc != nullptr) {
			npc_dup = new NPCType;
			memcpy(npc_dup, made_npc, sizeof(NPCType));
		}

		NPC* npca = new NPC(
				(npc_dup!=nullptr)?npc_dup:npc_type,	//make sure we give the NPC the correct data pointer
				0,
				glm::vec4(dX, dY,dZ, 0.0f),
				FlyMode3);


		if(!npca->GetSwarmInfo()){
			AA_SwarmPetInfo* nSI = new AA_SwarmPetInfo;
			npca->SetSwarmInfo(nSI);
			npca->GetSwarmInfo()->duration = new Timer(pet_duration*1000);
		}
		else{
			npca->GetSwarmInfo()->duration->Start(pet_duration*1000);
		}

		//removing this prevents the pet from attacking
		npca->GetSwarmInfo()->owner_id = GetID();

		//we allocated a new NPC type object, give the NPC ownership of that memory
		if(npc_dup != nullptr)
			npca->GiveNPCTypeData(npc_dup);

		entity_list.AddNPC(npca, true, true);

		npca->SetUtilityTempPetSpellID(spell_id);
		
		delete made_npc;
		return npca;
	}

	return nullptr;
}

void Mob::SendSpellAnimGFX(uint16 targetid, uint16 spell_id, float aoerange)
{
	if (!targetid)
		return;

	if (aoerange < 200.0f)
		aoerange = 200.0f;

	EQApplicationPacket app(OP_Action, sizeof(Action_Struct));
	Action_Struct* a = (Action_Struct*)app.pBuffer;
	a->target = targetid;
	a->source = this->GetID();
	a->type = 231;
	a->spell = spell_id;
	a->sequence = 231;

	app.priority = 1;
	entity_list.QueueCloseClients(this, &app, false, aoerange);
}

int32 Mob::CalcSpellPowerAmtClients(uint16 spell_id, int effectid,Mob* caster)
{
	if (!IsValidSpell(spell_id) || !caster)
		return 0;

	int base = 0;//Percent Modifier
	int limit = 0; //Distance to check
	int max = 0; //Max amount of clients to draw from [+ FromTarget, - FromCaster]
	int count = 0;

	bool FromTargetLoc = true;

	for (int i = 0; i <= EFFECT_COUNT; i++)
	{
		if (spells[spell_id].effectid[i] == SE_SpellPowerAmtClients){

			if (spells[spell_id].base[i]){

				base = spells[spell_id].base[i];
				limit = spells[spell_id].base2[i];
				max = spells[spell_id].max[i];
			}
		}
	}

	//Shout("DEBUG: CalcSpellPowerAmtClients %i %i %i", base, limit,max);

	if (max < 0){
		FromTargetLoc = false;
		max *= -1;
	}

	if (base){

		if (FromTargetLoc)
			count = entity_list.CountCloseClients(this, caster, limit, max);
		else
			count = entity_list.CountCloseClients(caster, caster, limit, max);
	
	}

	if (count > max)
		count = max;

	//Shout("DEBUG: CalcSpellPowerAmtClients COUNT %i [BOOL %i]", count, FromTargetLoc);
	return count * base;
}

int EntityList::CountCloseClients(Mob *target, Mob *caster, float dist, int max_count)
{
	float dist2 = dist * dist;

	int count = 0;

	auto it = client_list.begin();
	while (it != client_list.end()) {
		Client *ent = it->second;

		if (ent && ent->Connected() && (ent->GetID() != caster->GetID())) {
			if (DistanceSquared(ent->GetPosition(), target->GetPosition()) <= dist2) {
				count++;

				if (max_count && (count > max_count))
					return max_count;
			}
		}
		++it;
	}

	return count;
}

int32 Mob::CalcCustomManaRequired(int32 mana_cost, uint16 spell_id)
{
	if (!spells[spell_id].mana_ratio) //Dmg/Mana = ratio
		return mana_cost;

	/*Type
	1= True Nuke/Heals determined by HP/Mana ratio
	2= All other effects determined by level * ratio/100
	*/
	
	int type = GetManaRatioType(spell_id);
	int value = 0;
	int new_mana_cost = mana_cost;

	if (type == 1){
		value = CalcSpellEffectValue(spell_id, 0, GetLevel());

		if (value < 0)
			value *= -1;

		new_mana_cost = (value * 100 / spells[spell_id].mana_ratio);

		if (spells[spell_id].cast_from_crouch)
			new_mana_cost *= 5;
		else if (spells[spell_id].min_dist_mod > 1)
			new_mana_cost *= spells[spell_id].min_dist_mod;
		else if (spells[spell_id].max_dist_mod > 1)
			new_mana_cost *= spells[spell_id].max_dist_mod;
	}
	else{
		new_mana_cost = GetLevel() * spells[spell_id].mana_ratio/100;
	}
		
	if (new_mana_cost < 0)
		new_mana_cost = mana_cost;
		
	return new_mana_cost;
}

int32 Mob::CalcCustomManaUsed(uint16 spell_id, int32 mana_used)
{
	//Shout("New Mana used = %i %i [DAMGE %i]", mana_used, GetCastFromCrouchInterval(),GetCastingFormulaValue());
	//Shout("TEST RATIO MOD %.2f", CalcDistributionModiferFloat(GetCastFromCrouchInterval(), 1, 5, 100, spells[spell_id].mana_ratio));
	
	if (!spells[spell_id].mana_ratio)
		return mana_used;

	int new_mana_used = mana_used;

	if (spells[spell_id].cast_from_crouch){

		int base_mana = mana_used / 5;

		int penalty_ratio = CalcDistributionModiferFloat(GetCastFromCrouchInterval(), 1, 5, 100, spells[spell_id].mana_ratio);
		int penalty_mod = (((GetScaledBaseEffectValue())*100/penalty_ratio)*100)/base_mana;

		/*
		int penalty_mod = 100; //In theory you should do a distrubtion from RATIO to 1 (Ie 300 - 100)
		if (GetCastFromCrouchInterval() == 1) { penalty_mod = 300; } //Ratio 1.0 x 100 ect penalty_ratio
		if (GetCastFromCrouchInterval() == 2) { penalty_mod = 200; } //Ratio 1.5
		if (GetCastFromCrouchInterval() == 3) { penalty_mod = 150; } //Ratio 2.0
		if (GetCastFromCrouchInterval() == 4) { penalty_mod = 120; } //Ratio 2.5 [ ((Damage/Ratio)/mana_used)/interval)]
		*/

		if (GetCastFromCrouchInterval())
			new_mana_used = ((base_mana * penalty_mod)/100) * GetCastFromCrouchInterval();
		else if (GetCastFromCrouchIntervalProj())
			new_mana_used = base_mana * GetCastFromCrouchIntervalProj();
	
	
		//Shout("New Mana used = OLD %i [NEW %i] %i [PENALTY %i]", mana_used, new_mana_used, GetCastFromCrouchInterval(),penalty_mod);
	}

	if (new_mana_used > 0)
		return new_mana_used;
	else
		return mana_used;
}

bool Client::IsPartyMember(Mob* other)
{
	if (!other || !other->IsClient())
		return false;

	if ((HasGroup() && other->CastToClient()->HasGroup()) && (GetGroup() == other->CastToClient()->GetGroup()))
		return true;

	if ((HasRaid() && other->CastToClient()->HasRaid()) && (GetRaid() == other->CastToClient()->GetRaid()))
		return true;

	return false;
}

void EntityList::ApplyEffectToTargetsOnTarget(Mob *caster, Mob *center, uint16 spell_id, float range)
{ 
	if (!IsValidSpell(spell_id) || !center || !caster)
		return;

	Mob *curmob;
	float dist = range;
	float dist2 = dist * dist;
	float dist_targ = 0;

	bool exclude_clients = false;
	if (caster->IsClient())
		exclude_clients = true;

	for (auto it = mob_list.begin(); it != mob_list.end(); ++it) {
		curmob = it->second;
		if (!curmob)
			continue;
		if (curmob->IsClient() && !curmob->CastToClient()->ClientFinishedLoading())
			continue;
		if (exclude_clients && (curmob->IsClient() || curmob->IsPetOwnerClient()))
			continue;
		if ((curmob == center) || (curmob == caster) || curmob->GetUtilityTempPetSpellID())
			continue;
		
		if (range){//If no range, check all in zone.
			dist_targ = DistanceSquared(curmob->GetPosition(), center->GetPosition());
			
			if (dist_targ > dist2)
				continue;
		}
		
		Mob* target = curmob->GetTarget();

		if (target && (target->GetID() == center->GetID()))
			caster->SpellOnTarget(spell_id, curmob, false, true, spells[spell_id].ResistDiff);
	}
}

int32 Mob::CalcSpellPowerTargetPctHP(uint16 spell_id, Mob* caster){

	if (!IsValidSpell(spell_id) || !caster)
		return 0;

	float ratio_modifier = 0;
	float dmod = 0;
	
	for (int i = 0; i <= EFFECT_COUNT; i++)
	{
		if (spells[spell_id].effectid[i] == SE_SpellPowerTargetPctHP){

			if (spells[spell_id].base[i]){

				int ratio = GetHPRatio();
				float min_mod = static_cast<float>(spells[spell_id].base[i]);
				float max_mod = static_cast<float>(spells[spell_id].base2[i]);
				float ratio_below = static_cast<float>(spells[spell_id].max[i]); //HP Pct that you must be below for modifier to take effect
				ratio_modifier = ratio_below - static_cast<float>(ratio);

				if (ratio_modifier > 0){
					dmod = CalcDistributionModiferFloat(ratio_modifier, 1.0f, ratio_below, min_mod, max_mod);
					Shout("dmod %.2f", dmod);
					
				}
		
				return static_cast<int32>(dmod);
			}
		}
	}

	return 0;
}

bool Client::EnduranceResourceCheck(int32 cost, bool FromArchery)
{
	if(GetEndurance() < cost) {
		Message(11, "You are too fatigued to use this skill right now.");
		
		if (FromArchery)
			SetEndurance(0);
		else
			SetEndurance(GetEndurance() + 1);

		return false;
	}
	
	SetEndurance(GetEndurance() - cost);
	return true;
}

void Client::Fling(Mob* target)
{

		EQApplicationPacket* outapp_push = new EQApplicationPacket(OP_Fling, sizeof(fling_struct));
		fling_struct* spu = (fling_struct*)outapp_push->pBuffer;

		spu->unk1						= 1;//Collision
		spu->travel_time				= 600;
		spu->unk3						= 1;
		spu->disable_fall_damage		= 1;
		spu->speed_z					= 10.0f;
		spu->new_y						= target->GetY();
		spu->new_x						= target->GetX();
		spu->new_z						= target->GetZ();
		outapp_push->priority = 5;
		CastToClient()->QueuePacket(outapp_push, false);
		
		//entity_list.QueueClients(this, outapp_push, true);
		//if(IsClient())
			//CastToClient()->FastQueuePacket(&outapp_push);

		Shout("Test Fling OFF");

}

void Mob::SendAppearanceEffectTest(uint32 parm1, uint32 avalue, uint32 bvalue, Client *specific_target){

	//Set apperance effect on NPC that will fade when the NPC dies. (Use original function if want perma effects)
	Shout("%i %i %i", parm1, avalue, bvalue);
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_LevelAppearance, sizeof(LevelAppearance_Struct));
	LevelAppearance_Struct* la = (LevelAppearance_Struct*)outapp->pBuffer;
	la->spawn_id = GetID();
	la->parm1 = parm1;
	la->parm2 = 0;
	la->parm3 = 0;
	la->parm4 = 0;
	la->parm5 = 0;
	// Note that setting the b values to 0 will disable the related effect from the corresponding parameter.
	// Setting the a value appears to have no affect at all.s
	la->value1a = avalue;
	la->value1b = bvalue;
	la->value2a = 2;
	la->value2b = 0;
	la->value3a = 2;
	la->value3b = 0;
	la->value4a = 2;
	la->value4b = 0;
	la->value5a = 2;
	la->value5b = 0;
	if(specific_target == nullptr) {
		entity_list.QueueClients(this,outapp);
	}
	else if (specific_target->IsClient()) {
		specific_target->CastToClient()->QueuePacket(outapp, false);
	}
	safe_delete(outapp);
}

void Client::SendActionPacket(uint16 targetid, uint8 type, uint16 spell_id, uint32 seq, uint16 unknown16, uint32 unknown18, uint32 unknown23,uint8 unknown29, uint8 buff_unknown)
{	
	//THIS IS A TEST FUNCTION USE SendSpellAnim(targetid,spell_id)
	if (!targetid)
		return;

	Shout("[%i] Type %i Spell id %i Seq %i", targetid, type, spell_id, seq);
	Shout("u16 [%i] u18 [%i] u23 [%i] u29 [%i] ubuff [%i]", unknown16, unknown18, unknown23, unknown29, buff_unknown);
	EQApplicationPacket app(OP_Action, sizeof(Action_Struct));
	Action_Struct* a = (Action_Struct*)app.pBuffer;
	a->target = targetid;
	a->source = this->GetID();
	a->type = type;
	a->spell = spell_id;
	a->sequence = type;

	a->unknown16 = unknown16;
	a->unknown18 = unknown18;
	a->unknown23 = unknown23;
	a->unknown29 = unknown29;
	a->buff_unknown = buff_unknown;

	app.priority = 1;
	entity_list.QueueCloseClients(this, &app);
}

void Client::PopupUI()
{	
	if (!HasSpellAwareness())
		return;

	//Shout("Check interval ");

	//const char *WindowTitle = "Bot Tracking Window";

	std::string WT;

	// Define the types of page breaks we need
	std::string indP = "&nbsp;";
	std::string indS = "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;";
	std::string indM = "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;";
	std::string indL = "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;";
	std::string div = " | ";

	std::string color_red = "<c \"#993333\">";
	std::string color_green = "<c \"#33FF99\">";
	std::string heroic_color = "<c \"#d6b228\">";
	std::string color_yellow = "<c \#FFFF00\">"; 


	//Used colors
	std::string bright_green = "<c \"#7CFC00\">";
	std::string bright_red = "<c \"#FF0000\">";
	std::string color_gold = "<c \"#FFFF66\">";
	std::string color_grey = "<c \"#808080\">"; 
	std::string color_white = "<c \#FFFFFF\">"; 
	std::string color_blue = "<c \"#9999FF\">";

	/*
	$Text=~ s/\{y\}/<c \"#CCFF33\">/g;
	$Text=~ s/\{lb\}/<c \"#00FFFF\">/g;
	$Text=~ s/\{r}/<c \"#FF0000\">/g;
	$Text=~ s/\{g}/<c \"#00FF00\">/g;
	$Text=~ s/\{gold}/<c \"#FFFF66\">/g;
	$Text=~ s/\{orange}/<c \"#FFA500\">/g;
	$Text=~ s/\{gray}/<c \"#808080\">/g;
	$Text=~ s/\{tan}/<c \"#daa520\">/g;
	*/
	
	

	WT = "&nbsp;"; //Space at top
	WT += "<br>";
	
	//bool SendPacket = false;
	bool IsCastingFound = false;

	for(int i = 0; i < GetMaxXTargets(); ++i){

		if (XTargets[i].ID){
			
			Mob* target = entity_list.GetMobID(XTargets[i].ID);
			if (target) {
				
				//SendPacket = true;
				
				bool Casting = target->IsCasting();
				bool IsTargetedAE = false;
				bool IsInDirectionalAE = false;
				bool IsTargetsTarget = false;

				Mob* targetstarget = nullptr;
				targetstarget = target->GetTarget();

				if (targetstarget && targetstarget->GetID() == GetID())
					IsTargetsTarget = true;
				
				//uint16 remain_time = 0;
				
				float distance = 0.0f;
				float tae_distance = 0.0f;
				float range = 0.0f;
				
				int target_type = 0;
				int cast_time_pct = 0;
				uint16 spell_id = SPELL_UNKNOWN;

				//Dynamic text colors
				std::string range_color = bright_green;
				std::string tae_range_color = bright_green;
				std::string spell_color = color_blue; //This will likely need to be revised.

				if (Casting){
					IsCastingFound = true;
					spell_id = target->CastingSpellID();
					cast_time_pct = (target->GetSpellEndTime().GetRemainingTime()*100 / spells[spell_id].cast_time);
					//Shout("TEST %i %i %i %i", target->GetSpellEndTime().GetRemainingTime(), spells[spell_id ].cast_time, cast_time_pct, GetActSpellCasttime(spell_id,spells[spell_id ].cast_time));
					//remain_time = static_cast<int>((target->GetSpellEndTime().GetRemainingTime() + 500)/1000);
					target_type = spells[spell_id ].targettype;

					//Get Range Type for NPC cast spells
					switch(target_type) {

						case ST_Self:
							range = 0;
							spell_color = color_gold;
							break;

						case ST_Target:
						case ST_TargetOptional:
						case ST_Tap:
							range = spells[spell_id].range;
							spell_color = bright_red;
							break;

						case ST_AECaster:
						case ST_HateList:
							range = spells[spell_id].aoerange;
							spell_color = "<c \"#6A5ACD\">";
							break;

						case ST_TargetAETap:
						case ST_AETarget:
							range = spells[spell_id].range;
							IsTargetedAE = true;
							spell_color = "<c \"#2EFE64\">";
							
							if (targetstarget)
								tae_distance = CalculateDistance(targetstarget->GetX(), targetstarget->GetY(), targetstarget->GetZ());

							break;

						case ST_Directional:
							range = spells[spell_id].aoerange;
							//Need to check if in directional
							spell_color = "<c \"#008080\">";
							if (target->SpellDirectionalTarget(spell_id, this))
								IsInDirectionalAE = true;
							break;

						case ST_TargetLocation:
							range = spells[spell_id].aoerange;
							spell_color = "<c \"#FFA500\">";
							break;
					}
						
					if (target_type == ST_TargetLocation)
						distance = CalculateDistance(GetTargetRingX(), GetTargetRingY(), GetTargetRingZ());
					else
						distance = CalculateDistance(target->GetX(), target->GetY(), target->GetZ());

					if (distance <= range && distance >= spells[spell_id].min_range)
						range_color = bright_red;

					if (tae_distance <= spells[spell_id].aoerange && distance >= spells[spell_id].min_range)
						tae_range_color = bright_red;

					if (target_type == ST_Directional && (!IsInDirectionalAE))
						range_color = bright_green; //If not in directional overide: TURN GREEN
				}
		
				//START OF LINE

				//Are you the target's target, ! = true
				if (IsTargetsTarget){
					WT += bright_green;
					WT += "[";
					WT += "&nbsp;";
					WT += "!";
					WT += "&nbsp;";
					WT += "]";
					WT += "</c>";
				}
				else{
					WT += "[";
					WT += "&nbsp;&nbsp;&nbsp;";
					WT += "]";
				}


				WT += "&nbsp;&nbsp;";	

				if (Casting && spell_id != SPELL_UNKNOWN){

					WT += color_gold;
					WT += target->GetCleanName();
					WT += "</c>";

					WT += "&nbsp;&nbsp;&nbsp;";	
					
					//SPELL NAME
					WT += spell_color; //This is type of spell define by target type
					WT += "<";
					WT += spells[spell_id].name;
					WT += ">";
					WT += "</c>";

					WT += "&nbsp;&nbsp;&nbsp;";
					
					/*
					//CAST TIME COUNT DOWN - NOW USES BAR
					WT += heroic_color;
					WT += "(";
					if (remain_time >= 0)
						WT += itoa(remain_time);
					else
						WT += "0";
					WT += ")";
					WT += "</c>";
					
					WT += "&nbsp;&nbsp;&nbsp;";
					*/

					//RANGE CHECK {Max YOU Min}
					WT += "{";
					WT += itoa(spells[spell_id].range);
					WT += "&nbsp;";
					WT += "::";
					WT += "&nbsp;";
					WT += range_color;
					WT += itoa(distance);
					WT += "</c>";
					WT += "&nbsp;";
					WT += "::";
					WT += "&nbsp;";
					WT += itoa(spells[spell_id].min_range);
					WT += "}";

					//Targeted AE range
					if(IsTargetedAE){
						WT += "&nbsp;&nbsp;&nbsp;";
						WT += tae_range_color;
						WT += "(";
						WT += "&nbsp;";
						
						
						if (IsTargetsTarget)
							WT += "!";
						else
							WT += itoa(tae_distance);

						WT += "&nbsp;";
						WT += ")";
						WT += "</c>";
					}

					WT += "<br>";


					bool gold_set = true;
					bool white_set = true;
					for(int i = 0; i < 100; ++i){
					
						if (cast_time_pct > i){

							if (gold_set){
								WT += color_gold;
								gold_set = false;
							}
							WT += "|";
						
						}
						else {
							if (white_set){
								WT += "</c>";
								WT += color_white;
								white_set = false;
							}
							WT += "|";
						}
					}
					WT += "</c>";

					//End Casting
				}
				else {
					//NOT CASTING
					WT += color_grey;
					WT += target->GetCleanName();
					WT += "</c>";
					WT += "<br>";
					WT += color_white;
					for(int i = 0; i < 100; ++i){
					WT += "|";
					}
					WT += "</c>";
				}
				
			}
			else{
				WT += "Error: XTarget ID exists BUT NO TARGET Found.";
				continue;
			}
	
		}//XTarget ID does not exists show blank slot.
		else {
			WT += "[";
			WT += "&nbsp;&nbsp;&nbsp;";
			WT += "]";
		}

		WT += "<br><br>";
	}
	
	//if(strlen(WT.c_str()) < 4090) //Don't send if too mana chara [Should not every come close to max limit chara]
		
	SendPopupToClient("Spell Casting Awareness", WT.c_str() , POPUPID_SPELL_AWARENESS, 1, 6000);
		
	if (IsCastingFound)
		spell_awareness_popup.Start(100);
	else
		spell_awareness_popup.Start(1000);

}

void Client::PopupUIDisc()
{	
	if (!HasDiscReuseAwareness())
		return;

	std::string WT;

	// Define the types of page breaks we need
	std::string indP = "&nbsp;";
	std::string indS = "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;";
	std::string indM = "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;";
	std::string indL = "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;";
	std::string div = " | ";

	//Used colors
	std::string bright_green = "<c \"#7CFC00\">";
	std::string bright_red = "<c \"#FF0000\">";
	std::string color_gold = "<c \"#FFFF66\">";
	std::string color_grey = "<c \"#808080\">"; 
	std::string color_white = "<c \#FFFFFF\">"; 
	std::string color_blue = "<c \"#9999FF\">";

	WT = "&nbsp;"; //Space at top
	WT += "<br>";
	
	pTimerType DiscTimer;
	uint32 remain = 0;
	for(int i = 1; i < (35); ++i){

		remain = 0;
		DiscTimer = pTimerDisciplineReuseStart + i;
		if(!p_timers.Expired(&database, DiscTimer, false)){ 
			remain = p_timers.GetRemainingTime(DiscTimer);
		}
		
		WT += itoa(i);
		WT += " :: ";
		WT += itoa(remain);
		WT += "<br>";
		
	}

	SendPopupToClient("Disc Reuse Awareness", WT.c_str() , POPUPID_SPELL_DISCREUSE, 1, 6000);

	return;
	//bool SendPacket = false;
	bool IsCastingFound = false;

	for(int i = 0; i < GetMaxXTargets(); ++i){

		if (XTargets[i].ID){
			
			Mob* target = entity_list.GetMobID(XTargets[i].ID);
			if (target) {
				
				//SendPacket = true;
				
				bool Casting = target->IsCasting();
				bool IsTargetedAE = false;
				bool IsInDirectionalAE = false;
				bool IsTargetsTarget = false;

				Mob* targetstarget = nullptr;
				targetstarget = target->GetTarget();

				if (targetstarget && targetstarget->GetID() == GetID())
					IsTargetsTarget = true;
				
				//uint16 remain_time = 0;
				
				float distance = 0.0f;
				float tae_distance = 0.0f;
				float range = 0.0f;
				
				int target_type = 0;
				int cast_time_pct = 0;
				uint16 spell_id = SPELL_UNKNOWN;

				//Dynamic text colors
				std::string range_color = bright_green;
				std::string tae_range_color = bright_green;
				std::string spell_color = color_blue; //This will likely need to be revised.

				if (Casting){
					IsCastingFound = true;
					spell_id = target->CastingSpellID();
					cast_time_pct = (target->GetSpellEndTime().GetRemainingTime()*100 / spells[spell_id].cast_time);
					//Shout("TEST %i %i %i %i", target->GetSpellEndTime().GetRemainingTime(), spells[spell_id ].cast_time, cast_time_pct, GetActSpellCasttime(spell_id,spells[spell_id ].cast_time));
					//remain_time = static_cast<int>((target->GetSpellEndTime().GetRemainingTime() + 500)/1000);
					target_type = spells[spell_id ].targettype;

					//Get Range Type for NPC cast spells
					switch(target_type) {

						case ST_Self:
							range = 0;
							spell_color = color_gold;
							break;

						case ST_Target:
						case ST_TargetOptional:
						case ST_Tap:
							range = spells[spell_id].range;
							spell_color = bright_red;
							break;

						case ST_AECaster:
						case ST_HateList:
							range = spells[spell_id].aoerange;
							spell_color = "<c \"#6A5ACD\">";
							break;

						case ST_TargetAETap:
						case ST_AETarget:
							range = spells[spell_id].range;
							IsTargetedAE = true;
							spell_color = "<c \"#2EFE64\">";
							
							if (targetstarget)
								tae_distance = CalculateDistance(targetstarget->GetX(), targetstarget->GetY(), targetstarget->GetZ());

							break;

						case ST_Directional:
							range = spells[spell_id].aoerange;
							//Need to check if in directional
							spell_color = "<c \"#008080\">";
							if (target->SpellDirectionalTarget(spell_id, this))
								IsInDirectionalAE = true;
							break;

						case ST_TargetLocation:
							range = spells[spell_id].aoerange;
							spell_color = "<c \"#FFA500\">";
							break;
					}
						
					if (target_type == ST_TargetLocation)
						distance = CalculateDistance(GetTargetRingX(), GetTargetRingY(), GetTargetRingZ());
					else
						distance = CalculateDistance(target->GetX(), target->GetY(), target->GetZ());

					if (distance <= range && distance >= spells[spell_id].min_range)
						range_color = bright_red;

					if (tae_distance <= spells[spell_id].aoerange && distance >= spells[spell_id].min_range)
						tae_range_color = bright_red;

					if (target_type == ST_Directional && (!IsInDirectionalAE))
						range_color = bright_green; //If not in directional overide: TURN GREEN
				}
		
				//START OF LINE

				//Are you the target's target, ! = true
				if (IsTargetsTarget){
					WT += bright_green;
					WT += "[";
					WT += "&nbsp;";
					WT += "!";
					WT += "&nbsp;";
					WT += "]";
					WT += "</c>";
				}
				else{
					WT += "[";
					WT += "&nbsp;&nbsp;&nbsp;";
					WT += "]";
				}


				WT += "&nbsp;&nbsp;";	

				if (Casting && spell_id != SPELL_UNKNOWN){

					WT += color_gold;
					WT += target->GetCleanName();
					WT += "</c>";

					WT += "&nbsp;&nbsp;&nbsp;";	
					
					//SPELL NAME
					WT += spell_color; //This is type of spell define by target type
					WT += "<";
					WT += spells[spell_id].name;
					WT += ">";
					WT += "</c>";

					WT += "&nbsp;&nbsp;&nbsp;";
					
					/*
					//CAST TIME COUNT DOWN - NOW USES BAR
					WT += heroic_color;
					WT += "(";
					if (remain_time >= 0)
						WT += itoa(remain_time);
					else
						WT += "0";
					WT += ")";
					WT += "</c>";
					
					WT += "&nbsp;&nbsp;&nbsp;";
					*/

					//RANGE CHECK {Max YOU Min}
					WT += "{";
					WT += itoa(spells[spell_id].range);
					WT += "&nbsp;";
					WT += "::";
					WT += "&nbsp;";
					WT += range_color;
					WT += itoa(distance);
					WT += "</c>";
					WT += "&nbsp;";
					WT += "::";
					WT += "&nbsp;";
					WT += itoa(spells[spell_id].min_range);
					WT += "}";

					//Targeted AE range
					if(IsTargetedAE){
						WT += "&nbsp;&nbsp;&nbsp;";
						WT += tae_range_color;
						WT += "(";
						WT += "&nbsp;";
						
						
						if (IsTargetsTarget)
							WT += "!";
						else
							WT += itoa(tae_distance);

						WT += "&nbsp;";
						WT += ")";
						WT += "</c>";
					}

					WT += "<br>";


					bool gold_set = true;
					bool white_set = true;
					for(int i = 0; i < 100; ++i){
					
						if (cast_time_pct > i){

							if (gold_set){
								WT += color_gold;
								gold_set = false;
							}
							WT += "|";
						
						}
						else {
							if (white_set){
								WT += "</c>";
								WT += color_white;
								white_set = false;
							}
							WT += "|";
						}
					}
					WT += "</c>";

					//End Casting
				}
				else {
					//NOT CASTING
					WT += color_grey;
					WT += target->GetCleanName();
					WT += "</c>";
					WT += "<br>";
					WT += color_white;
					for(int i = 0; i < 100; ++i){
					WT += "|";
					}
					WT += "</c>";
				}
				
			}
			else{
				WT += "Error: XTarget ID exists BUT NO TARGET Found.";
				continue;
			}
	
		}//XTarget ID does not exists show blank slot.
		else {
			WT += "[";
			WT += "&nbsp;&nbsp;&nbsp;";
			WT += "]";
		}

		WT += "<br><br>";
	}
	
	//if(strlen(WT.c_str()) < 4090) //Don't send if too mana chara [Should not every come close to max limit chara]
		
	SendPopupToClient("Spell Casting Awareness", WT.c_str() , POPUPID_SPELL_DISCREUSE, 1, 6000);
		
	if (IsCastingFound)
		spell_awareness_popup.Start(100);
	else
		spell_awareness_popup.Start(1000);

}

/*
void Client::ExpeditionInfo()
{

	const char *ExpName = "Expedition Test";
	const char *leadername = "Sorvani";
	
    EQApplicationPacket *outapp = new EQApplicationPacket(OP_DzExpeditionInfo, sizeof(ExpeditionInfo_Struct));
    ExpeditionInfo_Struct* ei = (ExpeditionInfo_Struct*)outapp->pBuffer;

    //This will turn into a SQL Query
    strcpy(ei->expedition_name, ExpName );
    strcpy(ei->leader_name, leadername);
    ei->max_players = 69;
    ei->clientid = this->character_id;
    QueuePacket(outapp);
    safe_delete(outapp);
}
*/
/*
	const char *WindowTitle = "Bot Tracking Window";

	std::string WindowText;
	int LastCon = -1;
	int CurrentCon = 0;
	Mob* curMob = nullptr;

	uint32 array_counter = 0;

	auto it = mob_list.begin();

	for (auto it = mob_list.begin(); it != mob_list.end(); ++it) {
	curMob = it->second;
		if (curMob && curMob->DistNoZ(*client)<=Distance) {
			if(curMob->IsTrackable()) {
				Mob* cur_entity = curMob;
				int Extras = (cur_entity->IsBot() || cur_entity->IsPet() || cur_entity->IsFamiliar() || cur_entity->IsClient());
				const char *const MyArray[] = {
					"a_","an_","Innkeep_","Barkeep_",
					"Guard_","Merchant_","Lieutenant_",
					"Banker_","Centaur_","Aviak_","Baker_",
					"Sir_","Armorer_","Deathfist_","Deputy_",
					"Sentry_","Sentinel_","Leatherfoot_",
					"Corporal_","goblin_","Bouncer_","Captain_",
					"orc_","fire_","inferno_","young_","cinder_",
					"flame_","gnomish_","CWG_","sonic_","greater_",
					"ice_","dry_","Priest_","dark-boned_",
					"Tentacle_","Basher_","Dar_","Greenblood_",
					"clockwork_","guide_","rogue_","minotaur_",
					"brownie_","Teir'","dark_","tormented_",
					"mortuary_","lesser_","giant_","infected_",
					"wharf_","Apprentice_","Scout_","Recruit_",
					"Spiritist_","Pit_","Royal_","scalebone_",
					"carrion_","Crusader_","Trooper_","hunter_",
					"decaying_","iksar_","klok_","templar_","lord_",
					"froglok_","war_","large_","charbone_","icebone_",
					"Vicar_","Cavalier_","Heretic_","Reaver_","venomous_",
					"Sheildbearer_","pond_","mountain_","plaguebone_","Brother_",
					"great_","strathbone_","briarweb_","strathbone_","skeletal_",
					"minion_","spectral_","myconid_","spurbone_","sabretooth_",
					"Tin_","Iron_","Erollisi_","Petrifier_","Burynai_",
					"undead_","decayed_","You_","smoldering_","gyrating_",
					"lumpy_","Marshal_","Sheriff_","Chief_","Risen_",
					"lascar_","tribal_","fungi_","Xi_","Legionnaire_",
					"Centurion_","Zun_","Diabo_","Scribe_","Defender_","Capt_",
					"blazing_","Solusek_","imp_","hexbone_","elementalbone_",
					"stone_","lava_","_",""
				};
				unsigned int MyArraySize;
				for ( MyArraySize = 0; true; MyArraySize++) { //Find empty string & get size
					if (!(*(MyArray[MyArraySize]))) break; //Checks for null char in 1st pos
				};
				if (NamedOnly) {
					bool ContinueFlag = false;
					const char *CurEntityName = cur_entity->GetName(); //Call function once
					for (int Index = 0; Index < MyArraySize; Index++) {
						if (!strncasecmp(CurEntityName, MyArray[Index], strlen(MyArray[Index])) || (Extras)) {
							ContinueFlag = true;
							break; //From Index for
						};
					};
					if (ContinueFlag) continue; //Moved here or would apply to Index for
				};

				CurrentCon = client->GetLevelCon(cur_entity->GetLevel());
				if(CurrentCon != LastCon) {

					if(LastCon != -1)
						WindowText += "</c>";

					LastCon = CurrentCon;

					switch(CurrentCon) {

						case CON_GREEN: {
							WindowText += "<c \"#00FF00\">";
							break;
						}

						case CON_LIGHTBLUE: {
							WindowText += "<c \"#8080FF\">";
							break;
						}
						case CON_BLUE: {
							WindowText += "<c \"#2020FF\">";
							break;
						}

						case CON_YELLOW: {
							WindowText += "<c \"#FFFF00\">";
							break;
						}
						case CON_RED: {
							WindowText += "<c \"#FF0000\">";
							break;
						}
						default: {
							WindowText += "<c \"#FFFFFF\">";
							break;
						}
					}
				}

				WindowText += cur_entity->GetCleanName();
				WindowText += "<br>";

				if(strlen(WindowText.c_str()) > 4000) {
					// Popup window is limited to 4096 characters.
					WindowText += "</c><br><br>List truncated ... too many mobs to display";
					break;
				}
			}
		}
	}
	WindowText += "</c>";

	client->SendPopupToClient(WindowTitle, WindowText.c_str());

	return;
*/

void Client::MarkNPCTest(Mob* Target, int Number)
{
	// Send a packet to all group members in this zone causing the client to prefix the Target mob's name
	// with the specified Number.
	//
	Shout("Number %i", Number);
	if(!Target || Target->IsClient())
		return;
	uint16	MarkedNPCs[MAX_MARKED_NPCS];
	if((Number < 1) || (Number > MAX_MARKED_NPCS))
		return;

	bool AlreadyMarked = false;

	uint16 EntityID = Target->GetID();

	for(int i = 0; i < MAX_MARKED_NPCS; ++i)
		if(MarkedNPCs[i] == EntityID)
		{
			if(i == (Number - 1))
				return;

			//UpdateXTargetMarkedNPC(i+1, nullptr);
			MarkedNPCs[i] = 0;

			AlreadyMarked = true;

			break;
		}

	if(!AlreadyMarked)
	{
		if(MarkedNPCs[Number - 1])
		{
			Mob* m = entity_list.GetMob(MarkedNPCs[Number-1]);
			if(m)
				m->IsTargeted(-1);

			//UpdateXTargetMarkedNPC(Number, nullptr);
		}

		if(EntityID)
		{
			Mob* m = entity_list.GetMob(Target->GetID());
			if(m)
				m->IsTargeted(1);
		}
	}

	MarkedNPCs[Number - 1] = EntityID;

	EQApplicationPacket *outapp = new EQApplicationPacket(OP_MarkNPC, sizeof(MarkNPC_Struct));

	MarkNPC_Struct* mnpcs = (MarkNPC_Struct *)outapp->pBuffer;

	mnpcs->TargetID = EntityID;

	mnpcs->Number = Number;

	Mob *m = entity_list.GetMob(EntityID);

	if(m)
		sprintf(mnpcs->Name, "%s", m->GetCleanName());

	QueuePacket(outapp);

	safe_delete(outapp);

	//UpdateXTargetMarkedNPC(Number, m);
}

int Mob::GetOldProjectileHit(Mob* spell_target, uint16 spell_id)
{
	int caster_anim = GetProjCastingAnimation(spell_id);
	float angle = static_cast<float>(GetProjAngle(spell_id));
	float tilt = static_cast<float>(GetProjTilt(spell_id));
	float arc = static_cast<float>(GetProjArc(spell_id));

	if (tilt == 525){ //Straightens out most melee weapons.
		//angle = 1000.0f;
		if ( (GetZ() - spell_target->GetZ()) > 10.0f)
			tilt = 200.0f; //Ajdust til for Z axis - Not perfect but good enough.
	}

	//Note: Field209 / powerful_flag : Used as Speed Variable and to flag TargetType Ring/Location as a Projectile
	//Note: pvpresistbase : Used to set tilt
	//Baseline 280 mod was calculated on how long it takes for projectile to hit target at 1 speed.
	float speed = static_cast<float>(GetProjSpeed(spell_id)) / 1000.0f; 
	float distance = CalculateDistance(spell_target->GetX(), spell_target->GetY(), spell_target->GetZ());
	float hit = 0.0f;
	float dist_mod = 270.0f;
	float speed_mod = 0.0f;

	if (speed >= 0.5f && speed < 1.0f)
		speed_mod = 175.0f - ((speed - 0.5f) * (175.0f - 100.0f));
	else if (speed >= 1.0f && speed < 2.0f)
		speed_mod = 100.0f - ((speed - 1.0f) * (100.0f - 70.0f));
	else if (speed >= 2.0f && speed < 3.0f)
		speed_mod = 70.0f - ((speed - 2.0f) * (70.0f - 60.0f));
	else if (speed >= 3.0f && speed < 4.0f)
		speed_mod = 60.0f - ((speed - 3.0f) * (60.0f - 50.0f));
	else if (speed >= 4.0f && speed <= 5.0f)
		speed_mod = 50.0f - ((speed - 4.0f) * (50.0f - 40.0f));

	dist_mod = (dist_mod * speed_mod) / 100.0f;
	hit = (distance * dist_mod) / 100; //#1

	//Shout("A Proj Speed %.2f Distance %.2f SpeedMod %.2f DistMod %.2f HIT [%.2f]", speed, distance, speed_mod, dist_mod, hit);
	//Close Distance Modifiers
	if (distance <= 35 && distance > 25)
		hit *= 1.6f;
	if (distance <= 25 && distance > 15)
		hit *= 1.8f;
	if (distance <= 15) 
		hit *= 2.0f;

	return hit;
}

//DEPRECIATED FUNCTIONS

/*OLD FORMULAS
float Mob::GetSpacerAngle(float aoerange, float total_angle)
{
	//Ratio of third to second row should be between 1.33-1.66 [Ie ratio = 7/4]
	//Rows = angle / length + 1; [60/15 + 1] = 5
	//Row = total_angle + 10 / 10;
	if (total_angle == 90){
		if (aoerange <= 150 && aoerange > 100)
			return 10.0f; //10
		if (aoerange <= 100 && aoerange > 50)
			return 15.0f;//7 [Was 20]
		if (aoerange <= 50 && aoerange > 0)
			return 30.0f;//2
	}
	else if (total_angle == 60){
		if (aoerange <= 150 && aoerange > 100)
			return 10.0f; //7
		if (aoerange <= 100 && aoerange > 50)
			return 20.0f;//5 [Was 20]
		if (aoerange <= 50 && aoerange > 0)
			return 30.0f;//2
	}

	else if (total_angle > 30 && total_angle < 60){ //Expected 45 degree [337, 23]
		if (aoerange <= 150 && aoerange > 100)
			return total_angle / 5.0f;//5 [Rows in angle + 5 / 10;]
		if (aoerange <= 100 && aoerange > 50)
			return total_angle / 3.0f;//3
		if (aoerange <= 50 && aoerange > 0)
			return total_angle / 2.0f;//1
	}

	else if (total_angle == 30){
		if (aoerange <= 150 && aoerange > 100)
			return 7.5f;//4 [Was 10]
		if (aoerange <= 100 && aoerange > 50)
			return 15.0f;//3
		if (aoerange <= 50 && aoerange > 0)
			return 30.0f;//1 used to be 1.0 for single central graphic.
	}

	else if (total_angle == 16){ //Expected 45 degree [352, 8]
		if (aoerange <= 150 && aoerange > 100)
			return 7.5f;//4
		if (aoerange <= 100 && aoerange > 50)
			return 15.0f;//3
		if (aoerange <= 50 && aoerange > 0)
			return 1.0f;//1 used to be 1.0 for single central graphic.
	}

	return 0.0f;
}
*/

/*
bool Mob::BeamDirectionalCustom(uint16 spell_id, int16 resist_adjust, bool FromTarget, Mob *target)
{

	float ae_width = spells[spell_id].aoerange; //This is the width of the AE that will hit targets.
	float ae_length = spells[spell_id].range; //This is total area checked for targets.
	int maxtargets = spells[spell_id].aemaxtargets; //C!Kayen
	float origin_heading = GetHeading();

	bool bad = IsDetrimentalSpell(spell_id);	
	bool target_client_only = false;
	bool target_npc_only = false;

	bool target_found = false;

	
	if (IsClient() || IsPetOwnerClient()){ //When client/pet is caster
		if (!bad)
			target_client_only = true;
		else
			target_npc_only = true;
	}
	else //When NPC is caster
		target_client_only = true;


	Shout("Mob::BeamDirectionalCustom :: Start Cube ae_width %.2f AOE range %.2f min range %.2f", ae_width, ae_length, spells[spell_id].min_range);
	
	std::list<Mob*> targets_in_range;
	std::list<Mob*> targets_in_rectangle;
	std::list<Mob*>::iterator iter;

	float ae_height = ae_length;
		if (ae_width > ae_length)
			ae_height = ae_width;

	entity_list.GetTargetsForConeArea(this, spells[spell_id].min_range, (ae_length + ae_width), ae_height , targets_in_range);
	iter = targets_in_range.begin();

	while(iter != targets_in_range.end())
	{
		if (!(*iter) 
			|| ((*iter)->GetUtilityTempPetSpellID())  //Skip Projectile and SpellGFX temp pets.
			|| (target_client_only && ((*iter)->IsNPC() && !(*iter)->IsPetOwnerClient()))//Exclude NPC that are not pets
			|| (target_npc_only && ((*iter)->IsClient() || (*iter)->IsPetOwnerClient()))//Exclude Clients and Pets
			|| (*iter)->BehindMob(this, (*iter)->GetX(),(*iter)->GetY())){
		    ++iter;
			continue;
		}

		(*iter)->Shout("Found!");

		if ((InRectangle(spell_id, (*iter)->GetX(), (*iter)->GetY(),origin_heading))){
			if(spells[spell_id].npc_no_los || CheckLosFN((*iter))) {
				(*iter)->CalcSpellPowerDistanceMod(spell_id, 0, this);
				target_found = true;
				if (maxtargets) 
					targets_in_rectangle.push_back(*iter);
				else
					SpellOnTarget(spell_id, (*iter), false, true, resist_adjust);
			}
		}

		++iter;
	}

	if (maxtargets)
		CastOnClosestTarget(spell_id, resist_adjust, maxtargets, targets_in_rectangle);

	if (!target_found)
		DirectionalFailMessage(spell_id);
	
	return true;
}
*/

/*OLD VERSION 3/7/16
bool Mob::TrySpellProjectileTargetRing(Mob* spell_target,  uint16 spell_id){
	
	if (!spell_target || !IsValidSpell(spell_id))
		return false;
	
	int bolt_id = -1;

	//Make sure there is an avialable projectile to be cast.
	for (int i = 0; i < MAX_SPELL_PROJECTILE; i++) {
		if (projectile_increment_ring[i] == 0){
			bolt_id = i;
			break;
		}
	}

	if (bolt_id < 0)
		return false;

	const char *item_IDFile = spells[spell_id].player_1;

	//Coded narrowly for Enchanter effect.
	if ((strcmp(item_IDFile, "PROJECTILE_WPN")) == 0){
		if (IsClient()) {
			ItemInst* inst = CastToClient()->m_inv.GetItem(MainPrimary);
			if (inst && CastToClient()->IsSpectralBladeEquiped()) 
				 item_IDFile = inst->GetItem()->IDFile;
			else 
				return false;
		}
	}

	bool SendProjectile = true;
	int caster_anim = GetProjCastingAnimation(spell_id);
	float angle = static_cast<float>(GetProjAngle(spell_id));
	float tilt = static_cast<float>(GetProjTilt(spell_id));
	float arc = static_cast<float>(GetProjArc(spell_id));

	if (tilt == 525){ //Straightens out most melee weapons.
		//angle = 1000.0f;
		if ( (GetZ() - spell_target->GetZ()) > 10.0f)
			tilt = 200.0f; //Ajdust til for Z axis - Not perfect but good enough.
	}

	//Note: Field209 / powerful_flag : Used as Speed Variable and to flag TargetType Ring/Location as a Projectile
	//Note: pvpresistbase : Used to set tilt
	//Baseline 280 mod was calculated on how long it takes for projectile to hit target at 1 speed.
	float speed = static_cast<float>(GetProjSpeed(spell_id)) / 1000.0f; 
	float distance = CalculateDistance(spell_target->GetX(), spell_target->GetY(), spell_target->GetZ());
	float hit = 0.0f;
	float dist_mod = 270.0f;
	float speed_mod = 0.0f;

	if (speed >= 0.5f && speed < 1.0f)
		speed_mod = 175.0f - ((speed - 0.5f) * (175.0f - 100.0f));
	else if (speed >= 1.0f && speed < 2.0f)
		speed_mod = 100.0f - ((speed - 1.0f) * (100.0f - 70.0f));
	else if (speed >= 2.0f && speed < 3.0f)
		speed_mod = 70.0f - ((speed - 2.0f) * (70.0f - 60.0f));
	else if (speed >= 3.0f && speed < 4.0f)
		speed_mod = 60.0f - ((speed - 3.0f) * (60.0f - 50.0f));
	else if (speed >= 4.0f && speed <= 5.0f)
		speed_mod = 50.0f - ((speed - 4.0f) * (50.0f - 40.0f));

	dist_mod = (dist_mod * speed_mod) / 100.0f;
	hit = (distance * dist_mod) / 100; //#1

	//Shout("A Proj Speed %.2f Distance %.2f SpeedMod %.2f DistMod %.2f HIT [%.2f]", speed, distance, speed_mod, dist_mod, hit);
	//Close Distance Modifiers
	if (distance <= 35 && distance > 25)
		hit *= 1.6f;
	if (distance <= 25 && distance > 15)
		hit *= 1.8f;
	if (distance <= 15) 
		hit *= 2.0f;

	//Shout("B Proj Speed %.2f Distance %.2f SpeedMod %.2f DistMod %.2f HIT [%.2f]", speed, distance, speed_mod, dist_mod, hit);
	
	projectile_spell_id_ring[bolt_id] = spell_id;
	projectile_target_id_ring[bolt_id]  = spell_target->GetID();
	projectile_increment_ring[bolt_id]  = 1;
	projectile_hit_ring[bolt_id] = static_cast<int>(hit);
	SetProjectileRing(true);

	SkillUseTypes skillinuse;
	
	if (caster_anim != 44) //44 is standard 'nuke' spell animation.
		skillinuse = static_cast<SkillUseTypes>(caster_anim);
	else
		skillinuse = SkillArchery;
	
	if (IsClient())
		ClientFaceTarget(spell_target);
	else
		FaceTarget(spell_target);
	

	if (SendProjectile){

		ProjectileAnimation(spell_target,0, false, speed,angle,tilt,arc, item_IDFile,skillinuse);

		//Enchanter triple blade effect - Requires adjusting angle based on heading to get correct appearance.
		if (spell_id == SPELL_GROUP_SPECTRAL_BLADE_FLURRY){ //This will likely need to be adjusted.
			float _angle =  CalcSpecialProjectile(spell_id);
			//Shout("DEBUG: TrySpellProjectileTargetRingSpecial ::  Angle %.2f [Heading %.2f]", _angle, GetHeading()/2);
			if (GetCastFromCrouchInterval() >= 1){
				ProjectileAnimation(spell_target,0, false, speed,_angle,tilt,600, item_IDFile,skillinuse);
				ProjectileAnimation(spell_target,0, false, speed,(_angle - 70),tilt,600, item_IDFile,skillinuse);
			}
			if (GetCastFromCrouchInterval() >= 2){
				ProjectileAnimation(spell_target,0, false, (speed - 0.2f),_angle,tilt,600, item_IDFile,skillinuse);
				ProjectileAnimation(spell_target,0, false, (speed - 0.2f),(_angle - 70),tilt,600, item_IDFile,skillinuse);
			}

			if (GetCastFromCrouchInterval() >= 3){
				ProjectileAnimation(spell_target,0, false, (speed - 0.4f),_angle,tilt,600, item_IDFile,skillinuse);
				ProjectileAnimation(spell_target,0, false, (speed - 0.4f),(_angle - 70),tilt,600, item_IDFile,skillinuse);
			}
		}
	}
	
	//Override the default projectile animation which is based on item type.
	if (caster_anim == 44)
		DoAnim(caster_anim, 0, true, IsClient() ? FilterPCSpells : FilterNPCSpells);
	
	return true;
}


void Mob::SpellProjectileEffectTargetRing()
{
	if (!HasProjectileRing())
		return;

	for (int i = 0; i < MAX_SPELL_PROJECTILE; i++) {
	
		if (!projectile_increment_ring[i])
			continue;
		
		if (projectile_increment_ring[i] > projectile_hit_ring[i]){
		//Shout("Inc %i Hit %i", projectile_increment_ring[i], projectile_hit_ring[i]);

			Mob* target = entity_list.GetMobID(projectile_target_id_ring[i]);
			if (target){
				uint16 p_spell_id = projectile_spell_id_ring[i];
				if (IsValidSpell(p_spell_id)){
					if (IsProjectile(p_spell_id)){ //Powerful Flag denotes 'Spell Projectile'
						entity_list.AESpell(this, target, p_spell_id, false, spells[p_spell_id].ResistDiff);

						if (HasProjectileAESpellHitTarget())
							TryApplyEffectProjectileHit(p_spell_id, this);
						else{
							//ProjectileTargetRingFailMessage(p_spell_id); //No longer needed since does graphic.
							//This provides a particle effect even if the projectile MISSES. Casts a graphic only spell.
							SpellFinished((spells[p_spell_id].spellanim + 10000), target, 10, 0, -1, -1000);
						}
						
						//Reset Projectile Ring Variables
						SetCastFromCrouchIntervalProj(0);
						SetProjectileAESpellHitTarget(false);
					}
				}
				//target->Depop(); //Depop Temp Pet at Ring Location - Now pets auto depop after 10 seconds
			}

			//Reset Projectile Ring variables.
			projectile_increment_ring[i] = 0;
			projectile_hit_ring[i] = 0;
			projectile_spell_id_ring[i] = 0;
			projectile_target_id_ring[i] = 0;
			
			if (!ExistsProjectileRing())
				SetProjectileRing(false);

		}
		else
			projectile_increment_ring[i]++;
	}
}

bool Mob::ExistsProjectileRing()
{
	for (int i = 0; i < MAX_SPELL_PROJECTILE; i++) {
	
		if (projectile_increment_ring[i])
			return true;
	}
	return false;
}

*/

/*
//DEPRECIATED NOW CALLED FROM ENTITYLIST
void Mob::ConeDirectionalCustom(uint16 spell_id, int16 resist_adjust)
{
			//C!Kayen - Do polygon area if no directional is set.
			if (!spells[spell_id].directional_start && !spells[spell_id].directional_end){
				//SpellGraphicTempPet(spell_id, 2);
				entity_list.AEBeamDirectional(this, spell_id, resist_adjust);
				//BeamDirectionalCustom(spell_id, resist_adjust);
				//RectangleDirectional(spell_id,resist_adjust);
				return;
			}

			SpellGraphicTempPet(1, spell_id, spells[spell_id].aoerange); //Experimental - only cast on if YOUR pet AND not cast on yet.

			//C!Kayen - TODO Need to add custom spell effect to set target_exclude_NPC
			int maxtargets = spells[spell_id].aemaxtargets; //C!Kayen
			bool target_found = false; //C!Kayen - Determine if message for no targets hit.

			bool taget_exclude_npc = false; //False by default!
			bool target_client_only = false;

			if (IsBeneficialSpell(spell_id) && IsClient())
				target_client_only = true;

			if (!IsClient() && taget_exclude_npc)
				target_client_only = true;

			float angle_start = spells[spell_id].directional_start + (GetHeading() * 360.0f / 256.0f);
			float angle_end = spells[spell_id].directional_end + (GetHeading() * 360.0f / 256.0f);

			while(angle_start > 360.0f)
				angle_start -= 360.0f;

			while(angle_end > 360.0f)
				angle_end -= 360.0f;

			std::list<Mob*> targets_in_range;
			std::list<Mob*> targets_in_cone; //C!Kayen - Get the targets within the cone
			std::list<Mob*>::iterator iter;

			float aoerange = spells[spell_id].aoerange;
			float min_aoerange = spells[spell_id].min_range;

			if (IsClient() && IsRangeSpellEffect(spell_id)){
				min_aoerange = 0.0f; //Calculated in the archery fire as a miss.
				if (aoerange == UseRangeFromRangedWpn())//Flags aoe range to use weapon ranage.
					aoerange = CastToClient()->GetArcheryRange(nullptr);
			}

			entity_list.GetTargetsForConeArea(this, min_aoerange, aoerange, aoerange / 2, targets_in_range);
			iter = targets_in_range.begin();

			//C!Kayen - Allow to hit caster
			if (IsBeneficialSpell(spell_id) && DirectionalAffectCaster(spell_id))
				SpellOnTarget(spell_id,this, false, true, resist_adjust);

			while(iter != targets_in_range.end())
			{
				if (!(*iter) || (!CastToClient()->GetGM() && target_client_only && ((*iter)->IsNPC() && !(*iter)->IsPetOwnerClient()))){
				    ++iter;
					continue;
				}

				if (!target_client_only && ((*iter)->IsNPC() && (*iter)->IsPetOwnerClient())){
					++iter;
					continue; //Skip client pets for determinetal spells
				}

				if ((*iter)->GetUtilityTempPetSpellID()){
					++iter;
					continue; //Skip Projectile and SpellGFX temp pets.
				}

				float heading_to_target = (CalculateHeadingToTarget((*iter)->GetX(), (*iter)->GetY()) * 360.0f / 256.0f);
				while(heading_to_target < 0.0f)
					heading_to_target += 360.0f;

				while(heading_to_target > 360.0f)
					heading_to_target -= 360.0f;

				if(angle_start > angle_end)
				{
					if((heading_to_target >= angle_start && heading_to_target <= 360.0f) ||
						(heading_to_target >= 0.0f && heading_to_target <= angle_end))
					{
						if(CheckLosFN((*iter)) || spells[spell_id].npc_no_los){
							(*iter)->CalcSpellPowerDistanceMod(spell_id, 0, this);
							target_found = true; //C!Kayen

							if (maxtargets)
								targets_in_cone.push_back(*iter);
							else
								SpellOnTarget(spell_id,(*iter), false, true, resist_adjust);

						}
					}
				}
				else
				{
					if(heading_to_target >= angle_start && heading_to_target <= angle_end)
					{
						if(CheckLosFN((*iter)) || spells[spell_id].npc_no_los) {
							(*iter)->CalcSpellPowerDistanceMod(spell_id, 0, this);
							target_found = true; //C!Kayen
							if (maxtargets) 
								targets_in_cone.push_back(*iter);
							else
								SpellOnTarget(spell_id, (*iter), false, true, resist_adjust);
						}
					}
				}
				++iter;
			}

			//C!Kayen - If maxtarget is set it will hit each of the closet targets up to max amount.
			//Ie. If you set to 1, it will only hit the closest target in the beam.
			if (maxtargets)
				CastOnClosestTarget(spell_id, resist_adjust, maxtargets, targets_in_cone);

			if (!target_found)
				DirectionalFailMessage(spell_id);

}
*/

/*
//!DEPRECIATED
bool Mob::RectangleDirectional(uint16 spell_id, int16 resist_adjust, bool FromTarget, Mob *target)
{
	float ae_width = spells[spell_id].aoerange; //This is the width of the AE that will hit targets.
	float radius = spells[spell_id].range; //This is total area checked for targets.
	int maxtargets = spells[spell_id].aemaxtargets; //C!Kayen

	bool taget_exclude_npc = false; //False by default!
			
	bool target_client_only = false;

	bool target_found = false;

	if (IsBeneficialSpell(spell_id) && IsClient())
		target_client_only = true;

	if (!IsClient() && taget_exclude_npc)
		target_client_only = true;
	
	std::list<Mob*> targets_in_range;
	std::list<Mob*> targets_in_rectangle;
	std::list<Mob*>::iterator iter;

	entity_list.GetTargetsForConeArea(this, spells[spell_id].min_range, radius, radius / 2, targets_in_range);
	iter = targets_in_range.begin();
	
	float dX = 0;
	float dY = 0;
	float dZ = 0;
	
	if (!FromTarget){
		CalcDestFromHeading(GetHeading(), radius, 5, GetX(), GetY(), dX, dY,  dZ);
		dZ = GetZ();
	}
	else {
		if (target){
			dX = target->GetX();
			dY = target->GetY();
			dZ = target->GetZ();
		}
		else
			return false;
	}

	
	//Shout("X Y Z %.2f %.2f %.2f DIstancehcek %.2f Vector Size = %i", dX, dY, dZ, CalculateDistance(dX, dY, dZ), targets_in_range.size());
	//'DEFENDER' is the virtual end point of the line being drawn based on range from which slope is derived. 

	float DEFENDER_X = dX;
	float DEFENDER_Y = dY;
	float ATTACKER_X = GetX();
	float ATTACKER_Y = GetY();

	float y_dif = DEFENDER_Y - ATTACKER_Y;
	float x_dif = DEFENDER_X - ATTACKER_X;

	float x1 = ATTACKER_X;
	float y1 = ATTACKER_Y;
	float x2 = DEFENDER_X;
	float y2 = DEFENDER_Y;

	//FIND SLOPE: Put it into the form y = mx + b
	float m = (y2 - y1) / (x2 - x1);
	float b = (y1 * x2 - y2 * x1) / (x2 - x1);
 
	while(iter != targets_in_range.end())
	{
		if (!(*iter) || (target_client_only && ((*iter)->IsNPC() && !(*iter)->IsPetOwnerClient())) 
			|| (*iter)->BehindMob(this, (*iter)->GetX(),(*iter)->GetY())){
		    ++iter;
			continue;
		}
		
		float Unit_X =(*iter)->GetX();
		float Unit_Y =(*iter)->GetY();
		float y_dif2 = Unit_Y - ATTACKER_Y;
	
		//#Only target units in the quadrant of the attacker using y axis
		if ( ((y_dif2 > 0) && (y_dif > 0)) || ((y_dif2 < 0) && (y_dif < 0)))
		{					
			//# target point is (x0, y0)
			float x0 = Unit_X;
			float y0 = Unit_Y;
			//# shortest distance from line to target point
			float d = abs( y0 - m * x0 - b) / sqrt(m * m + 1);
					
			if (d <= ae_width)
			{
				if(CheckLosFN((*iter)) || spells[spell_id].npc_no_los) {
					(*iter)->CalcSpellPowerDistanceMod(spell_id, 0, this);
					target_found = true;
					if (maxtargets) 
						targets_in_rectangle.push_back(*iter);
					else
						SpellOnTarget(spell_id, (*iter), false, true, resist_adjust);
				}
			}
		}
		++iter;
	}

	if (maxtargets)
		CastOnClosestTarget(spell_id, resist_adjust, maxtargets, targets_in_rectangle);

	if (!target_found)
		DirectionalFailMessage(spell_id);
	
	return true;
}
*/

