// Autocombat routines

#import "UltimaAutocombat.h"

#import "UltimaIncludes.h"
#import "UltimaMacIF.h"
#import "UltimaMain.h"
#import "UltimaMisc.h"
#import "UltimaSpellCombat.h"

extern unsigned char    Player[21][65], Party[64], Experience[17];
extern unsigned char    CharX[4], CharY[4], CharTile[4], CharShape[4], careerTable[12];
extern unsigned char    MonsterX[8], MonsterY[8], MonsterTile[8], MonsterHP[8];
extern Boolean          gAutoCombat;
extern short            gMonType, zp[255];
extern char             g5521, g56E7;

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

    GetKeyMouse(0);
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
        Boolean allowDiagonal = !(CFPreferencesGetAppBooleanValue(U3PrefNoDiagonals, kCFPreferencesCurrentApplication, NULL));
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
    if (CFPreferencesGetAppBooleanValue(U3PrefNoDiagonals, kCFPreferencesCurrentApplication, NULL))
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
        if (CFPreferencesGetAppBooleanValue(U3PrefNoDiagonals, kCFPreferencesCurrentApplication, NULL))
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
        if (CFPreferencesGetAppBooleanValue(U3PrefNoDiagonals, kCFPreferencesCurrentApplication, NULL))
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
            if (!CFPreferencesGetAppBooleanValue(U3PrefNoDiagonals, kCFPreferencesCurrentApplication, NULL))
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
    Boolean allowDiagonal = (!CFPreferencesGetAppBooleanValue(U3PrefNoDiagonals, kCFPreferencesCurrentApplication, NULL));
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
