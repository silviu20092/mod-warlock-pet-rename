/*
 * Credits: silviu20092
 */

#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "Player.h"
#include "Pet.h"
#include "DatabaseEnv.h"
#include "GameTime.h"

class npc_warlock_pet_renamer : public CreatureScript
{
private:
    static constexpr int VISUAL_FEEDBACK_SPELL_ID = 46331;

    static Pet* GetAllowedPetForRename(Player* player)
    {
        Pet* pet = player->GetPet();
        if (!pet)
            return nullptr;

        return pet->IsPet() && pet->GetOwnerGUID() == player->GetGUID() && pet->GetCharmInfo() != nullptr ? pet : nullptr;
    }

    static void NormalizeName(std::string& name)
    {
        std::transform(name.begin(), name.end(), name.begin(), tolower);
        name[0] = std::toupper(name[0]);
    }

    static void HandlePetRename(Player* player, const char* nameStr)
    {
        Pet* pet = GetAllowedPetForRename(player);
        if (!pet)
            return;

        std::string name(nameStr);
        NormalizeName(name);

        PetNameInvalidReason res = ObjectMgr::CheckPetName(name);
        if (res != PET_NAME_SUCCESS)
        {
            player->GetSession()->SendPetNameInvalid(res, name, nullptr);
            return;
        }

        if (sObjectMgr->IsReservedName(name))
        {
            player->GetSession()->SendPetNameInvalid(PET_NAME_RESERVED, name, nullptr);
            return;
        }

        if (sObjectMgr->IsProfanityName(name))
        {
            player->GetSession()->SendPetNameInvalid(PET_NAME_PROFANE, name, nullptr);
            return;
        }

        pet->SetName(name);
        player->CastSpell(pet, VISUAL_FEEDBACK_SPELL_ID, true);

        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_PET_NAME);
        stmt->SetData(0, name);
        stmt->SetData(1, player->GetGUID().GetCounter());
        stmt->SetData(2, pet->GetCharmInfo()->GetPetNumber());
        CharacterDatabase.Execute(stmt);

        pet->SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, uint32(GameTime::GetGameTime().count())); // cast can't be helped
    }

    static std::string GetPetInfo(const Pet* pet)
    {
        std::string type = "unknown";
        switch (pet->GetEntry())
        {
            case NPC_INFERNAL:
                type = "infernal";
                break;
            case NPC_IMP:
                type = "imp";
                break;
            case NPC_FELHUNTER:
                type = "felhunter";
                break;
            case NPC_VOIDWALKER:
                type = "voidwalker";
                break;
            case NPC_SUCCUBUS:
                type = "succubus";
                break;
            case NPC_DOOMGUARD:
                type = "doomguard";
                break;
            case NPC_FELGUARD:
                type = "felguard";
                break;
        }

        return pet->GetName() + " (" + type + ")";
    }
public:
    npc_warlock_pet_renamer() : CreatureScript("npc_warlock_pet_renamer")
    {
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (player->getClass() != CLASS_WARLOCK)
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "|cffb50505WARLOCKS ONLY|r", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
        else
        {
            Pet* pet = GetAllowedPetForRename(player);
            if (!pet)
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "|cffb50505PLEASE SUMMON YOUR PET|r", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
            else
            {
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Current pet: " + GetPetInfo(pet), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
                AddGossipItemFor(player, GOSSIP_ICON_TALK, "Rename current pet", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3, "Type in your desired pet name in the next popup!", 0, true);
            }
        }

        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Nevermind...", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        if (action == GOSSIP_ACTION_INFO_DEF)
        {
            ClearGossipMenuFor(player);
            return OnGossipHello(player, creature);
        }
        else if (action == GOSSIP_ACTION_INFO_DEF + 1)
        {
            CloseGossipMenuFor(player);
            return true;
        }

        CloseGossipMenuFor(player);
        return false;
    }

    bool OnGossipSelectCode(Player* player, Creature* /*creature*/, uint32 /*sender*/ , uint32 action, const char* code) override
    {
        if (action == GOSSIP_ACTION_INFO_DEF + 3)
            HandlePetRename(player, code);

        CloseGossipMenuFor(player);
        return true;
    }
};

void AddSC_npc_warlock_pet_renamer()
{
    new npc_warlock_pet_renamer();
}
