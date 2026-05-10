#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <bool.h>

//declaring mischallanous variables
int edamage = 0;
int hit = 0;
int choice = 0; // used to record choices in character creation

// declaring weapon modifiers e.g. duel weapons
int attacks1 = 1
int attacks2 = 1
int attacks3 = 1
int attacks4 = 0
int daggers = 0

// declaring player stats and setting them to 0
int phealth = 1;
int pstr = 3;
int pac = 10; // armour class
int pdamage = 0;// this will hold the damage when they attack
int pdex = 3

// declaring enemy stats

// declaring weapons used in advance
char weapon1[] = "";
int weapon1dam = 0;
int weapon1bonus = 0;
char weapon2[] = "";
int weapon1dam = 0;
int weapon1bonus = 0;
char weapon3[] = "";
int weapon1dam = 0;
int weapon1bonus = 0;

int main(){
    }
    srand(time(NULL));
    int pattack(){
        int attackused = 0;
        printf("type 1 to use %s/n", weapon1); // get the attack they're using
        printf("type 2 to use %s/n", weapon2);
        printf("type 3 to use %s/n", weapon3);
        scanf("%d",attackused);
        switch(attackused){ // calculate damage and roll d20
            case 1:
                for(i=0,i<attacks1,i++) // for dual weilding and extra attacks
                    int hit =((rand() % 20) + 1) + weapon1bonus;
                    int pdamage = ((rand() % ( weapon1dam- 1 + 1)) + 1);
                break;
            case 2:
                for(i=0,i<attacks2,i++)
                    int hit =((rand() % 20) + 1) + weapon2bonus;
                    int pdamage = ((rand() % (weapon2dam - 1 + 1)) + 1);
                break; 
            case 3:
                for(i=0,i<attacks3,i++)
                    int hit =((rand() % 20) + 1) + weapon3bonus;
                    int pdamage = ((rand() % (weapon3dam- 1 + 1)) + 1);
                break;
        if attacks4 != 0:
            for(i=0,i>daggers,i++)
                printf("Type one to throw a dagger, type anything else to end turn. You have %d daggers left/n", &daggers)
                scanf("%d",choice)
                if choice==1:
                    int hit =((rand() % 20) + 1) + weapon4bonus;
                    int pdamage = ((rand() % (4 - 1 + 1)) + 1);
                    int daggers-=1
        return pdamage
        }
        if hit>eac: // chech if they hit
            ehealth-=pdamage;
            printf("You did %d damage, the enemy has %d health left/n", &pdamage, &ehealth);
        else:
            printf("You missed!/n");
        return 0
    }
    int eattack(){// enemy attack
        printf("The enemy is attacking/n")
    }
    // Character creation:
    printf("It's time to create your character!/n")
    printf("It's time to choose your first weapon/n")
    printf("press one to choose longsword/n")
    printf("press two to choose duel scimatars/n")
    printf("press three to choose shortsword and dagger/n")
    scanf("%d",choice)
    switch(choice){
        case 1:
            char weapon1[] = "longsword"
            int weapon1dam=10
            printf("You have choosen a sword. You do more damage/n")
        case 2:
            char weapon1[] = "duel scimitars"
            int attacks1 +=1
            int weapon1dam+=6
            print("You chose %s. You now have 2 melee attacks./n", &weapon1)
        case 3:
            char weapon1[] = "shortsword"
            daggers+=4
            weapon1dam = 6
            attacks4+=1
            ptintf("you have chosen a shortsword and four daggers. You can make any number of dagger attacks each turn/n")
    printf("It's time to choose your second weapon/n")
    printf("press one to choose gauntlet. this works well with the longsword, and increases your damage/n")
    printf("press two to choose longbow. this works well with dual scimitars")
    printf("press three to choose shortbow and two daggers. these work well with the shortsword and daggers")
    scanf("%d", choice)
    switch(choice){
        case 1:
            char weapon2[] = "gauntlet(cannot attack)"
            int weapon1dam+=4
            if weapon1{}=="longsword"
                int weapon1bonus=4
                int weapon2bonus=4
                int weapon1dam+=1
        case 2:
            char weapon2[] = "longbow"

        case 3:
    }
    }
}


