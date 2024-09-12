#include <curses.h>
#include <stdlib.h>
#include <pthread.h>

#define UP  	 1
#define DOWN 	-1
#define LEFT  	 2
#define RIGHT  	-2

struct Snake
{
	int row;
	int column;
	struct Snake *next;
};

struct Snake *head = NULL;
struct Snake *tail = NULL;
int key;
int direction;
int score;
int gameCnt = 1;
int totalScore;
struct Snake food;

void initCurse()
{
	initscr();
	keypad(stdscr, 1);
	noecho();
}

int traversalSnakeFood(int i, int j)
{
	if (food.row == i && food.column == j){
		return 1;	
	}

	return 0;
}

int traversalSnakeNode(int i, int j)
{
	struct Snake *p;
	p = head;
	while (p != NULL){
		if (p->row == i && p->column == j){
			return 1;
		}
		p = p->next;
	}
	
	return 0;
}

void gameMap()
{
	int i, j;
	int row = 20;
	int column = 20;

	move(0, 0);
	
	for (i=0; i<row; i++){
		if (i == 0){
			for (j=0; j<column; j++){
				printw("--");
			}
			printw("\n");	
		}
		if (i>=0 && i<=row-1){
			for (j=0; j<column+1; j++){
				if (j == 0 || j == column){
					 printw("|");
				}
				else if (traversalSnakeFood(i, j)){
					printw("$$");
				}
				else if (traversalSnakeNode(i, j)){
					printw("[]");
				}
				else{
					printw("  ");
				}
			}
			printw("\n");
		}
		if (i == row-1){
			for (j=0; j<column; j++){
                                printw("--");
                        }
                        printw("\n");
			printw("Your current score is: %d \n", score);
			printw("Your game count: %d \n", gameCnt);
			printw("Your total score is: %d \n", totalScore);
			printw("By NUIST WF\n");
			printw("Your control direction: ");
		}
	}
}

void addSnakeTailNode()
{
	struct Snake *new;
	new = (struct Snake*)malloc(sizeof(struct Snake));

	switch (direction){
		case UP:
			new->row = tail->row - 1;
			new->column = tail->column;
			break;
		case DOWN:
			new->row = tail->row + 1;
			new->column = tail->column;
			break;
		case LEFT:
			new->row = tail->row;
			new->column = tail->column - 1;
			break;
		case RIGHT:
			new->row = tail->row;
			new->column = tail->column + 1;
			break;
	}
	new->next = NULL;

	tail->next = new;
	tail = new;	
}

void deleteSnakeHeadNode()
{
	struct Snake *p;
	p = head;

	head = head->next;

	free(p);
}

void initSnakeFood()
{
	int x = rand()%20;
	int y = rand()%20;

	food.row = x;
	food.column = y;
}

void initSnakeNode()
{
	direction = RIGHT;

	initSnakeFood();	

	struct Snake *p;
	while (head != NULL){
		p = head;
		head = head->next;
		free(p);
	}		
	
	head = (struct Snake*)malloc(sizeof(struct Snake));
	
	head->row = 1;
	head->column = 1;
	head->next = NULL;
	
	tail = head;
	
	addSnakeTailNode();
	addSnakeTailNode();
}

int snakeDie()
{
	struct Snake *p;	
	p = head;
	while (p->next != NULL){
		if ((tail->row == p->row) && (tail->column == p->column)){
			return 1;
		}
		p = p->next;
	}
	
	if (tail->row < 0 || tail->row == 20 || tail->column == 0 || tail->column == 20){
		return 1;
	}

	return 0;
}

void moveSnake()
{	
	addSnakeTailNode();

	if ((tail->row == food.row) && (tail->column == food.column)){
		score++;
		totalScore++;
		initSnakeFood();
	}
	else{
		deleteSnakeHeadNode();
	}
	
	if (snakeDie()){
		initSnakeNode();
		gameCnt++;
		score = 0;	
	}
}

void turn(int tempdirection)
{
	if (abs(direction) != abs(tempdirection)){
		direction = tempdirection;
	}
}

void* directionMove()
{
	while (1){
		key = getch();
		switch (key){
			case KEY_UP:
				turn(UP);
				printw("UP   ");
				break;
			case KEY_DOWN:
				turn(DOWN);
				printw("DOWN ");
				break;
			case KEY_LEFT:
				turn(LEFT);
				printw("LEFT ");
				break;
			case KEY_RIGHT:
				turn(RIGHT);
				printw("RIGHT");
				break;
		}
	}
}

void* refreshInterface()
{
	while (1){
		moveSnake();
		gameMap();
		refresh();
		usleep(200000);
	}
}

int main()
{
	pthread_t mythread1, mythread2;

	initCurse();
	gameMap();
	initSnakeNode();

	pthread_create(&mythread1, NULL, directionMove, NULL);
	pthread_create(&mythread2, NULL, refreshInterface, NULL);
	
	while(1);
	endwin();
	return 0;
}
