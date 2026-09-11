// Autocombat routines

#import "UltimaAutocombat.h"

#import "U3Platform.h"
#import "UltimaIncludes.h"
#import "UltimaMacIF.h"
#import "UltimaMain.h"
#import "UltimaMisc.h"
#import "UltimaSpellCombat.h"


extern unsigned char    Player[21][65], Experience[17];
extern unsigned char    Party[65], Macro[32];
extern unsigned char    CharX[4], CharY[4], CharTile[4], CharShape[4], careerTable[12];
extern unsigned char    MonsterX[8], MonsterY[8], MonsterTile[8], MonsterHP[8];
extern unsigned char    TileArray[128];
extern Boolean          gAutoCombat;
extern Boolean          gDone;
extern char             gKeyPress;
extern short            gMonType, zp[255];
extern char             g5521, g56E7;
extern short            spellnum;

unsigned char           futureMonX[8], futureMonY[8];

// ____________________________________________________________________
// Local prototypes

Boolean CombatCharHere(short x, short y);
char LineUpToMonster(short);
char MonsterNearby(short);
Boolean MonsterCanAttack(short x, short y);
char DirToNearestMonster(short);
Boolean NearlyDead(short who);
void SetupNow(void);
void SetupFuture(void);
Boolean FutureMonsterHere(short x, short y);
char MonsterLinedUp(short whut, short x, short y);
char AutoMoveChar(short chnum, short deltaX, short deltaY);
short ThreatValue(void);

// ____________________________________________________________________

#pragma mark -

void AutoCombat(short chnum) {
    char clss;
    short lowestHP, lowChar;
    short x, y, rosNum, chnum2, wpn, dir, magic;
    long hp;
    Boolean isWiz, isCler, isMulti, castMittar;

    U3PlatformGetKeyMouse(0);
    rosNum = Party[7 + chnum];
    magic = Player[rosNum][25];
    clss = Player[rosNum][23];
    isMulti = (clss == careerTable[8] || clss == careerTable[10]);
    isWiz = (clss == careerTable[2] || clss == careerTable[6] || clss == careerTable[9] || isMulti);
    isCler = (clss == careerTable[1] || clss == careerTable[4] || clss == careerTable[7] || isMulti);
    x = CharX[chnum];
    y = CharY[chnum];
    // Nearly dead, run away!
    if (NearlyDead(chnum + 1) && MonsterCanAttack(x, y)) {
        Boolean allowDiagonal = !(U3PlatformGetBooleanPreference(U3PreferenceNoDiagonals));
        if (!MonsterCanAttack(x, y + 1) && !CombatCharHere(x, y + 1)) {
            AddMacro('2');
            return;
        }
        if (allowDiagonal) {
            if (!MonsterCanAttack(x - 1, y + 1) && !CombatCharHere(x - 1, y + 1)) {
                AddMacro('1');
                return;
            }
            if (!MonsterCanAttack(x + 1, y + 1) && !CombatCharHere(x + 1, y + 1)) {
                AddMacro('3');
                return;
            }
        }
        if (!MonsterCanAttack(x - 1, y) && !CombatCharHere(x - 1, y)) {
            AddMacro('4');
            return;
        }
        if (!MonsterCanAttack(x + 1, y) && !CombatCharHere(x + 1, y)) {
            AddMacro('6');
            return;
        }
        if (!MonsterCanAttack(x, y - 1) && !CombatCharHere(x, y - 1)) {
            AddMacro('8');
            return;
        }
        if (allowDiagonal) {
            if (!MonsterCanAttack(x - 1, y - 1) && !CombatCharHere(x - 1, y - 1)) {
                AddMacro('7');
                return;
            }
            if (!MonsterCanAttack(x + 1, y - 1) && !CombatCharHere(x + 1, y - 1)) {
                AddMacro('9');
                return;
            }
        }
        //gAutoCombat = FALSE; // can't run away, turn off auto combat!
        //return;
    }
    // Can cast Repond, haven't casted it yet, and these are orcs?
    if ((gMonType == 0x30) && (g5521 == 0) && isWiz) {
        AddMacro('A');
        if (isMulti)
            AddMacro('W');
        AddMacro('C');
        return;
    }
    // Can cast Pontori, haven't casted it yet, and these are skellyz?
    if ((gMonType == 0x32) && (g56E7 == 0) && isCler) {
        AddMacro('A');
        if (isMulti)
            AddMacro('C');
        AddMacro('C');
        return;
    }
    /* Cure a poisoned party member before choosing an offensive action. */
    if (magic >= 35 && isCler) {
        short poisonedChar = -1;
        for (chnum2 = 0; chnum2 < Party[2]; chnum2++) {
            short target = Party[7 + chnum2];
            if (target != 0 && Player[target][17] == 'P') {
                poisonedChar = chnum2;
                break;
            }
        }
        if (poisonedChar >= 0) {
            AddMacro('1' + poisonedChar);
            if (isMulti)
                AddMacro('C');
            AddMacro('H');
            AddMacro('C');
            return;
        }
    }
    // Can cast nameless spell and there is a big threat
    if (magic >= 75 && isWiz && (ThreatValue() > 60)) {
        AddMacro('P');
        if (isMulti)
            AddMacro('W');
        AddMacro('C');
        return;
    }
    // Can cast ZXKUQYB and there is a bigger threat
    if (magic >= 70 && isCler && (ThreatValue() > 80)) {
        AddMacro('O');
        if (isMulti)
            AddMacro('C');
        AddMacro('C');
        return;
    }
    // Can cast Sanctu, and a character needs it?
    if (magic >= 10 && isCler) {
        lowestHP = 9999;
        lowChar = -1;
        for (chnum2 = 0; chnum2 < 4; chnum2++) {
            hp = Player[Party[7 + chnum2]][26] * 256 + Player[Party[7 + chnum2]][27];
            if (CheckAlive(chnum2) && hp < lowestHP) {
                lowestHP = hp;
                lowChar = chnum2;
            }
        }
        if (lowestHP < 75) {
            AddMacro('1' + lowChar);
            AddMacro('C');
            if (isMulti)
                AddMacro('C');
            AddMacro('C');
            return;
        }
    }
    // Can cast Mittar and not already weilding a magic bow?
    castMittar = FALSE;
    wpn = Player[rosNum][48];
    if ((magic >= 5) && isWiz && (wpn != 9) && (wpn != 13))
        castMittar = TRUE;
    if (wpn == 3 || wpn == 5 || wpn == 9 || wpn == 13 || castMittar) {    // projectile weapon
        SetupNow();
        dir = MonsterLinedUp(5, CharX[chnum], CharY[chnum]);
        if (dir != 0 && dir != ' ') {
            if (castMittar) {
                AddMacro(dir);
                AddMacro('B');
                if (isMulti)
                    AddMacro('W');
                AddMacro('C');
            } else {
                AddMacro(dir);
                AddMacro('A');
            }
            return;
        } else {
            SetupFuture();
            dir = LineUpToMonster(chnum);
            AddMacro(dir);
        }
    } else {   // hand to hand weapon only
        if (NearlyDead(chnum + 1) && CharY[chnum] < 10) {
            AddMacro(' ');
            return;
        }    // don't advance!
        dir = MonsterNearby(chnum);
        if (dir != 0) {
            //if (NearlyDead(chnum+1)) { AddMacro(' '); return; } // don't advance!
            AddMacro(dir);
            AddMacro('A');
            return;
        } else {
            SetupFuture();
            dir = DirToNearestMonster(chnum);
            AddMacro(dir);
            return;
        }
    }
}

Boolean U3AutoCombatSelfTest(void) {
    unsigned char savedMacro[32], savedTileArray[128], savedCharX[4], savedCharY[4];
    unsigned char savedCharTile[4], savedCharShape[4];
    unsigned char savedMonsterX[8], savedMonsterY[8], savedMonsterHP[8];
    unsigned char savedMonsterTile[8];
    unsigned char savedParty2 = Party[2], savedParty3 = Party[3], savedParty4 = Party[4];
    unsigned char savedParty7 = Party[7], savedParty16 = Party[16];
    unsigned char savedClass = Player[1][23], savedWeapon = Player[1][48];
    unsigned char savedMagic = Player[1][25], savedStatus = Player[1][17];
    unsigned char savedHPHigh = Player[1][26], savedHPLow = Player[1][27];
    unsigned char savedExperience = Experience[8];
    short savedMonType = gMonType, savedSpell = spellnum;
    char saved5521 = g5521, saved56E7 = g56E7;
    Boolean savedDone = gDone;
    memcpy(savedMacro, Macro, sizeof(savedMacro));
    memcpy(savedTileArray, TileArray, sizeof(savedTileArray));
    memcpy(savedCharX, CharX, sizeof(savedCharX));
    memcpy(savedCharY, CharY, sizeof(savedCharY));
    memcpy(savedCharTile, CharTile, sizeof(savedCharTile));
    memcpy(savedCharShape, CharShape, sizeof(savedCharShape));
    memcpy(savedMonsterX, MonsterX, sizeof(savedMonsterX));
    memcpy(savedMonsterY, MonsterY, sizeof(savedMonsterY));
    memcpy(savedMonsterTile, MonsterTile, sizeof(savedMonsterTile));
    memcpy(savedMonsterHP, MonsterHP, sizeof(savedMonsterHP));

    Party[2] = 1;
    Party[7] = 1;
    Player[1][23] = careerTable[0];
    Player[1][48] = 0;
    CharX[0] = 5;
    CharY[0] = 5;
    MonsterX[0] = 5;
    MonsterY[0] = 4;
    MonsterHP[0] = 1;
    gMonType = 0x30;
    g5521 = g56E7 = 0;
    memset(Macro, 0, sizeof(Macro));
    AutoCombat(0);
    Boolean meleePassed = Macro[0] == 'A' && Macro[1] == '8';

    /* Two high-value targets should make a wizard choose the area spell branch. */
    Player[1][23] = careerTable[2];
    Player[1][25] = 75;
    Player[1][17] = 'G';
    Player[1][26] = 0;
    Player[1][27] = 100;
    Experience[8] = 40;
    MonsterHP[0] = MonsterHP[1] = 1;
    g5521 = 1; /* Repond has already been used; reach threat-based selection. */
    memset(Macro, 0, sizeof(Macro));
    AutoCombat(0);
    Boolean threatPassed = Macro[0] == 'C' && Macro[1] == 'P';

    /* A generated C,B,8 macro must reach Cast and damage the aligned target. */
    Party[3] = 0x80;
    Party[4] = 0;
    Party[16] = 0;
    Player[1][23] = careerTable[2];
    Player[1][25] = 5;
    Player[1][17] = 'G';
    Player[1][26] = 0;
    Player[1][27] = 100;
    Player[1][48] = 0;
    CharTile[0] = 2;
    CharShape[0] = 0x80;
    MonsterX[0] = 5;
    MonsterY[0] = 2;
    MonsterTile[0] = 2;
    MonsterHP[0] = 100;
    gMonType = 0x32;
    g5521 = g56E7 = 0;
    gDone = FALSE;
    memset(TileArray, 2, sizeof(TileArray));
    memset(Macro, 0, sizeof(Macro));
    AutoCombat(0);
    Boolean spellMacroPassed = Macro[0] == 'C' && Macro[1] == 'B' && Macro[2] == '8';
    Boolean commandConsumed = U3PlatformGetKeyMouse(0) && gKeyPress == 'C';
    Boolean spellExecuted = commandConsumed && Cast(1, 1) &&
        Player[1][25] == 0 && MonsterHP[0] < 100;
    Boolean passed = meleePassed && threatPassed && spellMacroPassed && spellExecuted;

    memcpy(Macro, savedMacro, sizeof(savedMacro));
    memcpy(TileArray, savedTileArray, sizeof(savedTileArray));
    memcpy(CharX, savedCharX, sizeof(savedCharX));
    memcpy(CharY, savedCharY, sizeof(savedCharY));
    memcpy(CharTile, savedCharTile, sizeof(savedCharTile));
    memcpy(CharShape, savedCharShape, sizeof(savedCharShape));
    memcpy(MonsterX, savedMonsterX, sizeof(savedMonsterX));
    memcpy(MonsterY, savedMonsterY, sizeof(savedMonsterY));
    memcpy(MonsterTile, savedMonsterTile, sizeof(savedMonsterTile));
    memcpy(MonsterHP, savedMonsterHP, sizeof(savedMonsterHP));
    Party[2] = savedParty2;
    Party[3] = savedParty3;
    Party[4] = savedParty4;
    Party[7] = savedParty7;
    Party[16] = savedParty16;
    Player[1][23] = savedClass;
    Player[1][48] = savedWeapon;
    Player[1][25] = savedMagic;
    Player[1][17] = savedStatus;
    Player[1][26] = savedHPHigh;
    Player[1][27] = savedHPLow;
    Experience[8] = savedExperience;
    gMonType = savedMonType;
    spellnum = savedSpell;
    g5521 = saved5521;
    g56E7 = saved56E7;
    gDone = savedDone;
    return passed;
}

Boolean U3AutoCombatSupportSpellSelfTest(void) {
    unsigned char savedMacro[32], savedTileArray[128], savedCharX[4], savedCharY[4];
    unsigned char savedCharTile[4], savedCharShape[4], savedMonsterX[8], savedMonsterY[8];
    unsigned char savedMonsterTile[8], savedMonsterHP[8], savedPartySlots[4];
    unsigned char savedPlayer1[65], savedPlayer2[65];
    unsigned char savedParty2 = Party[2], savedParty16 = Party[16];
    short savedMonType = gMonType, savedSpell = spellnum;
    char saved5521 = g5521, saved56E7 = g56E7, savedKeyPress = gKeyPress;
    Boolean savedDone = gDone;
    memcpy(savedMacro, Macro, sizeof(savedMacro));
    memcpy(savedTileArray, TileArray, sizeof(savedTileArray));
    memcpy(savedCharX, CharX, sizeof(savedCharX));
    memcpy(savedCharY, CharY, sizeof(savedCharY));
    memcpy(savedCharTile, CharTile, sizeof(savedCharTile));
    memcpy(savedCharShape, CharShape, sizeof(savedCharShape));
    memcpy(savedMonsterX, MonsterX, sizeof(savedMonsterX));
    memcpy(savedMonsterY, MonsterY, sizeof(savedMonsterY));
    memcpy(savedMonsterTile, MonsterTile, sizeof(savedMonsterTile));
    memcpy(savedMonsterHP, MonsterHP, sizeof(savedMonsterHP));
    memcpy(savedPartySlots, Party + 7, sizeof(savedPartySlots));
    memcpy(savedPlayer1, Player[1], sizeof(savedPlayer1));
    memcpy(savedPlayer2, Player[2], sizeof(savedPlayer2));

    Party[2] = 2;
    Party[7] = 1;
    Party[8] = 2;
    Party[9] = Party[10] = 0;
    Party[16] = 0;
    Player[1][17] = 'G';
    Player[1][23] = careerTable[1];
    Player[1][25] = 10;
    Player[1][26] = 0;
    Player[1][27] = 100;
    Player[2][17] = 'G';
    Player[2][26] = 0;
    Player[2][27] = 50;
    CharX[0] = 5;
    CharY[0] = 5;
    CharX[1] = 2;
    CharY[1] = 2;
    CharTile[0] = CharTile[1] = 2;
    CharShape[0] = CharShape[1] = 0x80;
    memset(MonsterHP, 0, sizeof(MonsterHP));
    gMonType = 0x34;
    g5521 = g56E7 = 0;
    gDone = FALSE;
    memset(TileArray, 2, sizeof(TileArray));
    memset(Macro, 0, sizeof(Macro));
    AutoCombat(0);
    Boolean spellMacroPassed = Macro[0] == 'C' && Macro[1] == 'C' && Macro[2] == '2';
    Boolean commandConsumed = U3PlatformGetKeyMouse(0) && gKeyPress == 'C';
    Boolean spellExecuted = commandConsumed && Cast(1, 1) &&
        Player[1][25] == 0 && Player[2][27] > 50;

    Player[1][25] = 35;
    Player[2][17] = 'P';
    memset(Macro, 0, sizeof(Macro));
    AutoCombat(0);
    Boolean cureMacroPassed = Macro[0] == 'C' && Macro[1] == 'H' && Macro[2] == '2';
    commandConsumed = U3PlatformGetKeyMouse(0) && gKeyPress == 'C';
    Boolean cureExecuted = commandConsumed && Cast(1, 1) &&
        Player[1][25] == 0 && Player[2][17] == 'G';
    Boolean passed = spellMacroPassed && spellExecuted && cureMacroPassed && cureExecuted;

    memcpy(Macro, savedMacro, sizeof(savedMacro));
    memcpy(TileArray, savedTileArray, sizeof(savedTileArray));
    memcpy(CharX, savedCharX, sizeof(savedCharX));
    memcpy(CharY, savedCharY, sizeof(savedCharY));
    memcpy(CharTile, savedCharTile, sizeof(savedCharTile));
    memcpy(CharShape, savedCharShape, sizeof(savedCharShape));
    memcpy(MonsterX, savedMonsterX, sizeof(savedMonsterX));
    memcpy(MonsterY, savedMonsterY, sizeof(savedMonsterY));
    memcpy(MonsterTile, savedMonsterTile, sizeof(savedMonsterTile));
    memcpy(MonsterHP, savedMonsterHP, sizeof(savedMonsterHP));
    memcpy(Party + 7, savedPartySlots, sizeof(savedPartySlots));
    Party[2] = savedParty2;
    Party[16] = savedParty16;
    memcpy(Player[1], savedPlayer1, sizeof(savedPlayer1));
    memcpy(Player[2], savedPlayer2, sizeof(savedPlayer2));
    gMonType = savedMonType;
    spellnum = savedSpell;
    g5521 = saved5521;
    g56E7 = saved56E7;
    gKeyPress = savedKeyPress;
    gDone = savedDone;
    return passed;
}

short ThreatValue(void) {   // total experience value of monsters
    char mon;
    short total, expval;

    expval = Experience[(gMonType / 2) & 0x0F];
    total = 0;
    for (mon = 0; mon < 8; mon++) {
        if (MonsterHP[mon] != 0)
            total += expval;
    }
    // Things that are poisonous are twice the threat
    if (gMonType == 0x1C || gMonType == 0x3C || gMonType == 0x38)
        total *= 2;
    return total;
}

char MonsterNearby(short chnum) {
    short x, y;

    x = CharX[chnum];
    y = CharY[chnum];
    if (CombatMonsterHere(x, y - 1) != 255)
        return '8';    // North
    if (CombatMonsterHere(x - 1, y) != 255)
        return '4';    // West
    if (CombatMonsterHere(x + 1, y) != 255)
        return '6';    // East
    if (CombatMonsterHere(x, y + 1) != 255)
        return '2';    // South
    if (U3PlatformGetBooleanPreference(U3PreferenceNoDiagonals))
        return 0;
    if (CombatMonsterHere(x - 1, y - 1) != 255)
        return '7';    // Northwest
    if (CombatMonsterHere(x - 1, y + 1) != 255)
        return '1';    // Southwest
    if (CombatMonsterHere(x + 1, y - 1) != 255)
        return '9';    // Northeast
    if (CombatMonsterHere(x + 1, y + 1) != 255)
        return '3';    // Northwest
    return 0;
}

Boolean MonsterCanAttack(short x, short y) {
    Boolean result = FALSE;
    short mon;

    /*  // first check for ones you can't run from
    if (gMonType==0x1C) return TRUE; // Man-O-War
    if (gMonType==0x1A) return TRUE; // Serpent
    if (gMonType==0x2C) return TRUE; // Wizard
    if (gMonType==0x36) return TRUE; // Daemon
    if (gMonType==0x3C) return TRUE; // Balron */    // don't bother, it's ok to run from these.
    // now check around this spot
    result |= (CombatMonsterHere(x - 1, y - 1) != 255);
    result |= (CombatMonsterHere(x, y - 1) != 255);
    result |= (CombatMonsterHere(x + 1, y - 1) != 255);
    result |= (CombatMonsterHere(x - 1, y) != 255);
    result |= (CombatMonsterHere(x + 1, y) != 255);
    result |= (CombatMonsterHere(x - 1, y + 1) != 255);
    result |= (CombatMonsterHere(x, y + 1) != 255);
    result |= (CombatMonsterHere(x + 1, y + 1) != 255);
    // now check for things that can shoot (dragons)
    if (gMonType == 0x3A) {
        for (mon = 0; mon < 8; mon++) {
            if (MonsterHP[mon]) {
                if (Absolute(MonsterX[mon] - x) == Absolute(MonsterY[mon] - y))
                    result = TRUE;
            }
        }
    }
    return result;
}

Boolean NearlyDead(short who) {   // 0=anybody, otherwise 1-4
    short chnum;
    long hp;
    Boolean nearlyDead;

    nearlyDead = FALSE;
    if (who > 0) {   // a specific character
        chnum = who - 1;
        hp = Player[Party[7 + chnum]][26] * 256 + Player[Party[7 + chnum]][27];
        if (hp < 50)
            nearlyDead = TRUE;
    } else {   // anybody
        for (chnum = 0; chnum < 4; chnum++) {
            if (CheckAlive(chnum) == FALSE) {
                nearlyDead = TRUE;
            } else {
                hp = Player[Party[7 + chnum]][26] * 256 + Player[Party[7 + chnum]][27];
                if (hp < 50)
                    nearlyDead = TRUE;
            }
        }
    }
    return nearlyDead;
}

void SetupNow(void) {
    short mon;

    for (mon = 0; mon < 8; mon++) {
        futureMonX[mon] = MonsterX[mon];
        futureMonY[mon] = MonsterY[mon];
    }
}

void SetupFuture(void) {
    short mon, chnum, distance, closestChar, closestVal, newx, newy, deltaX, deltaY;

    SetupNow();
    for (mon = 0; mon < 8; mon++) {
        if (MonsterHP[mon] != 0) {
            closestChar = 8;     // non-player
            closestVal = 128;    // way big
            for (chnum = 0; chnum < 4; chnum++) {
                distance = Absolute(futureMonX[mon] - CharX[chnum]) + Absolute(futureMonY[mon] - CharY[chnum]);
                if (distance < closestVal) {
                    closestVal = distance;
                    closestChar = chnum;
                }
            }
            deltaX = GetHeading(CharX[closestChar] - futureMonX[mon]);
            deltaY = GetHeading(CharY[closestChar] - futureMonY[mon]);
            newx = futureMonX[mon] + deltaX;
            newy = futureMonY[mon] + deltaY;
            if (FutureMonsterHere(newx, newy)) {
                newx = futureMonX[mon];
                newy = futureMonY[mon] + deltaY;
                if (FutureMonsterHere(newx, newy)) {
                    newx = futureMonX[mon] + deltaX;
                    newy = futureMonY[mon];
                    if (FutureMonsterHere(newx, newy)) {
                        newx = futureMonX[mon];
                        newy = futureMonY[mon];
                    }
                }
            }
            futureMonX[mon] = newx;
            futureMonY[mon] = newy;
        }
    }
}

Boolean FutureMonsterHere(short x, short y) {   // monster or character
    short mon;
    Boolean result;
    result = FALSE;
    for (mon = 7; mon >= 0; mon--) {
        if (MonsterHP[mon] != 0) {
            if (futureMonX[mon] == x && futureMonY[mon] == y)
                result = TRUE;
        }
    }
    for (mon = 0; mon < 4; mon++) {
        if (CheckAlive(mon)) {
            if (CharX[mon] == x && CharY[mon] == y)
                result = TRUE;
        }
    }
    return result;
}

char DirToNearestMonster(short chnum) {  // returns key to 'press' to head towards
                                         // nearest baddie
    short mon, distance, closestMonster, closestVal;
    short deltaX, deltaY;

    closestMonster = 8;    // non-monster
    closestVal = 128;      // way big
    for (mon = 0; mon < 8; mon++) {
        if (MonsterHP[mon] != 0) {
            distance = Absolute(futureMonX[mon] - CharX[chnum]) + Absolute(futureMonY[mon] - CharY[chnum]);
            if (distance < closestVal) {
                closestVal = distance;
                closestMonster = mon;
            }
        }
    }
    deltaX = GetHeading(futureMonX[closestMonster] - CharX[chnum]);
    deltaY = GetHeading(futureMonY[closestMonster] - CharY[chnum]);
    return AutoMoveChar(chnum, deltaX, deltaY);
}

Boolean CombatCharHere(short x, short y) {
    short value, chnum;
    Boolean isOneHere;

    isOneHere = FALSE;
    value = GetXYTile(x, y);
    if (value != 2 && value != 4 && value != 6 && value != 0x10)
        isOneHere = TRUE;
    for (chnum = 0; chnum < 4; chnum++) {
        if (CharX[chnum] == x && CharY[chnum] == y)
            isOneHere = TRUE;
    }
    return isOneHere;
}

char LineUpToMonster(short chnum) {  // returns key to 'press' to end up lined up
                                     // to a monster next turn.
    char dir;

    if (CharX[chnum] < 6) {   // left half of screen
        dir = MonsterLinedUp(chnum, CharX[chnum] + 1, CharY[chnum]);
        if (dir)
            return AutoMoveChar(chnum, 1, 0);
        dir = MonsterLinedUp(chnum, CharX[chnum] - 1, CharY[chnum]);
        if (dir)
            return AutoMoveChar(chnum, -1, 0);
        dir = MonsterLinedUp(chnum, CharX[chnum], CharY[chnum] - 1);
        if (dir)
            return AutoMoveChar(chnum, 0, -1);
        dir = MonsterLinedUp(chnum, CharX[chnum], CharY[chnum] + 1);
        if (dir)
            return AutoMoveChar(chnum, 0, 1);
        if (U3PlatformGetBooleanPreference(U3PreferenceNoDiagonals))
            return DirToNearestMonster(chnum);
        dir = MonsterLinedUp(chnum, CharX[chnum] + 1, CharY[chnum] - 1);
        if (dir)
            return dir;
        dir = MonsterLinedUp(chnum, CharX[chnum] + 1, CharY[chnum] + 1);
        if (dir)
            return dir;
        dir = MonsterLinedUp(chnum, CharX[chnum] - 1, CharY[chnum] - 1);
        if (dir)
            return dir;
        dir = MonsterLinedUp(chnum, CharX[chnum] - 1, CharY[chnum] + 1);
        if (dir)
            return dir;
        return DirToNearestMonster(chnum);
    } else {   // right half of screen
        dir = MonsterLinedUp(chnum, CharX[chnum] - 1, CharY[chnum]);
        if (dir)
            return AutoMoveChar(chnum, -1, 0);
        dir = MonsterLinedUp(chnum, CharX[chnum] + 1, CharY[chnum]);
        if (dir)
            return AutoMoveChar(chnum, 1, 0);
        dir = MonsterLinedUp(chnum, CharX[chnum], CharY[chnum] - 1);
        if (dir)
            return AutoMoveChar(chnum, 0, -1);
        dir = MonsterLinedUp(chnum, CharX[chnum], CharY[chnum] + 1);
        if (dir)
            return AutoMoveChar(chnum, 0, 1);
        if (U3PlatformGetBooleanPreference(U3PreferenceNoDiagonals))
            return DirToNearestMonster(chnum);
        dir = MonsterLinedUp(chnum, CharX[chnum] - 1, CharY[chnum] - 1);
        if (dir)
            return dir;
        dir = MonsterLinedUp(chnum, CharX[chnum] - 1, CharY[chnum] + 1);
        if (dir)
            return dir;
        dir = MonsterLinedUp(chnum, CharX[chnum] + 1, CharY[chnum] - 1);
        if (dir)
            return dir;
        dir = MonsterLinedUp(chnum, CharX[chnum] + 1, CharY[chnum] + 1);
        if (dir)
            return dir;
        return DirToNearestMonster(chnum);
    }
}

char MonsterLinedUp(short chnum, short x, short y) {  // returns key to 'press' to shoot nearest
                                                      // baddie.  0 = Nothing lined up
    short mon, distance, closestMonster, closestVal;
    short deltaX, deltaY;
    char keyToPress;
    Boolean thisOne;

    closestMonster = 8;    // non-monster
    closestVal = 128;      // way big
    for (mon = 0; mon < 8; mon++) {
        thisOne = FALSE;
        if (MonsterHP[mon] != 0) {
            if (futureMonX[mon] == x || futureMonY[mon] == y)
                thisOne = TRUE;
            if (!U3PlatformGetBooleanPreference(U3PreferenceNoDiagonals))
                thisOne |= (Absolute(x - futureMonX[mon]) == Absolute(y - futureMonY[mon]));
        }
        if (thisOne) {
            distance = Absolute(futureMonX[mon] - x) + Absolute(futureMonY[mon] - y);
            if (distance < closestVal) {
                closestVal = distance;
                closestMonster = mon;
            }
        }
    }
    if (closestMonster == 8)
        return 0;
    deltaX = GetHeading(futureMonX[closestMonster] - x);
    deltaY = GetHeading(futureMonY[closestMonster] - y);
    keyToPress = 0;
    if (deltaX == -1 && deltaY == 1)
        keyToPress = '1';    // Southwest
    if (deltaX == 0 && deltaY == 1)
        keyToPress = '2';    // South
    if (deltaX == 1 && deltaY == 1)
        keyToPress = '3';    // Southeast
    if (deltaX == -1 && deltaY == 0)
        keyToPress = '4';    // West
    if (deltaX == 1 && deltaY == 0)
        keyToPress = '6';    // East
    if (deltaX == -1 && deltaY == -1)
        keyToPress = '7';    // Northwest
    if (deltaX == 0 && deltaY == -1)
        keyToPress = '8';    // North
    if (deltaX == 1 && deltaY == -1)
        keyToPress = '9';    // Northeast
    if (chnum < 4)
        return AutoMoveChar(chnum, deltaX, deltaY);
    return keyToPress;
}

char AutoMoveChar(short chnum, short deltaX, short deltaY) {
    char keyToPress;
    short saveDeltaX;

    // if not allowed to move diagonally, and the nearest monster is diagonally
    // away, choose vertical over horizontal.
    Boolean allowDiagonal = (!U3PlatformGetBooleanPreference(U3PreferenceNoDiagonals));
    if (allowDiagonal == 0 && deltaX != 0 && deltaY != 0)
        deltaX = 0;
    if (!CombatCharHere(CharX[chnum] + deltaX, CharY[chnum] + deltaY))
        goto doKeyNow;
    if (deltaX == 0) {
        if (allowDiagonal) {
            deltaX = 1;
            if (!CombatCharHere(CharX[chnum] + deltaX, CharY[chnum] + deltaY))
                goto doKeyNow;
            deltaX = -1;
            if (!CombatCharHere(CharX[chnum] + deltaX, CharY[chnum] + deltaY))
                goto doKeyNow;
        }
        deltaY = 0;
        deltaX = 1;
        if (!CombatCharHere(CharX[chnum] + deltaX, CharY[chnum] + deltaY))
            goto doKeyNow;
        deltaX = -1;
        if (!CombatCharHere(CharX[chnum] + deltaX, CharY[chnum] + deltaY))
            goto doKeyNow;
        deltaX = 2;
        goto doKeyNow;    // PASS!@1
    }
    if (deltaY == 0) {
        if (allowDiagonal) {
            deltaY = -1;
            if (!CombatCharHere(CharX[chnum] + deltaX, CharY[chnum] + deltaY))
                goto doKeyNow;
            deltaY = 1;
            if (!CombatCharHere(CharX[chnum] + deltaX, CharY[chnum] + deltaY))
                goto doKeyNow;
        }
        deltaX = 0;
        deltaY = -1;
        if (!CombatCharHere(CharX[chnum] + deltaX, CharY[chnum] + deltaY))
            goto doKeyNow;
        deltaY = 1;
        if (!CombatCharHere(CharX[chnum] + deltaX, CharY[chnum] + deltaY))
            goto doKeyNow;
        deltaY = 2;
        goto doKeyNow;    // I give up, I'm stuck, pass
    }
    // must be an attempted diagonal move
    saveDeltaX = deltaX;
    deltaX = 0;
    if (!CombatCharHere(CharX[chnum] + deltaX, CharY[chnum] + deltaY))
        goto doKeyNow;
    deltaX = saveDeltaX;
    deltaY = 0;
    if (!CombatCharHere(CharX[chnum] + deltaX, CharY[chnum] + deltaY))
        goto doKeyNow;
    deltaX = 2;    // pass!@
doKeyNow:
    keyToPress = ' ';    // Pass
    if (deltaX == -1 && deltaY == 1)
        keyToPress = '1';    // Southwest
    if (deltaX == 0 && deltaY == 1)
        keyToPress = '2';    // South
    if (deltaX == 1 && deltaY == 1)
        keyToPress = '3';    // Southeast
    if (deltaX == -1 && deltaY == 0)
        keyToPress = '4';    // West
    if (deltaX == 1 && deltaY == 0)
        keyToPress = '6';    // East
    if (deltaX == -1 && deltaY == -1)
        keyToPress = '7';    // Northwest
    if (deltaX == 0 && deltaY == -1)
        keyToPress = '8';    // North
    if (deltaX == 1 && deltaY == -1)
        keyToPress = '9';    // Northeast
    return keyToPress;
}
