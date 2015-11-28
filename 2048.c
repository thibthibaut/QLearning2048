/*
 ============================================================================
 Name        : 2048.c
 Author      : Maurits van der Schee
 Description : Console version of the game "2048" for GNU/Linux
 ============================================================================
 */

#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>

#define SIZE 4
#define ETA 0.5
#define GAMA 0.5
#define QSIZE 10000


typedef struct qvalue_t{

	char* stateHash;    //String giving a unique id for a state
	double actions[4];//Q-values for a given state a stored in actions array

}qvalue_t;

uint32_t score=0;
uint8_t scheme=0;
uint32_t globalQvalueCounter=0;
uint32_t gameCounter=0;
uint32_t qsize = QSIZE;
qvalue_t* q;

void getColor(uint8_t value, char *color, size_t length) {
	uint8_t original[] = {8,255,1,255,2,255,3,255,4,255,5,255,6,255,7,255,9,0,10,0,11,0,12,0,13,0,14,0,255,0,255,0};
	uint8_t blackwhite[] = {232,255,234,255,236,255,238,255,240,255,242,255,244,255,246,0,248,0,249,0,250,0,251,0,252,0,253,0,254,0,255,0};
	uint8_t bluered[] = {235,255,63,255,57,255,93,255,129,255,165,255,201,255,200,255,199,255,198,255,197,255,196,255,196,255,196,255,196,255,196,255};
	uint8_t *schemes[] = {original,blackwhite,bluered};
	uint8_t *background = schemes[scheme]+0;
	uint8_t *foreground = schemes[scheme]+1;
	if (value > 0) while (value--) {
		if (background+2<schemes[scheme]+sizeof(original)) {
			background+=2;
			foreground+=2;
		}
	}
	snprintf(color,length,"\033[38;5;%d;48;5;%dm",*foreground,*background);
}

void drawBoard(uint8_t board[SIZE][SIZE]) {
	uint8_t x,y;
	char c;
	char color[40], reset[] = "\033[m";
	printf("\033[H");

	printf("2048.c %17d pts\n\n",score);

	for (y=0;y<SIZE;y++) {
		for (x=0;x<SIZE;x++) {
			getColor(board[x][y],color,40);
			printf("%s",color);
			printf("       ");
			printf("%s",reset);
		}
		printf("\n");
		for (x=0;x<SIZE;x++) {
			getColor(board[x][y],color,40);
			printf("%s",color);
			if (board[x][y]!=0) {
				char s[8];
				snprintf(s,8,"%u",(uint32_t)1<<board[x][y]);
				uint8_t t = 7-strlen(s);
				printf("%*s%s%*s",t-t/2,"",s,t/2,"");
			} else {
				printf("   ·   ");
			}
			printf("%s",reset);
		}
		printf("\n");
		for (x=0;x<SIZE;x++) {
			getColor(board[x][y],color,40);
			printf("%s",color);
			printf("       ");
			printf("%s",reset);
		}
		printf("\n");
	}
	printf("\n");
	printf("        ←,↑,→,↓ or q        \n");
	printf("\033[A"); // one line up
}

uint8_t findTarget(uint8_t array[SIZE],uint8_t x,uint8_t stop) {
	uint8_t t;
	// if the position is already on the first, don't evaluate
	if (x==0) {
		return x;
	}
	for(t=x-1;t>=0;t--) {
		if (array[t]!=0) {
			if (array[t]!=array[x]) {
				// merge is not possible, take next position
				return t+1;
			}
			return t;
		} else {
			// we should not slide further, return this one
			if (t==stop) {
				return t;
			}
		}
	}
	// we did not find a
	return x;
}

bool slideArray(uint8_t array[SIZE]) {
	bool success = false;
	uint8_t x,t,stop=0;

	for (x=0;x<SIZE;x++) {
		if (array[x]!=0) {
			t = findTarget(array,x,stop);
			// if target is not original position, then move or merge
			if (t!=x) {
				// if target is zero, this is a move
				if (array[t]==0) {
					array[t]=array[x];
				} else if (array[t]==array[x]) {
					// merge (increase power of two)
					array[t]++;
					// increase score
					score+=(uint32_t)1<<array[t];
					// set stop to avoid double merge
					stop = t+1;
				}
				array[x]=0;
				success = true;
			}
		}
	}
	return success;
}

void rotateBoard(uint8_t board[SIZE][SIZE]) {
	uint8_t i,j,n=SIZE;
	uint8_t tmp;
	for (i=0; i<n/2; i++) {
		for (j=i; j<n-i-1; j++) {
			tmp = board[i][j];
			board[i][j] = board[j][n-i-1];
			board[j][n-i-1] = board[n-i-1][n-j-1];
			board[n-i-1][n-j-1] = board[n-j-1][i];
			board[n-j-1][i] = tmp;
		}
	}
}

bool moveUp(uint8_t board[SIZE][SIZE]) {
	bool success = false;
	uint8_t x;
	for (x=0;x<SIZE;x++) {
		success |= slideArray(board[x]);
	}
	return success;
}

bool moveLeft(uint8_t board[SIZE][SIZE]) {
	bool success;
	rotateBoard(board);
	success = moveUp(board);
	rotateBoard(board);
	rotateBoard(board);
	rotateBoard(board);
	return success;
}

bool moveDown(uint8_t board[SIZE][SIZE]) {
	bool success;
	rotateBoard(board);
	rotateBoard(board);
	success = moveUp(board);
	rotateBoard(board);
	rotateBoard(board);
	return success;
}

bool moveRight(uint8_t board[SIZE][SIZE]) {
	bool success;
	rotateBoard(board);
	rotateBoard(board);
	rotateBoard(board);
	success = moveUp(board);
	rotateBoard(board);
	return success;
}

bool findPairDown(uint8_t board[SIZE][SIZE]) {
	bool success = false;
	uint8_t x,y;
	for (x=0;x<SIZE;x++) {
		for (y=0;y<SIZE-1;y++) {
			if (board[x][y]==board[x][y+1]) return true;
		}
	}
	return success;
}

uint8_t countEmpty(uint8_t board[SIZE][SIZE]) {
	uint8_t x,y;
	uint8_t count=0;
	for (x=0;x<SIZE;x++) {
		for (y=0;y<SIZE;y++) {
			if (board[x][y]==0) {
				count++;
			}
		}
	}
	return count;
}

bool gameEnded(uint8_t board[SIZE][SIZE]) {
	bool ended = true;
	if (countEmpty(board)>0) return false;
	if (findPairDown(board)) return false;
	rotateBoard(board);
	if (findPairDown(board)) ended = false;
	rotateBoard(board);
	rotateBoard(board);
	rotateBoard(board);
	return ended;
}

void addRandom(uint8_t board[SIZE][SIZE]) {
	static bool initialized = false;
	uint8_t x,y;
	uint8_t r,len=0;
	uint8_t n,list[SIZE*SIZE][2];

	if (!initialized) {
		srand(time(NULL));
		initialized = true;
	}

	for (x=0;x<SIZE;x++) {
		for (y=0;y<SIZE;y++) {
			if (board[x][y]==0) {
				list[len][0]=x;
				list[len][1]=y;
				len++;
			}
		}
	}

	if (len>0) {
		r = rand()%len;
		x = list[r][0];
		y = list[r][1];
		n = 1; //(rand()%10)/9+1; //SIMPLIFY THE GAME A LITTLE BIT
		board[x][y]=n;
	}
}

void initBoard(uint8_t board[SIZE][SIZE]) {
	uint8_t x,y;
	for (x=0;x<SIZE;x++) {
		for (y=0;y<SIZE;y++) {
			board[x][y]=0;
		}
	}
	addRandom(board);
	addRandom(board);
	//drawBoard(board);
	score = 0;
}

void setBufferedInput(bool enable) {
	static bool enabled = true;
	static struct termios old;
	struct termios new;

	if (enable && !enabled) {
		// restore the former settings
		tcsetattr(STDIN_FILENO,TCSANOW,&old);
		// set the new state
		enabled = true;
	} else if (!enable && enabled) {
		// get the terminal settings for standard input
		tcgetattr(STDIN_FILENO,&new);
		// we want to keep the old setting to restore them at the end
		old = new;
		// disable canonical mode (buffered i/o) and local echo
		new.c_lflag &=(~ICANON & ~ECHO);
		// set the new settings immediately
		tcsetattr(STDIN_FILENO,TCSANOW,&new);
		// set the new state
		enabled = false;
	}
}

int test() {
	uint8_t array[SIZE];
	// these are exponents with base 2 (1=2 2=4 3=8)
	uint8_t data[] = {
		0,0,0,1,	1,0,0,0,
		0,0,1,1,	2,0,0,0,
		0,1,0,1,	2,0,0,0,
		1,0,0,1,	2,0,0,0,
		1,0,1,0,	2,0,0,0,
		1,1,1,0,	2,1,0,0,
		1,0,1,1,	2,1,0,0,
		1,1,0,1,	2,1,0,0,
		1,1,1,1,	2,2,0,0,
		2,2,1,1,	3,2,0,0,
		1,1,2,2,	2,3,0,0,
		3,0,1,1,	3,2,0,0,
		2,0,1,1,	2,2,0,0
	};
	uint8_t *in,*out;
	uint8_t t,tests;
	uint8_t i;
	bool success = true;

	tests = (sizeof(data)/sizeof(data[0]))/(2*SIZE);
	for (t=0;t<tests;t++) {
		in = data+t*2*SIZE;
		out = in + SIZE;
		for (i=0;i<SIZE;i++) {
			array[i] = in[i];
		}
		slideArray(array);
		for (i=0;i<SIZE;i++) {
			if (array[i] != out[i]) {
				success = false;
			}
		}
		if (success==false) {
			for (i=0;i<SIZE;i++) {
				printf("%d ",in[i]);
			}
			printf("=> ");
			for (i=0;i<SIZE;i++) {
				printf("%d ",array[i]);
			}
			printf("expected ");
			for (i=0;i<SIZE;i++) {
				printf("%d ",in[i]);
			}
			printf("=> ");
			for (i=0;i<SIZE;i++) {
				printf("%d ",out[i]);
			}
			printf("\n");
			break;
		}
	}
	if (success) {
		printf("All %u tests executed successfully\n",tests);
	}
	return !success;
}

void signal_callback_handler(int signum) {
	printf("         TERMINATED         \n");
	setBufferedInput(true);
	printf("\033[?25h\033[m");
	exit(signum);
}



//Just move !
bool move(int direction, uint8_t board[SIZE][SIZE]){

	switch (direction) {
		case 0:
			return moveUp(board);
		case 1:
			return moveRight(board);
		case 2:
			return moveDown(board);
		case 3:
			return moveLeft(board);
		default:
			return false;
	}
}

/* This function return a string corresponding to a unique identifier for a state */
char* computerStateHash(uint8_t board[SIZE][SIZE]){

	uint8_t c = 0;
	char* to_return = (char*) malloc(17*sizeof(char));

	for(int i=0; i<SIZE; i++){
		for(int j=0; j<SIZE; j++){
			to_return[c] = 'a' + (char)board[i][j];
			c++;
		}
	}
to_return[c] = '\0';
return to_return;
}

/* Returns a pointer on the Q-value for a given state hash */
qvalue_t* getQvalue(char* state, qvalue_t* q){

	for(int i=0; i<globalQvalueCounter; i++){

		if( strcmp(state, q[i].stateHash ) == 0  ){
			return &(q[i]); //Not sure...
		}
	}

return NULL;
}

/* GET MAX */
uint32_t getMax( qvalue_t* qvalue ){

	uint32_t max = 0;

	for(int i=0; i<4; i++){

		if(qvalue->actions[i] > max)
			max = qvalue->actions[i];
	}

	return max;
}

void updateQ(char* state, uint8_t action, uint32_t maxQnext){

	//Let's see if this Q value already exists
	for(int i=0; i<globalQvalueCounter; i++){

		if( strcmp(state, q[i].stateHash ) == 0  ){
			q[i].actions[action] = q[i].actions[action] + ETA*( score + GAMA*maxQnext - q[i].actions[action] );
			return;
		}
	}

	//If we haven't return yet then we need to create a new Q-value for this state
	q[globalQvalueCounter].stateHash = state;
	q[globalQvalueCounter].actions[action] = q[globalQvalueCounter].actions[action] + ETA*( score + GAMA*maxQnext - q[globalQvalueCounter].actions[action] );
	globalQvalueCounter++;
	//Maybe we need to reallocate memory
	if(globalQvalueCounter == qsize - 1){
		qsize *= 2;
		q = (qvalue_t*) realloc(q, qsize*sizeof(qvalue_t) );

		for(int i=globalQvalueCounter; i>qsize; i++){
			qvalue_t qval;
			qval.stateHash = "xxxxxxxxxxxxxxxx";
			qval.actions[0] = 0.0;
			qval.actions[1] = 0.0;
			qval.actions[2] = 0.0;
			qval.actions[3] = 0.0;
			q[i] = qval;
		}


		printf("\a");
	}

}

int main(int argc, char *argv[]) {
	uint8_t board[SIZE][SIZE];
	char c;
	bool success;
	int action;
	char* state;
	char* nextstate; //current state, nextstate
	qvalue_t* nextQvalue;
	uint32_t maxQnext; //Max Q value for next state

	time_t startT, stopT;

	//Debug
	uint32_t stepsCounter = 0;

	//Q is an array of qvalues
	q = (qvalue_t*) malloc(qsize*sizeof(qvalue_t));

	for(int i=0; i>qsize; i++){
		qvalue_t qval;
		qval.stateHash = "xxxxxxxxxxxxxxxx";
		qval.actions[0] = 0.0;
		qval.actions[1] = 0.0;
		qval.actions[2] = 0.0;
		qval.actions[3] = 0.0;
		q[i] = qval;
	}

	scheme = 2;

	if (argc == 2 && strcmp(argv[1],"test")==0) {
		return test();
	}
	if (argc == 2 && strcmp(argv[1],"blackwhite")==0) {
		scheme = 1;
	}
	if (argc == 2 && strcmp(argv[1],"ugly")==0) {
		scheme = 0;
	}

	printf("\033[?25l\033[2J");

	// register signal handler for when ctrl-c is pressed
	signal(SIGINT, signal_callback_handler);

	initBoard(board);
	setBufferedInput(false);

	startT = time(NULL);

	/*START MAIN LOOP */
	while (true) {

/*	if(gameCounter == 1000){
		stopT = time(NULL);
		printf("\n\n\n TIME: %lf",difftime(stopT,startT) );
	}*/

	/*	c=getchar();
		switch(c) {
			case 97:	// 'a' key
			case 104:	// 'h' key
			case 68:	// left arrow
				success = moveLeft(board);  break;
			case 100:	// 'd' key
			case 108:	// 'l' key
			case 67:	// right arrow
				success = moveRight(board); break;
			case 119:	// 'w' key
			case 107:	// 'k' key
			case 65:	// up arrow
				success = moveUp(board);    break;
			case 115:	// 's' key
			case 106:	// 'j' key
			case 66:	// down arrow
				success = moveDown(board);  break;
			default: success = false;
		}*/

		/* QLEARNING ALGORITHM TAKES PLACE HERE */

		//Set this state as the current state (Update current state)
		state = computerStateHash(board);

		//Take a random move
		action = rand()%4;
		success = move( action , board );
		stepsCounter++;

		nextstate = computerStateHash(board); //We are now in 'nextstate'

		nextQvalue = getQvalue(nextstate, q); //See if Q(nextstate) exists
		nextQvalue = NULL;
		if( nextQvalue == NULL ){
			maxQnext = 0;
		}
		else{
				maxQnext = getMax( nextQvalue ); //Find greatest Q-value for nextstate
				//printf("nextQvalue DOES exist !!\n");
		}

		updateQ(state, action, maxQnext);

		if (success) {
			//drawBoard(board);
			//usleep(1500);
			addRandom(board);
			//drawBoard(board);

			//Debug
			if( stepsCounter%100 == 0){
			printf("\n\n\n");
			printf("state hash: %s\n", state );
			printf("number of new q-values: %d\n", globalQvalueCounter );
			printf("number of new games: %d\n", gameCounter );
			printf("number of moves: %d\n", stepsCounter );
			printf("first q value. state hash %s, %f - %f - %f - %f\n", q[0].stateHash, q[0].actions[0], q[0].actions[1], q[0].actions[2], q[0].actions[3] );
			drawBoard(board);
			}

			if (gameEnded(board)) {
				//printf("         GAME OVER          \n");
				gameCounter++;
				//usleep(150000);
				//TODO: Save score
				initBoard(board);
				//drawBoard(board);
			}
		}


	/*	if (c=='q') {
			printf("        QUIT? (y/n)         \n");
			c=getchar();
			if (c=='y') {
				break;
			}
			drawBoard(board);
		}
		if (c=='r') {
			printf("       RESTART? (y/n)       \n");
			c=getchar();
			if (c=='y') {
				initBoard(board);
			}
			drawBoard(board);
		} */


	}//End of while


	setBufferedInput(true);

	printf("\033[?25h\033[m");

	return EXIT_SUCCESS;
}
