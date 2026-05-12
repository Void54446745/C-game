#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

//declaring mischallanous variables
int id = 0; // used to mark which weapon3 they have
int hit = 0;
int choice = 0; // used to record choices in character creation
int cloak_charges = 0;
bool sheilded = false;
int damage = 0;
bool alive = true;

// declaring weapon modifiers e.g. duel weapons
int attacks1 = 1;
int attacks2 = 1;
int attacks3 = 1;
int attacks4 = 0;
int daggers = 0;

// declaring player stats and setting them to 0
int phealth = 1;
int pstr = 3;
int pac = 10; // armour class
int pdamage = 0;// this will hold the damage when they attack
int pdex = 3;
int i = 0;

// declaring enemy stats
int edamage = 0;
int enemy = 0;
int ehealth = 0;
int eac = 0;
char etype[] = "";
int eattacks = 1;

// declaring weapons used in advance
char weapon1[] = "";
int weapon1dam = 0;
int weapon1bonus = 0;
char weapon2[] = "";
int weapon2dam = 0;
int weapon2bonus = 0;
char weapon3[] = "";
int weapon3dam = 0;
int weapon3bonus = 0;
int weapon4bonus = 0;

int choose_enemy(int enemy){
    switch(enemy) {
    case 1:
        strcpy(etype,"goblin");
        eac = 10;
        ehealth = 7;
        edamage = 6;
        break;
    case 2:
        strcpy(etype,"skeleton");
        eac = 13;
        ehealth = 13;
        edamage = 6;
        break;
    case 3:
        strcpy(etype,"orc");
        eac = 14;
        ehealth = 18;
        edamage = 8;
        break;
    case 4:
        strcpy(etype,"owlbear");
        eac = 15;
        ehealth = 32;
        edamage = 10;
        break;
    case 5:
        strcpy(etype,"beholder");
        eattacks = 10;
        eac = 15;
        ehealth = 57;
        edamage = 6;
        break;
    case 6:
        strcpy(etype,"kracken");
        eac = 20;
        ehealth = 100;
        edamage = 20;
        break;
    default:
        printf("ERROR: NO ENEMY TYPE GIVEN");

    return 0;
}

int do_maintinance() {
    if(id == 3){
        if (cloak_charges == 3){
            daggers += 4;
            cloak_charges = 0;
        }
        cloak_charges += 1;
        }
        printf("you have %d health left, %d daggers, and %d cloak charges\n", phealth, daggers, cloak_charges);
    }
    if(sheilded==1){
        sheilded = false;
        pac-=4;
    }
    if(phealth<0){
        alive = false;
    }
}

int pattack() {
        int attackused = 0;
        printf("type 1 to use %s\n", weapon1); // get the attack they're using
        printf("type 2 to use %s\n", weapon2);
        printf("type 3 to use %s\n", weapon3);
        scanf("%d", &attackused);
        switch(attackused){ // calculate damage and roll d20
            case 1:
                for(i=0; i<attacks1; i++) { // for dual weilding and extra attacks
                    hit = ((rand() % 20) + 1) + weapon1bonus;
                    pdamage = ((rand() % (weapon1dam - 1 + 1)) + 1);
                }
                break;
            case 2:
                for(i=0; i<attacks2; i++) {
                    hit = ((rand() % 20) + 1) + weapon2bonus;
                    pdamage = ((rand() % (weapon2dam - 1 + 1)) + 1);
                }
                break; 
            case 3:
                switch(id){
                    case 1:
                        printf("You raise your shield, increasing your AC by 4 for this turn\n");
                        pac += 4;
                        sheilded = true;
                        break;
                    case 2:
                        printf("Type one to increase your attack damage by 4, type two to increase your AC by 4, type three to heal for 6\n");
                        scanf("%d", &choice);
                        switch(choice) {
                            case 1:
                                weapon1bonus += 4;
                                weapon2bonus += 4;
                                weapon3bonus += 4;
                                break;
                            case 2:
                                pac += 4;
                                break;
                            case 3:
                                phealth += 6;
                                break;
                        }
                        break;
                    default:
                        printf("the cloak doesn't seem to do anything when you try to use it, but you feel like it might be doing something in the background\n");
                    }
                break;
        }
        if(daggers!= 0) {
            for(i=0; i<daggers; i++) {
                printf("Type one to throw a dagger, type anything else to end turn. You have %d daggers left\n", daggers);
                scanf("%d", &choice);
                if(choice == 1) {
                    hit = ((rand() % 20) + 1) + weapon4bonus;
                    pdamage = ((rand() % (4 - 1 + 1)) + 1);
                    daggers -= 1;
                }
            }
        }
        if(hit > eac) {
            // check if they hit
            ehealth -= pdamage;
            printf("You did %d damage, the enemy has %d health left\n", pdamage, ehealth);
        } else {
            printf("You missed!\n");
        }
        return 0;
    }

int eattack() { // automated enemy attack
    hit = ((rand() % 20) + 1);
    damage = ((rand() % (edamage - 1 + 1)) + 1);
    if(hit > pac) {
        phealth-= damage;
        printf("Enemy hit you for %d damage, you have %d health left\n", damage, phealth); 
    } else {
        printf("Enemy missed!\n");
    }
    return 0;
}

int main() {
    srand(time(NULL));
    
    // Character creation:
    printf("It's time to create your character!\n");
    printf("It's time to choose your first weapon\n");
    printf("press one to choose longsword\n");
    printf("press two to choose duel scimitars\n");
    printf("press three to choose shortsword and dagger\n");
    scanf("%d", &choice);
    switch(choice) {
        case 1:
            strcpy(weapon1, "longsword");
            weapon1dam = 10;
            printf("You have chosen a sword. You do more damage\n");
            break;
        case 2:
            strcpy(weapon1, "duel scimitars");
            attacks1 += 1;
            weapon1dam += 6;
            printf("You chose %s. You now have 2 melee attacks.\n", weapon1);
            break;
        case 3: {;
            strcpy(weapon1, "shortsword");
            printf("You have chosen a shortsword and four daggers. You can make any number of dagger attacks each turn\n");
            daggers += 4;
            weapon1dam = 6;
            break;
        }
    }
    
    printf("It's time to choose your second weapon\n");
    printf("press one to choose gauntlet. this works well with the longsword, and increases your damage\n");
    printf("press two to choose longbow. this works well with dual scimitars\n");
    printf("press three to choose shortbow and two daggers. these work well with the shortsword and daggers\n");
    scanf("%d", &choice);
    switch(choice) {
        case 1:
            strcpy(weapon2, "gauntlet(cannot attack)");
            weapon1dam += 4;
            if(strcmp(weapon1, "longsword") == 0) {
                weapon1bonus += 4;
                weapon1dam += 1;
            }
            break;
        case 2:
            strcpy(weapon2, "longbow");
            weapon2dam = 8;
            if(strcmp(weapon1, "duel scimitars") == 0) {
                weapon1bonus += 4;
                weapon2bonus += 2;
                weapon2dam += 2;
            }
            break;
        case 3:
            strcpy(weapon2, "shortbow");
            weapon2dam = 6;
            daggers += 2;
            if(strcmp(weapon1, "shortsword") == 0) {
                weapon1bonus += 4;
                weapon2bonus += 4;
                weapon2dam += 2;
            }
            break;
    }
    printf("Time to choose your third weapon");
    printf("thase will have abilities");
    printf("Press one to choore a shield, to increase your passive armour, and give you a temporary defense buff when used");
    printf("Press two to choose a magic potion, which can either buff your attack, your ac or heal you when used");
    printf("Press three to chose a magic cloak, which will give you four daggers once every three turns");
    scanf("%d", &choice);
    switch(choice) {
        case 1:
            strcpy(weapon3, "shield");
            pac += 2;
            id = 1;
            break;
        case 2:
            strcpy(weapon3, "magic potion");
            id = 2;
            break;
        case 3:
            strcpy(weapon3, "magic cloak");
            id = 3;
            break;
    printf("Choose what enemy to fight: 1 for goblin, 2 for skeleton, 3 for orc, 4 for owlbear, 5 for beholder, 6 for kraken.");
    scanf("%d",choice);
    choose_enemy(choice);
    printf("%s",etype);
    printf("%d",eac);
    printf("%d",edamage);
    printf("%d",ehealth);
    printf("%d",eattacks);
    }
    
    return 0;
}
