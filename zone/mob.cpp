/*	EQEMu: Everquest Server Emulator
	Copyright (C) 2001-2002 EQEMu Development Team (http://eqemu.org)

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
#include "../common/debug.h"
#include "masterentity.h"
#include "../common/spdat.h"
#include "string_ids.h"
#include "worldserver.h"
#include "quest_parser_collection.h"
#include "../common/string_util.h"

#include <sstream>
#include <math.h>
#include <limits.h>

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
		float		in_heading,
		float		in_x_pos,
		float		in_y_pos,
		float		in_z_pos,

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
		uint32		in_scalerate
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
		flee_timer(FLEE_CHECK_TIMER)

{
	targeted = 0;
	tar_ndx=0;
	tar_vector=0;
	tar_vx=0;
	tar_vy=0;
	tar_vz=0;
	tarx=0;
	tary=0;
	tarz=0;
	fear_walkto_x = -999999;
	fear_walkto_y = -999999;
	fear_walkto_z = -999999;
	curfp = false;

	AI_Init();
	SetMoving(false);
	moved=false;
	rewind_x = 0;		//Stored x_pos for /rewind
	rewind_y = 0;		//Stored y_pos for /rewind
	rewind_z = 0;		//Stored z_pos for /rewind
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


	// sanity check
	if (runspeed < 0 || runspeed > 20)
		runspeed = 1.25f;

	heading		= in_heading;
	x_pos		= in_x_pos;
	y_pos		= in_y_pos;
	z_pos		= in_z_pos;
	light		= in_light;
	texture		= in_texture;
	helmtexture	= in_helmtexture;
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
		SpellProcs[j].spellID = SPELL_UNKNOWN;
		SpellProcs[j].chance = 0;
		SpellProcs[j].base_spellID = SPELL_UNKNOWN;
		DefensiveProcs[j].spellID = SPELL_UNKNOWN;
		DefensiveProcs[j].chance = 0;
		DefensiveProcs[j].base_spellID = SPELL_UNKNOWN;
		RangedProcs[j].spellID = SPELL_UNKNOWN;
		RangedProcs[j].chance = 0;
		RangedProcs[j].base_spellID = SPELL_UNKNOWN;
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

	delta_heading = 0;
	delta_x = 0;
	delta_y = 0;
	delta_z = 0;
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
	casting_spell_type = 0;
	casting_spell_inventory_slot = 0;
	target = 0;

	for (int i = 0; i < MAX_SPELL_PROJECTILE; i++) { projectile_spell_id[i] = 0; }
	for (int i = 0; i < MAX_SPELL_PROJECTILE; i++) { projectile_target_id[i] = 0; }
	for (int i = 0; i < MAX_SPELL_PROJECTILE; i++) { projectile_increment[i] = 0; }
	for (int i = 0; i < MAX_SPELL_PROJECTILE; i++) { projectile_x[i] = 0; }
	for (int i = 0; i < MAX_SPELL_PROJECTILE; i++) { projectile_y[i] = 0; }
	for (int i = 0; i < MAX_SPELL_PROJECTILE; i++) { projectile_z[i] = 0; }
	projectile_timer.Disable();

	memset(&itembonuses, 0, sizeof(StatBonuses));
	memset(&spellbonuses, 0, sizeof(StatBonuses));
	memset(&aabonuses, 0, sizeof(StatBonuses));
	spellbonuses.AggroRange = -1;
	spellbonuses.AssistRange = -1;
	pLastChange = 0;
	SetPetID(0);
	SetOwnerID(0);
	typeofpet = petCharmed;		//default to charmed...
	petpower = 0;
	held = false;
	nocast = false;
	focused = false;
	_IsTempPet = false;
	pet_owner_client = false;

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
	cur_wp_x = 0;
	cur_wp_y = 0;
	cur_wp_z = 0;
	cur_wp_pause = 0;
	patrol=0;
	follow=0;
	follow_dist = 100;	// Default Distance for Follow
	flee_mode = false;
	fear_walkto_x = -999999;
	fear_walkto_y = -999999;
	fear_walkto_z = -999999;
	curfp = false;
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

	targetring_x = 0.0f;
	targetring_y = 0.0f; 
	targetring_z = 0.0f;

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
	hate_list.SetOwner(this);

	m_AllowBeneficial = false;
	m_DisableMelee = false;
	for (int i = 0; i < HIGHEST_SKILL+2; i++) { SkillDmgTaken_Mod[i] = 0; }
	for (int i = 0; i < HIGHEST_RESIST+2; i++) { Vulnerability_Mod[i] = 0; }

	emoteid = 0;
	endur_upkeep = false;
	
	//C!Kayen
	momentum = 0.0f; 
	for (int i = 0; i < HIGHEST_SKILL+2; i++) { WpnSkillDmgBonus[i] = 0; }
	for (int i = 0; i < HIGHEST_RESIST+2; i++) { SpellResistTypeDmgBonus[i] = 0; }
	leap_increment = 0;
	leap_spell_id = 0;
	leap_x = 0.0f;	leap_y = 0.0f;	leap_z = 0.0f;
		
	for (int i = 0; i < MAX_SPELL_PROJECTILE; i++) {projectile_spell_id_ring[i] = 0; }
	for (int i = 0; i < MAX_SPELL_PROJECTILE; i++) {projectile_target_id_ring[i] = 0; }
	for (int i = 0; i < MAX_SPELL_PROJECTILE; i++) {projectile_increment_ring[i] = 0; }
	for (int i = 0; i < MAX_SPELL_PROJECTILE; i++) {projectile_hit_ring[i] = 0; }
	ActiveProjectileRing = false;
	ActiveProjectile = false;
	ProjectilePet = false;
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
	fast_buff_tick_count = 0;
	has_fast_buff = false;

	charge_effect = 0;
	charge_effect_increment = 0;

	bravery_recast = 0;

	effect_field_timer.Disable();
	aura_field_timer.Disable();
	fast_buff_tick_timer.Disable();
	stun_resilience_timer.Disable();
	charge_effect_timer.Disable();

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

	if ((this->GetPetType() == petCharmed) && (invisible || hidden || improved_hidden))
	{
		Mob* formerpet = this->GetPet();

		if(formerpet)
			formerpet->BuffFadeByEffect(SE_Charm);
	}
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

float Mob::_GetMovementSpeed(int mod) const
{
	// List of movement speed modifiers, including AAs & spells:
	// http://everquest.allakhazam.com/db/item.html?item=1721;page=1;howmany=50#m10822246245352
	if (IsRooted())
		return 0.0f;
	if (IsPseudoRooted())
		return 0.000001f;

	float speed_mod = runspeed;

	speed_mod += GetMomentum(); //C!Kayen 

	// These two cases ignore the cap, be wise in the DB for horses.
	if (IsClient()) {
		if (CastToClient()->GetGMSpeed()) {
			speed_mod = 3.125f;
			if (mod != 0)
				speed_mod += speed_mod * static_cast<float>(mod) / 100.0f;
			return speed_mod;
		} else {
			Mob *horse = entity_list.GetMob(CastToClient()->GetHorseId());
			if (horse) {
				speed_mod = horse->GetBaseRunspeed();
				if (mod != 0)
					speed_mod += speed_mod * static_cast<float>(mod) / 100.0f;
				return speed_mod;
			}
		}
	}

	int aa_mod = 0;
	int spell_mod = 0;
	int runspeedcap = RuleI(Character,BaseRunSpeedCap);
	int movemod = 0;
	float frunspeedcap = 0.0f;

	runspeedcap += itembonuses.IncreaseRunSpeedCap + spellbonuses.IncreaseRunSpeedCap + aabonuses.IncreaseRunSpeedCap;
	aa_mod += itembonuses.BaseMovementSpeed + spellbonuses.BaseMovementSpeed + aabonuses.BaseMovementSpeed;
	spell_mod += spellbonuses.movementspeed + itembonuses.movementspeed;

	// hard cap
	if (runspeedcap > 225)
		runspeedcap = 225;

	if (spell_mod < 0)
		movemod += spell_mod;
	else if (spell_mod > aa_mod)
		movemod = spell_mod;
	else
		movemod = aa_mod;

	// cap negative movemods from snares mostly
	if (movemod < -85)
		movemod = -85;

	if (movemod != 0)
		speed_mod += speed_mod * static_cast<float>(movemod) / 100.0f;

	// runspeed caps
	frunspeedcap = static_cast<float>(runspeedcap) / 100.0f;
	if (IsClient() && speed_mod > frunspeedcap)
		speed_mod = frunspeedcap;

	// apply final mod such as the -47 for walking
	// use runspeed since it should stack with snares
	// and if we get here, we know runspeed was the initial
	// value before we applied movemod.
	if (mod != 0)
		speed_mod += runspeed * static_cast<float>(mod) / 100.0f;

	if (speed_mod <= 0.0f)
		speed_mod = IsClient() ? 0.0001f : 0.0f;

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

	if(strlen(ns->spawn.lastName) == 0) {
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
	switch(ns->spawn.class_)
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
		strcpy(ns->spawn.lastName,"Adventure Merchant");
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
		strcpy(ns->spawn.lastName, "Mercenary Recruiter");
		break;
	default:
		strcpy(ns2->spawn.lastName, ns->spawn.lastName);
		break;
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

	ns->spawn.heading	= FloatToEQ19(heading);
	ns->spawn.x			= FloatToEQ19(x_pos);//((int32)x_pos)<<3;
	ns->spawn.y			= FloatToEQ19(y_pos);//((int32)y_pos)<<3;
	ns->spawn.z			= FloatToEQ19(z_pos);//((int32)z_pos)<<3;
	ns->spawn.spawnId	= GetID();
	ns->spawn.curHp	= static_cast<uint8>(GetHPRatio());
	ns->spawn.max_hp	= 100;		//this field needs a better name
	ns->spawn.race		= race;
	ns->spawn.runspeed	= runspeed;
	ns->spawn.walkspeed	= runspeed * 0.5f;
	ns->spawn.class_	= class_;
	ns->spawn.gender	= gender;
	ns->spawn.level		= level;
	ns->spawn.deity		= deity;
	ns->spawn.animation	= 0;
	ns->spawn.findable	= findable?1:0;
	ns->spawn.light		= light;
	ns->spawn.showhelm = 1;

	ns->spawn.invis		= (invisible || hidden) ? 1 : 0;	// TODO: load this before spawning players
	ns->spawn.NPC		= IsClient() ? 0 : 1;
	ns->spawn.IsMercenary = (IsMerc() || no_target_hotkey) ? 1 : 0;

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
	ns->spawn.equip_chest2 = texture;

//	ns->spawn.invis2 = 0xff;//this used to be labeled beard.. if its not FF it will turn mob invis

	if(helmtexture && helmtexture != 0xFF)
	{
		ns->spawn.helm=helmtexture;
	} else {
		ns->spawn.helm = 0;
	}

	ns->spawn.guildrank	= 0xFF;
	ns->spawn.size			= size;
	ns->spawn.bodytype = bodytype;
	// The 'flymode' settings have the following effect:
	// 0 - Mobs in water sink like a stone to the bottom
	// 1 - Same as #flymode 1
	// 2 - Same as #flymode 2
	// 3 - Mobs in water do not sink. A value of 3 in this field appears to be the default setting for all mobs
	// (in water or not) according to 6.2 era packet collects.
	if(IsClient())
	{
		ns->spawn.flymode = FindType(SE_Levitate) ? 2 : 0;
	}
	else
		ns->spawn.flymode = flymode;

	ns->spawn.lastName[0] = '\0';

	strn0cpy(ns->spawn.lastName, lastname, sizeof(ns->spawn.lastName));

	for(i = 0; i < _MaterialCount; i++)
	{
		ns->spawn.equipment[i] = GetEquipmentMaterial(i);
		if (armor_tint[i])
		{
			ns->spawn.colors[i].color = armor_tint[i];
		}
		else
		{
			ns->spawn.colors[i].color = GetEquipmentColor(i);
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
		// #appearnace 44 3 makes it jump but not visible difference to 3
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
void Mob::SendHPUpdate()
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

	// send to self - we need the actual hps here
	if(IsClient())
	{
		EQApplicationPacket* hp_app2 = new EQApplicationPacket(OP_HPUpdate,sizeof(SpawnHPUpdate_Struct));
		SpawnHPUpdate_Struct* ds = (SpawnHPUpdate_Struct*)hp_app2->pBuffer;
		ds->cur_hp = CastToClient()->GetHP() - itembonuses.HP;
		ds->spawn_id = GetID();
		ds->max_hp = CastToClient()->GetMaxHP() - itembonuses.HP;
		CastToClient()->QueuePacket(hp_app2);
		safe_delete(hp_app2);
	}
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
		if (this->IsClient())
			this->CastToClient()->FastQueuePacket(&app,false);
	}
	else
	{
		if(move_tic_count == RuleI(Zone, NPCPositonUpdateTicCount))
		{
			entity_list.QueueClients(this, app, (iSendToSelf==0), false);
			move_tic_count = 0;
		}
		else
		{
			entity_list.QueueCloseClients(this, app, (iSendToSelf==0), 800, nullptr, false);
			move_tic_count++;
		}
	}
	safe_delete(app);
}

// this is for SendPosition()
void Mob::MakeSpawnUpdateNoDelta(PlayerPositionUpdateServer_Struct *spu){
	memset(spu,0xff,sizeof(PlayerPositionUpdateServer_Struct));
	spu->spawn_id	= GetID();
	spu->x_pos		= FloatToEQ19(x_pos);
	spu->y_pos		= FloatToEQ19(y_pos);
	spu->z_pos		= FloatToEQ19(z_pos);
	spu->delta_x	= NewFloatToEQ13(0);
	spu->delta_y	= NewFloatToEQ13(0);
	spu->delta_z	= NewFloatToEQ13(0);
	spu->heading	= FloatToEQ19(heading);
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
	spu->x_pos		= FloatToEQ19(x_pos);
	spu->y_pos		= FloatToEQ19(y_pos);
	spu->z_pos		= FloatToEQ19(z_pos);
	spu->delta_x	= NewFloatToEQ13(delta_x);
	spu->delta_y	= NewFloatToEQ13(delta_y);
	spu->delta_z	= NewFloatToEQ13(delta_z);
	spu->heading	= FloatToEQ19(heading);
	spu->padding0002	=0;
	spu->padding0006	=7;
	spu->padding0014	=0x7f;
	spu->padding0018	=0x5df27;
	if(this->IsClient())
		spu->animation = animation;
	else
		spu->animation	= pRunAnimSpeed;//animation;
	spu->delta_heading = NewFloatToEQ13(static_cast<float>(delta_heading));
}

void Mob::ShowStats(Client* client)
{
	if (IsClient()) {
		CastToClient()->SendStatsWindow(client, RuleB(Character, UseNewStatsWindow));
	}
	else if (IsCorpse()) {
		if (IsPlayerCorpse()) {
			client->Message(0, "  CharID: %i  PlayerCorpse: %i", CastToCorpse()->GetCharID(), CastToCorpse()->GetDBID());
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
		client->Message(0, "  MR: %i  PR: %i  FR: %i  CR: %i  DR: %i Corruption: %i", GetMR(), GetPR(), GetFR(), GetCR(), GetDR(), GetCorrup());
		client->Message(0, "  Race: %i  BaseRace: %i  Texture: %i  HelmTexture: %i  Gender: %i  BaseGender: %i", GetRace(), GetBaseRace(), GetTexture(), GetHelmTexture(), GetGender(), GetBaseGender());
		if (client->Admin() >= 100)
			client->Message(0, "  EntityID: %i  PetID: %i  OwnerID: %i AIControlled: %i Targetted: %i", GetID(), GetPetID(), GetOwnerID(), IsAIControlled(), targeted);

		if (IsNPC()) {
			NPC *n = CastToNPC();
			uint32 spawngroupid = 0;
			if(n->respawn2 != 0)
				spawngroupid = n->respawn2->SpawnGroupID();
			client->Message(0, "  NPCID: %u  SpawnGroupID: %u Grid: %i LootTable: %u FactionID: %i SpellsID: %u ", GetNPCTypeID(),spawngroupid, n->GetGrid(), n->GetLoottableID(), n->GetNPCFactionID(), n->GetNPCSpellsID());
			client->Message(0, "  Accuracy: %i MerchantID: %i EmoteID: %i Runspeed: %f Walkspeed: %f", n->GetAccuracyRating(), n->MerchantType, n->GetEmoteID(), n->GetRunspeed(), n->GetWalkspeed());
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
		anim->action = 10;
		anim->value=animnum;
	}
	else{
		anim->action = animnum;
		anim->value=type;
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

	x_pos = x;
	y_pos = y;
	z_pos = z;
	if (heading != 0.01)
		this->heading = heading;
	if(IsNPC())
		CastToNPC()->SaveGuardSpot(true);
	if(SendUpdate)
		SendPosition();
}

void Mob::SendIllusionPacket(uint16 in_race, uint8 in_gender, uint8 in_texture, uint8 in_helmtexture, uint8 in_haircolor, uint8 in_beardcolor, uint8 in_eyecolor1, uint8 in_eyecolor2, uint8 in_hairstyle, uint8 in_luclinface, uint8 in_beard, uint8 in_aa_title, uint32 in_drakkin_heritage, uint32 in_drakkin_tattoo, uint32 in_drakkin_details, float in_size) {

	uint16 BaseRace = GetBaseRace();

	if (in_race == 0) {
		this->race = BaseRace;
		if (in_gender == 0xFF)
			this->gender = GetBaseGender();
		else
			this->gender = in_gender;
	}
	else {
		this->race = in_race;
		if (in_gender == 0xFF) {
			uint8 tmp = Mob::GetDefaultGender(this->race, gender);
			if (tmp == 2)
				gender = 2;
			else if (gender == 2 && GetBaseGender() == 2)
				gender = tmp;
			else if (gender == 2)
				gender = GetBaseGender();
		}
		else
			gender = in_gender;
	}
	if (in_texture == 0xFF) {
		if (in_race <= 12 || in_race == 128 || in_race == 130 || in_race == 330 || in_race == 522)
			this->texture = 0xFF;
		else
			this->texture = GetTexture();
	}
	else
		this->texture = in_texture;

	if (in_helmtexture == 0xFF) {
		if (in_race <= 12 || in_race == 128 || in_race == 130 || in_race == 330 || in_race == 522)
			this->helmtexture = 0xFF;
		else if (in_texture != 0xFF)
			this->helmtexture = in_texture;
		else
			this->helmtexture = GetHelmTexture();
	}
	else
		this->helmtexture = in_helmtexture;

	if (in_haircolor == 0xFF)
		this->haircolor = GetHairColor();
	else
		this->haircolor = in_haircolor;

	if (in_beardcolor == 0xFF)
		this->beardcolor = GetBeardColor();
	else
		this->beardcolor = in_beardcolor;

	if (in_eyecolor1 == 0xFF)
		this->eyecolor1 = GetEyeColor1();
	else
		this->eyecolor1 = in_eyecolor1;

	if (in_eyecolor2 == 0xFF)
		this->eyecolor2 = GetEyeColor2();
	else
		this->eyecolor2 = in_eyecolor2;

	if (in_hairstyle == 0xFF)
		this->hairstyle = GetHairStyle();
	else
		this->hairstyle = in_hairstyle;

	if (in_luclinface == 0xFF)
		this->luclinface = GetLuclinFace();
	else
		this->luclinface = in_luclinface;

	if (in_beard == 0xFF)
		this->beard	= GetBeard();
	else
		this->beard = in_beard;

	this->aa_title = 0xFF;

	if (in_drakkin_heritage == 0xFFFFFFFF)
		this->drakkin_heritage = GetDrakkinHeritage();
	else
		this->drakkin_heritage = in_drakkin_heritage;

	if (in_drakkin_tattoo == 0xFFFFFFFF)
		this->drakkin_tattoo = GetDrakkinTattoo();
	else
		this->drakkin_tattoo = in_drakkin_tattoo;

	if (in_drakkin_details == 0xFFFFFFFF)
		this->drakkin_details = GetDrakkinDetails();
	else
		this->drakkin_details = in_drakkin_details;

	if (in_size <= 0.0f)
		this->size = GetSize();
	else
		this->size = in_size;

	// Forces the feature information to be pulled from the Player Profile
	if (this->IsClient() && in_race == 0) {
		this->race = CastToClient()->GetBaseRace();
		this->gender = CastToClient()->GetBaseGender();
		this->texture = 0xFF;
		this->helmtexture = 0xFF;
		this->haircolor = CastToClient()->GetBaseHairColor();
		this->beardcolor = CastToClient()->GetBaseBeardColor();
		this->eyecolor1 = CastToClient()->GetBaseEyeColor();
		this->eyecolor2 = CastToClient()->GetBaseEyeColor();
		this->hairstyle = CastToClient()->GetBaseHairStyle();
		this->luclinface = CastToClient()->GetBaseFace();
		this->beard	= CastToClient()->GetBaseBeard();
		this->aa_title = 0xFF;
		this->drakkin_heritage = CastToClient()->GetBaseHeritage();
		this->drakkin_tattoo = CastToClient()->GetBaseTattoo();
		this->drakkin_details = CastToClient()->GetBaseDetails();
		switch(race){
			case OGRE:
				this->size = 9;
				break;
			case TROLL:
				this->size = 8;
				break;
			case VAHSHIR:
			case BARBARIAN:
				this->size = 7;
				break;
			case HALF_ELF:
			case WOOD_ELF:
			case DARK_ELF:
			case FROGLOK:
				this->size = 5;
				break;
			case DWARF:
				this->size = 4;
				break;
			case HALFLING:
			case GNOME:
				this->size = 3;
				break;
			default:
				this->size = 6;
				break;
		}
	}

	EQApplicationPacket* outapp = new EQApplicationPacket(OP_Illusion, sizeof(Illusion_Struct));
	memset(outapp->pBuffer, 0, sizeof(outapp->pBuffer));
	Illusion_Struct* is = (Illusion_Struct*) outapp->pBuffer;
	is->spawnid = this->GetID();
	strcpy(is->charname, GetCleanName());
	is->race = this->race;
	is->gender = this->gender;
	is->texture = this->texture;
	is->helmtexture = this->helmtexture;
	is->haircolor = this->haircolor;
	is->beardcolor = this->beardcolor;
	is->beard = this->beard;
	is->eyecolor1 = this->eyecolor1;
	is->eyecolor2 = this->eyecolor2;
	is->hairstyle = this->hairstyle;
	is->face = this->luclinface;
	//is->aa_title = this->aa_title;
	is->drakkin_heritage = this->drakkin_heritage;
	is->drakkin_tattoo = this->drakkin_tattoo;
	is->drakkin_details = this->drakkin_details;
	is->size = this->size;

	entity_list.QueueClients(this, outapp);
	safe_delete(outapp);
	mlog(CLIENT__SPELLS, "Illusion: Race = %i, Gender = %i, Texture = %i, HelmTexture = %i, HairColor = %i, BeardColor = %i, EyeColor1 = %i, EyeColor2 = %i, HairStyle = %i, Face = %i, DrakkinHeritage = %i, DrakkinTattoo = %i, DrakkinDetails = %i, Size = %f",
		this->race, this->gender, this->texture, this->helmtexture, this->haircolor, this->beardcolor, this->eyecolor1, this->eyecolor2, this->hairstyle, this->luclinface, this->drakkin_heritage, this->drakkin_tattoo, this->drakkin_details, this->size);
}

uint8 Mob::GetDefaultGender(uint16 in_race, uint8 in_gender) {
//std::cout << "Gender in: " << (int)in_gender << std::endl; // undefined cout [CODEBUG]
	if ((in_race > 0 && in_race <= GNOME )
		|| in_race == IKSAR || in_race == VAHSHIR || in_race == FROGLOK || in_race == DRAKKIN
		|| in_race == 15 || in_race == 50 || in_race == 57 || in_race == 70 || in_race == 98 || in_race == 118) {
		if (in_gender >= 2) {
			// Female default for PC Races
			return 1;
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

void Mob::QuestReward(Client *c, uint32 silver, uint32 gold, uint32 platinum) {

	EQApplicationPacket* outapp = new EQApplicationPacket(OP_Sound, sizeof(QuestReward_Struct));
	memset(outapp->pBuffer, 0, sizeof(outapp->pBuffer));
	QuestReward_Struct* qr = (QuestReward_Struct*) outapp->pBuffer;

	qr->from_mob = GetID();		// Entity ID for the from mob name
	qr->silver = silver;
	qr->gold = gold;
	qr->platinum = platinum;

	if(c)
		c->QueuePacket(outapp, false, Client::CLIENT_CONNECTED);

	safe_delete(outapp);
}

void Mob::CameraEffect(uint32 duration, uint32 intensity, Client *c, bool global) {


	if(global == true)
	{
		ServerPacket* pack = new ServerPacket(ServerOP_CameraShake, sizeof(ServerCameraShake_Struct));
		memset(pack->pBuffer, 0, sizeof(pack->pBuffer));
		ServerCameraShake_Struct* scss = (ServerCameraShake_Struct*) pack->pBuffer;
		scss->duration = duration;
		scss->intensity = intensity;
		worldserver.SendPacket(pack);
		safe_delete(pack);
		return;
	}

	EQApplicationPacket* outapp = new EQApplicationPacket(OP_CameraEffect, sizeof(Camera_Struct));
	memset(outapp->pBuffer, 0, sizeof(outapp->pBuffer));
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
	memset(outapp->pBuffer, 0, sizeof(outapp->pBuffer));
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

	// Make the new name unique and set it
	strn0cpy(temp_name, entity_list.MakeNameUnique(temp_name), 64);


	// Send the new name to all clients
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_MobRename, sizeof(MobRename_Struct));
	memset(outapp->pBuffer, 0, sizeof(outapp->pBuffer));
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
	if (_appearance != app) {
		_appearance = app;
		SendAppearancePacket(AT_Anim, GetAppearanceValue(app), true, iIgnoreSelf);
		if (this->IsClient() && this->IsAIControlled())
			SendAppearancePacket(AT_Anim, ANIM_FREEZE, false, false);
	}
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
	if (ownerid == 0 && this->IsNPC() && this->GetPetType() != petCharmed)
		this->Depop();
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

float Mob::Dist(const Mob &other) const {
	float xDiff = other.x_pos - x_pos;
	float yDiff = other.y_pos - y_pos;
	float zDiff = other.z_pos - z_pos;

	return sqrtf( (xDiff * xDiff)
				+ (yDiff * yDiff)
				+ (zDiff * zDiff) );
}

float Mob::DistNoZ(const Mob &other) const {
	float xDiff = other.x_pos - x_pos;
	float yDiff = other.y_pos - y_pos;

	return sqrtf( (xDiff * xDiff)
				+ (yDiff * yDiff) );
}

float Mob::DistNoRoot(const Mob &other) const {
	float xDiff = other.x_pos - x_pos;
	float yDiff = other.y_pos - y_pos;
	float zDiff = other.z_pos - z_pos;

	return ( (xDiff * xDiff)
			+ (yDiff * yDiff)
			+ (zDiff * zDiff) );
}

float Mob::DistNoRoot(float x, float y, float z) const {
	float xDiff = x - x_pos;
	float yDiff = y - y_pos;
	float zDiff = z - z_pos;

	return ( (xDiff * xDiff)
			+ (yDiff * yDiff)
			+ (zDiff * zDiff) );
}

float Mob::DistNoRootNoZ(float x, float y) const {
	float xDiff = x - x_pos;
	float yDiff = y - y_pos;

	return ( (xDiff * xDiff) + (yDiff * yDiff) );
}

float Mob::DistNoRootNoZ(const Mob &other) const {
	float xDiff = other.x_pos - x_pos;
	float yDiff = other.y_pos - y_pos;

	return ( (xDiff * xDiff) + (yDiff * yDiff) );
}

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

bool Mob::PlotPositionAroundTarget(Mob* target, float &x_dest, float &y_dest, float &z_dest, bool lookForAftArc) {
	bool Result = false;

	if(target) {
		float look_heading = 0;

		if(lookForAftArc)
			look_heading = GetReciprocalHeading(target);
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
				target->CastToClient()->MovePC(zone->GetZoneID(), zone->GetInstanceID(), x_pos, y_pos, z_pos, target->GetHeading(), 0, SummonPC);
			}
			else {
#ifdef BOTS
				if(target && target->IsBot()) {
					// set pre summoning info to return to (to get out of melee range for caster)
					target->CastToBot()->SetHasBeenSummoned(true);
					target->CastToBot()->SetPreSummonX(target->GetX());
					target->CastToBot()->SetPreSummonY(target->GetY());
					target->CastToBot()->SetPreSummonZ(target->GetZ());

				}
#endif //BOTS
				target->GMMove(x_pos, y_pos, z_pos, target->GetHeading());
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
		bFound = hate_list.RemoveEnt(mob);
		if(hate_list.IsEmpty())
		{
			AI_Event_NoLongerEngaged();
			zone->DelAggroMob();
		}
	}
	if(GetTarget() == mob)
	{
		SetTarget(hate_list.GetTop(this));
	}

	return bFound;
}

void Mob::WipeHateList()
{
	if(IsEngaged())
	{
		hate_list.Wipe();
		AI_Event_NoLongerEngaged();
	}
	else
	{
		hate_list.Wipe();
	}
}

uint32 Mob::RandomTimer(int min,int max) {
	int r = 14000;
	if(min != 0 && max != 0 && min < max)
	{
		r = MakeRandomInt(min, max);
	}
	return r;
}

uint32 NPC::GetEquipment(uint8 material_slot) const
{
	if(material_slot > 8)
		return 0;
	int invslot = Inventory::CalcSlotFromMaterial(material_slot);
	if (invslot == -1)
		return 0;
	return equipment[invslot];
}

void Mob::SendWearChange(uint8 material_slot)
{
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_WearChange, sizeof(WearChange_Struct));
	WearChange_Struct* wc = (WearChange_Struct*)outapp->pBuffer;

	wc->spawn_id = GetID();
	wc->material = GetEquipmentMaterial(material_slot);
	wc->elite_material = IsEliteMaterialItem(material_slot);
	wc->color.color = GetEquipmentColor(material_slot);
	wc->wear_slot_id = material_slot;

	entity_list.QueueClients(this, outapp);
	safe_delete(outapp);
}

void Mob::SendTextureWC(uint8 slot, uint16 texture, uint32 hero_forge_model, uint32 elite_material, uint32 unknown06, uint32 unknown18)
{
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_WearChange, sizeof(WearChange_Struct));
	WearChange_Struct* wc = (WearChange_Struct*)outapp->pBuffer;

	wc->spawn_id = this->GetID();
	wc->material = texture;
	if (this->IsClient())
		wc->color.color = GetEquipmentColor(slot);
	else
		wc->color.color = this->GetArmorTint(slot);
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
	wc->color.color = color;
	wc->wear_slot_id = material_slot;

	entity_list.QueueClients(this, outapp);
	safe_delete(outapp);
}

void Mob::WearChange(uint8 material_slot, uint16 texture, uint32 color)
{
	armor_tint[material_slot] = color;

	EQApplicationPacket* outapp = new EQApplicationPacket(OP_WearChange, sizeof(WearChange_Struct));
	WearChange_Struct* wc = (WearChange_Struct*)outapp->pBuffer;

	wc->spawn_id = this->GetID();
	wc->material = texture;
	wc->color.color = color;
	wc->wear_slot_id = material_slot;

	entity_list.QueueClients(this, outapp);
	safe_delete(outapp);
}

int32 Mob::GetEquipmentMaterial(uint8 material_slot) const
{
	const Item_Struct *item;
	int ornamentationAugtype = RuleI(Character, OrnamentationAugmentType);
	item = database.GetItem(GetEquipment(material_slot));
	if(item != 0)
	{
		if	// for primary and secondary we need the model, not the material
		(
			material_slot == MaterialPrimary ||
			material_slot == MaterialSecondary
		)
		{
			if (this->IsClient()){
				int currMatslot = MaterialPrimary == material_slot ? MainPrimary : MainSecondary;
				const ItemInst* inst = CastToClient()->m_inv[currMatslot];
				if (inst && inst->GetOrnamentationAug(ornamentationAugtype)) {
					item = inst->GetOrnamentationAug(ornamentationAugtype)->GetItem();
					return atoi(&item->IDFile[2]);
				}
				else {
					if (strlen(item->IDFile) > 2)
						return atoi(&item->IDFile[2]);
					else	//may as well try this, since were going to 0 anyways
						return item->Material;
				}
			}
			else {
				if (strlen(item->IDFile) > 2)
					return atoi(&item->IDFile[2]);
				else	//may as well try this, since were going to 0 anyways
					return item->Material;
			}
		}
		else
		{
			return item->Material;
		}
	}

	return 0;
}

uint32 Mob::GetEquipmentColor(uint8 material_slot) const
{
	const Item_Struct *item;

	item = database.GetItem(GetEquipment(material_slot));
	if(item != 0)
	{
		return item->Color;
	}

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
// solar: this is like the above, but the first parameter is a string id
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
void Mob::Warp( float x, float y, float z )
{
	if(IsNPC()) {
		entity_list.ProcessMove(CastToNPC(), x, y, z);
	}

	x_pos = x;
	y_pos = y;
	z_pos = z;

	Mob* target = GetTarget();
	if (target) {
		FaceTarget( target );
	}

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
	//std::cout<<"Tlevel: "<<(int)tlevel<<std::endl; // cout undefined [CODEBUG]
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

	return(casttime);
}

void Mob::ExecWeaponProc(const ItemInst *inst, uint16 spell_id, Mob *on) {
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
			mlog(CLIENT__SPELLS, "Player %s, Weapon Procced invalid spell %u", this->GetName(), spell_id);
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

	if(twinproc_chance && (MakeRandomInt(0,99) < twinproc_chance))
		twinproc = true;

	if (IsBeneficialSpell(spell_id)) {
		SpellFinished(spell_id, this, 10, 0, -1, spells[spell_id].ResistDiff, true);
		if(twinproc)
			SpellOnTarget(spell_id, this, false, false, 0, true);
	}
	else if(!(on->IsClient() && on->CastToClient()->dead)) { //dont proc on dead clients
		SpellFinished(spell_id, on, 10, 0, -1, spells[spell_id].ResistDiff, true);
		if(twinproc)
			SpellOnTarget(spell_id, on, false, false, 0, true);
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
	if (spellbonuses.haste < 0) {
		if (-spellbonuses.haste <= spellbonuses.inhibitmelee)
			return 100 - spellbonuses.inhibitmelee;
		else
			return 100 + spellbonuses.haste;
	}

	if (spellbonuses.haste == 0 && spellbonuses.inhibitmelee)
		return 100 - spellbonuses.inhibitmelee;

	int h = 0;
	int cap = 0;
	int level = GetLevel();

	if (spellbonuses.haste)
		h += spellbonuses.haste - spellbonuses.inhibitmelee;
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
		Map::Vertex me;
		me.x = new_x;
		me.y = new_y;
		me.z = z_pos+z_offset;
		Map::Vertex hit;
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
		Map::Vertex me;
		me.x = new_x;
		me.y = new_y;
		me.z = z_pos+z_offset;
		Map::Vertex hit;
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
				if (val < 0 && abs(val) > worst_snare)
					worst_snare = abs(val);
			}
		}
	}

	return worst_snare;
}

void Mob::TriggerDefensiveProcs(const ItemInst* weapon, Mob *on, uint16 hand, int damage)
{
	if (!on)
		return;

	on->TryDefensiveProc(weapon, this, hand);

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

void Mob::SetDeltas(float dx, float dy, float dz, float dh) {
	delta_x = dx;
	delta_y = dy;
	delta_z = dz;
	delta_heading = static_cast<int>(dh);
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

	if (aa_trigger && IsClient()){
		//focus_spell = aaid
		trigger_spell_id = CastToClient()->CalcAAFocus(focusTriggerOnCast, focus_spell, spell_id);

		if(IsValidSpell(trigger_spell_id) && GetTarget())
			SpellFinished(trigger_spell_id, GetTarget(), 10, 0, -1, spells[trigger_spell_id].ResistDiff);
	}

	else{
		trigger_spell_id = CalcFocusEffect(focusTriggerOnCast, focus_spell, spell_id);

		if(IsValidSpell(trigger_spell_id) && GetTarget()){
			SpellFinished(trigger_spell_id, GetTarget(),10, 0, -1, spells[trigger_spell_id].ResistDiff);
			CheckNumHitsRemaining(NUMHIT_MatchingSpells,0, focus_spell);
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
				if(MakeRandomInt(0, trig_chance) <= spells[spell_id].base[i])
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
		if(MakeRandomInt(0, 100) <= spells[spell_id].base[effect])
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
							int count = hate_list.SummonedPetCount(this);
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
			if(MakeRandomInt(0, 100) <= focus)
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
					if(MakeRandomInt(0, 100) <= focus)
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
			CheckNumHitsRemaining(NUMHIT_MatchingSpells, tmp_buffslot);
	}

	value += GetSpellResistTypeDmgBonus(); //C!Kayen
	value += spellbonuses.IncomingSpellDmgPct[GetSpellResistType(spell_id)] + spellbonuses.IncomingSpellDmgPct[HIGHEST_RESIST]; //C!Kayen
	return value;
}

int16 Mob::GetSkillDmgTaken(const SkillUseTypes skill_used)
{
	int skilldmg_mod = 0;

	int16 MeleeVuln = spellbonuses.MeleeVulnerability + itembonuses.MeleeVulnerability + aabonuses.MeleeVulnerability;
	
	// All skill dmg mod + Skill specific
	skilldmg_mod += itembonuses.SkillDmgTaken[HIGHEST_SKILL+1] + spellbonuses.SkillDmgTaken[HIGHEST_SKILL+1] +
					itembonuses.SkillDmgTaken[skill_used] + spellbonuses.SkillDmgTaken[skill_used];

	//Innate SetSkillDamgeTaken(skill,value)
	if ((SkillDmgTaken_Mod[skill_used]) || (SkillDmgTaken_Mod[HIGHEST_SKILL+1]))
		skilldmg_mod += SkillDmgTaken_Mod[skill_used] + SkillDmgTaken_Mod[HIGHEST_SKILL+1];

	skilldmg_mod += MeleeVuln;
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
	if(target == nullptr || !IsValidSpell(spell_id))
		return;

	int focus_spell = CastToClient()->GetSympatheticFocusEffect(focusSympatheticProc,spell_id);

		if(IsValidSpell(focus_spell)){
			int focus_trigger = spells[focus_spell].base2[0];
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

			CheckNumHitsRemaining(NUMHIT_MatchingSpells, 0, focus_spell);
		}
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
						if(MakeRandomInt(0,99) < spells[spell_id].base[i])
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
			if(MakeRandomInt(0, 99) < static_cast<int>(aabonuses.SpellOnKill[i + 1]))
				SpellFinished(aabonuses.SpellOnKill[i], this, 10, 0, -1, spells[aabonuses.SpellOnKill[i]].ResistDiff);
		}

		if(itembonuses.SpellOnKill[i] && IsValidSpell(itembonuses.SpellOnKill[i]) && (level >= itembonuses.SpellOnKill[i + 2])){
			if(MakeRandomInt(0, 99) < static_cast<int>(itembonuses.SpellOnKill[i + 1]))
				SpellFinished(itembonuses.SpellOnKill[i], this, 10, 0, -1, spells[aabonuses.SpellOnKill[i]].ResistDiff);
		}

		if(spellbonuses.SpellOnKill[i] && IsValidSpell(spellbonuses.SpellOnKill[i]) && (level >= spellbonuses.SpellOnKill[i + 2])) {
			if(MakeRandomInt(0, 99) < static_cast<int>(spellbonuses.SpellOnKill[i + 1]))
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
			if(MakeRandomInt(0, 99) < static_cast<int>(aabonuses.SpellOnDeath[i + 1])) {
				SpellFinished(aabonuses.SpellOnDeath[i], this, 10, 0, -1, spells[aabonuses.SpellOnDeath[i]].ResistDiff);
			}
		}

		if(itembonuses.SpellOnDeath[i] && IsValidSpell(itembonuses.SpellOnDeath[i])) {
			if(MakeRandomInt(0, 99) < static_cast<int>(itembonuses.SpellOnDeath[i + 1])) {
				SpellFinished(itembonuses.SpellOnDeath[i], this, 10, 0, -1, spells[itembonuses.SpellOnDeath[i]].ResistDiff);
			}
		}

		if(spellbonuses.SpellOnDeath[i] && IsValidSpell(spellbonuses.SpellOnDeath[i])) {
			if(MakeRandomInt(0, 99) < static_cast<int>(spellbonuses.SpellOnDeath[i + 1])) {
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

	dmg_mod += GetScaleDamageNumhits(); //C!Kayen

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
		mlog(COMBAT__DAMAGE, "Melee lifetap healing for %d damage.", damage);

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

	if(chance && MakeRandomInt(0, 99) < chance)
		return true;

	return false;
}

void Mob::SpellProjectileEffect()
{
	bool time_disable = false;

	for (int i = 0; i < MAX_SPELL_PROJECTILE; i++) {

		if (projectile_increment[i] == 0){
			continue;
		}

		Mob* target = entity_list.GetMobID(projectile_target_id[i]);

		float dist = 0;

		if (target)
			dist = target->CalculateDistance(projectile_x[i], projectile_y[i],  projectile_z[i]);

		int increment_end = 0;
		increment_end = static_cast<int>(dist / 10) - 1; //This pretty accurately determines end time for speed for 1.5 and timer of 250 ms

		if (increment_end <= projectile_increment[i]){

			if (target && IsValidSpell(projectile_spell_id[i]))
				SpellOnTarget(projectile_spell_id[i], target, false, true, spells[projectile_spell_id[i]].ResistDiff, true);

			projectile_spell_id[i] = 0;
			projectile_target_id[i] = 0;
			projectile_x[i] = 0, projectile_y[i] = 0, projectile_z[i] = 0;
			projectile_increment[i] = 0;
			time_disable = true;
		}

		else {
			projectile_increment[i]++;
			time_disable = false;
		}
	}

	if (time_disable)
		projectile_timer.Disable();
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

					amount = fabs(value) / (100.0f); // to bring the values in line, arbitarily picked

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

	if((fabs(my_x - cur_x) > 0.01) || (fabs(my_y - cur_y) > 0.01)) {
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
	if (skill_num <= HIGHEST_SKILL)
		SkillDmgTaken_Mod[skill_num] = value;


	else if (skill_num == 255 || skill_num == -1)
		SkillDmgTaken_Mod[HIGHEST_SKILL+1] = value;
}

int16 Mob::GetModSkillDmgTaken(const SkillUseTypes skill_num)
{
	if (skill_num <= HIGHEST_SKILL)
		return SkillDmgTaken_Mod[skill_num];

	else if (skill_num == 255 || skill_num == -1)
		return SkillDmgTaken_Mod[HIGHEST_SKILL+1];

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
			return Skill1HPiercing; // change to 2HPiercing once activated
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

	int spell_level = spells[spell_id].classes[(GetClass()%16) - 1];
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

	if (fabs(angle - heading) <= 80.0)
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

	float y_diff = fabs(this_y - mob_y);
	float x_diff = fabs(this_x - mob_x);
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
		if (id == "classes") {stat = spells[spell_id].classes[slot]; }
		else if (id == "dieties") {stat = spells[spell_id].deities[slot];}
	}

	if (slot < 12){
		if (id == "base") {stat = spells[spell_id].base[slot];}
		else if (id == "base2") {stat = spells[spell_id].base2[slot];}
		else if (id == "max") {stat = spells[spell_id].max[slot];}
		else if (id == "formula") {spells[spell_id].formula[slot];}
		else if (id == "effectid") {spells[spell_id].effectid[slot];}
	}

	if (slot < 4){
		if (id == "components") { spells[spell_id].components[slot];}
		else if (id == "component_counts") {spells[spell_id].component_counts[slot];} 
		else if (id == "NoexpendReagent") {spells[spell_id].NoexpendReagent[slot];}
	}

	if (id == "range") {stat = static_cast<int32>(spells[spell_id].range); }
	else if (id == "aoerange") {stat = static_cast<int32>(spells[spell_id].aoerange);}
	else if (id == "pushback") {stat = static_cast<int32>(spells[spell_id].pushback);}
	else if (id == "pushup") {stat = static_cast<int32>(spells[spell_id].pushup);}
	else if (id == "cast_time") {stat = spells[spell_id].cast_time;}
	else if (id == "recovery_time") {stat = spells[spell_id].recovery_time;}
	else if (id == "recast_time") {stat = spells[spell_id].recast_time;}
	else if (id == "buffdurationformula") {stat = spells[spell_id].buffdurationformula;}
	else if (id == "buffduration") {stat = spells[spell_id].buffduration;}
	else if (id == "AEDuration") {stat = spells[spell_id].AEDuration;}
	else if (id == "mana") {stat = spells[spell_id].mana;}
	//else if (id == "LightType") {stat = spells[spell_id].LightType;} - Not implemented
	else if (id == "goodEffect") {stat = spells[spell_id].goodEffect;}
	else if (id == "Activated") {stat = spells[spell_id].Activated;}
	else if (id == "resisttype") {stat = spells[spell_id].resisttype;}
	else if (id == "targettype") {stat = spells[spell_id].targettype;}
	else if (id == "basedeiff") {stat = spells[spell_id].basediff;}
	else if (id == "skill") {stat = spells[spell_id].skill;}
	else if (id == "zonetype") {stat = spells[spell_id].zonetype;}
	else if (id == "EnvironmentType") {stat = spells[spell_id].EnvironmentType;}
	else if (id == "TimeOfDay") {stat = spells[spell_id].TimeOfDay;}
	else if (id == "CastingAnim") {stat = spells[spell_id].CastingAnim;}
	else if (id == "SpellAffectIndex") {stat = spells[spell_id].SpellAffectIndex; }
	else if (id == "disallow_sit") {stat = spells[spell_id].disallow_sit; }
	//else if (id == "spellanim") {stat = spells[spell_id].spellanim; } - Not implemented
	else if (id == "uninterruptable") {stat = spells[spell_id].uninterruptable; }
	else if (id == "ResistDiff") {stat = spells[spell_id].ResistDiff; }
	else if (id == "dot_stacking_exemp") {stat = spells[spell_id].dot_stacking_exempt; }
	else if (id == "RecourseLink") {stat = spells[spell_id].RecourseLink; }
	else if (id == "no_partial_resist") {stat = spells[spell_id].no_partial_resist; }
	else if (id == "short_buff_box") {stat = spells[spell_id].short_buff_box; }
	else if (id == "descnum") {stat = spells[spell_id].descnum; }
	else if (id == "effectdescnum") {stat = spells[spell_id].effectdescnum; }
	else if (id == "npc_no_los") {stat = spells[spell_id].npc_no_los; }
	else if (id == "reflectable") {stat = spells[spell_id].reflectable; }
	else if (id == "bonushate") {stat = spells[spell_id].bonushate; }
	else if (id == "EndurCost") {stat = spells[spell_id].EndurCost; }
	else if (id == "EndurTimerIndex") {stat = spells[spell_id].EndurTimerIndex; }
	else if (id == "IsDisciplineBuf") {stat = spells[spell_id].IsDisciplineBuff; }
	else if (id == "HateAdded") {stat = spells[spell_id].HateAdded; }
	else if (id == "EndurUpkeep") {stat = spells[spell_id].EndurUpkeep; }
	else if (id == "numhitstype") {stat = spells[spell_id].numhitstype; }
	else if (id == "numhits") {stat = spells[spell_id].numhits; }
	else if (id == "pvpresistbase") {stat = spells[spell_id].pvpresistbase; }
	else if (id == "pvpresistcalc") {stat = spells[spell_id].pvpresistcalc; }
	else if (id == "pvpresistcap") {stat = spells[spell_id].pvpresistcap; }
	else if (id == "spell_category") {stat = spells[spell_id].spell_category; }
	else if (id == "can_mgb") {stat = spells[spell_id].can_mgb; }
	else if (id == "dispel_flag") {stat = spells[spell_id].dispel_flag; }
	else if (id == "MinResist") {stat = spells[spell_id].MinResist; }
	else if (id == "MaxResist") {stat = spells[spell_id].MaxResist; }
	else if (id == "viral_targets") {stat = spells[spell_id].viral_targets; }
	else if (id == "viral_timer") {stat = spells[spell_id].viral_timer; }
	else if (id == "NimbusEffect") {stat = spells[spell_id].NimbusEffect; }
	else if (id == "directional_start") {stat = static_cast<int32>(spells[spell_id].directional_start); }
	else if (id == "directional_end") {stat = static_cast<int32>(spells[spell_id].directional_end); }
	else if (id == "not_extendable") {stat = spells[spell_id].not_extendable; }
	else if (id == "suspendable") {stat = spells[spell_id].suspendable; }
	else if (id == "viral_range") {stat = spells[spell_id].viral_range; }
	else if (id == "spellgroup") {stat = spells[spell_id].spellgroup; }
	else if (id == "rank") {stat = spells[spell_id].rank; }
	else if (id == "powerful_flag") {stat = spells[spell_id].powerful_flag; }
	else if (id == "CastRestriction") {stat = spells[spell_id].CastRestriction; }
	else if (id == "AllowRest") {stat = spells[spell_id].AllowRest; }
	else if (id == "InCombat") {stat = spells[spell_id].InCombat; }
	else if (id == "OutofCombat") {stat = spells[spell_id].OutofCombat; }
	else if (id == "aemaxtargets") {stat = spells[spell_id].aemaxtargets; }
	else if (id == "maxtargets") {stat = spells[spell_id].maxtargets; }
	else if (id == "persistdeath") {stat = spells[spell_id].persistdeath; }
	else if (id == "min_dist") {stat = static_cast<int32>(spells[spell_id].min_dist); }
	else if (id == "min_dist_mod") {stat = static_cast<int32>(spells[spell_id].min_dist_mod); }
	else if (id == "max_dist") {stat = static_cast<int32>(spells[spell_id].max_dist); }
	else if (id == "min_range") {stat = static_cast<int32>(spells[spell_id].min_range); }
	else if (id == "DamageShieldType") {stat = spells[spell_id].DamageShieldType; }
	
	return stat;
}

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

	if (spells[spell_id].aemaxtargets && spells[spell_id].maxtargets)
		hit_count = spells[spell_id].maxtargets; //Hijack this field if set WITH aemaxtargets, should equal or > aemaxtargets. 

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

			if (!spells[spell_id].maxtargets)
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

bool Mob::RectangleDirectional(uint16 spell_id, int16 resist_adjust, bool FromTarget, Mob *target)
{
	/*
	float ae_width = spells[spell_id].range; //This is the width of the AE that will hit targets.
	float radius = spells[spell_id].aoerange; //This is total area checked for targets.
	int maxtargets = spells[spell_id].aemaxtargets; //C!Kayen
	*/

	float ae_width = spells[spell_id].aoerange; //This is the width of the AE that will hit targets.
	float radius = spells[spell_id].range; //This is total area checked for targets.
	int maxtargets = spells[spell_id].aemaxtargets; //C!Kayen

	bool taget_exclude_npc = false; //False by default!
			
	bool target_client_only = false;

	if (IsBeneficialSpell(spell_id) && IsClient())
		target_client_only = true;

	if (!IsClient() && taget_exclude_npc)
		target_client_only = true;

	//Shout("Start Cube ae_width %.2f AOE range %.2f min range %.2f", ae_width, radius, spells[spell_id].min_range);
	
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
		
		//(*iter)->Shout("In AOE range");

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
				//(*iter)->Shout("In BEAM range D: [%.2f]" ,d);

				if(CheckLosFN((*iter)) || spells[spell_id].npc_no_los) {
					(*iter)->CalcSpellPowerDistanceMod(spell_id, 0, this);
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
	
	return true;
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

bool Mob::AACastSpell(uint16 spell_id, uint16 target_id)
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

	if(!CastSpell(spell_id, target_id))
		return false;

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
	//This value is always defined as a NEGATIVE in the database when doing CasterRestrictions.
	//*NOTE IMPLMENTED YET FOR FROM CastRestriction Field - Will write as needed.
	/*If return TRUE spell met all restrictions and can continue (this = CASTER).
	This check is used when the spell_new field CastRestriction is defined OR spell effect '0'(DD/Heal) has a defined limit

	Range 20000 - 200010	: Limit to CastFromCrouch Interval Projectile
	THIS IS A WORK IN PROGRESS
	*/ 

	if (value >= 0)
		return true;

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

		dist_targ = center->DistNoRoot(*curmob);

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

		dist_targ = center->DistNoRoot(*curmob);

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

void Mob::SetMoving(bool move, bool in_combat_range) 
{ 
	moving = move; 
	delta_x = 0; delta_y = 0; delta_z = 0; delta_heading = 0; 
	
	if (!in_combat_range && !moving)
		SetMomentum(0);
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

bool Mob::SingleTargetSpellInAngle(uint16 spell_id, Mob* spell_target){

	if (!spells[spell_id].directional_start && !spells[spell_id].directional_end)
		return true;
		
	if (spell_target){

		if (spell_target == this)
			return true;

		if (!spell_target->InAngleMob(this, spells[spell_id].directional_start,spells[spell_id].directional_end)){
			//Message_StringID(13,CANT_SEE_TARGET);
			Message(MT_SpellFailure, "You must face your target to use this ability!");
			return false;
		}
	}

	return true;
}

bool Mob::SpellDirectionalTarget(uint16 spell_id, Mob *target)
{
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
	TemporaryPets(spell_id, nullptr, pet_name); //Create pet.
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
					
					/* Min Distance required to use spell. - Disabled makes it clunky to use atm due to inability to make target ring red.
					if (CalculateDistance(GetTargetRingX(), GetTargetRingY(), GetTargetRingZ()) < 50){
						Message(MT_SpellFailure, "You portal is too unstable, and collapses.");
						return false;
					}
					*/

					CastToClient()->MovePC(zone->GetZoneID(), zone->GetInstanceID(), GetTargetRingX(), GetTargetRingY(), GetTargetRingZ(), GetHeading()*2);
					Message(MT_Spells, "You enter a temporal rift.");
					SendSpellEffect(spells[spell_id].spellanim, 4000, 0,true, false);
				}
				else
					GMMove(GetTargetRingX(), GetTargetRingY(), GetTargetRingZ(), GetHeading());
			}
		}
	}

	return true;
}

void Mob::SetTargetLocationLoc(uint16 target_id, uint16 spell_id)
{
	if (spells[spell_id].targettype == ST_TargetLocation){
		Mob* target = entity_list.GetMob(target_id);
		if (target) {
			targetring_x = target->GetX();
			targetring_y = target->GetY();
			targetring_z = target->GetZ();
		}
		else {
			targetring_x = 0.0f;
			targetring_y = 0.0f;
			targetring_z = 0.0f;
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

	TypesTemporaryPets(GetProjectileTargetRingPetID(), nullptr, "#",duration, false);
	NPC* temppet = nullptr;
	//temppet = GetTempPetByTypeID(GetProjectileTargetRingPetID(), true); //NOT USED ANYMORE- Save for now.
	temppet = entity_list.GetTempPetByNPCTypeID(GetProjectileTargetRingPetID(), GetID(), true);
	
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
		Shout("DEBUG::ProjectileTargetRing: Critical error no temppet (%i) Found in database", GetProjectileTargetRingPetID());

	return false;
}

NPC *EntityList::GetTempPetByNPCTypeID(uint32 npc_id, uint16 ownerid, bool SetVarTargetRing)
{
	if (npc_id == 0 || npc_list.empty())
		return nullptr;

	auto it = npc_list.begin();
	while (it != npc_list.end()) {
		if (it->second->GetNPCTypeID() == npc_id){
			if (it->second->GetSwarmOwner() == ownerid){
				if (SetVarTargetRing){
					if (!it->second->IsProjectilePet()){
						it->second->SetProjectilePet(true);
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
		if (spell_id == 2006){ //This will likely need to be adjusted.
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
		return;;

	for (int i = 0; i < MAX_SPELL_PROJECTILE; i++) {
	
		if (!projectile_increment_ring[i])
			continue;
		
		if (projectile_increment_ring[i] > projectile_hit_ring[i]){
		//Shout("Inc %i Hit %i", projectile_increment_ring[i], projectile_hit_ring[i]);

			Mob* target = entity_list.GetMobID(projectile_target_id_ring[i]);
			if (target){
				uint16 p_spell_id = projectile_spell_id_ring[i];
				if (IsValidSpell(p_spell_id)){
					if (spells[p_spell_id].powerful_flag){ //Powerful Flag denotes 'Spell Projectile'
						entity_list.AESpell(this, target, p_spell_id, false, spells[p_spell_id].ResistDiff);

						if (HasProjectileAESpellHitTarget())
							TryApplyEffectProjectileHit(p_spell_id);
						else
							ProjectileTargetRingFailMessage(p_spell_id);
						
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

void Mob::TryApplyEffectProjectileHit(uint16 spell_id)
{
	if(!IsValidSpell(spell_id))
		return;

	for(int i = 0; i < EFFECT_COUNT; i++){
		if (spells[spell_id].effectid[i] == SE_ApplyEffectProjectileHit){
			if(MakeRandomInt(0, 100) <= spells[spell_id].base[i]){

				if (!GetCastFromCrouchIntervalProj())
					SpellFinished(spells[spell_id].base2[i], this, 10, 0, -1, spells[spell_id].ResistDiff);

				else {
					for(int j = 0; j < GetCastFromCrouchIntervalProj(); j++){
						SpellFinished(spells[spell_id].base2[i], this, 10, 0, -1, spells[spell_id].ResistDiff);
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

//#### C!Projectile2

bool Mob::TrySpellProjectile2(Mob* spell_target,  uint16 spell_id){

	if (!spell_target || !IsValidSpell(spell_id))
		return false;
	//Note: Field209 / powerful_flag : Used as Speed Variable (in MS > 500) and to flag TargetType Ring/Location as a Projectile
	//Note: pvpresistbase : Used to set tilt

	int bolt_id = -1;

	//Make sure there is an avialable bolt to be cast.
	for (int i = 0; i < MAX_SPELL_PROJECTILE; i++) {
		if (projectile_spell_id[i] == 0){
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
		projectile_spell_id[bolt_id] = spell_id;
		projectile_target_id[bolt_id] = spell_target->GetID();
		projectile_x[bolt_id] = GetX(), projectile_y[bolt_id] = GetY(), projectile_z[bolt_id] = GetZ();
		projectile_increment[bolt_id] = 1;
		SetProjectile(true);
	}

	SkillUseTypes skillinuse;

	if (caster_anim != 44) //44 is standard 'nuke' spell animation.
		skillinuse = static_cast<SkillUseTypes>(caster_anim);
	else
		skillinuse = SkillArchery;

	if (SendProjectile){

		ProjectileAnimation(spell_target,0, false, speed,angle,tilt,arc, item_IDFile,skillinuse);

		//Enchanter triple blade effect - Requires adjusting angle based on heading to get correct appearance.
		if (spells[spell_id].spellgroup == 2006){ //This will likely need to be adjusted.
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

void Mob::SpellProjectileEffect2()
{
	if (!HasProjectile())
		return;;

	for (int i = 0; i < MAX_SPELL_PROJECTILE; i++) {

		if (projectile_increment[i] == 0){
			continue;
		}
		
		Mob* target = entity_list.GetMobID(projectile_target_id[i]);
		
		float distance = 0.0f;
		
		if (target) 
				distance = target->CalculateDistance(projectile_x[i], projectile_y[i],  projectile_z[i]);

		float hit = 0.0f;
		float dist_mod = 270.0f;
		float speed_mod = 0.0f;
		float projectile_speed_ring = static_cast<float>(spells[projectile_spell_id[i]].powerful_flag) / 1000.0f;
	
		if (projectile_speed_ring >= 0.5f && projectile_speed_ring < 1.0f)
			speed_mod = 175.0f - ((projectile_speed_ring - 0.5f) * (175.0f - 100.0f));
		else if (projectile_speed_ring >= 1.0f && projectile_speed_ring < 2.0f)
			speed_mod = 100.0f - ((projectile_speed_ring - 1.0f) * (100.0f - 70.0f));
		else if (projectile_speed_ring >= 2.0f && projectile_speed_ring < 3.0f)
			speed_mod = 70.0f - ((projectile_speed_ring - 2.0f) * (70.0f - 60.0f));
		else if (projectile_speed_ring >= 3.0f && projectile_speed_ring < 4.0f)
			speed_mod = 60.0f - ((projectile_speed_ring - 3.0f) * (60.0f - 50.0f));
		else if (projectile_speed_ring >= 4.0f && projectile_speed_ring <= 5.0f)
			speed_mod = 50.0f - ((projectile_speed_ring - 4.0f) * (50.0f - 40.0f));

		dist_mod = (dist_mod * speed_mod) / 100.0f;
		hit = (distance * dist_mod) / 100;

		//Shout("A Proj Speed %.2f Distance %.2f SpeedMod %.2f DistMod %.2f HIT [%.2f]", speed, distance, speed_mod, dist_mod, hit);
		//Close Distance Modifiers
		/*
		if (distance <= 35 && distance > 25)
			hit *= 1.6f;
		if (distance <= 25 && distance > 15)
			hit *= 1.8f;
		if (distance <= 15) 
			hit *= 2.0f;
		*/

		uint16 increment = static_cast<int>(hit);

		if (increment <= projectile_increment[i]){

			if (target && IsValidSpell(projectile_spell_id[i]))
				SpellOnTarget(projectile_spell_id[i], target, false, true, spells[projectile_spell_id[i]].ResistDiff, true);

			projectile_spell_id[i] = 0;
			projectile_target_id[i] = 0;
			projectile_x[i] = 0, projectile_y[i] = 0, projectile_z[i] = 0;
			projectile_increment[i] = 0;
			SetProjectile(false);
		}

		else {
			projectile_increment[i]++;
		}
	}
}

bool Mob::ExistsProjectile()
{
	for (int i = 0; i < MAX_SPELL_PROJECTILE; i++) {
	
		if (projectile_increment[i])
			return true;
	}
	return false;
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

void Mob::CalcTotalBaseModifierCurrentHP(int32 &damage, uint16 spell_id, Mob* caster, int effectid)
{
	int mod = 0;
	mod += GetSpellPowerAmtHits(); //Scale based on how many targets were hit by spell prior to this target.
	mod += CalcSpellPowerHeightMod(damage, spell_id, caster);
	mod += CalcFromCrouchMod(damage, spell_id,caster, effectid);
	mod += CalcSpellPowerFromBuffSpellGroup(spell_id, caster);

	//Shout("DEBUG::CalcTotalBaseModifierCurrentHP :: PRE DMG %i Mod %i", damage,mod);
	if (mod)
		damage += damage*mod/100;

	//Shout("DEBUG::CalcTotalBaseModifierCurrentHP :: POST DMG %i Mod %i", damage,mod);
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
		//Shout("remain %i < flat_time_cmp %i", remain_time, flat_time_cmp);
		if (remain_time < flat_time_cmp) {

			std::string WT;

			//Build string to send if cast is not completed.
			if (remain_time > 35){

				WT = spells[CastingSpellID()].name;
				WT += " ";
				WT += "< ";
				WT += itoa(flat_time);
				WT += " >";
				//Shout("Remain Time %i :: %s", GetSpellEndTime().GetRemainingTime(), WT);
			
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
				//Shout("DoAdjustRecastTimer(): RemainTime %i RecastEnd %i   [%s]", RemainTime,recast_mem_spells[i], spells[m_pp.mem_spells[i]].name);
				//Shout("DoAdjustRecastTimer(): <%s> %i < %i", spells[m_pp.mem_spells[i]].name, RemainTime, recast_mem_spells[i]);
				if (RemainTime <= recast_mem_spells[i]){
					//Shout("DoAdjustRecastTimer::: Trigger (Adjust Time) [%i] NAME: %s", recast_mem_spells[i], spells[CastToClient()->m_pp.mem_spells[i]].name);
					
					//Cast the Refresh Spell specified in the array.
					if (IsValidSpell(refreshid_mem_spells[i])){
						//Shout("refreshid_mem_spells[i] = %i", refreshid_mem_spells[i]);
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

	//Shout("EffectAdjustRecastTimer::: Recast Adjust:[ %i] SpellRefresh ID: [%i] SpellGroup: [%i] Spell Group Limit Bool [%i]", recast_adjust, spellid_refresh, spellgroupadjust,SpellGroupLimited);
	if (recast_adjust){
		for(unsigned int i =0 ; i < MAX_PP_MEMSPELL; ++i) {
			memspell_id = m_pp.mem_spells[i];
			RemainTime = 0;
			if(IsValidSpell(memspell_id)) {
			//Shout("[%i]	pp.Group [%i] SpelltoAdjust.Group",i, spells[memspell_id].spellgroup, spellgroupadjust);

				if (SpellGroupLimited && (spellgroupadjust && (spells[memspell_id].spellgroup == spellgroupadjust)))
					RemainTime = GetPTimers().GetRemainingTime(pTimerSpellStart + CastToClient()->m_pp.mem_spells[i]);
				else if (!SpellGroupLimited)
					RemainTime = GetPTimers().GetRemainingTime(pTimerSpellStart + CastToClient()->m_pp.mem_spells[i]);

				//Shout("RemainTime %i  > recast_adjust %i", RemainTime, recast_adjust);
				if (RemainTime && (RemainTime > recast_adjust)){
					recast_mem_spells[i] = recast_adjust;
					refreshid_mem_spells[i] = spellid_refresh;
					//Shout("EffectAdjustRecastTimer::: <Activiated %s > Recast ADjust %i [ %i ] Remain Time: %i ", spells[memspell_id].name,recast_adjust,recast_mem_spells[i], RemainTime);
					SetAdjustRecastTimer(true);
					adjustrecast_timer.Start(1000);
				}
			}
		}
	}
	
	//Shout("%i SE_FcTimerRefresh:::: Remain %i", i, CastToClient()->GetPTimers().GetRemainingTime(pTimerSpellStart + CastToClient()->m_pp.mem_spells[i]));
	//Shout("%i SE_FcTimerRefresh:::: Expired %i",i,CastToClient()->GetPTimers().Expired(&database, pTimerSpellStart + spell_id, false));
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
	if (GetProjSpeed(spell_id)){ //Need to use seperate variable if dealing with projectile (clears at different time).
		SetCastFromCrouchIntervalProj(charge_interval);
		SetCastFromCrouchInterval(charge_interval); //For recast adjust code purposes.
	}
	else
		SetCastFromCrouchInterval(charge_interval);
	
	spellend_timer.Start(1);

	//Set Standing Apperance from duck/jump
	SendAppearancePacket(AT_Anim, ANIM_STAND);
	playeraction = 0;
	SetFeigned(false);
	BindWound(this, false, true);
	camp_timer.Disable();

	return true;
}

int32 Mob::CalcFromCrouchMod(int32 &damage, uint16 spell_id, Mob* caster, int effectid){

	if (!caster || (IsValidSpell(spell_id) && !spells[spell_id].cast_from_crouch))
		return 0;

	int32 interval = 0;
	if (GetProjSpeed(spell_id)){
		//See CasterRestriction - If these is true we fire MORE projectiles instead of scaling.
		if (spells[spell_id].base2[effectid] <= -20000 && spells[spell_id].base2[effectid] >= -20010)
			return 0;

		interval = caster->GetCastFromCrouchIntervalProj();
	}
	else
		interval = caster->GetCastFromCrouchInterval();

	if (!interval)
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
		if((spells[spell_id].LightType == 1) && !CastToClient()->IsSpectralBladeEquiped()){
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

	if ((strcmp(pettype, "reaper")) == 0){
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

	//2: Target RING - Determine if target pets spawn at location or path to location. [Limit value in SE_TemporaryPetNoAgggro]
	if (spells[spell_id].targettype == ST_Ring && (IsEffectInSpell(spell_id,SE_TemporaryPetsNoAggro))){
		
		int limit = 0;
		for(int i = 0; i < EFFECT_COUNT; i++){
			if (spells[spell_id].effectid[i] == SE_TemporaryPetsNoAggro)
				limit = spells[spell_id].base2[i];
		}

		if (!limit)
			GMMove(owner->GetTargetRingX(), owner->GetTargetRingY(), owner->GetTargetRingZ(), GetHeading(), true);
		else if (limit == 1)
			MoveTo(owner->GetTargetRingX(), owner->GetTargetRingY(), owner->GetTargetRingZ(), GetHeading(), true);
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
					if(MakeRandomInt(0, 100) <= spells[spell_id].base[i]) {
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
					if(MakeRandomInt(0, 100) <= spells[spell_id].base[i]) {
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
					if(MakeRandomInt(0, 100) <= spells[spell_id].base[i]) {
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
					if(MakeRandomInt(0, 100) <= spells[spell_id].base[i]) {
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

//C!SpellEffects :: SE_TryCastonSpellFinished

void Mob::TryCastonSpellFinished(Mob *target, uint16 spell_id)
{
	if(!IsClient() || target == nullptr || !IsValidSpell(spell_id))
		return;
	
	for(int i = 0; i < EFFECT_COUNT; i++)
	{
		if (spells[spell_id].effectid[i] == SE_TryCastonSpellFinished)
		{
			if(MakeRandomInt(1, 100) <= spells[spell_id].base[i])
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

int32 Mob::CalcSpellPowerHeightMod(int32 &damage, uint16 spell_id, Mob* caster){
	//'this' = target base = Max Distance limit = Max Modifer (Min dist = 0, Min Mod = 1)
	if (!IsValidSpell(spell_id) || !caster)
		return 0;

	int mod = 0;
	for (int i = 0; i <= EFFECT_COUNT; i++)
	{
		if (spells[spell_id].effectid[i] == SE_SpellPowerHeightMod){

			if (spells[spell_id].base[i]){

				int distance = caster->GetCastingZDiff();
				int16 max_distance = spells[spell_id].base[i];
				int16 max_modifier = spells[spell_id].base2[i]*100;

				if (distance > max_distance)
					distance = max_distance;

				mod = 1 + ((distance * (max_modifier/max_distance))/100);
				//Shout("DEBUG::CalcSpellPowerHeightMod :: MOD %i Distance %i", mod, distance);
			}
		}
	}
	SetCastingZDiff(0); //Remove the zdiff variable. - Prob should move this to later in the function or zero casting vero OR when proj lands?
	if (mod)
		mod = (mod - 1) * 100;
	
	return mod;
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

		dist_targ = center->DistNoRoot(*curmob);

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

		dist_targ = center->DistNoRoot(*curmob);

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
		leap_spell_id = spell_id;
		leap_x = GetX();
		leap_y = GetY();
		leap_z = GetZ();
		leap_increment = 1;

		//Break deterimental snares and roots.
		int buff_count = GetMaxTotalSlots();
		for(int i = 0; i < buff_count; i++) {
			if (IsValidSpell(buffs[i].spellid)){

				if (spells[buffs[i].spellid].dispel_flag)
					continue;

				for(int d = 0; d < EFFECT_COUNT; d++)
				{
					if (spells[spell_id].effectid[d] == SE_Root){
						BuffFadeBySlot(i);
						continue;
					}

					if (spells[spell_id].effectid[d] == SE_MovementSpeed && !spells[buffs[i].spellid].goodEffect){
						BuffFadeBySlot(i);
						continue;
					}
				}
			}
		}
	}
}

void Mob::LeapProjectileEffect()
{
	//Due to my inability to calculate the predicted distance - Hack job for this ability.
	//Using 8 pushback and 30 pushup results in a distance ~ 56 which takes about 75 increments
	//Trigger is set on TryOnClientUpdate when increment = 75

	if (!leap_increment)
		return;

	leap_increment++;
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
	Message(MT_SpellFailure, "Your spell failed to find a target.");

}

void Mob::ProjectileTargetRingFailMessage(uint16 spell_id)
{
	if (spells[spell_id].spellgroup == 2000)
		Message(MT_SpellFailure, "Your spectral blade missed!");
	else if (spells[spell_id].spellgroup == 2006)
		Message(MT_SpellFailure, "Your spectral blades missed!");

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
		fast_buff_tick_count += 1;

		if (fast_buff_tick_count >= spellbonuses.FastManaRegen[2]){
			BuffFadeBySpellID(spellbonuses.FastManaRegen[3]);
			fast_buff_tick_count = 0;
		}
	}
	else
		fast_buff_tick_timer.Disable();
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

	const NPCType *npc_type = database.GetNPCType(pet.npc_id);
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
	made_npc->d_meele_texture1 = 0;
	made_npc->d_meele_texture2 = 0;
	for (int i = EmuConstants::MATERIAL_BEGIN; i <= EmuConstants::MATERIAL_END; i++)	{
		made_npc->armor_tint[i] = GetEquipmentColor(i);
	}
	made_npc->loottable_id = 0;

	npc_type = made_npc;

	int summon_count = 0;
	summon_count = pet.count;

	if(summon_count > MAX_SWARM_PETS)
		summon_count = MAX_SWARM_PETS;

	static const float swarm_pet_x[MAX_SWARM_PETS] = { 5, -5, 5, -5, 10, -10, 10, -10, 8, -8, 8, -8 };
	static const float swarm_pet_y[MAX_SWARM_PETS] = { 5, 5, -5, -5, 10, 10, -10, -10, 8, 8, -8, -8 };

	while(summon_count > 0) {
		NPCType *npc_dup = nullptr;
		if(made_npc != nullptr) {
			npc_dup = new NPCType;
			memcpy(npc_dup, made_npc, sizeof(NPCType));
		}

		NPC* npca = new NPC(
				(npc_dup!=nullptr)?npc_dup:npc_type,	//make sure we give the NPC the correct data pointer
				0,
				GetX()+swarm_pet_x[summon_count], GetY()+swarm_pet_y[summon_count],
				GetZ(), GetHeading(), FlyMode3);

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

	if (target){

		if (GetDiscHPRestriction(spell_id) > 0){
			if (static_cast<int>(target->GetHPRatio()) > GetDiscHPRestriction(spell_id)){
				Message(13, "Your target must be weakened under %i percent health to execute this attack!", GetDiscHPRestriction(spell_id));
				return false;
			}
		}
		
		if (CombatRange(target)){

			if (IsFacingMob(target)){
				
				if (GetDiscLimitToBehind(spell_id)){ //1 = Must attack from behind.
						
					if (BehindMob(target, GetX(), GetY()))
						return true;
					else{
						Message(13, "You must be behind your target to execute this attack!");
						return false;
					}
				}
				else
					return true;
			}
			else
				Message_StringID(13, CANT_SEE_TARGET);
		}
		else
			Message_StringID(13, TARGET_OUT_OF_RANGE);
	}
	return false;
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
	int mitigation = spellbonuses.ScaleMitigationNumhits[0];
	int slot = spellbonuses.ScaleMitigationNumhits[1];

	if (mitigation && slot >= 0){
		if (IsValidSpell(buffs[slot].spellid)){
			int numhits = buffs[slot].numhits;
			return (numhits * mitigation / 100);
		}
	}

	return 0;
}

int16 Mob::GetScaleDamageNumhits()
{
	int damage = spellbonuses.ScaleDamageNumhits[0];
	int slot = spellbonuses.ScaleDamageNumhits[1];

	if (damage && slot >= 0){
		if (IsValidSpell(buffs[slot].spellid)){
			int numhits = buffs[slot].numhits;
			return (numhits * damage / 100);
		}
	}
	return 0;
}

float Mob::GetScaleHitChanceNumhits()
{
	int hitchance = spellbonuses.ScaleHitChanceNumhits[0];
	int slot = spellbonuses.ScaleHitChanceNumhits[1];

	if (hitchance && slot >= 0){
		if (IsValidSpell(buffs[slot].spellid)){
			int numhits = buffs[slot].numhits;
			return static_cast<float>(numhits * hitchance / 100);
		}
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

		//Shout("Distance %i Increment %i",GetChargeEffect(), charge_effect_increment);
	}
}

void Client::TryChargeHit()
{
	if (!spellbonuses.ChargeEffect[0])
		return;

	charge_effect_increment++;
	//Shout("[%i] Client: X %.2f Y %.2f Z %.2f",charge_effect_increment, GetX(), GetY(), GetZ());
	 
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

		//Shout("Trigger Charge Hit: Incr [%i] Damage [%i / %i] Mod [%i]", charge_effect_increment, 1 + GetAC()/2 ,damage, mod);

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

	if (leap_increment) { 
		if (leap_increment >= 75) { //Trigger spell on next possible position update after landing.
			float dist = CalculateDistance(leap_x, leap_y,  leap_z);
			if (dist > 40.0f) {
				if (IsValidSpell(leap_spell_id)){
					for (int i=0; i < EFFECT_COUNT; i++){
						if(spells[leap_spell_id].effectid[i] == SE_CastOnLeap){
							if (IsValidSpell(spells[leap_spell_id].base[i]) && leap_spell_id != spells[leap_spell_id].base[i])
								SpellFinished(spells[leap_spell_id].base[i], this, 10, 0, -1, spells[leap_spell_id].ResistDiff);
						}
					}
				}
			}
			else
				Message(MT_SpellFailure, "Your leap failed to gather enough momentum.");

			 //Reset variable to end
			leap_spell_id = 0;
			leap_increment = 0;
			leap_x = 0.0f;
			leap_y = 0.0f;
			leap_z = 0.0f;
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
			DoBuffTic(buffs[buffs_i].spellid, buffs_i, buffs[buffs_i].ticsremaining, buffs[buffs_i].casterlevel, entity_list.GetMob(buffs[buffs_i].casterid));

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
					if (CastToClient()->GetClientVersion() == EQClientUnderfoot && buffs[buffs_i].numhits > 0)
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

		switch(effect)
		{
			default:
			{
				// do we need to do anyting here?
			}
		}
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

	if (critical_chance && defender && FlankMob(defender)){
		
		if(critical_chance < -100)
			critical_chance = -100;
		
		return critical_chance;
	}
	else
		return 0;
}

void Mob::CustomTickUpdates()
{
	if (GetBraveryRecast())
		SetBraveryRecast(GetBraveryRecast() - 6);
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
	int dm_range = max_range - min_range;
	int dm_mod_interval = max_mod - min_mod;
	int dist_from_min = range - min_range;

	if (!dm_range)
		return 0;//Saftey

	int mod = min_mod + (dist_from_min * (dm_mod_interval/dm_range));
	return mod;
}

void Mob::AbsorbMelee(int32 &damage, Mob* attacker)
{
	if (!attacker || attacker->IsClient())
		return;

	if (!spellbonuses.AbsorbMeleeDamage[0])
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

void Mob::ConeDirectionalCustom(uint16 spell_id, int16 resist_adjust)
{
			//C!Kayen - Do polygon area if no directional is set.
			if (!spells[spell_id].directional_start && !spells[spell_id].directional_end){
				RectangleDirectional(spell_id,resist_adjust);
				return;
			}

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

			entity_list.GetTargetsForConeArea(this, spells[spell_id].min_range, spells[spell_id].aoerange, spells[spell_id].aoerange / 2, targets_in_range);
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

void Mob::RangerGainNumHitsOutgoing(uint8 type, SkillUseTypes skill_used)
{
	if (!spellbonuses.RangerGainNumhitsSP[0])
		return;
	//Shout("Type: %i Amt %i Buff %i Max %i",type, spellbonuses.RangerGainNumhitsSP[0],spellbonuses.RangerGainNumhitsSP[1],spellbonuses.RangerGainNumhitsSP[2]);
	int amt = 0;
	int slot = -1;

	if (type == NUMHIT_OutgoingHitSuccess) {

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

	else if (type == NUMHIT_OutgoingHitAttempts){

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

	int anim = MakeRandomInt(0,1);

	if (anim)
		DoAnim(animPiercing);
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

//C!Misc - Functions still in development

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

/* DEPRECIATED
Mob* Mob::GetTempPetByTypeID(uint32 npc_typeid, bool SetVarTargetRing)
{
	std::list<NPC*> npc_list;
	entity_list.GetNPCList(npc_list);

 	for(std::list<NPC*>::iterator itr = npc_list.begin(); itr != npc_list.end(); ++itr) 
	{
		NPC* n = *itr;
		if (n->GetSwarmInfo()) {
			if (n->GetSwarmInfo()->owner_id == GetID() && n->npctype_id == npc_typeid) {
				if (SetVarTargetRing){
					if (!n->IsProjectilePet()){
						n->SetProjectilePet(true);
						return n;
					}
				}
				else
					return n;
			}
		}
	}
	return nullptr;
}
*/
