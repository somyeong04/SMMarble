//
//  main.c
//  SMMarble
//
//  Created by Juyeop Kim on 2023/11/05.
//

#include <time.h>
#include <string.h>
#include <windows.h>
#include "smm_object.h"
#include "smm_database.h"
#include "smm_common.h"

#define BOARDFILEPATH "marbleBoardConfig.txt"
#define FOODFILEPATH "marbleFoodConfig.txt"
#define FESTFILEPATH "marbleFestivalConfig.txt"



//board configuration parameters
static int board_nr;
static int food_nr;
static int festival_nr;

static int player_nr;

static int end = 0; //게임 종료를 나타내는 변수 
static int i;


typedef struct player {
        int energy;
        int position;
        char name[MAX_CHARNAME];
        int accumCredit;
        int flag_graduate;
        int num_lecture;
        int temp;
        int tmp;
} player_t;

static player_t *cur_player;
//static player_t cur_player[MAX_PLAYER];

#if 0
static int player_energy[MAX_PLAYER];
static int player_position[MAX_PLAYER];
static char player_name[MAX_PLAYER][MAX_CHARNAME];
#endif

//function prototypes
int isGraduated(void); //check if any player is graduated
 //print grade history of the player
void goForward(int player, int step); //make player go "step" steps on the board (check if player is graduated)
void printPlayerStatus(void); //print all player status at the beginning of each turn
float calcAverageGrade(int player); //calculate average grade of the player
void* findGrade(int player, char *lectureName); //find the grade from the player's grade history
void printGrades(int player); //print all the grade history of the player


void printGrades(int player) // 플레이어의 성적 출력하는 함수 
{
    void *gradePtr;
    if(cur_player[player].num_lecture==0)
    {
    	printf("Printing player %s's grade (average %f) ::::\n",cur_player[player].name,0);
	}
	else
	{
		printf("Printing player %s's grade (average %f) ::::\n", cur_player[player].name, calcAverageGrade(player));
	    for (i=0;i<smmdb_len(LISTNO_OFFSET_GRADE + player);i++)
	    {
	        gradePtr = smmdb_getData(LISTNO_OFFSET_GRADE + player, i);
	        printf("%s (credit:%d) : %s\n", smmObj_getNodeName(gradePtr), smmObj_getNodeCredit(gradePtr), smmObj_getNodeGradeType(gradePtr));
	    }
	}
}

void printPlayerStatus(void) // 플레이어의 상태 출력하는 함수 
{
     void* boardPtr;
     int type;
     printf("\n\n==================================== PLAYER STATUS ====================================\n");
     for (i=0;i<player_nr;i++)
     {
         boardPtr = smmdb_getData(LISTNO_NODE, cur_player[i].position);
         type = smmObj_getNodeType(boardPtr);
         if (type == SMMNODE_TYPE_LABORATORY && cur_player[i].tmp == 1)
         {
             printf("%s at %d.%s(exp.), credit: %d, energy: %d\n"
                 , cur_player[i].name
                 , cur_player[i].position
                 , smmObj_getNodeName(boardPtr)
                 , cur_player[i].accumCredit
                 , cur_player[i].energy);
         }
         else
         {
             printf("%s at %d.%s, credit: %d, energy: %d\n"
                 , cur_player[i].name
                 , cur_player[i].position
                 , smmObj_getNodeName(boardPtr)
                 , cur_player[i].accumCredit
                 , cur_player[i].energy);
         }
     }
     printf("=======================================================================================\n\n");
}

void generatePlayers(int n, int initEnergy) //generate a new player
{
     //n time loop
     for (i=0;i<n;i++)
     {
         //input name
         printf("Input player %i's name: ", i); //안내 문구 
         scanf("%s", cur_player[i].name);
         fflush(stdin);
         
         //set position
         //player_position[i] = 0;
         cur_player[i].position = 0;
         
         //set energy
         //player_energy[i] = initEnergy;
         cur_player[i].energy = initEnergy;
         cur_player[i].accumCredit = 0;
         cur_player[i].flag_graduate = 0;
         cur_player[i].num_lecture = 0;
         cur_player[i].temp=0;
         cur_player[i].tmp=0;
     }
}


int rolldie(int player) // 주사위를 굴려 나온 결과를 반환하는 함수 
{
    char c;
    printf("Press any key to roll a die (press g to see grade): ");
    c = getchar();
    fflush(stdin);
    
#if 1
    if (c == 'g') // 사용자가 g 입력 시  플레이어의 성적 출력 
        printGrades(player);
        c = 0;
#endif
    
    return (rand()%MAX_DIE + 1);
}

float calcAverageGrade(int player) //calculate average grade of the player
{
    float sum = 0;
    int cnt = 0;
    for (i = 0; i < smmdb_len(LISTNO_OFFSET_GRADE + player); i++)
    {
        void* gradePtr = smmdb_getData(LISTNO_OFFSET_GRADE + player, i);
        switch (smmObj_getNodeGrade(gradePtr))
        {
            case 0:
                sum += 4.3;
                break;
            case 1:
                sum += 4.0;
                break;
            case 2:
                sum += 3.7;
                break;
            case 3:
                sum += 3.3;
                break;
            case 4:
                sum += 3.0;
                break;
            case 5:
                sum += 2.7;
                break;
            case 6:
                sum += 2.3;
                break;
            case 7:
                sum += 2.0;
                break;
            case 8:
                sum += 1.7;
                break;
        }
        cnt++;
    }
    return (sum / cnt);
}

//action code when a player stays at a node
int actionNode(int player)
{
	if(cur_player[player].flag_graduate==1 && cur_player[player].position==0)
	{
		return 0;
	}
    void *boardPtr = smmdb_getData(LISTNO_NODE, cur_player[player].position );
    //int type = smmObj_getNodeType( cur_player[player].position );
    int type = smmObj_getNodeType( boardPtr );
    char *name = smmObj_getNodeName( boardPtr );
    void *gradePtr;
    char choice[10];
    static int threshold;
    int p_di;
    int cant = 0;
    srand(time(NULL));
    switch(type)
    {
        //case lecture: 강의 참여 여부를 물어봄 
        case SMMNODE_TYPE_LECTURE: 
            while (1)
            {
               printf("Lecture %s (credit:%d, energy:%d) starts! are you going to join? or drop? :"
                    , smmObj_getNodeName(boardPtr)
                    , smmObj_getNodeCredit(boardPtr)
                    , smmObj_getNodeEnergy(boardPtr));
                scanf("%s", choice);
                if ((!strcmp(choice, "join")) || (!strcmp(choice, "drop")))
                {
                    break;
                }
                printf("Invalid input! input \"drop\" or \"join\"!\n");
            }
            if (choice[0] == 'j') // 강의에 참여하는 경우 
            {
                if (cur_player[player].energy >= smmObj_getNodeEnergy(boardPtr))
                {
                    for (i = 0; i < smmdb_len(LISTNO_OFFSET_GRADE + player); i++)
                    {
                    	if(cur_player[player].num_lecture == 0)
                    	{
                    		break;
						}
                        gradePtr = smmdb_getData(LISTNO_OFFSET_GRADE + player, i);
                        if (!strcmp(smmObj_getNodeName(boardPtr),smmObj_getNodeName(gradePtr)))
                        {
                            cant = 1;
                            break;
                        }
                    }
                    if (cant == 0) // 강의를 처음 수강하는 경우 
                    {
                        cur_player[player].accumCredit += smmObj_getNodeCredit(boardPtr);
                        if (cur_player[player].accumCredit >= GRADUATE_CREDIT)
                        {
                            cur_player[player].flag_graduate = 1;
                        }
                        cur_player[player].energy -= smmObj_getNodeEnergy(boardPtr);
                        gradePtr = smmObj_genObject(smmObj_getNodeName(boardPtr), smmObjType_grade, 0, smmObj_getNodeCredit(boardPtr), 0, (rand() % 8));
                        smmdb_addTail(LISTNO_OFFSET_GRADE + player, gradePtr);
                        printf("%s successfully takes the lecture %s with grade %s (average : %f, remained energy : %d)\n"
                            , cur_player[player].name
                            , smmObj_getNodeName(gradePtr)
                            , smmObj_getNodeGradeType(gradePtr)
                            , calcAverageGrade(player)
                            , cur_player[player].energy);
                        cur_player[player].num_lecture++;
                    }
                    else if (cant == 1) // 강의를 수강한 적 있는 경우 
                    {
                        printf("%s has already taken the lecture %s!!", cur_player[player].name, smmObj_getNodeName(boardPtr));
                        cant = 0;
                    }
                }
                else // 에너지가 부족한 경우 
                {
                    printf("%s is too hungry to take the lecture %s (remained:%d, required:%d)\n"
                        , cur_player[player].name
                        , smmObj_getNodeName(boardPtr)
                        , cur_player[player].energy
                        , smmObj_getNodeEnergy(boardPtr));
                }
            }
            else if(choice[0]=='d') // 강의를 드랍하는 경우 
            {
                printf("Player %s drops the lecture %s!\n", cur_player[player].name, smmObj_getNodeName(boardPtr));
            }
            fflush(stdin);
            break;
            
		//case Restaurant
        case SMMNODE_TYPE_RESTAURANT:
            cur_player[player].energy += smmObj_getNodeEnergy(boardPtr);
            printf("Let\'s eat in %s and charge %d energies (remained energy : %d)\n"
                , smmObj_getNodeName(boardPtr)
                , smmObj_getNodeEnergy(boardPtr)
                , cur_player[player].energy);
            break;
            
		//case Laboratory 
        case SMMNODE_TYPE_LABORATORY:
            if (cur_player[player].temp == 0) // 실험실 시간이 아닌 경우 
            {
                printf("This is not experiment time. You can go through this lab.\n");
            }
            else if (cur_player[player].temp == 1) // 실험실 시간인 경우 
            {
            	if(cur_player[player].tmp == 0)
            	{
            		threshold = rand() % MAX_DIE + 1;
            		cur_player[player].tmp = 1;
				}
                printf("Experiment time! Let's see if you can satisfy professor (threshold: %d)\n", threshold);
                p_di = rolldie(player);
                cur_player[player].energy-=smmObj_getNodeEnergy(boardPtr);
                
                if (p_di >= threshold) // 실험 성공 
                {
                    printf("Experiment result : %d, success! %s can exit this lab!\n", p_di, cur_player[player].name);
                    cur_player[player].temp = 0;
                    cur_player[player].tmp = 0;
                }
                else // 실험 실패 
                {
                    printf("Experiment result : %d, fail T_T. %s needs more experiment......\n", p_di, cur_player[player].name);
                }
            }
            break;

		//case Home
        case SMMNODE_TYPE_HOME:
            cur_player[player].energy += smmObj_getNodeEnergy(boardPtr);
            printf("Returned to HOME! energy charged by %d (total : %d)\n", smmObj_getNodeEnergy(boardPtr), cur_player[player].energy);
            break;
        
        //case Go to Lab
        case SMMNODE_TYPE_GOTOLAB:
            for (i = 0; i < smmdb_len(LISTNO_NODE); i++)
            {
                boardPtr = smmdb_getData(LISTNO_NODE, i);
                if (smmObj_getNodeType(boardPtr) == SMMNODE_TYPE_LABORATORY)
                {
                    cur_player[player].position = i;
                    break;
                }
            }
            printf("OMG! This is experiment time!! Player %s goes to the lab.\n", cur_player[player].name);
            cur_player[player].temp = 1;
            break;
        
        //case FoodChance
        case SMMNODE_TYPE_FOODCHANCE:
            printf("%s gets a food chance! press any key to pick a food card:", cur_player[player].name);
            char c = getchar();
            fflush(stdin);
            p_di = rand() % smmdb_len(LISTNO_FOODCARD);
            void* foodPtr = smmdb_getData(LISTNO_FOODCARD, p_di);
            cur_player[player].energy += smmObj_getNodeEnergy(foodPtr);
            printf("%s picks %s and charges %d (remained energy : %d)\n"
                , cur_player[player].name
                , smmObj_getNodeName(foodPtr), smmObj_getNodeEnergy(foodPtr), cur_player[player].energy);
            break;
        
        //case Festival
        case SMMNODE_TYPE_FESTIVAL:
            printf("%s participates to Snow Festival! press any key to pick a festival card:", cur_player[player].name);
            char c_f = getchar();
            fflush(stdin);
            p_di = rand() % smmdb_len(LISTNO_FESTCARD);
            void* festPtr = smmdb_getData(LISTNO_FESTCARD, p_di);
            printf("MISSION: %s !!\n(Press any key when mission is ended.)", smmObj_getNodeName(festPtr));
            c = getchar();
            break;
    }
}

void end_print(int player) //플레이어의 성적 히스토리를 출력하는 함수 
{
    void* gradePtr;
    printf("History of %s\n", cur_player[player].name);
    Sleep(1000);
    for (i = 0; i < smmdb_len(LISTNO_OFFSET_GRADE + player); i++)
    {
        gradePtr = smmdb_getData(LISTNO_OFFSET_GRADE + player, i);
        printf("%d. %s, credit: %d, grade: %s\n"
            , i, smmObj_getNodeName(gradePtr)
            , smmObj_getNodeCredit(gradePtr)
            , smmObj_getNodeGradeType(gradePtr));
        Sleep(1000);
    }
}

void goForward(int player, int step) // 플레이어를 주어진 스텝만큼 전진시키는 함수 
{
    void *boardPtr;
    printf("-> Result : %d\n", step); // 플레이어가 이동할 거리 출력 
    cur_player[player].position += step; 
    if (cur_player[player].position >= smmdb_len(LISTNO_NODE))
    {
        if (cur_player[player].flag_graduate == 1) // 졸업 조건 만족 & 홈 노드 O
        {
            end = 1;
            printf("%s has achieved the target credit and arrived at home!!\n\n\n",cur_player[player].name);
            end_print(player);
            cur_player[player].position = 0;
        }
        else if (cur_player[player].flag_graduate == 0 && cur_player[player].position != smmdb_len(LISTNO_NODE)) // 졸업 조건 만족 X & 홈 노드 X 
        {
            cur_player[player].energy += 18;
            printf("Returned to HOME! energy charged by 18 (total : %d)\n", cur_player[player].energy);
            cur_player[player].position -= smmdb_len(LISTNO_NODE);
        }
        else if (cur_player[player].flag_graduate == 0 && cur_player[player].position == smmdb_len(LISTNO_NODE)) // 졸업 조건 만족  X & 홈 노드 O
        {
        	cur_player[player].position -= smmdb_len(LISTNO_NODE);
		}
    }
    boardPtr = smmdb_getData(LISTNO_NODE, cur_player[player].position );
    if((cur_player[player].flag_graduate == 1 && cur_player[player].position!=0)||cur_player[player].flag_graduate==0)
    {
        for (i = 0; i < step; i++) // 플레이어가 해당 턴에 지나온 노드 차례대로 출력 
		{
            Sleep(300);
            int pos = (cur_player[player].position - step + i + smmdb_len(LISTNO_NODE)) % smmdb_len(LISTNO_NODE);
            printf("=> Jump to %s\n", smmObj_getNodeName(smmdb_getData(LISTNO_NODE, pos)));
        }
        
        // 플레이어가 해당 턴에 최종적으로 도착한 노드 출력 
        Sleep(300);
		printf("-> %s go to node %i (name: %s)\n\n", 
	    cur_player[player].name, cur_player[player].position,
	    smmObj_getNodeName(boardPtr));
	}
}

int main(int argc, const char * argv[]) {
    
    FILE* fp;
    char name[MAX_CHARNAME];
    int type;
    int credit;
    int energy;
    int initEnergy;
    int turn=0;
    
    board_nr = 0;
    food_nr = 0;
    festival_nr = 0;
    
    srand(time(NULL));
    
    
    //1. import parameters ---------------------------------------------------------------------------------
    //1-1. boardConfig 
    if ((fp = fopen(BOARDFILEPATH,"r")) == NULL)
    {
        printf("[ERROR] failed to open %s. This file should be in the same directory of SMMarble.exe.\n", BOARDFILEPATH);
        getchar();
        return -1;
    }
    
    printf("Reading board component......\n");
    while ( fscanf(fp, "%s %d %d %d", name, &type, &credit, &energy) == 4 ) //read a node parameter set
    {
        //store the parameter set
        //(char* name, smmObjType_e objType, int type, int credit, int energy, smmObjGrade_e grade)
        void *boardObj = smmObj_genObject(name, smmObjType_board, type, credit, energy, 0);
        smmdb_addTail(LISTNO_NODE, boardObj);
        void *testptr=smmdb_getData(LISTNO_NODE, board_nr);
        
        if (type == SMMNODE_TYPE_HOME)
           initEnergy = energy;
        board_nr++;
    }

    fclose(fp);
    for (i = 0;i<board_nr;i++)
    {
        void *boardObj = smmdb_getData(LISTNO_NODE, i);
        
        printf("node %i : %s, %i(%s), credit %i, energy %i\n", 
                     i, smmObj_getNodeName(boardObj), 
                     smmObj_getNodeType(boardObj), smmObj_getTypeName(smmObj_getNodeType(boardObj)),
                     smmObj_getNodeCredit(boardObj), smmObj_getNodeEnergy(boardObj));
    }
    printf("Total number of board nodes : %i\n", board_nr);
    //printf("(%s)", smmObj_getTypeName(SMMNODE_TYPE_LECTURE));
    
    strcpy(name, "");
    type = -1;
    credit = -1;
    energy = -1;
    
    //1-2. food card config 
    if ((fp = fopen(FOODFILEPATH,"r")) == NULL)
    {
        printf("[ERROR] failed to open %s. This file should be in the same directory of SMMarble.exe.\n", FOODFILEPATH);
        return -1;
    }
    
    printf("\n\nReading food card component......\n");
    while (fscanf(fp, "%s %i", name, &energy) == 2) //read a food parameter set
    {
        void* foodObj = smmObj_genObject(name, smmObjType_card, type, credit, energy, 0);
        smmdb_addTail(LISTNO_FOODCARD, foodObj);
        food_nr++;
    }
    fclose(fp);
    for (i = 0; i < food_nr; i++)
    {
        void* foodObj = smmdb_getData(LISTNO_FOODCARD, i);

        printf("Food card %i : %s, energy %i\n",
            i, smmObj_getNodeName(foodObj),
            smmObj_getNodeEnergy(foodObj));
    }
    
	printf("Total number of food cards : %i\n", food_nr);

    strcpy(name, "");
    type = -1;
    credit = -1;
    energy = -1;
    
    //1-3. festival card config 
    if ((fp = fopen(FESTFILEPATH,"r")) == NULL)
    {
        printf("[ERROR] failed to open %s. This file should be in the same directory of SMMarble.exe.\n", FESTFILEPATH);
        return -1;
    }
    
    printf("\n\nReading festival card component......\n");
    while (fscanf(fp, "%s", name) == 1) //read a festival card string
    {
        void* festivalObj = smmObj_genObject(name, smmObjType_card, type, credit, energy, 0);
        smmdb_addTail(LISTNO_FESTCARD, festivalObj);
        festival_nr++;
    }
    fclose(fp);
    for (i = 0; i < festival_nr; i++)
    {
        void* festivalObj = smmdb_getData(LISTNO_FESTCARD, i);

        printf("Festival card %i : %s\n",
            i, smmObj_getNodeName(festivalObj));
    }
    printf("Total number of festival cards : %i\n\n", festival_nr);

    printf("=======================================================================================\n");
    printf("---------------------------------------------------------------------------------------\n");
    printf("                Sookmyung Marble !! Let's Graduate (total credit : %d)!!\n", GRADUATE_CREDIT);
    printf("---------------------------------------------------------------------------------------\n");
    printf("=======================================================================================\n");


    //2. Player configuration ---------------------------------------------------------------------------------
    do
    {
        //input player number to player_nr
        printf("\nInput player no.: ");
        scanf("%d", &player_nr);
        fflush(stdin);
    }
    while (player_nr < 0 || player_nr >  MAX_PLAYER);
    
    cur_player = (player_t*)malloc(player_nr*sizeof(player_t));
    generatePlayers(player_nr, initEnergy);
    
    
    //3. SM Marble game starts ---------------------------------------------------------------------------------
    while (end == 0) //is anybody graduated?
    {
        int die_result;
    
    
    //4. Game Play ---------------------------------------------------------------------------------------------
        //4-1. initial printing
        printPlayerStatus();
        printf("\nThis is %s\'s turn :::: ",cur_player[turn].name);
        //4-2. die rolling (if not in experiment)
		if(cur_player[turn].temp==0)
		{
			die_result = rolldie(turn);
			goForward(turn, die_result);
		}        
        
        //4-3. go forward
        

		//4-4. take action at the destination node of the board
        actionNode(turn);
        
        //4-5. next turn
        turn = (turn + 1)%player_nr;
    }
    
    
    free(cur_player);
    free_M(board_nr,food_nr,festival_nr);
	
    system("PAUSE");
    return 0;
}
